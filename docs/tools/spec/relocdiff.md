# `relocdiff` - Relocation-level diff of a unit (both sides, four difference classes; `--by-owner` aligns per containing symbol)

<!-- generated from the module docstring of `tools/objdiff/relocdiff.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Relocation-level diff of one unit: the target object's relocations against ours, on both sides.

## Users

profiles (`.claude/agents`) (3); skills (1); docs (3)

## CLI

```
python tools/objdiff/relocdiff.py g3d/fn_8005AA28
python tools/objdiff/relocdiff.py --unit Pl/pl_act --unit menu/menu_note
python tools/objdiff/relocdiff.py g3d/fn_8005AA28 --section .text --rows 0
python tools/objdiff/relocdiff.py g3d/fn_8005AA28 --json r.json
python tools/objdiff/relocdiff.py g3d/fn_8005AA28 --check      # exit 1 when a relocation differs
python tools/objdiff/relocdiff.py Network/network_state --by-owner   # compact, robust to moved code
python tools/objdiff/relocdiff.py --selftest
```
Flags: `--all`, `--by-owner`, `--check`, `--json`, `--rows`, `--section`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: obj/ + src/ objects -> tables, exit.

## Invariants and rules

* **Two pairings.** The default view pairs by section offset, so a function that moved (or one inserted instruction) makes every later relocation read as target-only + ours-only. `--by-owner` groups the relocations by the symbol that contains them and aligns each group in order: it prints only the real differences (a wrong name, type or addend, an inserted or dropped relocation, a symbol on one side only) and a non-failing `note` when only the offsets moved. Use `--by-owner` on a unit with residual `.text` differences; use the default view for the both-sides tables.
* **What it prints,** for each section that either side relocates:
* both sides' relocations - section offset, target symbol name, relocation type, addend - merged on the offset so the two sides read across;
* the diff, in the four classes a lane acts on: (a) relocations **only the target** has; (b) relocations **only ours** has; (c) the **same offset pointing at a different symbol** (the objdiff-invisible class); (d) the **same symbol with a different type or addend**;
* an explicit line when the two sets are **identical** - the answer most of the time, and the thing that makes "byte- and relocation-identical" checkable rather than assumed.
* The unit's two object paths and their mtimes are named in the header (so a stale read is visible), and a prebuilt object older than a source in the unit's include closure is flagged with `stale` (the same rule `tools/objdiff/freshguard.py` enforces for the scorers).
* **Reuse, not re-implementation.** The relocations (with the RELA addend), the four classes and the owner view are `lib.objcompare`'s (`reloc_rows`, `reloc_classes`, `by_owner`); the relocation type names are `objcompare.legacy_reloc_name`; the unit and its paths come from `tools/unitutil.py`; the staleness arithmetic is `tools/objdiff/freshguard.py`. This tool is the section-relocation *view*; `sectiongap.py` stays the section size/byte view and pairs relocations by name, while this one pairs by offset and carries addends. It deliberately does not decode an instruction's field, resolve a symbol in the link, or judge what `splits.txt` claims.

## Lib dependencies

objcompare, report (freshness, stamps); `unitutil.resolve_unit` for the unit.

## Test contract

Tier: fixture.
Today's selftest (`tools/objdiff/relocdiff_selftest.py`): Two halves, both offline: * the **pure rule** (`diff_relocs`) driven with plain tuples - the four classes named in the tool's contract (a target-only, b ours-only, c same-offset-different-symbol, d same-symbol-different- type/addend), the identical answer, the moved-offset case that must NOT be read as a symbol change, and the multiset identity that keeps an unchanged relocation out of the diff; * the **reader** (`read_relocs`) against ELF32 big-endian fixtures written by this file's builder (`sectiongap_selftest.build_elf`, extended to carry a RELA addend), so a symbol index resolves to a name, the `.rela<target>` section maps to the section it relocates, and the addend survives the read.
Target: `tools/tests/objdiff/test_relocdiff.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

Only six relocation kinds are named (`objcompare.legacy_reloc_name`); the full table would print `R_PPC_REL14` for the 6 `R_PPC_11` lines of `--all` on this tree.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Why this tool exists.** objdiff scores a *relocation-name* mismatch as equal - this project runs it with `functionRelocDiffs=none`, so a `bl`/`lis`/`addi` to the wrong symbol of the same shape reads 100 % - and a byte diff cannot see it either, because the relocated field holds the same value. Three separate lanes hand-rolled this comparison in throwaway scripts before it was shipped. The case that matters is a unit whose `.text` is byte-identical to the target's while its relocations are not: this tree holds 16 of them (e.g. `g3d/fn_8005AA28`, `menu/menu_note`, `Pl/pl_act`, `Pl/fn_80229ECC`), every one scored 100.00000 % by `unitscore.py`.
