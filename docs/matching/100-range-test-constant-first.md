---
id: 100
title: A signed range test keeps its two compares only when the upper bound is written constant-first
status: works
problem: Retail tests an index with two signed compares (`cmpwi x,0; blt` then `cmpwi x,32; bge`) and our build emits one `cmplwi x,31`, which shortens the function and shifts every branch after it.
tags: [source-shape]
applies: [Network]
demo: 100-range-test-constant-first.cpp
---

# 100. A signed range test keeps its two compares only when the upper bound is written constant-first

**Problem.** A bounds test `if (id < 0 || id >= 32)` compiles to a single unsigned compare (`cmplwi r6,31`
plus one branch), while retail has the two signed compares and their two branches. The function comes out one or
two instructions short, so the positional score drops far more than the one test explains.

**Why it happens.** MWCC folds `x >= 0 && x < N` / `x < 0 || x >= N` into one unsigned comparison when it
recognises the pattern `variable OP constant` on both sides. Writing the upper bound with the constant on the left
(`N <= x`) is the same condition but not the shape the folding rule matches, so both signed compares survive.

**How to work it.** When retail shows `cmpwi x,0` + `cmpwi x,N` for one range test, write the upper half
constant-first: `x < 0 || N <= x` (or `x >= 0 && x < N` becomes `x >= 0 && !(N <= x)` - prefer the first form).
When retail *does* show the single `cmplwi x,N-1`, keep the natural spelling (or `(u32)x > N-1`).

**Result.** `Network/NetworkSessionManagerPat`'s `handleServerTimeout`: 96.42 -> 100 % with
`id < 0 || 32 <= id || item->id_000 <= 0`, the only change. `Network/NetworkSessionStable.cpp` independently
uses `4 <= index` for its slot bounds test for the same reason (its unit header).

**Example.**

```c
if (id < 0 || id >= 32) { ... }   /* one cmplwi r3,31 */
if (id < 0 || 32 <= id) { ... }   /* cmpwi r3,0; blt; cmpwi r3,32; bge - retail's shape */
```
