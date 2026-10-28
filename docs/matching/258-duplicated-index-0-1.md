---
id: 258
title: Duplicated index-0/1 blocks whose cmplw operands come out reversed are a two-trip loop MWCC unrolled
status: works
problem: Two copies of the same test against element 0 and element 1 match except that each `cmplw` has its operands in the other order, and swapping the source operands or casting them changes nothing.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [18, 222]
---

# 258. Duplicated index-0/1 blocks whose cmplw operands come out reversed are a two-trip loop MWCC unrolled

**Problem.** A function tests a value against `a[0]` and then `a[1]` in two identical blocks (`if (a[0] != 0 &&
x == a[0]) {...} if (a[1] != 0 && x == a[1]) {...}`), and every instruction matches except the equality compare:
retail has `cmplw r0,r3` (x, then the element) where ours has `cmplw r3,r0`. Writing the operands the other way
round, a `(u32)` cast or a named local for either side compiles to the same object.

**Why it happens.** In the written-out form the frontend CSEs `a[0]` from the `!= 0` test into the compare and
emits the equality with that reused value first (`backend-03`: `cmpl cr15,r93,r94`, element first); the operand
order of an `==` in the source is not kept. When the source is a `for (j = 0; j < 2; j++)` loop over the elements,
MWCC unrolls the two-trip loop itself and the compare comes out with the loaded `x` first, as retail has it.

**How to work it.** When duplicated element-0/element-1 blocks differ only by `cmplw`/`cmpw` operand order, rewrite
them as a loop over the index with a new counter (declared after the function's other locals) and re-measure.

**Result.** `menu/menu_infomation` (Wii/1.3, -O3, `cflags_menu`): `equip_deco_skills_compare` 99.88 -> 100 (both
`cmplw` fixed, no other row moved).

**Example.**

```
found = 0;
for (j = 0; j < 2; j++) {               /* was two blocks on worn_ids[0] and worn_ids[1] */
    if (worn_ids[j] != 0 && shown_ids[i] == worn_ids[j]) {
        ...
        found = 1;
    }
}
```
