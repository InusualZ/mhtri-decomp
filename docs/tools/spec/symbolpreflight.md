# `symbolpreflight` - Pre-flight one symbol: boundary, owner range, configured lib/flags, collision kind/severity, draft splits/configure blocks

<!-- generated from the module docstring of `tools/units/symbolpreflight.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Pre-flight one symbol before any source is written: who owns the address, and what would registration touch?

## Users

skills (2); CLAUDE.md (2); docs (6); imported by `dataclaim`, `dataqueue`, `ledger`, `linkorder`

## CLI

```
python tools/units/symbolpreflight.py <address|name> [--json]
symbol      name, section, address, type, size
boundary    the previous and next symbol in that section, and whether this symbol's end meets the next
symbol's start (a delta is a boundary that will have to be explained)
owner       the splits.txt unit whose range covers the address, if any, and the range itself
configured  that unit's lib, mw_version, cflags and Matching/NonMatching flag from configure.py
collision   the kind from the skill's stop list, its severity, and the analysis still owed
drafts      a splits.txt block and a configure.py entry, as text - nothing is written
```
Flags: `--json`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: map, splits, configure.py, config.json -> report.

## Invariants and rules

* Offline and bounded: it reads `config/RMHE08/symbols.txt` (through `tools/symbols/symedit.py`'s parser), `config/RMHE08/splits.txt`, `configure.py` and, when it exists, `build/RMHE08/config.json` (dtk's split output, where the per-function `auto_*` objects live).
* What it answers, in the order the decompile skill needs it:
* Severities are the skill's four: `escalate` (an anomaly that should not exist), `approve` (analyse, propose, then wait for approval), `proceed` (the owner's claim is weak or absent) and `never touch`.
* Nothing here decides anything: it reports the mechanical facts and names the analysis that remains.

## Lib dependencies

project.Ownership, project.configure.

## Test contract

Tier: fixture (the derived-row suite becomes a smoke check of agreement with Ownership).
Today's selftest (`tools/units/preflight_selftest.py`): No build and no shelling out: it imports the pre-flight module and checks its report against real repo data, so a symbols.txt / splits.txt / configure.py parser regression is caught in milliseconds. Nothing here reads `build/`: every row is derived from tracked config/ files, so the run (and the count) is identical in MAIN, which has a build tree, and in a fresh worktree, which has none. **Two kinds of row, and why.**
Target: `tools/tests/units/test_symbolpreflight.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

`preflight_selftest.py` pins live-tree symbols (`_rom_copy_info`, `memmove`)
