# `rso` - Parse RSO module headers, sections and export/import tables (dormant: the RSO splitter blocker, docs/rso-modules.md)

<!-- generated from the module docstring of `tools/rso/inventory.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Inventory RSO modules: header, sections, and the export/import symbol tables.

## Users

docs (3); imported by `symbols`

## CLI

```
python tools/rso/inventory.py                                  # every *.rso under orig/<game>/files
python tools/rso/inventory.py --rso <file> --sections --symbols
python tools/rso/inventory.py --json build/tmp/rso_inventory.json
```
Flags: `--dir`, `--json`, `--rso`, `--sections`, `--symbols`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: orig/**/*.rso -> listing/JSON.

## Invariants and rules

* RSO layout (from decomp-toolkit `src/util/rso.rs`, verified against the retail files):
```
0x00 u32 next          (always 0, filled in at runtime)
0x04 u32 prev          (always 0, filled in at runtime)
0x08 u32 num_sections
0x0C u32 section_info_offset      (always 0x58)
0x10 u32 name_offset              (original *build* path, e.g. D:\MH3_EUR\...\map00.plf)
0x14 u32 name_size
0x18 u32 version                  (always 1)
0x1C u32 bss_size
0x20 u8  prolog_section / 0x21 epilog_section / 0x22 unresolved_section / 0x23 bss_section
0x24 u32 prolog_offset / 0x28 epilog_offset / 0x2C unresolved_offset
0x30 u32 internal_rel_offset / 0x34 internal_rel_size
0x38 u32 external_rel_offset / 0x3C external_rel_size
0x40 u32 export_table_offset / 0x44 export_table_size / 0x48 export_table_name_offset
0x4C u32 import_table_offset / 0x50 import_table_size / 0x54 import_table_name_offset
```
* Section info: num_sections x (u32 offset_and_flags, u32 size); offset = value & ~1, bit 0 = executable. Export symbol: (u32 name_offset, u32 offset, u32 section_index, u32 name_hash) 16 bytes Import symbol: (u32 name_offset, u32 offset, u32 reloc_link) 12 bytes

## Lib dependencies

binary, project.

## Test contract

Tier: none (dormant).
No selftest today.
Target: `tools/tests/rso/test_rso.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

dormant: the RSO splitter blocker

## Moved from the module docstring (WP6)

From `tools/rso/symbols.py`:

The export table is the only symbol information a retail RSO carries: name, section index, section
offset and a name hash - no sizes and no local symbols. So the generated `symbols.txt` is a *seed*:
it pins the names and addresses the original link exported, and the analyzer fills in the rest once a
module can be split (see docs/rso-modules.md for the splitter blocker).

Section names are derived from evidence, and the derivation is recorded per section:

    proven    .init   the section holding the module's prolog entry point
              .ctors  the section holding the `_ctors` label
              .dtors  the section holding the `_dtors` label
              .bss    the section with no file bytes whose size equals the header's bss size
    inferred  everything else, by index order from the conventional REL section pool

Usage:
    python tools/rso/symbols.py --all [--out config/RMHE08] [--report build/tmp/rso-symbols/summary.md]
    python tools/rso/symbols.py --rso <file.rso> --print
