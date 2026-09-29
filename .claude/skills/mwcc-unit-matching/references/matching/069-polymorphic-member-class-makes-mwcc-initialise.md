---
id: 69
title: A POLYMORPHIC MEMBER CLASS makes MWCC initialise the vptr of every array element
status: works
problem: Declaring the four `Pat` record types as **classes** with a `NetworkSmallObject` member blew `NetworkSessionManagerPat`'s constructor from **252 to 544 bytes**: the arrays are built with `__construct_array`, and MWCC emits a vptr store loop for every element.
tags: [vtable, process]
applies: []
demo:
---

# 69. A POLYMORPHIC MEMBER CLASS makes MWCC initialise the vptr of every array element

**Problem.** Declaring the four `Pat` record types as **classes** with a `NetworkSmallObject` member blew
`NetworkSessionManagerPat`'s constructor from **252 to 544 bytes**: the arrays are built with
`__construct_array`, and MWCC emits a vptr store loop for every element.

**Why try it.** The target's constructor only has vptr stores where the array element is actually dispatched
through; a member that is itself polymorphic turns every element into a polymorphic object, and the
constructor grows with the element count. The discriminator is the target's vptr-store count inside the
array-building loop, not the class *names*.

**Result.** The records are **structs whose member is a struct with a vtable member** - the table belongs to
another owner and is only *read* through it (rule 10 Case 2) - and only the object at `+0x3CC` is dispatched
through. The constructor came back to 252 bytes and the unit's 22 bodies matched against real signatures.

**Example.** The shape that 252-byte constructor wants:

```cpp
struct NetworkSmallObject { void** __vt; /* +0x00 */ };   /* another owner's table, only read */
struct PatRecord { NetworkSmallObject object; /* +0x00 */ u32 state; /* +0x04 */ };
```

