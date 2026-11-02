---
id: 291
title: A self-reassignment after a load gives an inline helper's local a statement-time web that colours after the call result
status: works
problem: A function written as `return helper(...)` over a static inline helper matches every opcode but a callee-saved register pair is swapped - the helper's parameter or local that holds a loaded value takes r31 where retail gives r31 to the result of a nested inline call
tags: [source-shape, allocator]
applies: [Wii/1.3]
related: [243, 244, 278]
demo: 
---

# 291. A self-reassignment after a load gives an inline helper's local a statement-time web that colours after the call result

**Problem.** `DWCi_GsRealloc` is `return DWCi_ReallocBlock(9, block, size, 32)` over a static inline helper whose
body is `newBlock = DWC_AllocEx(...); if (block) { header = block - 1; oldSize = header->size; copy; DWC_Free }`.
Every opcode matches, but ours has `oldSize` in r31 and `newBlock` in r30 where retail has them the other way
round (ten operand rows). No declaration order of the helper's locals, no renaming, no block-local, no cast or
arithmetic on the load, no dead pre-assignment and no `opt_lifetimes off` moves either node.

**Why it happens.** Inside an inlined helper every variable is a frontend `@` temporary, and `tools/mwcc-debugger`'s
first PCode (dump 03) plus `regalloc-gpr-pass-1-all.txt` show how they are numbered: the helper's parameters and
locals get their temporaries at inline entry (`@963`, whatever their declaration order or name), a local assigned
from a nested inline call is rebound to that call's result temporary, created when its statement is lowered
(`@967`), and a local assigned an address expression is rebound to the expression's temporary at its statement
(`@979`). The allocator numbers the vregs in descending `@` order and colours from the highest, so an entry-time
temporary (the loaded `oldSize`) always colours before the nested call's result (`newBlock`), whatever the source
order. A plain load keeps the variable on its entry temporary; a *reassignment to itself* after the load
(`oldSize = (u32)oldSize;`) is "an assignment from another expression" in idea 278's sense and creates a fresh web
at that statement (`@981`), numbered after the call result, so the call result colours first and takes r31.

**How to work it.** When an inline helper's loaded local outranks a nested inline call's result in the colouring,
reassign the local to itself (with a cast, so it reads as deliberate) right after the load. The new web is created
at that statement, so it also sits *after* every temporary created by the statements before it - here the
`header` expression temporary, which is why the row stops short of 100: the two nodes created at statements keep
their statement order.

**Result.** `DWCi/dwc_init` (Wii/1.3, -O4,p): `DWCi_GsRealloc` 99.03846 -> 99.42308 (14 -> 9 differing bytes,
`newBlock` now r31 as retail; `header`/`oldSize` still r30/r29 for r29/r30), unit 99.80315 -> 99.88189, no other row
moved. Ruled out on the same row, all equal or worse: the six declaration orders of the helper's locals, 23
renamings, `oldSize` as a parameter or a block-local, a dead `oldSize = 0`/`= size`/`= -size` before the load, a
cast/`+ 0`/`* 1`/`| 0` on the load, an inline accessor for the size (loses the frontend CSE of `block - 1`, so the
`subi` moves after the copy call), a direct body (named locals number below every temporary, and `header` is always
a temporary), `DWC_Free` outside the null test, the free written through `header`, `#pragma opt_lifetimes off`.

**Example.**

```c
static inline void* DWCi_ReallocBlock(u32 name, void* block, u32 size, u32 align) {
    void* newBlock;
    u32 oldSize;
    DWCMemHeader* header;
    newBlock = DWC_AllocEx(name, size, align);          /* nested inline: result temporary @967 -> r31 */
    if (newBlock == NULL) {
        return NULL;
    }
    if (block != NULL) {
        header = (DWCMemHeader*)block - 1;              /* expression temporary @979 */
        oldSize = header->size;                         /* a plain load keeps the entry temporary @963 (r31!) */
        oldSize = (u32)oldSize;                         /* a fresh web @981, coloured after the call result */
        DWCi_Np_CPUCopyFast((u8*)newBlock, (const u8*)block, oldSize > size ? size : oldSize);
        DWC_Free(name, block, oldSize);
    }
    return newBlock;
}
```
