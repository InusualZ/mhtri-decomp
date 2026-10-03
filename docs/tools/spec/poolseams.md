# `poolseams` - The literal pool as TU evidence: groups of registered units that share a pool entry (fold candidates), from datagap's census

<!-- generated from the module docstring of `tools/units/poolseams.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

poolseams.py - the literal pool as TU-seam evidence: which registered units are ONE original translation unit.

## Users

skills (3); docs (6); imported by `attribute`, `datagap`, `flipcheck`, `sectiongap`, `tudiscover`

## CLI

```
python tools/units/poolseams.py                 # the census: groups of registered units that share a pool
python tools/units/poolseams.py --unit <unit>   # the group a unit belongs to (or "no pool-sharing group")
python tools/units/poolseams.py --json out.json
python tools/units/poolseams.py --selftest
```
Flags: `--json`, `--no-values`, `--root`, `--selftest`, `--top`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: datagap census, splits, map, DOL -> groups.

## Invariants and rules

* This module turns that into data. It reads nothing itself: the references come from `datagap.census` (the one relocation reader over the registered units' TARGET objects), the claims from `splits.txt`, the symbol types from the map, the values (optional) from the retail DOL. Everything below is a pure function of those, so the `--selftest` fixtures need no build tree.
* (`datagap.py --pool-seams` is the same census; `tudiscover.py at`, `datagap.py` deferral classes, `flipcheck.py`, `sectiongap.py` and `brief.py` consume `group_of`.)
* What is and is not evidence (measured exceptions, docs/pool-seams.md section 4):
* **literal** - an `.sdata2` object of 4 or 8 bytes, or an `.sdata` string: an edge.
* **non-literal** - any other `.sdata2`/`.sdata` object, and every `.data`/`.bss`/`.sbss`/`.rodata` object: a named global, a table or a variable. Several TUs reference those legitimately; never an edge.
* a literal whose value is the int->float magic `0x43300000_80000000` / `0x43300000_00000000` is still a per-TU pool entry (the linker does not synthesise it), so it stays an edge and is only *counted* (`magic`).
* a unit that holds one value at two pool addresses **spans several TUs** (`coarse`): its group is reported, but its pool is not a single run, so its adjacency/order verdicts are demoted.

## Lib dependencies

refs.census, project, binary.dol.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/units/test_poolseams.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* The premise (docs/pool-seams.md, playbook idea 94, measured 2026-09-30): MWCC emits **one literal pool per translation unit**, one entry per distinct value, in first-use order, and `mwldeppc` does not merge pools across objects. So a pool literal (an `.sdata2` float/double, an `.sdata` string) whose address is read by two registered units means those units are **one** original TU that the registry has cut into pieces - a *fold candidate*, and a unit that is a partial pool of that TU can never reproduce its `.sdata2`/`.sdata` alone. The converse: the same value at two addresses inside what a registry unit treats as one TU cannot happen (the compiler would have reused the first entry), so such a unit already spans more than one TU.
