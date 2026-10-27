---
id: 200
title: A compound assignment keeps a float product's operand order: x *= c is fmuls x,c
status: works
problem: Retail's `fmuls fD,fX,fC` (value first, constant or factor second) comes out as `fmuls fD,fC,fX` from both `x = x * c` and `x = c * x`, or the two loads of an `fadds` swap registers, and no operand order in the expression moves it.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [18, 151]
---

# 200. A compound assignment keeps a float product's operand order: x *= c is fmuls x,c

**Problem.** The first divergence is a float multiply (or add) whose two source registers are swapped against
retail - `fmuls f0,f1,f0` where retail has `fmuls f0,f0,f1` - often with the loads before it in the other order
too. Writing the operands the other way round changes nothing: `x = x * c`, `x = c * x` and `(a - b) * t` and
`t * (a - b)` all compile to the same instruction.

**Why it happens.** MWCC puts a binary float expression's operands in its own canonical order, so the source order
of `a * b` is not observable. A compound assignment (`x *= c`, `d *= t`) is lowered as an update of `x`, which keeps
`x` as the first operand. A value the expression reads through a local is likewise loaded before the constant it is
combined with (`f32 ground = p->y; ... 300.0f + ground` loads `p->y` first).

**How to work it.** When the residual is only the operand order (or the load order) of one `fmuls`/`fadds`, rewrite
the expression as an update: `y *= k;` for `y = y * k`, or a local `d = a - b; d *= t; r = d + b;` for
`r = (a - b) * t + b`. For an add whose field load comes first in retail, read the field into a local. Measure each
statement on its own; a rewrite that turns an expression into a store and a reload is a regression.

**Result.** `camera/camera_main`: `camera_lobby_viewport_fit` 98.03 -> 100 % (`y *= 0.93333334f;` for
`y = (f32)y * 0.93333334f;`), `camera_dcm_step` 99.63 -> 100 % (`d = key->roll - key[-1].roll; d *= t;
cam->roll = d + key[-1].roll;`), `camera_follow_dive_ck` 98.54 -> 100 % (the ground height through a local before
`300.0f + ground`). `.sdata2` byte-identical before and after.

**Example.**

```
s16 h;  h = (f32)h * 0.93333334f;   /* lfs f1,k ; fmuls f0,f1,f0 */
s16 h;  h *= 0.93333334f;           /* lfs f1,k ; fmuls f0,f0,f1   (retail) */
```
