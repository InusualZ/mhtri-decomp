# `vtableaudit` - Rule 10 audit: code-pointer runs a unit owns but neither emits nor references, `+0x00` table stores in source, section completeness; `--diff REF` is the gate's add-only row; `--at` dumps slots

<!-- generated from the module docstring of `tools/units/vtableaudit.py` at ec2609b46 by the tools-design lane; tightened by hand where marked -->

## Purpose

Sweep registered units for a **vtable they own but do not emit** - the rule-10 defect made mechanical.

## Users

the landing gate (3); profiles (`.claude/agents`) (14); skills (2); CLAUDE.md (1); docs (12); imported by `land`, `vtslot`

## CLI

```
python tools/units/vtableaudit.py                    # every registered unit, runs + refs + section diffs
python tools/units/vtableaudit.py --runs             # only the owned code-pointer runs
python tools/units/vtableaudit.py --sections         # only the section-size differences
python tools/units/vtableaudit.py --order            # only the .data emission-order findings
python tools/units/vtableaudit.py --fields           # only the fn-table-pointer fields at +0x00
python tools/units/vtableaudit.py --unit Pl/pl_master
python tools/units/vtableaudit.py --diff <ref>       # exit 0 = the batch adds no rule-10 violation
python tools/units/vtableaudit.py --at 0x80050F28   # one vtable, its slots and each target's owner
python tools/units/vtableaudit.py --json
python tools/units/vtableaudit.py --selftest
```
Flags: `--at`, `--diff`, `--fields`, `--json`, `--main`, `--order`, `--runs`, `--sections`, `--selftest`, `--unit`.
Exit codes: 0 ok, 1 findings or refusal, 2 could not run (the `lib.findings` convention; today's tool documents none, so `migration.md` records the current behaviour before changing it).
`--json`: the `lib.findings` schema `{tool, rows, ok, summary}` where the tool has `--json`; otherwise none.

## Inputs and outputs

Inputs -> outputs: splits, map, DOL, objects, src -> rows.

## Invariants and rules

* Three forms of the same defect, and what the tool reports for each:
* **(a) code-pointer runs inside the unit's own data ranges.** For every unit, walk the data sections it owns in `config/RMHE08/splits.txt` (`.data`, `.rodata`, `.sdata`, `.sdata2`, `.ctors`, `.dtors`, `extab`, `extabindex`) and find maximal runs of consecutive words whose value is an address inside `.text`. A word's value is read from the unit's own target object (`build/RMHE08/obj/<Unit>.o`): a word carrying a relocation is resolved through that relocation (`section base + symbol + addend`; the object's bytes hold the unlinked addend there, the *same* reason `dataclaim` masks reloc sites), a word without one is a plain linked value. A run needs **at least two entries** - a single code pointer in a data section is an ordinary pointer, not a table.
* **(b) whether OUR side is legitimate.** A run our object **emits** (the section covers it and every word is a code pointer) or **references** (a relocation anywhere in our object resolves to an address inside the run) is fine. A run we own and **neither emit nor reference** is the violation - the table is absent from our object entirely. The legal rule-10 Case 2 shape is the mirror image and is explicitly **not** flagged, only recorded: `self->vtbl = lbl_XXXXXXXX;` where the address is outside every registered range is another TU's table, and storing its address is the correct way to reproduce the call without dragging a class into this TU. The source scan for that shape reports every hit with its classification: `external` (no registered range owns it - legal), `foreign` (another unit owns it - legal), `own` (this unit's range - the shape rule 10 forbids, reported).
* **(c) section completeness.** The same defect seen from the other side: a `NonMatching` unit hides a missing `.data`/`.rodata` section entirely, because the target's bytes are scored against nothing of ours. Every non-`.text` section whose size differs between our object and the target object is reported, marked `missing` (ours is 0), `extra` (the target has none), `short` or `long`.
* **(d) emission order (warn-level, `docs/data-order-seams.md`, playbook row 80).** MWCC emits one TU's `.data` as globals, strings (`@NNN`), then vtables in the **reverse** of class definition order. In our built object every `__vt__*` symbol must therefore come after every non-string `.data` symbol (`vtable-before-data`), and the vtables must descend in class order (`vtable-order`; the order comes from the class definitions in the unit's source and the project headers it includes, a vtable whose class cannot be resolved is skipped and counted). `@NNN` / `@STRING@<inline function>` string literals **after** a vtable are not a finding: they are the "inline tail" (the strings of inline functions - in-class bodies, free `inline` functions - are emitted after the vtables, unmerged), so only a non-string, non-vtable symbol (an initialised global) after a vtable is flagged. A finding means the source order or a hand-modelled table is wrong. These findings are **reported, never part of the `--diff` violation set** - the gate row refuses only the rule-10 kinds above.
* Read-only by construction: no `ninja`, no compile, no link, no write anywhere. Section parsing is `tools/elf/elfsect.py`; the registered-unit list, the splits and the map are `lib.project` (the one place that reads them); the DOL header supplies the `.text` ranges.
* `--main` points the sweep at a tree holding `configure.py` and `build/` (default: the tree this file lives in), which is how a worktree audits `MAIN`'s built objects without a build of its own. `--diff <ref>` is the `land.py` gate row's core: the violation set is computed twice - once with the batched tree and once with `<ref>`'s text (`configure.py`, `symbols.txt`, `splits.txt` and the `src/**` that carries an assignment, read with `git show`) - and only a set that *grew* is a refusal. The run half is judged with the working tree's objects on both sides, so the ownership change a batch makes (a new `.data` claim) is what the diff sees; a batch that only *removed* an emission is not refused by that half - it is named in the report instead.
* **`--diff` keys are rename-stable on both halves (2026-10-04).** A run is keyed by its start address, a source
  write by `<file>:<line>:<symbol>` with the file translated through the batch's `diff -M` renames. The back side
  reads the ref's map for the text half, but the objects are the working tree's, so a relocation in them is resolved
  through the working tree's map first and the ref's for a name only it has (`load_tree`'s `object_symbols`): a map
  rename of a table's first entry no longer drops that word from the run and moves the key (L1 round 2,
  `35fe065d8` vs its parent: 2 added + 2 silent removals before, 0 / 0 after). An added run whose start AND end
  each lie within `SHIFT_WORDS` (1) word of a removed run of the same section is reported `SHIFTED` (the same table:
  its first or last word stopped resolving), never added; the removed set is printed (`REMOVED`) and is in the
  `--json` output (`removed`, `shifted`). Only `added` exits 1. **An overlap alone does not pair (2026-10-05):** the
  first version paired any overlap, and a replay of the `850127ccb` recut showed it pairing a new 630-word run with a
  removed 7-word one (and a 79-word run with an 8-word one) - two of the three rule-10 keys that landing really added
  (and took as recorded allowances) would have passed silently. The gate row (`landing/rows/rules.py`) shares the
  sweep **and** `diff_rows` (since 2026-10-05), so the lane-side `--diff` and the gate give one verdict.
* **A recut's re-owned run is credited (2026-10-06).** The back side reads the working tree's objects with the
  base's map, so a run a recut gave from unit A to a new unit B is invisible there and read as ADDED. A `text_ref`
  sweep now carries the base's data-range owners (`owners`, plus `matching_at_ref` from the base `configure.py`), and
  `violation_rows` hands them to `diff_rows` as one bookkeeping row (`owners:<ref>`, kind `owners`: never added,
  removed or counted). An added run whose address lay, at the base, inside another registered unit's range is
  `REOWNED` (`reowned: [[key, base owner]]` in `--json`) and leaves `added` - unless the base owner was `Matching`
  (its object emitted the table, so the new owner not emitting it is a regression) or the base owner is the head
  owner. Replay of `9db8fbe62` (the lobby-tail recut, which needed six `--allow-rule10` keys) against its parent with
  that commit's own objects: 6 added -> 0 added, 6 re-owned (3 from `lobby/lb_server_sel_trans.cpp` to
  `Network/NetworkStreamSink.cpp`, 3 from `SO/soi.cpp` to `VF/vf.cpp`). The gate row gets the credit through
  `diff_rows` unchanged; it does not print the re-owned list yet.
* **A unit with two ranges of one section (2026-10-05).** dtk writes one object section per split range, so a
  unit with two `.data` ranges has two sections named `.data` in its target object, each with its own `.rela.data`.
  The reader keys sections and relocations by `object_key` (`.data`, `.data#2`, in address order), reads the k-th
  range out of the k-th section at its own start, and resolves our object's single `.data` through a segment base
  (`range_layout`: the ranges back to back); section completeness sums a repeated name (`section_sizes`). Before,
  the name-keyed reader kept only the last `.data`, so the first range was read out of the second section: every
  run of the second block was reported again at the first block's addresses and the first block's real runs were
  lost. Reproduced on `850127ccb^`'s `Network/NetworkCommunityPat.cpp` (`.data` 0x805FBC78/0x805FC358, re-split
  in a worktree): 9 phantom runs, each the twin of a second-block run at +0x6E0 (`805FBD68` = `805FC448`), and
  2 real first-block runs missing (`805FBC78` 346 words, `805FC1E8` 79 words); the current tree has no such unit,
  so `--json` over MAIN is unchanged (396 keys before and after).
* **`referenced` is a relocation inside the run (2026-10-05).** `audit_unit` passed the range START to
  `_verdict_run`, so the window was the range's first `4 * words` bytes: a store of the range's first object's address
  marked a later, abandoned run `referenced`. It passes the run's own address now, as the docstring always said. Over
  MAIN's build (`2e6610017`): run verdicts referenced 185 -> 17, violation 374 -> 542; rule-10 keys 396 -> 564 (+168,
  none removed) over 81 units (top: `Pl/pl_act_step.cpp` 9, `enemy/em016_prog.cpp`, `ai/ai_npc.cpp`,
  `ef/eft_slot.cpp` 6 each, `enemy/em003_prog.cpp`, `lobby/lb_npc.cpp` 5 each). They are the honest rule-10 baseline:
  the gate computes both sides with the same code, so the change cannot refuse a batch by itself. The one landed
  `--allow-rule10` key in the history (`run:.data:805FB0F8`, the Pat claim) is in neither key set, and an allowance is
  checked only against its own batch's growth, so only a future batch's keys are affected.
* `--at <addr>` is the census mode: it reads the vtable at `<addr>` straight out of the DOL and prints every slot with the registered unit that **owns** its target (by address, never by name) and the symbol the map names there, plus the reference object's relocation symbol for that slot (`dossier.parse_elf`). One lane hand-built that list twice and got 62 of 114 slots wrong, each time by parsing the DOL header's grouped offset/address/size fields by hand - this mode parses them once, in `dol_segments`/`dol_read`.

## Lib dependencies

binary (elf, dol), project, cscan (`type_definitions`/`members`/`CLASS_OPEN_RE` for the fn-table index, `strip_comments`
for every source scan, `INCLUDE_RE` for the class order), git, repo (the default `--main` is the invocation's tree).
Measured (WP3d): `--json` identical on the live tree; over the 931 `src/`+`include/` files the old regex blanker and
`lib.cscan.strip_comments` differ on 2 files and the type index (1 596 types), the fn-table fields (3), the assignment
scan (34 hits) and the class order of all 354 units are identical.

## Test contract

Tier: fixture (the tree is hashed before and after).
Today's selftest (`tools/units/vtableaudit_selftest.py`): Four things here can silently make the sweep lie, so each gets its own block of checks: * the **run rule** - `find_runs` must find maximal blocks of consecutive code pointers and must not call a single pointer a table (a run needs two entries), because one stray code address in a data section is an ordinary pointer; * **address resolution** - a word's value comes from the relocation that sits on it (`section base + symbol + addend`), and a symbol that is *undefined* in the object has no address there at all: it is resolved through `symbols.txt` or the `_XXXXXXXX` spelling every `lbl_`/`fn_` name carries. Get this wrong and the sweep reports the wrong addresses, or nothing at all; * the **verdict** - a run our object emits or references is fine, an owned run it neither emits nor references is the rule-10 violation, and a `.ctors`/`extab` run is **not** a vtable (`n/a`), because a run the linker/compiler puts in `.ctors` cannot be one; * the **section comparison** - a non-`.text` section our object does not carry at all is the `missing` case the report exists for, and metadata (`.symtab`, `.rela*`, `.comment`, `.note.split`) is never compared. The real `src/`, `configure.py`, `splits.txt`, `symbols.txt`, `build/` and DOL are never read or written: every fixture lives in a temp directory, so the selftest is green on a tree with no build. The fixture tree is hashed before and after the sweep, which is how "the tool only reads" is checked rather than promised. A `referenced`-window fixture puts a 2-word run at `.data+0x10` and a relocation of ours at `.data+0` (a violation) or at `.data+0x10` (referenced); the range-start window fails both. A second fixture (`ElfBuilder`) gives one unit two `.data` ranges and a target object with two `.data` sections: each block's run must come back at its own address, with no range mismatch and no section-size difference (the pre-fix reader fails five of those checks). The re-own block (3
checks, 136 in all): a run another unit owned at the base is re-owned, a `Matching` base owner's run and an unowned
run stay added, no owners row means every run is added, and a run its base owner still owns is added.
Target: `tools/tests/units/test_vtableaudit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

* A re-owned run's verdict at the base is not computed (that needs the base's objects): the credit assumes a
  non-`Matching` owner did not emit it. A non-`Matching` unit whose source did emit the table, recut to one that
  does not, would be credited.
* The land gate's rule-10 row (`landing/rows/rules.py`) takes the credit but prints only shifted/removed, not
  `reowned`.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this tool exists (owner's backlog #0, 2026-09-26). `docs/plan.md` §6.5 rule 10 (playbook row 52) says a table of code pointers inside a unit's own registered ranges is **compiler output**: declare the class with its `virtual` methods and let MWCC emit the table. A hand-modelled table - or a table the unit simply never emits - is **not evidence of inheritance**, and for a `NonMatching` unit nothing fails: the bytes come from the DOL, the unit scores 100 %, and the defect only surfaces at the flip, when a `Matching` object replaces the original and the DOL hash moves. The audit that found this was done **by hand once** (0 instances, 2026-09-26); this is that audit as a tool so it stays true.
* **The scan keys on the assignment and on OWNERSHIP, never on the table symbol's spelling** (fixed 2026-09-27). It used to match `vtable = lbl_XXXXXXXX` and nothing else, so `self->vtable = &NetworkSessionManagerVTable;` in `Network/fn_803D3CE8.cpp` - a table at 0x805FA908, inside that unit's own band - was invisible to it and survived a landing review; the owner found it by reading the file. A rule about *ownership* cannot be enforced by a scan keyed on a *name*. The symbol on the right is now resolved through `symbols.txt` (any spelling) and classified against the unit's own registered ranges, and the member it is assigned to is matched either by the legacy `vtable`/`vtbl` name or by **the definition index**: any struct/class member at `+0x00` whose type is a pointer to a struct whose members are function pointers (the owner's heuristic - a function-pointer-table pointer at `+0x00` IS a class with inheritance). That is `fn_table_fields`, and it is what makes the `_VTable`-named blind spot impossible to repeat.
* What the tool **cannot** decide, said plainly: whether an emitted table came from a `virtual` class or from a hand-written array of the same bytes. Both compile to the same object; the difference is in the source (`virtual` methods plus the constructor that stores the table vs. an array initializer). The tool's job is the part that is decidable - a table that is *absent* from our object - and it names the emitted ones so a reviewer can check the source. `docs/plan.md` §6.5 rule 10's "a table we wrote is not evidence of inheritance" therefore still lands on the reviewer; the tool removes the invisible case.
