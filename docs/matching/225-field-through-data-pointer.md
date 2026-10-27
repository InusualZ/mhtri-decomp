---
id: 225
title: A field read at a negative offset from a walked data pointer is read through that pointer (container-of), not through the record
status: works
problem: Retail reads a record's header field as `lbz rX,-0xA(rData)` off the pointer its loop walks, while ours keeps the record pointer live beside the data pointer and reads `lbz rX,0x2(rRec)` - one callee-saved register more and every colour after it shifted.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [96, 124]
---

# 225. A field read at a negative offset from a walked data pointer is read through that pointer (container-of), not through the record

**Problem.** A key record `{u16 tick; u8 flags; u8 pad; u8 type[8]; data[]}` is located as `rec = keys + idx *
stride`, its payload as `src = rec->mData`, and the flags are tested after that. Retail computes `src` straight into
the register the copy loop later increments and reads the flags as `lbz r0,-0xA(src)`; ours keeps `rec` and `src` in
two registers (`addi r4,r22,0xc` / `lbz r0,0x2(r22)`). In a function with several such pairs the extra register
pushes the save set one register lower (`_savegpr_20` for retail's `_savegpr_21`) and the extab differs too.

**Why it happens.** MWCC does not rewrite `rec->mFlags` as `src[-10]` itself: once `rec` is a value of its own the
allocator keeps it. Reading the field through the data pointer makes `rec` a dead temporary.

**How to work it.** Spell the record as a container-of over the data pointer, with the offset taken from the type:

```
#define KEY_OF(data) ((const Key*)((const u8*)(data) - (u32) & ((const Key*)0)->mData))
const f32* src = ((const Key*)(keys + idx * stride))->mData.mValue;
if (KEY_OF(src)->mFlags == 0) { ... }
```

Keep a named record pointer only where retail reads another field off it (`type = recA->mType` stays `addi r27,r3,4`).

**Result.** `ef/ef_animcurve` (Wii/1.3, -O3, peephole off): `ef_anim_curve_u8` 98.57 -> 98.87, `ef_anim_curve_f32`
98.48 -> 98.89, `ef_anim_curve_rotate` 98.26 -> 99.04, and `rotate`'s save set and extab became retail's.

**Example.**

```
const u8* src = ((const EfAnimCurveKey*)(keys + idx * stride))->mData.mByte;
if (EF_ANIM_KEY_OF(src)->mFlags == 0) {      /* lbz r0,-0xa(r21) */
```
