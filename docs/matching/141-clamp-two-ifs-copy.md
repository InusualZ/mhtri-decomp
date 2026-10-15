---
id: 141
title: A clamp is two independent ifs over a copy, not a nested ternary
status: works
problem: Retail clamps with `mr r0,v ; bge ; li r0,lo ; cmpwi v,hi ; ble ; li r0,hi ; stb r0`, where ours branches to a join block per arm.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [34, 119]
---

# 141. A clamp is two independent ifs over a copy, not a nested ternary

**Problem.** A stick or axis value is clamped to a range and stored as a byte. Retail copies the value, tests the
low bound, overwrites the copy, then tests the high bound *on the original* and overwrites again before one store.
A nested ternary (or `if/else if`) gives ours a branch to a join block from each arm.

**Why it happens.** Two independent `if`s over a copy are two separate conditional assignments with one store
after them; an `else` (or a ternary) makes the second test conditional on the first and introduces the join.
The copy's type matters: an `s8` copy adds an `extsb` that retail lacks; an `s16` copy does not.

**How to work it.** Write `c = v; if (v < lo) c = lo; if (v > hi) c = hi; *dst = c;` with `c` as `s16` (not
`s8`). Compare the branch count first.

**Result.** `WPAD/WPADHIDParser` (Wii/1.3): with ideas 140 and 142 the stick clamps took `wpadReportButtonsExt8`
to 100 and `wpadDecodeExtDualStick` 84.82 -> 98.34 (the clamp is not measured alone; the unit header records the
shape).

**Example.**

```c
value = (u8)(*sample)->nunchuk.stickX - (u8)cb->calB.nunchuk.stickX[0];
clamped = value;                     /* an s16 copy */
if (value < -0x80) {
    clamped = -0x80;
}
if (value > 0x7F) {
    clamped = 0x7F;
}
(*sample)->nunchuk.stickX = clamped;
```
