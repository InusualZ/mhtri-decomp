---
id: 324
title: A regsim plan that lists block-scope locals: hoist them to function scope in the plan's order
status: works
problem: regsim reports an EXACT plan whose order mixes function-scope and block-scope locals, and reordering the function-scope declarations alone leaves the callee-saved colours wrong
tags: [allocator, source-shape]
applies: []
demo: 
---

# 324. A regsim plan that lists block-scope locals: hoist them to function scope in the plan's order

**Problem.** `mt.py regsim run <unit> <fn>` prints `plan 1 ... EXACT` with entries such as `other line 2741, block 3`
between function-scope entries. Reordering only the function-scope declarations leaves the callee-saved colours
swapped (the diff is a pure r29/r30 or r26/r27 rename).

**Why it happens.** The colouring order follows the order the locals are declared in the whole function. A local
declared inside a loop or `if` block takes its number after every function-scope local, so a plan that wants it
earlier is unreachable from inside the block.

**By tool.** `mt.py regsim run <unit> <fn> --apply-order [--dry-run]` writes this edit for the best plan that
lists block-scope locals (names declared in two blocks stay in their blocks), measures it and keeps it only on a gain.

**How to work it.** Declare every named entry of the plan at function scope in the plan's order (initialisers become
assignments at the old position, `T x = f();` keeps its call order), then measure. A local that appears twice
(two blocks both name an `i`) stays in its blocks. `.pi` helpers are not needed: the edit is the declaration list
plus turning `T v = e;` inside the block into `v = e;`.

**Result.** `WUDiSyncCheckInquiry` 99.46 -> 100 (level/node/wbc/device hoisted before `next`),
`WUDiInsertDeviceNodeTail` 98.53 -> 100 (`other` hoisted after `level`, `node` declared after `bdAddr`),
`village_tex_load` 99.65 -> 100 (`entry`/`slot` hoisted between `buf` and `i`), `em_get_unique_work` 98.60 -> 100,
`NHTTPi_Startup` 95.8 -> 97.9, `snd_player_banks_load` 97.08 -> 98.11, `lb_kakou_weapon_flags_get` 97.29 -> 97.81.

**Example.**

```
/* before */ void f(void) { s32 next = 0xFF; ... { u32 level = ...; Node* node; ... } }
/* after  */ void f(void) { u32 level; Node* node; s32 next = 0xFF; ... { level = ...; ... } }
```
