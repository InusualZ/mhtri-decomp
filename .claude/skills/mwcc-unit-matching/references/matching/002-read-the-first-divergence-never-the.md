---
id: 2
title: Read the *first divergence*, never the percentage
status: works
problem: `match_percent` is positional: one inserted or deleted instruction shifts every following instruction, so a function that is a single instruction away from a match reports exactly the same ~0 % as a function that is entirely wrong.
tags: [measurement]
applies: []
demo:
---

# 2. Read the *first divergence*, never the percentage

**Problem.** `match_percent` is positional: one inserted or deleted instruction shifts every following
instruction, so a function that is a single instruction away from a match reports exactly the same ~0 % as
a function that is entirely wrong.

**Why try it.** A 0 % that really means "one extra instruction in the prologue" sends you hunting in the
wrong place, and it is the single most common way to waste a day on a decomp.

**Result.** Make every decision from "first diff @N `<instruction>`": the instruction names the code shape,
which is what tells you whether to look at flags or at source.

**Example**

```
camellia_encrypt128  target 3228  ours 3228  100.00%  IDENTICAL
camellia_setup256    target 4860  ours 4860   99.83%  first-diff@0 stwu r1, -0x1d0(r1)

# other first divergences from the same unit, each naming a different code shape:
#   @12  extrwi r0, r10, 8, 16      -> index computation is fused (a peephole/level artefact)
#   @67  srwi r0, r0, 16            -> byte extraction is not fused
#   @1   mflr r0                    -> prologue/register-save idiom
```
