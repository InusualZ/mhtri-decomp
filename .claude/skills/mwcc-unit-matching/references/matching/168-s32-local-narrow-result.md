---
id: 168
title: Hold a narrow call result in an s32 local so it is narrowed at the use
status: works
problem: A call's result passed on to a `u16` parameter is copied first (`mr r0,r3 ... clrlwi r5,r0,16`, +4 bytes) where retail narrows it straight out of r3 (`bl ; clrlwi r5,r3,16`).
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [57, 66, 72]
---

# 168. Hold a narrow call result in an s32 local so it is narrowed at the use

**Problem.** A count returned by one call and passed to a `u16` parameter of the next is moved to r0 and narrowed
later in ours; retail narrows it directly from r3 into the argument register, one instruction shorter.

**Why it happens.** A `u16` local is narrowed at its assignment and kept in its own register until the call. An
`s32` local keeps the full return value, and the narrowing to the parameter's type is done once, at the call, from
r3 (the parameter type decides the mask, idea 57).

**How to work it.** Declare the local holding the result `s32` (or the callee's own return type), not the type of the
parameter it is passed to.

**Result.** `ef/ef_emitter` (Wii/1.3): fn_800A6350, fn_800A6420, fn_800A6938 and fn_800A8D30 96.6-97.9 -> 100.
`ef/ef_cube` 99.80 -> 99.87 from `ef_random_u16` returning `s32`.

**Example.**

```
s32 size = UtlistSize(list);       /* bl UtlistSize ; clrlwi r5,r3,16 - retail */
GetArray(list, buf, size);         /* u16 parameter; a `u16 size` local gives mr r0,r3 ... clrlwi r5,r0,16 */
```
