---
id: 52
title: A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance
status: works
problem: A unit whose registered ranges contain a vtable can score 100 % while its source only *views* that table (a struct of function pointers, a cast `extern`). For a `NonMatching` unit the bytes come from the DOL, so nothing fails and the class can be entirely absent from the reconstruction. Used as evidence it is worse than useless, because it is circular: the worker wrote the layout it is "verifying".
tags: [vtable, source-shape, allocator, measurement]
applies: []
demo:
---

# 52. A vtable we own must be compiler-emitted; a hand-modelled table is not evidence of inheritance

**Problem.** A unit whose registered ranges contain a vtable can score 100 % while its source only *views* that
table (a struct of function pointers, a cast `extern`). For a `NonMatching` unit the bytes come from the DOL, so
nothing fails and the class can be entirely absent from the reconstruction. Used as evidence it is worse than
useless, because it is circular: the worker wrote the layout it is "verifying".

**Why try it.** The distinction decides whether a unit can ever flip. `Matching` substitutes our object for the
original, so a vtable we own but do not emit breaks the DOL hash - or silently loses the section when
`export_all` is off (row 36).

**Result.** (1) A vtable inside our registered ranges is **compiler output**: declare the class with its
`virtual` methods and the constructor that stores the table, and let MWCC emit it - never write the entries,
never declare an owned table `extern`. (2) A vtable *outside* our ranges is the original bytes: reference its
`lbl_` symbol, and viewing it through a struct of typed function pointers is the standard way to reproduce a
virtual call's codegen without dragging a class definition into the TU (declaring the class would make MWCC emit
a table into our object - extra bytes). (3) Inheritance is settled from the object, never from a table we wrote:
the vtable's slot addresses read out of the DOL, the constructor storing the vtable, the base/derived
constructor chain, and the destructor's base-tail call.

**Example.** Audited repo-wide (2026-09-26): 23 `vtable = lbl_*` assignments in `src/`, **all 23** aimed at
addresses no registered unit owns, so all correct; of the 6 code-pointer runs inside registered ranges, 5 are
`.ctors` initializer runs plus the RSO and exception function tables (my detector's honest false positives), and
the single genuine look-alike is `Pl/pl_master.cpp`'s `jumptable_805C5FA0` (9 words, the weapon-class switch
table) in a `Matching` unit whose hash is green - compiler-emitted. Zero hand-built tables, zero
owned-but-unemitted vtables.
