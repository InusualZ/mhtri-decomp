---
id: 320
title: A forward jump into a later case is a fall-through guarded by a local that is live anyway
status: works
problem: Retail's case A branches over case B's leading test into case C's body; with no goto and no case label inside a block the duplicated tail or the reordered cases cost 5-10 points of score.
tags: [source-shape, allocator]
applies: [Wii/1.0, Wii/1.3]
demo: 
related: [183, 213, 219]
---

# 320. A forward jump into a later case is a fall-through guarded by a local that is live anyway

**Problem.** Retail's case A ends by branching forward past case B's leading test (or prefix) into the shared body
that follows it. Goto-free and label-free, the options are a duplicated tail (the function grows, every later branch
moves: `pl_act_fishing` 90.2 %) or moving the shared body (positional diff: 77 %).

**Why it happens.** The jump skips code that sits between two case labels; only a test in the skipped code can
reproduce the skip once case A falls through.

**How to work it.** Keep retail's order. Case A falls through into B; B's skipped part is wrapped in `if (guard)`,
where `guard` is something that already differs between the direct entry and the fall-in:

* a local the code already keeps live (`pl_act_fishing`: `item = 0` before the switch, 0x1A on the grade-1 path, so
  the grade 2/3 prefix is `if (item != 0x1A)`; `lobby_net_update`: `i = -1` before the switch, a loop counter >= 0 on
  case 0's fall-in); no new register, so the colouring does not move;
* the switch variable reloaded (`pl_act_net_hook_b`: `if (field == 0x17F)` around the 0x17F arm placed between
  case 2 and the 0x246 arm);
* a new `s32 resume` local (`plsearch_page19_step`) works but takes a callee-saved register and shifts the others by
  one: prefer a live local.

The guard costs one compare and a branch (3 instructions); a fresh flag also costs the `li` and a register.
Check the prologue (`_savegpr_N`) after adding one: a flag that is live across the whole switch keeps its register
through every call and can stop the compiler reading a known 0 from another local's register
(`pl_act_net_hook_main`: regalloc dump, the separate `li 0` temp conflicts with all four saved locals).

**Result.** `pl_act_fishing` 90.22 -> 99.39, `plsearch_page19_step` 92.89 -> 98.06, `lobby_net_update` 93.20 -> 96.21,
`pl_act_net_hook_b` 97.06 -> 97.88. Not applicable where the guard would sit in front of a dispatch compare that
retail folds into a range test (`pl_gather_item_take`: 97.2 % kept) or where retail jumps backwards.

**Example.**

```
case 1:
    item = ...;                 /* 0x1A on the path that skips the prefix */
    if (item != 0x1A) { ...; return; }
case 2:
case 3:
    if (item != 0x1A) { prefix(); }   /* item == 0 on a direct entry */
    shared_tail();
```
