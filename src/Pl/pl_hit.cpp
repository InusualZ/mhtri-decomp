/*
 * Pl/pl_hit.cpp - the hit registry: the two live-hit lists (attacks and bodies), the per-frame hit pass that pairs
 *   them, the damage and reaction bookkeeping a resolved hit applies to its owners (players, enemies, AI NPCs), the
 *   enemy part spheres, the hit id lists and the small hit-entry accessors.  C++; the map's plain names are `extern "C"`.
 * RANGE. .text 0x80299ED8-0x8029F61C (55 functions); extab 0x80013524-0x80013624, extabindex 0x80030E34-0x80030FB4,
 *   .data 0x805CDC88-0x805CDE78, .bss 0x806AC8A8-0x806AC8B8, .sdata 0x80792278-0x80792298, .sdata2 0x8079A398-0x8079A3F0.
 *   Seams: the left edge is `Pl/pl_body_push.cpp`'s (its pool 0x8079A380-0x8079A398 repeats 0.0f/0.5f/40.0f/0.001f, which
 *   this unit's pool holds again at 0x8079A39C/0x8079A3E0..; `coll_push_out` and `body_push_pl_body` call 0x802999D8 and
 *   0x80299CE4, so the push code runs up to 0x80299ED8).  The right edge: the `.bss` hit registry 0x806AC8A8 is read
 *   from 0x80299ED8 to 0x8029F60C, the item table head 0x806AC8B8 from `get_item_data_ptr` (0x8029F61C, `menu/menu_item.cpp`)
 *   on, and the `__FILE__` "menu_item.cpp" is cited only after 0x802A5444.
 * FLAGS. `cflags_pl` (configure.py's Pl lib), plus `#pragma optimization_level 4` (the switch dispatches of
 *   `hit_apply_em`/`hit_shape_get` test their case ranges with `subi`+`cmplwi`, playbook 108; no row moves either way
 *   elsewhere) and `#pragma pool_data off` (the damage tables are addressed one `lis`/`addi` each, not off one base).
 * NAMES. GUESS: the file name (no `__FILE__` string in the range; the bodies run the hit registry).  The runtime dump's real
 *   names: `body_set`, `hit_flag_set`, `hit_result_check`; the map's own `get_hit_id__Fv`.  GUESS names from the bodies
 *   and callers: `hit_registry_clear`, `hit_registry_update`, `hit_life_update`, `hit_targets_reset`, `body_hit_mask_ck`,
 *   `hit_side_kind_get`, `hit_flag48_high_ck`, `hit_em_se_req`, `hit_em_effect_se_req`, `hit_level_se_req`,
 *   `hit_pl_effect_se_req` (the hit-sound dispatchers), `hit_dir_y_get`, `hit_dir_x_get`, `hit_owner_reaction_set`,
 *   `hit_owner_rate_get`, `pl_elem_resist_get`, `hit_damage_pl`/`_em`/`_npc` (the damage appliers),
 *   `hit_part_rate_get`, `hit_reaction_priority_get`, `hit_apply_pl`/`_em`/`_npc`/`_body` (the per-target contact
 *   tests), `ai_npc_hit_capsule_get`, `hit_area_match_ck`, `pl_hit_capsule_get`, `body_part_shape_get`, `hit_shape_get`,
 *   `hit_list_capsule_ck`, `hit_effect_spawn`, `hit_data_apply`, `hit_attack_list_push`, `hit_body_list_push`,
 *   `hit_source_set`, `hit_owner_set`, `hit_id_list_absent_ck`, `hit_id_list_clear`, `hit_id_list_push`, `hit_motion_set`,
 *   `hit_mask_ck`, `hit_flags_clear`, `hit_flag_clear`, `hit_flag_ck`, `hit_attack_flag_ck`, `hit_knock_set`,
 *   `hit_push_enabled_ck`, `hit_camera_push_enabled_ck`, `hit_body_list_get`; the data `hit_registry`,
 *   `hit_reaction_priority_tbl`, `hit_sharpness_side_rate_tbl`, `hit_result_gauge_rate_tbl`, `hit_sharpness_rate_tbl`,
 *   `hit_sharpness_elem_rate_tbl`, `hit_sharpness_status_rate_tbl`, `hit_result_sharpness_tbl` (the sharpness reading
 *   rests on the raw rates 0.5/0.75/1.0/1.05/1.2/1.32/1.5 by `_PLW` +0x56B).  `em_part_sphere_get` is the registry
 *   name `hud/cockpit` gave it.  Registry proposals for foreign callees (GUESS): `pl_act_kind7_ck` (0x8027C030),
 *   `em_hit_kind_accept_ck` (0x8012F110), `ai_npc_time_scale_get` (0x802D29DC), `se_hit_bank_req` (0x800DC0A8),
 *   `se_req_pos_id113_level` (0x800DC560), `se_req_pos_id110_level` (0x800DC348), `se_req_pos_id62/85/86/186/101/99/98/87`,
 *   `ai_npc_counter_roll` (0x802D3E4C), `eft013_guard_spawn` (0x80106DC8), `pl_attack_up_mark` (0x8027D698),
 *   `pl_defense_up_mark` (0x8027D6A4), `eft002_pair_spawn` (0x800FBE64), `eft002_model_spawn` (0x800FC1DC),
 *   `eft013_target_set` (0x80107914), `eft020_actor_spawn` (0x80114D28), `pl_sharpness_add` (0x80278674),
 *   `pl_class7_meter_add` (0x8027CA48).
 * RESIDUALS. Every row is written; the partial rows:
 *  - `hit_side_kind_get`: the branch sense of one `fcmpo`, and its tail block laid out in the other order;
 *  - `hit_id_list_absent_ck`: retail keeps an unused accumulator (`li r5,0`, `+9` per outer iteration) MWCC deletes;
 *  - `hit_registry_update`: register permutation, and the enemy kind test's 0x10 branch is laid out after the busy test;
 *  - `pl_elem_resist_get`: retail places the default case right after the compare chain;
 *  - `hit_owner_reaction_set`, `hit_dir_y_get`: register permutations; `hit_life_update`: retail reads the second motion
 *    pair through a `+4` base register;
 *  - `hit_level_se_req`, `hit_pl_effect_se_req`: retail computes the sound level into `r4` before it moves `pos` into
 *    `r3` (ours moves `pos` first; a level local, an if/else and an inline helper measure the same); the second also
 *    allocates its `pos`/`damage`/`guarded` registers in another order;
 *  - `hit_damage_pl`, `hit_damage_npc`: retail loads the 80.0 pool constant before the defence field in
 *    `damage * def / (def + 80)` (four spellings measure the same); `hit_damage_pl`'s element-hit block keeps one
 *    `extsh` retail lacks;
 *  - `hit_damage_em`: register permutation across its 14 callee-saved locals;
 *  - data: `.data`/`.bss` byte-identical; `.sdata`/`.sdata2` differ only by the target's tail padding; 21 callees carry
 *    registry names the map does not have yet, so a flip waits for those renames.
 * SHAPES. A hit whose owner kind is not 0/1/3 skips the motion check through `continue` in a `do { } while (0)`, as does
 *   `hit_damage_pl`'s status case that skips the build-up; the attack-kind tests take the kind into an `s32` (retail
 *   compares the single kinds with `cmpwi`, the pairs with `subi`+`cmplwi`); `hit_data_apply` copies the signed strength
 *   through a local before the sign test.  The midpoints are `(a + b) / 2.0f` (a `* 0.5f` swaps the `fmuls` operands).
 *   The 16-bit masks of `hit_damage_em` are cleared with 16-bit constants (`0xFFF7`), which keeps retail's `andi.`.
 */

#pragma optimization_level 4
#pragma pool_data off
#include "types.h"
#include "nw4r/math.h"
#include "Pl/hit_w.h"
#include "Pl/pl_hit.h"
#include "ef/fn_800CDB2C.h"   /* PlayMode_ck (rule 2: the owner is `ef/system_core.cpp`) */
#include "stage/stg_w.h"      /* get_now_areano (rule 2) */
#include "Pl/plw.h"
#include "Pl/pl_act.h"         /* `Pl_motion_input_ck`, `pl_motion_scratch_clear` (rule 2) */
#include "Pl/fn_802693C4.h"    /* `Get_motion_no` (rule 2) */
#include "Pl/pl_act_kind7_ck.h"
#include "Pl/fn_8025F088.h"    /* `get_move_work_adrs` / `get_move_work_max` (rule 2) */
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_8012EC74.h"  /* `em_get_mot_no` (rule 2) */
#include "enemy/fn_8012BDF4.h"  /* `em_busy_ck` (rule 2) */
#include "enemy/get_em_base_scale.h"
#include "enemy/em_damage_record_clear.h"
#include "enemy/em_hit_kind_accept_ck.h"
#include "ai/ai_npc.h"         /* `_AINPC_W`, the view with the model at +0x008 */
#include "ai/ainpc_w.h"
#include "ai/ai_get_motion_no.h"
#include "ai/ai_npc_time_scale_get.h"
#include "stage/shell.h"       /* `_SHELL_W`, a hit owner of kind 2 */
#include "fn_8004CAD8.h"       /* `calcVecAng2`, `calcVecAngX2` (rule 2) */
#include "mh3_pad.h"            /* `copyVec3` (rule 2) */
#include "g3d/g3d_calcworld.h"  /* `addVec3To` (rule 2) */
#include "ef/fn_800AEE48.h"     /* `vec3_sub_assign` (rule 2) */
#include "sound/mhchar_node_id_get.h"
#include "Pl/pl_hit_sphere.h"   /* `PlCapsule` */
#include "Pl/pl_joint_body.h"
#include "ef/effect.h"            /* `eft_vec_ctor`, `eft_shape_init` (rule 2) */
#include "ef/eft_capsule_ctor.h"
#include "sound/se_hit_bank_req.h"
#include "sound/se_req_pos_id113_level.h"
#include "sound/se_req_pos_id110_level.h"
#include "sound/se_req_pos_id62.h"
#include "sound/se_req_pos_id85.h"
#include "sound/se_req_pos_id86.h"
#include "sound/se_req_pos_id186.h"
#include "sound/se_req_pos_id101.h"
#include "sound/se_req_pos_id99.h"
#include "sound/se_req_pos_id98.h"
#include "sound/se_req_pos_id87.h"
#include "ai/ai_npc_counter_roll.h"
#include "ai/ainpc_area_buff_apply.h"
#include "ef/eft013_guard_spawn.h"
#include "Pl/pl_guard_ck.h"
#include "Pl/pl_attack_up_mark.h"
#include "Pl/pl_defense_up_mark.h"
#include "Pl/Pl_master_ck.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_sharpness_add.h"
#include "Pl/pl_class7_meter_add.h"
#include "Pl/pl_act_hold_notify.h"
#include "enemy/em_damage_hp_add.h"
#include "enemy/em_area_ck.h"
#include "enemy/em_act_trap_ck.h"   /* `em_attacker_rec_get` (rule 2) */
#include "ef/eft002_pair_spawn.h"
#include "ef/eft002_model_spawn.h"
#include "ef/eft013_target_set.h"
#include "ef/eft020_actor_spawn.h"
#include "ef/eft013_shell_spawn.h"
#include "ef/eft013_set_from_sel.h"
#include "Pl/pl_coll.h"           /* `HitSphere` (rule 2) */
#include "fn_8004CAD8/mtx.h"        /* `MTX34_ctor` (rule 2) */
#include "sound/mhchar_joint_mtx_get.h"
#include "enemy/em_flags836_ck.h"
#include "enemy/em_timer_0x94A_ck.h"
#include "g3d/mtx34_rot_xyz_deg.h"
#include "g3d/g3d_calcview.h"      /* `mtx34_concat` (rule 2) */

/* `pl_act_held_ck`: the owner (`Pl/pl_act.cpp`, declared in `Pl/pl_act.h`) defines a second, unused parameter its callers
 * never load, so the call goes through a one-parameter view of the same address (as in `Pl/pl_act_step.cpp`). */
#define pl_act_held_ck_c1 ((u32 (*)(_PLW*))pl_act_held_ck)

/* `em_water_check` (`enemy/fn_8012EC74.h` spells its return `f32`): the caller tests an integer, so it goes through a
 * `u32` view of the same address. */
#define em_water_check_c1 ((u32 (*)(_ENEMY_WORK*))em_water_check)

/* The unit's own rows the bodies above them call. */
extern "C" {
void hit_apply_pl(_HIT_W* hit, _PLW* plw);
void hit_apply_em(_HIT_W* hit, _ENEMY_WORK* em);
void hit_apply_npc(_HIT_W* hit, _AINPC_W* npc);
void hit_apply_body(_HIT_W* hit, _BODY_W* body);
s32 hit_shape_get(_HIT_W* hit, _HIT_SIZE_DATA* shape, PlCapsule* capsule, HitSphere* sphere);
s32 body_part_shape_get(_BODY_W* body, EmWeakPoint* part, PlCapsule* capsule, HitSphere* sphere);
void hit_damage_npc(_AINPC_W* npc, _HIT_W* hit, nw4r::math::VEC3* contact, nw4r::math::VEC3* hit_pos,
                    nw4r::math::VEC3* npc_pos);
void hit_effect_spawn(_HIT_W* hit, nw4r::math::VEC3* contact, nw4r::math::VEC3* from, nw4r::math::VEC3* to, f32 water_y,
                      EmPartRates* rates);
void ai_npc_hit_capsule_get(_AINPC_W* npc, PlCapsule* out);
void hit_damage_pl(_PLW* plw, _HIT_W* hit, nw4r::math::VEC3* contact, nw4r::math::VEC3* hit_pos,
                   nw4r::math::VEC3* pl_pos);
void pl_hit_capsule_get(_PLW* plw, PlCapsule* out);
void hit_damage_em(_ENEMY_WORK* em, _HIT_W* hit, EmWeakPoint* part, _HIT_SIZE_DATA* shape, nw4r::math::VEC3* contact,
                   nw4r::math::VEC3* hit_pos, nw4r::math::VEC3* em_pos);
}

/* The registry of live hits (`.bss`). */
HitRegistry hit_registry;

/* The priority of each damage reaction id (0x00-0x24); a hit's reaction replaces an enemy's when it ranks as high. */
u8 hit_reaction_priority_tbl[0x25] = {
    0, 1, 1, 1, 2, 2, 1, 1, 1, 1, 7, 6, 8, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 4, 2,
};

/* The cut rate by sharpness level (rows) and hit side kind (columns). */
f32 hit_sharpness_side_rate_tbl[7 * 6] = {
    0.4f, 0.3f, 0.5f, 1.0f, 1.05f, 0.3f,
    0.4f, 0.3f, 0.5f, 1.0f, 1.05f, 0.3f,
    0.4f, 0.3f, 0.5f, 1.0f, 1.05f, 0.3f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 0.3f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 0.3f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 0.3f,
    1.0f, 1.0f, 1.0f, 1.0f, 1.05f, 0.3f,
};

/* The class-7 gauge rate by hit result. */
f32 hit_result_gauge_rate_tbl[6] = {
    0.5f, 0.5f, 0.75f, 1.0f, 1.1f, 0.5f,
};

/* The raw damage rate by sharpness level. */
f32 hit_sharpness_rate_tbl[7] = {
    0.5f, 0.75f, 1.0f, 1.05f, 1.2f, 1.32f, 1.5f,
};

/* The element damage rate by sharpness level. */
f32 hit_sharpness_elem_rate_tbl[7] = {
    0.25f, 0.5f, 0.75f, 1.0f, 1.0625f, 1.125f, 1.2f,
};

/* The status build-up rate by sharpness level. */
f32 hit_sharpness_status_rate_tbl[8] = {
    0.5f, 0.75f, 1.0f, 1.05f, 1.2f, 1.32f, 1.5f, 0.0f,
};

/* The weapon wear by hit result: the plain value, then the value with flag 0x1000. */
s16 hit_result_sharpness_tbl[6 * 2] = {
    -2, -6, -2, -6, -1, -3, -1, -3, -1, -3, -2, -6,
};

/* 0x80299ED8 (0x20): clears the registry's list heads and counts. */
extern "C" void hit_registry_clear(void)
{
    hit_registry.head_0x00 = 0;
    hit_registry.head_0x04 = 0;
    hit_registry.count_0x08 = 0;
    hit_registry.count_0x0A = 0;
}

/* 0x80299EF8 (0x248): counts one attack hit down by its owner's time scale: the start delay first, then its live
 * frames and its two side values; ends the hit when its owner left the attack's motions.  Returns whether it is live. */
extern "C" s32 hit_life_update(_HIT_W* hit)
{
    f32 rate;

    if (hit->active_0x05 == 0) {
        return 0;
    }
    if (hit_flag_ck(hit, 0x100) == 1) {
        do {
            u16 motion;

            switch (hit->owner_kind_0x06) {
            case 0:
                motion = Get_motion_no((_PLW*)hit->owner_0x10);
                break;
            case 1:
                motion = em_get_mot_no((_ENEMY_WORK*)hit->owner_0x10);
                break;
            case 3:
                motion = ai_get_motion_no((_AINPC_W*)hit->owner_0x10);
                break;
            default:
                continue;
            }
            if (motion != hit->field_0x018 && motion != hit->field_0x01A && motion != hit->field_0x01C
                && motion != hit->field_0x01E) {
                hit->active_0x05 = 0;
                return 0;
            }
        } while (0);
    }
    if (hit->owner_kind_0x06 == 1 && hit_flag_ck(hit, 0x200) == 1) {
        rate = get_em_base_scale((_ENEMY_WORK*)hit->owner_0x10);
    } else if (hit->owner_kind_0x06 == 0 && hit_flag_ck(hit, 0x200) == 1) {
        rate = ((_PLW*)hit->owner_0x10)->field_0x358;
    } else if (hit->owner_kind_0x06 == 3 && hit_flag_ck(hit, 0x200) == 1) {
        rate = ai_npc_time_scale_get((_AINPC_W*)hit->owner_0x10);
    } else {
        rate = 2.0f;
    }
    if (hit->delay_0x38 <= 0.0f) {
        if (hit->time_0x3C > 0.0f) {
            hit->time_0x3C -= rate;
            if (hit->value_0x50 > 0.0f) {
                if ((hit->side_flags_0x33 & 1) == 0) {
                    hit->side_flags_0x33 |= 1;
                } else {
                    hit->value_0x50 -= rate;
                    if (hit->value_0x50 <= 0.0f) {
                        hit->value_0x50 = 0.0f;
                        hit->side_flags_0x33 |= 2;
                    }
                }
            } else if (hit->value_0x54 > 0.0f) {
                if ((hit->side_flags_0x33 & 2) == 0) {
                    hit->side_flags_0x33 |= 2;
                } else {
                    hit->value_0x54 -= rate;
                    if (hit->value_0x54 <= 0.0f) {
                        hit->value_0x54 = 0.0f;
                    }
                }
            }
            return 1;
        }
        hit->active_0x05 = 0;
    } else if (hit->delay_0x38 > 0.0f) {
        hit->delay_0x38 -= rate;
        if (hit->delay_0x38 <= 0.0f) {
            hit->delay_0x38 = 0.0f;
        }
    }
    return 0;
}

/* 0x8029A140 (0x5C): the four-way kind the entry's two side values select: 0 when the first is the unusable
 * default, 1/2 by which side carries a positive value and 3 when the second is the unusable default. */
extern "C" u32 hit_side_kind_get(_HIT_W* self)
{
    if (self->value_0x50 <= -100.0f) {
        return 0;
    }
    if (self->value_0x50 > 0.0f) {
        return 1;
    }
    if (self->value_0x54 > 0.0f) {
        return 2;
    }
    if (self->value_0x54 <= -100.0f) {
        return 3;
    }
    return 2;
}

/* 0x8029A19C (0xD8): clears every player's, enemy's and the NPC's per-frame hit state before the pass. */
extern "C" void hit_targets_reset(void)
{
    s32 i;
    u16 count;
    _PLW* plw;
    _ENEMY_WORK* em;

    plw = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < count; i++, plw++) {
        pl_motion_scratch_clear(plw);
    }
    em = (_ENEMY_WORK*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    for (i = 0; i < count; i++, em++) {
        em->field_0x833 = 0;
        em_damage_record_clear(em);
    }
    ainpc_w.field_0x216 = 0;
    ainpc_w.field_0x212 = 0;
    ainpc_w.field_0x21E = 0;
    ainpc_w.field_0x22A = 0;
    ainpc_w.field_0x230 = 0;
    ainpc_w.field_0x224 = 0;
}

/* 0x8029A274 (0xE4): whether the hit's attack-flag byte shares one of the body slot's kind bits (0x20, 0x10, 1, 2, 4, 8). */
extern "C" s32 body_hit_mask_ck(_BODY_W* body, _HIT_W* hit)
{
    u8 mask = body->source_kind;

    if ((mask & 0x20) != 0 && (hit->attack_flags_0x31 & 0x20) != 0) {
        return 1;
    }
    if ((mask & 0x10) != 0 && (hit->attack_flags_0x31 & 0x10) != 0) {
        return 1;
    }
    if ((mask & 1) != 0 && (hit->attack_flags_0x31 & 1) != 0) {
        return 1;
    }
    if ((mask & 2) != 0 && (hit->attack_flags_0x31 & 2) != 0) {
        return 1;
    }
    if ((mask & 4) != 0 && (hit->attack_flags_0x31 & 4) != 0) {
        return 1;
    }
    if ((mask & 8) != 0 && (hit->attack_flags_0x31 & 8) != 0) {
        return 1;
    }
    return 0;
}

/* 0x8029A358 (0x430): the frame's hit pass: resets the targets, counts the body slots' timers down, then pairs every
 * live attack of the registry with the players, enemies, the NPC and the body slots of its area that it has not hit. */
extern "C" void hit_registry_update(void)
{
    _HIT_W* hit;
    HitRegistry* reg;

    reg = &hit_registry;
    hit = reg->head_0x00;
    hit_targets_reset();
    if (Pl_motion_input_ck(1) != 1) {
        _BODY_W* body;

        for (body = reg->body_head_0x04; body != NULL; body = body->next_0x00) {
            if (body->field_0x005 != 0 && body->hit_timer_0x0A > 0) {
                body->hit_timer_0x0A--;
            }
        }
        for (; hit != NULL; hit = hit->next_0x00) {
            _PLW* plw;
            _ENEMY_WORK* em;
            u16 count;
            s32 i;

            if (hit_life_update(hit) == 0) {
                continue;
            }
            plw = (_PLW*)get_move_work_adrs(2);
            count = get_move_work_max(2);
            for (i = 0; i < count; plw++, i++) {
                if (plw->slot_active != 0 && plw->hit_pending_0x39E == 0
                    && hit_area_match_ck(hit->area_0x0F, plw->area_0x16) != 0 && plw->kind_0x09 != 8
                    && hit_id_list_absent_ck(plw->hit_ids_0x53C, hit->hit_id_0x0C) != 0) {
                    s32 kind = hit->kind_0x42;

                    if ((u32)(kind - 0xA) <= 1) {
                        continue;
                    }
                    if ((u32)(kind - 0xE) <= 1 || kind == 0x20) {
                        if (pl_act_kind7_ck(plw) == 0) {
                            continue;
                        }
                    } else if (pl_act_held_ck_c1(plw) == 1) {
                        continue;
                    }
                    if ((((hit->owner_kind_0x06 != 0 || hit->owner_0x10 != plw)
                             && (hit->kind_0x07 != 0 || hit->owner_0x14 != plw))
                            || hit_flag_ck(hit, 0x40) != 0)
                        && hit_attack_flag_ck(hit, 1) != 0) {
                        hit_apply_pl(hit, plw);
                    }
                }
            }
            em = (_ENEMY_WORK*)get_move_work_adrs(3);
            count = get_move_work_max(3);
            for (i = 0; i < count; em++, i++) {
                if (em->active != 0 && hit_area_match_ck(hit->area_0x0F, em->area_no) != 0
                    && hit_id_list_absent_ck(em->hit_ids_0xAC0, hit->hit_id_0x0C) != 0
                    && em_hit_kind_accept_ck(em, hit->kind_0x42) != 0
                    && (((hit->owner_kind_0x06 != 1 || hit->owner_0x10 != em)
                            && (hit->kind_0x07 != 1 || hit->owner_0x14 != em))
                        || hit_flag_ck(hit, 0x40) != 0)
                    && hit_attack_flag_ck(hit, 2) != 0) {
                    s32 kind = hit->kind_0x42;

                    if ((u32)(kind - 0xA) > 1) {
                        if (kind == 0x10) {
                            if (em->team != 0x19) {
                                continue;
                            }
                        }
                    } else if (em_busy_ck(em) == 0) {
                        continue;
                    }
                    hit_apply_em(hit, em);
                }
            }
            if ((u32)(hit->kind_0x42 - 0xA) > 1) {
                _BODY_W* target;

                if (ainpc_w.active != 0 && ainpc_w.field_0x216 == 0
                    && hit_area_match_ck(hit->area_0x0F, ainpc_w.area) != 0
                    && hit_id_list_absent_ck(ainpc_w.hit_ids, hit->hit_id_0x0C) != 0 && ainpc_w.field_0x214 <= 0
                    && (((hit->owner_kind_0x06 != 3 || hit->owner_0x10 != &ainpc_w)
                            && (hit->kind_0x07 != 3 || hit->owner_0x14 != &ainpc_w))
                        || hit_flag_ck(hit, 0x40) != 0)
                    && hit_attack_flag_ck(hit, 4) != 0) {
                    hit_apply_npc(hit, &ainpc_w);
                }
                for (target = reg->body_head_0x04; target != NULL; target = target->next_0x00) {
                    if ((u32)(hit->kind_0x42 - 0xA) > 1 && target->field_0x005 != 0
                        && hit_area_match_ck(hit->area_0x0F, target->source) != 0
                        && hit_id_list_absent_ck(target->hit_ids_0x16, hit->hit_id_0x0C) != 0
                        && target->hit_timer_0x0A == 0 && body_hit_mask_ck(target, hit) != 0) {
                        hit_apply_body(hit, target);
                    }
                }
            }
        }
    }
}

/* 0x8029A788 (0x18): whether bit 15 of the hit's +0x48 word is set. */
extern "C" u32 hit_flag48_high_ck(_HIT_W* hit)
{
    return (hit->field_0x048 & 0x8000) != 0;
}

/* 0x8029A7A0 (0x88): an enemy's hit sound from the attacking player's SE bank, when the hit is in the current area and
 * its effect kind is not 0xFF or 1. */
extern "C" void hit_em_se_req(_PLW* attacker, _ENEMY_WORK* em, _HIT_W* hit, nw4r::math::VEC3* pos)
{
    s32 effect;

    if (hit->area_0x0F == get_now_areano() && (effect = hit->field_0x04D) != 0xFF && effect != 1) {
        se_hit_bank_req(em, attacker, hit, pos);
    }
}

/* 0x8029A828 (0x204): an enemy's hit sound by the hit's effect kind: the fixed effect sounds, the attacking player's
 * bank sound for the cut/blunt kinds 2-5 (with the matching result byte), else the plain hit sound. */
extern "C" void hit_em_effect_se_req(_PLW* attacker, _ENEMY_WORK* em, _HIT_W* hit, nw4r::math::VEC3* pos)
{
    u8 effect;

    if (hit->area_0x0F != get_now_areano()) {
        return;
    }
    effect = hit->field_0x04D;
    switch (effect) {
    case 12:
        se_req_pos_id62(pos);
        break;
    case 13:
        se_req_pos_id85(pos);
        break;
    case 7:
        se_req_pos_id86(pos);
        break;
    case 2:
        if (attacker != NULL) {
            hit->result = 3;
            se_hit_bank_req(em, attacker, hit, pos);
        } else {
            se_req_pos_id113_level(pos, 0);
        }
        break;
    case 3:
        if (attacker != NULL) {
            hit->result = 4;
            se_hit_bank_req(em, attacker, hit, pos);
        } else {
            se_req_pos_id113_level(pos, 0);
        }
        break;
    case 4:
        if (attacker != NULL) {
            hit->result = 2;
            se_hit_bank_req(em, attacker, hit, pos);
        } else {
            se_req_pos_id113_level(pos, 0);
        }
        break;
    case 5:
        if (attacker != NULL) {
            hit->result = 0;
            se_hit_bank_req(em, attacker, hit, pos);
        } else {
            se_req_pos_id113_level(pos, 0);
        }
        break;
    case 14:
        se_req_pos_id186(pos);
        break;
    case 0xFF:
        break;
    default:
        if ((u8)(effect + 0xF8) <= 1 && em->team == 40) {
            se_req_pos_id101(pos);
        } else {
            se_req_pos_id113_level(pos, 0);
        }
        break;
    }
}

/* 0x8029AA2C (0x84): the plain hit sound at the hit's strength level (0 up to 30, 1 up to 60, else 2). */
extern "C" void hit_level_se_req(_HIT_W* hit, nw4r::math::VEC3* pos)
{
    s32 effect;
    s32 level;

    if (hit->area_0x0F == get_now_areano() && (effect = hit->field_0x04D) != 0xFF) {
        if (hit->life_0x40 > 60) {
            level = 2;
        } else {
            level = hit->life_0x40 > 30;
        }
        se_req_pos_id113_level(pos, level);
    }
}

/* 0x8029AAB0 (0x208): a player's hit sound by the hit's effect kind: the guard sound for kind 1, the attacking player's
 * bank sound for kinds 2-5, else (unless `guarded`) the hard-hit sound or the hit sound at the damage's level. */
extern "C" void hit_pl_effect_se_req(_PLW* plw, _PLW* attacker, _HIT_W* hit, nw4r::math::VEC3* pos, s16 damage,
                                     s32 guarded)
{
    s32 effect;

    if (hit->area_0x0F == get_now_areano() && (effect = hit->field_0x04D) != 0xFF) {
        switch (effect) {
        case 1:
            se_req_pos_id98(pos);
        default:
            if (guarded == 1) {
                break;
            }
            if ((hit->mask_0x46 & 4) != 0 || (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0)) {
                se_req_pos_id99(pos);
                break;
            }
            se_req_pos_id110_level(pos, damage > 60 ? 2 : damage > 30);
            break;
        case 2:
            if (guarded == 1) {
                break;
            }
            if (attacker != NULL) {
                hit->result = 3;
                se_hit_bank_req(NULL, attacker, hit, pos);
            } else {
                se_req_pos_id113_level(pos, 0);
            }
            break;
        case 3:
            if (guarded == 1) {
                break;
            }
            if (attacker != NULL) {
                hit->result = 4;
                se_hit_bank_req(NULL, attacker, hit, pos);
            } else {
                se_req_pos_id113_level(pos, 0);
            }
            break;
        case 4:
            if (guarded == 1) {
                break;
            }
            if (attacker != NULL) {
                hit->result = 2;
                se_hit_bank_req(NULL, attacker, hit, pos);
            } else {
                se_req_pos_id113_level(pos, 0);
            }
            break;
        case 5:
            if (guarded == 1) {
                break;
            }
            if (attacker != NULL) {
                hit->result = 0;
                se_hit_bank_req(NULL, attacker, hit, pos);
            } else {
                se_req_pos_id113_level(pos, 0);
            }
            break;
        }
    }
}

/* 0x8029ACB8 (0xAC): the knock direction's y angle: from `from` towards `to` for a positional hit (flag 0x10), else
 * the owner's facing; plus the hit's offset in 256ths of a turn. */
extern "C" u16 hit_dir_y_get(_HIT_W* hit, nw4r::math::VEC3* from, nw4r::math::VEC3* to)
{
    u32 angle;

    if ((hit->flags_0x43 & 0x10) != 0) {
        angle = calcVecAng2(from, to);
    } else {
        switch (hit->owner_kind_0x06) {
        case 0:
            angle = ((_PLW*)hit->owner_0x10)->rot_0x54.y;
            break;
        case 1:
            angle = ((_ENEMY_WORK*)hit->owner_0x10)->field_0x1C0;
            break;
        case 2:
            angle = ((_SHELL_W*)hit->owner_0x10)->rot_y_0x28;
            break;
        case 3:
            angle = ((_AINPC_W*)hit->owner_0x10)->vec_0x190.y;
            break;
        }
    }
    return angle + (hit->dir_ofs_0x44 << 8);
}

/* 0x8029AD64 (0x94): the knock direction's x angle: from `from` towards `to` for a positional hit, else the owner's
 * pitch. */
extern "C" u16 hit_dir_x_get(_HIT_W* hit, nw4r::math::VEC3* from, nw4r::math::VEC3* to)
{
    u32 angle;

    if ((hit->flags_0x43 & 0x10) != 0) {
        angle = calcVecAngX2(from, to);
    } else {
        switch (hit->owner_kind_0x06) {
        case 0:
            angle = ((_PLW*)hit->owner_0x10)->rot_0x54.x;
            break;
        case 1:
            angle = ((_ENEMY_WORK*)hit->owner_0x10)->field_0x1BC;
            break;
        case 2:
            angle = ((_SHELL_W*)hit->owner_0x10)->rot_x_0x24;
            break;
        case 3:
            angle = ((_AINPC_W*)hit->owner_0x10)->vec_0x190.x;
            break;
        }
    }
    return angle;
}

/* 0x8029ADF8 (0xC4): stores the hit's reaction into the player that owns its second slot (flag 8): by the result, a
 * sideways (3) or doubled backwards (4) value, else nothing. */
extern "C" void hit_owner_reaction_set(_HIT_W* hit)
{
    _PLW* plw;

    if (hit->kind_0x07 == 0 && hit_flag_ck(hit, 8) == 1 && (plw = (_PLW*)hit->owner_0x14) != NULL) {
        u8 value;

        plw->field_0x3A2 = 0;
        plw->field_0x3A3 = 0;
        value = hit->reaction_0x58;
        switch (hit->result) {
        case 0:
        case 2:
        case 5:
            plw->field_0x3A2 = 0;
            plw->field_0x3A3 = 0;
            break;
        case 3:
            plw->field_0x3A2 = 0;
            plw->field_0x3A3 = value;
            break;
        case 4:
            plw->field_0x3A3 = 0;
            plw->field_0x3A2 = value * 2;
            break;
        }
    }
}

/* 0x8029AEBC (0x9C): the owner's hit-strength rate: a player's or the NPC's own while flag 0x20 is set, an enemy's
 * shell rate, else 1. */
extern "C" f32 hit_owner_rate_get(_HIT_W* hit)
{
    f32 rate = 1.0f;

    switch (hit->owner_kind_0x06) {
    case 0:
        if (hit_flag_ck(hit, 0x20) == 1) {
            rate = ((_PLW*)hit->owner_0x10)->skill_rate_0x3BC;
        } else {
            rate = 1.0f;
        }
        break;
    case 1:
        rate = ((_ENEMY_WORK*)hit->owner_0x10)->shell_rate_0x7B4;
        break;
    case 3:
        if (hit_flag_ck(hit, 0x20) == 1) {
            rate = ((_AINPC_W*)hit->owner_0x10)->field_0x204;
        } else {
            rate = 1.0f;
        }
        break;
    }
    return rate;
}

/* 0x8029AF58 (0xA0): the player's resistance to element `elem` (0..4), as a whole number. */
extern "C" s16 pl_elem_resist_get(_PLW* plw, u16 elem)
{
    s16 resist;

    switch (elem) {
    case 0:
        resist = plw->elem_resist_0x3C4;
        break;
    case 1:
        resist = plw->elem_resist_0x3C8;
        break;
    case 2:
        resist = plw->elem_resist_0x3CC;
        break;
    case 4:
        resist = plw->elem_resist_0x3D0;
        break;
    case 3:
        resist = plw->elem_resist_0x3D4;
        break;
    default:
        resist = 0;
        break;
    }
    return resist;
}

/* 0x8029AFF8 (0x7D0): applies a hit to a player: marks the hit's target and the knock directions, guards it (a spark)
 * or spawns the hit effect, records the hit id, applies a master player's attack/defence-up support hits, works out
 * the damage (owner rate, defence, element resistance; a heal for kind 31), the element and status build-ups, and
 * plays the hit sound. */
extern "C" void hit_damage_pl(_PLW* plw, _HIT_W* hit, nw4r::math::VEC3* contact, nw4r::math::VEC3* hit_pos,
                              nw4r::math::VEC3* pl_pos)
{
    u32 rot[3];
    f32 rate;
    f32 scale;
    f32 damage;
    f32 status;
    u16 guard_angle;
    u16 elem;
    s16 resist;
    s16 amount;

    hit->result = 2;
    hit->target_0x2C = plw;
    hit->target_kind_0x30 = 0;
    hit->target_part_0x28 = NULL;
    plw->hit_pending_0x39E = 1;
    plw->field_0x398 = hit_dir_y_get(hit, hit_pos, pl_pos) + 0x8000;
    plw->field_0x39C = hit_dir_x_get(hit, hit_pos, pl_pos);
    plw->knock_dir_0x39A = hit_dir_y_get(hit, hit_pos, pl_pos) + 0x8000;
    plw->field_0x3A0 = hit->flags_0x43;
    plw->field_0x39F = hit->kind_0x42;
    if (Pl_master_ck(plw) == 1 && (u8)(plw->field_0x39F + 0xE7) <= 1 && hit->owner_kind_0x06 == 1) {
        plw->sub_area_0x3B6 = ((_ENEMY_WORK*)hit->owner_0x10)->field_0x01A;
    }
    plw->field_0x3A1 = hit->field_0x045;
    guard_angle = plw->field_0x0A8 - plw->field_0x398 + 0x4000;
    if ((u32)pl_guard_ck(plw, guard_angle) == 1) {
        if (hit->field_0x04C != 0) {
            rot[0] = 0;
            rot[1] = hit_dir_y_get(hit, hit_pos, pl_pos);
            rot[2] = 0;
            eft013_guard_spawn(contact, rot, 1.0f, hit);
        }
    } else if (plw->kind_0x09 == 3) {
        hit_effect_spawn(hit, contact, hit_pos, pl_pos, plw->field_0x064, NULL);
    } else {
        hit_effect_spawn(hit, contact, hit_pos, pl_pos, -10000.0f, NULL);
    }
    hit_id_list_push(plw->hit_ids_0x53C, &plw->hit_id_count_0x564, hit->hit_id_0x0C);
    rate = hit_owner_rate_get(hit);
    scale = 1.0f;
    if (Pl_master_ck(plw) == 1) {
        switch (hit->kind_0x42) {
        case 27:
            pl_attack_up_set(plw, 1, 10);
            plw->field_0x44A = 5400;
            pl_attack_up_mark(plw, 10, 5400);
            break;
        case 28:
            pl_attack_up_set(plw, 1, 5);
            plw->field_0x44A = 9000;
            pl_attack_up_mark(plw, 20, 9000);
            break;
        case 29:
            if ((u32)pl_defense_up_set(plw, 1, 40) == 1) {
                plw->field_0x44E = 5400;
            }
            pl_defense_up_mark(plw, 1, 5400);
            break;
        case 30:
            if ((u32)pl_defense_up_set(plw, 1, 20) == 1) {
                plw->field_0x44E = 9000;
            }
            pl_defense_up_mark(plw, 2, 9000);
            break;
        }
    }
    damage = scale * (hit->life_0x40 * rate);
    damage = damage - (damage * plw->defense_0x3C0) / (plw->defense_0x3C0 + 80.0f);
    scale = 1.0f;
    elem = 0xFFFF;
    if ((hit->mask_0x46 & 4) != 0) {
        elem = 0;
    } else if ((hit->mask_0x46 & 0x10) != 0) {
        elem = 1;
    } else if ((hit->mask_0x46 & 0x40) != 0) {
        elem = 2;
    } else if ((hit->mask_0x46 & 0x20) != 0) {
        elem = 4;
    } else if ((hit->mask_0x46 & 0x80) != 0) {
        elem = 3;
    }
    resist = pl_elem_resist_get(plw, elem);
    if (resist != 0) {
        scale = (100.0f - resist) / 100.0f;
    }
    if (hit->kind_0x42 == 31) {
        amount = -hit->life_0x40;
        if (Pl_cat_skill_ck(plw, 3) == 1) {
            amount += (s16)(amount / 10);
        }
        if (Pl_Skill_ck(plw, 152) == 1) {
            amount += (s16)(amount / 4);
        } else if (Pl_Skill_ck(plw, 153) == 1) {
            amount -= (s16)(amount / 4);
        }
    } else {
        amount = damage * scale;
    }
    if (hit->attack_kind_0x32 != 1 || hit_flag_ck(hit, 0x80) != 0 || hit->kind_0x42 == 31) {
        if (elem != 0xFFFF) {
            s16 level;

            level = (hit->field_0x05A != 0 ? hit->field_0x05A : 10) - resist;
            if ((plw->elem_hit_0x38C[elem] < 0 ? -plw->elem_hit_0x38C[elem] : plw->elem_hit_0x38C[elem])
                < (level < 0 ? -level : level)) {
                plw->elem_hit_0x38C[elem] = level;
            }
        }
        status = hit->field_0x04E;
        if ((hit->field_0x048 & 1) != 0) {
            plw->field_0x3F2 = hit->field_0x04A;
        }
        if ((hit->field_0x048 & 2) != 0) {
            plw->field_0x3EC = hit->field_0x04A;
        }
        if ((hit->field_0x048 & 4) != 0) {
            plw->field_0x3F8 = hit->field_0x04A;
        }
        if ((hit->field_0x048 & 0x400) != 0) {
            plw->field_0x40E = hit->field_0x04A;
        }
        if ((hit->field_0x048 & 0x1000) != 0) {
            plw->field_0x420 = hit->field_0x04A;
        }
        if (hit_flag48_high_ck(hit) == 1 && (hit->field_0x048 & 0x10) != 0) {
            plw->status_pending_0x424[0] = hit->field_0x04A;
        }
        if (hit_flag48_high_ck(hit) == 1 && (hit->field_0x048 & 0x20) != 0) {
            plw->status_pending_0x424[1] = hit->field_0x04A;
        }
        if (hit_flag48_high_ck(hit) == 1 && (hit->field_0x048 & 0x40) != 0) {
            plw->status_pending_0x424[2] = hit->field_0x04A;
        }
        if (hit_flag48_high_ck(hit) == 1 && (hit->field_0x048 & 0x80) != 0) {
            plw->status_pending_0x424[4] = hit->field_0x04A;
        }
        if (hit_flag48_high_ck(hit) == 1 && (hit->field_0x048 & 0x200) != 0) {
            plw->status_pending_0x424[3] = hit->field_0x04A;
        }
        do {
            switch (hit->kind_0x42) {
            case 4:
                if (Pl_Skill_ck(plw, 88) == 1) {
                    continue;
                }
                if (Pl_Skill_ck(plw, 89) != 1) {
                    break;
                }
                continue;
            case 36:
            case 5:
                if (Pl_Skill_ck(plw, 89) == 1) {
                    continue;
                }
                break;
            }
            plw->field_0x3FE += (s16)status;
        } while (0);
        plw->field_0x38A += amount;
    }
    if ((u32)pl_guard_ck(plw, guard_angle) == 1) {
        hit->result = 5;
        hit_owner_reaction_set(hit);
        hit_pl_effect_se_req(plw, NULL, hit, pl_pos, amount, 1);
        if (hit->owner_kind_0x06 != 1) {
            hit->result = 2;
        }
        hit_owner_reaction_set(hit);
        return;
    }
    hit_owner_reaction_set(hit);
    if (hit->owner_kind_0x06 == 0 && hit->kind_0x07 == 0) {
        hit_pl_effect_se_req(plw, (_PLW*)hit->owner_0x14, hit, pl_pos, amount, 0);
    } else {
        hit_pl_effect_se_req(plw, NULL, hit, pl_pos, amount, 0);
    }
}

/* 0x8029B7C8 (0x12C): the larger of the two weighted part rates (`rate_a` by the part's first percentage, `rate_b` by
 * its second, both lowered by 15 while the enemy's 0x94A timer runs); `side` receives which one won. */
extern "C" f32 hit_part_rate_get(_ENEMY_WORK* em, EmPartRates* rates, u8* side, f32 rate_a, f32 rate_b)
{
    f32 percent_a;
    f32 percent_b;
    f32 result_a;
    f32 result_b;

    percent_a = rates->cut;
    percent_b = rates->blunt;
    if (em_timer_0x94A_ck(em) == 1) {
        percent_a -= 15.0f;
        if (percent_a < 0.0f) {
            percent_a = 0.0f;
        }
        percent_b -= 15.0f;
        if (percent_b < 0.0f) {
            percent_b = 0.0f;
        }
    }
    result_a = (rate_a / 100.0f) * (percent_a / 100.0f);
    result_b = (rate_b / 100.0f) * (percent_b / 100.0f);
    if (result_a >= result_b) {
        *side = 0;
        return result_a;
    }
    *side = 1;
    return result_b;
}

/* 0x8029B8F4 (0x24): the priority of a damage reaction id, or 0 for an id past the table. */
extern "C" u32 hit_reaction_priority_get(u32 id)
{
    if ((u8)id > 0x24) {
        return 0;
    }
    return hit_reaction_priority_tbl[(u8)id];
}

/* 0x8029B918 (0xF94): applies a hit to an enemy part: records the hit, works out the cut, element and status rates (a
 * player's weapon against the part's percentages, a shell's or a plain hit's), wears the weapon, plays the hit sound
 * and effect, and adds the damage, the hate, the reaction, the element and status build-ups and the stun to the
 * enemy's damage record. */
extern "C" void hit_damage_em(_ENEMY_WORK* em, _HIT_W* hit, EmWeakPoint* part, _HIT_SIZE_DATA* shape,
                              nw4r::math::VEC3* contact, nw4r::math::VEC3* hit_pos, nw4r::math::VEC3* em_pos)
{
    u8 side;
    f32 rate;
    f32 cut;
    f32 elem_rate;
    f32 status_rate;
    f32 elem;
    f32 elem_a;
    f32 elem_b;
    f32 elem_c;
    f32 elem_d;
    f32 elem_e;
    EmPartRates* rates;
    _PLW* plw;
    u16 mask;
    s32 cut_ok;
    u8 heavy;
    s16 damage;
    u8 status;
    s16 wear;
    s16 half;
    u8 halved;
    s32 side_kind;
    s16 build;

    hit_id_list_push(em->hit_ids_0xAC0, &em->hit_id_count_0xAE8, hit->hit_id_0x0C);
    em->field_0x833 = 1;
    hit->target_0x2C = em;
    hit->target_kind_0x30 = 1;
    hit->target_part_0x28 = part;
    rate = hit_owner_rate_get(hit);
    cut = 1.0f;
    elem_rate = cut;
    status_rate = cut;
    rates = (EmPartRates*)em_attacker_rec_get(em, (u8)part->rates_index);
    mask = hit->mask_0x46;
    cut_ok = 0;
    heavy = hit_flag_ck(hit, 0x400) == 1;
    if (em->team == 25 && part->rates_index <= 4) {
        hit->field_0x05B |= 2;
    }
    if (hit_flag_ck(hit, 0x20) == 1 && hit->attack_kind_0x32 == 1) {
        switch (hit->owner_kind_0x06) {
        case 0:
            plw = (_PLW*)hit->owner_0x10;
            break;
        case 2:
            if (hit->kind_0x07 == 0) {
                plw = (_PLW*)hit->owner_0x14;
            } else {
                plw = NULL;
            }
            break;
        default:
            plw = NULL;
            break;
        }
        if (plw == NULL) {
            return;
        }
        mask &= 0xFFF7;
        if ((hit->mask_0x46 & 1) != 0) {
            cut_ok = 1;
        }
        switch ((u8)hit_side_kind_get(hit)) {
        case 0:
            side_kind = 2;
            break;
        case 1:
            side_kind = 0;
            break;
        case 2:
            if (shape->stab_0x06 == 1) {
                side_kind = 4;
            } else {
                side_kind = 3;
            }
            break;
        case 3:
            side_kind = 2;
            break;
        }
        cut = hit_sharpness_rate_tbl[plw->field_0x56B] * hit_sharpness_side_rate_tbl[side_kind + plw->field_0x56B * 6];
        cut *= hit_part_rate_get(em, rates, &side, plw->cut_rate_0x568, plw->blunt_rate_0x569);
        elem_rate = hit_sharpness_elem_rate_tbl[plw->field_0x56B];
        if (hit_flag_ck(hit, 0x1000) == 1) {
            elem_rate *= 0.25f;
        }
        status_rate = hit_sharpness_status_rate_tbl[plw->field_0x56B];
        if (side == 0) {
            cut_ok = 0;
        }
        if (cut >= 1.0f) {
            hit->result = 4;
        } else if (cut >= 0.45f) {
            hit->result = 3;
        } else if (cut >= 0.25f) {
            hit->result = 2;
        } else {
            hit->result = 0;
        }
        if ((s8)hit->field_0x059 > 0 || (hit->flags_0x43 & 0x80) != 0) {
            pl_act_add_class7_gauge(plw, (s16)((s8)hit->field_0x059 > 0
                                                   ? (s8)hit->field_0x059 * hit_result_gauge_rate_tbl[hit->result]
                                                   : hit->life_0x40 * hit_result_gauge_rate_tbl[hit->result]));
        }
        if (hit_flag_ck(hit, 0x2000) == 1) {
            hit_flag_clear(hit, 0x2000);
            pl_class7_meter_add(plw, 40);
        }
        half = hit_flag_ck(hit, 0x1000) == 1;
        halved = 0;
        wear = hit_result_sharpness_tbl[half + hit->result * 2];
        if (hit->result == 0 && Pl_cat_skill_ck(plw, 19) == 1 && half == 0) {
            wear = wear / 2;
            if (wear == 0) {
                wear = -1;
            }
            halved = 1;
        }
        pl_sharpness_add(plw, wear, halved);
        hit_em_se_req(plw, em, hit, contact);
    } else {
        if (hit_flag_ck(hit, 0x20) == 1 && hit->attack_kind_0x32 == 4) {
            cut *= hit_part_rate_get(em, rates, &side, 100.0f, 100.0f);
            if (side == 0) {
                cut_ok = 0;
                mask = (mask & 0xFFFE) | 2;
            } else {
                cut_ok = 1;
                mask = (mask & 0xFFFD) | 1;
            }
            if (cut >= 1.0f) {
                hit->result = 4;
            } else if (cut >= 0.45f) {
                hit->result = 3;
            } else if (cut >= 0.25f) {
                hit->result = 2;
            } else {
                hit->result = 0;
            }
        } else {
            if (hit_flag_ck(hit, 1) == 1) {
                f32 shell = rates->shell;

                if (em_timer_0x94A_ck(em) == 1) {
                    shell -= 15.0f;
                    if (shell < 0.0f) {
                        shell = 0.0f;
                    }
                }
                cut = shell / 100.0f;
            } else {
                cut = 1.0f;
            }
            hit->result = 2;
            mask &= 0xFFFE;
        }
        if ((s32)hit->owner_kind_0x06 == 1) {
            if (em_area_ck(em) == 1) {
                hit_level_se_req(hit, contact);
            }
        } else if (hit->kind_0x07 == 0) {
            hit_em_effect_se_req((_PLW*)hit->owner_0x14, em, hit, contact);
        } else {
            hit_em_effect_se_req(NULL, em, hit, contact);
        }
    }
    if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0) {
        elem_a = (f32)hit->field_0x04A * (f32)rates->elem[0] / 100.0f;
    } else {
        elem_a = 0.0f;
    }
    if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x20) != 0) {
        elem_b = (f32)hit->field_0x04A * (f32)rates->elem[1] / 100.0f;
    } else {
        elem_b = 0.0f;
    }
    if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x200) != 0) {
        elem_c = (f32)hit->field_0x04A * (f32)rates->elem[2] / 100.0f;
    } else {
        elem_c = 0.0f;
    }
    if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x40) != 0) {
        elem_d = (f32)hit->field_0x04A * (f32)rates->elem[3] / 100.0f;
    } else {
        elem_d = 0.0f;
    }
    if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x80) != 0) {
        elem_e = (f32)hit->field_0x04A * (f32)rates->elem[4] / 100.0f;
    } else {
        elem_e = 0.0f;
    }
    elem = elem_rate * (elem_a + elem_b + elem_c + elem_d + elem_e);
    hit_owner_reaction_set(hit);
    if (em_water_check_c1(em) == 1) {
        hit_effect_spawn(hit, contact, hit_pos, em_pos, em->field_0x210, rates);
    } else {
        hit_effect_spawn(hit, contact, hit_pos, em_pos, -10000.0f, rates);
    }
    if ((hit->mask_0x46 & 0x100) != 0) {
        em_net_flag_apply_0(em);
    }
    if (hit->kind_0x42 == 31) {
        rate = -1.0f * hit->life_0x40;
        damage = rate;
    } else {
        rate = rate * (hit->life_0x40 * cut);
        if (rate + elem > 0.0f && rate + elem < 1.0f) {
            damage = 1;
        } else {
            damage = rate + elem;
        }
    }
    status = (s8)hit->field_0x04E * status_rate;
    switch (hit->attack_kind_0x32) {
    case 1:
        if (hit->owner_kind_0x06 == 0) {
            plw = (_PLW*)hit->owner_0x10;
        } else if (hit->kind_0x07 == 0) {
            plw = (_PLW*)hit->owner_0x14;
        } else {
            break;
        }
        if (plw != NULL) {
            pl_act_hold_notify(plw, 0);
            em_damage_hate_add(em, 0, plw->chunk_ofs, damage, heavy);
        }
        break;
    case 2: {
        _ENEMY_WORK* owner;

        if (damage > 0) {
            damage = damage / 4;
            if (damage < 1) {
                damage = 1;
            }
        } else {
            damage = damage / 4;
        }
        rate *= 0.25f;
        if (status != 0) {
            status = (u32)status >> 2;
            if (status == 0) {
                status = 1;
            }
        }
        if (hit->owner_kind_0x06 == 1) {
            owner = (_ENEMY_WORK*)hit->owner_0x10;
        } else if (hit->kind_0x07 == 1) {
            owner = (_ENEMY_WORK*)hit->owner_0x14;
        } else {
            break;
        }
        if (owner != NULL) {
            em_damage_hate_add(em, 1, owner->group, damage, heavy);
        }
        break;
    }
    case 4:
        em_damage_hate_add(em, 2, 0, damage, heavy);
        break;
    }
    em_damage_hp_add(em, part->kind, damage, heavy);
    if (hit->kind_0x42 == 17) {
        em_damage_flags_set(em, 1);
    }
    if ((u8)hit_reaction_priority_get(hit->kind_0x42) >= (u8)hit_reaction_priority_get((u8)em->reaction_0x828)) {
        em->reaction_0x828 = hit->kind_0x42;
        em->reaction_b_0x82A = hit->kind_0x42;
        em->reaction_level_0x830 = hit->field_0x045;
        em->field_0x816 = hit_dir_y_get(hit, hit_pos, em_pos) + 0x8000;
    }
    if ((mask & 2) != 0) {
        em_damage_attr_add(em, part->kind, 1, rate, 0, heavy);
    }
    if ((mask & 1) != 0 && cut_ok != 0) {
        em_damage_attr_add(em, part->kind, 0, rate, 0, heavy);
    }
    if ((mask & 4) != 0 || (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0)) {
        em_damage_attr_add(em, part->kind, 3, rate, elem, heavy);
    }
    if ((mask & 0x10) != 0 || (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x20) != 0)) {
        em_damage_attr_add(em, part->kind, 4, rate, elem, heavy);
    }
    if ((mask & 0x40) != 0 || (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x40) != 0)) {
        em_damage_attr_add(em, part->kind, 5, rate, elem, heavy);
    }
    if ((mask & 0x20) != 0 || (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x80) != 0)) {
        em_damage_attr_add(em, part->kind, 6, rate, elem, heavy);
    }
    if ((mask & 0x80) != 0 || (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x200) != 0)) {
        em_damage_attr_add(em, part->kind, 7, rate, elem, heavy);
    }
    build = hit->field_0x04A;
    if (build > 0) {
        if (hit_flag_ck(hit, 0x1000) == 1) {
            build = build >> 1;
            if (build == 0) {
                build = 1;
            }
        }
    } else {
        build = 0;
    }
    if ((hit->field_0x048 & 1) != 0) {
        em_damage_attr_add(em, part->kind, 9, 0, build, heavy);
    }
    if ((hit->field_0x048 & 2) != 0) {
        em_damage_attr_add(em, part->kind, 10, 0, build, heavy);
    }
    if ((hit->field_0x048 & 4) != 0) {
        em_damage_attr_add(em, part->kind, 11, 0, build, heavy);
    }
    if ((hit->field_0x048 & 8) != 0) {
        em_damage_attr_add(em, part->kind, 12, 0, build, heavy);
    }
    if ((hit->field_0x048 & 0x400) != 0) {
        em_damage_attr_add(em, part->kind, 13, 0, build, heavy);
    }
    if ((hit->field_0x048 & 0x800) != 0) {
        em_damage_attr_add(em, part->kind, 14, 0, build, heavy);
    }
    if (hit->kind_0x42 == 22) {
        em_damage_attr_add(em, part->kind, 8, 0, hit->life_0x40, heavy);
        if (hit->owner_kind_0x06 == 0) {
            em_damage_part_hit_set(em, ((_PLW*)hit->owner_0x10)->chunk_ofs);
        }
    }
    em_damage_stun_add(em, part->kind, part->rates_index, status);
}

/* 0x8029C8AC (0x2B4): applies a hit to the AI NPC: marks the hit's target, rolls the counter-attack (a guard spark
 * and sound) or spawns the hit effect, records the hit id, applies the area buffs of the support kinds, and adds the
 * damage reduced by the NPC's defence (none for a blocked hit), the knock direction and the status values. */
extern "C" void hit_damage_npc(_AINPC_W* npc, _HIT_W* hit, nw4r::math::VEC3* contact, nw4r::math::VEC3* hit_pos,
                               nw4r::math::VEC3* npc_pos)
{
    u32 rot[3];
    s32 blocked;
    f32 damage;
    s16 amount;

    blocked = 0;
    hit->result = 2;
    hit->target_0x2C = npc;
    hit->target_kind_0x30 = 3;
    hit->target_part_0x28 = NULL;
    hit_owner_reaction_set(hit);
    npc->field_0x21A = hit->kind_0x42;
    npc->hit_level_0x21B = hit->field_0x045;
    if ((u32)ai_npc_counter_roll(npc) == 1) {
        if (hit->field_0x04C != 0) {
            rot[0] = 0;
            rot[1] = hit_dir_y_get(hit, hit_pos, npc_pos);
            rot[2] = 0;
            eft013_guard_spawn(contact, rot, 0.8f, hit);
            se_req_pos_id87(contact);
        }
    } else if (npc->variant == 2) {
        hit_effect_spawn(hit, contact, hit_pos, npc_pos, npc->field_0x1A0, NULL);
    } else {
        hit_effect_spawn(hit, contact, hit_pos, npc_pos, -10000.0f, NULL);
    }
    hit_id_list_push(npc->hit_ids, &npc->hit_id_count, hit->hit_id_0x0C);
    switch (hit->kind_0x42) {
    case 31:
        ainpc_area_buff_apply(5);
        break;
    case 27:
    case 28:
        ainpc_area_buff_apply(1);
        break;
    case 29:
    case 30:
        ainpc_area_buff_apply(2);
        ainpc_area_buff_apply(8);
        break;
    }
    npc->field_0x216 = 1;
    damage = hit->life_0x40;
    damage = damage - (damage * npc->field_0x208) / (npc->field_0x208 + 80.0f);
    if (hit->attack_kind_0x32 == 1) {
        damage = 0.0f;
        blocked = 1;
    }
    amount = damage;
    if (amount > 0.0f && amount < 1.0f) {
        amount = 1;
    }
    npc->damage_0x212 += amount;
    npc->field_0x218 = hit_dir_y_get(hit, hit_pos, npc_pos) + 0x8000;
    if (blocked == 0) {
        if ((hit->field_0x048 & 1) != 0) {
            npc->field_0x224 = hit->field_0x04A;
        }
        if ((hit->field_0x048 & 2) != 0) {
            npc->field_0x21E = hit->field_0x04A;
        }
        if ((hit->field_0x048 & 4) != 0) {
            npc->field_0x22A = hit->field_0x04A;
        }
        npc->field_0x230 += hit->field_0x04E;
    }
}

/* 0x8029CB60 (0x21C): tests every hit-size record of the hit against the player's body capsule and applies the
 * first contact to the player. */
extern "C" void hit_apply_pl(_HIT_W* hit, _PLW* plw)
{
    PlCapsule hit_capsule;
    PlCapsule pl_capsule;
    HitSphere hit_sphere;
    PlCapsuleBox hit_box;
    PlCapsuleBox pl_box;
    nw4r::math::VEC3 contact;
    nw4r::math::VEC3 hit_pos;
    nw4r::math::VEC3 pl_pos;
    _HIT_SIZE_DATA* shape;
    s32 hit_kind;
    s32 struck;

    eft_capsule_ctor((nw4r::math::CAPSULE*)&hit_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&pl_capsule);
    eft_vec_ctor(&hit_sphere.pos);
    eft_shape_init((struct EftShape*)&hit_box);
    eft_shape_init((struct EftShape*)&pl_box);
    VEC3_ctor(&contact);
    VEC3_ctor(&hit_pos);
    VEC3_ctor(&pl_pos);
    pl_hit_capsule_get(plw, &pl_capsule);
    pl_capsule_box_set(&pl_capsule, &pl_box);
    shape = hit->size_data_0x08;
    struck = 0;
    for (; shape->joint != -1; shape++) {
        hit_kind = hit_shape_get(hit, shape, &hit_capsule, &hit_sphere);
        if (hit_kind == 2) {
            continue;
        }
        if (hit_kind == 1) {
            pl_capsule_box_set(&hit_capsule, &hit_box);
        }
        if (hit_kind == 0) {
            if ((u32)pl_sphere_capsule_contact(&hit_sphere, &pl_box, &contact) == 1) {
                struck = 1;
                copyVec3(&hit_pos, &hit_sphere.pos);
                pl_pos.x = (pl_capsule.start.x + pl_capsule.end.x) / 2.0f;
                pl_pos.y = (pl_capsule.start.y + pl_capsule.end.y) / 2.0f;
                pl_pos.z = (pl_capsule.start.z + pl_capsule.end.z) / 2.0f;
            }
        } else if ((u32)pl_capsule_capsule_contact(&hit_box, &pl_box, &contact) == 1) {
            struck = 1;
            hit_pos.x = (hit_capsule.start.x + hit_capsule.end.x) / 2.0f;
            hit_pos.y = (hit_capsule.start.y + hit_capsule.end.y) / 2.0f;
            hit_pos.z = (hit_capsule.start.z + hit_capsule.end.z) / 2.0f;
            pl_pos.x = (pl_capsule.start.x + pl_capsule.end.x) / 2.0f;
            pl_pos.y = (pl_capsule.start.y + pl_capsule.end.y) / 2.0f;
            pl_pos.z = (pl_capsule.start.z + pl_capsule.end.z) / 2.0f;
        }
        if (struck != 0) {
            hit_damage_pl(plw, hit, &contact, &hit_pos, &pl_pos);
            return;
        }
    }
}

/* 0x8029CD7C (0x394): tests every hit-size record of the hit against every weak point of the enemy the attack kind
 * can strike, and applies the first contact to the enemy. */
extern "C" void hit_apply_em(_HIT_W* hit, _ENEMY_WORK* em)
{
    PlCapsule hit_capsule;
    PlCapsule em_capsule;
    HitSphere hit_sphere;
    HitSphere em_sphere;
    nw4r::math::VEC3 contact;
    nw4r::math::VEC3 hit_pos;
    nw4r::math::VEC3 em_pos;
    PlCapsuleBox hit_box;
    PlCapsuleBox em_box;
    _HIT_SIZE_DATA* shape;
    EmWeakPoint* part;
    s32 hit_kind;
    s32 em_kind;
    s32 struck;

    eft_capsule_ctor((nw4r::math::CAPSULE*)&hit_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&em_capsule);
    eft_vec_ctor(&hit_sphere.pos);
    eft_vec_ctor(&em_sphere.pos);
    VEC3_ctor(&contact);
    VEC3_ctor(&hit_pos);
    VEC3_ctor(&em_pos);
    eft_shape_init((struct EftShape*)&hit_box);
    eft_shape_init((struct EftShape*)&em_box);
    shape = hit->size_data_0x08;
    struck = 0;
    for (; shape->joint != -1; shape++) {
        part = em->parts_0x8C8;
        hit_kind = hit_shape_get(hit, shape, &hit_capsule, &hit_sphere);
        if (hit_kind == 2) {
            continue;
        }
        if (hit_kind == 1) {
            pl_capsule_box_set(&hit_capsule, &hit_box);
        }
        for (; part->joint != -1; part++) {
            if (em_flags836_ck(em, part->part) == 1) {
                continue;
            }
            switch (hit->kind_0x42) {
            case 10:
                if ((part->flags & 3) == 0) {
                    continue;
                }
                break;
            case 11:
                if ((part->flags & 0x18) == 0) {
                    continue;
                }
                break;
            case 4:
            case 5:
            case 36:
                if ((part->flags & 4) == 0) {
                    continue;
                }
                break;
            default:
                if ((part->flags & 0x152) != 0) {
                    continue;
                }
                break;
            }
            em_kind = em_part_sphere_get(em, part, &em_capsule, &em_sphere);
            if (hit_kind == 0) {
                if (em_kind == 0) {
                    if ((u32)pl_sphere_contact(&hit_sphere, &em_sphere, &contact) == 1) {
                        struck = 1;
                        copyVec3(&hit_pos, &hit_sphere.pos);
                        copyVec3(&em_pos, &em_sphere.pos);
                    }
                } else {
                    pl_capsule_box_set(&em_capsule, &em_box);
                    if ((u32)pl_sphere_capsule_contact(&hit_sphere, &em_box, &contact) == 1) {
                        struck = 1;
                        copyVec3(&hit_pos, &hit_sphere.pos);
                        em_pos.x = (em_capsule.start.x + em_capsule.end.x) / 2.0f;
                        em_pos.y = (em_capsule.start.y + em_capsule.end.y) / 2.0f;
                        em_pos.z = (em_capsule.start.z + em_capsule.end.z) / 2.0f;
                    }
                }
            } else if (em_kind == 0) {
                if ((u32)pl_capsule_sphere_contact(&hit_box, &em_sphere, &contact) == 1) {
                    struck = 1;
                    hit_pos.x = (hit_capsule.start.x + hit_capsule.end.x) / 2.0f;
                    hit_pos.y = (hit_capsule.start.y + hit_capsule.end.y) / 2.0f;
                    hit_pos.z = (hit_capsule.start.z + hit_capsule.end.z) / 2.0f;
                    copyVec3(&em_pos, &em_sphere.pos);
                }
            } else {
                pl_capsule_box_set(&em_capsule, &em_box);
                if ((u32)pl_capsule_capsule_contact(&hit_box, &em_box, &contact) == 1) {
                    struck = 1;
                    hit_pos.x = (hit_capsule.start.x + hit_capsule.end.x) / 2.0f;
                    hit_pos.y = (hit_capsule.start.y + hit_capsule.end.y) / 2.0f;
                    hit_pos.z = (hit_capsule.start.z + hit_capsule.end.z) / 2.0f;
                    em_pos.x = (em_capsule.start.x + em_capsule.end.x) / 2.0f;
                    em_pos.y = (em_capsule.start.y + em_capsule.end.y) / 2.0f;
                    em_pos.z = (em_capsule.start.z + em_capsule.end.z) / 2.0f;
                }
            }
            if (struck != 0) {
                hit_damage_em(em, hit, part, shape, &contact, &hit_pos, &em_pos);
                return;
            }
        }
    }
}

/* 0x8029D110 (0x348): tests every hit-size record of the hit against every shape of the body slot; on the first
 * contact records the hit id in the slot and, unless the slot is a stage object the hit is too weak for, adds the
 * hit's damage to it. */
extern "C" void hit_apply_body(_HIT_W* hit, _BODY_W* body)
{
    PlCapsule hit_capsule;
    PlCapsule body_capsule;
    HitSphere hit_sphere;
    HitSphere body_sphere;
    nw4r::math::VEC3 contact;
    nw4r::math::VEC3 hit_pos;
    nw4r::math::VEC3 body_pos;
    PlCapsuleBox hit_box;
    PlCapsuleBox body_box;
    _HIT_SIZE_DATA* shape;
    EmWeakPoint* part;
    s32 hit_kind;
    s32 body_kind;
    s32 struck;

    eft_capsule_ctor((nw4r::math::CAPSULE*)&hit_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&body_capsule);
    eft_vec_ctor(&hit_sphere.pos);
    eft_vec_ctor(&body_sphere.pos);
    VEC3_ctor(&contact);
    VEC3_ctor(&hit_pos);
    VEC3_ctor(&body_pos);
    eft_shape_init((struct EftShape*)&hit_box);
    eft_shape_init((struct EftShape*)&body_box);
    shape = hit->size_data_0x08;
    struck = 0;
    for (; shape->joint != -1; shape++) {
        part = body->shape_0x0C;
        hit_kind = hit_shape_get(hit, shape, &hit_capsule, &hit_sphere);
        if (hit_kind == 2) {
            continue;
        }
        if (hit_kind == 1) {
            pl_capsule_box_set(&hit_capsule, &hit_box);
        }
        for (; part->joint != -1; part++) {
            body_kind = body_part_shape_get(body, part, &body_capsule, &body_sphere);
            if (hit_kind == 0) {
                if (body_kind == 0) {
                    if ((u32)pl_sphere_contact(&hit_sphere, &body_sphere, &contact) == 1) {
                        struck = 1;
                        copyVec3(&hit_pos, &hit_sphere.pos);
                        copyVec3(&body_pos, &body_sphere.pos);
                    }
                } else {
                    pl_capsule_box_set(&body_capsule, &body_box);
                    if ((u32)pl_sphere_capsule_contact(&hit_sphere, &body_box, &contact) == 1) {
                        struck = 1;
                        copyVec3(&hit_pos, &hit_sphere.pos);
                        body_pos.x = (body_capsule.start.x + body_capsule.end.x) / 2.0f;
                        body_pos.y = (body_capsule.start.y + body_capsule.end.y) / 2.0f;
                        body_pos.z = (body_capsule.start.z + body_capsule.end.z) / 2.0f;
                    }
                }
            } else if (body_kind == 0) {
                if ((u32)pl_capsule_sphere_contact(&hit_box, &body_sphere, &contact) == 1) {
                    struck = 1;
                    hit_pos.x = (hit_capsule.start.x + hit_capsule.end.x) / 2.0f;
                    hit_pos.y = (hit_capsule.start.y + hit_capsule.end.y) / 2.0f;
                    hit_pos.z = (hit_capsule.start.z + hit_capsule.end.z) / 2.0f;
                    copyVec3(&body_pos, &body_sphere.pos);
                }
            } else {
                pl_capsule_box_set(&body_capsule, &body_box);
                if ((u32)pl_capsule_capsule_contact(&hit_box, &body_box, &contact) == 1) {
                    struck = 1;
                    hit_pos.x = (hit_capsule.start.x + hit_capsule.end.x) / 2.0f;
                    hit_pos.y = (hit_capsule.start.y + hit_capsule.end.y) / 2.0f;
                    hit_pos.z = (hit_capsule.start.z + hit_capsule.end.z) / 2.0f;
                    body_pos.x = (body_capsule.start.x + body_capsule.end.x) / 2.0f;
                    body_pos.y = (body_capsule.start.y + body_capsule.end.y) / 2.0f;
                    body_pos.z = (body_capsule.start.z + body_capsule.end.z) / 2.0f;
                }
            }
            if (struck != 0) {
                hit_id_list_push(body->hit_ids_0x16, &body->hit_id_count_0x3E, hit->hit_id_0x0C);
                if ((body->source_kind & 0x30) == 0 || hit->field_0x045 >= 30) {
                    body->struck_0x06 = 1;
                    body->damage_0x14 += hit->life_0x40;
                    if ((body->source_kind & 0x30) != 0) {
                        hit->field_0x05B |= 1;
                    }
                }
                return;
            }
        }
    }
}

/* 0x8029D458 (0x88): the AI NPC's hit capsule: from its "NULL" joint to its "HD-00" joint, radius 40. */
extern "C" void ai_npc_hit_capsule_get(_AINPC_W* npc, PlCapsule* out)
{
    nw4r::math::VEC3 unused_a;
    nw4r::math::VEC3 unused_b;
    MHchar* chr;

    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    chr = &npc->model;
    chr->get_joint_wpos(mhchar_node_id_get(chr, "NULL"), &out->start);
    chr->get_joint_wpos(mhchar_node_id_get(chr, "HD-00"), &out->end);
    out->radius = 40.0f;
}

/* 0x8029D4E0 (0x21C): tests every hit-size record of the hit against the AI NPC's capsule and applies the first
 * contact to the NPC. */
extern "C" void hit_apply_npc(_HIT_W* hit, _AINPC_W* npc)
{
    PlCapsule hit_capsule;
    PlCapsule npc_capsule;
    HitSphere hit_sphere;
    PlCapsuleBox hit_box;
    PlCapsuleBox npc_box;
    nw4r::math::VEC3 contact;
    nw4r::math::VEC3 hit_pos;
    nw4r::math::VEC3 npc_pos;
    _HIT_SIZE_DATA* shape;
    s32 hit_kind;
    s32 struck;

    eft_capsule_ctor((nw4r::math::CAPSULE*)&hit_capsule);
    eft_capsule_ctor((nw4r::math::CAPSULE*)&npc_capsule);
    eft_vec_ctor(&hit_sphere.pos);
    eft_shape_init((struct EftShape*)&hit_box);
    eft_shape_init((struct EftShape*)&npc_box);
    VEC3_ctor(&contact);
    VEC3_ctor(&hit_pos);
    VEC3_ctor(&npc_pos);
    ai_npc_hit_capsule_get(npc, &npc_capsule);
    pl_capsule_box_set(&npc_capsule, &npc_box);
    shape = hit->size_data_0x08;
    struck = 0;
    for (; shape->joint != -1; shape++) {
        hit_kind = hit_shape_get(hit, shape, &hit_capsule, &hit_sphere);
        if (hit_kind == 2) {
            continue;
        }
        if (hit_kind == 1) {
            pl_capsule_box_set(&hit_capsule, &hit_box);
        }
        if (hit_kind == 0) {
            if ((u32)pl_sphere_capsule_contact(&hit_sphere, &npc_box, &contact) == 1) {
                struck = 1;
                copyVec3(&hit_pos, &hit_sphere.pos);
                npc_pos.x = (npc_capsule.start.x + npc_capsule.end.x) / 2.0f;
                npc_pos.y = (npc_capsule.start.y + npc_capsule.end.y) / 2.0f;
                npc_pos.z = (npc_capsule.start.z + npc_capsule.end.z) / 2.0f;
            }
        } else if ((u32)pl_capsule_capsule_contact(&hit_box, &npc_box, &contact) == 1) {
            struck = 1;
            hit_pos.x = (hit_capsule.start.x + hit_capsule.end.x) / 2.0f;
            hit_pos.y = (hit_capsule.start.y + hit_capsule.end.y) / 2.0f;
            hit_pos.z = (hit_capsule.start.z + hit_capsule.end.z) / 2.0f;
            npc_pos.x = (npc_capsule.start.x + npc_capsule.end.x) / 2.0f;
            npc_pos.y = (npc_capsule.start.y + npc_capsule.end.y) / 2.0f;
            npc_pos.z = (npc_capsule.start.z + npc_capsule.end.z) / 2.0f;
        }
        if (struck != 0) {
            hit_damage_npc(npc, hit, &contact, &hit_pos, &npc_pos);
            return;
        }
    }
}

/* 0x8029D6FC (0x48): whether two area bytes agree, or the scene is not in play mode 3. */
extern "C" u32 hit_area_match_ck(u8 a, u8 b)
{
    if (a == b || PlayMode_ck() != 3) {
        return 1;
    }
    return 0;
}

/* 0x8029D744 (0x150): the player's hit capsule: from the midpoint of its "LL-02"/"LR-02" joints to its "HD-00" joint,
 * shortened by the 40.0 radius at both ends. */
extern "C" void pl_hit_capsule_get(_PLW* plw, PlCapsule* out)
{
    nw4r::math::VEC3 leg_l;
    nw4r::math::VEC3 leg_r;
    nw4r::math::VEC3 axis;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 tmp0;
    nw4r::math::VEC3 tmp1;
    MHchar* chr;

    VEC3_ctor(&leg_l);
    VEC3_ctor(&leg_r);
    VEC3_ctor(&axis);
    VEC3_ctor(&dir);
    chr = &((PlJointBody*)plw->physics_0x13C)->chr_0x04;
    chr->get_joint_wpos(mhchar_node_id_get(chr, "LL-02"), &leg_l);
    chr->get_joint_wpos(mhchar_node_id_get(chr, "LR-02"), &leg_r);
    addVec3(&tmp0, &leg_l, &leg_r);
    copyVec3(&out->start, &tmp0);
    vec3_scale_in_place(&out->start, 0.5f);
    chr->get_joint_wpos(mhchar_node_id_get(chr, "HD-00"), &out->end);
    subVec3(&tmp1, &out->end, &out->start);
    copyVec3(&axis, &tmp1);
    if (vec3_length_sq(&axis.x) > 0.001f) {
        vec3_normalize_into(&dir, &axis);
    } else {
        copyVec3(&dir, &axis);
    }
    vec3_scale_in_place(&dir, 40.0f);
    addVec3To(&out->start, &dir);
    vec3_sub_assign((Vec*)&out->end, (Vec*)&dir);
    out->radius = 40.0f;
}

/* 0x8029D894 (0x230): places an enemy weak point in the world, scaled by the enemy's model scale: a sphere on the
 * enemy's position (joint 1000) or on its joint, or a capsule across one or two joints.  Returns 1 for a capsule. */
extern "C" s32 em_part_sphere_get(_ENEMY_WORK* em, EmWeakPoint* part, PlCapsule* capsule, HitSphere* sphere)
{
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 ofs_b;
    nw4r::math::MTX34 mtx;
    s32 result;

    VEC3_ctor(&ofs);
    VEC3_ctor(&ofs_b);
    MTX34_ctor(&mtx);
    result = 0;
    ofs.x = part->offset.x;
    ofs.y = part->offset.y;
    ofs.z = part->offset.z;
    if (part->joint == 1000) {
        sphere->radius = part->radius * get_em_scale(em);
        copyVec3(&sphere->pos, &em->pos);
        cpSetRotMatrix((_CP_VECTOR*)&em->field_0x1BC, &mtx);
        mulVecMat(&ofs, &mtx);
        sphere->pos.x += ofs.x;
        sphere->pos.y += ofs.y;
        sphere->pos.z += ofs.z;
        return 0;
    }
    get_joint_wmat_em(em, part->joint, &mtx);
    if ((u32)(part->shape - 1) > 1) {
        if (part->shape == 0) {
            sphere->radius = part->radius * get_em_scale(em);
            mulVecMat(&ofs, &mtx);
            sphere->pos.x = ofs.x + mtx.m[0][3];
            sphere->pos.y = ofs.y + mtx.m[1][3];
            sphere->pos.z = ofs.z + mtx.m[2][3];
            result = 0;
        }
    } else {
        capsule->radius = part->radius * get_em_scale(em);
        ofs_b.x = part->offset_b.x;
        ofs_b.y = part->offset_b.y;
        ofs_b.z = part->offset_b.z;
        mulVecMat(&ofs, &mtx);
        capsule->start.x = ofs.x + mtx.m[0][3];
        capsule->start.y = ofs.y + mtx.m[1][3];
        capsule->start.z = ofs.z + mtx.m[2][3];
        if (part->shape == 2) {
            get_joint_wmat_em(em, part->joint_b, &mtx);
        }
        mulVecMat(&ofs_b, &mtx);
        capsule->end.x = ofs_b.x + mtx.m[0][3];
        capsule->end.y = ofs_b.y + mtx.m[1][3];
        capsule->end.z = ofs_b.z + mtx.m[2][3];
        result = 1;
    }
    return result;
}

/* 0x8029DAC4 (0x214): places a body slot's shape record in the world on its owner's position and rotation (a stage
 * slot, kinds 4/5, uses the origin): a sphere (shape 0) or a capsule (shape 1).  Returns 1 for a capsule; a shape
 * not on joint 1000 gives an empty sphere. */
extern "C" s32 body_part_shape_get(_BODY_W* body, EmWeakPoint* part, PlCapsule* capsule, HitSphere* sphere)
{
    nw4r::math::VEC3 base;
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 ofs_b;
    nw4r::math::MTX34 mtx;
    void* owner;
    s32 result;

    VEC3_ctor(&base);
    VEC3_ctor(&ofs);
    VEC3_ctor(&ofs_b);
    MTX34_ctor(&mtx);
    result = 0;
    if (part->joint != 1000) {
        sphere->radius = 0.0f;
        sphere->pos.x = 0.0f;
        sphere->pos.y = 0.0f;
        sphere->pos.z = 0.0f;
        return 0;
    }
    if ((u32)(body->kind - 4) > 1) {
        switch (body->kind) {
        case 0:
            owner = (void*)body->field_0x010;
            copyVec3(&base, &((_PLW*)owner)->vec_0x03C);
            cpSetRotMatrix(&((_PLW*)owner)->rot_0x54, &mtx);
            break;
        case 1:
            owner = (void*)body->field_0x010;
            copyVec3(&base, &((_ENEMY_WORK*)owner)->pos);
            cpSetRotMatrix((_CP_VECTOR*)&((_ENEMY_WORK*)owner)->field_0x1BC, &mtx);
            break;
        case 2:
            owner = (void*)body->field_0x010;
            copyVec3(&base, &((_SHELL_W*)owner)->pos_0x18);
            cpSetRotMatrix((_CP_VECTOR*)&((_SHELL_W*)owner)->rot_x_0x24, &mtx);
            break;
        case 3:
            owner = (void*)body->field_0x010;
            copyVec3(&base, &((_AINPC_W*)owner)->pos_0x178);
            cpSetRotMatrix(&((_AINPC_W*)owner)->vec_0x190, &mtx);
            break;
        }
    } else {
        setVector3(&base, 0.0f, 0.0f, 0.0f);
        mtx34_identity(&mtx);
    }
    ofs.x = part->offset.x;
    ofs.y = part->offset.y;
    ofs.z = part->offset.z;
    mulVecMat(&ofs, &mtx);
    addVec3To(&ofs, &base);
    switch (part->shape) {
    case 0:
        sphere->radius = part->radius;
        copyVec3(&sphere->pos, &ofs);
        result = 0;
        break;
    case 1:
        capsule->radius = part->radius;
        copyVec3(&capsule->start, &ofs);
        ofs_b.x = part->offset_b.x;
        ofs_b.y = part->offset_b.y;
        ofs_b.z = part->offset_b.z;
        mulVecMat(&ofs_b, &mtx);
        addVec3To(&ofs_b, &base);
        copyVec3(&capsule->end, &ofs_b);
        result = 1;
        break;
    }
    return result;
}

/* 0x8029DCD8 (0x620): places one hit-size record of the hit in the world: a sphere on the owner's position (joint
 * 1000), the owner's own body capsule (1001), else a sphere or a capsule on the owner's joint.  Returns 0 for a sphere,
 * 1 for a capsule, 2 when the enemy part is switched off. */
extern "C" s32 hit_shape_get(_HIT_W* hit, _HIT_SIZE_DATA* shape, PlCapsule* capsule, HitSphere* sphere)
{
    nw4r::math::VEC3 ofs;
    nw4r::math::VEC3 ofs_b;
    nw4r::math::VEC3 base;
    nw4r::math::MTX34 mtx;
    nw4r::math::MTX34 rot;
    void* owner;
    MHchar* chr;
    s32 result;
    f32 scale;

    VEC3_ctor(&ofs);
    VEC3_ctor(&ofs_b);
    VEC3_ctor(&base);
    MTX34_ctor(&mtx);
    MTX34_ctor(&rot);
    result = 0;
    scale = hit->scale_0x24;
    switch (hit->owner_kind_0x06) {
    case 0:
        owner = hit->owner_0x10;
        chr = &((PlJointBody*)((_PLW*)owner)->physics_0x13C)->chr_0x04;
        break;
    case 1:
        owner = hit->owner_0x10;
        if (em_flags836_ck((_ENEMY_WORK*)owner, shape->part) == 1) {
            return 2;
        }
        break;
    case 2:
        owner = hit->owner_0x10;
        break;
    case 3:
        owner = hit->owner_0x10;
        chr = &((_AINPC_W*)owner)->model;
        break;
    }
    ofs.x = shape->offset.x * scale;
    ofs.y = shape->offset.y * scale;
    ofs.z = shape->offset.z * scale;
    if (shape->joint == 1000) {
        sphere->radius = shape->radius * scale;
        switch (hit->owner_kind_0x06) {
        case 0:
            copyVec3(&sphere->pos, &((_PLW*)owner)->vec_0x03C);
            cpSetRotMatrix(&((_PLW*)owner)->rot_0x54, &mtx);
            break;
        case 1:
            copyVec3(&sphere->pos, &((_ENEMY_WORK*)owner)->pos);
            cpSetRotMatrix((_CP_VECTOR*)&((_ENEMY_WORK*)owner)->field_0x1BC, &mtx);
            sphere->radius *= get_em_scale((_ENEMY_WORK*)owner);
            break;
        case 2:
            copyVec3(&sphere->pos, &((_SHELL_W*)owner)->pos_0x18);
            cpSetRotMatrix((_CP_VECTOR*)&((_SHELL_W*)owner)->rot_x_0x24, &mtx);
            break;
        case 3:
            copyVec3(&sphere->pos, &((_AINPC_W*)owner)->pos_0x178);
            cpSetRotMatrix(&((_AINPC_W*)owner)->vec_0x190, &mtx);
            break;
        }
        mulVecMat(&ofs, &mtx);
        sphere->pos.x += ofs.x;
        sphere->pos.y += ofs.y;
        sphere->pos.z += ofs.z;
        return 0;
    }
    if (shape->joint == 1001) {
        capsule->radius = shape->radius * scale;
        switch (hit->owner_kind_0x06) {
        case 0:
            capsule->start.x = ((_PLW*)owner)->vec_0x03C.x;
            capsule->start.y = ((_PLW*)owner)->vec_0x03C.y;
            capsule->start.z = ((_PLW*)owner)->vec_0x03C.z;
            capsule->end.x = ((_PLW*)owner)->vec_0x048.x;
            capsule->end.y = ((_PLW*)owner)->vec_0x048.y;
            capsule->end.z = ((_PLW*)owner)->vec_0x048.z;
            break;
        case 1:
            capsule->start.x = ((_ENEMY_WORK*)owner)->pos.x;
            capsule->start.y = ((_ENEMY_WORK*)owner)->pos.y;
            capsule->start.z = ((_ENEMY_WORK*)owner)->pos.z;
            capsule->end.x = ((_ENEMY_WORK*)owner)->vec_0x194.x;
            capsule->end.y = ((_ENEMY_WORK*)owner)->vec_0x194.y;
            capsule->end.z = ((_ENEMY_WORK*)owner)->vec_0x194.z;
            break;
        case 2:
            capsule->start.x = ((_SHELL_W*)owner)->pos_0x18.x;
            capsule->start.y = ((_SHELL_W*)owner)->pos_0x18.y;
            capsule->start.z = ((_SHELL_W*)owner)->pos_0x18.z;
            capsule->end.x = ((_SHELL_W*)owner)->vec_0x30.x;
            capsule->end.y = ((_SHELL_W*)owner)->vec_0x30.y;
            capsule->end.z = ((_SHELL_W*)owner)->vec_0x30.z;
            break;
        case 3:
            capsule->start.x = ((_AINPC_W*)owner)->pos_0x178.x;
            capsule->start.y = ((_AINPC_W*)owner)->pos_0x178.y;
            capsule->start.z = ((_AINPC_W*)owner)->pos_0x178.z;
            capsule->end.x = ((_AINPC_W*)owner)->pos_0x184.x;
            capsule->end.y = ((_AINPC_W*)owner)->pos_0x184.y;
            capsule->end.z = ((_AINPC_W*)owner)->pos_0x184.z;
            break;
        }
        return 1;
    }
    switch (hit->owner_kind_0x06) {
    case 0:
        mhchar_joint_mtx_get(chr, shape->joint, &mtx);
        break;
    case 1:
        get_joint_wmat_em((_ENEMY_WORK*)owner, shape->joint, &mtx);
        break;
    case 3:
        mhchar_joint_mtx_get(chr, shape->joint, &mtx);
        break;
    }
    switch (shape->shape) {
    case 0:
        sphere->radius = shape->radius * scale;
        if (hit->owner_kind_0x06 == 1) {
            sphere->radius *= get_em_scale((_ENEMY_WORK*)owner);
        }
        mulVecMat(&ofs, &mtx);
        sphere->pos.x = ofs.x + mtx.m[0][3];
        sphere->pos.y = ofs.y + mtx.m[1][3];
        sphere->pos.z = ofs.z + mtx.m[2][3];
        result = 0;
        break;
    case 1:
    case 2:
        capsule->radius = shape->radius * scale;
        if (hit->owner_kind_0x06 == 1) {
            capsule->radius *= get_em_scale((_ENEMY_WORK*)owner);
        }
        ofs_b.x = shape->offset_b.x * scale;
        ofs_b.y = shape->offset_b.y * scale;
        ofs_b.z = shape->offset_b.z * scale;
        mulVecMat(&ofs, &mtx);
        capsule->start.x = ofs.x + mtx.m[0][3];
        capsule->start.y = ofs.y + mtx.m[1][3];
        capsule->start.z = ofs.z + mtx.m[2][3];
        if (shape->shape == 2) {
            switch (hit->owner_kind_0x06) {
            case 0:
                mhchar_joint_mtx_get(chr, shape->joint_b, &mtx);
                break;
            case 1:
                get_joint_wmat_em((_ENEMY_WORK*)owner, shape->joint_b, &mtx);
                break;
            case 3:
                mhchar_joint_mtx_get(chr, shape->joint_b, &mtx);
                break;
            }
        }
        mulVecMat(&ofs_b, &mtx);
        capsule->end.x = ofs_b.x + mtx.m[0][3];
        capsule->end.y = ofs_b.y + mtx.m[1][3];
        capsule->end.z = ofs_b.z + mtx.m[2][3];
        result = 1;
        break;
    case 3:
        capsule->radius = shape->radius * scale;
        base.x = mtx.m[0][3];
        base.y = mtx.m[1][3];
        base.z = mtx.m[2][3];
        mtx34_rot_xyz_deg(&rot, shape->offset.x, shape->offset.y, shape->offset.z);
        mtx34_concat(&mtx, &mtx, &rot);
        ofs.x = 0.0f;
        ofs.y = 0.0f;
        ofs.z = shape->offset_b.x;
        ofs_b.x = 0.0f;
        ofs_b.y = 0.0f;
        ofs_b.z = shape->offset_b.z;
        mulVecMat(&ofs, &mtx);
        mulVecMat(&ofs_b, &mtx);
        capsule->start.x = ofs.x + base.x;
        capsule->start.y = ofs.y + base.y;
        capsule->start.z = ofs.z + base.z;
        capsule->end.x = ofs_b.x + base.x;
        capsule->end.y = ofs_b.y + base.y;
        capsule->end.z = ofs_b.z + base.z;
        result = 1;
        break;
    }
    return result;
}

/* 0x8029E2F8 (0x64): the shape of the hit's first hit-size record placed in the world (`hit_shape_get`), when `index`
 * is 0; 2 when the hit has none. */
extern "C" s32 hit_list_capsule_ck(_HIT_W* hit, PlCapsule* capsule, HitSphere* sphere, u16 index)
{
    _HIT_SIZE_DATA* shape;
    s32 result;

    shape = hit->size_data_0x08;
    result = 2;
    while (shape->joint != -1) {
        if (index == 0) {
            result = hit_shape_get(hit, shape, capsule, sphere);
            break;
        }
        shape++;
    }
    return result;
}

/* 0x8029E35C (0xC80): spawns a hit's visual effects at the contact point by its effect kind (+0x4C, low six bits):
 * the hit-mark and element effects, the spark by the hit's result, and the deeper variant (`kind + 4`) when the
 * contact is below `water_y`. */
extern "C" void hit_effect_spawn(_HIT_W* hit, nw4r::math::VEC3* contact, nw4r::math::VEC3* from, nw4r::math::VEC3* to,
                                 f32 water_y, EmPartRates* rates)
{
    _CP_VECTOR rot;
    nw4r::math::VEC3 unused;
    u32 deep;
    u8 fx;

    VEC3_ctor(&unused);
    if ((hit->field_0x04C & 0x3F) == 0) {
        return;
    }
    rot.x = 0;
    rot.y = hit_dir_y_get(hit, from, to);
    rot.z = 0;
    fx = hit->field_0x04C;
    if ((fx & 0x80) != 0) {
        eft002_pair_spawn(contact, 9, hit, 0);
    } else if ((fx & 0x40) != 0) {
        eft002_pair_spawn(contact, 10, hit, 0);
    }
    if (rates != NULL && rates->stun_0x08 != 0 && hit->field_0x04E != 0
        && (hit->owner_kind_0x06 == 0 || hit->kind_0x07 == 0)) {
        eft002_pair_spawn(contact, 16, hit, 0);
    }
    if (hit->target_kind_0x30 == 1 && ((_ENEMY_WORK*)hit->target_0x2C)->team == 40) {
        eft013_guard_spawn(contact, &rot.x, 1.0f, hit);
        return;
    }
    if (water_y > -10000.0f && contact->y < water_y) {
        deep = 4;
    } else {
        deep = 0;
    }
    switch (hit->field_0x04C & 0x3F) {
    case 1:
    case 28:
    case 29:
        if (hit->kind_0x07 != 1 && hit->owner_kind_0x06 != 1) {
            if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0) {
                eft002_pair_spawn(contact, 8, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x40) != 0) {
                eft002_model_spawn(contact, 14, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x20) != 0) {
                eft002_pair_spawn(contact, 15, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x200) != 0) {
                eft002_pair_spawn(contact, 17, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x80) != 0) {
                eft002_model_spawn(contact, 18, hit, 0);
            } else if ((hit->field_0x048 & 4) != 0) {
                eft002_model_spawn(contact, 19, hit, 0);
            } else if ((hit->field_0x048 & 2) != 0) {
                eft002_pair_spawn(contact, 20, hit, 0);
            } else if ((hit->field_0x048 & 1) != 0) {
                eft002_pair_spawn(contact, 21, hit, 0);
            }
        }
        switch (hit->field_0x04C & 0x3F) {
        case 28:
            eft002_pair_spawn(contact, 22, hit, 0);
            break;
        case 29:
            eft002_pair_spawn(contact, 23, hit, 0);
            break;
        }
        switch (hit->result) {
        case 0:
        case 1:
            eft013_guard_spawn(contact, &rot.x, 1.0f, hit);
            break;
        case 2:
            eft013_shell_spawn(contact, deep + 2, &rot, 1.0f, hit);
            eft002_pair_spawn(contact, 2, hit, 0);
            break;
        case 3:
            eft013_shell_spawn(contact, deep + 1, &rot, 1.0f, hit);
            eft002_pair_spawn(contact, 1, hit, 0);
            break;
        case 4:
            eft013_shell_spawn(contact, deep, &rot, 1.0f, hit);
            eft002_pair_spawn(contact, 0, hit, 0);
            break;
        }
        break;
    case 15:
    case 16:
    case 17:
    case 18:
        switch (hit->field_0x04C & 0x3F) {
        case 15:
            eft002_pair_spawn(contact, 11, hit, 0);
            break;
        case 16:
            eft002_pair_spawn(contact, 11, hit, 1);
            break;
        case 17:
            eft002_pair_spawn(contact, 11, hit, 2);
            break;
        case 18:
            eft002_pair_spawn(contact, 11, hit, 3);
            break;
        }
        eft013_shell_spawn(contact, deep + 2, &rot, 0.8f, hit);
        eft002_pair_spawn(contact, 5, hit, 0);
        break;
    case 21:
        eft002_pair_spawn(contact, 12, hit, 0);
        eft002_pair_spawn(contact, 4, hit, 0);
        eft013_shell_spawn(contact, deep + 1, &rot, 0.8f, hit);
        break;
    case 5:
        eft013_shell_spawn(contact, deep, &rot, 1.0f, hit);
        eft002_pair_spawn(contact, 3, hit, 0);
        break;
    case 6:
        eft013_shell_spawn(contact, deep + 1, &rot, 1.0f, hit);
        eft002_pair_spawn(contact, 4, hit, 0);
        break;
    case 7:
        eft013_shell_spawn(contact, deep + 2, &rot, 1.0f, hit);
        eft002_pair_spawn(contact, 5, hit, 0);
        break;
    case 8:
        eft013_shell_spawn(contact, 3, &rot, 0.5f, hit);
        break;
    case 3:
        eft002_pair_spawn(contact, 7, hit, 0);
        break;
    case 4:
        eft002_pair_spawn(contact, 8, hit, 0);
        break;
    case 20:
        eft013_target_set(hit);
        break;
    case 2:
        eft002_pair_spawn(contact, 6, hit, 0);
        break;
    case 13:
        eft013_set_from_sel(hit, contact, 21);
        break;
    case 14:
        eft013_set_from_sel(hit, contact, 8);
        break;
    case 19:
        if (hit->kind_0x07 != 1 && hit->owner_kind_0x06 != 1) {
            if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0) {
                eft002_pair_spawn(contact, 8, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x40) != 0) {
                eft002_model_spawn(contact, 14, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x20) != 0) {
                eft002_pair_spawn(contact, 15, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x200) != 0) {
                eft002_pair_spawn(contact, 17, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x80) != 0) {
                eft002_model_spawn(contact, 18, hit, 0);
            }
        }
        eft013_shell_spawn(contact, deep + 1, &rot, 1.0f, hit);
        eft002_pair_spawn(contact, 4, hit, 0);
        break;
    case 22:
        eft002_pair_spawn(contact, 13, hit, 0);
        break;
    case 24:
        eft002_pair_spawn(contact, 13, hit, 1);
        break;
    case 23:
        eft002_pair_spawn(contact, 11, hit, 4);
        break;
    case 25:
        eft020_actor_spawn(hit->target_0x2C, hit->target_kind_0x30, 0, 1.0f);
        break;
    case 26:
        eft020_actor_spawn(hit->target_0x2C, hit->target_kind_0x30, 1, 1.0f);
        break;
    case 27:
        eft020_actor_spawn(hit->target_0x2C, hit->target_kind_0x30, 2, 1.0f);
        break;
    case 30:
        eft002_pair_spawn(contact, 4, hit, 0);
        break;
    case 31:
        eft002_pair_spawn(contact, 3, hit, 0);
        break;
    case 32:
        eft002_pair_spawn(contact, 4, hit, 0);
        break;
    case 33:
        eft002_pair_spawn(contact, 5, hit, 0);
        break;
    case 34:
        eft002_pair_spawn(contact, 24, hit, 0);
        eft013_shell_spawn(contact, deep + 2, &rot, 1.0f, hit);
        break;
    case 35:
        eft002_pair_spawn(contact, 11, hit, 3);
        break;
    case 36:
        eft002_pair_spawn(contact, 25, hit, 0);
        if (hit->kind_0x07 != 1 && hit->owner_kind_0x06 != 1) {
            if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0) {
                eft002_pair_spawn(contact, 8, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x40) != 0) {
                eft002_model_spawn(contact, 14, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x20) != 0) {
                eft002_pair_spawn(contact, 15, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x200) != 0) {
                eft002_pair_spawn(contact, 17, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x80) != 0) {
                eft002_model_spawn(contact, 18, hit, 0);
            } else if ((hit->field_0x048 & 4) != 0) {
                eft002_model_spawn(contact, 19, hit, 0);
            } else if ((hit->field_0x048 & 2) != 0) {
                eft002_pair_spawn(contact, 20, hit, 0);
            } else if ((hit->field_0x048 & 1) != 0) {
                eft002_pair_spawn(contact, 21, hit, 0);
            }
        }
        break;
    case 37:
        eft002_pair_spawn(contact, 26, hit, 0);
        if (hit->kind_0x07 != 1 && hit->owner_kind_0x06 != 1) {
            if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x10) != 0) {
                eft002_pair_spawn(contact, 8, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x40) != 0) {
                eft002_model_spawn(contact, 14, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x20) != 0) {
                eft002_pair_spawn(contact, 15, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x200) != 0) {
                eft002_pair_spawn(contact, 17, hit, 0);
            } else if (hit_flag48_high_ck(hit) == 0 && (hit->field_0x048 & 0x80) != 0) {
                eft002_model_spawn(contact, 18, hit, 0);
            } else if ((hit->field_0x048 & 4) != 0) {
                eft002_model_spawn(contact, 19, hit, 0);
            } else if ((hit->field_0x048 & 2) != 0) {
                eft002_pair_spawn(contact, 20, hit, 0);
            } else if ((hit->field_0x048 & 1) != 0) {
                eft002_pair_spawn(contact, 21, hit, 0);
            }
        }
        break;
    default:
        eft002_pair_spawn(contact, 2, hit, 0);
        break;
    case 0:
        break;
    }
}

/* 0x8029EFDC (0x30): pushes an active hit onto the registry's attack list. */
extern "C" void hit_attack_list_push(_HIT_W* hit)
{
    HitRegistry* reg = &hit_registry;
    _HIT_W* prev;

    if (hit->active_0x05 == 0) {
        return;
    }
    prev = reg->head_0x00;
    reg->head_0x00 = hit;
    hit->next_0x00 = prev;
    reg->count_0x08++;
}

/* 0x8029F00C (0x30): the same push onto the registry's body list. */
extern "C" void hit_body_list_push(_HIT_W* hit)
{
    HitRegistry* reg = &hit_registry;
    _HIT_W* prev;

    if (hit->active_0x05 == 0) {
        return;
    }
    prev = reg->head_0x04;
    reg->head_0x04 = hit;
    hit->next_0x00 = prev;
    reg->count_0x0A++;
}

/* 0x8029F03C (0xC): records one hit's owner and its kind. */
extern "C" void hit_source_set(_HIT_W* self, u8 kind, void* owner /* untyped: caller-owned payload - the shell, player or enemy record the kind names */)
{
    self->owner_0x10 = owner;
    self->owner_kind_0x06 = kind;
}

/* 0x8029F048 (0xC): records the second owner slot of a hit. */
extern "C" void hit_owner_set(_HIT_W* self, u8 kind, void* owner /* untyped: caller-owned payload - the shell, player or enemy record the kind names */)
{
    self->owner_0x14 = owner;
    self->kind_0x07 = kind;
}

/* 0x8029F054 (0x30): hands out the next hit id, wrapping around at 60000. */
u16 get_hit_id(void)
{
    HitRegistry* reg = &hit_registry;

    reg->next_id_0x0C++;
    if (reg->next_id_0x0C >= 0xEA60) {
        reg->next_id_0x0C = 0;
    }
    return reg->next_id_0x0C;
}

/* 0x8029F084 (0xEC): whether `id` is absent from the two ten-entry halves of a 20-entry id list. */
extern "C" u32 hit_id_list_absent_ck(u16* ids, u16 id)
{
    s32 checked;   /* the original's running count of tested entries (its increments are +9) */
    s32 row;

    checked = 0;
    for (row = 0; row < 2; row++, ids += 10) {
        s32 j;

        for (j = 0; j < 10; j++) {
            if (id == ids[j]) {
                return 0;
            }
        }
        checked += 9;
    }
    return 1;
}

/* 0x8029F170 (0x64): empties a 20-entry id list and clears the count byte that indexes it. */
extern "C" void hit_id_list_clear(u16* ids, u8* count)
{
    s32 i;

    *count = 0;
    for (i = 0; i < 20; i++) {
        ids[i] = 0xFFFF;
    }
}

/* 0x8029F1D4 (0x30): appends `id` to the ring, wrapping the count around at 20. */
extern "C" void hit_id_list_push(u16* ids, u8* count, u16 id)
{
    ids[*count] = id;
    (*count)++;
    if ((u8)*count >= 20) {
        *count = 0;
    }
}

/* 0x8029F204 (0x1C4): arms a hit entry from an attack record: a fresh hit id, no result, the record's kind, flags,
 * masks and strength, its two side values (0xFF: none) and its owner's area. */
extern "C" void hit_data_apply(_HIT_W* hit, _HIT_DATA* data)
{
    s8 strength;

    hit->active_0x05 = 1;
    hit->hit_id_0x0C = get_hit_id();
    hit->result = 0xFF;
    hit->attack_flags_0x31 = 0;
    hit->life_0x40 = data->life_0x04;
    hit->kind_0x42 = data->kind_0x06;
    hit->flags_0x43 = data->flags_0x07;
    hit->dir_ofs_0x44 = data->dir_ofs_0x08;
    hit->field_0x045 = data->field_0x09;
    hit->mask_0x46 = data->mask_0x0A;
    hit->field_0x048 = data->field_0x0C;
    hit->field_0x04A = data->field_0x0E;
    hit->field_0x04C = data->field_0x10;
    hit->field_0x04D = data->effect_0x11;
    hit->field_0x04E = strength = data->strength_0x12;
    if (strength < 0) {
        hit->field_0x04E = 0;
    }
    hit->side_flags_0x33 = 0;
    hit->field_0x059 = data->field_0x16;
    hit->field_0x05A = data->field_0x17;
    hit->scale_0x24 = 1.0f;
    if (data->side_a_0x13 == 0xFF) {
        hit->value_0x50 = -100.0f;
        hit->value_0x54 = 0.0f;
    } else {
        hit->value_0x50 = data->side_a_0x13;
        if (data->side_b_0x14 == 0xFF) {
            hit->value_0x54 = -100.0f;
        } else {
            hit->value_0x54 = data->side_b_0x14;
        }
    }
    hit->reaction_0x58 = data->reaction_0x15;
    hit->target_part_0x28 = NULL;
    hit->target_0x2C = NULL;
    hit->target_kind_0x30 = 0;
    hit->field_0x05B = 0;
    switch (hit->owner_kind_0x06) {
    case 0:
        hit->area_0x0F = ((_PLW*)hit->owner_0x10)->area_0x16;
        break;
    case 1:
        hit->area_0x0F = ((_ENEMY_WORK*)hit->owner_0x10)->area_no;
        break;
    case 2:
        hit->area_0x0F = ((_SHELL_W*)hit->owner_0x10)->area_0x08;
        break;
    case 3:
        hit->area_0x0F = ((_AINPC_W*)hit->owner_0x10)->area;
        break;
    }
}

/* 0x8029F3C8: fills a body slot from a body record (the appearance/hit state of a body hit); `kind` picks the
 * work byte copied and its sub-code, the second argument is stored, not read. */
void body_set(_BODY_W* body, _BODY_DATA* data, u8 kind, u32 work, u8 mode)
{
    _BODY_DATA* work_rec = (_BODY_DATA*)work;

    body->field_0x005 = 1;
    body->struck_0x06 = 0;
    body->kind = kind;
    body->field_0x010 = work;
    body->field_0x00A = 0;
    body->data = (u32)data;
    body->source_kind = 0;
    body->damage_0x14 = 0;
    body->field_0x03F = mode;
    switch (kind) {
    case 0:
        body->source = work_rec->area;
        body->source_kind = 1;
        break;
    case 1:
        body->source = work_rec->field_0x1E1;
        body->source_kind = 2;
        break;
    case 2:
        body->source = work_rec->chunk_ofs;
        body->source_kind = 8;
        break;
    case 3:
        body->source = work_rec->field_0x1A4;
        body->source_kind = 4;
        break;
    case 4:
        body->source = get_now_areano();
        body->source_kind = 16;
        break;
    case 5:
        body->source = get_now_areano();
        body->source_kind = 32;
        break;
    }
}

/* 0x8029F4C4: flags a hit entry as populated and stores the four values the caller resolved. */
extern "C" void hit_motion_set(_HIT_W* hit, u16 a, u16 b, u16 c, u16 d)
{
    hit_flag_set(hit, 0x100);
    hit->field_0x018 = a;
    hit->field_0x01A = b;
    hit->field_0x01C = c;
    hit->field_0x01E = d;
}

/* 0x8029F51C: whether one of the hit entry's +0x05B bits is set. */
extern "C" u32 hit_mask_ck(_HIT_W* hit, u32 mask)
{
    return (hit->field_0x05B & (u8)mask) != 0;
}

/* 0x8029F538: clears the hit entry's flag word. */
extern "C" void hit_flags_clear(_HIT_W* hit)
{
    hit->flags = 0;
}

/* 0x8029F544: sets hit flags. */
void hit_flag_set(_HIT_W* hit, u32 flags)
{
    hit->flags |= flags;
}

/* 0x8029F554: clears hit flags. */
extern "C" void hit_flag_clear(_HIT_W* hit, u32 flags)
{
    hit->flags &= ~flags;
}

/* 0x8029F564: whether any of the requested hit flags is set. */
extern "C" u32 hit_flag_ck(_HIT_W* hit, u32 flags)
{
    return (hit->flags & flags) != 0;
}

/* 0x8029F57C: whether one of the hit entry's +0x031 bits is set. */
extern "C" u32 hit_attack_flag_ck(_HIT_W* hit, u32 mask)
{
    return (hit->field_0x031 & (u8)mask) != 0;
}

/* 0x8029F598: the hit entry's result id, or 0xFF when the entry carries no result. */
u8 hit_result_check(_HIT_W* hit)
{
    if (hit->result_valid == 0) {
        return 255;
    }
    return hit->result;
}

/* 0x8029F5B4: stores the attack's damage value as the entry's +0x03C float. */
extern "C" void hit_knock_set(_HIT_W* hit, s16 value)
{
    hit->field_0x03C = (f32)value;
}

/* 0x8029F5E4: whether the hit part's flags are clear of the 0xD2 set (the parts a body push skips). */
extern "C" u32 hit_push_enabled_ck(EmWeakPoint* part)
{
    return (part->flags & 0xD2) == 0;
}

/* 0x8029F5F8: whether the hit part's flags are clear of the 0xB2 set (the parts the camera push skips). */
extern "C" u32 hit_camera_push_enabled_ck(EmWeakPoint* part)
{
    return (part->flags & 0xB2) == 0;
}

/* 0x8029F60C: the head of the registry's body list. */
extern "C" _BODY_W* hit_body_list_get(void)
{
    return hit_registry.body_head_0x04;
}
