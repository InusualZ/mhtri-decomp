---
id: 179
title: A stack copy whose address is passed is a by-value struct parameter
status: works
problem: the caller copies a local struct word by word (lwz/stw) to a fresh stack slot and passes that slot's address
tags: [source-shape, allocator]
applies: [hud/cockpit_quest]
demo: 
---

# 179. A stack copy whose address is passed is a by-value struct parameter

**Problem.** Before a call, retail copies a struct it already has on the stack (`lwz r0,16(r1); stw r0,12(r1)`, or
three word pairs for a `VEC3`) into a new slot and passes `addi rN,r1,12`; a source that passes `&local` has no copy,
and one that copies by hand into a second local (`T tmp = local; f(&tmp);`) copies field by field (`lha/sth`) and
schedules the copy elsewhere.

**Why it happens.** MWCC passes a struct argument by value as a pointer to a caller-made temporary: the copy is the
compiler's, word-wise, placed right before the argument setup.  The callee body is the same either way (it reads
through the pointer), so the callee row matches under both spellings and only the callers show it.

**How to work it.** Look for the copy-then-address shape at each call site; declare the callee's parameter by value
(`void f(VEC3 pos)`, `void g(..., _mh_ivec2_ pos)`), spell its uses `pos.x` instead of `pos->x`, and pass the struct
itself.  Re-measure the callee too (it should not move).

**Result.** hud/cockpit_quest: `cockpit_shell_marker_draw` 91.1 -> 100, `cockpit_eye_marker_draw`/`_split` written at
100 with `cockpit_aim_marker_draw(VEC3)`; `cockpit_split_marker_draw` 100 with `quest_marker_draw(..., _mh_ivec2_ pos)`
(the old hand copy in `quest_marker_arm` stayed at 100 with the plain call).

**Example.**

```
void cockpit_aim_marker_draw(VEC3 pos);      /* not (const VEC3* pos) */

void cockpit_shell_marker_draw(_PLW* plw) {
    VEC3 pos;
    VEC3_ctor(&pos);
    eft053_shell_pos_project(plw, &pos);
    cockpit_aim_marker_draw(pos);           /* retail: three lwz/stw pairs + addi r3,r1,8 */
}
```
