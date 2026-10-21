---
id: 171
title: Use the copy constructor's returned this as the object, not the named copy
status: works
problem: A handle is copy-constructed into a local and then used; retail takes the object from the copy constructor's return value (r3), ours re-forms the local's address, and the registers and stack slots around the call differ.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [63, 107]
---

# 171. Use the copy constructor's returned this as the object, not the named copy

**Problem.** A lookup copies a handle into a local through an out-of-line copy constructor and then indexes it.
Retail continues from the constructor's return value; ours reloads the local's address after the call.

**Why it happens.** A constructor returns `this` in r3. Retail's source used the constructed expression itself, so
the result register feeds the next call; naming the copy and using the name makes MWCC re-form `addi rX,r1,off`.

**How to work it.** Use the copy constructor's result as the object in the same expression, and declare the copy's
local before the value it is constructed from (`obj` before `resolved`).

**Result.** `g3d/g3d_resanmscn` (Wii/1.3): the nine dictionary lookups moved from 87/90 to 100/99.88 with this
shape together with idea 170's 32-aligned key records (the unit absorbed 26 rows, 99.91 -> 99.93).

**Example.**

```
u32 obj;
u32 resolved;
found = (s32)(*reinterpret_cast<ResDic*>(copy_ctor(&obj, &(resolved = get_dic(self)))))[ResName(&key)];
```
