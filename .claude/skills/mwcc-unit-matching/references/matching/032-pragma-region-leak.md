---
id: 32
title: A pragma region is not local to the functions it covers
status: works
problem: A function whose residual is a missing instruction - a `lis` the build CSE-ed away, a base re-materialised at a merge - needs a scoped `#pragma peephole off` pair (row 28's spelling). The pair fixes it, but the *reset* also decides where the region ends.
tags: [pragma]
applies: []
demo:
---

# 32. A pragma region is not local to the functions it covers

**Problem.** A function whose residual is a missing instruction - a `lis` the build CSE-ed away, a base
re-materialised at a merge - needs a scoped `#pragma peephole off` pair (row 28's spelling). The pair fixes it,
but the *reset* also decides where the region ends.

**Why try it.** The pragma is per-region, not per-function: everything between the two pragmas is affected, so the
reset's position is a free variable and moving it changes codegen in functions the author never named.

**Result.** In `src/main.cpp`, scoping `#pragma peephole off` around `fn_8003FEBC`/`fn_8003FF98` closed them
(98.00 -> 99.82 and 99.02 -> 99.95), and moving the reset past two further functions flipped `fn_8004029C`
99.52 -> 100 and `fn_80040360` 99.47 -> 100. Unit 97.77 -> 99.25 %; matched 228 -> 234 across the campaign.

**Example.** The pragma pair in `src/main.cpp` around those two functions: widening the region by two functions
fixed two functions it was never aimed at. When a pragma pair is in play, re-measure the whole unit - where the
region ends is part of the change.
