---
id: 32
title: A pragma region is not local to the functions it covers
status: works
problem: A function whose residual is a missing instruction - a `lis` the build CSE-ed away, a base re-materialised at a merge - needs a scoped `#pragma peephole off` pair (row 28's spelling). The pair fixes it, but the *reset* also decides where the region ends.
tags: [pragma]
applies: []
demo: 032-pragma-region-leak.cpp
reviewed: 2026-09-29
related: [16, 28, 33, 39, 41, 61]
---

# 32. A pragma region is not local to the functions it covers

**Problem.** A function whose residual is a missing instruction - a `lis` the build CSE-ed away (common
subexpression elimination: the compiler reusing an earlier computation), a base re-materialised at a merge - needs
a scoped `#pragma peephole off` pair (row 28's spelling). The pair fixes it, but the *reset* also decides where the
region ends.

**How it looks.** After adding `#pragma peephole off` before one function and `#pragma peephole reset` after it,
a *different* function's score moves - up or down - although its source was not touched. Or the pragma "fixes"
the aimed function and a later function that used to match drops a few points.

**Why it happens.** A pragma applies to every function *defined* between the two directives, in file order, not to
the one you had in mind. So the reset's position is a free variable, and moving it changes codegen in functions the
author never named (a function past the reset is compiled with the file's normal settings, a function inside it
with the pragma's).

**How to work it.**

1. Put the region around the aimed function only, then re-measure the **whole unit**, not that function.
2. If a neighbour moved, try the reset one function earlier or later; keep the placement that scores best
   overall and record the choice in the unit's header comment.
3. Prefer a unit-level flag over a region when several functions need the same thing (idea 33), and prefer idea
   16's per-function pragma vocabulary when only one function should differ.

**When NOT to apply.** Do not widen a region on the theory that "it did no harm" without measuring: the same
mechanism that closed two functions there also regressed others in earlier probes. And a pragma is not a fix for a
function that only needs a unit flag (idea 33).

**Example.** `#pragma peephole off` ... `#pragma peephole reset`: both `aimed_at` and `not_aimed_at` in the demo
compile with the unfused `clrlwi`+`slwi`, and only `after_reset` gets the folded `rlwinm`.

**Result.** Measured at the time in `src/main.cpp`: scoping `#pragma peephole off` around
`fn_8003FEBC`/`fn_8003FF98` closed them (98.00 -> 99.82 and 99.02 -> 99.95), and moving the reset past two further
functions flipped `fn_8004029C` 99.52 -> 100 and `fn_80040360` 99.47 -> 100. Unit 97.77 -> 99.25 %; matched 228 ->
234 across the campaign. The file's header comment records the current placement.

**Demonstration.** `032-pragma-region-leak.cpp` (`ideas.py demo-check 32`) shows the region: a second function in
the region is compiled unfused although it was never named, and the function after the reset is folded. It shows
the *region*; that moving the reset flips a function's allocator choice (the `lis r31` coalescing above) is a
unit-level measurement no single object here reproduces.
