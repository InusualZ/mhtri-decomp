---
id: 135
title: Index a table by the struct field, not by the validated local
status: works
problem: Same instructions, but the `mulli`/table-base pair of a table lookup swaps registers (r8/r9) after the index has been range-checked through a local.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [18, 124]
---

# 135. Index a table by the struct field, not by the validated local

**Problem.** A settings function reads a mode into a local, validates it, then indexes parameter tables. Ours
and retail have the same `mulli` and table base, but in swapped registers (ours r9/r8, retail r8/r9), and no
declaration order moves them.

**Why it happens.** Indexing by the local reuses the local's virtual register, born at the validation; indexing
by the field (`tbl[reverb->mode]`) creates a new load at the use, born later, which is coloured in the other
order (idea 124's mechanism).

**How to work it.** After a validation through a local, index the tables by the struct field the local was read
from; keep the local only for the check.

**Result.** Wii/1.3: `AX/AXFXReverbHiExp` `AXFXReverbHiExpApplySettings` 99.80 -> 100 and `AX/AXFXReverbStdExp`
`AXFXReverbStdExpApplySettings` 99.80 -> 100.

**Example.**

```c
u32 mode = reverb->mode;

if (mode >= 8) {
    return FALSE;
}
...
reverb->length0 = sHiExpModeLength[reverb->mode][2];    /* not: sHiExpModeLength[mode][2] */
```
