"""Write the one file a worker is handed: `tools/units/briefs/<slug>.md`.

docs/plan.md 7.3, §5.2. Four workers in separate processes inherit nothing from the orchestrator's context, so
the brief has to be self-contained and has to say the same thing every time. It has exactly six parts:

1. the unit      - path, lib, mw_version, the real cflags, object and target paths, the `.text` range
2. the inventory - every symbol the unit owns, its address, size and current measured %
3. the residuals - the unit's file-header comment, so a re-brief never re-derives settled work
4. the decided   - the flags landed for this lib, and the data ranges deliberately not claimed
5. the task      - the functions still under the bar, in address order (or an explicit --task)
6. the rules     - `docs/plan.md` §6.5 and §8 verbatim, plus the measurement loop

    python tools/units/brief.py <unit> [--task "..."] [--stdout] [--json] [--selftest]

The brief is written into MAIN (`<main>/tools/units/briefs/`), not into a worker's worktree, so it outlives
the worktree the same way the outbox does. Its file name and the paths in part 4 come from the unit's
**active claim**: the slug is the claim's branch minus `worker/` - the name `land.py`'s gate keys the outbox
by - never a name re-derived from the unit path, and a brief for an unclaimed unit says so instead of
inventing one.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
# the brief carries the plan's text verbatim (<=, >=, em dashes), and a Windows console is cp1252: without this
# `--stdout` dies on the first such character while the file write (UTF-8) is fine
try:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
except Exception:
    pass
sys.path.insert(0, os.path.dirname(HERE))

import unitutil  # noqa: E402
from units import recompile as rc  # noqa: E402
from units import claims  # noqa: E402

SRC_EXT = (".c", ".cpp", ".cp", ".cxx", ".cc")
BAR = 80.0


def source_name(unit: str) -> str:
    """`Pl/pl_act` -> `Pl/pl_act.cpp`; a full name is left alone."""
    return unit if unit.endswith(SRC_EXT) else unit + ".cpp"


def claim_for(main: str, unit: str) -> dict:
    """The unit's active claim from `MAIN/.pi/claims.json` - `{}` when it is unclaimed."""
    return claims.load_registry(main).get(unit.strip("/"), {})


def claim_slug(claim: dict) -> str | None:
    """The handoff slug `land.py` keys the outbox by: the claim's branch minus `worker/`.

    The one implementation is `claims.slug_of_branch`; this is the dict-shaped wrapper `handoff_paths` uses.
    Read from the branch instead of re-deriving it from the unit path: the branch *is* the claim's identity (it
    is the lock), a round may name it with its own suffix, and the gate looks the outbox up by the branch. An
    unclaimed unit has no branch and therefore no slug: never invent one.
    """
    return claims.slug_of_branch((claim or {}).get("branch"))


def unclaimed_notice(main: str, unit: str) -> str:
    """The plain statement a brief for an unclaimed unit carries in place of an invented slug."""
    return ("**This unit has no active claim.** `%s` carries no `branch` for `%s`, so there is no slug to key "
            "the outbox by - the gate reads `MAIN/.pi/outbox/<branch minus worker/>.json`, and a made-up name "
            "is refused. Claim the unit first (`python tools/units/claims.py claim %s`) and use the brief "
            "written afterwards." % (claims.registry_path(main), unit, unit))


def handoff_paths(main: str, unit: str) -> dict:
    """Where a worker's artefacts go, derived from the claim (see `claim_slug`).

    The ack file and the rescue ref stay `claims.py`'s own (`claims.ack_path`/`claims.slug`): the heartbeat is
    written by `claims.py ack`, so the brief has to name the file that command actually writes.
    """
    unit = unit.strip("/")
    claim = claim_for(main, unit)
    slug = claim_slug(claim)
    return {
        "claimed": bool(slug),
        "branch": claim.get("branch"),
        "slug": slug,
        "ack": claims.ack_path(main, unit),
        "rescue": "refs/rescue/%s" % claims.slug(unit),
        "outbox": claims.outbox_path(main, unit) if slug else None,
        "notes": claims.notes_path(main, unit) if slug else None,
    }


def splits_range(main: str, unit: str) -> dict:
    """The unit's section ranges from `splits.txt`, e.g. {'.text': (start, end, size)}."""
    path = os.path.join(main, "config", "RMHE08", "splits.txt")
    want = source_name(unit.strip("/"))
    out, current = {}, None
    if not os.path.exists(path):
        return out
    for line in open(path, encoding="utf-8", errors="replace"):
        if line[:1] not in (" ", "\t") and line.rstrip().endswith(":") and not line.startswith("#"):
            current = line.strip()[:-1]
            continue
        m = re.match(r"\s+(\S+)\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)", line)
        if m and current and current.lstrip("/") == want:
            start, end = int(m.group(2), 16), int(m.group(3), 16)
            out[m.group(1)] = (start, end, end - start)
    return out


def symbols_in_range(main: str, start: int, end: int) -> list[dict]:
    """The unit's symbols from the map, parsed in-process.

    `symbols.txt` must never reach an agent's context, but a tool reading it programmatically is exactly what
    `attribute.py` and `tudiscover` do. Parsing it here instead of shelling out to `symedit.py --json` also
    avoids that command's multi-document output, which is not a single JSON value.

    A failure is reported, never swallowed: an empty inventory would let a worker believe a unit is finished.
    """
    path = os.path.join(main, "config", "RMHE08", "symbols.txt")
    rows: list[dict] = []
    if not os.path.exists(path):
        print("WARNING: no %s" % path, file=sys.stderr)
        return rows
    pattern = re.compile(r"^(\S+) = (\S+):(0x[0-9A-Fa-f]+); // type:(\w+)( size:(0x[0-9A-Fa-f]+))?")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = pattern.match(line)
            if not m:
                continue
            addr = int(m.group(3), 16)
            if start <= addr < end:
                rows.append({"name": m.group(1), "section": m.group(2), "address": addr,
                             "size": int(m.group(6), 16) if m.group(6) else 0, "type": m.group(4)})
    if not rows:
        print("WARNING: no symbols found in 0x%X-0x%X - check the split range" % (start, end), file=sys.stderr)
    return rows


def report_scores(main: str, unit: str) -> dict:
    """Per-symbol scores for this unit out of `report.json`, keyed by symbol name."""
    path = os.path.join(main, "build", "RMHE08", "report.json")
    if not os.path.exists(path):
        return {}
    data = json.loads(open(path, encoding="utf-8").read())
    want = source_name(unit.strip("/"))
    stem = os.path.splitext(want)[0]
    best = {}
    for u in data.get("units", []):
        name = (u.get("name") or "").replace("\\", "/")
        if not (name.endswith(want) or name.endswith(stem) or name.endswith("/" + stem)):
            continue
        for fn in u.get("functions") or []:
            pct = fn.get("fuzzy_match_percent", fn.get("match_percent"))
            if fn.get("name") and pct is not None:
                best[fn["name"]] = float(pct)
    return best


def header_comment(main: str, wt: str, unit: str) -> str:
    """The unit's file-header comment - the place its residuals live."""
    for root in (wt, main):
        path = os.path.join(root, "src", *source_name(unit).split("/"))
        if not os.path.exists(path):
            continue
        text = open(path, encoding="utf-8", errors="replace").read()
        m = re.match(r"\s*(/\*.*?\*/|//[^\n]*(?:\n//[^\n]*)*)", text, re.S)
        if m:
            return m.group(1).strip()
    return "(no source file yet)"


def flags_for(main: str, unit: str) -> tuple[list[str], str]:
    try:
        tokens = rc.ninja_command(main, unit)
    except SystemExit as exc:
        return [], str(exc)
    return [t for t in tokens if t.startswith("-")], ""


def lib_for(main: str, unit: str) -> str:
    text = open(os.path.join(main, "configure.py"), encoding="utf-8", errors="replace").read()
    want = "\"%s\"" % source_name(unit)
    for block in re.finditer(r"\{\s*\n\s*\"lib\": \"([^\"]+)\"[^{}]*?\"objects\": \[(.*?)\]\s*,\s*\n\s*\}", text, re.S):
        if want in block.group(2):
            return block.group(1)
    return "(unknown)"


def plan_section(main: str, heading: str) -> str:
    """One section of docs/plan.md, verbatim - the rules have exactly one source."""
    path = os.path.join(main, "docs", "plan.md")
    if not os.path.exists(path):
        return ""
    text = open(path, encoding="utf-8", errors="replace").read()
    start = text.find(heading)
    if start < 0:
        return ""
    nxt = re.search(r"\n## ", text[start + len(heading):])
    return text[start:start + len(heading) + (nxt.start() if nxt else len(text))].strip()


def data_queue_entries(main: str, unit: str) -> list[dict]:
    path = os.path.join(main, "tools", "units", "data-queue.json")
    if not os.path.exists(path):
        return []
    try:
        data = json.loads(open(path, encoding="utf-8").read())
    except json.JSONDecodeError:
        return []
    return [e for e in (data if isinstance(data, list) else data.get("entries", []))
            if e.get("unit") == unit]


def build(main: str, wt: str, unit: str, task: str | None) -> dict:
    unit = unit.strip("/")
    rng = splits_range(main, unit)
    text_range = rng.get(".text")
    syms = symbols_in_range(main, text_range[0], text_range[1]) if text_range else []
    scores = report_scores(main, unit)
    tokens, err = flags_for(main, unit)
    lib = lib_for(main, unit)
    obj = os.path.join(wt, "build", "RMHE08", "src", *source_name(unit).split("/"))
    obj = os.path.splitext(obj)[0] + ".o"
    target = os.path.join(main, "build", "RMHE08", "obj", *source_name(unit).split("/"))
    target = os.path.splitext(target)[0] + ".o"
    for sym in syms:
        sym["percent"] = scores.get(sym["name"])
    below = [s for s in syms if (s.get("percent") is None or s["percent"] < BAR)]
    handoff = handoff_paths(main, unit)
    return {
        "unit": unit, "slug": handoff["slug"], "claimed": handoff["claimed"],
        "branch": handoff["branch"], "handoff": handoff, "lib": lib, "worktree": wt, "main": main,
        "source": os.path.join(wt, "src", *source_name(unit).split("/")),
        "object": obj, "target": target,
        "sections": rng, "flags": tokens, "flag_error": err,
        "symbols": syms, "below_bar": len(below),
        "task": task, "task_symbols": [s["name"] for s in below] if not task else [],
        "header": header_comment(main, wt, unit),
        "data_queue": data_queue_entries(main, unit),
    }


def render(main: str, b: dict, task: str | None) -> str:
    rng = b["sections"]
    txt = rng.get(".text")
    lines = []
    lines.append("# Brief: %s" % b["unit"])
    lines.append("")
    lines.append("Read this file, do the task, write your report where §4 says. Nothing outside this file is a rule.")
    lines.append("")
    lines.append("## 0 · Acknowledge first, then heartbeat")
    lines.append("")
    lines.append("Before anything else, say you are alive:")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/claims.py ack %s --agent <your-name> --pane <your-pane>" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("That writes `%s`. Re-run it **with `--progress <symbol>` every time you finish a "
                 "function** - it is the heartbeat by which the orchestrator tells a stalled worker from a "
                 "working one, and it takes a second." % b["handoff"]["ack"])
    lines.append("")
    lines.append("**The timeout policy:** if there is no ack within **%d seconds**, or no progress for "
                 "**%d minutes**, the orchestrator reclaims the unit - your commits are copied to "
                 "`%s` first, then the worktree goes away and the unit is re-briefed to someone "
                 "else. Talk to the orchestrator with the ack and the outbox, not by being busy."
                 % (120, 20, b["handoff"]["rescue"]))
    lines.append("")
    lines.append("## 1 · The unit")
    lines.append("")
    lines.append("| | |")
    lines.append("| --- | --- |")
    lines.append("| unit | `%s` |" % b["unit"])
    lines.append("| lib | `%s` |" % b["lib"])
    lines.append("| source (yours) | `%s` |" % b["source"])
    lines.append("| object (yours) | `%s` |" % b["object"])
    lines.append("| target (read-only, MAIN) | `%s` |" % b["target"])
    lines.append("| worktree | `%s` |" % b["worktree"])
    lines.append("| build | **yours**: `build/RMHE08` in your worktree - compile and measure there, never in MAIN's `build/` |")
    lines.append("| sections | %s |" % (", ".join("%s 0x%X-0x%X" % (s, a, e) for s, (a, e, _n) in sorted(rng.items()))
                                        or "(none in splits.txt)"))
    if b["flags"]:
        lines.append("")
        lines.append("The real command line (flags only; `recompile.py` builds the full one):")
        lines.append("")
        lines.append("```")
        lines.append(" ".join(b["flags"]))
        lines.append("```")
    lines.append("")
    lines.append("## 2 · The inventory (every symbol the unit owns, and where it stands)")
    lines.append("")
    lines.append("| symbol | address | size | measured % |")
    lines.append("| --- | --- | --- | --- |")
    for s in b["symbols"]:
        pct = s.get("percent")
        mark = "" if (pct is not None and pct >= BAR) else " **<- open**"
        lines.append("| `%s` | 0x%X | %s | %s%s |"
                     % (s["name"], s["address"] or 0, s["size"], "n/a" if pct is None else "%.2f" % pct, mark))
    lines.append("")
    lines.append("## 3 · What is already known (the unit's own header, verbatim)")
    lines.append("")
    lines.append("```c")
    lines.append(b["header"])
    lines.append("```")
    lines.append("")
    lines.append("## 4 · Where your output goes")
    lines.append("")
    lines.append("* your source, committed **on your branch** (one commit): `%s`" % b["source"])
    if b["handoff"]["claimed"]:
        lines.append("* `%s` - the outbox `land.py`'s gate reads. It is named after your claim's branch "
                     "(`%s` minus `worker/`), so write it exactly here; do not invent a name."
                     % (b["handoff"]["outbox"], b["handoff"]["branch"]))
        lines.append("* `%s`" % b["handoff"]["notes"])
        lines.append("* a ≤ 15-line digest in your reply")
    else:
        lines.append("* %s" % unclaimed_notice(main, b["unit"]))
        lines.append("* a ≤ 15-line digest in your reply")
    lines.append("")
    lines.append("**End your turn by calling the `subagent_done` tool** (a one-line summary). Do not just reply "
                 "with text: the completion signal the orchestrator is woken by is the sidecar that tool writes, "
                 "and the automatic path does not fire for long runs - a worker that only replies parks in "
                 "`phase: waiting` and its result is never delivered (`.pi/notes/handoff-root-cause.md`).")
    lines.append("")
    lines.append("## 5 · The task")
    lines.append("")
    if task:
        lines.append(task)
    elif b["task_symbols"]:
        lines.append("%d of this unit's symbols are below the %.0f %% bar. Work them **in address order**, "
                     "biggest first where two are equal:" % (b["below_bar"], BAR))
        lines.append("")
        for name in b["task_symbols"][:40]:
            lines.append("* `%s`" % name)
        if len(b["task_symbols"]) > 40:
            lines.append("* ... and %d more" % (len(b["task_symbols"]) - 40))
    elif not b["symbols"]:
        lines.append("**Do not start: the inventory came back empty** - the unit's range is not in splits.txt, its symbols are not in the map, or the map proxy failed. Report it instead of guessing; the warning printed when this brief was generated says which of the three it was.")
    else:
        lines.append("Every symbol is at or above the bar. Confirm it, then improve the worst one if you can do so "
                     "without regressing anything.")
    if b["data_queue"]:
        lines.append("")
        lines.append("**Data this unit may own** (measured second pass - propose, do not claim): "
                     + ", ".join("%s 0x%X-0x%X (%s)" % (e.get("section"), e.get("start", 0), e.get("end", 0),
                                                        e.get("verdict", "unmeasured")) for e in b["data_queue"]))
    lines.append("")
    lines.append("## 6 · The rules")
    lines.append("")
    lines.append("Measurement loop (never `ninja`, never the link, never the split):")
    lines.append("")
    lines.append("```sh")
    lines.append("**Build in your worktree, never in MAIN's.** `build/RMHE08` there is yours alone; MAIN's belongs to")
    lines.append("the orchestrator, and several workers share this machine. The target objects you diff against are read-only")
    lines.append("in MAIN, and your worktree's `build/tools` is seeded with the toolchain (`dtk`, `objdiff-cli`, `sjiswrap`).")
    lines.append("")
    lines.append("python tools/units/recompile.py %s --measure <symbol>   # compiles YOUR source in YOUR worktree" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("A worker never touches `splits.txt`, `configure.py`, `symbols.txt` or `AGENTS.md`, never runs "
                 "`ninja`/the split/the link/`ok`, and never commits on `main`. Everything you need changed goes "
                 "into the outbox's `config_requests`.")
    lines.append("")
    lines.append("**You may fan out subagents** for parallel work. They run in *your* worktree, on *your* branch; "
                 "they never commit (you make the one commit); you assign them disjoint files or functions; and you "
                 "are accountable for what they produce - **re-measure every claim they make**, exactly as the "
                 "orchestrator re-measures yours. Hand each of them this whole part verbatim: a subagent that has "
                 "not read it will name a field `unk4`, reach it with a pointer cast, or use a `goto`, and that "
                 "becomes repair work charged to you.")
    lines.append("")
    lines.append(plan_section(main, "### 6.5 Type and naming discipline"))
    lines.append("")
    lines.append(plan_section(main, "## 8. Invariants"))
    return "\n".join(lines).rstrip() + "\n"


def selftest() -> int:
    fails, checks = [], 0

    def check(name, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            fails.append("%s: got %r want %r" % (name, got, want))

    check("source_name adds the extension", source_name("Pl/pl_act"), "Pl/pl_act.cpp")
    check("source_name keeps one", source_name("main.cpp"), "main.cpp")
    check("source_name handles .c", source_name("RSO/runtime"), "RSO/runtime.cpp")
    check("splits parses a block", splits_range(".", "Pl/pl_act").get(".text") is not None
          and len(splits_range(".", "Pl/pl_act")[".text"]) == 3, True)
    check("splits ignores an unknown unit", splits_range(".", "Nope/nothing"), {})
    check("plan_section finds §6.5", "Type and naming discipline" in plan_section(".", "### 6.5 Type and naming discipline"), True)
    check("§6.5 now carries the goto rule", "`goto` is forbidden" in plan_section(".", "### 6.5 Type and naming discipline"), True)
    check("the plan has the acknowledgement section",
          "Acknowledgement" in plan_section(".", "### 5.6 Acknowledgement"), True)
    check("the subagent contract is in §5.5",
          "fan out subagents" in plan_section(".", "### 5.5 A worker may fan out subagents"), True)
    check("plan_section finds §8", "Invariants" in plan_section(".", "## 8. Invariants"), True)
    check("plan_section is empty for nonsense", plan_section(".", "### 99 nope"), "")

    # the slug is the claim's branch minus worker/, because that is what land.py's gate keys the outbox by
    import tempfile
    from units import handoff as handoff_mod
    with tempfile.TemporaryDirectory() as tmp:
        main = os.path.join(tmp, "mhtri-dtk")
        claims.save_registry(main, {
            "Pl/pl_act": {"branch": claims.branch_for("Pl/pl_act"), "worktree": os.path.join(tmp, "ws"),
                          "base": "0" * 40},
            "Pl/pl_skill": {"branch": claims.branch_for("Pl/pl_skill") + "-dd6e"},
        })
        check("an unclaimed unit has no claim", claim_for(main, "RSO/runtime"), {})
        check("no branch means no slug", claim_slug({}), None)
        check("the slug is the branch minus worker/", claim_slug(claim_for(main, "Pl/pl_act")),
              claims.slug("Pl/pl_act"))
        check("claim_slug is claims.py's one rule", claim_slug(claim_for(main, "Pl/pl_skill")),
              claims.claim_slug(main, "Pl/pl_skill"))
        h = handoff_paths(main, "Pl/pl_act")
        check("a claimed unit is marked claimed", h["claimed"], True)
        check("the outbox is <slug>.json", os.path.basename(h["outbox"]), claims.slug("Pl/pl_act") + ".json")
        check("the outbox is the one handoff.py names", os.path.basename(h["outbox"]),
              os.path.basename(handoff_mod.outbox_path(main, "Pl/pl_act")))
        check("the outbox is claims.py's own outbox path", h["outbox"], claims.outbox_path(main, "Pl/pl_act"))
        check("the outbox lives in MAIN/.pi/outbox", os.path.dirname(h["outbox"]),
              os.path.join(main, ".pi", "outbox"))
        check("the notes path uses the same slug", os.path.basename(h["notes"]), claims.slug("Pl/pl_act") + ".md")
        check("the notes path is claims.py's own", h["notes"], claims.notes_path(main, "Pl/pl_act"))
        check("a branch suffix survives into the slug", handoff_paths(main, "Pl/pl_skill")["slug"],
              claims.slug("Pl/pl_skill") + "-dd6e")
        check("the ack stays claims.py's own path", h["ack"], claims.ack_path(main, "Pl/pl_act"))
        check("the rescue ref stays claims.py's slug", h["rescue"], "refs/rescue/%s" % claims.slug("Pl/pl_act"))
        u = handoff_paths(main, "RSO/runtime")
        check("an unclaimed unit has no slug", u["slug"], None)
        check("an unclaimed unit has no outbox path", u["outbox"], None)
        check("an unclaimed unit is marked", u["claimed"], False)
        check("the unclaimed notice says so plainly",
              "no active claim" in unclaimed_notice(main, "RSO/runtime"), True)
    if fails:
        print("FAIL (%d)" % len(fails))
        for f in fails:
            print("  " + f)
        return 1
    print("ok - %d checks" % checks)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("unit", nargs="?", help="unit path from the repository root, e.g. Pl/pl_act")
    ap.add_argument("--task", default=None, help="override part 5 with your own task text")
    ap.add_argument("--out", default=None)
    ap.add_argument("--stdout", action="store_true")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        return selftest()
    if not args.unit:
        ap.print_help()
        return 0

    wt = rc.worktree_root()
    main = rc.main_root(wt)
    b = build(main, wt, args.unit, args.task)
    text = render(main, b, args.task)
    if args.json:
        print(json.dumps({k: v for k, v in b.items() if k != "header"}, indent=2))
        return 0
    if args.stdout:
        print(text)
        return 0
    if not b["slug"]:
        # the brief still has to have a home, but the file name must not pass itself off as the handoff slug:
        # it falls back to claims.py's own slug and the brief says the unit is unclaimed
        print("WARNING: %s has no active claim in %s - the brief says so and offers no outbox path; the file "
              "name falls back to the registry slug" % (args.unit, claims.registry_path(main)), file=sys.stderr)
    out = args.out or os.path.join(main, "tools", "units", "briefs",
                                   (b["slug"] or claims.slug(args.unit)) + ".md")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    open(out, "w", encoding="utf-8", newline="\n").write(text)
    print("wrote %s (%d lines, %d symbols, %d below the bar)"
          % (out, text.count("\n"), len(b["symbols"]), b["below_bar"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
