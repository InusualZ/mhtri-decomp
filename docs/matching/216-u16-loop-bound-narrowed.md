---
id: 216
title: A u16 loop bound narrowed after the counter's li: keep the call's raw result and narrow it in the loop condition
status: works
problem: Retail sets the loop counter (`li r27,0`) before narrowing the bound (`clrlwi r29,r3,16`), and ours narrows first, so the two instructions swap and the counter, pointer and bound registers rotate.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [19, 20]
---

# 216. A u16 loop bound narrowed after the counter's li: keep the call's raw result and narrow it in the loop condition

**Problem.** A search loop over `n = get_count()` entries: retail emits `mr r28,r3` (the base pointer), the count
call, `li r27,0` (the counter) and only then `clrlwi r29,r3,16` (the bound). Ours writes `u16 count =
(u16)get_count();` and emits the `clrlwi` straight after the call, before the `li`; the colouring of the three
loop registers follows the emission order, so every use in the loop body differs.

**Why it happens.** A narrowed initialiser is an assignment MWCC emits where the declaration is. A raw `u32`
local narrowed in the condition (`i < (u16)count`) is a loop-invariant expression, hoisted to the loop preheader
after the counter's initialisation.

**How to work it.** Declare the bound with the callee's own return type, unnarrowed, and narrow it where it is
compared. Try the declaration order of the locals separately (it moves the register numbers, not the schedule).

**Result.** `lobby/lb_npc` (Wii/1.3): `lb_arm_wrestle_step` 99.43 -> 99.99 (`u32 count = get_move_work_max(2);
... i < (u16)count`); declaration orders alone reached 99.43, `s32 count = (u16)...` 99.43.

**Example.**

```
_PLW* plw = (_PLW*)get_move_work_adrs(2);
u32 count = get_move_work_max(2);
s16 i;

for (i = 0; i < (u16)count; i++, plw++) {   /* li r27,0 then clrlwi r29,r3,16 */
```
