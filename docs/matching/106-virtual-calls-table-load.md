---
id: 106
title: A virtual call's table load through the saved object register: peephole folded retail's mr r3 + lwz r12,0(r3)
status: works
problem: A virtual call loads its table through the callee-saved object register (`lwz r12,0(r30)`) where retail moves the object into r3 first and loads through r3; a vtable store uses `addi r3` where retail uses `addi r0`
tags: [pragma, vtable]
applies: [Wii/1.3]
demo: 
related: [105]
---

# 106. A virtual call's table load through the saved object register: peephole folded retail's mr r3 + lwz r12,0(r3)

**Problem.** One register field differs in an otherwise identical row: retail spells a virtual call
`mr r3,r30` ... `lwz r12,0x0(r3)`, ours `lwz r12,0x0(r30)` (the `mr` still happens for the argument). A constructor's
vtable store shows the twin: retail `addi r0,r3,__vt__X@l; stw r0,0(r29)`, ours `addi r3,...; stw r3,...`.

**Why it happens.** The peephole pass rewrites a load through a register that was just copied into the copy's
source; retail's object was built with that rewrite off in these units, as with the kept `clrlwi` + `cmpwi` pairs.

**How to work it.** Put `#pragma peephole off` over the function (or the unit, when the unit's other rows also keep
retail's unfused forms) and re-measure every row of the unit. Try this before idea 105, which covers the same symptom
when the call takes by-value aggregate arguments.

**Result.** `g3d/g3d_scnobj` (file-wide `peephole off`): the ScnObj and ScnGroup constructors, DefG3dProcScnLeaf,
G3dProcGatherScnObj, G3dProcCalcWorld and Remove(ScnObj*) 99.76..99.94 -> 100, no row lower; `g3d/g3d_anmvis`
g3d_apply_vis_anm_result 99.90 -> 100; `g3d/g3d_scnmdlsmpl` the five DynamicCast instances 99.84 -> 100.

**Example.**

```
#pragma peephole off
void ScnGroup::G3dProcCalcWorld(u32 param, const math::MTX34* pParent) { ... mpScnObjArray[i]->G3dProc(...); }
```
