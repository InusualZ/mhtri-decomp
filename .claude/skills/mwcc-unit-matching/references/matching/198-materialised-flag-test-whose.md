---
id: 198
title: A materialised flag test whose false arm comes first is flag == 0 ? 0 : cond
status: works
problem: Retail tests a byte flag with cmpwi/bne into the true arm, lays the false arm (li r0,0; b) first and then compares the materialised 0/1 again; ours branches straight on && or puts the false arm last
tags: [source-shape]
applies: [GC/3.0a5.2]
demo: 
related: [71, 160]
---

# 198. A materialised flag test whose false arm comes first is flag == 0 ? 0 : cond

**Problem.** Retail computes a two-part condition into a register before testing it: `lbz; cmpwi r,0; bne T;
li r0,0; b J; T: <cond as 0/1 (cntlzw/srwi or subfic/or/srwi)>; J: cmpwi r0,0; beq ...`. `if (flag && cond)`
gives two plain conditional branches; `flag ? cond : 0` materialises the value but lays the true arm first
(`beq F; <cond>; b J; F: li r0,0`); `!flag ? 0 : cond` turns the byte into `cntlzw`/`srwi.`.

**Why it happens.** MWCC lowers `?:` as "test, jump to the second arm, fall into the first", so the arm written
first is the one laid out first; the test `flag == 0` keeps a compare-and-branch where `!flag` on a `u8` is
computed as a value.

**How to work it.** Spell the test `flag == 0 ? 0 : cond` (the false arm first, the flag compared against 0).
Try it per site: a site retail wrote as `&&` gets worse.

**Result.** `homebutton/hbm_text_panel` (GC/3.0a5.2): `hbmPanelHandleSpecialKey` 97.05 -> 100 (three sites:
`inputLatched ? mode != 1 : 0` 98.29, swapping the arms 99.69, `== 0` 100); `hbmPanelHandleMessage`
95.36 -> 96.56 over four of its five `inputLatched && mode != 1` sites (the fifth, and `hbmPanelDrawComposition`'s,
lose and stay `&&`).

**Example.**

```
if (self->inputLatched == 0 ? 0 : self->mode != 1) {   /* lbz; cmpwi; bne T; li r0,0; b J; T: subfic/or/srwi */
    hbmPanelTypeChar(self, c, 0);
}
```
