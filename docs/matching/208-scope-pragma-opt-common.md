---
id: 208
title: Scope #pragma opt_common_subs off to a function whose retail reloads what MWCC keeps
status: works
problem: Retail loads a value a second time where ours keeps it in a register - a field re-read after a call, a loop test's load repeated at the top of the body, or an indirect call's table pointer loaded after the arguments instead of before them.
tags: [pragma, source-shape]
applies: [Wii/1.3]
demo: 
related: [120, 130]
---

# 208. Scope #pragma opt_common_subs off to a function whose retail reloads what MWCC keeps

**Problem.** Retail loads a value again where ours reuses the first load: a struct field re-read after a call
(`lha r4,0(r29)` after `bl get_joint_num`), the loop condition's `lhz` repeated at the top of the body, or a
table call's pointer (`lwz r8,shell_set_func_ptr@sda21`) loaded after `r3..r7` are set instead of before them.
The row is one or two instructions longer than ours, or the same length with one load moved.

**Why it happens.** MWCC's frontend common-subexpression pass merges the two reads (for a table call, the callee
load with the last argument, which is the same pointer) into one value that is then scheduled first. Retail built
these functions with that pass off while the rest of the unit keeps it on, so it is a per-function setting.

**How to work it.** Bracket the one function with `#pragma opt_common_subs off` above it and
`#pragma opt_common_subs reset` after it, then re-measure the whole unit. It is narrower than
`global_optimizer off` (idea 120): propagation, loop-invariant motion and strength reduction stay on. Never apply
it file-wide. A field that retail reloads after its own store is idea 130 (`volatile`), not this.
When the pragma fixes the table call but costs the row a value retail *does* keep (a repeated `(u32)(kind - 2)`
test in one case, two fields read once and compared several times), name that value as a local inside the scoped
function: the pass no longer merges it, the local does, and its declaration position decides its register.

**Result.** `ai/ai_npc` (Wii/1.3, `-O3` with `optimization_level 4`): `ai_npc_weak_point_pos_get` 88.66 -> 100,
`fn_802D32B4` 96.32 -> 100, `ai_npc_step57_arm` 94.48 -> 100, `ai_npc_m3_step6_arm` 95.23 -> 100,
`ai_npc_m3_step19_arm` 98.18 -> 100, `ai_npc_step31_arm` 98.29 -> 100, `ai_npc_step13_arm` 99.29 -> 100, and four
more rows up; no row moved down. The `shell_set_func_ptr->slot(..., shell_set_func_ptr)` calls of those step arms
had resisted an inline helper and every argument spelling. File-wide the same pragma took 13 rows up and 23 down
(-195 bytes of matched code). `enemy/em024_ai`: ten shell-job rows bracketed, nine kept (+123 matched bytes); the
tenth (`em024_act7_sub16`, 99.10 -> 98.06 under the pragma) reached 100 once its case-local `(u32)(kind - 2)` became
a function-level `kind_off` declared before `joint`, and `em024_act7_sub26` reached 100 with its two field reads
named (`first`/`second`).

**Example.**

```
#pragma opt_common_subs off
void ai_npc_weak_point_pos_get(_AINPC_W* self, EmWeakPoint* point, _ENEMY_WORK* enemy, nw4r::math::VEC3* out)
{
    ...
    if ((u32)point->joint >= ((MHchar*)enemy->char_0x024)->get_joint_num()) {   /* retail re-reads joint */
    ...
}
#pragma opt_common_subs reset
```
