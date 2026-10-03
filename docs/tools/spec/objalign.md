# `objalign` - Post-compile ninja step: lower a section's `sh_addralign` to `lowbit(claimed start)` so mwld can place an odd-start unit

<!-- generated from the module docstring of `tools/elf/objalign.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Clamp a compiled object's section alignment to what its link address allows.

## Users

skills (3); docs (6); imported by `objextab`

## CLI

```
python tools/elf/objalign.py <object> [--splits config/RMHE08/splits.txt]
[--unit <splits key>] [-v] [--dry-run]
python tools/elf/objalign.py --selftest
```
Flags: `--dry-run`, `--selftest`, `--splits`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: object + splits.txt -> object.

## Invariants and rules

* MWCC emits every section with ``sh_addralign = 8``. That is fine for a section whose claimed start is 8-aligned, but a unit whose claimed start is only 4-mod-8 cannot be linked at that address: mwld rounds the section up to the next 8-byte boundary and every later section shifts with it, which breaks the DOL hash while the object itself stays byte-identical.
* Retail really does have such units, and dtk's own ``dol split`` writes the value the address can honour into the target objects it synthesises: the target's ``sh_addralign`` is ``lowbit(section start)``, bounded only by what MWCC emitted (never more than 8 for a data section). ``Pl/fn_8023C2D0.os`` ``.data`` is the worked example: our object has size 0x84C alignment 8, the target has the same size with alignment 4, and setting that one field to 4 makes the flip link to the original DOL byte for byte.
* This tool applies the same rule to the object MWCC produced, chained after ``dtk extab clean`` in the compile rule (``tools/project.py``). It is a **strict lowering** - a section whose start is 8-aligned keeps alignment 8 - so it is a no-op for every unit that links today, and it cannot move the DOL on its own.
* Evidence and the reproduction recipe: ``docs/matching.md``, "An odd-start ``.data`` claim cannot be linked with MWCC's alignment".

## Lib dependencies

binary.elf, project.splits.

## Test contract

Tier: fixture.
Today's selftest: in-file `selftest()` (`--selftest`).
Target: `tools/tests/elf/test_objalign.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
