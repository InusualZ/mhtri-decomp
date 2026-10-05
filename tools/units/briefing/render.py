"""The brief's text: the six parts, the shared steps, the stamp, the cluster index. Spec: docs/tools/spec/briefing.md.
CLI: none (`tools/units/brief.py`)."""
from __future__ import annotations

import json
import os
import re

from tools.lib.lanes import launch, naming, registry
from tools.units.briefing import sources

#: Every brief carries an invisible stamp of what it was written from, so `--pool` can tell current from stale.
STAMP_MARK = "<!-- brief-stamp:"
_STAMP_RE = re.compile(r"^<!-- brief-stamp: (\{.*\}) -->\s*$", re.M)


def _stamp_line(stamp: dict) -> str:
    return "%s %s -->" % (STAMP_MARK, json.dumps(stamp, sort_keys=True, separators=(",", ":")))


def unit_stamp(unit: str, sections: dict) -> dict:
    """The identifying fields of a unit brief: the unit and its section ranges."""
    return {"kind": "unit", "unit": naming.norm_unit(unit),
            "sections": {k: [int(v[0]), int(v[1])] for k, v in sorted(sections.items())}}


def entry_stamp(main: str, unit: str) -> dict:
    """The stamp the brief for `unit` should carry now."""
    return unit_stamp(unit, sources.splits_range(main, unit))


def brief_stamp(path: str) -> dict | None:
    """The stamp a written brief carries - None for an unstamped or unreadable one."""
    try:
        with open(path, encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return None
    m = _STAMP_RE.search(text)
    if not m:
        return None
    try:
        return json.loads(m.group(1))
    except ValueError:
        return None


def config_schema_lines() -> list[str]:
    """The outbox `config_requests` schema as the brief renders it, from handoff.py's one definition.

    The brief is what the worker is told to follow, so it has to state the vocabulary the validator accepts:
    a worker that invents `data`/`flags`/`tool` (or a non-object `flags_probed`) produces a handoff the gate
    refuses, which is exactly what happened to `RSO/runtime`'s first outbox. Imported lazily because
    `handoff.py` imports this module at load time.
    """
    lines = ["",
             "The validator accepts exactly these kinds - the fields marked * are required:",
             "",
             "| kind | required | also | what it is |",
             "| --- | --- | --- | --- |"]
    for row in sources.config_schema_rows():
        needs = ", ".join("`%s`*" % f for f in row["needs"]) or "(none)"
        also = ", ".join("`%s`" % f for f in row["also"]) or ""
        lines.append("| `%s` | %s | %s | %s |" % (row["kind"], needs, also, row["means"]))
    lines.append("")
    lines.append("`flags_probed` is a list of `{ \"flags\", \"effect\", \"verdict\" }` objects, with the"
                 " verdict one of `%s`. `python tools/units/handoff.py <unit> --template` prints the whole"
                 " skeleton, and `handoff.py <unit> --check <file>` validates what you wrote."
                 % "/".join(sources.flag_probe_verdicts()))
    return lines


def integrator_lines() -> list[str]:
    """The integrator-request contract as the brief renders it, from `lib.requests` (the one schema integrate.py
    reads): where requests go, the kinds, the STOPGAP marker and the trial rule for a lane applying one itself."""
    from tools.lib import requests as _requests
    lines = ["",
             "### Integrator requests (a foreign symbol you need named, declared or moved)",
             "",
             "A change in a unit you do not own is not yours to make: file it, one JSON object per line, in "
             "`MAIN/.pi/outbox/<slug>-requests.json`, and `python tools/units/integrate.py` applies the batch "
             "(renames with their sweep, owner-header declarations, the STOPGAP blocks it clears). "
             "`python tools/units/handoff.py --check-requests <file>` validates it. Fields: `id` (`<slug>#<n>`), "
             "`kind`, `symbol` or `address`, `section`, `proposed_name`, `confidence` (certain|evidence|guess), "
             "`prototype` (the exact C line), `evidence`; optional `owner_unit` (cross-checked against the map) and "
             "`stopgap` (`{file, id}`). A `decl` of an `fn_`/`lbl_` symbol must carry `proposed_name`. A `decl` that "
             "needs several declarations of one owner files them once as `prototypes` (a list of C lines, or of "
             "`{symbol|address, proposed_name, prototype}` objects; no top-level `symbol`): they apply together, "
             "under one STOPGAP block, or not at all.",
             "",
             "| kind | needs | what it is |",
             "| --- | --- | --- |"]
    for row in _requests.schema_rows():
        lines.append("| `%s` | %s | %s |" % (row["kind"], ", ".join("`%s`" % n for n in row["needs"]) or "-",
                                            row["means"]))
    lines += ["",
              "While the request is open, a declaration you cannot do without goes in a STOPGAP block that names it: "
              "`/* STOPGAP-BEGIN(<id>) */ ... /* STOPGAP-END(<id>) */`. The marker exempts nothing (the declarations "
              "still count for rules 2 and 7 at the gate), and a block whose id is no open request is itself a "
              "stylelint finding; integrate deletes the block when it applies the request.",
              "",
              "Trial rule (owner, 2026-10-04): you may apply a `rename` or `decl-move` yourself when the owner unit "
              "has no live claim (`claims.py list`), the confidence is `certain` or `evidence`, and `integrate.py "
              "--dry-run` plus the build show names-only drift for every other unit. GUESS names, record "
              "unification and seams stay with the integrator."]
    return lines


def unclaimed_notice(main: str, unit: str) -> str:
    """The plain statement a brief for an unclaimed unit carries in place of an invented slug."""
    return ("**This unit has no active claim.** `%s` carries no `branch` for `%s`, so there is no slug to key "
            "the outbox by - the gate reads `MAIN/.pi/outbox/<branch minus worker/>.json`, and a made-up name "
            "is refused. Claim the unit first (`python tools/units/claims.py claim %s`) and use the brief "
            "written afterwards." % (registry.registry_path(main), unit, unit))


def _cpp_step(lines: list[str]) -> None:
    """Append section 5c (the C++ class rule) - shared by both brief renderers.

    Owner finding, 2026-09-26: a lane reconstructed a C++ class's methods as free functions taking an explicit
    `self` (403 `self->` uses in Network/fn_8041A87C.cpp) although the target's own string pool spells the two
    class names. The *shape* is part of the match: MWCC emits retail's canonical `lwz r12,0(r3)` /
    `lwz r12,<slot>(r12)` dispatch only for a genuine `virtual`, and a member function's `this` and mangling are
    what the map's names and objdiff pairing expect.
    """
    lines.append("")
    lines.append("### 5c · If the evidence says C++, write the class - not a struct with `self`")
    lines.append("")
    lines.append("A `__FILE__` string naming `Class::method`, a mangled definition, the canonical "
                 "`lwz r12,0(r3)`/`lwz r12,<slot>(r12)` dispatch, an adjustor thunk (`subi r3,r3,0x14`), a "
                 "ctor/dtor pair or a string pool spelling `Class::` all say the range is a class's methods: "
                 "write **member functions on the class**, with the layout annotations (sizes, field offsets) "
                 "kept on the class's fields.")
    lines.append("* MWCC emits retail's canonical virtual dispatch only for a genuine `virtual`; a struct of "
                 "function pointers stages the table through a temporary and loses the function's score.")
    lines.append("* A member function's `this` arrives in r3 and its name mangles the way the map spells it - a "
                 "free function with an explicit `self` gets neither.")
    lines.append("* If a function measures worse in the class form, keep the better-scoring shape and record "
                 "both numbers in the unit header - but never leave `self` style in place because it was "
                 "written first.")
    lines.append("")


def _data_step(lines: list[str]) -> None:
    """Append section 5d (the data step) - shared by both brief renderers.

    Owner instruction, 2026-09-26: a decompiler lane matches **data** as well as code. objdiff's unit score
    does not count a wrong data section, so a lane could hand over a unit whose code matched and whose
    `.data`/`.sdata2` was ours-extra - which is where the 20-unit flip-blocker list came from. Both the
    registered-unit renderer and the proposal renderer call this, because a proposal lane registers its unit
    and is exactly the lane that needs the step.
    """
    lines.append("")
    lines.append("### 5d · Data (measure it, then claim what your object emits)")
    lines.append("")
    lines.append("objdiff's unit score does not count a wrong data section, so measure it: "
                 "`python tools/units/datagap.py --unit <unit>` prints the per-section gap between the target "
                 "object and yours (`build/RMHE08/obj/...o` vs `build/RMHE08/src/...o`). Record it before and "
                 "after your work.")
    lines.append("")
    lines.append("* `ours-extra` - your source defines a table or constant the original TU did not own - is the "
                 "usual defect. A pooled constant is fixed by declaring the map's symbol `extern` and using it "
                 "as a load operand, **never** by defining it (playbook 29/58: a definition makes MWCC emit "
                 "both the named constant and its pool copy, so `.sdata2` grows instead of clearing).")
    lines.append("* Claim the data your object **emits**: exact `start:`/`end:` in `splits.txt`, then "
                 "`rm -f build/RMHE08/config.json` and rebuild - a claim edit that never re-splits links the "
                 "old object and reports a false green. `.data`, `.sdata`, `.ctors` and `.dtors` claims are "
                 "safe; **a partial `.sdata2` claim breaks the link** (playbook 23).")
    lines.append("* A pool entry is claimable **only while your unit is its sole referencer** (playbook 58). A "
                 "private entry is exactly what the claim is for - claim it, flip the unit to "
                 "`Object(Matching)` and say so; a shared entry can be neither claimed nor named in source, so "
                 "write the measured blocker in the unit header and report it.")
    lines.append("* Finish with the numbers: the unit's sections and bytes before/after, and whether "
                 "`python tools/units/datagap.py --flip-blockers` lists this unit.")
    lines.append("")
    lines.append("**A range you need but cannot claim: file it, do not dead-end.** When the rows your unit "
                 "needs sit in data no split range covers, you may not add it to `splits.txt` and rule 12 "
                 "refuses a bare `extern` - so file the request instead of leaving it in prose:")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/dataqueue.py --request <your-unit> 0xADDR --size <n> \\")
    lines.append("    --evidence '<who else references it; ideally no other unit>' \\")
    lines.append("    --unblocks '<the rows/symbols it unblocks>'")
    lines.append("```")
    lines.append("")
    lines.append("That writes `.pi/data-requests.json` (gitignored, deduplicated, byte-deterministic; the orchestrator's `collect` merges it into MAIN's). It is only "
                 "the **filing channel**: the ruling is the orchestrator's, so raise the request through the "
                 "supervisor protocol - never edit `splits.txt` yourself.")


def _precommit_lines(lines: list[str]) -> None:
    """Append the §4 pre-commit gate check and staging hygiene - shared by both renderers.

    The land gate's naming rule is importable, so a worker can run it from its own worktree instead of
    paying a refusal round; two lanes were refused after a full unit of work because they did not run it
    first. And `git add` aborts the *whole* add, silently, when any path it is given no longer exists -
    a rename commit staged only the `git mv` because it also passed the renamed-away path.
    """
    lines.append("")
    lines.append("**Check the gate from your own worktree before you commit** - run its naming rule (the")
    lines.append("lint) and its band check yourself instead of paying a refusal round. The lint must add no")
    lines.append("violation and the second command must print `[]`:")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/stylelint.py --diff main             # run with your worktree as cwd")
    lines.append("python -c \"import sys;sys.path[:0]=['tools','tools/units'];from units import land;"
                 "print(land.band_ownership_warnings(r'.','main'))\"   # likewise")
    lines.append("```")
    lines.append("")
    lines.append("**Stage the whole change, then read the commit back.** `git add <path>` aborts the *whole* add")
    lines.append("- silently - when any `<path>` no longer exists: a rename commit staged only the `git mv` because")
    lines.append("it also passed the renamed-away path. Use `git add -A` with no path arguments, then")
    lines.append("`git show --stat` and check the commit holds every file you touched.")
    lines.append("")


def _dump_asm_hint(lines: list[str]) -> None:
    """The recon prerequisite both renderers were missing: `tudiscover` reads `build/RMHE08/asm/`.

    The build does not write that directory (`config.yml` sets `write_asm: false`), so a lane's first
    `python tools/splits/tudiscover.py at 0x...` reports **0 functions** and reads as a tool bug -
    measured 2026-09-27: it cost a lane its first turn. Since `lib.artifacts` (2026-10-04) `tudiscover`
    makes the dump itself when it is missing or stale (`python tools/splits/dump_asm.py`, 3.5 s measured),
    so the hint says that and names the one status command.

    The objdump path is named here for the same reason: two lanes lost minutes on 2026-09-28 to
    `objdump: command not found`, because nothing is on `PATH` and the build's copy is under `build/`.
    """
    lines.append("**Derived artifacts - a tool refreshes its own; run `python tools/units/fresh.py status` to "
                 "see.** `build/RMHE08/asm/` is not built by default (`write_asm: false`): `tudiscover` runs "
                 "`python tools/splits/dump_asm.py` (~3.5 s) itself when the dump is missing or stale "
                 "(`--no-refresh` reads it as it is), and `callers.py` answers from the split objects unless "
                 "you pass `--refresh`. `FRESH=auto|warn|refuse` overrides a tool's default.")
    lines.append("")
    lines.append("**To read one function's bytes, use the build's own binutils - nothing is on `PATH`:** "
                 "`build/binutils/powerpc-eabi-objdump.exe -d build/RMHE08/obj/<Unit>.o` "
                 "(`ninja tools` fetches it if absent).")
    lines.append("")


def _measure_lines(lines: list[str], unit: str) -> None:
    """Append the working measurement loop (§6) - shared by both renderers.

    The per-lane loop is the shipped tools, now that the git-bash compile path is fixed (`absolutize`
    rewrote cmd's `/c` to `C:\\c`, so the child was an *interactive* `cmd`, printed its banner, and no
    object was ever written). `measure.py` answers the whole unit in one compile + one `report generate`
    with a per-symbol before/after delta; `recompile.py --measure` is the single-symbol proof. The ninja
    path is still valid, and its one trap is that `report.json` is an order-only target of `all_source`.

    The whole-tree `ninja changes` line is playbook 60's check (a declaration change is part of codegen,
    so it can move a row in an unrelated TU); it is one command and it belongs in the checklist.
    """
    lines.append("")
    lines.append("**Measure your own tree with the shipped loop.** `measure.py` compiles the unit once and")
    lines.append("scores **every** symbol with one official `report generate` (the number `report.json` carries):")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/measure.py <your-unit>                  # every symbol, one compile (~0.3 s)")
    lines.append("python tools/units/measure.py <your-unit> --against-main   # each row's before/after delta vs MAIN")
    lines.append("python tools/units/recompile.py <your-unit> --measure <symbol>   # one symbol, the proof step")
    lines.append("```")
    lines.append("")
    lines.append("`<your-unit>` is the path from the repository root (`Pl/pl_act`, `Camellia/camellia`); the")
    lines.append("extension may be omitted. **A score is only real if the object was rewritten:** both tools")
    lines.append("delete the object before compiling and refuse to score one older than its source, and a compile")
    lines.append("that fails prints `FAILED`, never a number. If you see a score, it came from this run.")
    lines.append("")
    lines.append("**The `ninja` path works too, and its one trap is freshness:** `build/RMHE08/report.json` is an")
    lines.append("order-only target of `all_source`, so after a source edit ninja says \"no work to do\" and you")
    lines.append("read the PREVIOUS build's scores.")
    lines.append("")
    lines.append("```sh")
    lines.append("rm -f build/RMHE08/report.json   # order-only target: see below")
    lines.append("ninja build/RMHE08/report.json")
    lines.append("python tools/objdiff/unitscore.py %s   # every symbol, one report read" % unit)
    lines.append("```")
    lines.append("")
    lines.append("**Score the whole unit in one command - never a scratch script.** `unitscore.py` prints every")
    lines.append("symbol of the unit's registered ranges worst-first with the report's own percentage, the unit's")
    lines.append("`matched_functions`/`matched_code`/`total_code` and a summary line, from **one** report read")
    lines.append("(zero objdiff invocations):")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/objdiff/unitscore.py <your-unit>                 # every row + the freshness verdict")
    lines.append("python tools/objdiff/unitscore.py <your-unit> --measure      # one objdiff call over the objects")
    lines.append("python tools/objdiff/unitscore.py <your-unit> --threshold 99 # only the rows below 99 %")
    lines.append("python tools/objdiff/unitscore.py <your-unit> --json         # the whole record, mtimes included")
    lines.append("```")
    lines.append("")
    lines.append("It **refuses to print a stale report's numbers** (exit 1) and names which mtime lost - the report,")
    lines.append("the unit's object, or the newest source under the unit (its include closure included, so a header")
    lines.append("edit counts). `--force-stale` overrides it and keeps the STALE verdict visible; `--measure` skips")
    lines.append("the report entirely and refuses instead when the object is older than its source - the same stale")
    lines.append("number from the other side. One symbol's **instruction rows** (the first divergence, never the")
    lines.append("percentage) are still `python tools/objdiff/symdiff.py -u <your-unit> <symbol>`.")
    lines.append("")
    lines.append("**`build/RMHE08/report.json` is an order-only target of `all_source`**: after a source edit ninja")
    lines.append("says \"no work to do\" and you read the PREVIOUS build's scores. `rm -f build/RMHE08/report.json`")
    lines.append("first - that trap cost one lane three iterations that looked like \"all new functions score 0 %\".")
    lines.append("`unitscore.py` refuses a report it can see is stale, so the trap is reported rather than read.")
    lines.append("")
    lines.append("**Before you report, the whole-tree check - one command:**")
    lines.append("")
    lines.append("```sh")
    lines.append("ninja changes      # every unit whose score moved vs the baseline")
    lines.append("```")
    lines.append("")
    lines.append("**A non-empty diff is a row moving in a unit you did not touch.** A declaration change is part of")
    lines.append("codegen (playbook 60), so changing which header declares a callee can move a neighbouring TU; a")
    lines.append("whole-tree diff is the only thing that shows it. Investigate what moved - never wave it through.")
    lines.append("")
    lines.append("**If you touched a tool, run its own selftest before you commit.** The land gate has a")
    lines.append("`all tool selftests pass` row (`docs/plan.md` 7.30), so a red selftest you never ran is a")
    lines.append("wasted landing - the `measure_selftest.py` was-red-for-weeks incident is why the row exists. One")
    lines.append("runner covers both shapes (`*_selftest.py` and `<tool> --selftest`):")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/selftest.py --changed   # only the selftests of the tools THIS diff touches (fast)")
    lines.append("python tools/selftest.py             # the whole suite (~30 s)")
    lines.append("```")
    lines.append("")
    lines.append("A failure names the tool. A *pre-existing* failure is parked in")
    lines.append("`tools/selftests-known-failures.json` with a reason and a date, and the runner prints")
    lines.append("`green except N parked` - parking is a deliberate, greppable entry, never a silent skip.")
    lines.append("")


def render(main: str, b: dict, task: str | None, pool: bool = False) -> str:
    rng = b["sections"]
    txt = rng.get(".text")
    lines = []
    lines.append("# Brief: %s" % b["unit"])
    lines.append("")
    lines.append(_stamp_line(unit_stamp(b["unit"], rng)))
    lines.append("")
    if pool:
        lines.append("> **Pooled brief** - prepared by `brief.py --pool` before the claim. `queue.py next` claims")
        lines.append("> this unit and hands you this file; the worktree and outbox paths below are the ones your")
        lines.append("> claim will have. Do not act on a pooled brief you were not handed.")
        lines.append("")
    lines.append("Read this file, do the task, write your report where §4 says. Nothing outside this file is a rule.")
    lines.append("")
    launch.your_tree_lines(lines, b)
    lines.append("## 0b · Acknowledge first, then heartbeat")
    lines.append("")
    lines.append("Before anything else, say you are alive:")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/claims.py ack %s --agent <your-name>" % b["unit"])
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
    lines.append("| edits | **yours only**: change files inside your worktree. MAIN's working tree belongs to the orchestrator, and other workers are often mid-round in it - an edit there blocks every batch gate |")
    lines.append("| sections | %s |" % (", ".join("%s 0x%X-0x%X" % (s, a, e) for s, (a, e, _n) in sorted(rng.items()))
                                        or "(none in splits.txt)"))
    lines.append("| language | %s |" % sources.language_cell(b.get("language")))
    if b["flags"]:
        lines.append("")
        lines.append("The real command line (flags only; `recompile.py` builds the full one):")
        lines.append("")
        lines.append("```")
        # a valued flag can be one token with a space in it (`-pragma cats off`); quote it back so the line
        # reads as the tokens the compiler actually gets
        lines.append(" ".join('"%s"' % t if " " in t else t for t in b["flags"]))
        lines.append("```")
        lines.append("")
        lines.append("The `-i` directories are MAIN's and are searched **in the order shown**, so never run "
                     "this line with MAIN as the working directory: `recompile.py` rewrites them to your "
                     "worktree's (yours first) and that is the compile a measurement has to come from - a "
                     "header you edited in your worktree is otherwise shadowed by MAIN's copy, silently.")
    lines.append("")
    lines.append(sources.language_paragraph(b.get("language")))
    sh = b.get("shared_headers") or []
    lines.append("")
    lines.append("**Shared headers this unit should reuse** (`docs/plan.md` \u00a76.5 rule 1, CLAUDE.md -> "
                 "Repository layout): a type or helper another unit already declares belongs under `include/` - "
                 "include it, never copy it. `include/**` is read-only for you: a change there goes in the outbox's "
                 "`config_requests`, not in your branch.")
    if sh:
        lines.append("")
        lines.append("| header | why | declaration(s) |")
        lines.append("| --- | --- | --- |")
        for h in sh:
            why = []
            if "duplicated" in h["reasons"]:
                why.append("**you define these too - include the header and delete your copies**")
            if "included" in h["reasons"]:
                why.append("already included")
            if "used" in h["reasons"]:
                why.append("you name these")
            if "mentioned" in h["reasons"]:
                why.append("your file header names these")
            names = h["duplicated"] + [n for n in h["used"] if n not in h["duplicated"]]
            shown = ", ".join("`%s`" % n for n in names[:12])
            if len(names) > 12:
                shown += " (+%d more)" % (len(names) - 12)
            lines.append("| `%s` | %s | %s |" % (h["header"], "; ".join(why), shown))
    else:
        lines.append("")
        lines.append("No shared header declares anything this unit names yet. If you need a type or helper another "
                     "unit already uses, do not define it here - put it in the outbox's `config_requests` so it can "
                     "move to `include/` first.")
    lines.append("")
    lines.append("## 2 · The inventory (every symbol the unit owns, and where it stands)")
    lines.append("")
    lines.append("| symbol | address | size | measured % |")
    lines.append("| --- | --- | --- | --- |")
    for s in b["symbols"]:
        pct = s.get("percent")
        mark = "" if (pct is not None and pct >= sources.BAR) else " **<- open**"
        lines.append("| `%s` | 0x%X | %s | %s%s |"
                     % (s["name"], s["address"] or 0, s["size"], "n/a" if pct is None else "%.2f" % pct, mark))
    lines.append("")
    if b.get("dossier"):
        lines.append(sources.render_dossier(b["dossier"]).rstrip())
        lines.append("")
    _dump_asm_hint(lines)
    lines.append("## 3 · What is already known (the unit's own header, verbatim)")
    lines.append("")
    lines.append("```c")
    lines.append(b["header"])
    lines.append("```")
    lines.append("")
    lines.append("## 4 · Where your output goes")
    lines.append("")
    lines.append("* your source, committed **on your branch** (one commit): `%s`" % b["source"])
    lines.append("")
    lines.append("**Self-check before you commit.** The land gate REFUSES a batch that adds any section 6.5")
    lines.append("violation, and a refusal costs the whole round - so run")
    lines.append("`python tools/units/stylelint.py --diff $(git merge-base HEAD main)` and fix what it reports for your files. MAIN")
    lines.append("is the base the gate lints against - linting against your own HEAD misses a violation")
    lines.append("that an already-merged header introduces. The rules that catch a new unit are **rule 2**,")
    lines.append("**rule 9** and **rule 11**: **rule 2** (a declaration belongs in the symbol's owner's header,")
    lines.append("never in your source; `include/unsplit/<band>.h` is the home when no unit owns it), **rule 9**")
    lines.append("(never spell a mangled name - call the owner's member or function through its real signature;")
    lines.append("`tools/units/mangle.py` proves the signature), and **rule 11** (never type a parameter or")
    lines.append("return `void *` - name the real type, or mark the declaration")
    lines.append("`/* untyped: <byte range|opaque handle|caller-owned payload> */`; a cast in a body is not")
    lines.append("a finding, and `grep -rn \"untyped:\" src include` lists every exemption).")
    lines.append("**Rule 13**: a `<Type>_<name>(<Type>* self, ...)` free function is `Type::name` spelled the C way -")
    lines.append("write the member (the map row then carries the mangling; `tools/units/methodize.py <Type>` prints")
    lines.append("the plan), or mark a genuine C function `/* free: <retail C linkage evidenced|SDK C struct> */`.")
    _precommit_lines(lines)
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
    lines.append("**End your turn with your report as the final message.** The orchestrator runs you through the "
                 "headless `claude --agent ... -p` session, which returns to it when your process exits, so your last "
                 "assistant message *is* the handoff; a pane-launched worker delivers the same message plus its outbox, and the "
                 "orchestrator reads them once the pane is idle. There is no completion tool to call - ending on a "
                 "tool call (or saying nothing) hands back an empty result, so make the digest the last thing you "
                 "write.")
    lines.append("")
    lines.append("## 5 · The task")
    lines.append("")
    if task:
        lines.append(task)
    elif b["task_symbols"]:
        lines.append("%d of this unit's symbols are below the %.0f %% bar. Work them **in address order**, "
                     "biggest first where two are equal:" % (b["below_bar"], sources.BAR))
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
        lines.append("**Data this unit may own** (measured second pass): "
                     + ", ".join("%s 0x%X-0x%X (%s)" % (e.get("section"), e.get("start", 0), e.get("end", 0),
                                                        e.get("verdict", "unmeasured")) for e in b["data_queue"])
                     + " - claim what your object *emits* (and only while this unit is its sole referencer); "
                       "propose the rest.")
    _cpp_step(lines)
    _data_step(lines)
    lines.append("")
    lines.append("## 6 · The rules")
    lines.append("")
    lines.append("Measurement loop (build YOUR unit in YOUR worktree; never the split, never MAIN):")
    lines.append("")
    lines.append("```sh")
    lines.append("**Build in your worktree, never in MAIN's.** `build/RMHE08` there is yours alone; MAIN's belongs to")
    lines.append("the orchestrator, and several workers share this machine. The target objects you diff against are read-only")
    lines.append("in MAIN, and your worktree's `build/tools` is seeded with the toolchain (`dtk`, `objdiff-cli`, `sjiswrap`).")
    lines.append("")
    lines.append("**Set the toolchain up by COPYING it - never junction it.** Copy `build/{compilers,binutils,tools}`; junction")
    lines.append("only the read-only `orig/RMHE08/{sys,files,disc}`; `build/RMHE08` must be the worktree's own. A junctioned")
    lines.append("`build/compilers` is dangerous: ninja decides it is missing and re-downloads it *through* the junction into")
    lines.append("MAIN's directory (observed: a `PermissionError` mid-write on `lmgr8c.dll`, MAIN's compiler one write away")
    lines.append("from being corrupted).")
    lines.append("")
    lines.append("**Run the full `ninja` in your worktree, with `build/RMHE08/ok` deleted first.** That target is order-only,")
    lines.append("so it prints `main.dol: OK` forever once its stamp exists; deleting the stamp forces a real hash check. It is")
    lines.append("the only way to prove your worktree builds and that the DOL still matches - and it cannot affect MAIN.")
    lines.append("")
    lines.append("**Before you hand-roll a search, use the two tools that do it mechanically:**")
    lines.append("")
    lines.append("```")
    lines.append("python tools/flags/infer.py %s            # which flags the TARGET object implies" % b["unit"])
    lines.append("python tools/flags/shapesearch.py -u %s --scan 20   # generate/compile/score/rank source variants" % b["unit"])
    lines.append("```")
    lines.append("")
    lines.append("`infer.py` reads the target bytes and names the flags (it is 100 % correct on 36 confident claims and abstains rather than guess); `shapesearch.py` writes variants of your source, compiles each with the real command line into a scratch copy and ranks them by the official metric - it took two functions to 100 % on `Pl/pl_act` in 30 seconds. A shapesearch *miss* is informative: zero differing rows with a 99.9 % score means relocation naming, so the data claim is the fix, not a shape.")
    _measure_lines(lines, b["unit"])
    lines.append("A worker **registers its own unit** (`splits.txt` + `configure.py`, part 2) and builds freely **in its own worktree** - including the full `ninja` - but never runs the **split**, the **link**, `land.py` or `claims.py release`, never edits `symbols.txt` or `CLAUDE.md`, and never commits on `main`. Everything else you need changed goes into the outbox's `config_requests`.")
    launch.teardown_lines(lines)
    lines.extend(config_schema_lines())
    lines.extend(integrator_lines())
    lines.append("")
    lines.append("**Do not fan out subagents.** You work alone in *your* worktree, on *your* branch, and make the "
                 "one commit; you have no tool to spawn agents. If the unit is too big for one lane, say so in "
                 "your report and the orchestrator will split it into more claims.")
    lines.append("")
    lines.append(sources.plan_section(main, "### 6.5 Type and naming discipline"))
    lines.append("")
    lines.append("**Rule 7 at your final path.** Rule 7 has **no exemption and no deferral**: every "
                 "`fn_XXXXXXXX`, `lbl_XXXXXXXX`, `loc_XXXXXXXX` and bare `unk*` identifier in `src/` is a "
                 "finding, whoever owns it. Every symbol the unit **defines** needs a name: use the map's "
                 "real name when the evidence has one, otherwise derive one from the symbol's own body and "
                 "the neighbours' scheme, and when the context supports only a guess, guess and mark it as a "
                 "GUESS in the unit header with the evidence behind it. `fn_XXXXXXXX` is never the resting "
                 "place, and a rename is the map **and** the source in one edit - request the map half of "
                 "your own unit's renames in the outbox (`config_requests`, `kind: rename`, "
                 "old/new/evidence), since you may not edit `symbols.txt` here.\n\nThe **only** grandfather "
                 "is the gate's `--diff`: an existing finding never blocks a landing, so touching a file "
                 "that already carries rule-7 findings is fine - but an ADDED one refuses. That holds for a "
                 "reference to another unit's unrenamed symbol too: the call you add is your batch's new "
                 "finding, so either rename the callee (its map half goes in the outbox) or do not add the "
                 "reference. A file with no bodies is held to the rule like any other, and a "
                 "`rule 7 deferred` comment exempts nothing.")
    lines.append("")
    lines.append(sources.plan_section(main, "## 8. Invariants"))
    return "\n".join(lines).rstrip() + "\n"


def brief_for(main: str, wt: str, unit: str, task: str | None = None, assume_claim: bool = False,
              pool: bool = False) -> tuple[dict, str]:
    """`(brief, text)` for a registered unit spelled either way, rendered against worktree `wt`."""
    b = sources.build(main, wt, unit, task, assume_claim=assume_claim)
    return b, render(main, b, task, pool=pool)


def cluster_index(main: str, key: str, wt: str, units: list[str], briefs: dict, handoff: dict,
                  reason: str) -> str:
    """The one file a cluster lane is handed: the units it holds (all sharing an owner header or a module),
    the per-unit briefs, the cluster's own ack key and the one outbox its branch names."""
    lines = ["# Cluster brief: %s" % key, ""]
    lines.append("Read this file, then each unit brief it lists. Nothing outside these files is a rule.")
    lines.append("")
    launch.your_tree_lines(lines, {"worktree": wt, "main": main})
    lines.append("## 0b · Acknowledge first, then heartbeat")
    lines.append("")
    lines.append("This lane holds **one claim for the whole cluster**, so acknowledge with the cluster's key (the "
                 "per-unit briefs' ack lines name a unit; use this one instead):")
    lines.append("")
    lines.append("```sh")
    lines.append("python tools/units/claims.py ack %s --agent <your-name>" % key)
    lines.append("```")
    lines.append("")
    lines.append("## 1 · The units (%s)" % reason)
    lines.append("")
    lines.append("They share an owner header (or a module), which is why one lane holds them all: a header edit for "
                 "one is a codegen input for the others (playbook 60), and two lanes editing it would collide.")
    lines.append("")
    lines.append("| unit | `.text` | brief |")
    lines.append("| --- | --- | --- |")
    for unit in units:
        rng = sources.splits_range(main, unit).get(".text")
        lines.append("| `%s` | %s | `%s` |" % (unit, "0x%08X-0x%08X" % (rng[0], rng[1]) if rng else "-",
                                               briefs[unit].replace("\\", "/")))
    lines.append("")
    lines.append("## 4 · Where your output goes")
    lines.append("")
    if handoff.get("outbox"):
        lines.append("* `%s` - the one outbox `land.py`'s gate reads for this lane (named after the cluster's "
                     "branch `%s` minus `worker/`); list every unit you touched in it." % (handoff["outbox"],
                                                                                         handoff["branch"]))
        lines.append("* `%s`" % handoff["notes"])
    lines.append("* a ≤ 15-line digest in your reply - per unit, what moved and what is left")
    launch.teardown_lines(lines)
    return "\n".join(lines).rstrip() + "\n"
