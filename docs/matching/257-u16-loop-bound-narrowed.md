---
id: 257
title: A u16 loop bound narrowed right after its call: hold it in an s32 local
status: works
problem: Retail narrows a u16 loop bound (`clrlwi r30,r3,16`) straight after the count call, ours emits it after the counter's `li`s in the preheader, so the bound and the walking pointer swap callee-saved registers.
tags: [source-shape, allocator]
applies: [Wii/1.3]
demo: 
related: [216, 63]
---

# 257. A u16 loop bound narrowed right after its call: hold it in an s32 local

**Problem.** A `for (i = 0; i < max; i++, p++)` walk over `get_move_work_max(2)` entries (a `u16` return):
retail emits `bl get_move_work_max; clrlwi r30,r3,16; li r29,0; li r28,0`, ours `bl; li; li; clrlwi r31,r3,16`.
Every use of the bound and of the walking pointer in the loop then differs by register (r30/r31 swapped).

**Why it happens.** With `u16 max = get_move_work_max(2);` the narrowing belongs to the int promotion at the
compare `i < max`, so MWCC re-creates it as a loop-invariant temporary in the preheader (after the counters'
`li`). That backend temporary is numbered above the named locals and is coloured first, taking r31. An `s32`
local receives the already-narrowed return value at the assignment, so the `clrlwi` stays beside the call and the
bound keeps its named-local number. This is the mirror of idea 216 (there retail wants the preheader form).

**How to work it.** Type the bound `s32` (the call's `u16` result is narrowed once, at the assignment). If the
`clrlwi` must instead land after the `li`s, idea 216 applies.

**Result.** `hud/cockpit_quest`: `cockpit_party_list_draw` 96.44 -> 100 %, `cockpit_icon_cursors_update`
95.79 -> 98.77 % (100 % after a declaration permutation); the same edit on two other `u16 max`/`u16 count` locals
of the unit measured worse (-9.8 B, -18 B), so it is per site.

**Example.**

```
s32 max;                       /* was: u16 max; */
max = get_move_work_max(2);
for (i = 0; i < max; i++, plw++) { ... }
```
