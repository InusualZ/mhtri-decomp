---
id: 245
title: A narrow range test that retail emits as addi 0xFE is (u8)(x + 0xFE), not (u8)(x - 2)
status: works
problem: Retail tests a byte range with `addi r0,rX,0xFE` (+254) before the `clrlwi`/`cmplwi`, ours with `subi r0,rX,2` - one OP_MISMATCH that keeps the row below 100.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [108, 117]
---

# 245. A narrow range test that retail emits as addi 0xFE is (u8)(x + 0xFE), not (u8)(x - 2)

**Problem.** A `u8` range test `(u8)(x - 2) <= 1` compiles to `subi r0,r4,2; clrlwi r0,r0,24; cmplwi r0,1`, while
retail has `addi r0,r4,0xFE` in the first slot: the same low byte, a different immediate, and objdiff scores it as an
operand mismatch.

**Why it happens.** MWCC keeps the constant the source wrote: subtracting 2 becomes `addi -2`, adding the byte
complement 0xFE becomes `addi 254`. Both narrow to the same byte, so only the spelling of the constant decides it.
The source was most likely written with an unsigned byte constant (or as a `u8` arithmetic expression whose
constant was already reduced modulo 256).

**How to work it.** Where the diff shows `addi rD,rA,0xFF`/`0xFE`/`0xFB` against our `subi rD,rA,1`/`2`/`5` in a
narrowed compare, write the constant as its byte complement: `(u8)(x + 0xFE) <= 1`.

**Result.** `lobby/lb_screen_step` (Wii/1.3 -O3, peephole off): `lb_smith_pl_view_step` 99.85 -> 100,
`lb_smith_make_draw` 99.97 -> 100, and the same instruction in `lb_smith_kakou_preview_set` (+0.24 B each).

**Example.**

```c
if ((u8)(self->step + 0xFE) <= 1) {   /* addi r0,r4,0xfe; clrlwi; cmplwi 1 */
    return;
}
```
