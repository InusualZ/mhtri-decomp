---
id: 112
title: A table's source position fixes its .data order against the switch tables of the functions that use it
status: works
problem: A unit's `.data` holds the right tables and the right switch tables but in another order, and the functions that use the tables load them through a section-anchor base where retail loads each by name - the tables were defined in the source above their user, not after it.
tags: [data, source-shape, sections]
applies: [Wii/1.3]
demo: 112-data-source-order.cpp
reviewed: 2026-10-07
related: [23, 29, 53, 80]
---

# 112. A table's source position fixes its .data order against the switch tables of the functions that use it

**Problem.** A unit's `.data` carries every table and every jump table the target does, at the right sizes, yet the
section differs: retail has a function's switch table *before* the initialised tables its function reads, ours has them
after (or the reverse), and the functions that read those tables show `lis`/`addi` pairs against `...data.0` + offset (a
section-anchor base) where retail names each table. Byte differences of a whole section and a relocation diff on every
table load read as a modelling defect; they are source order.

**Why it happens.** MWCC emits initialised data and a function's jump tables into `.data` in **source order**: a table
is placed where its definition sits relative to the functions around it. A table defined *above* its user is emitted
before that user's switch table; one defined *below* it, after. A table defined before its user also invites the
compiler to reach several of them through one shared base (the section symbol `...data.0` plus a constant offset)
instead of by name - retail did not, so the `lis`/`addi` pairs relocate against another symbol. Declaring the table
`extern` above its user and defining it after the user gives retail's shape: the loads name the table (one relocation
symbol per table) and the switch tables keep their places around it.

**How to work it.**

1. Read the target's `.data` order (`tools/splits/dataorder.py at <addr>`; the demo's `order` assertions show the same view):
   which tables sit between which functions' switch tables.
2. For each table that retail places *after* the function that uses it, put `extern Type name[N];` above that function
   and move the definition (with its initialiser) below it. Keep a table that retail places before its user's switch
   table where it is.
3. Re-measure the unit's `.data` byte-for-byte (`flipcheck`) and the user functions' relocations (`relocdiff --by-owner`):
   the anchor-base loads turn into by-name loads exactly when the definition moves below the user.

**Result.** `enemy/em029_prog` (the dispatchers `em_action4_dispatch`/`em_action5_dispatch` and their sound programs): the
sound programs are defined after `em_action5_dispatch` with an `extern` declaration above `em_action4_dispatch`; the
dispatchers load each table by name, the switch tables keep their places around them, and `.data` is byte-identical to the
target (unit flipped, `main.dol` OK). The demo reproduces the emission order and the anchor-base load in isolation.

**Example.**

```
extern Rec tbl[2];                 // above the user: loads name `tbl`
void user(int n) { switch (n) { ... } sink(tbl[n & 1].a); }
Rec tbl[2] = { {1, 2}, {3, 4} };   // after the user: emitted after its switch table
```
