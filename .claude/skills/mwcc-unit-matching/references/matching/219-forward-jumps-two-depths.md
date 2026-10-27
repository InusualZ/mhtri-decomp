---
id: 219
title: Forward jumps into joins at two nesting depths: an inline helper's return for the far one, a do-while(0) break for the near one
status: works
problem: Retail jumps forward from one arm past a whole later pass into a shared block, and from another arm past that block too (or from a loop's limit exit past the code after the loop); a flag keeps its test (MWCC does not thread it) and the goto that matches is rule 8.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [145, 183, 213]
---

# 219. Forward jumps into joins at two nesting depths: an inline helper's return for the far one, a do-while(0) break for the near one

**Problem.** Retail's flow has two forward joins and jumps into each from different nesting depths - camera's wall
turn skips the push pass and lands in the ground clamp, the swimming line check skips the ground clamp, and the turn
loop's step limit skips the rotation after the loop. Written with flags (`turned = 1`, `if (turned == 0)`,
`if (blocked == 0)` after the loop) every flag keeps its `li`/`cmpwi`/`bne` and the colouring shifts.

**Why it happens.** MWCC has no jump threading and no tail merging (idea 213; also measured here on scratch TUs:
a constant flag before a `break`, the loop variable re-tested after its own loop, an inline function's constant
`return 1` tested by its caller, and identical `if`/`case` tails all keep their test or their copy, at `-O3` and
under `optimization_level 4`). The goto spelling and the flag spelling already differ in the first non-empty PCode
dump (`backend-03-after-unreachable-block-deletion`) by exactly the re-test block, and no backend pass removes it.
So the jumps have to be the source's own: `break`, `continue`, a `return`, and the `return` of an **inline**
function, which MWCC emits as a jump to the end of the inlined body.

**How to work it.** Read the target's predecessors of each join first. When every jump is *forward*:

1. the farthest join (the code after which both paths meet again) is the end of a `static inline` helper; the jump
   to it is an early `return` (return what the caller still needs - camera returns the hit kind);
2. the nearer join is the end of a `do { ... } while (0)` inside the helper; the jump to it is `break` (no loop or
   switch may sit between, or the `break` binds there);
3. a loop exit that skips the code after the loop is a second helper holding the loop and that code, the exit
   written as `return` - this keeps retail's `blocked = 1; b test` entry, which idea 145's `for (;;)` drops.

A `switch (0) { default: ... }` inside the `do { } while (0)` (idea 183's break/continue split) gives the same
layout without the helper, but it is a label used as a device (rule 8). A backward jump, or one into the middle of
another case, has no such spelling (idea 213). The helpers pay argument copies (idea 126): the by-value counter
gets its own register and the helper's temporaries other stack slots.

**Result.** `camera/camera_main` `camera_follow_collide` (Wii/1.3, -O3, peephole off): 95.70 (a `turned` flag) ->
**97.81**, 1516 -> 1476 B (= retail), every branch paired; goto ceiling in a scratch copy 97.96 (98.66 with `clamped`
hoisted and the floor stored to `pos.y` first). Measured on the way: rotation moved into a `for (;;)` 95.73, flag
kept for the turn inside `do { } while (0)` 95.31, `switch (0)` form 97.87 (rejected). Walls recorded with idea 213:
`enemy/em018_prog` `em018_motion_events` (case 0x55 into 0x56's tail) 98.70, `Network/network_pat_control`
`updateMessagePool` (states 3/4/6 backwards into states 1/2) 75.96, `quest/quest_entry` `quest_main_step` (fail and
clear tails entered backwards) 83.72.

**Example.**

```
static inline void wall_turn(Cam* cam, u32 step)
{
    u8 blocked = 1;
    while (blocked != 0) {
        step += 0x80;
        if (step >= 0x10000)
            return;                        /* jump past the rotation */
        blocked = wall_ck(cam, step);
    }
    rotate(cam, step);
}

static inline u8 move(Cam* cam)
{
    u8 kind = 0;
    do {
        if (wall) {
            wall_turn(cam, 0);
            break;                         /* jump into the ground clamp */
        }
        push(cam);
        if (swimming) {
            kind = line_ck(cam);
            return kind;                   /* jump past the ground clamp */
        }
        kind = line_ck(cam);
    } while (0);
    ground_clamp(cam);
    return kind;
}
```
