# `sectiongap` - Per-section gap of a unit's two objects: sizes, first differing byte, count and the relocation list (the extab/extabindex rows datagap skips)

<!-- generated from the module docstring of `tools/units/sectiongap.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Per-section gap between a unit's target object and ours: size, bytes, and relocations.

## Users

skills (1); docs (3)

## CLI

```
python tools/units/sectiongap.py --unit Network/NetworkWiiMediator
python tools/units/sectiongap.py --unit Network/fn_803D3CE8
python tools/units/sectiongap.py --unit <u> --all-sections   # add .comment/.symtab/.strtab churn
python tools/units/sectiongap.py --selftest
```
Flags: `--all-sections`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: src/obj objects -> rows.

## Invariants and rules

* A unit can read 100 % in objdiff and still not be the target object. `datagap.py` compares section *sizes* and looks at the data sections only, so the two compiler-generated exception sections it skips - `extab` and `extabindex` - are exactly where a flip can die; and a record can be the **right size** with its relocations at the **wrong offsets**, which no size check can see.
* This tool compares a unit's two objects section by section:
```
build/RMHE08/obj/<path>.o   the target, split out of the DOL
build/RMHE08/src/<path>.o   ours, compiled from src/
```
* and prints, per differing section, the three things a lane acts on - the two sizes, the first differing byte and the differing-byte count, and the **relocation list** (type, offset, symbol name) - in the `unit section ours target why` voice `datagap.py` and `vtableaudit.py` use. A clean unit says so.
* Two filings asked for this tool:
* **F39** (`datagap.py` ignores `extab`/`extabindex`) - filed first by the `constructNetworkWiiMediator` lane and voted again in `.pi/notes/initnetworksessionstable-d599.md` ("Tooling and environment"): on the pre-fix object `datagap.py` printed `0 unit(s) listed, 0 of them with a real ours-extra gap` for a unit whose object was 16 bytes short in `extab`, because `extab`/`extabindex` are not in its `DATA_SECTIONS` set; and `flipcheck.py` reports the sizes and byte counts but not the relocated symbol names *inside* the record. The lane's words - "a `sectiongap.py --unit <u>` printing each differing section with its reloc names would have named this defect in one command" - are this tool's shape.
* **F41** (`.pi/notes/production-trial.md`, "PIPELINE 1, PHASE 1"): a record can have the **right size** with its relocations at the **wrong offsets** - `Network/NetworkWiiMediator` carries the correct `extab` 0x1a8 with its three `__dl__FPv` relocations at +0x14/+0xac/+0xb4 against the target's +0x2c/+0x34/+0x54 - and `Network/fn_803D3CE8` is missing two-thirds of its record (0x2e4 against 0x4ec). A size check alone cannot see either.
* **Why a sibling tool and not a `datagap.py` patch.** Teaching `datagap.py` the two exception sections would only add a *size* row; `Network/NetworkWiiMediator`'s `extab` is 0x1a8 on **both** sides, so no size comparison - extended or not - can name F41. Naming it needs each section's bytes and relocation list, a different question from `datagap.py`'s data-gap scan (and F39's own wording asks for `sectiongap.py`).
* Known answers on this tree (both objects present): Network/NetworkWiiMediator -> `extab` same size, `__dl__FPv` relocations at both offset sets Network/fn_803D3CE8 -> `extab` 0x2e4 against the target's 0x4ec Network/initNetworkSessionStable -> clean (the negative control: `.text`, `extab`, `extabindex` and every relocation are identical).
* What it does **not** compare, deliberately - the honest limit: section *order* (dtk's synthesised object lays sections out differently from MWCC's), alignment, the `.comment` active-flags table, and anything beyond the single object pair. Whether a relocation's name resolves in the link, and what `splits.txt` claims for a range, are `flipcheck.py`'s questions, not this tool's. A section present on one side only is reported by name and size; a section whose bytes are shorter is compared over the bytes that exist, and the size line carries the rest.

## Lib dependencies

objcompare (`object_sections`, `sections`); `lib.units.Unit.resolve`, `poolseams.note_for_unit`.

## Test contract

Tier: fixture.
Today's selftest (`tools/units/sectiongap_selftest.py`): No build, no `ninja` and no repository state: every object is an ELF32 big-endian image written by this file, so the contract is pinned - how a `.rela<target>` section maps to the section it relocates, how a symbol index resolves to a name, that metadata sections are excluded by default, and that a same-size record whose relocations moved is reported with **both** offset sets (F41) while a short record is reported with both sizes (F39). A section present on one side, a differing byte and a reloc name on one side only are pinned too; `compare_objects` is checked to be silent on identical objects.
Target: `tools/tests/units/test_sectiongap.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.
