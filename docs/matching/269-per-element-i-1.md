---
id: 269
title: A per-element (i + 1) * 2 index beside a strength-reduced i * 2 is an unrolled inner loop over x[i + 1 - k]
status: works
problem: Retail recomputes `addi rX,i,1; slwi rX,rX,1; lhax` for x[i+1] each iteration while x[i] rides a strength-reduced offset and x[i+2] a pointer, and reloads the invariant coefficients inside the loop; ours walks one pointer and hoists them.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [258, 224]
---

# 269. A per-element (i + 1) * 2 index beside a strength-reduced i * 2 is an unrolled inner loop over x[i + 1 - k]

**Problem.** A filter loop reads `x[i + 2]`, `x[i + 1]` and `x[i]`. Ours folds all three into one pointer walk
(`lha 4(rP)`, `lha 2(rP)`, `lha 0(rP)`) and hoists the two coefficient loads out of the loop. Retail keeps three
different address forms - `x[i + 2]` through a pointer, `x[i]` through a strength-reduced `i * 2` offset with
`lhax`, and `x[i + 1]` as a fresh `addi; slwi; lhax` on every (unrolled) iteration - and reloads the
coefficients (`lha 0(rC)`/`lha 2(rC)`) inside the loop. `opt_loop_invariants off` reproduces only the reloads;
`opt_strength_reduction off` loses every loop; no compiler version or `-O` level changes it.

**Why it happens.** The source has a small inner loop over the predictor order, `for (k = 0; k < 2; k++) pred +=
x[i + 1 - k] * coef[k]`. MWCC fully unrolls it after the outer loop's invariants have been moved, so the
coefficient loads stay in the body, and the unrolled `i + 1 - 0` index is not recognised as an induction
expression (only the `k = 1` copy, which folds to `i`, joins the strength-reduced offset). The `k`-independent
`x[i + 2]` still gets its own pointer. An index written `x[i + k]` (adding the inner counter) does fold into a
displacement, so the subtraction is part of the shape.

**How to work it.** When one element of a neighbourhood is re-indexed per iteration while its neighbours are
strength-reduced, rewrite the closed-form sum as an inner loop over the order with the subtracting index; if the
coefficient row is addressed as `base + 4 * v` (`add rR,rCoef,rV4`) rather than a walking pointer, index a flat
table `coef[v * 2 + k]` instead of `coef[v][k]`.

**Result.** `DSPADPCM/dspadpcm` (Wii/1.3, `cflags_base`): `encodeAdpcmFrame` 82.80 -> 90.36 % from the two order
loops plus the flat table (1276 -> 1352 B against 1344), then 95.42 % at 1344 B once the uninitialised
best-vector locals and the declaration order were fixed.

**Example.**

```
s32 pred = 0;
for (k = 0; k < 2; k++) {                    /* was pcm[i + 1] * coef[v][0] + pcm[i] * coef[v][1] */
    pred += pcm[i + 1 - k] * coef[v * 2 + k];
}
r = ((pcm[i + 2] << 11) - pred) / 2048;
```
