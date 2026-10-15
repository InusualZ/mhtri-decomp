---
id: 140
title: A dead srawi beside an extrwi is a shifted field extracted through a signed short
status: works
problem: Retail extracts a bit-field from a raw byte with `extrwi` and also emits an unused `srawi rX,rX,n`, which ours lacks, so the row is one instruction short.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [77, 139]
---

# 140. A dead srawi beside an extrwi is a shifted field extracted through a signed short

**Problem.** An extension decoder pulls a few bits out of a report byte. Retail has the `extrwi` that extracts
them from the raw byte *and* an `srawi rX,rX,n` whose result is never used; ours has only the `extrwi`.

**Why it happens.** The original expression shifted a signed intermediate and narrowed twice:
`(s16)((s16)((s32)b >> n) & m)`. The front end emits the arithmetic shift of the `s32` value; the back end then
folds the shift-and-mask of the raw byte into `extrwi` but does not delete the now-dead `srawi`. A plain
`(b >> n) & m` never creates it.

**How to work it.** When retail shows a dead `srawi` next to an `extrwi` of the same byte, spell the field
`(s16)((s16)((s32)b >> n) & m)` and measure; if retail's extend differs, try idea 139's mask spelling for the low
bits.

**Result.** `WPAD/WPADHIDParser` (Wii/1.3): `wpadReportButtonsExt8` -> 100 (together with ideas 141 and 142),
`wpadDecodeExtDualStick` 84.82 -> 98.34. Ruled out on `wpadDecodeExtBoard`'s middle weights: 11 cast spellings of
`(d << 2) | ((e >> n) & 3)`, none better when applied to all six.

**Example.**

```c
/* the low two bits of the Nunchuk X axis: extrwi + the dead srawi, as retail */
accX = (s16)((s16)((s16)(ext[2] << 2) & ~3) | (s16)((s16)((s32)ext[5] >> 2) & 3));
```
