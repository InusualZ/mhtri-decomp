---
id: 224
title: An -O4-unrolled loop's register order follows the declarations and its for-header increment order
status: works
problem: Under `#pragma optimization_level 4` a hand-unrolled loop (or a short indexed loop) matches every opcode but the counter and the walking pointer sit in each other's registers, and the per-iteration `addi`s come in another order.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
---

# 224. An -O4-unrolled loop's register order follows the declarations and its for-header increment order

**Problem.** In an -O4 unit retail's short constant-count loops are unrolled by MWCC (a body copied 4 or 8 times
inside a `mtctr`/`bdnz` loop, or straight-line). A source that spells the copies out by hand, or indexes `p[i]`, gets
the opcodes but swaps the counter's and the walking pointer's registers, and the closing `addi`s (counter, pointer,
second index) come in a different order.

**Why it happens.** The unroller copies the body of the loop the source wrote, so the copies are retail's only when
the source is the plain loop. The allocator then colours the loop's variables in declaration order (first declared
gets the higher register), and the closing increments are emitted in the order the for-header lists them.

**How to work it.** Write the loop once (`for (col = 0; col < 8; ...)`), walk a pointer instead of indexing when
retail keeps one pointer with fixed displacements, declare the counter before the pointer, and order the
for-header increments as retail's closing `addi`s read (`col++, entry++, index++`).

**Result.** `menu/menu_result` (-O4 file-wide, idea 108): `q_result_box_list_draw` 92.06 -> 100 (the hand-unrolled
two-by-four copy became one loop over `col`, then `s16 col;` above `MenuListEntry* entry;`, then the header order),
`multi_box_rem_exist_ck` 96.27 -> 100 (the dead `li r3,0`/`addi r3,r3,7` counter retail keeps appeared once the
eight hand-written tests became `for (i = 0; i < 16; i++, items++)` with `s32 i;` declared first),
`multi_box_grid_clear` 97.40 -> 99.78, `q_result_equip_sel_count` 90.99 -> 100 (declaration order alone).

**Example.**

```
s16 col;                 /* declared before the pointer: retail's r6 */
MenuListEntry* entry;    /* r7 */

entry = entries;
for (col = 0; col < 8; col++, entry++, index++) {
    entry->present_0x02 = enabled;
    entry->index_0x06 = index;
}
```
