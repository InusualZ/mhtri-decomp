# `objsame` - Whether every compiled object of two build trees is the same modulo `@N` pool numbering

## Purpose

Compares `build/RMHE08/src/**.o` of two build trees (MAIN and a slot, by default) section by section, relocation by
relocation and defined symbol by defined symbol, with every compiler label's `@<digits>` read as `@N`, and prints the
units that differ. It is the byte-neutrality proof of a header-only change: "every object is the same as the base's".

## Users

Integrators and lanes proving a shared-header or declaration-only change moved no object (the network pilot wrote
`objsnap.py` by hand for this, about 15 minutes per integration).

## CLI

```
python tools/objdiff/objsame.py                      # MAIN vs the invocation's tree
python tools/objdiff/objsame.py BASE TREE            # two named trees
python tools/objdiff/objsame.py --unit "Network/*"   # only units whose stem matches (repeatable)
python tools/objdiff/objsame.py --all-sections       # .comment and the string tables too
python tools/objdiff/objsame.py --json               # lib.findings schema + base, tree, compared, same
```
Exit codes: 0 every compared object is the same and none is one-sided, 1 a unit differs, is unreadable or exists in one
tree only, 2 a tree has no `build/RMHE08/src/`.
`--json`: `{tool: "objsame", rows, ok, summary, base, tree, compared, same}`; one FAIL row per differing, unreadable or
one-sided unit, its `detail` the reasons.

## Inputs and outputs

Reads the two trees' compiled objects; writes stdout only. It **builds nothing**: build both trees first (`ninja
all_source`, or the unit objects you care about).

## Invariants and rules

* "The same" is: every non-metadata section present on both sides with equal size and bytes; every relocation section's
  sorted `(offset, type, symbol)` rows equal with symbol names modulo `@N`; the defined symbols' `(name modulo @N,
  section, value, size, bind)` equal (`STT_FILE` rows ignored). The first difference of each kind is named.
* `@N`: MWCC numbers its labels (`@123`, `@7@func@var`) in emission order, so one literal added or removed in a header
  renumbers every later label of every includer; that is not a codegen change and is never reported. Measured
  2026-10-04 at 042956de2, MAIN vs three slots: 21/14/0 units differ, against 92/88/0 when names are compared raw.
* A unit present in one tree only is reported (`BASE`/`TREE`), never silently skipped.

## Lib dependencies

`lib.objcompare` (`object_sections`, `load`, `first_difference`, `differing_bytes`), `lib.findings`, `lib.repo`,
`lib.binary.elf`.

## Test contract

Tier: fixture (`tools/tests/objdiff/test_objsame.py`, 10 checks, objects built with `ElfBuilder` in a `FixtureTree`):
renumbered pool labels are the same; a `.text` byte, a relocation name, a `.sdata2` constant and a new symbol each
differ and are named; one-sided objects are reported; `--unit` narrows; the CLI's three exit codes and the JSON schema.

## Known gaps

* It does not compare the target split objects (`build/RMHE08/obj/`): those come from the DOL split, and
  `verifyunit`/`land`'s drift row already fingerprint them.
* A `.comment` or string-table difference is ignored unless `--all-sections` (a compiler-version change would show in
  the code anyway).
