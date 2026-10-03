# `elfsect` - Minimal ELF32-BE section dumper and importable `sections()` reader (the skill's byte-level helper)

<!-- generated from the module docstring of `tools/elf/elfsect.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Minimal ELF32-BE section dumper and importable `sections()` reader (the skill's byte-level helper).

## Users

profiles (`.claude/agents`) (1); skills (16); CLAUDE.md (2); docs (8); imported by `dataclaim`, `sectiongap`, `unwindcut`, `vtableaudit`

## CLI

Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: object -> sections.

## Invariants and rules

* (none beyond the purpose)

## Lib dependencies

binary.elf.

## Test contract

Tier: fixture.
No selftest today.
Target: `tools/tests/elf/test_elfsect.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

the skill imports `sections()`; keep the name
