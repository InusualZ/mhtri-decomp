# `symdiff` - Side-by-side instruction diff of one symbol, or every symbol's official score for a unit; refuses a stale prebuilt object

<!-- generated from the module docstring of `tools/objdiff/symdiff.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Side-by-side instruction diff for one symbol of a unit.

## Users

profiles (`.claude/agents`) (1); skills (10); CLAUDE.md (1); docs (21); imported by `unitscore`

## CLI

```
python tools/objdiff/symdiff.py -u <unit>                             # list every symbol + score
python tools/objdiff/symdiff.py -u <unit> <symbol> [n] [--all]        # runs objdiff for you
python tools/objdiff/symdiff.py <diff.json> <symbol> [n] [--all]      # reuse an existing diff
python tools/objdiff/symdiff.py -u <unit> --force-stale               # score a stale object anyway
```
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: objects -> diff text.

## Invariants and rules

* `-u <unit>` scores the unit's **prebuilt** object (`build/RMHE08/src/<unit>.o`), so it refuses to print numbers when a source or header under the unit is newer than that object (exit 1, naming the newer file): a lane that measured a stale object reported two "improvements" that were never built. The rule and its arithmetic live in `lib.report` (freshness), shared with `unitscore.py`; `--force-stale` scores it anyway and says so on stderr.
* Left = the -1 object, Right = the -2 object (file mode), or target/base in project mode (`-p . -u <unit>`, where left = target and right = our build).
* The `match` figure printed with `-u` is the **official report metric** (`report generate`'s `fuzzy_match_percent` - what `build/RMHE08/report.json`, `ledger.py` and `land.py` read), with the positional `diff` value shown beside it when it differs. A pre-existing `diff.json` has no unit context, so that path prints the positional value and says so: objdiff's `match_percent` is positional (one inserted/deleted instruction shifts every later instruction, so a single early divergence can report ~0 %) and its relocation default differs from the report's. Read the first divergence, not the percentage.

## Lib dependencies

report, units, repo.

## Test contract

Tier: fixture.
Today's selftest (`tools/objdiff/symdiff_selftest.py`): The first incident this closes: the shared `build/tmp/unitutil/unitutil_report.json` raised `PermissionError [WinError 5]` while another process held it, twice, costing a measurement round (2026-09-28). `session_tmpdir()` gives each invocation its own directory; `retry_transient` covers the residual one-shot lock `os.remove`/`open` can still raise. The second: `-u <unit>` scores the unit's **prebuilt** object, and the stale-object incident is that a lane read it twice without the source having been rebuilt. `stale_reasons` (through `tools/objdiff/freshguard.py`) must name the newer file, cover a header in the include closure, and `main()` must refuse (exit 1, no score printed) rather than report a build that no longer exists.
Target: `tools/tests/objdiff/test_symdiff.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* A bare `-u <unit>` is the **first measurement of a unit in one command**: it lists every symbol the unit owns with its official report score, worst first. It used to raise an IndexError traceback (measured 2026-09-27: a lane lost its first turn to it), and the scores are the same `fuzzy_match_percent` the single-symbol path prints, so the listing and the diff never disagree.
* Every run writes its project, report and diff JSON under a **unique** temp directory (`session_tmpdir()`), removed at exit. The shared `build/tmp/unitutil/unitutil_report.json` was held by another process twice and raised `PermissionError [WinError 5]`, costing a measurement round (2026-09-28); a unique directory (with a transient-lock retry) removes the collision.
