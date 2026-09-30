---
id: 18
title: Named temporaries, declaration order and operand order steer the allocator
status: works
problem: Once the opcodes, sizes and relocations all match, the residual is often nothing but register numbers, and it looks unreachable from the source side.
tags: [source-shape, allocator]
applies: [Wii/1.3, GC/3.0a3]
demo: 018-temporaries-and-operand-order.cpp
reviewed: 2026-09-29
related: [19, 20, 22, 35, 63, 72]
---

# 18. Named temporaries, declaration order and operand order steer the allocator

**Problem.** Once the opcodes, sizes and relocations all match, the residual is often nothing but register
numbers, and it looks unreachable from the source side.

**How it looks.** The first divergence is the same instruction with a different register (`r4` where retail has
`r0`, `mr r31,r3` where retail has `mr r30,r3`), or a compare with its two operands swapped, or a whole
`_savegpr_14` vs `_savegpr_15` prologue (one more callee-saved register in use).

**Why it works.** MWCC colours live ranges from the *source's* temporary structure, not only from the data flow.
An unnamed sub-expression is a short-lived temp with its own range; a named local gets its own colour; the order in
which two locals are declared (or assigned) decides which one gets the lower register; and operand order is
observable, because `a < b` and `b > a` emit `cmplw` with swapped operands.

**How to work it.** Change one thing at a time and re-read the first divergence:

1. Name a repeated sub-expression (`u32 no = p->name_offset;`) - or inline a named one.
2. Swap the operands of a compare (`hash == p->hash` vs `p->hash == hash`).
3. Reorder the declarations of two locals (idea 63 shows locals are coloured before parameters).
4. Give a value a pointer type before a byte offset (or the reverse); use `int` vs `u32` for a count (idea 72).
5. Stop when the residual is the exact mirror of retail (idea 22) - or try the dead-copy trick (idea 35).

**Example** (measured at the time, `RSO/runtime`):

```c
/* 61.45 % -> 100 %: signed count for the record-form test, named pointer for the colour */
int count = pModule->import_symbol_table_size / 12;   /* srwi. r0,r0,3 + ble */
u32 offset = 0;
RSOImport* pEntry;                                    /* r4; unnamed, the offset is pushed to r6 */
while (count-- > 0) {
    pEntry = (RSOImport*)((u8*)pModule->import_symbol_table_offset + offset);
    if (pEntry->code_offset == pModule->unresolved_function_offset) return FALSE;
    offset += 12;
}
```

**Demonstration.** `018-temporaries-and-operand-order.cpp` (`ideas.py demo-check 18`): `p->hash == hash` gives
`cmplw r0,r4` and `hash == p->hash` gives `cmplw r4,r0`; `a < b` gives `cmplw r3,r4` and `b > a` gives
`cmplw r4,r3`; a value read twice is loaded once into its own callee-saved register when named
(4 `lwz`, the fourth being the epilogue's) and reloaded when the expression is repeated (5 `lwz`).

**When NOT to apply.** A residual that survives twenty spellings is not a source-shape one: see idea 22 (mirror)
and idea 35 (dead copies). Do not rename variables to steer the allocator in *finished* code without
re-measuring the whole unit (a name change can move a neighbour, idea 24).

**Evidence** (`RSO/runtime`, dated 2026-09-2x, the single biggest lever on that unit). `fn_804DA7E4` 61.45 ->
**100 %** (signed `int count` for the `srwi.`+`ble` entry test plus a named `RSOImport* pEntry` so the pointer lands
in r4 and the byte offset in r5 instead of r0/r6); `fn_804DA834` 81.56 -> **100 %** (a named `u32 no =
<entry>.name_offset` at all three `strcmp` sites, a named pointer *before* a named offset in the backward scan,
and `hash == p->hash` rather than the reverse); `RSOLink` 98.20 -> **100 %** (function-scope declaration order:
the loop pointer before the entry pointer). `RSOStaticLocateObject` needed the message as a local `char* msg =
...`; using the symbol inline cost an extra `@ha` register and a whole `_savegpr_14` vs `_savegpr_15` colouring.
