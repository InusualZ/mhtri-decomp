---
id: 316
title: A frontend-hoisted product loads its operands in reverse: opt_common_subs off restores left-first loads
status: works
problem: `lwz r3,x; lwz r0,y; mullw r3,r3,r0` in retail, while `x * y` gives the loads swapped and `y * x` gives `mullw r3,r0,r3`, for a product the function repeats (`x * y` and `x * y / 4`)
tags: [pragma, source-shape]
applies: [Wii/1.3]
related: [208, 289]
demo: 
---

# 316. A frontend-hoisted product loads its operands in reverse: opt_common_subs off restores left-first loads

**Problem.** A product of two fields that the function uses more than once (the SDK's THPSimple
`OSRoundUp32B(xSize * ySize)` then `OSRoundUp32B(xSize * ySize / 4)`) comes out one operand off whatever the spelling:
`x * y` emits the `ySize` load first, `y * x` emits the loads in retail's order but `mullw` reads them the other way.
Casts, `(&s)->x`, `*&x` and per-line operand mixes all give one of those two forms.

**Why it happens.** The frontend common-subexpression pass hoists the repeated product into a temporary, and the first
PCode dump (`tools/mwcc-debugger`, `backend-03-after-unreachable-block-deletion`) already shows its loads in the
**reverse** of the operand order with the multiply reading them in operand order. Left-operand-first loads with
`mullw left,right` are not reachable while the product is hoisted.

**How to work it.** Scope `#pragma push` / `#pragma opt_common_subs off` / `#pragma pop` around the function and write
the product in the SDK's `x * y` order. Without the frontend CSE the loads come out left first and the multiply keeps
the order; the backend still shares the product between the uses, so nothing else moves.

**Result.** `menu/movie`: `THPSimpleCalcNeedMemory` 99.58 -> 100 and `THPSimpleSetBuffer` 99.91 -> 100 (the only
diff in each was the `mullw` operand order), no other row moved; `y * x` under the pragma stays at the old score.

**Example.**

```
#pragma push
#pragma opt_common_subs off
u32 CalcNeedMemory(void) {
    u32 size = OSRoundUp32B(info.xSize * info.ySize);
    size += OSRoundUp32B(info.xSize * info.ySize / 4);
    return size;
}
#pragma pop
```
