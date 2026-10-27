---
id: 230
title: Retail indexes three bytes with lbzx from precomputed +1/+2 offsets: a second table read with the same index keeps them
status: works
problem: Ours reads tbl[i*3+1]/tbl[i*3+2] as add + lbz 1/2(rX) where retail computes addi rA,rI,1 / addi rB,rI,2 up front and loads lbzx from the table base.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [166]
---

# 230. Retail indexes three bytes with lbzx from precomputed +1/+2 offsets: a second table read with the same index keeps them

**Problem.** A colour (or any 3-byte record) read as `c.r = tbl[i*3]; c.g = tbl[i*3+1]; c.b = tbl[i*3+2];` compiles
to `lbzx` for the first byte, then `add rX,rI,rBase` and `lbz 1(rX)` / `lbz 2(rX)`; retail has `addi r6,r5,1` and
`addi r7,r5,2` computed before the table's `lis`/`addi`, and three `lbzx` - one instruction more per read, so the
function is 4 bytes longer per site in retail.

**Why it happens.** With one table, MWCC folds the constant of `i*3+1` into the load's displacement. When the
same three indices are used against a **second** table, `i*3+1` and `i*3+2` become common subexpressions and are
kept in registers, so every load is `lbzx`. If the second table's results are stored to a local that is
overwritten before any use, the stores are dead and go, while the index registers stay.

**How to work it.** When the target has the hoisted `addi +1/+2` and only one table's loads, read the sibling
table with the same index as well (the original was a copy of a block that fills both colours). An `s32`/`u8`
offset local, a `const u8*` base and `(u32)` index casts all leave the folded form.

**Result.** `ef/eft018` `eft_light_lantern_update` (Wii/1.3, -O4 via pragma, peephole off): two light-kind blocks
that fill only `color_b` -> filling `color_a` from `eft_light_colors_a` first: 10088 -> 10096 B (= retail),
99.64 -> 99.97; the `color_a` stores are not emitted.

**Example.**

```
u8 kind = source->field_0x5BA;
color_a.r = eft_light_colors_a[kind * 3];        /* dead: color_a is refilled before use */
color_a.g = eft_light_colors_a[kind * 3 + 1];
color_a.b = eft_light_colors_a[kind * 3 + 2];
color_b.r = eft_light_colors_b[kind * 3];        /* lbzx r0, r4, r5 */
color_b.g = eft_light_colors_b[kind * 3 + 1];    /* lbzx r0, r4, r6 (r6 = r5 + 1, hoisted) */
color_b.b = eft_light_colors_b[kind * 3 + 2];
```
