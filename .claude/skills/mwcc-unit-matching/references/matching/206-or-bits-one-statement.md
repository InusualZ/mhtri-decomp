---
id: 206
title: A run of OR-ed constant bits kept as separate ori: one compound assignment per bit
status: works
problem: retail ORs a word with several constants as one `ori`/`oris` each between one `lwz` and one `stw`, where `*p = *p | A | B | C` folds them into a single `ori` of the combined mask
tags: [source-shape]
applies: [Wii/1.3]
demo: 
---

# 206. A run of OR-ed constant bits kept as separate ori: one compound assignment per bit

**Problem.** A mask builder sets several bits of an out-parameter (`*kinds = *kinds | 0x80 | 0x100 | 0x200 | ...`).
Ours emits one `lwz`, one `ori r0,r0,0xc780` and one `stw`; retail emits the same `lwz`/`stw` pair with one `ori`
per constant (`ori 0x80`, `ori 0x100`, ...), so the row is short by several instructions and every later offset moves.

**Why it happens.** In one expression MWCC folds the constant operands of an `|` chain before code generation. As
separate statements (`*kinds |= 0x80; *kinds |= 0x100;`) each statement is its own `|`; the optimizer then forwards
the stored value to the next load (one `lwz`, one `stw`) but does not merge the constants.

**How to work it.** Count the `ori`s between the load and the store in the retail listing and write one `|=` per
constant, in retail's order.

**Result.** `lobby/lb_pane_ui` (cflags_lobby, Wii/1.3, `-O3`): `lb_entry_pane_sort_masks_get` 93.36 -> 100 %
(three runs of three to six bits).

**Example.**

```c
/* retail: lwz r0,0(r4); ori r0,r0,0x80; ori r0,r0,0x100; ori r0,r0,0x200; stw r0,0(r4) */
*kinds |= 0x80;
*kinds |= 0x100;
*kinds |= 0x200;
```
