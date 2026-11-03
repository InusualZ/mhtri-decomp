---
id: 293
title: A single-use local assigned an address expression is substituted by its temporary; a second direct use keeps it a named node that colours last
status: works
problem: Every opcode matches but a pointer local computed from a parameter (`header = block - 1`) takes a callee-saved register above the function's other locals, and no declaration order moves it - retail has it in the lowest register of the group
tags: [source-shape, allocator]
applies: [Wii/1.3]
related: [243, 244, 291]
demo: 
---

# 293. A single-use local assigned an address expression is substituted by its temporary; a second direct use keeps it a named node that colours last

**Problem.** `DWCi_GsRealloc` computes `header = (DWCMemHeader*)block - 1`, loads `header->size`, copies, and frees
through `DWC_Free(name, block, size)` (an inline whose own `block - 1` the frontend CSEs into `header`). Every opcode
matches, but `header` takes r31 (or r30) where retail gives it r29, below the new block (r31) and the old size (r30).
`regsim search` on the dump names the fix - "declare @979 (`header`'s temporary) as a local" - yet `header` *is*
declared a local, and no declaration order, no inline-helper form, no `opt_propagation off`/`opt_lifetimes off`, no
`register`, no dead second assignment changes its register.

**Why it happens.** The first PCode dump (`backend-03`) has no `header` node at all: the frontend substitutes a local
that is assigned once from an address expression and used **once** (`header->size`) by the expression's `@` temporary,
then CSEs the inlined free's `block - 1` onto that temporary. A `@` temporary is numbered above every named local, so
it colours first (idea 243) and takes the highest callee-saved register. A local with a load as its definition
(`oldSize`) or with several uses is kept as a named node, numbered in declaration order below every temporary.

**How to work it.** Give the pointer local a second **direct** use in the source - here, write the free through it under
the block's own null test (`if (block != NULL) stDwcFreeFunc(name, header, oldSize);`) instead of through the inline
`DWC_Free(name, block, oldSize)` whose `block - 1` the frontend would otherwise CSE onto the temporary. The frontend
then keeps `header` a named local (dump: `r34 oldSize`, `r35 newBlock`, `r36 @979` becomes a named `header` below
them), and the declaration order (first declared = highest number) does the rest: `newBlock`, `oldSize`, `header`. A
second use that changes the expression (`header + 1` for the block, `header + 1` as the copy source) changes the code
(an `addi`, a `addic.` null test) and does not count; a dead second *assignment* does not either (the substitution is
decided per use, and the dead store is gone before numbering).

**Result.** `DWCi/dwc_init` (Wii/1.3, -O4,p): `DWCi_GsRealloc` 99.42308 -> 100 (the last 9 bytes), unit 10/10 rows,
flipcheck READY and the trial flip links the original DOL. Ruled out on the same row: the inline-helper form of idea
291 with the self-cast placed before the `header` statement (11 placements: a load re-binds the variable to its entry
temporary, so only a web created *after* the load survives), a second `header` definition or self-cast after the size
web (the CSE keeps the first temporary), the direct body under `opt_propagation off`, `opt_lifetimes off`, both, with a
dead `header = NULL`, with `register`, with `header + 1` as the free's or the copy's block (95-98.8 %).

**Example.**

```c
void* DWCi_GsRealloc(void* block, u32 size) {
    void* newBlock;                                  /* r31: first declared = highest named node */
    u32 oldSize;                                     /* r30 */
    DWCMemHeader* header;                            /* r29: kept named by its two direct uses */
    newBlock = DWC_AllocEx(DWC_ALLOCTYPE_GS, size, 32);
    if (newBlock == NULL) {
        newBlock = NULL;
    } else if (block != NULL) {
        header = (DWCMemHeader*)block - 1;
        oldSize = header->size;
        DWCi_Np_CPUCopyFast((u8*)newBlock, (const u8*)block, oldSize > size ? size : oldSize);
        if (block != NULL) {
            stDwcFreeFunc(DWC_ALLOCTYPE_GS, header, oldSize);   /* the second use; DWC_Free(…, block, …) is
                                                                   CSE'd onto the temporary instead */
        }
    }
    return newBlock;
}
```
