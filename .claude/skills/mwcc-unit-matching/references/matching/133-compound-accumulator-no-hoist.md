---
id: 133
title: An accumulator written acc += x keeps retail's per-iteration zero load
status: works
problem: Retail reloads `lfs fN,0.0f` inside an unrolled accumulator loop on every iteration, where ours hoists the zero out of the loop and the instruction count differs.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [38, 124]
---

# 133. An accumulator written acc += x keeps retail's per-iteration zero load

**Problem.** A float accumulator reset and summed inside a loop (the comb filters of a reverb callback) has
retail's `lfs fN,<0.0f>` inside the loop body for every iteration, while ours loads the zero once in the preheader;
the loop is a different length and every later register shifts.

**Why it happens.** `acc = acc + x` and `acc += x` are the same arithmetic but not the same IR: the compound form
keeps the accumulator's reset and update as one read-modify-write per iteration, which the loop-invariant pass
leaves where it is, while the plain assignment exposes the zero as an invariant operand it can hoist.

**How to work it.** Where retail reloads a constant inside a loop that ours hoists, look for an accumulator
written as `x = x + y` and spell it `x += y`; measure the row (no other function should move).

**Result.** Wii/1.3: `AX/AXFXReverbHiExp` `AXFXReverbHiExpCallback` +2.2 and `AX/AXFXReverbStdExp`
`AXFXReverbStdExpCallback` +1.5 points from the `+=` spelling alone (unit headers' SHAPES lines).

**Example.**

```c
comb = 0.0f;
for (j = 0; j < 3; j++) {
    line = (f32*)reverb->delay2[c][j];
    a = line[reverb->filter_idx[j]];
    comb += a;                        /* not: comb = comb + a */
    line[reverb->filter_idx[j]] = delayed + a * reverb->filter_gain[j];
}
```
