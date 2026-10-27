---
id: 221
title: An element tested before a call and stored after it: read the tests through an inline element accessor
status: works
problem: Ours keeps an array element's address in a saved register across a call (an extra saved register, a bigger frame) where retail keeps only the scaled index and rebuilds the base after the call.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [96, 165]
---

# 221. An element tested before a call and stored after it: read the tests through an inline element accessor

**Problem.** `if (a[i].p != NULL && a[i].state == 0) { a[i].p->start(); a[i].state = 1; ... }`: ours CSEs the
`a[i].state` address of the test with the store after the call, so it lives in r31 across the call (one more saved
register, frame -0x20 against -0x10). Retail keeps only `i * size` and `this` saved and rebuilds the base after the
call. Slot references, local pointers, early returns, nested `if`s and casts all compile to ours.

**Why it happens.** An inline accessor's parameter is a fresh copy of the index, so the address expressions inside
the inlined body are not the same expression as the ones written with the caller's own index, and the test's address
cannot be reused for the store. Only the scaled index (shared lower down) survives the call.

**How to work it.** Add a one-line inline member that returns the element (`NetworkSessionSlot* getSlot(s8 index)
{ return &slots_14828[index]; }`) and read the tests and the call through it; keep the stores after the call written
directly (`slots_14828[index].state = 1`). Accessors over only the tests, or over the call too, are separate
variants - measure both. It is the inverse of idea 96 (name the element pointer when retail keeps it).

**Result.** `Network/NetworkSessionStable` `connect` 89.12 -> 100.00 (the tests and the `start` call through
`getSlot`; the same accessor on the tests alone left 89.12).

**Example.**

```
if (getSlot(index)->connection_2C != NULL && getSlot(index)->linkState_00 == 0) {
    getSlot(index)->connection_2C->start(a, b);
    slots_14828[index].linkState_00 = 1;
```
