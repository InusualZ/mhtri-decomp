---
id: 161
title: Calls nested in a call's arguments run right to left, so a conditional last argument is materialised first
status: works
problem: Two calls (or a conversion and a `cond ? true : false`) feeding one call's arguments run in the other order from retail - the `bl`s swap, or a materialised 0/1 comes after the argument copies instead of before them.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [18, 105, 124]
---

# 161. Calls nested in a call's arguments run right to left, so a conditional last argument is materialised first

**Problem.** A call whose arguments are themselves calls or conversions evaluates them in the other order from
retail: the `bl` for the first argument comes first in ours and last in retail. Or a trailing flag argument
(`(ctrl & 8) ? true : false`) is materialised after the other arguments' copies, where retail computes it first.

**Why it happens.** MWCC evaluates a call's argument expressions right to left and only then moves the results into
r3 and up, so the last argument's call or branchy expression runs first. Hoisting the arguments into locals in
source order evaluates them left to right instead.

**How to work it.** Where retail runs the operand calls in reverse source order, put them back inside the argument
list: `f(conv(a), conv(b))` calls `conv(b)` first. For an operand pair whose retail order is reversed inside an
expression, an inline helper `f(a_expr, b_expr)` reverses it the same way. Write a boolean flag argument as the
conditional expression inside the call, not as a prepared local.

**Result.** Wii/1.3: `ef/ef_drawstrategyimpl` `ef_vec3_transform` -> 100 (conversions as call arguments);
`ef/ef_particle` `PSVECSquareDistance(conv(a), conv(b))` and fn_800A7750's `scale_a * scale_b / 100` through an
inline helper; `g3d/g3d_draw1mat1shp` SetupMaterial -> 100 with `(ctrl & 8) ? true : false` as the last argument of
`g3d_state_load_mat_chan`. The before figures were not recorded.

**Example.**

```
d = PSVECSquareDistance(to_vec(a), to_vec(b));   /* bl to_vec(b) ; bl to_vec(a) - retail */
VEC3 va = to_vec(a); VEC3 vb = to_vec(b);         /* bl to_vec(a) first - ours */
```
