---
id: 69
title: A POLYMORPHIC MEMBER CLASS makes MWCC initialise the vptr of every array element
status: works
problem: Declaring array-element record types as classes with a polymorphic member blew a constructor from 252 to 544 bytes: the arrays are built with `__construct_array` and every element gets a constructor call that stores its vptr.
tags: [vtable, source-shape]
applies: [Network]
demo: 069-polymorphic-member-vptr-init.cpp
reviewed: 2026-09-29
related: [52, 68, 76]
---

# 69. A POLYMORPHIC MEMBER CLASS makes MWCC initialise the vptr of every array element

**Problem.** Declaring the four `Pat` record types as **classes** with a `NetworkSmallObject` member blew
`NetworkSessionManagerPat`'s constructor from **252 to 544 bytes**: the arrays are built with
`__construct_array` (the runtime helper that runs an element constructor over an array), and MWCC emits the
element constructors - each of which stores the vptr (the hidden pointer to the vtable) - for every element.

**How it looks.** Our constructor is much longer than the target's, with a `bl __construct_array` (and
synthesised `__ct__`/`__dt__` for the record) where the target only stores fields.

**Why it happens.** A member that is itself polymorphic makes every containing record, and every array of
them, need construction; the constructor grows with the element machinery. The discriminator is the target's
vptr-store count inside the array-building code, not the class *names*.

**How to work it.** Model the records as **structs** whose member is a plain struct with a `vtable` pointer
field (the table belongs to another owner and is only *read* through it - rule 10 Case 2), and make only the
object that the target really dispatches through a class. Compare the constructor size and its relocations.

**When NOT to apply.** If the target's constructor does contain `__construct_array` and per-element vptr
stores, the members really are polymorphic: keep the classes. This idea trades class fidelity for a viewed
table, which rule 10 permits only for tables outside the unit's own ranges.

**Result.** Measured at the time (2026-09): only the object at `+0x3CC` is dispatched through; the
constructor came back to 252 bytes and the unit's 22 bodies matched against real signatures.

**Example.**

```cpp
struct NetworkSmallObject { void** __vt; /* +0x00 */ };   /* another owner's table, only read */
struct PatRecord { NetworkSmallObject object; /* +0x00 */ u32 state; /* +0x04 */ };
```

**Demonstration.** `069-polymorphic-member-vptr-init.cpp` (`ideas.py demo-check 69`): a holder of four
records with a `Small` (virtual dtor) member calls `__construct_array` and gets a synthesised `__ct__4RecC`;
the same holder built from the struct view is a constructor with no `bl` at all.
