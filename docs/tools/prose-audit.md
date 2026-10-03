# Prose audit: the documentation embedded in `tools/`

Measured at `ec2609b46` with a tokenizer pass over the 168 Python files (docstring lines vs `#` comment lines vs code),
a count of `2026-MM-DD` dates in prose, of incident phrases (`The incident this closes`, `Why this exists`, `measured 2026`,
`used to`, `cost a lane`, `rediscovered`, ...), and of `python tools/...` usage lines inside module docstrings.

## Totals

* 13 768 docstring lines + 5 526 comment lines = **19 294 lines of prose, 17 % of all tool lines** (113 597).
* 5 192 of the docstring lines are **module** docstrings; 49 files open with a docstring of 40 lines or more; the largest are
  `land.py` (162), `stylelint.py` (132), `backlog.py` (118), `attribute.py` (111), `accessextent.py` (108), `slots.py` (104).
* 196 dated references in 42 files and 112 incident phrases: the prose is largely a **rationale log** (which incident
  produced which rule), interleaved with the spec (what the tool does) and the CLI (416 usage lines).
* Comment *blocks* of four lines or more: 51 in `land.py`, 30 in `stylelint.py`, 20 in `mwlink_debugger.py`, 12 in `claims.py`.

## Where each kind belongs (the rule this audit applies)

| kind of prose | today | target |
| --- | --- | --- |
| the one-line purpose | first docstring line | stays (the header template in `README.md`) |
| usage / CLI shape | docstring usage block (416 lines) | the spec's CLI section; the code keeps one line naming the shape |
| the invariants and rules (what is refused and why) | docstring paragraphs, often bold-led | the spec's "Invariants and rules", tightened, one bullet per rule |
| the incident narrative that produced a rule | docstring + comment blocks (196 dates) | the spec keeps the rule and at most the date in parentheses; the narrative goes to `docs/tools/retired.md` (history) or is dropped when `.pi/notes/` already holds it |
| design notes about *other* tools ("X should move here", "land.py does not call this yet") | docstrings | `design.md` / `migration.md`; never in a tool |
| per-function doc of a pure helper | function docstrings | stays when it states a contract; dropped when it restates the name |
| vendored upstream prose (`mwcc-debugger/upstream`, `ninja_syntax`) | as-is | as-is (provenance) |

## Stale or contradictory prose (the code or the tree disagrees with the comment)

1. **Rule count.** `tools/units/stylelint.py:3925-3940` documents rules 1-12 in its table while the tool implements rule 13
   (`rule13_findings:1710`, `build_rule13_context:1562`) and CLAUDE.md says thirteen; `tools/agents/sync_profiles.py:208`
   says "N = 12 today". Both are now wrong; the generated profile block says `rules 1-13`.
2. **The landing path.** `tools/units/land.py` docstring and `unionresolve.py:20-21` say the untracked `.pi/bin/union.py` /
   `applybranch.sh` were "brought into the repo" and that `land.py --branch` applies a branch; `CLAUDE.md` (operational loop,
   step 3) and `tools/units/unionguard.py:4` still describe `.pi/bin/applybranch.sh` + `union.py` as the landing path, and the
   untracked scripts still exist in `MAIN/.pi/bin/` (`applybranch.sh`, `landbranch.sh`, `mergelane.py`, `union.py`) and still end
   in `git add -A` and `grep -v '^ M AGENTS.md'` (a file renamed to `CLAUDE.md` on 2026-09-29). Two landing procedures are documented;
   one is tracked.
3. **The harness.** `tools/units/claims.py` carries 50 `herdr` references (pane list/read/close, `pane_probe:201`,
   `herdr_panes:96`): the pi harness's pane tool. The project moved to Claude Code on 2026-09-29 (`CLAUDE.md`, "Agent harness");
   `slots.py:349` reads live lanes from `~/.claude/sessions/<pid>.json`. The pane-based stall detection in `claims.py` is a dead
   path described as live.
4. **Worker counts.** `handoff.py:5` "Twelve workers in twelve processes", `claims.py:3` "up to twelve externally spawned worker
   agents", `brief.py:3` "Four workers in separate processes" - the policy is six slots (`CLAUDE.md`, "Operational mode").
5. **`src/auto`.** 41 mentions in 9 files (`promote.py` 7, `stylelint.py` 10, `typeregistry.py` 5, `attribute.py` 4, ...); the
   bucket is retired (`docs/plan.md` section 12, `attribute.py:1963` itself says so). `promote.py`'s whole purpose statement
   ("Promote a `src/auto` unit to a real name") describes a flow that no longer exists.
6. **`unitutil.py:12`** "(in this repo today that is `Camellia/camellia`, the only unit with source)" - the tree has hundreds.
7. **Playbook pointers.** `tools/flags/README.md:4` and `infer.py:369` cite `docs/matching.md` rows 39-46 as the playbook;
   `docs/matching.md` is an entry page since the one-file-per-idea split, and ids are `docs/matching/NNN-*.md`.
8. **`docs/pipeline.md` 9.3** says `sectiongap.py` is "being built" and 9.6 says a docs diff maps to no selftest "until the mapping
   lands (R7)"; both landed (`sectiongap_selftest.py`, `selftest.py:116-129` `SOURCE_ENTRIES`). Out of `tools/`, but the spec
   index replaces that roster.
9. **Design notes parked in docstrings.** `sharedfiles.py:21-23` "the next writers that should move here: symedit, dataqueue" (never
   moved); `linkorder.py:2902` "`land.py` does not call this yet"; `callees.py:2141` a paragraph on why it lives in `tools/units/`
   and not `tools/objdiff/`; `unitscore.py:1292` "Reuse, not re-implementation" listing which module owns what. These are
   `design.md` content.
10. **Snapshots committed as docs.** `tools/flags/infer-run.md` (an `infer.py --markdown` run) and `tools/units/relocaudit-findings.md`
    (a sweep at HEAD `f7b49ff`) are dated outputs; both are stale the day after and belong under `.pi/notes/` or nowhere.
11. **`docs/plan.md 7.N` / roadmap citations.** 183 occurrences in 30 files cite roadmap item numbers (`7.2`, `7.5`, `7.9`, `7.12`,
    `7.17`, `7.21`, ...). They are history pointers, not rules; the spec carries the rule and the citation goes.
12. **Numbers that move.** `callers.py:2193` "245 258 references over 54 256 target addresses ... 8-10 s", `pairgap.py:1080` "12,777 of
    the 20,507 rows", `rescue.py:3739` "193 of them by 2026-09-27", `backlog.py:2007` "266 outboxes / 703 requests" - measurements
    of one day written as facts.
13. **The `--selftest` contract described two ways.** `selftest.py:1402-1412` says a tool is tested by a standalone file *or* a
    `--selftest` flag and collapses delegating pairs; `pairgap_selftest.py` and `subproc_selftest.py` exist only to be discovered
    ("this half exists so the discovery ... sees a standalone"), i.e. the convention is documented as a workaround of its own runner.

## Per-file table

Sorted by prose volume (docstring + comment lines). "module doc" is the opening docstring's line count; "dated" counts
`2026-MM-DD` in prose; "usage lines" are `python tools/...` lines in the module docstring.

| file | lines | module doc | all doc | `#` lines | `#` blocks (>=4) | prose % | dated | incident phrases | usage lines |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `tools/units/land.py` | 5172 | 162 | 775 | 536 | 51 | 28 | 60 | 4 | 5 |
| `tools/units/stylelint.py` | 4924 | 132 | 747 | 337 | 30 | 24 | 20 | 5 | 7 |
| `tools/units/slots.py` | 3384 | 104 | 606 | 200 | 8 | 26 | 4 | 3 | 9 |
| `tools/mwlink_debugger.py` | 3743 | 88 | 412 | 254 | 20 | 19 | 0 | 0 | 0 |
| `tools/units/claims.py` | 2847 | 40 | 433 | 194 | 12 | 24 | 14 | 5 | 6 |
| `tools/units/backlog.py` | 2036 | 118 | 389 | 94 | 9 | 27 | 7 | 0 | 7 |
| `tools/units/recompile.py` | 1248 | 87 | 367 | 64 | 5 | 39 | 0 | 2 | 1 |
| `tools/units/datagap.py` | 2816 | 42 | 288 | 139 | 7 | 17 | 10 | 0 | 7 |
| `tools/units/attribute.py` | 1361 | 111 | 372 | 46 | 2 | 34 | 4 | 1 | 6 |
| `tools/units/brief.py` | 2479 | 45 | 334 | 82 | 4 | 18 | 7 | 0 | 3 |
| `tools/units/accessextent.py` | 2064 | 108 | 278 | 127 | 11 | 21 | 0 | 0 | 1 |
| `tools/units/vtableaudit.py` | 1380 | 92 | 302 | 66 | 3 | 30 | 5 | 2 | 10 |
| `tools/splits/tudiscover.py` | 2168 | 73 | 262 | 99 | 5 | 18 | 1 | 1 | 0 |
| `tools/units/queue.py` | 1569 | 47 | 246 | 78 | 5 | 23 | 5 | 1 | 4 |
| `tools/units/callers.py` | 1848 | 68 | 235 | 68 | 4 | 18 | 0 | 2 | 4 |
| `tools/splits/splitcheck.py` | 3578 | 45 | 203 | 93 | 2 | 9 | 0 | 0 | 3 |
| `tools/units/mergebranch.py` | 1583 | 63 | 183 | 109 | 7 | 20 | 10 | 3 | 3 |
| `tools/units/langcheck.py` | 1188 | 65 | 178 | 87 | 4 | 24 | 1 | 1 | 5 |
| `tools/units/verifyunit.py` | 670 | 69 | 223 | 35 | 1 | 43 | 2 | 2 | 1 |
| `tools/units/promote.py` | 1223 | 91 | 181 | 46 | 1 | 20 | 1 | 0 | 4 |
| `tools/project.py` | 2176 | 0 | 7 | 217 | 6 | 12 | 0 | 0 | 0 |
| `tools/units/flipcheck.py` | 844 | 36 | 168 | 47 | 4 | 28 | 0 | 2 | 3 |
| `tools/splits/dataattach.py` | 2377 | 37 | 159 | 55 | 0 | 10 | 0 | 1 | 5 |
| `tools/units/recompile_selftest.py` | 1214 | 20 | 159 | 52 | 1 | 20 | 0 | 0 | 1 |
| `tools/units/dataclaim.py` | 1616 | 51 | 143 | 66 | 2 | 15 | 0 | 2 | 7 |
| `tools/unitutil.py` | 739 | 18 | 180 | 22 | 1 | 31 | 0 | 1 | 0 |
| `tools/mwcc-debugger/mwcc_debugger.py` | 2084 | 22 | 91 | 102 | 6 | 10 | 0 | 0 | 0 |
| `tools/units/m2cinput.py` | 727 | 71 | 142 | 37 | 2 | 28 | 0 | 0 | 3 |
| `tools/units/undefrefs.py` | 743 | 57 | 134 | 35 | 0 | 25 | 1 | 1 | 4 |
| `tools/units/dataqueue.py` | 902 | 55 | 133 | 32 | 1 | 20 | 0 | 0 | 6 |
| `tools/objdiff/pairgap.py` | 877 | 66 | 127 | 37 | 0 | 21 | 1 | 0 | 7 |
| `tools/selftest.py` | 885 | 63 | 110 | 49 | 1 | 20 | 2 | 2 | 7 |
| `tools/units/callees.py` | 940 | 50 | 113 | 44 | 1 | 18 | 0 | 1 | 2 |
| `tools/symbols/symedit.py` | 1194 | 37 | 105 | 50 | 6 | 14 | 0 | 2 | 0 |
| `tools/units/typeregistry.py` | 985 | 41 | 104 | 48 | 0 | 17 | 0 | 0 | 5 |
| `tools/units/dossier.py` | 937 | 33 | 97 | 51 | 0 | 18 | 1 | 1 | 2 |
| `tools/flags/infer.py` | 1055 | 47 | 84 | 63 | 3 | 16 | 0 | 2 | 4 |
| `tools/splits/applysplits.py` | 1899 | 35 | 93 | 45 | 1 | 8 | 0 | 0 | 7 |
| `tools/units/vtslot.py` | 549 | 56 | 119 | 19 | 0 | 27 | 0 | 0 | 6 |
| `tools/mwcc-debugger/versions.py` | 551 | 64 | 83 | 50 | 6 | 26 | 0 | 0 | 0 |
| `tools/units/relocaudit.py` | 396 | 49 | 99 | 29 | 0 | 36 | 1 | 2 | 5 |
| `tools/units/tooling.py` | 989 | 44 | 66 | 50 | 3 | 13 | 2 | 3 | 7 |
| `tools/units/unionresolve.py` | 434 | 37 | 82 | 31 | 0 | 29 | 2 | 2 | 1 |
| `tools/units/poolseams.py` | 623 | 32 | 89 | 22 | 0 | 20 | 1 | 2 | 4 |
| `tools/agents/sync_profiles.py` | 366 | 48 | 85 | 19 | 1 | 32 | 3 | 3 | 4 |
| `tools/units/unwindcut.py` | 695 | 53 | 81 | 23 | 0 | 17 | 0 | 0 | 3 |
| `tools/flags/shapes.py` | 817 | 19 | 76 | 24 | 0 | 14 | 0 | 0 | 0 |
| `tools/objdiff/unitscore.py` | 487 | 43 | 85 | 14 | 1 | 22 | 0 | 4 | 6 |
| `tools/units/unionprose.py` | 416 | 26 | 74 | 24 | 1 | 26 | 1 | 0 | 1 |
| `tools/units/vtableaudit_selftest.py` | 728 | 23 | 45 | 52 | 2 | 15 | 0 | 1 | 2 |
| `tools/units/unionguard.py` | 608 | 44 | 78 | 15 | 1 | 17 | 4 | 3 | 3 |
| `tools/elf/objextab.py` | 483 | 42 | 69 | 23 | 0 | 22 | 0 | 0 | 2 |
| `tools/git/commitlint.py` | 435 | 51 | 67 | 24 | 1 | 24 | 0 | 0 | 5 |
| `tools/symbols/phantom.py` | 839 | 48 | 76 | 15 | 0 | 13 | 0 | 0 | 5 |
| `tools/units/measure.py` | 559 | 42 | 80 | 11 | 0 | 18 | 0 | 0 | 1 |
| `tools/units/preflight_selftest.py` | 510 | 44 | 72 | 18 | 0 | 20 | 1 | 2 | 1 |
| `tools/units/sectiongap.py` | 317 | 54 | 84 | 6 | 0 | 32 | 0 | 0 | 4 |
| `tools/mwcc-debugger/locate/verify_pcode.py` | 625 | 33 | 62 | 27 | 1 | 16 | 0 | 0 | 0 |
| `tools/objdiff/relocdiff.py` | 441 | 48 | 79 | 10 | 0 | 22 | 0 | 2 | 7 |
| `tools/units/flipcheck_selftest.py` | 552 | 29 | 40 | 49 | 1 | 18 | 0 | 0 | 2 |
| `tools/units/worktreehook.py` | 629 | 45 | 73 | 13 | 0 | 16 | 1 | 1 | 3 |
| `tools/flags/shapesearch.py` | 578 | 40 | 66 | 17 | 0 | 16 | 0 | 1 | 8 |
| `tools/units/attribute_selftest.py` | 907 | 15 | 18 | 64 | 1 | 10 | 3 | 3 | 2 |
| `tools/splits/dataorder.py` | 561 | 22 | 68 | 13 | 0 | 17 | 0 | 1 | 0 |
| `tools/units/playbook.py` | 731 | 35 | 50 | 30 | 1 | 12 | 0 | 1 | 4 |
| `tools/objdiff/symdiff.py` | 284 | 33 | 76 | 3 | 0 | 31 | 2 | 2 | 4 |
| `tools/units/handoff.py` | 452 | 27 | 47 | 32 | 2 | 19 | 2 | 0 | 4 |
| `tools/units/rescue.py` | 541 | 39 | 51 | 28 | 1 | 17 | 1 | 0 | 2 |
| `tools/units/verifyunit_selftest.py` | 557 | 23 | 58 | 21 | 0 | 16 | 0 | 0 | 1 |
| `tools/units/linkorder.py` | 758 | 38 | 51 | 27 | 1 | 12 | 0 | 2 | 3 |
| `tools/units/sharedfiles.py` | 369 | 24 | 68 | 10 | 0 | 25 | 0 | 0 | 1 |
| `tools/units/lane.py` | 385 | 31 | 61 | 16 | 1 | 22 | 4 | 1 | 4 |
| `tools/mwcc-debugger/upstream/mwcc_debugger.py` | 1742 | 0 | 0 | 76 | 0 | 5 | 0 | 0 | 0 |
| `tools/units/recordmerge.py` | 549 | 39 | 51 | 25 | 1 | 16 | 0 | 0 | 3 |
| `tools/units/reportdiff.py` | 631 | 37 | 53 | 23 | 0 | 13 | 0 | 1 | 2 |
| `tools/units/backlog_selftest.py` | 976 | 11 | 12 | 63 | 3 | 8 | 3 | 1 | 2 |
| `tools/units/mangle.py` | 316 | 24 | 52 | 21 | 1 | 26 | 0 | 0 | 4 |
| `tools/units/methodize.py` | 635 | 43 | 61 | 11 | 0 | 12 | 1 | 0 | 7 |
| `tools/units/promote_batch.py` | 514 | 45 | 56 | 16 | 0 | 15 | 0 | 0 | 5 |
| `tools/symbols/dumpmap.py` | 633 | 40 | 60 | 11 | 0 | 12 | 0 | 0 | 0 |
| `tools/units/wtsafe.py` | 290 | 22 | 64 | 3 | 0 | 27 | 1 | 1 | 2 |
| `tools/units/measure_selftest.py` | 531 | 21 | 57 | 7 | 0 | 13 | 0 | 0 | 1 |
| `tools/units/ledger.py` | 554 | 19 | 53 | 10 | 0 | 13 | 0 | 0 | 3 |
| `tools/units/subproc.py` | 235 | 26 | 55 | 8 | 0 | 30 | 0 | 1 | 0 |
| `tools/objdiff/freshguard.py` | 189 | 26 | 55 | 6 | 0 | 37 | 0 | 2 | 0 |
| `tools/units/dataseams.py` | 334 | 19 | 57 | 3 | 0 | 21 | 0 | 1 | 2 |
| `tools/units/undefrefs_selftest.py` | 420 | 14 | 22 | 37 | 2 | 16 | 0 | 0 | 2 |
| `tools/agents/ideas.py` | 524 | 31 | 46 | 12 | 1 | 13 | 1 | 1 | 7 |
| `tools/git/guard.py` | 197 | 21 | 53 | 3 | 0 | 34 | 0 | 1 | 0 |
| `tools/objdiff/unitscore_selftest.py` | 381 | 22 | 30 | 25 | 2 | 16 | 0 | 1 | 2 |
| `tools/elf/objalign.py` | 267 | 28 | 41 | 12 | 0 | 23 | 0 | 0 | 2 |
| `tools/objdiff/metric_selftest.py` | 434 | 23 | 43 | 10 | 0 | 14 | 0 | 1 | 1 |
| `tools/units/relocaudit_selftest.py` | 253 | 19 | 29 | 23 | 0 | 23 | 0 | 0 | 2 |
| `tools/agents/sync_playbook_index.py` | 370 | 26 | 41 | 8 | 1 | 15 | 0 | 0 | 6 |
| `tools/flags/infer_selftest.py` | 404 | 14 | 29 | 20 | 0 | 15 | 0 | 0 | 2 |
| `tools/git/prepcommit.py` | 333 | 17 | 42 | 6 | 0 | 17 | 0 | 1 | 1 |
| `tools/flags/variants/camellia.py` | 284 | 12 | 26 | 19 | 0 | 17 | 0 | 0 | 0 |
| `tools/units/promote_selftest.py` | 592 | 21 | 25 | 20 | 0 | 8 | 0 | 0 | 2 |
| `tools/flags/tryvar.py` | 184 | 33 | 42 | 1 | 0 | 26 | 0 | 1 | 5 |
| `tools/units/m2cinput_selftest.py` | 388 | 11 | 16 | 27 | 1 | 13 | 0 | 0 | 1 |
| `tools/flags/mwcc_matrix.py` | 170 | 24 | 39 | 2 | 0 | 27 | 0 | 1 | 4 |
| `tools/units/escape.py` | 292 | 23 | 33 | 8 | 0 | 16 | 0 | 1 | 6 |
| `tools/units/unwindcut_selftest.py` | 295 | 15 | 28 | 13 | 0 | 16 | 0 | 0 | 2 |
| `tools/units/vtslot_selftest.py` | 302 | 16 | 26 | 14 | 0 | 15 | 0 | 0 | 2 |
| `tools/git/commitlint_selftest.py` | 295 | 9 | 12 | 27 | 0 | 16 | 0 | 0 | 2 |
| `tools/ninja_syntax.py` | 253 | 5 | 15 | 24 | 2 | 18 | 0 | 0 | 0 |
| `tools/units/checklf.py` | 153 | 24 | 37 | 1 | 0 | 29 | 0 | 2 | 4 |
| `tools/units/declclash.py` | 230 | 24 | 30 | 8 | 0 | 19 | 0 | 0 | 4 |
| `tools/agents/ideas_demo.py` | 305 | 21 | 30 | 4 | 0 | 12 | 0 | 0 | 0 |
| `tools/objdiff/relocdiff_selftest.py` | 215 | 16 | 22 | 12 | 0 | 19 | 0 | 0 | 2 |
| `tools/mwcc-debugger/locate/verify_pcode_selftest.py` | 289 | 19 | 22 | 11 | 0 | 13 | 0 | 0 | 1 |
| `tools/units/promote_batch_selftest.py` | 288 | 18 | 19 | 14 | 0 | 13 | 0 | 0 | 2 |
| `tools/units/symbolpreflight.py` | 412 | 22 | 31 | 2 | 0 | 9 | 0 | 0 | 1 |
| `tools/units/tooling_selftest.py` | 236 | 16 | 17 | 16 | 1 | 16 | 2 | 1 | 2 |
| `tools/agents/sync_profiles_selftest.py` | 240 | 19 | 20 | 12 | 0 | 15 | 1 | 0 | 2 |
| `tools/rso/inventory.py` | 154 | 27 | 28 | 4 | 0 | 24 | 0 | 0 | 3 |
| `tools/units/sectiongap_selftest.py` | 245 | 11 | 20 | 12 | 0 | 15 | 0 | 0 | 2 |
| `tools/unitutil_selftest.py` | 220 | 18 | 21 | 11 | 0 | 17 | 0 | 1 | 1 |
| `tools/units/lanecmd.py` | 161 | 16 | 26 | 5 | 0 | 22 | 0 | 1 | 1 |
| `tools/objdiff/slotmap.py` | 157 | 16 | 29 | 1 | 0 | 21 | 0 | 0 | 2 |
| `tools/splits/gen_trk_vectors.py` | 119 | 27 | 28 | 1 | 0 | 28 | 0 | 2 | 1 |
| `tools/units/linkorder_selftest.py` | 319 | 10 | 13 | 15 | 0 | 10 | 0 | 0 | 1 |
| `tools/rso/symbols.py` | 198 | 18 | 22 | 5 | 0 | 15 | 0 | 0 | 2 |
| `tools/splits/dump_asm.py` | 114 | 20 | 26 | 1 | 0 | 27 | 0 | 0 | 3 |
| `tools/agents/edit.py` | 212 | 19 | 25 | 1 | 0 | 15 | 0 | 1 | 3 |
| `tools/mwcc-debugger/fetch_gdb.py` | 147 | 18 | 20 | 6 | 1 | 20 | 0 | 2 | 1 |
| `tools/objdiff/symdiff_selftest.py` | 197 | 13 | 14 | 10 | 0 | 14 | 1 | 1 | 1 |
| `tools/agents/profileprobe.py` | 136 | 18 | 19 | 3 | 0 | 17 | 0 | 0 | 1 |
| `tools/units/recordmerge_selftest.py` | 245 | 10 | 11 | 11 | 0 | 10 | 0 | 0 | 1 |
| `tools/objdiff/freshguard_selftest.py` | 123 | 15 | 16 | 5 | 0 | 20 | 0 | 0 | 1 |
| `tools/agents/ideas_selftest.py` | 365 | 7 | 8 | 12 | 0 | 6 | 0 | 0 | 1 |
| `tools/splits/matchinggain.py` | 139 | 14 | 19 | 1 | 0 | 17 | 0 | 0 | 2 |
| `tools/git/guard_selftest.py` | 192 | 6 | 8 | 11 | 0 | 12 | 0 | 0 | 1 |
| `tools/units/checklf_selftest.py` | 135 | 9 | 10 | 9 | 0 | 17 | 0 | 0 | 2 |
| `tools/units/playbook_selftest.py` | 264 | 9 | 10 | 9 | 0 | 8 | 0 | 0 | 2 |
| `tools/agents/sync_playbook_index_selftest.py` | 210 | 8 | 10 | 8 | 0 | 10 | 0 | 0 | 1 |
| `tools/flags/shapes_selftest.py` | 198 | 16 | 17 | 1 | 0 | 11 | 0 | 0 | 1 |
| `tools/mwcc-debugger/locate/pass_points.py` | 97 | 14 | 15 | 3 | 0 | 20 | 0 | 0 | 0 |
| `tools/units/ledger_selftest.py` | 190 | 11 | 14 | 4 | 0 | 11 | 0 | 0 | 1 |
| `tools/flags/optsweep.py` | 78 | 12 | 14 | 3 | 0 | 27 | 0 | 0 | 4 |
| `tools/units/queue_selftest.py` | 35 | 15 | 16 | 1 | 0 | 61 | 0 | 0 | 1 |
| `tools/flags/shapesearch_selftest.py` | 116 | 13 | 14 | 2 | 0 | 17 | 0 | 0 | 2 |
| `tools/flags/frame.py` | 75 | 12 | 14 | 1 | 0 | 24 | 0 | 0 | 5 |
| `tools/splits/dump_asm_selftest.py` | 139 | 11 | 14 | 1 | 0 | 14 | 0 | 0 | 1 |
| `tools/units/typeregistry_selftest.py` | 33 | 13 | 14 | 1 | 0 | 58 | 0 | 0 | 1 |
| `tools/units/unionguard_selftest.py` | 34 | 13 | 14 | 1 | 0 | 56 | 1 | 1 | 2 |
| `tools/mwcc-debugger/locate/dissect.py` | 168 | 11 | 12 | 2 | 0 | 9 | 0 | 0 | 0 |
| `tools/mwcc-debugger/make_port.py` | 1083 | 12 | 13 | 1 | 6 | 1 | 0 | 1 | 0 |
| `tools/spawnretry.py` | 50 | 11 | 14 | 0 | 0 | 34 | 1 | 1 | 0 |
| `tools/transform_dep.py` | 84 | 0 | 0 | 14 | 1 | 20 | 0 | 0 | 0 |
| `tools/units/dossier_selftest.py` | 33 | 12 | 13 | 1 | 0 | 54 | 0 | 0 | 2 |
| `tools/elf/dwarfmap.py` | 231 | 6 | 8 | 4 | 0 | 6 | 0 | 0 | 0 |
| `tools/units/declclash_selftest.py` | 137 | 10 | 11 | 1 | 0 | 10 | 0 | 0 | 1 |
| `tools/download_tool.py` | 152 | 0 | 0 | 11 | 1 | 9 | 0 | 0 | 0 |
| `tools/mwcc-debugger/locate/extract_upstream_tables.py` | 55 | 9 | 10 | 1 | 0 | 23 | 0 | 0 | 0 |
| `tools/units/accessextent_selftest.py` | 30 | 9 | 10 | 1 | 0 | 48 | 0 | 0 | 2 |
| `tools/units/callees_selftest.py` | 30 | 9 | 10 | 1 | 0 | 48 | 0 | 0 | 2 |
| `tools/units/callers_selftest.py` | 30 | 9 | 10 | 1 | 0 | 48 | 0 | 0 | 2 |
| `tools/units/unionresolve_selftest.py` | 25 | 9 | 10 | 1 | 0 | 55 | 0 | 0 | 2 |
| `tools/decompctx.py` | 178 | 0 | 0 | 10 | 1 | 7 | 0 | 0 | 0 |
| `tools/objdiff/pairgap_selftest.py` | 20 | 7 | 8 | 1 | 0 | 53 | 0 | 0 | 1 |
| `tools/agents/edit_selftest.py` | 138 | 3 | 4 | 3 | 0 | 6 | 0 | 0 | 1 |
| `tools/units/subproc_selftest.py` | 17 | 5 | 6 | 1 | 0 | 54 | 0 | 0 | 0 |
| `tools/selftest_site/sitecustomize.py` | 16 | 5 | 6 | 0 | 0 | 40 | 0 | 0 | 0 |
| `tools/changes_fmt.py` | 162 | 0 | 0 | 2 | 0 | 2 | 0 | 0 | 0 |
| `tools/spawnretry_selftest.py` | 85 | 1 | 1 | 1 | 0 | 3 | 0 | 0 | 0 |
| `tools/elf/elfsect.py` | 44 | 0 | 0 | 1 | 0 | 2 | 0 | 0 | 0 |
| `tools/__init__.py` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
