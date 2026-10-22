---
id: 178
title: A float argument loaded early among the integer argument setup: move the float parameter forward in the callee's declaration
status: works
problem: The lfs f1 of a float argument sits before (or after) the li/addi of the integer arguments in retail, and the whole argument block is misaligned in the diff
tags: [source-shape]
applies: []
demo: 
---

# 178. A float argument loaded early among the integer argument setup: move the float parameter forward in the callee's declaration

**Problem.** Retail loads `lfs f1,radius` right after `addi r4,...` and before `addi r5`/`li r6`..`li r9`; ours loads it
last, just before the `bl`, and six instructions show as moved (one function read 76 %).

**Why it happens.** MWCC materialises call arguments in parameter order. Registers are assigned per class (r3.. for
integers, f1.. for floats), so a float parameter's position in the list is invisible in the registers but decides
where its load is scheduled.

**How to work it.** Read where the float load sits among the integer argument setup and place the float parameter at
that position in the callee's declaration (the callee's own body is unaffected: f1 is f1 wherever it is declared).

**Result.** camera/camera_main: `coll_line_check(from, to, radius, hit, ...)` (radius third, not last) took
camera_follow_wall_dist 94.74 -> 100 and camera_follow_wall_ck 76.00 -> 100; `coll_push_out(pos, radius, a, b)` the same.

**Example.**

```
s32 coll_line_check(VEC3* from, VEC3* to, f32 radius, VEC3* hit, s32 kind, s32 mask, u8 area, s32 mode);
/* addi r3; addi r4; lfs f1; addi r5; li r6 ... li r9; bl */
```
