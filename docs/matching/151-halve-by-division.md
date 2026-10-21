---
id: 151
title: x / 2.0f gives retail's fmuls x,0.5 operand order
status: works
problem: Retail halves a float with `fmuls fD,fX,f0.5` and ours has the operands the other way round (`fmuls fD,f0.5,fX`).
tags: [source-shape]
applies: [GC/3.0a5.2, Wii/1.3]
demo: 
related: [18]
---

# 151. x / 2.0f gives retail's fmuls x,0.5 operand order

**Problem.** A float halved in retail is `fmuls fD,fX,fH` with the pooled `0.5` second; `x * 0.5f` and
`0.5f * x` both give ours the reverse operand order.

**Why it happens.** The original divided by two; MWCC turns a division by a power of two into a multiplication by
its reciprocal and keeps the dividend as the first operand.

**How to work it.** Where retail's `fmuls` has the `0.5` (or another reciprocal) second and no source multiply
order reproduces it, spell the division: `x / 2.0f`.

**Result.** Used in `homebutton/hbm_kb_cursor` `hbmKbCursorScrollToCaret` (GC/3.0a5.2, unit header SHAPES line);
the before/after row numbers were not recorded. Wii/1.3: `g3d/g3d_light` `LightObj::InitLightShininess`
(`GXInitLightAttn(..., shininess / 2.0f, 0.0f, 1.0f - shininess / 2.0f)`) is 100 with the division and 99.58 with
`shininess * 0.5f` (measured 2026-10-21 with `recompile.py --measure`, same 96 bytes, the `fmuls` operands swapped).

**Example.**

```c
bottom = top + height / 2.0f;    /* fmuls f1,f1,f0(0.5) - not height * 0.5f */
```
