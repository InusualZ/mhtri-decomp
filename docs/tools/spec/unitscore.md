# `unitscore` - Every symbol of a unit from one report read (or one `--measure` call), with a freshness verdict that refuses a stale report

<!-- generated from the module docstring of `tools/objdiff/unitscore.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Every symbol of one unit, from **one** report read - and a freshness verdict that refuses a stale one.

## Users

profiles (`.claude/agents`) (6); skills (3); docs (9)

## CLI

```
python tools/objdiff/unitscore.py <unit>                    # every symbol, worst first (0 objdiff calls)
python tools/objdiff/unitscore.py <unit> --measure          # score the objects on disk (exactly 1 call)
python tools/objdiff/unitscore.py <unit> --threshold 99.9   # only the rows below the threshold
python tools/objdiff/unitscore.py <unit> --json             # the whole record, mtimes included
python tools/objdiff/unitscore.py <unit> --force-stale      # score anyway, STALE stays in the output
python tools/objdiff/unitscore.py <unit> --refresh          # ninja build/RMHE08/report.json first (costs a build)
python tools/objdiff/unitscore.py <unit> --measure --refresh   # ninja the unit's object first, then one call
python tools/objdiff/unitscore.py <unit> --baseline B.json  # the unit's rows against a claim-time report, by address
python tools/objdiff/unitscore.py --baseline B.json         # every unit of the report against B (exit 1 on a down row)
python tools/objdiff/unitscore.py --selftest
```
Flags: `--baseline`, `--force-stale`, `--json`, `--measure`, `--refresh`, `--report`, `--selftest`, `--threshold`.
Exit codes: **Exit status is the answer**: 0 the numbers are printable (current, or explicitly forced), 1 the freshness guard refused, 2 a usage or input error (no report, an unreadable report, the unit missing from it) - a traceback is never the answer.
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: report.json or objects -> table/JSON.

## Invariants and rules

* `<unit>` is the path from the repository root (`quest/arenatask`, `Pl/pl_act`); the extension may be omitted, and `src/`/`main/`/`build/RMHE08/...` prefixes are accepted.
* **Reuse, not re-implementation.** The unit and its paths come from `lib.units.Unit.resolve` in the invocation's tree (`lib.repo.repo_root`); the report entry's identity and scoring conventions come from `tools/units/verifyunit.py` (`report_unit`, `unit_stem`, and its "a function with no `fuzzy_match_percent` key is 0 %, not 100 %" reading); the one-report score comes from `unitutil.report_functions` through `tools/objdiff/symdiff.py`'s scratch/retry helpers. There is one implementation of each, and this file holds none of them. The staleness arithmetic itself (the three mtimes, the include closure, the verdict) is `tools/objdiff/freshguard.py`, shared with `symdiff.py`, so "which file is newer" has one answer.
* **Exit status is the answer**: 0 the numbers are printable (current, or explicitly forced), 1 the freshness guard refused, 2 a usage or input error (no report, an unreadable report, the unit missing from it) - a traceback is never the answer.
* **`--baseline B` pairs rows by address, never by name (2026-10-04).** Every row is keyed by its
  `metadata.virtual_address` (`lib.report.diff_by_address`), so a row renamed or moved to another unit since the
  claim-time report still pairs; the output lists `UP`/`DOWN` (both names when they differ), `NEW` and `REMOVED`
  rows and a `renamed` count, and a `DOWN` row is exit 1. With a unit, the current side is that unit's rows (from
  the report, or the objects with `--measure`, after the freshness guard) and the baseline side every baseline row
  at one of their addresses plus the rows the unit held; with no unit, every unit of `--report` (no freshness
  guard: the current report's path and mtime are printed). It replaces the lanes' `cmp.py`, which keyed a row by
  `(unit, section offset)`: on L1's round-2 pair (slot1 `before-report.json` against its final report) `cmp.py`
  read 200 up / 70 down, every one of the 70 a rename or a move whose row did not fall; `--baseline` reads 130 up,
  0 down, 132 renamed, 20 507 paired.
* **The freshness policy is `lib.artifacts`' (2026-10-04).** Default `refuse` (unchanged); `--force-stale` is `FRESH=warn`; `FRESH=auto` rebuilds what the score reads (`--refresh`'s target, which is now the registry's `report` / `objects` command) only when the guard would refuse, then scores. Fresh: output unchanged.
* **`--refresh` costs a build, and says so.** It runs `ninja build/RMHE08/report.json` in the unit's tree before the read: the report depends on `all_source`, so every stale object of the tree is compiled first (and a changed map or `splits.txt` re-splits). With `--measure` it builds only the unit's object, the one thing that mode reads. The record carries `refreshed: {target, seconds, ok, error}`; a failed build is exit 2 with ninja's tail, and the freshness guard still runs on the rebuilt files. `--refresh --report R` is a usage error: an arbitrary report is not a ninja target.

## Lib dependencies

report, units, repo, proc.

## Test contract

Tier: fixture (fake repo outside the tree).
Today's selftest (`tools/objdiff/unitscore_selftest.py`): **Fixtures only.** The tree is a fake repository in the system temp (`configure.py`, `src/demo/unit.cpp` with a header closure, `build/RMHE08/obj/` so `unitutil._versions()` finds the version, and a hand-written `report.json`), and every mtime is set with `os.utime`, so the check count and the outcome are identical in MAIN, in a fresh worktree and in a slot. The fake tree is created outside the repository on purpose: a temp directory inside it is inside a git worktree, and `unitutil.repo_root()` would then resolve the real tree and read the real report instead of the fixture. The fixture is deliberately **not** a git worktree, and does not need to be. `unitutil.repo_root()` roots a run at the *invocation's* tree - its git worktree when there is one, else the `cwd` when the `cwd` is a tree at all - so a run with `cwd=<fixture>` resolves the fixture even though `git rev-parse` answers nothing. It used to fall back to the tool's own directory and silently score the real build; the fix is `unitutil.repo_root(start=)` / `resolve_unit(spec, root=)` and the `cwd`-that-is-a-tree rule.
`--baseline` (block 15): a renamed row and a row moved in from a neighbour pair by address, a fall is DOWN and exits 1,
a lost row is REMOVED, the no-unit mode, no down row exits 0, an unreadable baseline exits 2; `tests/lib/test_report.py`
pins `diff_by_address`. Keying by name instead fails 6 checks.
Target: `tools/tests/objdiff/test_unitscore.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Why this tool exists.** Scoring a unit one symbol at a time costs one objdiff invocation per symbol, and a 70-row unit is 70 of them; lanes kept hand-writing the same driver (`build/tmp/unitreport.py`, `build/scratch/score.py`, `.pi/scratch/score.py`) and the re-inventions produced *wrong* numbers - one scored a stale object twice and reported two false improvements. A report over the tree already carries every symbol of every unit; the only thing missing was a reader that does not lie about its freshness.
* **The hazard this is built around.** `build/RMHE08/report.json` is an **order-only** target of `all_source` in `build.ninja`: after a source edit, `ninja build/RMHE08/report.json` prints "no work to do" and the file still holds the PREVIOUS build's scores. That cost one lane three iterations that looked like "all new functions score 0 %". So every run measures and prints three mtimes - the report's, the unit's object's and the newest source under the unit - and **refuses to print numbers** (exit 1) when the report predates either of the other two. `--force-stale` overrides the refusal; the STALE verdict and its reasons stay in the output and in the JSON (`"freshness"`), so a forced run is never mistakable for a clean one.
* `--measure` is the escape hatch for exactly that case: instead of the project report it scores the unit's already-built object pair with **one** `objdiff report generate` (the same primitive `symdiff.py -u <unit>` and `measure.py` use, ~0.2 s for a 19-symbol unit). The guard then applies to the object: a source newer than the object means the object was never rebuilt, and that is refused the same way (that is the incident above, seen from the other side). Neither mode ever issues N calls: report mode issues zero.
