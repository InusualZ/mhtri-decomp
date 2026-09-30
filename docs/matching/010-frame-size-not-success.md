---
id: 10
title: Do not use frame size as a success signal
status: works
problem: When the last remaining diff is a stack-frame size, it is tempting to accept any flag variant that produces the target's frame - and several do.
tags: [flags, allocator]
applies: []
demo:
reviewed: 2026-09-29
related: [2, 13, 16]
---

# 10. Do not use frame size as a success signal

**Problem.** When the last remaining diff is a stack-frame size, it is tempting to accept any flag variant
that produces the target's frame - and several do.

**How it looks.** The first divergence is `stwu r1,-0x1d0(r1)` (the prologue's frame allocation) against ours
`-0x1e0`; a variant makes the two agree and the score barely moves, or drops.

**Why it happens.** A frame size is a coarse, quantised number: the EABI rounds the frame up to a multiple of 16
bytes, so many different sets of locals and spills land on the same value. A variant can hit the target's frame
while emitting completely wrong code. A frame-size hit looks like a precise binary signal, which is exactly why it
needs a second check.

**How to work it.** Always confirm a frame hit with per-function match percentages, function sizes and the
first divergence (idea 2); `tools/flags/frame.py -u <unit>` lists frames per function without objdiff, which is
what makes it good for narrowing and useless as proof.

**When NOT to apply.** A frame that differs from the target's is still real information (a missing local, a spill
the allocator kept or dropped); the rule is only that agreement is not proof. Whether the retail level is 3 or 4
is decided by the body (idea 13), not by the frame.

**Result.** Frame size is a filter, never the answer.

**Example**

```
-opt size   -> target frame -0x1d0 but 35% of the code matching   (not the answer)
baseline    -> frame -0x1e0, 82.72% (older metric) / 99.83% (v3.6.1)
```

**Evidence.** Both lines come from the Camellia unit's `camellia_setup256`, measured early in the campaign;
the two percentages are the same objects under two objdiff versions (idea 15).
