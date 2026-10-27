---
id: 215
title: A dead value in r3 at a call to a void(void) callee: declare the callee with the owner's empty parameter list
status: works
problem: Retail calls a function with some value already in r3 and no extension, while ours passes it as an argument (an extra `extsb`/`clrlwi`, the call moved before a store) because a consumer's declaration invented a parameter.
tags: [source-shape]
applies: [Wii/1.3]
demo: 
related: [178]
---

# 215. A dead value in r3 at a call to a void(void) callee: declare the callee with the owner's empty parameter list

**Problem.** Retail reads a field into r3 (`lbz r3,0(r30)`), stores derived values, then calls a function with
r3 untouched. Read naively, r3 is the argument, so a consumer's declaration grew a parameter (`void
camera_lobby_mode_req(s8 step)`) and the call was written `camera_lobby_mode_req(work->step_0x00++)`. Ours then
sign-extends the argument (`extsb r3,r3`) and issues the call before the stores that follow the increment.

**Why it happens.** r3 is the first scratch register, so the allocator naturally leaves the loaded byte there
after `addi r0,r3,1`; a call that takes no argument simply does not clobber it first. The owner's definition is
the authority on the arity, not the register state at the call.

**How to work it.** When a call's "argument" is unextended, unused by the callee's own body, or its register is the
plain leftover of an earlier load, read the owner's definition (`symedit.py at <addr>`, the owner's source). Fix
the leaf/owner header to the real signature and write the side effects as statements in source order
(`work->step_0x00++; work->count_0x06 = 89; camera_lobby_mode_req();`).

**Result.** `lobby/lb_npc` (Wii/1.3): `lb_arm_wrestle_step` loses the `extsb` and its call order matches (the
call after the `sth` of the counter); with idea 214 the row goes 91.44 -> 98.88 before the remaining loop fixes.
The owner `camera/camera_main.cpp` defines `camera_lobby_mode_req(void)`; the leaf header
`src/camera/camera_lobby_mode_req.h` had declared `(s8 step)`.

**Example.**

```
/* retail: lbz r3,0(r30); addi r0,r3,1; stb r0,0(r30); li r0,89; sth r0,6(r30); bl camera_lobby_mode_req */
work->step_0x00++;
work->count_0x06 = 89;
camera_lobby_mode_req();
```
