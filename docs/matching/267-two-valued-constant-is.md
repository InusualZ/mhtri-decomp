---
id: 267
title: A two-valued constant is an if/else: retail schedules the default li between the compare and its branch
status: works
problem: Retail's `li rX,default` sits between a `cmplwi`/`cmpwi` and its branch; ours loads it before the compared field.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [18]
---

# 267. A two-valued constant is an if/else: retail schedules the default li between the compare and its branch

**Problem.** Retail reads `lbz r0,F(r31); cmplwi r0,2; li r30,15; bne L; li r30,12`, ours
`li r30,15; lbz r0,F(r31); cmplwi r0,2; bne L; li r30,12`: one instruction moved, so objdiff scores an insert and a
delete although both objects have the same instructions.

**Why it happens.** `x = 15; if (c) x = 12;` gives the default its own statement ahead of the test, and the scheduler
keeps it above the load. An `if (c) x = 12; else x = 15;` puts the default in the fall-through block, and the scheduler
then fills the compare-to-branch slot with it, which is retail's order.

**How to work it.** Where the default `li` of a two-valued local is one slot early, rewrite the
default-then-override pair as an `if`/`else` (keep the branch order the target's `bne`/`beq` shows). It does not apply
when the default is also the value of a third path (`pl_act_status_step`'s `step = 1` measured -24.6 B), so measure.

**Result.** `Pl/pl_act_step` (Wii/1.3, `mt.py tryvar`): `fn_8025A7FC` +12.6 B, `pl_act_fishing`'s `limit` +12.6 B,
`pl_act_slinger_aim` 98.65 -> 100 (+8.0 B), `fn_802602A0`'s yaw rate +7.8 B.

**Example.**

```
if (self->field_0x128 == 2) {     /* not: threshold = 15; if (...) threshold = 12; */
    threshold = 12;
} else {
    threshold = 15;
}
```
