---
id: 256
title: Two record tests that keep an unfolded li/cmplwi on the starting constant are a two-iteration loop
status: works
problem: Retail repeats the same test on two records at a fixed stride and keeps `li r0,0xff ; cmplwi r0,0xff` (or `li r0,0 ; cmplw r0,rX`) on the accumulator's starting value, which ours folds away
tags: [source-shape]
applies: [Wii/1.0]
demo: 
related: [184]
---

# 256. Two record tests that keep an unfolded li/cmplwi on the starting constant are a two-iteration loop

**Problem.** A getter tests the same fields of two records (`+0x489`/`+0x492`, then `+0x4E5`/`+0x4EE`: a 0x5C
stride) and updates an accumulator. Retail's first copy still compares the accumulator's *initial constant*
against itself (`li r0,0xff ; cmplwi r0,0xff ; beq`), a test constant propagation would have removed. Ours, written
as two straight-line blocks, folds it and is 20 bytes short.

**Why it happens.** MWCC unrolls a constant-count loop late, after constant propagation has run, so the first
iteration keeps the comparison against the accumulator's starting value. Straight-line code is propagated before
any of that.

**How to work it.** Look for a fixed stride between the two blocks' offsets and find the record type that has that
size (here the `_HIT_W` attack entries at `_PLW`+0x484 and +0x4E0, size 0x5C). Write a `for (i = 0; i < 2; i++)`
over an array view of the records.

**Result.** `Pl/pl_act` (Wii/1.0): `pl_hit_result_best_get` 88.02 -> 100 and `pl_hit_level_max_get` 91.40 -> 100.

**Example.**

```c
_HIT_W* hit = (_HIT_W*)self->hit_0x484;
u8 v = 255;
s32 i;

for (i = 0; i < 2; i++) {
    if (hit[i].active_0x05 != 0 && hit[i].result != 255 && (v == 255 || v < hit[i].result)) {
        v = hit[i].result;
    }
}
```
