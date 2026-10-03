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
* Read-only by construction: no `ninja`, no compile, no link, no write anywhere. Section parsing is `tools/elf/elfsect.py`; the registered-unit list is `langcheck.registered_units` (the one place that reads `config.libs`); the DOL header supplies the `.text` ranges.
* `--main` points the sweep at a tree holding `configure.py` and `build/` (default: the tree this file lives in), which is how a worktree audits `MAIN`'s built objects without a build of its own. `--diff <ref>` is the `land.py` gate row's core: the violation set is computed twice - once with the batched tree and once with `<ref>`'s text (`configure.py`, `symbols.txt`, `splits.txt` and the `src/**` that carries an assignment, read with `git show`) - and only a set that *grew* is a refusal. The run half is judged with the working tree's objects on both sides, so the ownership change a batch makes (a new `.data` claim) is what the diff sees; a batch that only *removed* an emission is not refused by that half - it is named in the report instead.
* `--at <addr>` is the census mode: it reads the vtable at `<addr>` straight out of the DOL and prints every slot with the registered unit that **owns** its target (by address, never by name) and the symbol the map names there, plus the reference object's relocation symbol for that slot (`dossier.parse_elf`). One lane hand-built that list twice and got 62 of 114 slots wrong, each time by parsing the DOL header's grouped offset/address/size fields by hand - this mode parses them once, in `dol_segments`/`dol_read`.

## Lib dependencies

binary, project, cscan, findings, names.

## Test contract

Tier: fixture (the tree is hashed before and after).
Today's selftest (`tools/units/vtableaudit_selftest.py`): Four things here can silently make the sweep lie, so each gets its own block of checks: * the **run rule** - `find_runs` must find maximal blocks of consecutive code pointers and must not call a single pointer a table (a run needs two entries), because one stray code address in a data section is an ordinary pointer; * **address resolution** - a word's value comes from the relocation that sits on it (`section base + symbol + addend`), and a symbol that is *undefined* in the object has no address there at all: it is resolved through `symbols.txt` or the `_XXXXXXXX` spelling every `lbl_`/`fn_` name carries. Get this wrong and the sweep reports the wrong addresses, or nothing at all; * the **verdict** - a run our object emits or references is fine, an owned run it neither emits nor references is the rule-10 violation, and a `.ctors`/`extab` run is **not** a vtable (`n/a`), because a run the linker/compiler puts in `.ctors` cannot be one; * the **section comparison** - a non-`.text` section our object does not carry at all is the `missing` case the report exists for, and metadata (`.symtab`, `.rela*`, `.comment`, `.note.split`) is never compared. The real `src/`, `configure.py`, `splits.txt`, `symbols.txt`, `build/` and DOL are never read or written: every fixture lives in a temp directory, so the selftest is green on a tree with no build. The fixture tree is hashed before and after the sweep, which is how "the tool only reads" is checked rather than promised.
Target: `tools/tests/units/test_vtableaudit.py` on `lib.testing` (`FixtureTree`/`GitFixture`/`ElfBuilder`); live-tree checks, if any, under `TIER='smoke'` and tolerant.

## Known gaps

None recorded.

## History (the incidents behind the rules - keep the rule, drop the narrative when the rule is stable)

* Why this tool exists (owner's backlog #0, 2026-09-26). `docs/plan.md` §6.5 rule 10 (playbook row 52) says a table of code pointers inside a unit's own registered ranges is **compiler output**: declare the class with its `virtual` methods and let MWCC emit the table. A hand-modelled table - or a table the unit simply never emits - is **not evidence of inheritance**, and for a `NonMatching` unit nothing fails: the bytes come from the DOL, the unit scores 100 %, and the defect only surfaces at the flip, when a `Matching` object replaces the original and the DOL hash moves. The audit that found this was done **by hand once** (0 instances, 2026-09-26); this is that audit as a tool so it stays true.
* **The scan keys on the assignment and on OWNERSHIP, never on the table symbol's spelling** (fixed 2026-09-27). It used to match `vtable = lbl_XXXXXXXX` and nothing else, so `self->vtable = &NetworkSessionManagerVTable;` in `Network/fn_803D3CE8.cpp` - a table at 0x805FA908, inside that unit's own band - was invisible to it and survived a landing review; the owner found it by reading the file. A rule about *ownership* cannot be enforced by a scan keyed on a *name*. The symbol on the right is now resolved through `symbols.txt` (any spelling) and classified against the unit's own registered ranges, and the member it is assigned to is matched either by the legacy `vtable`/`vtbl` name or by **the definition index**: any struct/class member at `+0x00` whose type is a pointer to a struct whose members are function pointers (the owner's heuristic - a function-pointer-table pointer at `+0x00` IS a class with inheritance). That is `fn_table_fields`, and it is what makes the `_VTable`-named blind spot impossible to repeat.
* What the tool **cannot** decide, said plainly: whether an emitted table came from a `virtual` class or from a hand-written array of the same bytes. Both compile to the same object; the difference is in the source (`virtual` methods plus the constructor that stores the table vs. an array initializer). The tool's job is the part that is decidable - a table that is *absent* from our object - and it names the emitted ones so a reviewer can check the source. `docs/plan.md` §6.5 rule 10's "a table we wrote is not evidence of inheritance" therefore still lands on the reviewer; the tool removes the invisible case.
