---
id: 236
title: A call result folded into a later call's argument: hold it in a local so the outer call's self move follows the inner call
status: works
problem: Retail moves self into r3 right after an inner call returns (before the float math on its result); ours moves it just before the outer bl, or the reverse
tags: [source-shape, allocator]
applies: []
demo: 
---

# 236. A call result folded into a later call's argument: hold it in a local so the outer call's self move follows the inner call

**Problem.** `f(self, 2.0f * (1.2f / g(self)))` compiles to `bl g; lfs; fdivs; lfs; fmuls; mr r3,self; bl f`, while
retail has `bl g; mr r3,self; lfs; fdivs; lfs; fmuls; bl f` - one instruction moved, two diff rows per call site.

**Why it happens.** With the call nested in the argument, MWCC evaluates the argument expression as one tree and
materialises `self` last. Once the inner call is its own statement, the outer call's arguments are evaluated in
parameter order (`self` first, then the float expression over the local), and the scheduler keeps that order.
The mirror case is the same lever: retail's `mr r3,self` sits **after** the float math when the whole float
argument is computed into a local first (`f32 t = expr; f(self, t, 0.02f);`).

**How to work it.** Read where `mr r3,<self>` sits relative to the float computation. Right after the inner `bl`:
hold the inner call's result in a local (`f32 s = g(self); f(self, 2.0f * (1.2f / s));`). After the float math:
hold the whole float argument in a local. Note that holding the *quotient* (`f32 k = 1.2f / g(self)`) does not
move it - the local must be the inner call's own result.

**Result.** enemy/em003_prog: `em003_act1_7` 97.53 -> 100, `em003_act1_8` 96.15 -> 100, `em003_act1_9` 98.76 ->
100 (four `get_em_base_scale` sites); the mirror on `em003_act2_6` (`em_motion_blend_step`) 97.56 -> 100.

**Example.**

```
f32 s = get_em_base_scale(self);           /* bl get_em_base_scale; mr r3,r31; lfs; fdivs; lfs; fmuls */
em_mot_speed_set(self, 2.0f * (1.2f / s)); /* bl em_mot_speed_set */
```
