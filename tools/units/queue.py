"""Hand a pooled brief to a worker: claim the unit, promote the brief, print the spawn line.

`brief.py --pool` prepares the brief for every registered unit that has no bodies yet (owner's ask,
2026-09-23: "prepare briefs in advance and queue new work right away"), so the orchestrator can start a
worker the instant a slot frees without deriving anything. This is the other half:

    python tools/units/queue.py next [--worker NAME] [--dry-run] [--json]
    python tools/units/queue.py list [--json]
    python tools/units/queue.py --selftest

`next` picks the pooled unit with the lowest `.text` address that is still **unclaimed**, takes the claim
(`claims.py claim` creates the worktree and the branch), promotes the pooled brief to the claim's own slug
path, and prints the exact spawn line - cwd, name and task text - to paste.

A pooled brief is claim-independent by construction: it is rendered against the worktree the claim *will*
create (`claims.worktree_for`) and against the branch `claims.py claim` *will* make (`worker/<slug(unit)>`),
so promoting it is a copy. A claim whose branch was named with its own suffix (a manual round) is re-rendered
instead, so the outbox path in the brief is always the claim's own. The unit is claimed **before** the brief
is handed out, so a worker never gets a brief whose outbox does not exist.

`list` shows the pool's state: briefs written, ready (unclaimed, no bodies), claimed (in flight), written (a
unit that has gained a body - `brief.py --pool` prunes those), covered (its range is registered already,
under whatever name - never handed out) and stale (no longer registered), plus the next few ready
candidates in address order.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
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


def state(main: str, entry: dict) -> str:
    """Where a pooled brief stands: `claimed`, `written`, `stale`, `unreadable` or `ready`.

    Only `ready` may be handed out. A claimed unit is in flight (its brief has already been promoted), a
    `written` unit has gained a body (the pool will prune it), and `stale` is no longer registered - so none
    of them is offered as new work.

    A **proposal** (option A) is the simpler case and takes a different route: it has no source and no
    registration to check, so while it is in the queue it is work to hand out, and once the queue drops it -
    which happens when `attribute.py queue` is re-run after its range was registered - it is stale.  Until
    the queue is re-run, a proposal whose range is *already* registered under another name is `covered`.
    """
    unit = entry.get("unit")
    if not unit:
        return "unreadable"
    if brief.claim_for(main, unit):
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


def next_entry(main: str, entries: list[dict] | None = None) -> dict | None:
    """The next ready brief: lowest `.text` address first, then unit name (the campaign's order)."""
    ready = [e for e in (entries if entries is not None else pool_entries(main)) if state(main, e) == "ready"]
    ready.sort(key=lambda e: (text_start(main, e["unit"]) is None, text_start(main, e["unit"]) or 0,
                              e["unit"] or ""))
    return ready[0] if ready else None


def spawn_line(main: str, unit: str, slug: str, wt: str, brief_path: str) -> dict:
    """The paste-ready spawn: agent, cwd and task text for the orchestrator.

    The task names the brief by its absolute MAIN path: the brief is written into MAIN *after* the worktree
    was created, so the worktree's own checkout does not contain it. The call is the default `subagent`
    tool's single mode (`agent`/`task`/`cwd`); it has no `name` parameter, so the slug is a label only, and
    it blocks and returns the worker's final message to the orchestrator when the child process exits.
    """
    task = ("Read %s (in MAIN) and do exactly what it says. "
            "Ack first: python tools/units/claims.py ack %s --agent worker-%s. "
            "You may fan out subagents. End your turn with your report: "
            "your final message is the result the orchestrator receives."
            % (brief_path.replace("\\", "/"), unit, slug))
    return {"agent": "worker", "name": "worker-%s" % slug, "cwd": wt, "task": task,
            "call": "subagent(agent=\"worker\", cwd=\"%s\", task=%s, timeoutMs=%d)"
                    % (wt.replace("\\", "/"), json.dumps(task), TIMEOUT_MS)}


def promote(main: str, unit: str, pool_path: str, claim_slug: str) -> str:
    """Copy a pooled brief to the claim's slug path, re-rendering it if the branch carries a suffix.

    The pooled brief was rendered against the branch `claims.py claim` makes (`worker/<slug(unit)>`), so for
    every claim this tool makes the copy is byte-for-byte correct. A manually named branch is different: its
    slug (and therefore its outbox path) is not the unit's, so the brief is re-rendered against the real
    claim rather than copied.
    """
    dest = os.path.join(main, "tools", "units", "briefs", claim_slug + ".md")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if claim_slug == claims.slug(unit):
        shutil.copyfile(pool_path, dest)
        return dest
    # A proposal must still dispatch on its queue entry here, not on a registered unit it does not have yet.
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


def next_brief(main: str, worker: str | None, dry_run: bool, claim_fn=None) -> dict:
    """Claim the next ready unit, promote its brief, and return the spawn.

    The unit is claimed first and the brief is only promoted once the claim exists, so a worker is never
    handed an outbox path that does not exist. `claim_fn` is injectable so the selftest can exercise the
    whole flow without a git worktree. Before the claim, `branch_error` refuses a MAIN whose HEAD is not
    `main`, because the worktree and branch are cut from that HEAD.
    """
    claim_fn = claim_fn or claims.claim
    if not dry_run:
        # the worktree/branch is cut from MAIN's HEAD, so a claim made on another branch is rooted wrong
        bad_branch = branch_error(main)
        if bad_branch:
            raise SystemExit("REFUSED queue next | %s" % bad_branch)
    entry = next_entry(main)
    if entry is None:
        raise SystemExit(
            "no ready brief in %s\n"
            "  run `python tools/units/brief.py --pool` first, or every pooled unit is claimed or written\n"
            "  see: python tools/units/queue.py list" % pool_dir(main))
    unit = entry["unit"]
    slug = claims.slug(unit)
    wt = claims.worktree_for(unit, main)
    info = {"unit": unit, "branch": claims.branch_for(unit), "worktree": wt, "dry_run": True} \
        if dry_run else claim_fn(unit, main, worker, False)
    claim_slug = claims.claim_slug(main, unit) or slug
    if dry_run:
        brief_path = os.path.join(main, "tools", "units", "briefs", claim_slug + ".md")
    else:
        brief_path = promote(main, unit, entry["path"], claim_slug)
    return {"unit": unit, "slug": slug, "claim_slug": claim_slug, "worktree": wt, "brief": brief_path,
            "pool_brief": entry["path"], "claim": info, "dry_run": dry_run,
            "spawn": spawn_line(main, unit, claim_slug, wt, brief_path)}


def pool_state(main: str) -> dict:
    """The pool's counts by state, plus the ready candidates in order."""
    entries = pool_entries(main)
    counts: dict[str, int] = {}
    for entry in entries:
        counts[state(main, entry)] = counts.get(state(main, entry), 0) + 1
    ready = [e for e in entries if state(main, e) == "ready"]
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
        check("dry-run's spawn names the worker", dry["spawn"]["name"], "worker-" + claims.slug("auto/stubB"))
        check("dry-run's spawn has the worktree as cwd", dry["spawn"]["cwd"], claims.worktree_for("auto/stubB", tmp))
        check("dry-run's spawn task names the brief", dry["brief"].replace("\\", "/") in dry["spawn"]["task"], True)
        check("dry-run's spawn task names the unit", "auto/stubB" in dry["spawn"]["task"], True)
        check("dry-run's spawn is a subagent call", dry["spawn"]["call"].startswith("subagent(agent=\"worker\""), True)
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
        check("the promoted brief is the pooled one",
              open(real["brief"], encoding="utf-8").read(), pooled_text)
        check("the pooled brief is copied, not consumed", os.path.exists(by_unit["auto/stubB"]["path"]), True)
        check("a claimed unit is still counted by pool_state", pool_state(tmp)["counts"].get("claimed"), 1)
        check("the promoted brief keeps the claim's outbox", "outbox" in open(real["brief"], encoding="utf-8").read(), True)
        claims.save_registry(tmp, {})

        # a branch named with a suffix is re-rendered, so its outbox path is the claim's own
        claims.save_registry(tmp, {"auto/stubA": {"branch": claims.branch_for("auto/stubA") + "-zz",
                                                  "worktree": claims.worktree_for("auto/stubA", tmp)}})
        suffixed = promote(tmp, "auto/stubA", by_unit["auto/stubA"]["path"],
                           claims.slug("auto/stubA") + "-zz")
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

    # an empty pool must refuse, not hand out a brief for a unit nobody prepared
    with tempfile.TemporaryDirectory() as empty:
        os.makedirs(os.path.join(empty, "src"))
        open(os.path.join(empty, "configure.py"), "w").write("config.libs = [\n]\n")
        check("an empty pool has no ready brief", pool_state(empty)["ready"], [])
        check("an empty pool has no next entry", next_entry(empty), None)
        try:
            next_brief(empty, None, dry_run=True)
            check("an empty pool refuses to hand out a brief", "no error", "SystemExit")
        except SystemExit as exc:
            check("an empty pool refuses to hand out a brief", "no ready brief" in str(exc), True)

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
    n.add_argument("--worker", default=None)
    n.add_argument("--dry-run", action="store_true")
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
        out = next_brief(main_wt, args.worker, args.dry_run)
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
