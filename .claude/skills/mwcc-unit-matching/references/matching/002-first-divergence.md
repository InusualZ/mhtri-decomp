---
id: 2
title: Read the *first divergence*, never the percentage
status: works
problem: `match_percent` is positional: one inserted or deleted instruction shifts every following instruction, so a function that is a single instruction away from a match reports exactly the same ~0 % as a function that is entirely wrong.
tags: [measurement]
applies: []
demo:
reviewed: 2026-09-29
related: [1, 3, 10, 15]
---

# 2. Read the *first divergence*, never the percentage

**Problem.** `match_percent` is positional: one inserted or deleted instruction shifts every following
instruction, so a function that is a single instruction away from a match reports exactly the same ~0 % as
a function that is entirely wrong.

**How it looks.** A function sits at 0-5 % although its diff is one extra `mr` or `lis` near the top; every
row below that point is "different" only because it moved one slot.

**Why it happens.** The score compares instructions row by row. An insertion or deletion misaligns the rest of
the function, so the percentage measures where the shift started, not how much code is wrong. A 0 % that
really means "one extra instruction in the prologue" sends you hunting in the wrong place, the most common way
to waste a day on a decomp.

**How to work it.** Make every decision from "first diff @N `<instruction>`" (the index of the first row that
differs and the instruction there). The instruction names the code shape, which tells you whether to look at
flags (idea 3) or at source. Fix the first divergence, re-measure, read the next one.

**When NOT to apply.** A register-only residual (identical opcodes, different register numbers) has its first
divergence early and says little; count how many rows differ and whether they are mirror images (idea 22).
`symdiff.py -u <unit> <symbol> N` prints the aligned rows; the relocation view catches a `bl` to a wrong symbol
that the row score treats as equal.

**Result.** The first divergence names the shape; the percentage is context only (idea 15).

**Example**

```
camellia_encrypt128  target 3228  ours 3228  100.00%  IDENTICAL
camellia_setup256    target 4860  ours 4860   99.83%  first-diff@0 stwu r1, -0x1d0(r1)

# other first divergences from the same unit, each naming a different code shape:
#   @12  extrwi r0, r10, 8, 16      -> index computation is fused (a peephole/level artefact)
#   @67  srwi r0, r0, 16            -> byte extraction is not fused
#   @1   mflr r0                    -> prologue/register-save idiom
```

**Evidence.** Camellia unit numbers as measured early in the campaign (older objdiff metric; see idea 15).
