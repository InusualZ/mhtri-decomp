---
id: 251
title: A loop that hoists only its 0/1 stores (each zero its own register) wants #pragma opt_strength_reduction off
status: works
problem: Retail's loop preheader copies one zero into a dozen registers and stack slots (`mr r22,r16` ..., `stw r16,0x130(r1)` ...) while ours hoists colour bytes, timer values and table addresses and shares one zero; the row is 200+ bytes short.
tags: [pragma, allocator]
applies: [Wii/1.3]
demo: 
---

# 251. A loop that hoists only its 0/1 stores (each zero its own register) wants #pragma opt_strength_reduction off

**Problem.** A long loop body full of `field = 0` / `field = 1` stores: retail's preheader materialises the zero once
(`li r16,0`, the offset counter) and then copies it into a separate register or stack slot for every store site
(`mr r22,r16` ... `mr r14,r16`, `stw r16,0x130(r1)` ...), with the one `1` in a single register; every other
constant (`li r0,0xFF` colour bytes, `li r0,5` timers, `lis/addi` table addresses) stays inline in the body. Ours
hoists those other constants and addresses into saved registers and folds every zero into one register, so the row is
hundreds of bytes short and every register after the prologue differs.

**Why it happens.** Under `-O3` the per-function loop pass hoists cheap invariants out of the loop. With
`opt_strength_reduction off` (`optimize_for_size on` gives the same bytes here) MWCC keeps the constants and
addresses at their uses and only the invariant stores' zero/one copies leave the loop, exactly the retail preheader.
`opt_common_subs off`, `opt_loop_invariants off`, `optimization_level 4` and `2` do not reproduce it.

**How to work it.** Bracket the one function with `#pragma opt_strength_reduction off` / `#pragma
opt_strength_reduction reset` and re-measure the unit; never file-wide without measuring every row.

**Result.** `lobby/fn_801E0ADC` `em007_model_eft_update` (3316 B): 88.81 (3072 B) -> 98.33 (3312 B); with the
range test spelled `(u16)(step + 0xFFFE) <= 2` (retail's `addis`+`addi`), 98.47 at 3316 B, the residual register
colouring only. No other row of the unit moved.

**Example.**

```
#pragma opt_strength_reduction off
void em007_model_eft_update(_EFT* eft)
{
    ...
    for (i = 0; i < work->model_count_0x00; i++) {
        switch (work->mode_0x28) { ... work->state_0x2B = 0; work->hold_timer_0x54 = 0; ... }
        ...
    }
}
#pragma opt_strength_reduction reset
```
