# `frame` - Per-function prologue frame size of a unit under flag/compiler variants, next to the target's

<!-- generated from the module docstring of `tools/flags/frame.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Per-function prologue frame size and length of a unit, without objdiff.

## Users

skills (4); CLAUDE.md (1); docs (3)

## CLI

```
python tools/flags/frame.py                          # the only unit in the repo
python tools/flags/frame.py -u <unit>                # any unit
python tools/flags/frame.py -u <unit> --flags-extra "-O3 -inline noauto"
python tools/flags/frame.py --obj build/<version>/obj/<Lib>/<file>.o   # read an existing object
python tools/flags/frame.py --versions 1.3 1.5       # try several compilers
```
Flags: `--flags-extra`, `--obj`, `--unit`, `--versions`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: unit -> table.

## Invariants and rules

* Compiles the unit with the exact ninja command line (optionally with `--flags-extra` overrides) into a scratch object, decodes each function's prologue `stwu r1,-N(r1)` and prints it next to the target object's frame, so "one extra 4-byte local" differences are visible immediately.

## Lib dependencies

units, binary.

## Test contract

Tier: none.
No selftest today.
Target: `tools/tests/flags/test_frame.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* `--versions` with no value listed the files of the compiler's own version directory (`lmgr8c.dll`, `mwcceppc.exe`,
  ...) instead of the installed versions; since WP3b it lists the sibling version directories (`lib.units.available_versions`).
