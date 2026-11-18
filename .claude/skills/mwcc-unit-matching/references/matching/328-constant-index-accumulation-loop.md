---
id: 328
title: Three constant-index conditional accumulations are a for loop MWCC unrolls: the first add keeps its extsh
status: works
problem: retail has `lha r0; extsh r31,r0` for the first of three `if (key == flag[k]) value += v[k]` adds while ours emits a bare `lha r31`
tags: [source-shape]
applies: [Wii/1.0]
demo: 
---

# 328. Three constant-index conditional accumulations are a for loop MWCC unrolls: the first add keeps its extsh

**Problem.** An `s16` accumulator is initialised to 0 and three `if (key == flag[0..2]) value += v[0..2]` blocks follow.
Retail extends the first load (`lha r0,0x10(r4); extsh r31,r0`); ours loads straight into the accumulator because
`0 + v[0]` folds and the `s16` value needs no extension.

**Why it happens.** The original was a `for (j = 0; j < 3; j++)` loop. MWCC unrolls it with constant offsets, but the
accumulator is not a known constant when the first iteration is generated, so the first add keeps its `add`/`extsh`.
Written out by hand the first add sees `value == 0` and folds.

**How to work it.** Rewrite the run of identical conditional blocks as a counted loop over the same arrays and let the
optimiser unroll it. Check the unrolled code still uses fixed displacements (it does for a trip count of 3).

**Result.** Pl/pl_act `fn_802753E4` 97.98 -> 100 (208 B). The same loop form did not help `lb_barter_stock_add` (a
pointer walk is partly folded, 94.9-96.0 %), where the first compare is folded into the base and the rest advance a
pointer.

**Example.**

```
for (j = 0; j < 3; j++) {
    if (key == param->flag_0x0C[j]) {
        value += param->value_0x10[j];
    }
}
```
