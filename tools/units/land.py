"""The land gate: one command that runs the batch checklist and refuses to let a bad batch through.

docs/plan.md 7.5 + 7.16, §11. Six manual commands and a regression-scan heredoc were the previous version of
this, rewritten every batch - and that is where a mistake hides. It is also the place where a green
`ninja build/RMHE08/ok` can lie: the `ok` stamp file may be from an earlier run, and a `NonMatching` batch
never relinks, so `main.elf` never runs and `ok` is the only edge that re-validates anything. So `verify`

* deletes `build/RMHE08/ok` (and `main.elf` when the batch flips an object) **before** the run and requires
  them to be recreated;
* checks every command's exit code, `configure.py`'s included - a failed `configure.py` leaves a stale
  `build.ninja` and every later number is a fiction;
* refuses a batch that moves the ground truth, that moved `main` since the batch base, that touches a file
  outside the batch's expected set, or whose outbox entry does not validate;
* runs the style lint when it exists (7.21), reports the ledger delta, and warns when a unit improved with no
  document or header change to show for it (7.10);
* refreshes the baseline afterwards (7.16), so `ninja changes` compares against the batch that just landed;
* and **releases the claim of every unit it just gated** (owner's rule, "Teardown is part of landing"): a
  landed unit must not leave a worktree, a merged branch or a registry entry behind. A release that is
  incomplete (a live pane, a worktree that would not go) fails the gate and names what held it; `--no-release`
  turns the step off for an orchestrator-only batch.

    python tools/units/land.py record-base [--json]
    python tools/units/land.py land --units a,b [--base SHA] [--no-build] [--no-outbox] [--no-release]
    python tools/units/land.py verify [--base SHA] [--units a,b] [--dry-run] [--no-build] [--no-outbox]
                                  [--no-release] [--allow-regression UNIT]

`land` is the one command and the one you should use: it runs `verify`, stages the batch's own files, commits
**with a pathspec** (`git commit -F msg -- <paths>`, so the whole index is never taken), releases the claims,
and prints a **single answer line** (`LANDED ...` / `REFUSED ...`) whose exit status is the answer. The gate log
goes to stderr, so piping stdout cannot lose the verdict - and a failed gate can never reach `git commit` (the
old flow wrote the message unconditionally, which is how a piped `| tail -3` committed a refused batch twice).
The pathspec is the other half of that safety: without it, another stream's *staged* edit was swept into the
batch's commit twice on 2026-09-23 (`85f3d4b5`, `d50fdd32` took `tools/units/langcheck.py`). Paths the index
holds but the batch does not are left staged and named in a warning.

The pathspec still takes every *allowed* dirty path, so an unrelated edit that was already sitting in the
working tree rode the next commit twice on 2026-09-24 (a prepared `docs/plan.md` under `85ddd7b6`, a
`src/RSO/runtime.c` header under `890631e8`). `record-base` now snapshots the paths already dirty when the
batch opens (`dirty_at_base`), and `land_stageable` excludes one unless the batch names it as a unit - the
snapshot is the only way to tell "already dirty at the base" from "dirty because of this batch". AGENTS.md's
LOCAL-ONLY block is the one exception: it is live state that is always dirty, not foreign work, so only a
real edit outside the block enters the snapshot.

`verify` never commits. It writes the message to `.git/land_msg.txt` **only when every check passed**, and
removes a stale one when it refuses; committing it stays a deliberate step for the rare manual case.

`--no-outbox` and `--no-release` are **separate** opt-outs: skipping the outbox/branch checks does not skip the
claim teardown (the old `--no-worker-units` did both, and a round that passed it left 18 worktrees and 3 dead
claims behind). `--no-worker-units` remains as an alias for `--no-outbox`.
"""

from __future__ import annotations

import argparse
import contextlib
import json
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, os.path.dirname(HERE) + os.sep + "git")
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "agents"))

import unitutil  # noqa: E402
import prepcommit as pc  # noqa: E402
import localonly  # noqa: E402
from units import brief as brief_mod  # noqa: E402
from units import claims  # noqa: E402
from units import handoff as handoff_mod  # noqa: E402
from units import recompile as rc  # noqa: E402

ALLOWED_PREFIXES = ("src/", "include/", "docs/", "tools/", ".agents/skills/")
ALLOWED_FILES = ("configure.py", "AGENTS.md", ".gitignore",
                 "config/RMHE08/splits.txt", "config/RMHE08/symbols.txt")
BASE_FILE = os.path.join(".pi", "land-base.json")


def run(args: list[str], cwd: str) -> subprocess.CompletedProcess:
    return subprocess.run(args, cwd=cwd, capture_output=True, text=True, errors="replace")


def git(args: list[str], cwd: str, check: bool = True) -> str:
    p = run(["git", *args], cwd)
    if check and p.returncode != 0:
        raise SystemExit("git %s failed in %s: %s" % (" ".join(args), cwd, p.stderr.strip()))
    return p.stdout


def agents_md_real_change(main: str) -> bool:
    """True when AGENTS.md differs from HEAD beyond its LOCAL-ONLY working-state section.

    AGENTS.md is dirty between every commit because the LOCAL-ONLY block is live state (non-negotiable 8),
    so recording it as foreign in the base snapshot would stop `land` committing *any* AGENTS.md edit - the
    block would be the only thing the snapshot ever saw. `localonly.find_block` is the marker parser of
    record, so reuse it rather than re-deriving the cut (rule 8's own text mentions the markers).
    """
    path = os.path.join(main, "AGENTS.md")
    if not os.path.exists(path):
        return False
    head = git(["show", "HEAD:AGENTS.md"], main, check=False)
    if not head:
        return False          # untracked: `land_stageable` already refuses it, so the answer does not matter
    try:
        with open(path, encoding="utf-8", newline="") as fh:
            text = fh.read()
        found = localonly.find_block(text)
    except (OSError, SystemExit):
        return True           # unreadable or malformed: treat it as a real change, the conservative choice
    if not found:
        return True
    start, _end, trimmed = found
    stripped = text[:start] + text[trimmed:]
    return stripped.replace("\r\n", "\n") != head.replace("\r\n", "\n")


def record_base(main: str) -> dict:
    head = git(["rev-parse", "HEAD"], main).strip()
    # HEAD == base here, so `changed_paths` is exactly "what was already dirty when the batch opened":
    # tracked edits and untracked files alike. `land_stageable` reads it back as the foreign-path guard.
    # AGENTS.md is the one exception: its LOCAL-ONLY block is live state, so only a real edit counts.
    dirty = changed_paths(main)
    if "AGENTS.md" in dirty and not agents_md_real_change(main):
        dirty = [p for p in dirty if p != "AGENTS.md"]
    data = {"base": head, "recorded_at": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "subject": git(["log", "-1", "--format=%s"], main).strip(),
            "ledger": ledger_numbers(main), "report": report_snapshot(main),
            "dirty_at_base": dirty}
    os.makedirs(os.path.join(main, ".pi"), exist_ok=True)
    with open(os.path.join(main, BASE_FILE), "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1)
    return data


def read_base(main: str) -> dict:
    path = os.path.join(main, BASE_FILE)
    if not os.path.exists(path):
        return {}
    try:
        return json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return {}


def land_message_path(main: str) -> str:
    """The gate's commit message. Only a green gate writes it (`write_land_message`); a red one clears it."""
    return os.path.join(main, ".git", "land_msg.txt")


def write_land_message(main: str, body: str) -> str:
    path = land_message_path(main)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(body)
    return path


def message_body_with_subject(body: str, subject: str) -> str:
    """Replace the gate message's subject line with a deliberate `--message`, keeping the gate's body.

    `verify` writes `land: <units>` followed by the ledger and gate summary. A caller's `--message` replaces
    only that first line, so the landed commit still records the ledger delta it was gated against.
    """
    _first, sep, rest = body.partition("\n")
    return subject + sep + rest


def clear_land_message(main: str) -> str | None:
    """Remove a stale message so a failed gate cannot be committed through `git commit -F .git/land_msg.txt`."""
    path = land_message_path(main)
    if os.path.exists(path):
        os.remove(path)
        return path
    return None


def land_decision(gate_ok: bool, stageable: list[str]) -> tuple[str, str]:
    """What `land` does after the gate: -> (`"commit"` | `"refuse"`, reason).

    The one command has to be safe when its output is piped (the exit status is then lost): the gate's verdict
    *is* the decision, and a red gate can never reach `git commit`. A green gate with nothing to stage is also
    a refusal - there is no batch to land.
    """
    if not gate_ok:
        return "refuse", "the gate failed - nothing staged or committed"
    if not stageable:
        return "refuse", "the gate passed but no batch path is stageable - nothing to commit"
    return "commit", ""


def message_error(subject: str | None) -> str | None:
    """Refuse an empty or whitespace-only `--message` before the gate does any work.

    The incident (2026-09-24, `71c244f9`): `--message "$(cat /tmp/msg1.txt)"`, where the shell's `/tmp` is not
    the one the file was written to, expands to the empty string. An empty override used to be dropped by
    `if subject:`, so the batch landed under the gate's fallback subject `land: <units>` instead of the message
    the worker wrote. Omitting `--message` is how a caller deliberately asks for that fallback; an empty or
    blank argument is the caller's bug, refused here - loudly, naming the argument - at the same cost as a lint
    refusal, and before `verify` runs.
    """
    if subject is None:
        return None
    if not subject.strip():
        kind = "empty" if subject == "" else "whitespace-only"
        return ("--message is %s (%r): pass the subject you meant, or omit --message to use the gate's "
                "default subject" % (kind, subject))
    return None


def branch_error(main: str) -> str | None:
    """Refuse to run the gate anywhere but `main`.

    The incident (2026-09-24): a worker told to "branch and commit there" ran
    `git checkout -b tools/stylelint-rule2-unsplit` in MAIN's checkout, so MAIN's HEAD left `main` and the
    next **14 landings** went onto that branch while the `main` ref sat at `e3ade082`. Nothing failed -
    `.pi/bin/applybranch.sh` and `land.py` both key off `main` - but a stale `main` silently changes what
    they *mean*: the merge-base slides backwards and the branch's diff starts describing already-landed
    units, re-applying them or listing them as deletions (it nearly deleted landed units the same day). The
    gate names the branch it found and refuses before any check runs.
    """
    branch = git(["rev-parse", "--abbrev-ref", "HEAD"], main).strip()
    if branch != "main":
        return ("HEAD is on %r, not main: `git checkout main` first - a batch landed off main puts its "
                "commits on the wrong ref and slides the merge-base" % branch)
    return None


def caller_branch_error(start: str | None = None) -> str | None:
    """`branch_error` asked about the tree the *caller* is in - the guard `record-base`/`verify` run under.

    Both are manual entry points on the landing path, and both read MAIN's tree. But `main` is resolved by
    `rc.main_root`, which walks the worktree list from wherever the caller stands and always answers with the
    first worktree git lists - MAIN. So a `record-base` run from inside a worker's worktree would quietly
    record MAIN's HEAD (or, if MAIN's HEAD had left `main`, a stale one) with nothing saying the wrong tree
    was asked. `land` refuses a HEAD that is not `main` through `branch_error(main)`; these two ask the same
    helper about the caller's own tree (`rc.worktree_root`), so a call from any worktree but MAIN is refused
    and names the branch it found.
    """
    return branch_error(rc.worktree_root(start))


def changed_status(main: str) -> list[tuple[str, str]]:
    """[(status, path)] for every change git reports, untracked files listed individually (`-uall`)."""
    out = git(["status", "--porcelain", "-uall"], main)
    rows = []
    for line in out.splitlines():
        if len(line) < 4:
            continue
        code, path = line[:2].strip(), line[3:].strip()
        if " -> " in path:
            # a rename is two paths, and both belong to the pathspec: passing only the new one leaves the
            # deletion staged, so the commit records an add where the batch meant a move
            old, path = path.split(" -> ")
            rows.append((code, old.strip('"')))
        rows.append((code, path.strip('"')))
    return rows


def changed_paths(main: str) -> list[str]:
    return [path for _code, path in changed_status(main)]


def unit_owned_paths(units: list[str]) -> set[str]:
    """The source paths a batch's units own: `src/<unit>.<ext>` and any path the unit names directly."""
    owned: set[str] = set()
    for unit in units:
        unit = unit.strip("/")
        if not unit:
            continue
        owned.add(unit)
        normalized = claims.norm_unit(unit)
        owned.add("src/" + normalized)
        for ext in (".c", ".cpp", ".cp"):
            owned.add(unit + ext)
            owned.add("src/" + normalized + ext)
    return owned


def base_dirty_paths(main: str) -> set[str]:
    """The paths already dirty when the batch base was recorded - another stream's in-flight work.

    `record_base` snapshots them while HEAD == base, so a path in this set was dirty *before* the batch
    touched anything. A base recorded before this snapshot existed has none, and the guard is off for that
    batch (re-record the base to arm it). `--base` only asserts HEAD; the recorded snapshot still names the
    batch's foreign work.
    """
    return set(read_base(main).get("dirty_at_base") or [])


def land_stageable(units: list[str], rows: list[tuple[str, str]],
                   base_dirty: set[str] | None = None) -> list[str]:
    """The paths `land` stages: the batch's own files, never another stream's in-flight work.

    A *tracked* change inside the allowed set is part of the batch (the cherry-pick, the shared-file edits) -
    except under `tools/`: that tree is where every worker keeps its own in-flight tools, so a tracked
    `tools/` change belongs to the batch only when the batch *names* that path as one of its units. Without
    this, `tools/units/langcheck.py` was a tracked change inside the allowed set, `land` staged it, and the
    batch's commit carried another stream's work (`85f3d4b5`, `d50fdd32`). An **untracked** file is staged
    when it is a named unit's own path or lives under `src/`/`include/` (source is the batch's), but not
    otherwise.

    `base_dirty` is the set `record_base` snapshotted when the batch opened: a path that was already dirty
    then is foreign, not batch material, even inside the allowed set (`docs/plan.md` under `85ddd7b6`,
    `src/RSO/runtime.c` under `890631e8`). The batch still stages a path it *names* as one of its units, so a
    unit the batch is genuinely working on keeps its existing behaviour.
    """
    owned = unit_owned_paths(units)
    foreign = base_dirty or set()
    stageable = []
    for code, path in rows:
        if outside_batch([path]):
            continue
        if path in foreign and path not in owned:
            continue          # already dirty at the batch base: another stream's work, leave it alone
        if code.startswith("??") and path not in owned and not path.startswith(("src/", "include/")):
            continue
        if path.startswith("tools/") and path not in owned:
            continue
        stageable.append(path)
    return stageable


def staged_elsewhere(main: str, stageable: list[str]) -> list[str]:
    """Paths already in the index that are not part of this batch - another stream's in-flight work.

    `land` commits with a pathspec, so these are never swept in; naming them is the warning that keeps the
    accident visible. The 2026-09-23 collision (a staged `tools/units/langcheck.py` landed under two unrelated
    unit commits, `85f3d4b5` and `d50fdd32`) happened because the commit had no pathspec and took the whole
    index.
    """
    staged = git(["diff", "--cached", "--name-only"], main).splitlines()
    return [p for p in staged if p and p not in stageable]


def foreign_warning(foreign: list[str]) -> str:
    """The warning `land` prints when the index holds paths outside the batch (a note, never a refusal)."""
    noun = "path" if len(foreign) == 1 else "paths"
    return ("WARNING: the index holds %d %s outside this batch - left staged, not committed: %s"
            % (len(foreign), noun, ", ".join(foreign)))


def commit_pathspec(main: str, msg_file: str, stageable: list[str]) -> subprocess.CompletedProcess:
    """Commit exactly the batch's paths. `git commit` with no pathspec commits the whole index; with one it
    commits the named paths (read from the working tree) and leaves every other staged path staged."""
    return run(["git", "commit", "-F", msg_file, "--", *stageable], main)


def stage_batch(main: str, stageable: list[str]) -> None:
    """`git add` the batch's paths. Deleted paths are left to the commit's pathspec: `git add` refuses a
    pathspec that matches no working-tree file, while `git commit -- <path>` records the deletion (staged or
    not) on its own. So a rename's source path can stay in the pathspec without breaking the staging step."""
    existing = [p for p in stageable if os.path.exists(os.path.join(main, p))]
    if existing:
        git(["add", "--", *existing], main)


def outside_batch(paths: list[str], allowed: tuple[str, ...] = ALLOWED_PREFIXES,
                  allowed_files: tuple[str, ...] = ALLOWED_FILES) -> list[str]:
    """Paths a batch may not touch: everything the plan keeps for the orchestrator, minus its own writes."""
    bad = []
    for path in paths:
        if path in allowed_files or path.startswith(allowed):
            continue
        bad.append(path)
    return bad


def flips_objects(main: str) -> bool:
    """True when configure.py gains `Object(Matching, ...)` relative to HEAD - the batch flips something."""
    p = run(["git", "diff", "HEAD", "--", "configure.py"], main)
    return bool(re.search(r"^\+.*Object\(\s*Matching", p.stdout or "", re.M))


def regression_rows(changes_json: str) -> list[tuple[str, str, float, float]]:
    """(unit, measure, before, after) for every measure that went down, from a report_changes.json."""
    if not os.path.exists(changes_json):
        return []
    data = json.loads(open(changes_json, encoding="utf-8").read())
    rows = []

    def walk(node):
        if isinstance(node, dict):
            name = node.get("name") or ""
            for key, value in (node.get("measures") or {}).items():
                if isinstance(value, dict) and "old" in value and "new" in value:
                    old, new = value["old"], value["new"]
                    if isinstance(old, (int, float)) and isinstance(new, (int, float)) and new < old - 1e-9:
                        rows.append((name, key, old, new))
            for key in ("units", "children", "categories"):
                for child in (node.get(key) or []):
                    walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(data)
    return [r for r in rows if "auto_" not in r[0]]


def report_snapshot(main: str) -> dict:
    """Per-unit measures and the sub-100 % symbols - the evidence a batch's delta is judged against.

    `build/RMHE08/report_changes.json` only carries DOL-level totals, and `ninja baseline` (which this tool runs
    at the end of a batch) rewrites the very baseline the comparison would need: two consecutive verifies of the
    same tree therefore both report "no regression" while the ledger says matched 231 -> 228. So the snapshot is
    taken at `record-base` and kept in `.pi/`, where nothing overwrites it.
    """
    path = os.path.join(main, "build", "RMHE08", "report.json")
    if not os.path.exists(path):
        return {}
    data = json.loads(open(path, encoding="utf-8").read())
    out = {}
    for unit in data.get("units", []):
        name = unit.get("name") or ""
        measures = unit.get("measures") or {}
        symbols = {}
        for fn in unit.get("functions") or []:
            pct = fn.get("fuzzy_match_percent", fn.get("match_percent"))
            if fn.get("name") and isinstance(pct, (int, float)) and pct < 100.0:
                symbols[fn["name"]] = round(float(pct), 4)
        if measures or symbols:
            out[name] = {
                "fuzzy": measures.get("fuzzy_match_percent"),
                "matched_code": measures.get("matched_code"),
                "symbols": symbols,
            }
    return out


def report_regressions(before: dict, after: dict, allow: list[str]) -> tuple[list[tuple], list[tuple]]:
    """-> (unauthorised, authorised) regressions as (unit, what, before, after).

    A unit's own `fuzzy` dropping, or any symbol dropping, is a regression; `allow` names units whose
    regression an explicit rule authorised (rule 8 of §6.5 costs score, and that cost is measured, not hidden).
    """
    unauthorised, authorised = [], []
    for unit, after_vals in after.items():
        prior = before.get(unit)
        if not prior or "auto_" in unit and "/auto/" not in unit:
            continue          # the auto_* scaffold losing symbols to a real unit is bookkeeping, not a regression
        hit_allowed = any(a in unit for a in allow)
        af, bf = prior.get("fuzzy"), after_vals.get("fuzzy")
        if isinstance(af, (int, float)) and isinstance(bf, (int, float)) and bf < af - 1e-9:
            (authorised if hit_allowed else unauthorised).append((unit, "unit fuzzy", af, bf))
        for sym, bpct in (prior.get("symbols") or {}).items():
            apct = (after_vals.get("symbols") or {}).get(sym)
            if apct is None:
                continue          # reached 100 %: not a regression
            if isinstance(apct, (int, float)) and apct < bpct - 1e-9:
                (authorised if hit_allowed else unauthorised).append((unit, sym, bpct, apct))
    return unauthorised, authorised


def ledger_numbers(main: str) -> dict:
    """The ledger's totals, mapped to the names the message uses (`ledger.py --json` nests them)."""
    p = run([sys.executable, os.path.join("tools", "units", "ledger.py"), "--json"], main)
    if p.returncode != 0:
        return {}
    try:
        data = json.loads(p.stdout)
    except json.JSONDecodeError:
        return {}
    totals = data.get("totals") or data
    return {
        "covered": totals.get("claimed_functions"),
        "closed": totals.get("closed"),
        "partial": totals.get("partial"),
        "unclaimed": totals.get("unclaimed"),
        "matched": totals.get("matched_functions"),
        "bytes": totals.get("matched_code"),
        "total_code": totals.get("total_code"),
        "matched_percent": totals.get("fuzzy_match_percent"),
    }


def summary(before: dict, after: dict) -> str:
    def num(value):
        try:
            return int(value)
        except (TypeError, ValueError):
            return value if isinstance(value, (int, float)) else None

    keys = ("covered", "closed", "partial", "matched", "bytes")
    parts = []
    for key in keys:
        b, a = num(before.get(key)), num(after.get(key))
        if isinstance(b, (int, float)) and isinstance(a, (int, float)):
            parts.append("%s %s -> %s" % (key, b, a))
    return ", ".join(parts) or "(ledger numbers unavailable)"


def branch_commits(main: str, unit: str) -> int:
    """How many commits the unit's worker branch has that main does not already have.

    The batch base is deliberately *not* part of this. A worker's branch is cut when the unit is claimed,
    and the orchestrator records a fresh base for every batch it lands, so `base..branch` is only
    meaningful while that base is main's HEAD; in a multi-batch round the recorded base is stale, which
    makes the count report 0 for work that *is* committed (or miss it entirely). The check's intent is
    simply "the worker's work exists as commits of its own on its branch that landing has not taken yet" -
    `main..branch`. A branch that predates the base but carries its own commit passes; a branch with
    nothing beyond main fails.

    The branch comes from the claim (`claims.claim_branch`), not from re-deriving it here: the unit may be
    spelled with or without its source extension, and the stored branch is the lock.
    """
    unit = claims.norm_unit(unit.strip("/"))
    branch = claims.claim_branch(main, unit)
    if not claims.branch_exists(main, branch):
        return 0
    p = run(["git", "rev-list", "--count", "main..%s" % branch], main)
    return int(p.stdout.strip()) if p.returncode == 0 and p.stdout.strip().isdigit() else 0


def outbox_units(main: str, units: list[str]) -> tuple[list[str], list[str]]:
    """-> (units whose outbox validates, problems)."""
    ok, problems = [], []
    for unit in units:
        unit = claims.norm_unit(unit.strip("/"))
        path = handoff_mod.outbox_path(main, unit)
        if not os.path.exists(path):
            problems.append("%s: no outbox at %s" % (unit, path))
            continue
        entry = json.loads(open(path, encoding="utf-8").read())
        rng = brief_mod.splits_range(main, unit)
        owned = {s["name"] for s in brief_mod.symbols_in_range(main, *rng[".text"][:2])} if rng.get(".text") else set()
        errors, _warnings = handoff_mod.validate(entry, owned)
        if errors:
            problems.extend("%s: %s" % (unit, e) for e in errors)
        else:
            ok.append(unit)
    return ok, problems


def release_plan(checks: list[tuple], units: list[str], release_claims: bool,
                 check_outbox: bool = True) -> list[str]:
    """The units whose claim `verify` releases: every gated unit, but only once every check so far passed.

    Releasing is a side effect, so it must not run behind a failed gate - a refused batch has to leave its
    worker's branch and worktree exactly as they are, or the retry has nothing to re-run. `--no-release`
    turns the step off entirely.

    `check_outbox` is an input that **does not affect the answer**, and that is the point: the old
    `--no-worker-units` flag skipped the outbox check *and* the release, so a round that passed it to quiet
    an outbox problem silently stopped tearing its workers down (18 worktrees and 3 dead claims left behind,
    2026-09-23). Outbox checking and release are separate opt-outs now.
    """
    del check_outbox  # independent of the release decision by design
    if not release_claims or not units:
        return []
    if any(not good for _n, good, _d, _i in checks):
        return []
    return list(units)


def branch_problems(main: str, units: list[str]) -> list[str]:
    """One line per unit whose worker branch does not carry its work as commits.

    A missing branch is the 2026-09-23 shape: `release --force` on an unreported worker deleted the branch
    (its only copy of the work) and left it at `refs/rescue/<slug>`, so the next gate refused with a bare "no
    branch" and the round had to work out how to get the work back. The refusal now names the rescue ref and
    the exact restore command.
    """
    problems = []
    for u in units:
        branch = claims.claim_branch(main, u)
        if not claims.branch_exists(main, branch):
            rescue = claims.rescue_exists(main, u)
            if rescue:
                problems.append("%s (no branch %s; its commits are preserved at %s - restore with "
                                "`git branch %s %s`)" % (u, branch, rescue, branch, rescue))
            else:
                problems.append("%s (no branch %s)" % (u, branch))
        elif branch_commits(main, u) == 0:
            problems.append("%s (branch %s has no commits of its own)" % (u, branch))
    return problems


def verify(main: str, units: list[str], base: str | None, dry_run: bool, no_build: bool,
           allow_regression: list[str] | None = None, check_outbox: bool = True,
           release_claims: bool = True) -> int:
    # a unit's *name* is its path without the source extension (`claims.norm_unit`): `Camellia/camellia` and
    # `Camellia/camellia.c` are one batch, and the gate must key its outbox, branch and splits the same way
    # whichever the orchestrator typed.
    units = [claims.norm_unit(u.strip("/")) for u in units]
    allow_regression = [a.strip() for a in (allow_regression or []) if a.strip()]
    checks: list[tuple[str, bool, str]] = []

    def check(name: str, good: bool, detail: str = "", info: str = "") -> None:
        checks.append((name, good, detail, info))

    # 1. ground truth
    truth = pc.ground_truth_error()
    check("ground truth (build.sha1 == the DOL's hash)", not truth, "; ".join(truth))

    # 2. the batch base: main must not have moved since the batch opened
    recorded = read_base(main)
    want_base = base or recorded.get("base")
    head = git(["rev-parse", "HEAD"], main).strip()
    if want_base:
        check("main has not moved since the batch base", head == want_base,
              "HEAD %s != base %s - a worker committed to main, or another stream landed"
              % (head[:8], want_base[:8]), info="base %s" % (want_base or "?")[:8])
    else:
        check("batch base recorded", False, "no base: run `land.py record-base` when the batch opens")

    # 3. the tree guard + the outbox of every unit in the batch
    paths = changed_paths(main)
    bad = outside_batch(paths)
    check("every changed path belongs to a batch", not bad, "not allowed in a batch: %s" % ", ".join(bad))
    if units and check_outbox:
        ok_units, problems = outbox_units(main, units)
        check("every unit's outbox validates", not problems, "; ".join(problems[:4]))
        uncommitted = branch_problems(main, units)
        check("every unit's branch carries its work as commits", not uncommitted,
              "no commits of its own on the branch (work left uncommitted in the worktree?): %s"
              % ", ".join(uncommitted))
    elif units:
        check("orchestrator-only batch (no worker outboxes to check)", True,
              info="%d unit(s): %s" % (len(units), ", ".join(units)))
    else:
        check("batch units named", False, "pass --units (or --no-outbox for an orchestrator-only batch)")

    # 4. the style lint (7.21), when it exists
    lint = os.path.join(main, "tools", "units", "stylelint.py")
    if os.path.exists(lint):
        p = run([sys.executable, lint, "--diff", want_base or "HEAD"], main)
        check("style lint (§6.5) adds no violation", p.returncode == 0, (p.stdout or p.stderr)[-300:])
    else:
        check("style lint (§6.5)", True, info="not built yet (roadmap 7.21) - skipped")

    before = recorded.get("ledger") or ledger_numbers(main)
    flip = flips_objects(main)
    ok_file = os.path.join(main, "build", "RMHE08", "ok")
    elf_file = os.path.join(main, "build", "RMHE08", "main.elf")

    if dry_run:
        for name, good, detail, info in checks:
            note = (detail if not good else "") or info
            print("%s %s%s" % ("PASS" if good else "FAIL", name, (" - " + note) if note else ""))
        print("\nwould then: delete build/RMHE08/ok%s, run configure.py -> ninja -> report.json -> "
              "regression scan -> ok -> ledger -> baseline" % (" and main.elf (this batch flips an object)" if flip else ""))
        return 0 if all(good for _n, good, _d, _i in checks) else 1

    failures = [name for name, good, _d, _i in checks if not good]
    if failures:
        for name, good, detail, info in checks:
            note = (detail if not good else "") or info
            print("%s %s%s" % ("PASS" if good else "FAIL", name, (" - " + note) if note else ""))
        print("\nREFUSING to build or stage anything: %s" % ", ".join(failures))
        return 1

    def gate(name: str, args: list[str]) -> bool:
        p = run(args, main)
        tail = ((p.stdout or "") + (p.stderr or "")).strip().splitlines()
        check(name + " (exit %d)" % p.returncode, p.returncode == 0, tail[-1] if tail else "")
        return p.returncode == 0

    # 5. the build, with the proof that the `ok` we read is this run's
    for stale in [ok_file] + ([elf_file] if flip else []):
        if os.path.exists(stale):
            os.remove(stale)
    started = time.time_ns()
    built = gate("configure.py", [sys.executable, "configure.py"])
    built = gate("ninja", ["ninja"]) and built
    gate("report.json", ["ninja", "build/RMHE08/report.json"])
    # the regression scan reads build/RMHE08/report_changes.json, which only `ninja changes` writes: without
    # this the scan reads the PREVIOUS batch's file and passes for the wrong reason (the first real run did
    # exactly that, while the ledger showed matched 231 -> 228).
    gate("ninja changes (DOL-level totals, informational)", ["ninja", "changes"])
    before_report = recorded.get("report") or {}
    after_report = report_snapshot(main)
    unauthorised, authorised = report_regressions(before_report, after_report, allow_regression)
    for row in authorised:
        print("note: regression ALLOWED by --allow-regression: %s %s %.2f -> %.2f" % row)
    check("no symbol or unit regressed", not unauthorised,
          "; ".join("%s %s %.2f -> %.2f" % r for r in unauthorised[:5]),
          info=("%d authorised regression(s)" % len(authorised)) if authorised else "")
    used = {a for a in allow_regression if any(a in row[0] for row in authorised)}
    check("every --allow-regression was actually needed", not [a for a in allow_regression if a not in used],
          "stale allowance(s), remove them: %s" % ", ".join(a for a in allow_regression if a not in used))
    if not before_report:
        check("the batch base carries a report snapshot", False,
              "record-base did not snapshot report.json (rebuild it and re-record the base)")
    all_regressed = [("", "", 0.0, 0.0)][:0] + [(u, w, b, a) for u, w, b, a in unauthorised + authorised]
    gate("ok (main.dol verified)", ["ninja", "build/RMHE08/ok"])
    fresh = os.path.exists(ok_file) and os.stat(ok_file).st_mtime_ns >= started
    check("ok was recreated by THIS run", fresh, "the ok stamp predates the run - it proves nothing")

    # 6. the ledger delta + the knowledge delta
    after = ledger_numbers(main)
    improved = any(isinstance(before.get(k), (int, float)) and isinstance(after.get(k), (int, float))
                   and after[k] > before[k] for k in ("closed", "matched", "bytes"))
    docs_changed = any(p.startswith(("docs/", "AGENTS.md")) for p in paths)
    headers_changed = any(p.startswith("src/") and p.endswith((".c", ".cpp", ".cp")) for p in paths)
    check("knowledge delta present if the batch improved something",
          (not improved) or docs_changed or headers_changed,
          "a unit improved and no docs/AGENTS.md/unit header changed in this batch (7.10)")

    # 7. the baseline, so the next batch's `ninja changes` compares against this one (7.16)
    if not no_build:
        gate("ninja baseline", ["ninja", "baseline"])

    # 8. the teardown (owner's rule): release the claim of every unit just gated. This runs after the build
    # checks and only when every one of them passed, so a refused batch keeps its branch and worktree for the
    # retry; an incomplete teardown is itself a failed check (a live pane is named, not silently kept).
    to_release = release_plan(checks, units, release_claims, check_outbox)
    for unit_name in to_release:
        out = claims.release(unit_name, main, force=False, dry_run=False)
        failed_step = next((s for s in out["steps"] if s["status"] == "failed"), None)
        note = out.get("refused") or (("failed: %s - %s" % (failed_step["label"], failed_step["why"]))
                                      if failed_step else "")
        info = "branch %s" % out["branch"]
        if out.get("release_ref"):
            info += "; un-merged commits rescued to %s" % out["release_ref"]
        check("claim released: %s" % unit_name, bool(out.get("complete")), note, info=info)
    if release_claims and units and not to_release:
        check("claim release deferred", True, info="a check above failed - the claim is left alone")
    elif units and not release_claims:
        check("claim release skipped", True, info="--no-release")

    print("%-58s %s" % ("check", "result"))
    for name, good, detail, info in checks:
        note = (detail if not good else "") or info
        print("%-58s %s%s" % (name[:58], "PASS" if good else "FAIL", ("  " + note[:80]) if note else ""))

    body = ["land: %s" % ("; ".join(units) if units else "batch"),
            "",
            "ledger: %s" % summary(before, after),
            "gates: ground truth ok, base %s, %d check(s), ok recreated=%s%s%s"
            % ((want_base or "?")[:8], len(checks), fresh, ", main.elf relinked" if flip else "",
               ", authorised regressions: %s" % ", ".join(sorted(allow_regression)) if allow_regression else ""),
            ""]
    failed = [name for name, good, _d, _i in checks if not good]
    if failed:
        # a failed gate must not leave a message a `git commit -F .git/land_msg.txt` could pick up: the old
        # flow wrote it unconditionally, so a piped `| tail -3` read a green-looking summary and committed a
        # batch whose gate had failed (twice, 2026-09-23). No message exists unless every check passed.
        stale = clear_land_message(main)
        if stale:
            print("removed the stale %s (a failed gate has no committable message)" % os.path.relpath(stale, main))
        print("\nledger: %s" % summary(before, after))
        print("FAILED: %s" % ", ".join(failed))
        return 1
    message = write_land_message(main, "\n".join(body))
    print("\nledger: %s" % summary(before, after))
    print("message written to %s - review it, then `git commit -F .git/land_msg.txt`" % message)
    print("READY: every check passed")
    return 0


def land(main: str, units: list[str], base: str | None, no_build: bool,
         allow_regression: list[str] | None = None, check_outbox: bool = True,
         release_claims: bool = True, subject: str | None = None) -> int:
    """The one command: gate -> stage the batch's files -> commit -> release, one answer line on stdout.

    The failure mode this closes: `verify`'s output was piped (`| tail -3`), the exit status was lost, and a
    batch whose gate had *failed* was committed by hand - twice, leaving a partial source on `main` while
    `ok` stayed green (the unit is `NonMatching`). So the gate's verdict is now the decision, not a report:

    * a red gate never reaches `git commit` (`land_decision`), and `verify` removes any stale message;
    * an empty or whitespace-only `--message` is refused before the gate runs (`message_error`): the empty
      shell substitution that expanded `$(cat /tmp/msg1.txt)` must not silently land the fallback subject;
    * a HEAD that is not `main` is refused before the gate runs (`branch_error`): a land run off `main` moves
      the wrong ref, and every later merge-base and branch diff is computed against a stale `main`;
    * the commit uses the gate's own message, so there is no separate `git commit -F` to get wrong;
    * the commit is `git commit -F msg -- <the batch's paths>`: no pathspec means the whole index, which
      swept another stream's staged edit into a land twice on 2026-09-23 (`85f3d4b5`, `d50fdd32`). Paths the
      index holds but the batch does not are left staged, and a warning names them;
    * the gate log goes to **stderr** and stdout carries exactly one answer line, so `tail -1` is the answer
      whether or not the exit status survived the pipe;
    * the exit status *is* the answer: 0 landed, 1 refused (or landed with an incomplete teardown).

    Releasing runs after the commit, never before: until `main` has the commits, the worker's branch is their
    only copy.
    """
    norm_units = [claims.norm_unit(u.strip("/")) for u in units]
    bad_message = message_error(subject)
    if bad_message:
        # a refusal must not leave a message `git commit -F .git/land_msg.txt` could pick up (2026-09-23)
        clear_land_message(main)
        print("REFUSED %s | %s" % (",".join(norm_units), bad_message))
        return 1
    bad_branch = branch_error(main)
    if bad_branch:
        # same rule as the message guard: a refusal leaves no committable message and touches nothing
        clear_land_message(main)
        print("REFUSED %s | %s" % (",".join(norm_units), bad_branch))
        return 1
    with contextlib.redirect_stdout(sys.stderr):
        gate_ok = verify(main, norm_units, base, dry_run=False, no_build=no_build,
                         allow_regression=allow_regression, check_outbox=check_outbox,
                         release_claims=False) == 0
    rows = changed_status(main)
    outside = outside_batch([path for _code, path in rows])
    if outside:
        clear_land_message(main)
        print("REFUSED %s | paths outside the batch appeared during the build: %s"
              % (",".join(norm_units), ", ".join(outside)))
        return 1
    stageable = land_stageable(norm_units, rows, base_dirty_paths(main))
    action, why = land_decision(gate_ok, stageable)
    if action != "commit":
        clear_land_message(main)
        print("REFUSED %s | %s" % (",".join(norm_units), why))
        return 1
    msg_file = land_message_path(main)
    if subject is not None:
        # the guard above means subject is a real one here, never the empty shell substitution
        write_land_message(main, message_body_with_subject(open(msg_file, encoding="utf-8").read(), subject))
    agents_md = "AGENTS.md" in stageable
    if agents_md:
        pc.localonly("pull")  # the LOCAL-ONLY block must not be committed (non-negotiable 8)
    try:
        stage_batch(main, stageable)
        foreign = staged_elsewhere(main, stageable)
        if foreign:
            print(foreign_warning(foreign), file=sys.stderr)
        # A pathspec, never a bare `git commit`: that takes the whole index and is how another stream's staged
        # edit landed under the batch's message twice on 2026-09-23. `git commit -- <paths>` reads the working
        # tree, so the LOCAL-ONLY block goes back into AGENTS.md *after* the commit, never before it.
        p = commit_pathspec(main, msg_file, stageable)
    finally:
        if agents_md:
            pc.localonly("push")
    if p.returncode != 0:
        clear_land_message(main)
        tail = ((p.stderr or p.stdout) or "").strip().splitlines()
        print("REFUSED %s | git commit failed: %s" % (",".join(norm_units), tail[-1] if tail else ""))
        return 1
    sha = git(["rev-parse", "--short", "HEAD"], main).strip()
    teardown, incomplete = [], []
    if release_claims:
        for u in norm_units:
            out = claims.release(u, main, force=False, dry_run=False)
            if out.get("complete"):
                teardown.append(u)
            else:
                incomplete.append("%s (%s)" % (u, out.get("refused") or "a teardown step failed"))
    ledger = summary(read_base(main).get("ledger") or {}, ledger_numbers(main))
    tail = ""
    if teardown:
        tail += " | teardown %s" % ",".join(teardown)
    if incomplete:
        tail += " | TEARDOWN INCOMPLETE: %s" % "; ".join(incomplete)
    print("LANDED %s %s | %s%s" % (sha, ",".join(norm_units), ledger, tail))
    return 1 if incomplete else 0


def selftest() -> int:
    import tempfile
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("inside the batch: a unit source", outside_batch(["src/Pl/pl_act.cpp"]), [])
    check("inside the batch: splits.txt", outside_batch(["config/RMHE08/splits.txt"]), [])
    check("outside the batch: orig", outside_batch(["orig/RMHE08/sys/main.dol"]), ["orig/RMHE08/sys/main.dol"])
    check("outside the batch: build", outside_batch(["build/RMHE08/main.dol"]), ["build/RMHE08/main.dol"])
    check("outside the batch: ground truth", outside_batch(["config/RMHE08/build.sha1"]),
          ["config/RMHE08/build.sha1"])
    check("outside the batch: the local-only state files", outside_batch([".pi/claims.json"]), [".pi/claims.json"])

    # `land` stages the batch's own files only: a tracked change is the batch, but an untracked file that is
    # not a named unit's source is another stream's in-flight work (the round's `tools/units/playbook.py`)
    rows = [(" M", "src/Pl/pl_act.cpp"), ("??", "src/Pl/pl_act.cpp"), ("??", "tools/units/playbook.py"),
            ("??", "tools/units/land.py"), ("??", "include/Foo.h"), ("??", "src/Other/other.cpp"),
            (" M", "configure.py"), ("??", "build/RMHE08/main.dol")]
    check("a tracked batch file is staged", "src/Pl/pl_act.cpp" in land_stageable(["Pl/pl_act"], rows), True)
    check("an untracked unit source is staged", "src/Pl/pl_act.cpp" in land_stageable(["Pl/pl_act"], rows), True)
    check("another worker's untracked tool is not",
          "tools/units/playbook.py" in land_stageable(["Pl/pl_act"], rows), False)
    check("an untracked source or header is staged",
          ("include/Foo.h" in land_stageable(["Pl/pl_act"], rows)
           and "src/Other/other.cpp" in land_stageable(["Pl/pl_act"], rows)), True)
    check("a unit named by its own path is staged",
          "tools/units/land.py" in land_stageable(["tools/units/land.py"], rows), True)
    check("a shared-file edit is staged", "configure.py" in land_stageable(["Pl/pl_act"], rows), True)
    check("build output is never staged", "build/RMHE08/main.dol" in land_stageable(["Pl/pl_act"], rows), False)
    # the incident: a *tracked* `tools/` change is another stream's work unless the batch names it
    check("another worker's tracked tool edit is not staged",
          "tools/units/langcheck.py" in land_stageable(["Pl/pl_act"], [(" M", "tools/units/langcheck.py")]), False)
    check("a tool the batch names is still staged",
          "tools/units/land.py" in land_stageable(["tools/units/land.py"], [(" M", "tools/units/land.py")]), True)

    # the 2026-09-24 hazard: a path that was already dirty when the batch base was recorded is another
    # stream's work, not batch material, even though it sits inside the allowed set - a prepared `docs/plan.md`
    # rode `85ddd7b6` and a `src/RSO/runtime.c` header rode `890631e8`. The snapshot comes from `record_base`.
    foreign_rows = [(" M", "docs/plan.md"), (" M", "src/RSO/runtime.c"), (" M", "src/Pl/pl_act.cpp"),
                    (" M", "configure.py")]
    dirty_at_base = {"docs/plan.md", "src/RSO/runtime.c"}
    check("a path dirty at the batch base is not staged",
          "docs/plan.md" in land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), False)
    check("... and neither is a foreign unit header",
          "src/RSO/runtime.c" in land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), False)
    check("a path the batch names is staged even if dirty at the base",
          "src/Pl/pl_act.cpp" in land_stageable(["Pl/pl_act"], foreign_rows, {"src/Pl/pl_act.cpp"}), True)
    check("a path clean at the base is still batch material",
          "configure.py" in land_stageable(["Pl/pl_act"], foreign_rows, dirty_at_base), True)
    check("without the base snapshot the old sweep is reproduced (the failure mode)",
          "docs/plan.md" in land_stageable(["Pl/pl_act"], foreign_rows), True)

    # the one-command path: the gate's verdict is the decision, so a red gate can never reach `git commit`
    check("a failed gate refuses the commit", land_decision(False, ["src/Pl/pl_act.cpp"]),
          ("refuse", "the gate failed - nothing staged or committed"))
    check("a green gate with nothing to stage refuses", land_decision(True, []),
          ("refuse", "the gate passed but no batch path is stageable - nothing to commit"))
    check("a green gate with a batch commits", land_decision(True, ["src/Pl/pl_act.cpp"]),
          ("commit", ""))

    # the 2026-09-24 incident: a `--message "$(cat /tmp/msg1.txt)"` whose file lived at a different `/tmp`
    # expanded to the empty string, `if subject:` dropped the override, and the batch landed under the gate's
    # fallback subject instead of the one the worker wrote (`71c244f9`). An empty or blank argument is refused
    # before the gate runs; omitting --message still asks for the gate's default subject; a real one lands.
    check("no --message uses the gate's default subject", message_error(None), None)
    empty_err = message_error("")
    check("an empty --message is refused", empty_err is not None, True)
    check("... and the refusal names --message", "--message" in (empty_err or ""), True)
    check("... and names it empty", "is empty" in (empty_err or ""), True)
    blank_err = message_error("  \t\n ")
    check("a whitespace-only --message is refused", blank_err is not None, True)
    check("... and the refusal names --message", "--message" in (blank_err or ""), True)
    check("... and names it whitespace-only", "whitespace-only" in (blank_err or ""), True)
    check("a real --message is accepted", message_error("ef: land fn_800FAE08 (41/41 symbols)"), None)
    check("a real --message replaces the gate subject and keeps its body",
          message_body_with_subject("land: Pl/pl_act\n\nledger: closed 1 -> 2\n",
                                    "ef: land fn_800FAE08 (41/41 symbols)"),
          "ef: land fn_800FAE08 (41/41 symbols)\n\nledger: closed 1 -> 2\n")

    # the warning that keeps a foreign staged edit visible: `land` leaves it alone and names it
    check("the foreign-index warning names the path",
          foreign_warning(["tools/units/langcheck.py"]),
          "WARNING: the index holds 1 path outside this batch - left staged, not committed: "
          "tools/units/langcheck.py")
    check("... and pluralises two", foreign_warning(["a", "b"]).count("paths"), 1)

    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".git"), exist_ok=True)
        stale = write_land_message(tmp, "land: old batch\n")
        check("a green gate writes the message", os.path.exists(stale), True)
        check("a failed gate removes it", clear_land_message(tmp), stale)
        check("... and it is gone", os.path.exists(stale), False)
        check("clearing a missing message is a no-op", clear_land_message(tmp), None)

    with tempfile.TemporaryDirectory() as tmp:
        # regression_rows() reads a report_changes.json fixture
        fixture = os.path.join(tmp, "changes.json")
        json.dump({"units": [
            {"name": "src/Pl/pl_act.cpp", "measures": {"matched_code": {"old": 100, "new": 90}}},
            {"name": "src/Pl/pl_skill.cpp", "measures": {"matched_code": {"old": 90, "new": 90}}},
            {"name": "main/auto_eft004_set_pl__FP4_PLWUcfffUl_text", "measures": {"matched_code": {"old": 10, "new": 1}}},
        ]}, open(fixture, "w"))
        rows = regression_rows(fixture)
        check("a regression is caught", len(rows), 1)
        check("the auto_ scaffold is ignored", any("auto_" in r[0] for r in rows), False)
        check("a registered auto/* unit is still tracked",
              regression_rows(fixture) != [] and all("main/auto/" not in r[0] for r in rows), True)
        check("a flat measure is not a regression", any(r[1] == "matched_code" and r[2] == 90 for r in rows), False)
        check("a missing changes file is not a crash", regression_rows(os.path.join(tmp, "nope.json")), [])

    # branch_commits() counts `main..branch`, not `<batch base>..branch`: a worker's branch is cut when the
    # unit is claimed, so in a multi-batch round it predates the base the orchestrator later records. Real
    # temp repos, because the check is entirely about git reachability. The unit is spelled extensionless and
    # then called with the extension: the branch must be found under either spelling (the 2026-09-23 fix).
    unit = "Pl/pl_act"
    branch = claims.branch_for(unit)

    def repo_git(path, *args):
        p = subprocess.run(["git", "-c", "user.email=selftest@example.invalid", "-c", "user.name=selftest",
                            "-c", "commit.gpgsign=false", *args], cwd=path, capture_output=True, text=True)
        if p.returncode != 0:
            raise RuntimeError("git %s: %s" % (" ".join(args), p.stderr.strip()))
        return p.stdout.strip()

    def repo_commit(path, msg):
        with open(os.path.join(path, "f.txt"), "a", encoding="utf-8") as fh:
            fh.write(msg + "\n")
        repo_git(path, "add", "-A")
        repo_git(path, "commit", "-q", "-m", msg)

    # `land` commits with a pathspec. `git commit` with none takes the whole index, which is how another
    # stream's staged `tools/units/langcheck.py` landed under two unrelated unit commits on 2026-09-23
    # (`85f3d4b5`, `d50fdd32`). The proof is a real repo, run through the real `land_stageable` and the real
    # commit helper: the batch's paths are committed, the foreign edit is not, and it is still staged
    # afterwards. A rename's deletion is part of the batch, so both of its paths go in the pathspec.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        for name in ("src/batch.c", "src/old.c", "tools/units/langcheck.py"):
            os.makedirs(os.path.dirname(os.path.join(tmp, name)), exist_ok=True)
            with open(os.path.join(tmp, name), "w", encoding="utf-8") as fh:
                fh.write("base\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src/batch.c"), "w", encoding="utf-8") as fh:
            fh.write("the batch\n")
        repo_git(tmp, "mv", "src/old.c", "src/new.c")   # a rename: its deletion belongs to the batch too
        with open(os.path.join(tmp, "tools/units/langcheck.py"), "w", encoding="utf-8") as fh:
            fh.write("another stream\n")
        repo_git(tmp, "add", "tools/units/langcheck.py")  # foreign, staged by another worker
        rows = changed_status(tmp)
        check("a rename reports both of its paths",
              ("src/old.c" in [p for _c, p in rows] and "src/new.c" in [p for _c, p in rows]), True)
        stageable = land_stageable(["Pl/pl_act"], rows)
        check("the tracked tool edit is not part of the batch", "tools/units/langcheck.py" in stageable, False)
        check("the foreign staged edit is named", staged_elsewhere(tmp, stageable), ["tools/units/langcheck.py"])
        stage_batch(tmp, stageable)
        msg_file = os.path.join(tmp, "msg.txt")
        with open(msg_file, "w", encoding="utf-8") as fh:
            fh.write("land: the batch\n")
        p = commit_pathspec(tmp, msg_file, stageable)
        check("the pathspec commit succeeds", p.returncode, 0)
        check("the batch's file is committed", repo_git(tmp, "show", "HEAD:src/batch.c"), "the batch")
        check("the rename's new path is committed",
              run(["git", "cat-file", "-e", "HEAD:src/new.c"], tmp).returncode, 0)
        check("the rename's deletion is committed",
              run(["git", "cat-file", "-e", "HEAD:src/old.c"], tmp).returncode != 0, True)
        check("the foreign edit is NOT committed",
              repo_git(tmp, "show", "HEAD:tools/units/langcheck.py"), "base")
        check("... and is still staged", repo_git(tmp, "diff", "--cached", "--name-only"),
              "tools/units/langcheck.py")
        check("... and still uncommitted", repo_git(tmp, "status", "--porcelain").startswith("M"), True)

    # the snapshot the guard reads: `record_base` must capture what was dirty when it ran, so a path the
    # batch edits afterwards is batch material and one that was dirty before it is foreign. AGENTS.md is
    # special: its LOCAL-ONLY block is live state and always dirty, so only a real edit may enter the set.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        os.makedirs(os.path.join(tmp, "src"), exist_ok=True)
        with open(os.path.join(tmp, "src", "a.c"), "w", encoding="utf-8") as fh:
            fh.write("base\n")
        with open(os.path.join(tmp, "AGENTS.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\n")
        repo_git(tmp, "add", "-A")
        repo_git(tmp, "commit", "-q", "-m", "base")
        with open(os.path.join(tmp, "src", "a.c"), "w", encoding="utf-8") as fh:
            fh.write("foreign\n")
        data = record_base(tmp)
        check("record_base snapshots the dirty set", data.get("dirty_at_base"), ["src/a.c"])
        check("the snapshot is read back", base_dirty_paths(tmp), {"src/a.c"})
        with open(os.path.join(tmp, "AGENTS.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\n" + localonly.BEGIN + "\nworking state\n" + localonly.END + "\n")
        check("a LOCAL-ONLY-only AGENTS.md is not foreign",
              "AGENTS.md" in (record_base(tmp).get("dirty_at_base") or []), False)
        with open(os.path.join(tmp, "AGENTS.md"), "w", encoding="utf-8") as fh:
            fh.write("base agents\nedited outside the block\n"
                     + localonly.BEGIN + "\nworking state\n" + localonly.END + "\n")
        check("a real AGENTS.md edit is foreign",
              "AGENTS.md" in (record_base(tmp).get("dirty_at_base") or []), True)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # the worker branch is cut here
        repo_git(tmp, "checkout", "-q", branch)
        repo_commit(tmp, "the worker's own work")  # committed on the branch
        repo_git(tmp, "checkout", "-q", "main")
        check("a branch with commits ahead of main passes", branch_commits(tmp, unit) > 0, True)
        check("the .cpp spelling finds the same branch", branch_commits(tmp, unit + ".cpp"),
              branch_commits(tmp, unit))

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # cut, but nothing committed on it
        check("a branch with no commits ahead of main fails", branch_commits(tmp, unit), 0)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        repo_git(tmp, "branch", branch)          # cut before the batch base
        repo_commit(tmp, "the batch base")        # main moves on while the worker works
        repo_git(tmp, "checkout", "-q", branch)
        repo_commit(tmp, "the worker's own work")
        repo_git(tmp, "checkout", "-q", "main")
        check("a branch cut before the batch base still passes with commits ahead",
              branch_commits(tmp, unit) > 0, True)

    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("a missing worker branch fails", branch_commits(tmp, unit), 0)

    # a `--force` release deletes the branch and parks the work at refs/rescue/<slug>; the gate's refusal must
    # name that ref and the exact command that puts the branch back (the 2026-09-23 "no branch" dead end)
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        rescue = claims.rescue_ref_name(unit)
        repo_git(tmp, "update-ref", rescue, repo_git(tmp, "rev-parse", "HEAD"))
        problems = branch_problems(tmp, [unit])
        check("a branch gone to a rescue ref is reported", len(problems), 1)
        check("... the refusal names the rescue ref", rescue in problems[0], True)
        check("... and the exact restore command",
              "git branch %s %s" % (claims.branch_for(unit), rescue) in problems[0], True)
        check("a missing branch with no rescue ref is still reported", len(branch_problems(tmp, ["Nope/none"])), 1)
        check("... and has no restore command to name",
              "restore with" in branch_problems(tmp, ["Nope/none"])[0], False)

    # outbox_units() must look where brief.py wrote: the claim's branch minus worker/, not slug(unit)
    entry = {"unit": "Pl/pl_act", "worker": "a", "finished_at": "2026-01-01T00:00:00", "unit_percent": 50.0,
             "symbols": [{"name": "fn_1", "percent": 50.0}], "residual": "none",
             "measured_with": "recompile.py", "config_requests": [], "flags_probed": [], "blockers": []}
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        claims.save_registry(tmp, {"Pl/pl_act": {"branch": claims.branch_for("Pl/pl_act") + "-dd6e"}})
        json.dump(entry, open(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + "-dd6e.json"), "w"))
        check("a branch-derived outbox is found", outbox_units(tmp, ["Pl/pl_act"]), (["Pl/pl_act"], []))
        # the same entry under the unit-path slug is not what brief.py wrote, so the gate must not accept it
        os.remove(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + "-dd6e.json"))
        json.dump(entry, open(os.path.join(tmp, ".pi", "outbox", claims.slug("Pl/pl_act") + ".json"), "w"))
        ok_units, problems = outbox_units(tmp, ["Pl/pl_act"])
        check("an outbox under the unit-path slug is not found", ok_units, [])
        check("and is reported as missing", "no outbox" in (problems[0] if problems else ""), True)

    # the case that failed: a registry keyed by the extensionless name, the gate asked for the .c spelling.
    # `land.py verify --units Camellia/camellia.c` must read the same outbox as `Camellia/camellia`.
    with tempfile.TemporaryDirectory() as tmp:
        os.makedirs(os.path.join(tmp, ".pi", "outbox"), exist_ok=True)
        claims.save_registry(tmp, {"Camellia/camellia": {"branch": "worker/camellia-67ed"}})
        json.dump(dict(entry, unit="Camellia/camellia"),
                  open(os.path.join(tmp, ".pi", "outbox", "camellia-67ed.json"), "w"))
        check("the extensionless spelling finds the outbox", outbox_units(tmp, ["Camellia/camellia"])[1], [])
        check("the .c spelling finds the same outbox", outbox_units(tmp, ["Camellia/camellia.c"])[1], [])
        check("both spellings resolve to one unit",
              outbox_units(tmp, ["Camellia/camellia"])[0] == outbox_units(tmp, ["Camellia/camellia.c"])[0], True)
        check("the outbox path ignores the spelling",
              claims.outbox_path(tmp, "Camellia/camellia.c"), claims.outbox_path(tmp, "Camellia/camellia"))

    # the teardown step (owner's rule): a green gate releases the batch's claims, a failed one leaves them
    green = [("ground truth", True, "", ""), ("ok", True, "", "")]
    red = [("ground truth", True, "", ""), ("ok was recreated by THIS run", False, "stale stamp", "")]
    check("a green gate releases the batch's claims", release_plan(green, ["Pl/pl_act"], True), ["Pl/pl_act"])
    check("a failed gate leaves the claims alone", release_plan(red, ["Pl/pl_act"], True), [])
    check("--no-release turns the teardown off", release_plan(green, ["Pl/pl_act"], False), [])
    check("an empty batch releases nothing", release_plan(green, [], True), [])
    # the 2026-09-23 bug: `--no-worker-units` skipped the outbox check *and* the release, so teardowns stopped
    # for a dozen landings. The two opt-outs are independent now: skipping the outbox check must not skip this.
    check("release runs when the outbox check is off",
          release_plan(green, ["Pl/pl_act"], True, check_outbox=False), ["Pl/pl_act"])
    check("... and is unchanged by it", release_plan(green, ["Pl/pl_act"], True, check_outbox=True),
          release_plan(green, ["Pl/pl_act"], True, check_outbox=False))
    check("--no-release still stops it with the outbox check off",
          release_plan(green, ["Pl/pl_act"], False, check_outbox=False), [])
    check("summary delta", summary({"closed": 284, "matched": 217}, {"closed": 290, "matched": 223}),
          "closed 284 -> 290, matched 217 -> 223")
    check("summary tolerates a missing side", summary({}, {}), "(ledger numbers unavailable)")

    # the branch guard: `land` runs on `main`, never on a worker's branch. The incident (2026-09-24): a worker
    # told to "branch and commit there" ran `git checkout -b tools/stylelint-rule2-unsplit` in MAIN's checkout,
    # so MAIN's HEAD left `main` and the next 14 landings went onto that branch while the `main` ref sat at
    # `e3ade082`. `applybranch.sh` and `land.py` both key off `main`, so a stale `main` does not fail - it
    # silently changes what their diff means. The gate must refuse before any check runs.
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("landing on main passes the branch guard", branch_error(tmp), None)
        repo_git(tmp, "checkout", "-q", "-b", "throwaway-check")
        err = branch_error(tmp)
        check("a land off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        # the refusal path itself: exit 1, no gate, and the stale message is cleared like every other refusal
        import io
        write_land_message(tmp, "land: stale batch\n")
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            code = land(tmp, ["Pl/pl_act"], None, no_build=True, subject="x")
        check("a land off main refuses with exit 1", code, 1)
        check("... the refusal names the branch", "throwaway-check" in buf.getvalue(), True)
        check("... and the stale message is cleared", os.path.exists(land_message_path(tmp)), False)

    # `record-base` and `verify` are the landing path's two other manual entry points. They read MAIN's tree,
    # but MAIN is resolved from wherever the caller stands (`rc.main_root`), so both must refuse a caller that
    # is not on `main` - a worker's worktree is on the claim's branch, not main, and a MAIN left on a throwaway
    # branch would record the wrong HEAD. `caller_branch_error` is `branch_error` asked about the caller's tree;
    # the entry points themselves are exercised through `main()` with the worktree root pointed at a temp repo.
    import io
    import unittest.mock
    with tempfile.TemporaryDirectory() as tmp:
        repo_git(tmp, "init", "-q")
        repo_git(tmp, "checkout", "-q", "-b", "main")
        repo_commit(tmp, "claim-time main")
        check("record-base/verify on main pass the caller guard", caller_branch_error(tmp), None)
        repo_git(tmp, "checkout", "-q", "-b", "throwaway-check")
        err = caller_branch_error(tmp)
        check("a record-base/verify off main is refused", err is not None, True)
        check("... the refusal names the branch it found", "throwaway-check" in (err or ""), True)
        check("... and tells the caller to checkout main", "git checkout main" in (err or ""), True)
        # the normal path: `record-base` from a tree on `main` still records that HEAD
        repo_git(tmp, "checkout", "-q", "main")
        with unittest.mock.patch.object(rc, "worktree_root", return_value=tmp), \
                unittest.mock.patch.object(sys, "argv", ["land.py", "record-base", "--json"]):
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                code = main()
        check("record-base on main still runs", code, 0)
        check("... and records the base it read", read_base(tmp).get("base"), repo_git(tmp, "rev-parse", "HEAD"))
        # both entry points refuse a caller off `main`, before reading or writing anything
        repo_git(tmp, "checkout", "-q", "throwaway-check")
        for cmd in ("record-base", "verify"):
            with unittest.mock.patch.object(rc, "worktree_root", return_value=tmp), \
                    unittest.mock.patch.object(sys, "argv", ["land.py", cmd]):
                buf = io.StringIO()
                with contextlib.redirect_stdout(buf):
                    code = main()
            check("land.py %s off main refuses" % cmd, code, 1)
            check("... names the branch it found", "throwaway-check" in buf.getvalue(), True)
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
    rb = sub.add_parser("record-base", help="record main's HEAD as the batch base")
    rb.add_argument("--json", action="store_true")
    v = sub.add_parser("verify", help="run the batch checklist (never commits; use `land` for that)")
    v.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    v.add_argument("--units", default=None, help="comma-separated units in this batch")
    v.add_argument("--dry-run", action="store_true", help="run the cheap checks only; touch nothing")
    v.add_argument("--no-build", action="store_true", help="skip the split/link/ok/baseline steps")
    v.add_argument("--no-outbox", "--no-worker-units", action="store_true", dest="no_outbox",
                   help="skip the outbox/branch checks (an orchestrator-only batch); release still runs, "
                        "add --no-release to skip that too. `--no-worker-units` is the old alias and no "
                        "longer turns the teardown off")
    v.add_argument("--no-release", action="store_true", dest="no_release",
                   help="do not release the batch's claims")
    v.add_argument("--allow-regression", action="append", default=[],
                   help="unit whose measured regression is authorised by a rule (recorded in the message); repeatable")
    v.add_argument("--json", action="store_true")
    ld = sub.add_parser("land", help="gate + stage + commit + release; one answer line, exit status is the answer")
    ld.add_argument("--base", default=None, help="expected main HEAD (default: the recorded base)")
    ld.add_argument("--units", required=True, help="comma-separated units in this batch")
    ld.add_argument("--no-build", action="store_true", help="skip the baseline step")
    ld.add_argument("--no-outbox", "--no-worker-units", action="store_true", dest="no_outbox",
                    help="skip the outbox/branch checks (release still runs)")
    ld.add_argument("--no-release", action="store_true", dest="no_release",
                    help="commit without releasing the batch's claims")
    ld.add_argument("--allow-regression", action="append", default=[],
                    help="unit whose measured regression is authorised by a rule; repeatable")
    ld.add_argument("--message", default=None, help="override the gate message's subject line")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    main = rc.main_root(rc.worktree_root())
    if args.cmd == "record-base":
        # manual entry point: it reads MAIN's tree, so it must refuse a caller that is not in MAIN on main
        bad_branch = caller_branch_error()
        if bad_branch:
            print("REFUSED record-base | %s" % bad_branch)
            return 1
        data = record_base(main)
        print(json.dumps(data, indent=2) if args.json else "base %s (%s)" % (data["base"], data["subject"]))
        return 0
    if args.cmd == "verify":
        bad_branch = caller_branch_error()
        if bad_branch:
            print("REFUSED verify | %s" % bad_branch)
            return 1
        units = [u.strip() for u in (args.units or "").split(",") if u.strip()]
        return verify(main, units, args.base, args.dry_run, args.no_build, args.allow_regression,
                      check_outbox=not args.no_outbox, release_claims=not args.no_release)
    if args.cmd == "land":
        units = [u.strip() for u in args.units.split(",") if u.strip()]
        return land(main, units, args.base, args.no_build, args.allow_regression,
                    check_outbox=not args.no_outbox, release_claims=not args.no_release,
                    subject=args.message)
    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
