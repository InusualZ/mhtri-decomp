---
id: 120
title: Scope `#pragma global_optimizer off` to one function
status: works
problem: A function's residual is a frontend-optimizer move - an `else` value computed above the branch, a loop-invariant hoisted, a load reused by CSE instead of reloaded, two loads scheduled in the other order - and turning the optimizer off for the whole file fixes it but breaks other functions.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
related: [16, 32, 33, 61]
---

# 120. Scope `#pragma global_optimizer off` to one function

**Problem.** A function's residual is a frontend-optimizer move - an `else` value computed above the branch, a
loop-invariant hoisted, a load reused by CSE instead of reloaded, two loads scheduled in the other order - and
turning the optimizer off for the whole file fixes it but breaks other functions.

**Why it happens.** `global_optimizer` is MWCC's frontend (IR) optimizer: hoisting, common-subexpression
elimination and code motion happen there, before the backend's scheduler and allocator. Retail built some
functions without those moves while the same unit's other functions need them, so it is a per-function
setting, not a unit flag (the unit's flags stay as they are, idea 33).

**How to work it.**

* Bracket only the one function: `#pragma push` / `#pragma global_optimizer off` above it, `#pragma pop` after
  it (idea 32: a region leaks to everything after it until it is reset).
* Re-measure the **whole unit**, not the function: a wider region is the failure mode.
* **When NOT to apply.** A pure register-allocation residual (same instructions, other register numbers) is
  not this idea: `RVLGX/GXAttr`'s `__GXSetVCD` got worse under the pragma.

**Result.** Measured on two RVLGX units; the evidence is each unit header's SHAPES line (`src/RVLGX/GXTransform.c`,
`src/RVLGX/GXAttr.c`).

* `RVLGX/GXTransform` `GXLoadTexMtxImm`: with the optimizer the `else` values are hoisted above the branches;
  the scoped pragma keeps both arms. The whole file off broke `GXLoadNrmMtxIndx3x3` and `GXSetClipMode`.
* `RVLGX/GXAttr` `__GXCalculateVLim` 87.47 -> 100 % (the VLim schedule loaded `tbl3[position]` before
  `tbl3[normal]`) and `GXSetVtxAttrFmtv` 96.70 -> 100 % (the format loop reused the loop test's `list->attr`
  instead of reloading it). The whole file off loses five other functions.
* `RVLGX/GXAttr` `__GXSetVCD` (a register-allocation residual) is worse under the pragma and stays outside it.

**Example.**

```c
#pragma push
#pragma global_optimizer off
void GXLoadTexMtxImm(const f32 mtx[][4], u32 id, s32 type)
{
    ...                 /* both if/else arms keep their own values */
}
#pragma pop
```

**More evidence.** `TRK/dolphin_trk` `TRKTranslateAddress` (Wii/1.3): with the optimizer on, the two MEM2
compares (`>= 0x10000000 && < 0x1C000000`) are merged into one `subis`/`subi` range test; the scoped
`global_optimizer off` keeps retail's two compares (unit header SHAPES line; the row's before/after was not
recorded). Idea 148 is a `todo` to try the same pragma on a switch lowered to a compare chain.
