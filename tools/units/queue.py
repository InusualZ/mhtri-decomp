"""Hand a pooled brief to a worker: claim the unit, promote the brief, print the spawn line.

`brief.py --pool` prepares the brief for every registered unit that has no bodies yet (owner's ask,
2026-09-23: "prepare briefs in advance and queue new work right away"), so the orchestrator can start a
worker the instant a slot frees without deriving anything. This is the other half:

    python tools/units/queue.py next [--count N] [--worker NAME] [--dry-run] [--json]
    python tools/units/queue.py list [--json]
    python tools/units/queue.py --selftest

`next` picks the pooled unit with the lowest `.text` address that is still **unclaimed**, takes the claim
(`claims.py claim` creates the worktree and the branch), renders the brief **from the current queue entry or
`splits.txt` range**, writes it to the claim's own slug path, and prints the exact spawn line - cwd, name and
task text - to paste. The brief is never copied from the pool: `brief.py --pool` skips a brief that already
exists, so a queue regeneration can leave every pooled file describing the old range, and copying one handed
a worker the wrong scope (`proposal/80119DEC`, 2026-09-25). The pool still decides *which* unit is next; it
is not the source of the worker's brief.

`next --count N` claims a **wave** of N: a stride of N through the address-ordered queue, never N
neighbours. Adjacency is the vector for almost every clash this campaign has had - the two halves of one
translation unit are two adjacent proposals (`proposal/8007270C`+`proposal/80073180`, both
`g3d_calcvtx.cpp`), a rule-2 boundary artefact appears when a neighbour registers a symbol you declare, and
neighbouring units share owner headers and types by construction - so a wave takes proposals `i, i+N,
i+2N, ...` instead. That is a **guarantee**, not a probability: two adjacent proposals can share a wave only
if both indices are congruent mod N, which is impossible for N > 1. Random sampling would still put both
halves of one TU in a wave about once in N tries. The stride is taken in the queue's own address order and
never re-sorted, so a wave is spread across the address bands for free; the cost is cross-unit knowledge
reuse (adjacent proposals tend to share a TU, a header, a type), so a wave is spread *within* a band rather
than scattered for its own sake.

A pooled brief is claim-independent by construction: it is rendered against the worktree the claim *will*
create (`claims.worktree_for`) and against the branch `claims.py claim` *will* make (`worker/<slug(unit)>`),
so `--pool` can prepare it before a claim exists. The claim path re-renders rather than copies (see
`promote`), so the brief a worker gets always matches the entry the queue holds at claim time, including a
manually named branch whose slug (and therefore outbox path) is its own. The unit is claimed **before** the
brief is written, so a worker never gets a brief whose outbox does not exist.

`list` shows the pool's state: briefs written, ready (unclaimed, no bodies), claimed (in flight), written (a
unit that has gained a body - `brief.py --pool` prunes those), covered (its range is registered already,
under whatever name - never handed out) and stale (no longer registered), plus the next few ready
candidates in address order.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))

from units import brief  # noqa: E402

# A unit that reconstructs a whole 100+-function range legitimately outruns the 30-minute
# single-run backstop: three rounds died mid-work on 2026-09-24 and one lost an uncommitted
# registration.  The spawn line carries the budget explicitly so it cannot silently regress, and
# the runner steers a checkpoint 5 minutes before it (see the pi-subagents config).
TIMEOUT_MS = 5400000
from units import claims  # noqa: E402
from units import recompile as rc  # noqa: E402

POOL_DEPTH = 5  # how many ready candidates `list` prints


def pool_dir(main: str) -> str:
    return brief.pool_dir(main)


def pool_entries(main: str) -> list[dict]:
    """Every brief in the pool, as `{slug, path, unit}` - `unit` is `None` for an unparseable file."""
    out = []
    d = pool_dir(main)
    if not os.path.isdir(d):
        return out
    for name in sorted(os.listdir(d)):
        if not name.endswith(".md"):
            continue
        path = os.path.join(d, name)
        out.append({"slug": name[:-3], "path": path, "unit": brief.brief_unit(path)})
    return out


def registered_norm(main: str) -> set[str]:
    return {claims.norm_unit(u) for u in brief.registered_units(main)}


def registered_text_ranges(main: str) -> list[tuple[int, int]]:
    """Every registered unit's `.text` block from `splits.txt`, as half-open `(start, end)` pairs."""
    out = []
    for unit in brief.registered_units(main):
        rng = brief.splits_range(main, unit)
        if ".text" in rng:
            start, end = rng[".text"][0], rng[".text"][1]   # splits_range may carry a third field
            out.append((start, end))
    return out


def ranges_cover(ranges: list[tuple[int, int]], start: int) -> bool:
    """Whether `start` falls inside any half-open `(start, end)` range."""
    return any(s <= start < e for s, e in ranges)


def covered_by_registered(main: str, unit: str) -> bool:
    """Whether a proposal's range already belongs to a registered translation unit.

    The proposal the pool holds registers under whatever unit **name** its worker chose, so the pool's
    "has no body yet" test cannot see that the range is gone - `800A99B4` and `800B99E8` were each handed
    out again after landing, under different names, which costs a whole round.  The addresses are the
    honest test: a proposal whose `.text` start sits inside a registered unit's `.text` is finished work.
    """
    p = brief.proposal_by_label(main, unit)
    if not p:
        return False
    text = p.get("text") or []
    if not text:
        return False
    return ranges_cover(registered_text_ranges(main), text[0])


def is_proposal(main: str, unit: str) -> bool:
    """Whether `unit` is a queue *label* rather than a registered unit path (option A)."""
    want = claims.norm_unit(unit)
    return any(claims.norm_unit(l) == want for l in brief.proposal_labels(main))


def state(main: str, entry: dict, branches: set[str] | None = None) -> str:
    """Where a pooled brief stands: `claimed`, `written`, `stale`, `unreadable` or `ready`.

    Only `ready` may be handed out. A claimed unit is in flight (its brief has already been promoted), a
    `written` unit has gained a body (the pool will prune it), and `stale` is no longer registered - so none
    of them is offered as new work.

    A claim is read the way `claims.claim` refuses one: its registry record **or its claim branch** (the lock,
    which survives a lost registry record - a half-torn-down release, or two claims racing on `save_registry`).
    Reading only the registry is what re-offered a claimed proposal on 2026-09-24 and made `queue.py next`
    refuse at the branch. `branches` is the batch of live `worker/` branches from `claims.worker_branches`,
    passed by the selectors; `None` asks for it once.

    A **proposal** (option A) is the simpler case and takes a different route: it has no source and no
    registration to check, so while it is in the queue it is work to hand out, and once the queue drops it -
    which happens when `attribute.py queue` is re-run after its range was registered - it is stale.  Until
    the queue is re-run, a proposal whose range is *already* registered under another name is `covered`.
    """
    unit = entry.get("unit")
    if not unit:
        return "unreadable"
    if brief.claim_for(main, unit) or claims.lock_held(main, unit, branches):
        return "claimed"
    if is_proposal(main, unit):
        return "covered" if covered_by_registered(main, unit) else "ready"
    if claims.norm_unit(unit) not in registered_norm(main):
        return "stale"
    path = os.path.join(main, "src", *brief.source_name(unit, main).split("/"))
    return "written" if brief.has_bodies(path) else "ready"


def text_start(main: str, unit: str) -> int | None:
    """The unit's `.text` start - from the proposal queue for a proposal, else from `splits.txt`."""
    p = brief.proposal_by_label(main, unit)
    if p:
        return p["text"][0]
    rng = brief.splits_range(main, unit)
    return rng[".text"][0] if ".text" in rng else None


def order_key(main: str, entry: dict) -> tuple:
    """The campaign's order: lowest `.text` address first, then unit name."""
    start = text_start(main, entry["unit"])
    return (start is None, start or 0, entry["unit"] or "")


def ordered_entries(main: str, entries: list[dict] | None = None) -> list[dict]:
    """**Every** pooled brief in address order - the queue as `next` walks it, unready entries included.

    The unready ones stay in the list on purpose: a wave strides over this order and skips what is not
    ready *without breaking the stride*, and two adjacent proposals must be adjacent here for that skip to
    mean anything.
    """
    entries = entries if entries is not None else pool_entries(main)
    return sorted(entries, key=lambda e: order_key(main, e))


def next_entry(main: str, entries: list[dict] | None = None) -> dict | None:
    """The next ready brief: lowest `.text` address first, then unit name (the campaign's order)."""
    ordered = ordered_entries(main, entries)
    branches = claims.worker_branches(main)
    ready = [e for e in ordered if state(main, e, branches) == "ready"]
    return ready[0] if ready else None


# --------------------------------------------------------------------------------------------------
# wave spread: SYSTEM (the module of the nearest registered unit), then address distance
# --------------------------------------------------------------------------------------------------
_SYSTEM_HINTS: dict[str, tuple[tuple[int, int], list[tuple[int, int, str]]]] = {}


def system_hints(main: str) -> list[tuple[int, int, str]]:
    """`(start, end, module)` for every registered `.text` range - the wave picker's system map.

    The module is the first path component of the unit's name (`enemy/fn_801478FC.cpp` -> `enemy`), which is
    what the linking band and a brief's class-3 evidence agree on.  It spreads a wave; it never decides a
    unit's home.  The cache is keyed on the file's stat, not just its path: a run that starts before the map
    exists must not keep answering "no systems" after it appears (the selftest's fixture does exactly that).
    """
    path = os.path.join(main, "config", "RMHE08", "splits.txt")
    try:
        st = os.stat(path)
    except OSError:
        return []
    key = (st.st_mtime_ns, st.st_size)
    cached = _SYSTEM_HINTS.get(main)
    if cached and cached[0] == key:
        return cached[1]
    rows: list[tuple[int, int, str]] = []
    text = open(path, encoding="utf-8").read()
    unit = None
    for line in text.replace('\r\n', '\n').split('\n'):
        if line and not line[0].isspace() and line.rstrip().endswith(':'):
            unit = line.rstrip()[:-1]
        m = re.match(r"\s+\.text\s+start:0x([0-9A-Fa-f]+) end:0x([0-9A-Fa-f]+)", line)
        if m and unit:
            rows.append((int(m.group(1), 16), int(m.group(2), 16),
                         unit.split('/')[0] if '/' in unit else unit))
    rows.sort()
    _SYSTEM_HINTS[main] = (key, rows)
    return rows


def hint_for(main: str, address: int) -> str:
    """The system a proposal's address sits in: the module of the nearest registered range ('?' if none)."""
    best = None
    for start, end, module in system_hints(main):
        d = 0 if start <= address < end else min(abs(address - start), abs(address - end))
        if best is None or d < best[0]:
            best = (d, module)
    return best[1] if best else '?'


def entry_address(main: str, entry: dict) -> int:
    """The `.text` address the queue orders by - from the pooled name (which always embeds it)."""
    name = str(entry.get('unit') or entry.get('label') or '')
    m = re.search(r'(?<![0-9A-Fa-f])([0-9A-Fa-f]{8})(?![0-9A-Fa-f])', name)
    if m:
        return int(m.group(1), 16)
    try:
        key = order_key(main, entry)
        return int(key[0]) if isinstance(key, (tuple, list)) and key else int(key)
    except Exception:
        return 0


def spread_picks(main: str, ordered: list[dict], is_ready: list[bool], count: int) -> list[int]:
    """`count` ready indices: SYSTEM diversity first, address distance second, never index-adjacent.

    Greedy and deterministic.  Start at the lowest ready address, then repeatedly take the candidate from a
    system no pick has used, preferring the farthest; when every system is used, the farthest overall.  A
    candidate index-adjacent to a pick is never considered (the two halves of one TU), so the wave comes back
    SHORT when adjacency blocks it - the caller reports the shortfall instead of relaxing into the defect.
    """
    ready = [i for i, ok in enumerate(is_ready) if ok]
    if not ready:
        return []
    addr = {i: entry_address(main, ordered[i]) for i in ready}
    hint = {i: hint_for(main, addr[i]) for i in ready}
    picks = [ready[0]]
    used = {hint[ready[0]]}
    while len(picks) < count:
        best, best_score = None, None
        for i in ready:
            if i in picks or any(abs(i - j) == 1 for j in picks):
                continue
            score = (1 if hint[i] not in used else 0, min(abs(addr[i] - addr[j]) for j in picks))
            if best_score is None or score > best_score:
                best, best_score = i, score
        if best is None:
            break
        picks.append(best)
        used.add(hint[best])
    return sorted(picks)


def wave(main: str, count: int, entries: list[dict] | None = None) -> list[dict]:
    """The `count` proposals one `--count N` wave claims: spread by SYSTEM, then by address distance.

    A wave used to take indices `0, N, 2N, ...` of the address-ordered queue - a COUNT-based stride, so a
    dense band filled it: six claims all landed in the `enemy` band, every brief edited the same `_ENEMY_WORK`
    record, and every landing needed a hand merge.  The picker now takes `spread_picks`: SYSTEM first (the
    module of the nearest registered unit, `hint_for`), address distance second, and never two
    index-adjacent proposals - adjacent ones are often the two halves of one TU, which is the guard the old
    stride had and this keeps as a HARD filter.  A wave therefore comes back SHORT when adjacency blocks it,
    and the caller reports the shortfall rather than relaxing into the defect.  `count == 1` is `next_entry`'s
    single pick unchanged.  Measured on the live pool, a 6-wave claims six different systems (`enemy`, `Pl`,
    `ai`, `hud`, `Runtime.PPCEABI.H`, `RSO`) instead of six units from one band.
    """
    if count < 1:
        raise SystemExit("REFUSED queue next | --count must be at least 1")
    ordered = ordered_entries(main, entries)
    branches = claims.worker_branches(main)
    is_ready = [state(main, e, branches) == "ready" for e in ordered]
    picked = spread_picks(main, ordered, is_ready, count) if count > 1 else \
        [next(i for i, ok in enumerate(is_ready) if ok)]
    return [ordered[i] for i in picked]


def spawn_line(main: str, unit: str, slug: str, wt: str, brief_path: str,
               profile: str = "decompiler") -> dict:
    """The paste-ready spawn: agent, cwd and task text for the orchestrator.

    `profile` is the agent profile the lane is launched with. A registration-and-reconstruction lane is
    unit work, which is the project's `decompiler` profile (`.agents/agents/decompiler.md`); `worker` stays
    the generic fallback and `fixer`/`merger` are for a refused gate and a refused apply.

    The task names the brief by its absolute MAIN path: the brief is written into MAIN *after* the worktree
    was created, so the worktree's own checkout does not contain it. The call is the default `subagent`
    tool's single mode (`agent`/`task`/`cwd`); it has no `name` parameter, so the slug is a label only, and
    it blocks and returns the worker's final message to the orchestrator when the child process exits.
    """
    task = ("Read %s (in MAIN) and do exactly what it says. "
            "Ack first: python tools/units/claims.py ack %s --agent %s-%s. "
            "You may fan out subagents. End your turn with your report: "
            "your final message is the result the orchestrator receives."
            % (brief_path.replace("\\", "/"), unit, profile, slug))
    return {"agent": profile, "name": "%s-%s" % (profile, slug), "cwd": wt, "task": task,
            "call": "subagent(agent=\"%s\", cwd=\"%s\", task=%s, timeoutMs=%d)"
                    % (profile, wt.replace("\\", "/"), json.dumps(task), TIMEOUT_MS)}


def promote(main: str, unit: str, claim_slug: str) -> str:
    """Render the brief for `unit` at the claim's slug path, **from the current entry**.

    The pool is a scheduling device, not the source of truth: `brief.py --pool` used to skip a brief that
    already existed, so `attribute.py queue` re-cutting a range left every pre-existing pooled brief
    describing the OLD scope, and copying that file handed the worker the wrong work - `proposal/80119DEC`
    got `.text 0x80119DEC..0x8011A34C` (two functions) while the queue had `..0x8011D448` (thirty-seven).
    The brief is therefore re-rendered here against the current queue entry (or `splits.txt` range) and the
    real claim, never copied, so a stale pooled file cannot reach a worker. The pool brief is still what
    *selects* the unit; it is just not what the worker is handed.
    """
    dest = os.path.join(main, "tools", "units", "briefs", claim_slug + ".md")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    b, text = brief.brief_for(main, claims.worktree_for(unit, main), unit, None)
    with open(dest, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    return dest


def branch_error(main: str) -> str | None:
    """Refuse to claim from a HEAD that is not `main`.

    The claim is rooted at MAIN's HEAD (`git worktree add -b <branch> <wt> <HEAD>`), so a claim made while
    MAIN sits on another branch roots the worker on that branch's tip instead of main's - the same
    2026-09-24 incident that moved 14 landings onto `tools/stylelint-rule2-unsplit`. Checked before the
    worktree exists, so a refusal leaves nothing behind. `None` when git cannot be asked (the selftests run
    in temp dirs that are not repositories).
    """
    p = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=main,
                       capture_output=True, text=True, errors="replace")
    branch = p.stdout.strip()
    if p.returncode != 0 or not branch:
        return None
    if branch != "main":
        return ("HEAD is on %r, not main: `git checkout main` first - a claim is rooted at MAIN's HEAD, "
                "so one made off main roots the worker on the wrong base" % branch)
    return None


def strictly_newer(main_lines, branch_lines):
    """How many lines the branch has that main's copy lacks, or None when the two have diverged.

    Pure, so the selftest can exercise the rule without a repository. A branch is *strictly newer* only when
    main's line set is a subset of the branch's: the branch lost nothing and gained something, so the file
    carries work that was never landed. A diverged file (both sides have lines the other lacks) returns
    None - that is the stale-or-superseded class the 2026-09-26 audit found on every branch left over from
    an already-landed unit (older pad names, older comment wording), and refusing a spawn on it would stop
    production for nothing.
    """
    m, b = set(main_lines), set(branch_lines)
    if m - b:
        return None
    return len(b - m) or None


def _file_lines(main: str, ref: str, path: str) -> list[str]:
    """One file's lines at `ref`, or [] when that ref has no such file (a branch's new file)."""
    p = subprocess.run(["git", "show", "%s:%s" % (ref, path)], cwd=main,
                       capture_output=True, text=True, errors="replace")
    return p.stdout.splitlines() if p.returncode == 0 else []


def unlanded_branches(main: str, ignore: set[str] | None = None) -> list[tuple[str, list[tuple[str, int]]]]:
    """Local branches whose content is strictly newer than main's, with the files and line counts.

    This is the guard the owner asked for on 2026-09-26: finished work must not sit on a branch while a new
    unit is started. It is content-based, not commit-based - the landing recipe cherry-picks *content*, so
    every worker branch stays ahead of main by commits after its unit lands, and a commit-count test would
    refuse every spawn forever. `[]` when git cannot be asked (the selftests run outside a repository).
    """
    ignore = set(ignore or ())
    p = subprocess.run(["git", "for-each-ref", "--format=%(refname:short)", "refs/heads"], cwd=main,
                       capture_output=True, text=True, errors="replace")
    if p.returncode != 0:
        return []
    out = []
    for branch in sorted(b.strip() for b in p.stdout.splitlines()):
        if not branch or branch == "main" or branch in ignore:
            continue
        d = subprocess.run(["git", "diff", "--name-only", "main...%s" % branch], cwd=main,
                           capture_output=True, text=True, errors="replace")
        if d.returncode != 0:
            continue
        hits = []
        for path in [ln for ln in d.stdout.splitlines() if ln.strip()]:
            n = strictly_newer(_file_lines(main, "main", path), _file_lines(main, branch, path))
            if n:
                hits.append((path, n))
        if hits:
            out.append((branch, hits))
    return out


def unlanded_error(main: str, allow: set[str] | None = None) -> str | None:
    """The refusal a spawn raises while any branch still holds work main does not have. None when clear.

    Deliberately narrow: only a *strictly newer* file counts (see `strictly_newer`), so this blocks on real
    unlanded work - as `worker/802e4978-...` was, a whole registered unit that no landing had ever taken -
    and not on the stale wording every landed branch leaves behind.
    """
    hits = unlanded_branches(main, allow)
    if not hits:
        return None
    lines = ["%d branch(es) hold work main does not have - land them (or delete them once the audit "
             "proves them stale) before claiming another unit:" % len(hits)]
    for branch, files in hits:
        shown = ", ".join("%s +%d lines" % (f, n) for f, n in files[:4])
        more = "" if len(files) <= 4 else " (+%d more file(s))" % (len(files) - 4)
        lines.append("  %s: %s%s" % (branch, shown, more))
    lines.append("  remedy: land it (docs/plan.md 12's recipe), or `git worktree remove <wt> && git branch -D "
                 "<branch>` once its delta is proven stale; `--allow-unlanded <branch>` parks one on purpose")
    return "\n".join(lines)


def no_ready(main: str) -> str:
    """The refusal both entry points raise when the queue has nothing to hand out."""
    return ("no ready brief in %s\n"
            "  run `python tools/units/brief.py --pool` first, or every pooled unit is claimed or written\n"
            "  see: python tools/units/queue.py list" % pool_dir(main))


def claim_entry(main: str, entry: dict, worker: str | None, dry_run: bool, claim_fn,
                profile: str = "decompiler") -> dict:
    """Claim one *selected* entry, promote its brief, and return its spawn.

    `next` and a wave differ only in selection, so this is the one claim path both take: the unit is
    claimed first and the brief is only promoted once the claim exists, so a worker is never handed an
    outbox path that does not exist. `claim_fn` is injectable so the selftest can exercise the whole flow
    without a git worktree.
    """
    unit = entry["unit"]
    slug = claims.slug(unit)
    wt = claims.worktree_for(unit, main)
    info = {"unit": unit, "branch": claims.branch_for(unit), "worktree": wt, "dry_run": True} \
        if dry_run else claim_fn(unit, main, worker, False)
    claim_slug = claims.claim_slug(main, unit) or slug
    if dry_run:
        brief_path = os.path.join(main, "tools", "units", "briefs", claim_slug + ".md")
    else:
        brief_path = promote(main, unit, claim_slug)
    return {"unit": unit, "slug": slug, "claim_slug": claim_slug, "worktree": wt, "brief": brief_path,
            "pool_brief": entry["path"], "claim": info, "dry_run": dry_run,
            "spawn": spawn_line(main, unit, claim_slug, wt, brief_path, profile)}


def next_brief(main: str, worker: str | None, dry_run: bool, claim_fn=None,
               profile: str = "decompiler", allow_unlanded=None) -> dict:
    """Claim the next ready unit, promote its brief, and return the spawn.

    Before the claim, `branch_error` refuses a MAIN whose HEAD is not `main`, because the worktree and
    branch are cut from that HEAD, and `unlanded_error` refuses while any branch still holds work main does
    not have (the owner's 2026-09-26 guard: do not start a unit while a finished one sits unlanded). The
    claim itself is `claim_entry`, shared with the `--count` wave path.
    """
    claim_fn = claim_fn or claims.claim
    if not dry_run:
        # the worktree/branch is cut from MAIN's HEAD, so a claim made on another branch is rooted wrong
        bad_branch = branch_error(main)
        if bad_branch:
            raise SystemExit("REFUSED queue next | %s" % bad_branch)
        blocked = unlanded_error(main, set(allow_unlanded or ()))
        if blocked:
            raise SystemExit("REFUSED queue next | %s" % blocked)
    entry = next_entry(main)
    if entry is None:
        raise SystemExit(no_ready(main))
    return claim_entry(main, entry, worker, dry_run, claim_fn, profile)


def next_briefs(main: str, worker: str | None, dry_run: bool, count: int, claim_fn=None,
                profile: str = "decompiler", allow_unlanded=None) -> dict:
    """Claim a wave of up to `count` spread proposals and return their spawns, in address order.

    Selection is `wave()` - a stride of `count` - and every claim goes through `claim_entry`, the same path
    the single pick uses. The same two guards as `next_brief` run first: HEAD must be `main`, and no branch
    may hold unlanded work. `claimed` below `requested` means the queue could not fill the wave: fewer than
    `count` were ready, or the rest were adjacent to a claim already in it. The wave claims what there is
    instead of failing, and the caller reports the shortfall.
    """
    claim_fn = claim_fn or claims.claim
    if not dry_run:
        bad_branch = branch_error(main)
        if bad_branch:
            raise SystemExit("REFUSED queue next | %s" % bad_branch)
        blocked = unlanded_error(main, set(allow_unlanded or ()))
        if blocked:
            raise SystemExit("REFUSED queue next | %s" % blocked)
    chosen = wave(main, count)
    if not chosen:
        raise SystemExit(no_ready(main))
    out = [claim_entry(main, entry, worker, dry_run, claim_fn, profile) for entry in chosen]
    return {"requested": count, "claimed": len(out), "shortfall": count - len(out), "dry_run": dry_run,
            "claims": out}


def pool_state(main: str) -> dict:
    """The pool's counts by state, plus the ready candidates in order."""
    entries = pool_entries(main)
    branches = claims.worker_branches(main)
    counts: dict[str, int] = {}
    ready = []
    for entry in entries:
        st = state(main, entry, branches)
        counts[st] = counts.get(st, 0) + 1
        if st == "ready":
            ready.append(entry)
    ready.sort(key=lambda e: (text_start(main, e["unit"]) is None, text_start(main, e["unit"]) or 0,
                              e["unit"] or ""))
    return {"dir": pool_dir(main), "entries": entries, "counts": counts, "ready": ready}


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    import tempfile
    # the unlanded-branch guard's rule, tested without a repository: only a strict superset blocks
    check("strictly_newer: identical", strictly_newer(["a", "b"], ["a", "b"]), None)
    check("strictly_newer: branch gained two", strictly_newer(["a"], ["a", "b", "c"]), 2)
    check("strictly_newer: new file", strictly_newer([], ["x", "y"]), 2)
    check("strictly_newer: diverged (stale pads)", strictly_newer(["a", "new"], ["a", "old"]), None)
    check("strictly_newer: main is newer", strictly_newer(["a", "b"], ["a"]), None)
    check("strictly_newer: empty branch", strictly_newer(["a"], []), None)
    with tempfile.TemporaryDirectory() as tmp:
        check("unlanded_branches outside a repo", unlanded_branches(tmp), [])
        check("unlanded_error outside a repo", unlanded_error(tmp), None)
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src", "auto"))
        # two stubs at different .text addresses, and one that already has a body
        for name in ("stubA", "stubB", "done"):
            open(os.path.join(tmp, "src", "auto", name + ".c"), "w").write("/* header only */\n")
        open(os.path.join(tmp, "src", "auto", "done.c"), "w").write("int f(void) { return 1; }\n")
        open(os.path.join(tmp, "configure.py"), "w").write(
            'config.libs = [\n    {\n        "lib": "auto",\n        "objects": [\n'
            '            Object(NonMatching, "auto/stubA.c"),\n'
            '            Object(NonMatching, "auto/stubB.c"),\n'
            '            Object(NonMatching, "auto/done.c"),\n'
            "        ],\n    },\n]\n")
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w").write(
            "auto/stubA.c:\n\t.text       start:0x80200000 end:0x80200100\n\n"
            "auto/stubB.c:\n\t.text       start:0x80100000 end:0x80100100\n")
        open(os.path.join(tmp, "config", "RMHE08", "symbols.txt"), "w").write(
            "fn_80100000 = .text:0x80100000; // type:function size:0x40\n"
            "fn_80200000 = .text:0x80200000; // type:function size:0x40\n")

        out = brief.pool(tmp)
        check("the pool wrote the two stubs only", sorted(out["wrote"]), ["auto/stubA", "auto/stubB"])
        entries = pool_entries(tmp)
        check("pool_entries finds both", len(entries), 2)
        check("a pooled brief is parseable", {e["unit"] for e in entries}, {"auto/stubA", "auto/stubB"})
        by_unit = {e["unit"]: e for e in entries}
        check("an unclaimed no-body stub is ready", state(tmp, by_unit["auto/stubA"]), "ready")
        check("the next ready brief is the lowest .text address",
              next_entry(tmp)["unit"], "auto/stubB")
        check("text_start reads the split range", text_start(tmp, "auto/stubA"), 0x80200000)

        # a claim (the registry, no git) takes the unit out of the ready set
        claims.save_registry(tmp, {"auto/stubB": {"branch": claims.branch_for("auto/stubB"),
                                                  "worktree": claims.worktree_for("auto/stubB", tmp)}})
        check("a claimed unit is not ready", state(tmp, by_unit["auto/stubB"]), "claimed")
        check("the next ready brief skips the claimed one", next_entry(tmp)["unit"], "auto/stubA")
        check("pool_state counts the claim", pool_state(tmp)["counts"].get("claimed"), 1)
        claims.save_registry(tmp, {})

        # a unit that gains a body is `written`, not ready
        open(os.path.join(tmp, "src", "auto", "stubA.c"), "w").write("int g(void) { return 2; }\n")
        check("a unit with a body is written", state(tmp, by_unit["auto/stubA"]), "written")
        open(os.path.join(tmp, "src", "auto", "stubA.c"), "w").write("/* header only */\n")

        # dry-run claims nothing, promotes nothing and still prints the spawn
        dry = next_brief(tmp, "w-demo", dry_run=True)
        check("dry-run claims nothing", claims.load_registry(tmp), {})
        check("dry-run leaves the pool brief in place", os.path.exists(by_unit["auto/stubB"]["path"]), True)
        check("dry-run picks the lowest address", dry["unit"], "auto/stubB")
        check("dry-run's spawn names the profile and the slug",
              dry["spawn"]["name"], "decompiler-" + claims.slug("auto/stubB"))
        check("dry-run's spawn has the worktree as cwd", dry["spawn"]["cwd"], claims.worktree_for("auto/stubB", tmp))
        check("dry-run's spawn task names the brief", dry["brief"].replace("\\", "/") in dry["spawn"]["task"], True)
        check("dry-run's spawn task names the unit", "auto/stubB" in dry["spawn"]["task"], True)
        check("dry-run's spawn is a subagent call",
              dry["spawn"]["call"].startswith("subagent(agent=\"decompiler\""), True)
        check("... and a proposal lane defaults to the decompiler profile",
              dry["spawn"]["agent"] == "decompiler", True)
        check("... which the orchestrator can override (a refused gate is a fixer lane)",
              spawn_line(tmp, "auto/x", "s", "/w", "/b", "fixer")["call"]
              .startswith("subagent(agent=\"fixer\""), True)
        check("the spawn omits the `name` the default tool has no parameter for",
              "name=" not in dry["spawn"]["call"], True)
        check("the spawn asks for a final-message handoff, not a tool",
              "final message" in dry["spawn"]["task"] and "subagent_done" not in dry["spawn"]["task"], True)

        # the real flow with an injected claim (no git worktree): promote, then remove the pool brief
        def fake_claim(unit, main, worker, dry_run):
            claims.save_registry(main, {unit: {"branch": claims.branch_for(unit),
                                               "worktree": claims.worktree_for(unit, main)}})
            return {"unit": unit, "branch": claims.branch_for(unit),
                    "worktree": claims.worktree_for(unit, main)}

        pooled_text = open(by_unit["auto/stubB"]["path"], encoding="utf-8").read()
        real = next_brief(tmp, "w-demo", dry_run=False, claim_fn=fake_claim)
        check("the real flow claims the unit", "auto/stubB" in claims.load_registry(tmp), True)
        check("the promoted brief is at the claim's slug path",
              os.path.basename(real["brief"]), claims.claim_slug(tmp, "auto/stubB") + ".md")
        check("the promoted brief exists", os.path.exists(real["brief"]), True)
        promoted_text = open(real["brief"], encoding="utf-8").read()
        check("the promoted brief is re-rendered, not a copy of the pooled file",
              promoted_text != pooled_text and "Pooled brief" not in promoted_text, True)
        check("... and it states the unit's current range", "0x80100000" in promoted_text, True)
        check("the pooled brief is left in place for `--pool` to refresh",
              os.path.exists(by_unit["auto/stubB"]["path"]), True)
        check("a claimed unit is still counted by pool_state", pool_state(tmp)["counts"].get("claimed"), 1)
        check("the promoted brief keeps the claim's outbox", "outbox" in promoted_text, True)
        claims.save_registry(tmp, {})

        # a branch named with a suffix is re-rendered too, so its outbox path is the claim's own
        claims.save_registry(tmp, {"auto/stubA": {"branch": claims.branch_for("auto/stubA") + "-zz",
                                                  "worktree": claims.worktree_for("auto/stubA", tmp)}})
        suffixed = promote(tmp, "auto/stubA", claims.slug("auto/stubA") + "-zz")
        check("a suffixed claim's brief is re-rendered", "-zz.json" in open(suffixed, encoding="utf-8").read(), True)
        claims.save_registry(tmp, {})

        # a brief whose unit is no longer registered is stale, never ready
        open(os.path.join(tmp, "src", "auto", "ghost.c"), "w").write("/* header only */\n")
        open(os.path.join(tmp, "tools", "units", "briefs", "pool", claims.slug("auto/ghost") + ".md"), "w").write(
            "# Brief: auto/ghost\n")
        ghost = [e for e in pool_entries(tmp) if e["unit"] == "auto/ghost"][0]
        check("an unregistered brief is stale", state(tmp, ghost), "stale")
        check("the stale brief is skipped", next_entry(tmp)["unit"], "auto/stubB")
        check("an unparseable pool file is unreadable",
              state(tmp, {"unit": None, "path": "x"}), "unreadable")

        check("spawn_line's call is one line", "\n" not in spawn_line(tmp, "auto/x", "s", "/w", "/b")["call"], True)

    # option A: a proposal is ready while it is in the queue, and stale once the queue drops it (which is
    # what happens when its range is registered and `attribute.py queue` is re-run)
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src"))
        os.makedirs(os.path.join(tmp, "tools", "units", "briefs", "pool"))
        open(os.path.join(tmp, "configure.py"), "w").write("config.libs = [\n]\n")
        claims.save_registry(tmp, {})
        label = "proposal/80161660_fn_80161660.cpp"
        with open(brief.queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [{"label": label, "text": [0x80161660, 0x801679B0],
                                                   "count": 52, "bytes": 25424, "cxx": True}]}, fh)
        open(os.path.join(tmp, "tools", "units", "briefs", "pool", claims.slug(label) + ".md"), "w").write(
            "# Proposal brief: %s\n" % label)
        entry = pool_entries(tmp)[0]
        check("a pooled proposal brief is parseable", entry["unit"], claims.norm_unit(label))
        check("a proposal is not a registered unit", is_proposal(tmp, label), True)
        check("a proposal in the queue is ready", state(tmp, entry), "ready")
        check("a proposal's address comes from the queue, not splits.txt",
              text_start(tmp, label), 0x80161660)
        check("next_entry picks the proposal", next_entry(tmp)["unit"], claims.norm_unit(label))
        claims.save_registry(tmp, {claims.norm_unit(label): {"branch": "worker/x", "worker": "me"}})
        check("a claimed proposal is not ready", state(tmp, entry), "claimed")
        check("a claimed proposal is not handed out", next_entry(tmp), None)
        claims.save_registry(tmp, {})

        # the re-hand bug: once the range is registered - under whatever name its worker chose - the
        # proposal is finished work, so neither the state nor next_entry may offer it again.
        check("a range inside a registered unit is covered",
              ranges_cover([(0x80160000, 0x80170000)], 0x80161660), True)
        check("a range end is not covered (half-open)", ranges_cover([(0x80160000, 0x80161660)], 0x80161660), False)
        check("no ranges covers nothing", ranges_cover([], 0x80161660), False)
        saved_units, saved_range = brief.registered_units, brief.splits_range
        brief.registered_units = lambda m: ["enemy/ef_emitterform"]
        brief.splits_range = lambda m, u: {".text": (0x80160000, 0x80170000, "extra")}
        check("a registered range covers the proposal", covered_by_registered(tmp, label), True)
        check("a covered proposal is not ready", state(tmp, entry), "covered")
        check("a covered proposal is not handed out", next_entry(tmp), None)
        brief.splits_range = lambda m, u: {".text": (0x80170000, 0x80180000)}
        check("a proposal outside every range stays ready", state(tmp, entry), "ready")
        check("an unregistered proposal is not covered", covered_by_registered(tmp, "proposal/80161670_fn_80161670"), False)
        brief.registered_units, brief.splits_range = saved_units, saved_range
        with open(brief.queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": []}, fh)
        check("a proposal dropped from the queue is stale", state(tmp, entry), "stale")
        check("a dropped proposal is not handed out", next_entry(tmp), None)
        check("is_proposal is false for an ordinary unit", is_proposal(tmp, "auto/stubA"), False)

    # a wave (`next --count N`) is a stride of N through the address order: two *adjacent* proposals can
    # share a wave only if their indices are congruent mod N, impossible for N > 1. That is the property
    # the campaign needs, because adjacency is what puts two workers on one TU (proposal/8007270C and
    # proposal/80073180 are both g3d_calcvtx.cpp), makes a neighbour register a symbol you declare
    # (the rule-2 boundary artefacts), and shares owner headers by construction.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src"))
        os.makedirs(os.path.join(tmp, "tools", "units", "briefs", "pool"))
        open(os.path.join(tmp, "configure.py"), "w").write("config.libs = [\n]\n")
        claims.save_registry(tmp, {})
        labels = ["proposal/%08X_fn_%08X.cpp" % (0x80160000 + 0x100 * i, 0x80160000 + 0x100 * i)
                  for i in range(6)]
        with open(brief.queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [
                {"label": l, "text": [0x80160000 + 0x100 * i, 0x80160000 + 0x100 * i + 0x80],
                 "count": 1, "bytes": 128, "cxx": False} for i, l in enumerate(labels)]}, fh)
        for l in labels:
            open(os.path.join(tmp, "tools", "units", "briefs", "pool", claims.slug(l) + ".md"),
                 "w", encoding="utf-8").write("# Proposal brief: %s\n" % l)
        units = [claims.norm_unit(l) for l in labels]

        def picks(chosen: list[dict]) -> list[int]:
            """The fixture indices a selection made, in the order it returned them."""
            return [units.index(e["unit"]) for e in chosen]

        def adjacent(indexes: list[int]) -> bool:
            """Whether any two picks are address-adjacent in the queue - the defect a wave must exclude."""
            return any(b - a == 1 for a, b in zip(indexes, indexes[1:]))

        check("the fixture is six adjacent ready proposals in address order",
              [e["unit"] for e in ordered_entries(tmp)], units)
        check("N=1 is the single pick next_entry makes", picks(wave(tmp, 1)), [0])
        check("... and next_entry still agrees", next_entry(tmp)["unit"], units[0])
        w3 = picks(wave(tmp, 3))
        check("N=3 starts at the lowest ready address", w3[:1], [0])
        check("... and then the two farthest non-adjacent indices", w3[1:], [2, 5])
        check("... and claims three spread proposals", (len(w3), adjacent(w3)), (3, False))
        for n in (2, 3):
            sel = picks(wave(tmp, n))
            check("N=%d claims N and never two adjacent" % n, (len(sel), adjacent(sel)), (n, False))
        w6 = picks(wave(tmp, 6))
        check("a wave of six on six adjacent proposals is capped by adjacency, not by readiness",
              (len(w6), adjacent(w6)), (3, False))

        # a claimed stride position is skipped and the walk continues at the next stride position, so the
        # wave still fills - index 3 is claimed, and the wave comes back with 0, 2 and 5
        claims.save_registry(tmp, {units[3]: {"branch": "worker/x", "worker": "me"}})
        claimed3 = picks(wave(tmp, 3))
        check("a claimed stride position is skipped", 3 not in claimed3, True)
        check("... the stride keeps going and the wave still fills", (len(claimed3), adjacent(claimed3)), (3, False))
        check("... taking the farthest non-adjacent ready neighbours", claimed3, [0, 2, 5])
        claims.save_registry(tmp, {})

        # the SYSTEM property: with a splits.txt naming two modules across the fixture's addresses, a wave
        # must span both - that is what keeps two workers out of one shared record/header.  It is written
        # here, at the end of this fixture's checks, so nothing before it sees registered ranges.
        os.makedirs(os.path.join(tmp, "config", "RMHE08"))
        open(os.path.join(tmp, "config", "RMHE08", "splits.txt"), "w", encoding="utf-8").write(
            "enemy/fn_low.cpp:\n\t.text       start:0x80160000 end:0x80160280\n\n"
            "Pl/fn_high.cpp:\n\t.text       start:0x80160280 end:0x80160600\n")
        check("a proposal's system is the module of the nearest registered range",
              [hint_for(tmp, text_start(tmp, u)) for u in units],
              ["enemy", "enemy", "enemy", "Pl", "Pl", "Pl"])
        pair = picks(wave(tmp, 2))
        check("a wave takes the systems it can reach, not the nearest addresses",
              [hint_for(tmp, text_start(tmp, units[i])) for i in pair], ["enemy", "Pl"])
        three = picks(wave(tmp, 3))
        check("... and a three-wave spans both systems while staying non-adjacent",
              (len(set(hint_for(tmp, text_start(tmp, units[i])) for i in three)), adjacent(three)), (2, False))

        # a covered proposal (its range is already registered under another name) is skipped the same way
        saved_units, saved_range = brief.registered_units, brief.splits_range
        brief.registered_units = lambda m: ["enemy/neighbour"]
        brief.splits_range = lambda m, u: {".text": (0x80160300, 0x80160400)}
        check("... a covered stride position is not ready either", state(tmp, ordered_entries(tmp)[3]), "covered")
        covered3 = picks(wave(tmp, 3))
        check("a covered stride position is skipped, stride intact",
              (3 in covered3, len(covered3), adjacent(covered3)), (False, 3, False))
        brief.registered_units, brief.splits_range = saved_units, saved_range

        # fewer than N ready: claim what exists and report the shortfall instead of failing
        claims.save_registry(tmp, {u: {"branch": "worker/x", "worker": "me"}
                                   for u in (units[1], units[3], units[4], units[5])})
        short = picks(wave(tmp, 3))
        check("fewer than N ready claims what exists", (len(short), adjacent(short)), (2, False))
        check("... and it is the ready pair, not the claimed neighbours", short, [0, 2])

        def fake_claim(unit, main, worker, dry_run):
            claims.save_registry(main, {unit: {"branch": claims.branch_for(unit),
                                               "worktree": claims.worktree_for(unit, main)}})
            return {"unit": unit, "branch": claims.branch_for(unit),
                    "worktree": claims.worktree_for(unit, main)}

        out = next_briefs(tmp, "w-wave", dry_run=False, count=3, claim_fn=fake_claim)
        check("a short wave reports the shortfall rather than failing",
              (out["requested"], out["claimed"], out["shortfall"]), (3, 2, 1))
        check("... every claim carries its own spawn",
              [c["unit"] for c in out["claims"]], [units[0], units[2]])
        check("... and every spawn is a subagent call",
              all(c["spawn"]["call"].startswith("subagent(agent=\"decompiler\"")
                  for c in out["claims"]), True)
        check("... claimed through the same path as a single pick",
              units[2] in claims.load_registry(tmp), True)

        # a full wave claims N proposals and nothing adjacent, on the untouched fixture
        claims.save_registry(tmp, {})
        dry3 = next_briefs(tmp, "w-wave", dry_run=True, count=3, claim_fn=fake_claim)
        check("a dry-run wave claims nothing", (dry3["claimed"], claims.load_registry(tmp)), (3, {}))
        check("... but returns every spawn the real wave would",
              [c["unit"] for c in dry3["claims"]], [units[0], units[2], units[5]])
        out = next_briefs(tmp, "w-wave", dry_run=False, count=3, claim_fn=fake_claim)
        wave3 = [units.index(c["unit"]) for c in out["claims"]]
        check("a full wave claims exactly N", (out["claimed"], out["shortfall"]), (3, 0))
        check("... the three are the spread picks, non-adjacent", (wave3, adjacent(wave3)), ([0, 2, 5], False))
        check("... and the addresses are the spaced ones",
              [text_start(tmp, c["unit"]) for c in out["claims"]],
              [0x80160000, 0x80160200, 0x80160500])
        try:
            next_briefs(tmp, None, dry_run=True, count=0)
            check("a --count below 1 is refused", "no error", "SystemExit")
        except SystemExit as exc:
            check("a --count below 1 is refused", "at least 1" in str(exc), True)
        try:
            wave(tmp, 0)
            check("... the selector refuses it too, so no caller can pass it through", "no error", "SystemExit")
        except SystemExit as exc:
            check("... the selector refuses it too, so no caller can pass it through", "at least 1" in str(exc), True)

    # The pool is NOT the source of truth at claim time. `brief.py --pool` used to skip a brief that already
    # existed, so `attribute.py queue` re-cutting a range left every pre-existing pooled brief describing the
    # OLD scope - and `queue.py next` copied it. `proposal/80119DEC` was handed `.text 0x80119DEC..0x8011A34C`
    # (two functions) while the queue said `..0x8011D448` (thirty-seven). The claim path must render the
    # current entry, and the stale pooled range must never appear in what the worker reads.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, "src"))
        os.makedirs(os.path.join(tmp, "tools", "units", "briefs", "pool"))
        open(os.path.join(tmp, "configure.py"), "w").write("config.libs = [\n]\n")
        claims.save_registry(tmp, {})
        label = "proposal/80119DEC_fn_80119DEC.cpp"
        pool_path = os.path.join(tmp, "tools", "units", "briefs", "pool", claims.slug(label) + ".md")
        open(pool_path, "w", encoding="utf-8").write(
            "# Proposal brief: %s\n\n"
            "> **Pooled brief** - prepared by `brief.py --pool` before the claim.\n\n"
            "| `.text` range | `0x80119DEC`-`0x8011A34C` (1376 bytes) |\n"
            "| functions | 2 |\n" % label)
        with open(brief.queue_path(tmp), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [{"label": label, "text": [0x80119DEC, 0x8011D448],
                                                "count": 37, "bytes": 13916, "cxx": True,
                                                "tu": {"verdict": "one-tu", "sources": ["eft029.cpp"],
                                                       "partial_source": None, "open_seams": []}}]}, fh)

        def fake_claim(unit, main, worker, dry_run):
            claims.save_registry(main, {unit: {"branch": claims.branch_for(unit),
                                               "worktree": claims.worktree_for(unit, main)}})
            return {"unit": unit, "branch": claims.branch_for(unit),
                    "worktree": claims.worktree_for(unit, main)}

        check("the stale pool brief still selects the unit", next_entry(tmp)["unit"],
              claims.norm_unit(label))
        check("... and the pooled file really states the old range",
              "0x8011A34C" in open(pool_path, encoding="utf-8").read(), True)
        claimed = next_brief(tmp, "w-stale", dry_run=False, claim_fn=fake_claim)
        rendered = open(claimed["brief"], encoding="utf-8").read()
        check("the claim renders the CURRENT queue range", "0x8011D448" in rendered, True)
        check("... and never the stale pooled range", "0x8011A34C" not in rendered, True)
        check("... with the current function count", "| functions | 37 |" in rendered, True)
        check("the stale pooled file is left untouched",
              "0x8011A34C" in open(pool_path, encoding="utf-8").read(), True)

    # an empty pool must refuse, not hand out a brief for a unit nobody prepared
    with tempfile.TemporaryDirectory() as empty:
        os.makedirs(os.path.join(empty, "src"))
        open(os.path.join(empty, "configure.py"), "w").write("config.libs = [\n]\n")
        check("an empty pool has no ready brief", pool_state(empty)["ready"], [])
        check("an empty pool has no next entry", next_entry(empty), None)
        check("an empty pool has no wave", wave(empty, 3), [])
        try:
            next_brief(empty, None, dry_run=True)
            check("an empty pool refuses to hand out a brief", "no error", "SystemExit")
        except SystemExit as exc:
            check("an empty pool refuses to hand out a brief", "no ready brief" in str(exc), True)
        try:
            next_briefs(empty, None, dry_run=True, count=3)
            check("an empty pool refuses a wave", "no error", "SystemExit")
        except SystemExit as exc:
            check("an empty pool refuses a wave", "no ready brief" in str(exc), True)

    # the claim is rooted at MAIN's HEAD: `queue.py next` must refuse when MAIN is not on `main`, or the
    # worker's worktree and branch are cut from the wrong tip (the 2026-09-24 stale-`main` incident)
    def qgit(path, *args):
        p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid",
                            "-c", "user.name=selftest", "-c", "commit.gpgsign=false", *args],
                           cwd=path, capture_output=True, text=True)
        if p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    with tempfile.TemporaryDirectory() as tmp:
        repo = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(repo)
        qgit(repo, "init", "-q")
        qgit(repo, "checkout", "-q", "-b", "main")
        open(os.path.join(repo, "f.txt"), "w").write("base\n")
        qgit(repo, "add", "-A")
        qgit(repo, "commit", "-q", "-m", "claim-time main")
        check("a claim on main passes the branch guard", branch_error(repo), None)
        qgit(repo, "checkout", "-q", "-b", "throwaway-check")
        err = branch_error(repo)
        check("a claim off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        try:
            next_brief(repo, None, dry_run=False)
            check("queue next refuses off main", "no error", "SystemExit")
        except SystemExit as exc:
            check("queue next refuses off main", "throwaway-check" in str(exc), True)
        check("... and claims nothing", os.path.exists(claims.registry_path(repo)), False)

    # The claim lock is the BRANCH, not the registry: `claims.claim` refuses on the branch, so a claim whose
    # registry entry was lost - a half-torn-down release, or two claims racing on `save_registry` - still owns
    # its unit. Selection read only the registry and re-offered such a proposal, then `queue.py next` refused
    # at the branch: the 2026-09-24 pool re-hand on the claim axis (the covered axis is `covered_by_registered`).
    with tempfile.TemporaryDirectory() as tmp:
        repo = os.path.join(tmp, "mhtri-dtk")
        os.makedirs(os.path.join(repo, "src"))
        os.makedirs(os.path.join(repo, "tools", "units", "briefs", "pool"))
        open(os.path.join(repo, "configure.py"), "w").write("config.libs = [\n]\n")
        qgit(repo, "init", "-q")
        qgit(repo, "checkout", "-q", "-b", "main")
        claimed_label = "proposal/80100000_fn_80100000.cpp"   # lower address: what selection would offer first
        fresh_label = "proposal/80200000_fn_80200000.cpp"
        with open(brief.queue_path(repo), "w", encoding="utf-8") as fh:
            json.dump({"version": 1, "units": [
                {"label": claimed_label, "text": [0x80100000, 0x80100100], "count": 2, "bytes": 256,
                 "cxx": False},
                {"label": fresh_label, "text": [0x80200000, 0x80200100], "count": 2, "bytes": 256,
                 "cxx": False},
            ]}, fh)
        for label in (claimed_label, fresh_label):
            open(os.path.join(repo, "tools", "units", "briefs", "pool", claims.slug(label) + ".md"),
                 "w", encoding="utf-8").write("# Proposal brief: %s\n" % label)
        qgit(repo, "add", "-A")
        qgit(repo, "commit", "-q", "-m", "pool-time main")
        by_unit = {e["unit"]: e for e in pool_entries(repo)}
        claimed_unit, fresh_unit = claims.norm_unit(claimed_label), claims.norm_unit(fresh_label)
        check("a proposal with no claim is ready", state(repo, by_unit[claimed_unit]), "ready")
        check("selection would offer it first", next_entry(repo)["unit"], claimed_unit)
        # the branch is the claim (`claims.claim` made it) but the registry record is gone
        qgit(repo, "branch", claims.branch_for(claimed_unit))
        check("... the registry records no claim", brief.claim_for(repo, claimed_unit), {})
        check("... but the branch is a held lock", claims.lock_held(repo, claimed_unit), True)
        check("a live claim branch makes the proposal claimed", state(repo, by_unit[claimed_unit]), "claimed")
        check("... and next_entry skips it for the fresh proposal", next_entry(repo)["unit"], fresh_unit)
        check("... the branch is counted claimed", pool_state(repo)["counts"].get("claimed"), 1)
        # releasing the claim (branch gone, registry empty) returns the proposal to the pool
        qgit(repo, "branch", "-D", claims.branch_for(claimed_unit))
        check("a released claim is ready again", state(repo, by_unit[claimed_unit]), "ready")
        check("... and next_entry offers it first again", next_entry(repo)["unit"], claimed_unit)

    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--selftest", action="store_true")
    sub = ap.add_subparsers(dest="cmd")
    n = sub.add_parser("next", help="claim the next ready unit, promote its brief, print the spawn")
    n.add_argument("--count", type=int, default=1,
                   help="claim a wave of N proposals spread with a stride of N through the address order"
                        " (default 1 - the single next proposal)")
    n.add_argument("--worker", default=None)
    n.add_argument("--profile", default="decompiler",
                   choices=["decompiler", "worker", "fixer", "merger"],
                   help="the agent profile the lane is launched with (default: decompiler - a proposal "
                        "lane registers and reconstructs a unit, which is unit work)")
    n.add_argument("--dry-run", action="store_true")
    n.add_argument("--allow-unlanded", action="append", default=[], metavar="BRANCH",
                   help="name a branch that is parked on purpose, so the unlanded-branch guard lets it "
                        "through (repeatable; default is to refuse while any branch holds work main lacks)")
    n.add_argument("--json", action="store_true")
    l = sub.add_parser("list", help="the pool's state and the next ready candidates")
    l.add_argument("--json", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.cmd:
        ap.print_help()
        return 0

    main_wt = rc.main_root(rc.worktree_root())
    if args.cmd == "list":
        st = pool_state(main_wt)
        if args.json:
            print(json.dumps({"dir": st["dir"], "counts": st["counts"],
                              "next": [e["unit"] for e in st["ready"][:POOL_DEPTH]]}, indent=2))
            return 0
        c = st["counts"]
        print("pool %s" % st["dir"])
        print("  briefs written : %d" % len(st["entries"]))
        print("  ready          : %d  (unclaimed, no bodies)" % c.get("ready", 0))
        print("  claimed        : %d  (in flight)" % c.get("claimed", 0))
        print("  written        : %d  (unit has a body - prune with `brief.py --pool`)" % c.get("written", 0))
        print("  stale          : %d  (no longer registered - prune with `brief.py --pool`)" % c.get("stale", 0))
        un = unlanded_branches(main_wt)
        if un:
            print("  unlanded       : %d branch(es) hold work main does not have - `next` refuses until they "
                  "are landed or deleted" % len(un))
            for branch, files in un[:6]:
                print("      %s (%s)" % (branch, ", ".join("%s +%d" % (f, n) for f, n in files[:3])))
        else:
            print("  unlanded       : none - every branch's content is contained in main")
        print("  covered        : %d  (range already registered under another name)" % c.get("covered", 0))
        print("  unreadable     : %d" % c.get("unreadable", 0))
        if st["ready"]:
            print("\nnext %d ready (address order):" % min(POOL_DEPTH, len(st["ready"])))
            for e in st["ready"][:POOL_DEPTH]:
                start = text_start(main_wt, e["unit"])
                print("  %-10s  %-42s  %s" % ("0x%X" % start if start is not None else "?", e["unit"], e["slug"]))
        else:
            print("\nno ready brief - run `python tools/units/brief.py --pool`")
        return 0

    if args.cmd == "next":
        if args.count != 1:
            out = next_briefs(main_wt, args.worker, args.dry_run, args.count, profile=args.profile,
                              allow_unlanded=args.allow_unlanded)
            if args.json:
                print(json.dumps(out, indent=2))
                return 0
            if out["dry_run"]:
                print("DRY RUN - nothing claimed, nothing written\n")
            else:
                print("claimed %d of %d proposals (stride %d through the address order):\n"
                      % (out["claimed"], out["requested"], out["requested"]))
            for i, c in enumerate(out["claims"], 1):
                start = text_start(main_wt, c["unit"])
                print("--- %d/%d  %s  %s"
                      % (i, out["claimed"], "0x%X" % start if start is not None else "?", c["unit"]))
                if not c["dry_run"]:
                    print("  branch   %s\n  worktree %s\n  brief    %s"
                          % (c["claim"].get("branch"), c["worktree"], c["brief"]))
                sp = c["spawn"]
                print("spawn this worker:")
                print("  agent: %s" % sp["agent"])
                print("  label: %s  (the tool takes no name)" % sp["name"])
                print("  cwd:   %s" % sp["cwd"])
                print("  task:  %s" % sp["task"])
                print("\n%s\n" % sp["call"])
            if out["shortfall"]:
                print("NOTE: claimed %d of the %d requested - the rest of the queue was not ready, or sat"
                      " next to a claim already in this wave; `python tools/units/brief.py --pool`"
                      " replenishes it" % (out["claimed"], out["requested"]))
            return 0
        out = next_brief(main_wt, args.worker, args.dry_run, profile=args.profile,
                         allow_unlanded=args.allow_unlanded)
        if args.json:
            print(json.dumps(out, indent=2))
            return 0
        sp = out["spawn"]
        if out["dry_run"]:
            print("DRY RUN - nothing claimed, nothing written\n")
        else:
            print("claimed %s\n  branch   %s\n  worktree %s\n  brief    %s\n"
                  % (out["unit"], out["claim"].get("branch"), out["worktree"], out["brief"]))
        print("spawn this worker:")
        print("  agent: %s" % sp["agent"])
        print("  label: %s  (the tool takes no name)" % sp["name"])
        print("  cwd:   %s" % sp["cwd"])
        print("  task:  %s" % sp["task"])
        print("\n%s" % sp["call"])
        return 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
