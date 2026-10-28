---
id: 268
title: A sum computed before a branch and reduced after it: initialise the local with the sum, apply the reduction later
status: works
problem: Retail computes `addi rX, rBit, 12` before a conditional mask and the `clrlwi rY, rX, 29` after it, while ours computes both after the branch, so the loop body is shifted by one slot.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 268. A sum computed before a branch and reduced after it: initialise the local with the sum, apply the reduction later

**Problem.** In a loop body, retail does the add half of `(bit + 12) % 8` early, among the loads before a
conditional `v &= mask`, and the `& 7` after the branch. Ours, written `sh = (bit + 12) % 8;` after the `if`,
emits both instructions after the branch, which moves every instruction after it.

**Why it happens.** MWCC keeps a statement's expression together: one assignment of `(a + b) % 8` is selected as
one tree after the branch. Two statements - `u32 sh = bit + 12;` in the declaration, `sh %= 8;` after the branch -
give the add its own position before the `if`, and the scheduler keeps it there.

A related shape from the same function: a pointer that retail materialises in `r3` before a loop and passes
to `memset` after it (`addi r3, r1, 0x18` hoisted above the loop) is a variable assigned before the loop
(`dst = result;` ... `memset(dst, 0, 8);`), not the array name spelled at the call.

**How to work it.** Read where retail places the two halves of the expression; split the initialiser so each
half sits where retail has it, and measure with `mt.py tryvar`.

**Result.** `MSL_C/strtold` (`__strtold`, -O4,p): 97.238 -> 97.600 % (+17.7 matched bytes) for the split `sh`,
then 97.600 -> 97.895 % (+14.4 B) for the early `dst = result`.

**Example.**

```
for (i = 0; i < 7; i++) {
    u8 v = *src;
    u32 sh = bit + 12;          /* add before the branch, as retail */

    if (bit + 8 > 0x34) {
        v &= (u8)(0xFF << (0x34 - bit));
    }
    sh %= 8;                    /* clrlwi after it */
    ...
}
```
