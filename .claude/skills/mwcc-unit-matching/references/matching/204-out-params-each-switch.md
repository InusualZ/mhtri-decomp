---
id: 204
title: Out-params each switch case passes by address get their own stack pair: scope them per case
status: works
problem: Retail's frame is larger and every call site that takes `&a, &b` uses its own stack pair (0x24/0x20, 0x1c/0x18, ...), ours reuses one pair (0xc/0x8) for all of them
tags: [allocator, source-shape]
applies: [Wii/1.3]
demo: 
---

# 204. Out-params each switch case passes by address get their own stack pair: scope them per case

**Problem.** A function calls the same out-parameter helper (`em_get_unique_work(n, &enemy, &mini)`) from
several `switch` cases. Retail's frame is 0x10 bigger and each call site stores, passes and reloads its own
pair of stack slots; ours declares `enemy`/`mini` once at function scope, so every site shares one pair and
every `stw`/`addi`/`lwz` against the frame differs by its offset (plus the frame size itself).

**Why it happens.** MWCC gives each address-taken local its own stack home and does not overlap homes of
variables declared in different blocks. A function-scope pair is one home; a pair declared inside each case's
block is one home per case, laid out in declaration order (the first block's pair at the highest offset, below
the function-scope locals).

**How to work it.** Read the frame offsets the target uses at each call site. If they differ per site, open a
block per case (`case 2: { struct X* a = NULL; struct Y* b = NULL; ... break; }`) and drop the function-scope
pair. The function-scope aggregates keep their relative order (declare them in the order of their homes, the
highest offset first).

**Result.** `quest/quest_entry` (Wii/1.3, `cflags_menu`): `quest_intruder_step` 99.10 -> 99.76 % (frame
0x80 -> 0x90, all 40 frame-offset ARG diffs gone), `quest_successive_step` 99.62 -> 99.65 %, together with
declaring `rec_a` before `rec_b`.

**Example.**

```
switch (state) {
case 2: {
    struct _ENEMY_WORK* enemy = NULL;
    struct _ENEMY_MINI_WORK* mini = NULL;

    if ((u8)em_get_unique_work(0, &enemy, &mini) != 0) { ... }
    break;
}
case 5: {
    struct _ENEMY_WORK* enemy = NULL;
    struct _ENEMY_MINI_WORK* mini = NULL;
    ...
}
}
```
