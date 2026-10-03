# `dwarfmap` - Dump the DWARF2 local-variable -> stack-slot map MWCC produced for one function of a `-gdwarf-2` object

<!-- generated from the module docstring of `tools/elf/dwarfmap.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Dump the DWARF2 local-variable -> stack-slot map MWCC produced for one function.

## Users

skills (2); CLAUDE.md (1); docs (1)

## CLI

Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: object -> table.

## Invariants and rules

* The object must be compiled with -gdwarf-2 (codegen is unchanged by -g, verified). Relocations in .rela.debug_info are applied manually (RELA, symbol value 0 + addend).

## Lib dependencies

binary.dwarf.

## Test contract

Tier: fixture (none today).
No selftest today.
Target: `tools/tests/elf/test_dwarfmap.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

no selftest
