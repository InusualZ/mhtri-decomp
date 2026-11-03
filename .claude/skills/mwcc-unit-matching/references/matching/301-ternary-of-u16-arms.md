---
id: 301
title: A ternary of u16 arms passed to a u16 parameter is masked in place: the sum coalesces with the argument register
status: works
problem: A consecutive-constant ternary feeding a call ends `addi r4,r4,K ; clrlwi r4,r4,16` in retail and `addi r0,r4,K ; clrlwi r4,r0,16` in ours, whatever the cast on the ternary or the local it goes through
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [57, 261, 240, 278]
---

# 301. A ternary of u16 arms passed to a u16 parameter is masked in place: the sum coalesces with the argument register

**Problem.** `em_mot_set(self, (u16)(a == 1 ? 0x1E : 0x1F), 2, 0)` compiles to retail's `subi ; subfic ; nor ; srawi r4`
mask (idea 240) followed by `addi r0,r4,0x1f ; clrlwi r4,r0,16`, while retail has `addi r4,r4,0x1f ; clrlwi r4,r4,16`:
the same instructions, the sum one register off. A `u16`/`s32` local, `+=` on a mask local, `#pragma opt_lifetimes
off` (idea 278), an `inline` helper, u16-typed arms alone, a u16 callee view alone, `0x1F - (a == 1)` and the
inverted ternary all keep the pair or change the compare sequence (16 spellings measured, 0 gain). Across the DOL
retail has 12 in-place and 12 scratch sites of this `srawi/srwi ; addi ; clrlwi` triple; our objects had 0 in-place
of 29 - the pair was open in em007_act (x2), em009_act, em016_prog (x2), em018_prog, em_act_step, lb_quest_board,
lb_quest_ui (x2), lb_screen_step and NetworkConnectionStable.

**Why it happens.** The first PCode (`tools/mwcc-debugger`, dump 03) shows the frontend lowering the ternary as
`srawi tM ; li tK,0x1F ; add tK,tM,tK ; rlwinm tR,tK,0,16,31 ; mr r4,tR`: the sum is written into the constant's
temporary and the `(u16)` conversion into a fresh one. `tR` is coalesced with r4; `tK` lives only from the `add` to
the `rlwinm` with neighbours r1/r3, so it colours r0 (`tM` is the `addi`'s base and so excludes r0 - it takes r4).
Retail's shape keeps the sum and the mask on one temporary: when the arms are already `u16` and the parameter is
`u16`, the conversion at the call is a re-narrowing of a `u16` value (idea 57's table: `u32`/`u16` value into a `u16`
parameter masks in place) and the frontend emits `rlwinm tK,tK` on the sum's own temporary, which is the one coalesced
with r4. Either half alone is not enough: int arms into a u16 parameter convert an `int` expression into a new
temporary, and u16 arms into an `s32` parameter need no mask at all and are promoted into a new temporary.

**How to work it.** Give the call a view (or the owner's prototype, when the owner proves it) whose parameter is `u16`,
and type both arms `u16`: `em_mot_set_c1(self, a == 1 ? (u16)0x1E : (u16)0x1F, 2, 0)` with
`#define em_mot_set_c1 ((void (*)(struct _ENEMY_WORK*, u16, s32, s32))em_mot_set)`. Keep the owner's evidence for the
width beside the view (`em_common.h`: `em_mot_set` narrows its motion with `clrlwi r4,r4,16` first). The scratch form
retail keeps elsewhere (a store, a local, a `>> 1` sum into a u16 parameter) is the plain spelling and must not be
converted.

**Result.** `enemy/em009_act` (Wii/1.3, -O3, peephole off): `em009_act2_11` 99.80 -> 100 (the last open row), unit
63/64 -> 64/64, flipcheck READY, trial flip `main.dol: OK`. The eleven sibling sites above are the same pair and the
same lever (each is a u16 motion/sprite/string id argument).

**Example.**

```
#define em_mot_set_c1 ((void (*)(struct _ENEMY_WORK*, u16, s32, s32))em_mot_set)
        em_mot_set_c1(self, a == 1 ? (u16)0x1E : (u16)0x1F, 2, 0);   /* srawi r4 ; addi r4,r4,0x1f ; clrlwi r4,r4,16 */
        em_mot_set(self, (u16)(a == 1 ? 0x1E : 0x1F), 2, 0);         /* srawi r4 ; addi r0,r4,0x1f ; clrlwi r4,r0,16 */
```
