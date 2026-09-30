---
id: 18
title: Named temporaries, declaration order and operand order steer the allocator
status: works
problem: Once the opcodes, sizes and relocations all match, the residual is often nothing but register numbers, and it looks unreachable from the source side.
tags: [source-shape, allocator]
applies: []
demo:
---

# 18. Named temporaries, declaration order and operand order steer the allocator

**Problem.** Once the opcodes, sizes and relocations all match, the residual is often nothing but register
numbers, and it looks unreachable from the source side.

**Why try it.** MWCC colours live ranges from the *source's* temporary structure, not only from the data
flow: an unnamed sub-expression is a short-lived temp with its own range, a named local gets its own
colour, and the order in which two locals are declared (or assigned) decides which one gets the lower
register. Operand order is observable as well - `a != b` and `b != a` emit `cmplw` with swapped operands.

**Result.** The single biggest lever on `RSO/runtime`, closing the last 2-39 % of three functions:
`fn_804DA7E4` 61.45 -> **100 %** (a signed `int count` for the `srwi.`+`ble` entry test plus a named
`RSOImport* pEntry` so the pointer lands in r4 and the byte offset in r5 instead of r0/r6),
`fn_804DA834` 81.56 -> **100 %** (a named `u32 no = <entry>.name_offset` at all three `strcmp` sites, a
named pointer *before* a named offset in the backward scan, and `hash == p->hash` rather than the
reverse), `RSOLink` 98.20 -> **100 %** (function-scope declaration order: the loop pointer before the entry
pointer). `RSOStaticLocateObject` needed the message as a local `char* msg = ...`; using the symbol inline
cost an extra `@ha` register and a whole `_savegpr_14` vs `_savegpr_15` colouring.

**Example**

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
