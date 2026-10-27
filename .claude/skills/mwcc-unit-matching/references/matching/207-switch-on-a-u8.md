---
id: 207
title: A switch on a u8 keeps unsigned compares in every arm only when the selector is cast to u32
status: works
problem: A compare-tree switch on a `u8` selector opens with `cmplwi` but the later case tests read `cmpwi` where retail keeps `cmplwi` throughout.
tags: [source-shape]
applies: [stage]
demo: 
---

# 207. A switch on a u8 keeps unsigned compares in every arm only when the selector is cast to u32

**Problem.** A sparse `switch (b)` on a `u8` is lowered to a compare tree: retail tests every case with `cmplwi r0,N`, ours
opens with `cmplwi` (the range split) and then tests the remaining cases with `cmpwi r0,N`. Same size, same branches; only the
compare opcode differs, so the row sits a fraction below 100 %.

**Why it happens.** The `u8` selector is promoted to `int` for the switch, so MWCC compares the leaf cases signed. A `(u32)`
selector makes the switch's controlling type unsigned, and every compare in the tree is then `cmplwi`, as retail's are.

**How to work it.** When a switch's case tests are `cmpwi` in ours and `cmplwi` in retail, write `switch ((u32)b)` (an
`unsigned int` cast is the same); a `(u8)` cast changes nothing.

**Result.** `stage/stg_w` `stage_area_effects_start` (2644 B): 99.64 -> 100 % with `switch ((u32)areano)` on map 21's area
switch (cases 0, 1, 3, 5, 6, 7).

**Example.**

```c
switch (areano) { case 0: case 1: case 3: ...; case 5: ...; case 6: ...; case 7: ...; }       /* cmplwi 1; cmpwi 3/5/6/7 */
switch ((u32)areano) { case 0: case 1: case 3: ...; case 5: ...; case 6: ...; case 7: ...; }  /* cmplwi throughout */
```
