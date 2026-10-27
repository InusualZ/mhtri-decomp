---
id: 211
title: A pool literal ordered before its first use comes from a dead initialiser earlier in the function
status: works
problem: the unit's .sdata2 holds the right literals but one sits earlier in the target than the instruction that first loads it
tags: [data, source-shape]
applies: []
demo: 
---

# 211. A pool literal ordered before its first use comes from a dead initialiser earlier in the function

**Problem.** Every row scores 100 % and `.sdata2` has the right size and values, but one literal sits earlier in the
target's pool than our first-use order puts it, so `flipcheck` reports the pool bytes and the pool relocations as
different (objdiff scores a reloc to the wrong pool word as a match).

**Why it happens.** MWCC adds a literal to the TU's pool when the front end first meets it, in source order. A dead
store (an initialiser the optimizer removes) still registers its literal, so the pool keeps a slot ahead of the
literal's first surviving load.

**How to work it.** Find the earliest place where the literal could have been written: usually a local declared
with an initialiser that is overwritten before it is read. Add that initialiser, rebuild, and byte-check `.sdata2`
against `build/RMHE08/obj/<unit>.o`. The row must stay at 100 %, because the store is removed.

**Result.** `ef/eft050_set` (`eft050_set_move`): the target pool reads 1.0, 1.5, **0.0**, 20.0, 1900.0, 2.125, ...
while ours read 1.0, 1.5, 20.0, 1900.0, 2.125, **0.0**, ... (the only 0.0f load is in the loop). With `f32 dist =
0.0f;` (`dist` is set to 20.0f before it is read) the 56-byte `.sdata2` is byte-identical, the row stays at 100 %, and
`flipcheck` went from NOT READY (10 of 56 pool bytes, 4 relocations) to READY; the flipped DOL hashes OK.

**Example.**

```
void move(_EFT* self)
{
    f32 dist = 0.0f;   /* dead: overwritten below, but 0.0f now takes the pool slot before 20.0f */
    ...
    dist = 20.0f;
    ...
    ofs.x = 0.0f;      /* the only surviving 0.0f load */
}
```
