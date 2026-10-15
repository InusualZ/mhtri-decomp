---
id: 139
title: A sign-extended byte keeps its extsh only through a mask
status: works
problem: Retail has `lbz rX ; extsh rX,rX` on a zero-extended byte that becomes an `s16`, and ours has only the `lbz`, so every later instruction shifts.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [95, 102, 140]
---

# 139. A sign-extended byte keeps its extsh only through a mask

**Problem.** Decoding a report, retail loads a byte (`lbz`) and sign-extends it to 16 bits (`extsh`) before
combining it into a coordinate; ours omits the `extsh` because an unsigned byte converted to `s16` is already in
range, so ours is one instruction short.

**Why it happens.** `(s16)b` on a `u8` is a no-op conversion MWCC drops. `(s16)(b & 0xFF)` is an `int`-typed
expression converted to `s16`; the conversion is kept as `extsh` (the mask itself folds into the `lbz`). The
original source masked the byte.

**How to work it.** Where retail has `lbz` + `extsh` on a byte that ours loads without the extend, spell the
read `(s16)(b & 0xFF)`. The register pair still matters (idea 102 for `extsb`): load the locals in retail's order.

**Result.** `WPAD/WPADHIDParser` (Wii/1.3): `wpadDecodeDpdInterleaved` 71.83 -> 100; `wpadDecodeDpd` 84.5 -> 98.7
(with its locals loaded in retail's order); `wpadAccCalibrationReadDone` +2.6 points.

**Example.**

```c
read[0].x = (s16)(raw[0] & 0xFF) | ((raw[2] << 4) & 0x300);   /* lbz ; extsh - (s16)raw[0] drops the extsh */
```
