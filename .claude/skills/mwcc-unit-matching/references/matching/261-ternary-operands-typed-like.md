---
id: 261
title: Ternary operands typed like the target keep or drop the conversion after the merge
status: works
problem: A constant ternary passed to a u16 parameter shows retail's `li; b; li; clrlwi` (or an s16 return lacks our `extsh`) and every cast spelling of the whole ternary folds or adds it.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
---

# 261. Ternary operands typed like the target keep or drop the conversion after the merge

**Problem.** Retail converts a ternary's result once, after the two arms join (`li r5,0x600; b; li r5,0xa00;
clrlwi r5,r5,16` before a call taking `u16`), or not at all (`srawi r3,r0,31` returned from an `s16` function with
no `extsh`). Ours folds the mask into each constant, or adds an `extsh` after the merge; casting the whole ternary
(`(u16)(c ? a : b)`) changes nothing.

**Why it happens.** The ternary's type is the usual-arithmetic type of its operands. With `int` constants the
conversion to the parameter/return type is applied to an `int` expression and MWCC either folds it per arm (a
constant) or emits it after the join (a computed value). Typing the operands themselves (`(u16)1536`, `(s16)-1`)
makes the ternary already that type: the promotion back to `int` for the argument is then a real zero-extension
that MWCC keeps after the join, and for a return of the same type no conversion is needed at all.

**How to work it.** Spell each arm with the target's type: `c ? (u16)a : (u16)b` for a `u16` parameter whose
retail call site masks after the join; `c ? (s16)0 : (s16)-1` for an `s16` return that retail leaves unextended.
Check the callee's parameter type first (playbook 57): the mask only appears when the parameter is the narrow type.

**Result.** `ai/ai_npc` (Wii/1.3, -O4): with `fn_802D30F8`'s limit typed `u16` (its own body 90.0 -> 100),
`ai_npc_m2_step2_arm` 99.22 -> 100, `ai_npc_m2_step5_arm` 99.19 -> 100, `ai_npc_m2_step7_arm` 99.09 -> 100
(`(u16)` arms); `fn_802CD20C` 98.02 -> 100 (`(s16)` arms on the return).

**Example.**

```
self->field_0x194 = fn_802D30F8(ang, self->field_0x194, distance > lim ? (u16)1536 : (u16)2560);
return pl_act6_class_ck(plw, 1) == 1 ? (s16)0 : (s16)-1;
```
