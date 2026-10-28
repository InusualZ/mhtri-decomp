---
id: 265
title: Two vtables sharing slot functions with no middle store: the second class derives from the first
status: works
problem: Two of a TU's class tables point at the same function in several slots, yet the static initialiser stores only the root's and each class's own table, so neither a common parent nor two siblings seems to fit.
tags: [vtable, data]
applies: [homebutton]
demo: 
---

# 265. Two vtables sharing slot functions with no middle store: the second class derives from the first

**Problem.** Two class tables in the target's `.data` hold the same function in most slots (only a few slots and
the destructor differ), but the static initialiser stores just the root class's table and then the object's own
table, with no store of a table in between - so a common intermediate parent looks ruled out, and two siblings
cannot share an override.

**Why it happens.** MWCC drops the dead middle store of an inline constructor chain: for `Root -> A -> B`, the
initialiser of a `B` object keeps the store of `Root`'s table (an inline base constructor writes another member
after it) and `B`'s table, but the store of `A`'s table, overwritten with nothing read in between, disappears. So
`B` derives from `A` (it inherits `A`'s slots and overrides a few), and `A`'s table is the other emitted one.
A probe with `Root/Mid/S1/S2` and two globals shows exactly two table stores per object.

**How to work it.** Read the two tables' relocations: the class whose table holds the other one's functions in
the slots it does not override is the derived one. Derive it from the other class in the source (members it
does not override are inherited; a member it adds is declared in the base class), keep the constructors inline,
and re-measure the static initialiser.

**Result.** homebutton/hbm_kb_event's `__sinit_\hbm_kb_event_cpp`: the closing state (table 0x80657868)
derived from the opening state (0x80657918) together with the tables' map names: 69.16 -> 77.66 %.

**Example.**

```cpp
class HkbKbStateOpening : public HkbKbState { public: HkbKbStateOpening() : frame(0.0f) {} ... f32 frame; };
class HkbKbStateClosing : public HkbKbStateOpening { public: virtual s32 getState() { return 3; } ... };
```
