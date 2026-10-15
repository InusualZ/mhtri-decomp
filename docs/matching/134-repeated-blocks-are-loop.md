---
id: 134
title: Repeated per-element blocks over an array field are a source loop
status: works
problem: Three near-identical blocks over `field[0]`, `field[1]`, `field[2]` match the first two but the third addresses through `addi r9,r4,8` + `stw ..,0xc(r9)` in retail where ours uses a plain displacement.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [19, 110]
---

# 134. Repeated per-element blocks over an array field are a source loop

**Problem.** The function repeats one small block per element of an array field (a wrap of three ring indices,
`if (++idx[n] >= len[n]) idx[n] = 0;`). Written out three times, the first elements match but retail's last
iteration forms a base register (`addi r9,r4,8`) and stores through it with a residual displacement
(`stw r0,0xc(r9)`), where ours stores at the full displacement.

**Why it happens.** The repetition is the compiler's: a short constant-count loop is fully unrolled, and the
unrolled copies keep the induction variable's address arithmetic (the base `+ 8` for the third element), which a
hand-unrolled source never creates.

**How to work it.** When unrolled blocks match except for a base-register `addi` in a later copy, write them as a
`for (j = 0; j < N; j++)` loop over the array field and let MWCC unroll it.

**Result.** Wii/1.3: `AX/AXFXReverbHiExp` `AXFXReverbHiExpCallback` +2.9 points (the filter-index wraps) and
`AX/AXFXReverbStdExp` `AXFXReverbStdExpCallback` +3.7 points from the loop form.

**Example.**

```c
for (j = 0; j < 3; j++) {           /* not three copies of the wrap */
    if (++reverb->filter_idx[j] >= reverb->filter_len[j]) {
        reverb->filter_idx[j] = 0;
    }
}
```
