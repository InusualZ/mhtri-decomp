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
python tools/objdiff/relocdiff.py --callees enemy/em001_prog ef/ef_particle   # wrong callee / mangling / linkage per function
python tools/objdiff/relocdiff.py --callees --json -     # every registered unit with both objects, the record on stdout
python tools/objdiff/relocdiff.py --values ai/ai_npc g3d/g3d_resmat   # a renamed pool word / string whose BYTES differ
python tools/objdiff/relocdiff.py --values --json F      # every registered unit with both objects (about 6 s)
python tools/objdiff/relocdiff.py --values --pairing offset ai/ai_npc   # the same-offset, same-instruction pairing only
python tools/objdiff/relocdiff.py --selftest
```
Flags: `--all`, `--by-owner`, `--callees`, `--check`, `--json`, `--pairing`, `--rows`, `--section`, `--selftest`, `--unit`,
`--values`.
`--values` prints `<unit>  <owner>+0xOFF  <type>  ours NAME = VALUE vs target NAME = VALUE` per value mismatch (a
shifted pair `+0xT (ours +0xO)`, an absent side `none`), then its class tag - none for `wrong`, `[shifted]`,
`[missing: ours never loads it here]`, `[extra: retail never loads it here]` - `[address, differs at +0xN]` when the
instruction takes the address (`addi`, a table word) and `[bytes equal]` when the owner's two bodies are otherwise
byte-identical, then `relocdiff --values: N value mismatch(es) in U of M unit(s); P renamed pair(s) read, E equal; not
read: ...; pairing align (n class, ...)`; exit 0 none, 1 some, 2 an object missing or unreadable. `--json F` (or `-`)
writes `[{unit, stem, pairs, same, differ: [{owner, section, class, offset, target_offset, ours_offset, type, ours,
ours_addend, target, target_addend, ours_value, target_value, width, kind, first_difference, owner_equal}], reordered,
skipped, error}]` (`--pairing offset`: no `class`, `*_offset` or `reordered`).
`--callees` prints `<unit>  <function>  <kind>: ours X vs target Y @+0xOFF; ...` (one line per function with a
difference; `extra: ours X`, `missing: target Y` when unpaired) and a count line; exit 0 none, 1 some, 2 an object
missing or unreadable. `--json F` (or `-` for stdout) writes `[{unit, stem, functions: [{function, diffs: [{kind,
ours, target, offset}]}], error}]`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: obj/ + src/ objects -> tables, exit.

## Invariants and rules

* **Two pairings.** The default view pairs by section offset, so a function that moved (or one inserted instruction) makes every later relocation read as target-only + ours-only. `--by-owner` groups the relocations by the symbol that contains them and aligns each group in order: it prints only the real differences (a wrong name, type or addend, an inserted or dropped relocation, a symbol on one side only) and a non-failing `note` when only the offsets moved. Use `--by-owner` on a unit with residual `.text` differences; use the default view for the both-sides tables.
* **`--callees`** is `lib.objcompare.callee_diffs`: per `.text` function both objects define that ours has written
  (more than 8 bytes; a function ours does not define is not judged), the relocation symbol names that differ. The two
  name sequences are aligned per function and the unaligned rows cancel by name (a slid instruction is no difference);
  each of our compiler labels (`@N`, `x$N`, section symbols) cancels one target pool or jump-table label (`lbl_<ADDR>`,
  `jumptable_<ADDR>`, `@N`) of the same relocation type, and the rest of ours are dropped as noise; what remains pairs
  in offset order and is classed `callee` (another symbol), `mangling` (one owner stem, two argument lists),
  `linkage` (one side C), `extra` or `missing`; one of our local labels and a target symbol of the same type at the
  same address (section, value + addend) are `name-only` (a naming difference, not a callee). Two named data labels (`lbl_80592EF0` vs `lbl_80592F68`) are a real
  difference, never noise. The motivating cases, all in the landed tree at 65b341058 and all found:
  `enemy/em001_prog` `fn_801502C8` (a 100 % row) calls `em_after_frame_check` where retail calls `em_frame_check`
  (+0x158; `fn_801507CC` twice); `ef/ef_particle` `fn_800AB220` passes `fn_800AB058`'s copies of the assert strings
  (four `lbl_` pairs); `enemy/em004_act` `fn_801CA004` calls `setTevKColor` with `D8_GXColor` where retail mangles
  `DP8_GXColor` (`mangling`, three calls). Whole tree: 374 functions in 108 of 367 units, 25.7 s.
* **`--values`** is `lib.objcompare.value_diffs`: objdiff and `rawsame` score `lfs f1, @123@sda21(r2)` against
  `lfs f1, lbl_80796EE0@sda21(r2)` as equal whatever the two pool words hold, and `--callees` cancels the pair as pool
  noise. Per owner (function or data object) both objects define - a stub ours has not written and `extab`/`extabindex`
  excepted - the two sides' value relocations are compared one of two ways (`--pairing`):
  * **`align` (the default, `aligned_value_diffs`)**: each side's value relocations, in offset order, are read on
    their own side and aligned by a heaviest common subsequence (`align_values`): a pair weighs 10 for one value at
    one place (same offset, type and instruction word: `same_place`), 6 for one value elsewhere (`same_value`: one
    name and addend whatever the relocation type, or one type, kind and bytes - an address-taken datum only as far as
    the shorter symbol runs, `compared_bytes`) and 5 for one place holding two values, so a code shift aligns the
    constants by value while unshifted code keeps its places. Classes: an aligned pair at one place with two values is
    **`wrong`**; an unaligned value the other side never loads in this owner is **`missing`** (retail's) or
    **`extra`** (ours); a missing and an extra of one type and kind between the same two aligned pairs are
    **`different`** (`[shifted]`); an unaligned value the other side loads elsewhere is `reordered` - listed in the
    JSON, counted under `not read`, never reported. An owner past `ALIGN_MAX_CELLS` (4M cells) falls back to `difflib`.
  * **`offset` (`offset_value_diffs`, the original rule)**: a relocation pairs with the target's at the **same offset
    in the owner, of the same type, patching the same instruction word**, and only name-differing pairs are read (a
    pair across two different instructions is counted `other-instruction`; an aligner on the type sequence read 303
    mismatches and offset pairing without the instruction test 474). In a function below 100 % every constant after
    a code shift can read as a wrong value.
  Census at `14ddf33b` (667 units): offset pairing 122 mismatches in 22 units (32612 renamed pairs, 32490 equal);
  aligned 59 in 17 units (33954 pairs, 33906 equal; 46 wrong, 2 different, 6 missing, 5 extra; 142 reordered). Of
  the 122, 76 aligned away, each with the same values in the same order on both sides (`lobby/lb_equip_page` 17,
  `enemy/em018_prog` 23, `ef/eft018` 11, `ef/eft053` 7, `fn_80059550` 6, `enemy/fn_802F5138` 3,
  `Pl/pl_act_step` 2 of 10, `enemy/fn_80138074` 2, `MSL/e_pow`, `SC/SCSystemConfig`, `enemy/em_common`,
  `fn_80047398`, `lobby/lb_pane_ui` 1 each), and 46 stayed, the four `[bytes equal]` rows among them
  (`pl_act_weapon_swing` and `eft033_move_joint` are rotations in unshifted code). The 13 new rows are shifted or
  one-sided values offset pairing cannot pair (`em022_act6_wander` loads the same 32.0-for-3.0 as `em022_act6_rush`;
  `eft_em_tint_model_create` and `eft023_fx` the signed for the unsigned int-to-float magic; `RSO/runtime` never loads
  a warning string). Two constants swapped where the code also shifted read `reordered`, not reported.
  Only `ADDR32`, `ADDR16`, `ADDR16_LO` and `EMB_SDA21` are read (`@ha`/`@hi` carry the high half, their `@l` partner is
  compared; a branch is `--callees`' question); offset pairing reads only a pair whose two symbol names differ. Each side reads the datum
  from its own object's section bytes, or - for an undefined name (another unit's `lbl_`) - from the retail DOL
  (`orig/RMHE08/sys/main.dol`, read-only) at the `symbols.txt` address, plus the addend. The width is the
  instruction's access width (`ppc.WIDTH`: `lfs` 4, `lfd` 8, `lhz` 2 ...; FP accesses print as numbers) or, for an
  address taken, the smaller of the two symbols' sizes (capped at 256 bytes; aligned pairing reads each side's own
  symbol and compares the shorter run). A word inside the datum that a relocation fills is masked (on both sides in
  offset pairing, on its own side when aligned: a split jump table carries linked addresses where ours carries
  zero); a datum made only of such words is `relocated`, a `.bss` target `bss`, a code target `code`, a name the map
  does not carry `unresolved` - all counted, none compared. Whole tree at `58e3e191` (665 units, 7 s): 32383 renamed
  pairs read, 32281 equal, 102 value mismatches (offset pairing) in 32 functions of 22 units, four of them in 100 % functions
  (`ai/ai_npc` `fn_802CF704`+0x6c 30.0 vs 50.0, `enemy/em022_prog` `em022_act6_rush`+0x13c 32.0 vs 3.0,
  `g3d/g3d_resmat` `GetResTexPlttInfo` `"g3d_resmat.cpp"` vs `"g3d_resmat_ac.h"` twice, `mh3_pad` `sdk_link_refs`).
  `flipcheck` carries the same pass (aligned, one `objcompare.value_brief` per row) as an advisory note (it never
  refuses).
* **What it prints,** for each section that either side relocates:
* both sides' relocations - section offset, target symbol name, relocation type, addend - merged on the offset so the two sides read across;
* the diff, in the four classes a lane acts on: (a) relocations **only the target** has; (b) relocations **only ours** has; (c) the **same offset pointing at a different symbol** (the objdiff-invisible class); (d) the **same symbol with a different type or addend**;
* an explicit line when the two sets are **identical** - the answer most of the time, and the thing that makes "byte- and relocation-identical" checkable rather than assumed.
* The unit's two object paths and their mtimes are named in the header (so a stale read is visible), and a prebuilt object older than a source in the unit's include closure is flagged with `stale` (the same rule `tools/objdiff/freshguard.py` enforces for the scorers).
* **Reuse, not re-implementation.** The relocations (with the RELA addend), the four classes and the owner view are `lib.objcompare`'s (`reloc_rows`, `reloc_classes`, `by_owner`); the relocation type names are `objcompare.legacy_reloc_name`; the unit and its paths come from `tools/unitutil.py`; the staleness arithmetic is `tools/objdiff/freshguard.py`. This tool is the section-relocation *view*; `sectiongap.py` stays the section size/byte view and pairs relocations by name, while this one pairs by offset and carries addends. It deliberately does not decode an instruction's field, resolve a symbol in the link, or judge what `splits.txt` claims.

## Lib dependencies

objcompare, report (freshness, stamps); `lib.units.Unit.resolve` for the unit.

## Test contract

Tier: fixture.
Today's selftest (`tools/objdiff/relocdiff_selftest.py`): Two halves, both offline: * the **pure rule** (`diff_relocs`) driven with plain tuples - the four classes named in the tool's contract (a target-only, b ours-only, c same-offset-different-symbol, d same-symbol-different- type/addend), the identical answer, the moved-offset case that must NOT be read as a symbol change, and the multiset identity that keeps an unchanged relocation out of the diff; * the **reader** (`read_relocs`) against ELF32 big-endian fixtures written by this file's builder (`sectiongap_selftest.build_elf`, extended to carry a RELA addend), so a symbol index resolves to a name, the `.rela<target>` section maps to the section it relocates, and the addend survives the read.
`--callees`: `tools/tests/objdiff/test_relocdiff_callees.py` (13 checks, `ElfBuilder` objects): a wrong callee, a
mangling and a C linkage are named with both spellings and the offset; moved relocations and a shifted clean call
cancel; `@N` labels cancel `lbl_`/`jumptable_` of the same type and not of another type; two named data labels differ;
a stub and a target-only function are not judged; the CLI line, exit 1 and `--json -`. Mutations: no stub skip fails 4,
no local-label rule fails 4, `owner_stem` as identity fails 3.
`--values`: `tools/tests/objdiff/test_relocdiff_values.py` (29 checks, `ElfBuilder`/`DolBuilder` objects): the access
widths and the smaller datum size; one pool word under two names equal and unequal (both values spelled, `owner_equal`);
`lfd` reads 8 bytes; `@ha` is not read; one name is not read; another instruction at the offset is `other-instruction`;
another offset does not pair; a stub is not judged; `.sbss` is `bss`; an undefined name read through the DOL (wrong and
right word) and `unresolved` without it; an address-taken string (smaller size, quoted, first differing byte); a split
jump table of relocated words is `relocated`; the CLI line, summary, exit 1/0 and `--json -`; `flipcheck`'s advisory
note. Mutations: no instruction check, no relocated mask, the first size instead of the smallest, no stub skip and no
DOL read each fail one check. The two offset-only cases (another instruction, another offset) pin `mode="offset"`.
Aligned pairing: `tools/tests/objdiff/test_relocdiff_align.py` (18 checks): a shift of three loads is two false values
under offset pairing and none aligned (also through the `difflib` fallback); a wrong value inside a shift is one
`different` row with both offsets; a wrong value in place is `wrong` with `owner_equal`; a rotation of four constants
in unshifted code is four `wrong`; `missing`, `extra` and `reordered`; one name reached by `@sda21` and by `lis`/`addi`
is one datum; an address-taken datum compares the shorter symbol's bytes and names the first differing byte; the CLI
exit codes for both pairings, the shifted line and the class summary; `flipcheck`'s note for a pure shift (none) and a
shifted wrong value. Mutations: place weight 0 fails 3, place weight below a crossing value pair fails 1, a type-bound
name match fails 1, no prefix compare fails 1, offset pairing as the default fails 12.
Target: `tools/tests/objdiff/test_relocdiff.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

Only six relocation kinds are named (`objcompare.legacy_reloc_name`); the full table would print `R_PPC_REL14` for the 6 `R_PPC_11` lines of `--all` on this tree.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* **Why this tool exists.** objdiff scores a *relocation-name* mismatch as equal - this project runs it with `functionRelocDiffs=none`, so a `bl`/`lis`/`addi` to the wrong symbol of the same shape reads 100 % - and a byte diff cannot see it either, because the relocated field holds the same value. Three separate lanes hand-rolled this comparison in throwaway scripts before it was shipped. The case that matters is a unit whose `.text` is byte-identical to the target's while its relocations are not: this tree holds 16 of them (e.g. `g3d/fn_8005AA28`, `menu/menu_note`, `Pl/pl_act`, `Pl/fn_80229ECC`), every one scored 100.00000 % by `unitscore.py`.
