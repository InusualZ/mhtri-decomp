---
id: 52
title: A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance
status: works
problem: A unit whose ranges contain a vtable can score 100 % while its source only *views* that table (a struct of function pointers, a cast `extern`); the bytes come from the DOL so nothing fails, the class is absent, and the layout is circular evidence.
tags: [vtable]
applies: []
demo: 052-vtable-compiler-emitted.cpp
reviewed: 2026-09-29
related: [36, 68, 69, 76]
---

# 52. A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance

**Problem.** A unit whose registered ranges contain a vtable can score 100 % while its source only *views* that
table (a struct of function pointers, a cast `extern`). For a `NonMatching` unit the bytes come from the DOL, so
nothing fails and the class can be entirely absent from the reconstruction. Used as evidence it is worse than
useless, because it is circular: the worker wrote the layout it is "verifying".

**How it looks.** The unit's `.data` range holds a table of code pointers that an object's `+0x00` field points at,
the `.data` section pairs at 100 %, and yet the source has no `class ... virtual` anywhere - only
`struct Table { void (*fn0)(...); ... }` or `extern void *lbl_XXXXXXXX;`. (A *vtable* is the table of a class's
virtual-method addresses that each object's first word, the *vptr*, points at.)

**Why it happens.** `Matching` substitutes our object for the original one in the link, so a vtable we own but do
not emit either breaks the DOL hash or silently loses the section (`export_all` is off - idea 36). Before that, a
`NonMatching` unit is never linked, so the hand-built layout is never tested.

**How to work it.** (1) A vtable *inside* our registered ranges is **compiler output**: declare the class with its
`virtual` methods and the constructor that stores the table, and let MWCC emit it - never write the entries, never
declare an owned table `extern`. (2) A vtable *outside* our ranges is the original bytes: reference its `lbl_`
symbol, and view it through a struct of typed function pointers - the standard way to reproduce a virtual call's
codegen without dragging a class definition into the TU (declaring the class would make MWCC emit a table into our
object: extra bytes). (3) Inheritance is settled from the object, never from a table we wrote: the vtable's slot
addresses read out of the DOL, the constructor storing the vtable, the base/derived constructor chain, and the
destructor's base-tail call (ideas 68, 69, 76 carry the follow-ups). Audit with `python
tools/units/vtableaudit.py` (`--runs`, `--sections`, `--diff <ref>`), which is what the land gate's rule-10 row uses.

**Result.** Audited repo-wide (2026-09-26): 23 `vtable = lbl_*` assignments in `src/`, **all 23** aimed at
addresses no registered unit owns, so all correct; of the 6 code-pointer runs inside registered ranges, 5 are
`.ctors` initializer runs plus the RSO and exception function tables (the detector's honest false positives), and
the single genuine look-alike is `Pl/pl_master.cpp`'s `jumptable_805C5FA0` (9 words, the weapon-class switch table)
in a `Matching` unit whose hash is green - compiler-emitted. Zero hand-built tables, zero owned-but-unemitted
vtables.

**When NOT to apply.** A *foreign* table (outside your ranges) is read, not defined: do not model its class. A run
of code pointers in `.data` is not automatically a vtable - jump tables (idea 53) and `.ctors` runs look the same
to a naive scan; the discriminator is the store of the address into an object's `+0x00`.

**Demonstration.** `052-vtable-compiler-emitted.cpp` (`ideas.py demo-check 52`): a two-method class with a
constructor makes MWCC emit `__vt__6Widget` (16 bytes: an 8-byte header plus two slots) and the constructor stores
its address (a relocation to it); the struct-of-function-pointers view of a foreign table adds nothing to `.data`
(the section is Widget's 16 bytes alone) and reaches the retail table only through the `lbl_805FA908` reference.
