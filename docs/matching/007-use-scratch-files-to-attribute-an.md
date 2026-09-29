---
id: 7
title: Use scratch files to attribute an instruction choice
status: works
problem: Two candidate source idioms produce visibly different instructions, and the full-unit diff cannot tell you whether the difference comes from the source idiom or from an optimizer pass.
tags: []
applies: []
demo:
---

# 7. Use scratch files to attribute an instruction choice

**Problem.** Two candidate source idioms produce visibly different instructions, and the full-unit diff
cannot tell you whether the difference comes from the source idiom or from an optimizer pass.

**Why try it.** A ten-line file compiled both ways answers that in seconds, and when one form flips the
output you know which pass is responsible - which is exactly the information a flag hunt needs.

**Result.** Scratch experiments attribute a symptom to a pass, and then to the flag that controls it. Two
examples that each pinned a flag: a cast-versus-explicit-mask byte extraction (the peephole pass fuses one
form and not the other), and a table lookup (data pooling decides whether the four tables share one `lis`
base).

**Example**

```sh
# (u8)(x >> 16) -> extrwi ;  (x >> 16) & 0xff -> srwi + clrlwi
mwcceppc.exe -O3 -opt nopeephole -c build/tmp/scratch/mask.c -o build/tmp/scratch
# -pool off -> 4 separate lis pairs, default -> 1 shared base
grep -c lis build/tmp/scratch/tab.s
```
