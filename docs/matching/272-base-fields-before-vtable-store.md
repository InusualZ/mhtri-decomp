---
id: 272
title: Fields stored before the table pointer belong to a polymorphic base: its constructor's own table store is dropped
status: works
problem: An inlined constructor stores two fields BEFORE the class's table pointer, while one class with a constructor always stores the table pointer first.
tags: [vtable, source-shape]
applies: [homebutton]
demo: 
---

# 272. Fields stored before the table pointer belong to a polymorphic base: its constructor's own table store is dropped

**Problem.** Retail's inlined constructor writes `prev = 0; next = 0;` and only then the table pointer at `+0x00`,
then the remaining field. A single class with `virtual` members and a member-initialiser list always stores the
table pointer first (`stw r5,0x80(r3)` ahead of the field stores), so the row sits one ARG triple short of 100 %.

**Why it happens.** MWCC stores a class's table pointer after its bases are constructed. With a polymorphic base
that initialises the fields, the base constructor's own table store is overwritten by the derived one before any
read, and the optimiser drops it: what is left is the base's field stores, the derived table store, the derived
fields. A non-polymorphic base instead moves the table pointer after the base's members (it lands at `+0x08`).

**How to work it.** Split the class: a polymorphic base with the fields stored first (its key function not
defined in this unit, so its table is not emitted) and the derived class with the rest. The base's inline
destructor still gets a weak out-of-line copy in the object; check that the derived table and its slots are what
the target has.

**Result.** homebutton/tiHKBManager, the HkbManager constructor inlining the sentinel's constructor: 99.57 % with
one class, 100 % with `HkbNode` (prev, next) as the base of `HkbManagerNode` (manager); the emitted
`__vt__14HkbManagerNode` is the target's 0x18-byte table.

**Example.**

```cpp
class Base { public: Base() : prev(0), next(0) {} virtual ~Base() {} virtual void a(); Base* prev; Base* next; };
class Node : public Base { public: Node(Owner* o) : owner(o) {} virtual void a(); Owner* owner; };
/* constructing a Node: stw prev, stw next, stw __vt__4Node, stw owner */
```
