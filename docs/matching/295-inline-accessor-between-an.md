---
id: 295
title: An inline accessor between an argument expression and an inlined callee numbers the value below the caller's locals
status: works
problem: An inlined callee's three loop values (a name argument, an offset table, the index) are the right instructions in the wrong callee-saved registers, the argument sitting highest where retail has it lowest, and no declaration order or named local reaches retail's order.
tags: [source-shape, allocator]
applies: [GC/3.0a5.2]
related: [126, 165, 243, 282]
demo: 
---

# 295. An inline accessor between an argument expression and an inlined callee numbers the value below the caller's locals

**Problem.** A function inlines a lookup twice (`FindAnimContent(pAnimBlock, pTargetPane->GetMaterial(i)->mName, 1)`);
the second copy matches every opcode but holds name/index/offsets in r27/r26/r24 where retail has r24/r27/r26.
Retail numbers the name *below* the caller's `pAnimBlock` and the offset table *above* it; ours has the name as the
highest node of the function and the table below `pAnimBlock`. A caller-side `const char* matName` declared at any
position (six measured), the callee's locals in any order, uninitialised-then-assigned spellings and a non-const
count all leave the pair swapped or move the name to the very bottom.

**Why it happens.** The colouring cascade of idea 243 reads the node numbers. A bare member read used as an argument
(`->mName`) is a frontend temporary numbered above every local, so it colours first. A named local is numbered in
reverse declaration order among the *named* locals only; the caller's expression-initialised locals (`pAnimBlock =
animRes.GetResourceBlock()`, the call results) take their initialiser's number and never move. The one class that
lands *between* the named locals and those initialiser temporaries is an **inline's parameter**: wrap the argument in
a one-line inline accessor (`GetMaterialName(pMaterial)`) and its `this`/parameter web is numbered below `pAnimBlock`
(idea 165 is the mirror image: an accessor's *return* temporary colours ahead; here the *parameter* colours behind).
The offset table has the same two spellings: read through the inline `ConvertOffsToPtr` its parameter copies number
it below `pAnimBlock`; written as the bare expression (`CONVERT_OFFS_TO_PTR`, idea 126) it is a temporary numbered
above. The two levers are independent and retail needs both: the accessor alone measures 99.57, the macro alone
99.61, together 100.

**How to work it.**
1. Read the three nodes' registers off the aligned diff and rank them against the caller's expression-initialised
   locals (`pAnimBlock` here): which must sit below, which above.
2. A value that must drop below them: pass it through a one-line `inline` accessor (`GetMaterialName(p)`) so the
   inlined callee's parameter carries it.
3. A value that must rise above them: spell its computation as a bare expression (the macro form of the helper).
4. Measure the combination, not each lever alone; each alone can score lower than the baseline.

**Result.** `homebutton/hkb_animation` (GC/3.0a5.2, cflags_hkb) `AnimPaneTree::Set` 99.61 -> 100 (608 B); the unit
25/26 -> 26/26 rows, flipcheck READY, trial flip `main.dol: OK`.

**Example.**

```
inline const char* GetMaterialName(const Material* pMaterial) { return pMaterial->mName; }

inline u16 FindAnimContent(const res::AnimationBlock* pAnimBlock, const char* animContName, u8 animContType) {
    const u32* animContOffsets = CONVERT_OFFS_TO_PTR(u32, pAnimBlock, pAnimBlock->animContOffsetsOffset); /* above */
    ...
}
animMatIdxs[i] = FindAnimContent(pAnimBlock, GetMaterialName(pTargetPane->GetMaterial(i)), 1);        /* below */
```
