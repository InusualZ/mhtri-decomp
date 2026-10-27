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
 * FLAGS. `cflags_pl` (configure.py's Pl lib).
 * NAMES. GUESS: the file name (no `__FILE__` string in the range; the bodies run the hit registry).  The runtime dump's real
 *   names: `body_set`, `hit_flag_set`, `hit_result_check`; the map's own `get_hit_id__Fv`.  GUESS names from the bodies
 *   and callers: `hit_registry_clear`, `hit_registry_update`, `hit_life_update`, `hit_targets_reset`, `body_hit_mask_ck`,
 *   `hit_side_kind_get`, `hit_flag48_high_ck`, `hit_dir_y_get`, `hit_dir_x_get`, `hit_owner_reaction_set`,
 *   `hit_owner_rate_get`, `pl_elem_resist_get`, `hit_reaction_priority_get` (and its table `hit_reaction_priority_tbl`),
 *   `hit_apply_pl`/`_em`/`_npc`/`_body` (the per-target appliers), `hit_area_match_ck`, `pl_hit_capsule_get`,
 *   `hit_shape_get`, `hit_list_capsule_ck`, `hit_data_apply`, `hit_attack_list_push`, `hit_body_list_push`,
 *   `hit_source_set`, `hit_owner_set`, `hit_id_list_absent_ck`, `hit_id_list_clear`, `hit_id_list_push`, `hit_motion_set`,
 *   `hit_mask_ck`, `hit_flags_clear`, `hit_flag_clear`, `hit_flag_ck`, `hit_attack_flag_ck`, `hit_knock_set`,
 *   `hit_push_enabled_ck`, `hit_camera_push_enabled_ck`, `hit_body_list_get`, the registry `hit_registry`;
 *   `em_part_sphere_get` is the registry name `hud/cockpit` gave it.  Registry proposals for foreign callees (GUESS):
 *   `pl_act_kind7_ck` (0x8027C030), `em_hit_kind_accept_ck` (0x8012F110), `ai_npc_time_scale_get` (0x802D29DC).
 * RESIDUALS. 17 rows unwritten (17116 B): 0x8029A7A0-0x8029ACB8 (the four hit-effect dispatchers, whose callees are the
 *   unnamed `sound/fn_800D7F54` effect rows), 0x8029AFF8-0x8029B8F4, 0x8029B918-0x8029D6FC except the written
 *   `hit_area_match_ck`, 0x8029D894-0x8029DCD8 (`em_part_sphere_get`, 0x8029DAC4), `hit_shape_get` (0x8029DCD8) and
 *   0x8029E35C-0x8029EFDC.  Most read the AI NPC through `ai/ai_npc.h`'s view (its model at +0x008), which defines its
 *   own `_HIT_W` and cannot be included beside `Pl/hit_w.h`; `ai/ainpc.h`'s view has no model.  The partial rows:
 *  - `hit_side_kind_get`: the branch sense of one `fcmpo`, and its tail block laid out in the other order;
 *  - `hit_id_list_absent_ck`: retail keeps an unused accumulator (`li r5,0`, `+9` per outer iteration) MWCC deletes;
 *  - `hit_registry_update`: register permutation, and the enemy kind test's 0x10 branch is laid out after the busy test;
 *  - `pl_elem_resist_get`: retail places the default case right after the compare chain;
 *  - `hit_owner_reaction_set`, `hit_dir_y_get`: register permutations; `hit_life_update`: retail reads the second motion
 *    pair through a `+4` base register;
 *  - data: the `.bss` registry, the `.data` tables and the pool are declared, not emitted; the `.sdata` joint names come
 *    from string literals ("LL-02", "LR-02", "HD-00"; "NULL" is the unwritten NPC capsule's).
 * SHAPES. A hit whose owner kind is not 0/1/3 skips the motion check through `continue` in a `do { } while (0)`;
 *   the attack-kind tests take the kind into an `s32` (retail compares the single kinds with `cmpwi`, the pairs with
 *   `subi`+`cmplwi`); `hit_data_apply` copies the signed strength through a local before the sign test.
 */

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
#include "ai/ainpc.h"
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

/* `pl_act_held_ck`: the owner (`Pl/pl_act.cpp`, declared in `Pl/pl_act.h`) defines a second, unused parameter its callers
 * never load, so the call goes through a one-parameter view of the same address (as in `Pl/pl_act_step.cpp`). */
#define pl_act_held_ck_c1 ((u32 (*)(_PLW*))pl_act_held_ck)

/* The unit's own rows the bodies above them call. */
extern "C" {
void hit_apply_pl(_HIT_W* hit, _PLW* plw);
void hit_apply_em(_HIT_W* hit, _ENEMY_WORK* em);
void hit_apply_npc(_HIT_W* hit, _AINPC_W* npc);
void hit_apply_body(_HIT_W* hit, _BODY_W* body);
s32 hit_shape_get(_HIT_W* hit, _HIT_SIZE_DATA* shape, PlCapsule* capsule, HitSphere* sphere);
}

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
                    && hit_area_match_ck(hit->area_0x0F, ainpc_w.field_0x1A4) != 0
                    && hit_id_list_absent_ck(ainpc_w.hit_ids_0x2F8, hit->hit_id_0x0C) != 0 && ainpc_w.field_0x214 <= 0
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
            angle = ((_AINPC_W*)hit->owner_0x10)->field_0x194;
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
            angle = ((_AINPC_W*)hit->owner_0x10)->field_0x190;
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
            rate = ((_AINPC_W*)hit->owner_0x10)->hit_rate_0x204;
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

/* 0x8029B8F4 (0x24): the priority of a damage reaction id, or 0 for an id past the table. */
extern "C" u32 hit_reaction_priority_get(u32 id)
{
    if ((u8)id > 0x24) {
        return 0;
    }
    return hit_reaction_priority_tbl[(u8)id];
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
    hit->field_0x028 = 0;
    hit->field_0x02C = 0;
    hit->field_0x030 = 0;
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
        hit->area_0x0F = ((_AINPC_W*)hit->owner_0x10)->field_0x1A4;
        break;
    }
}

/* 0x8029F3C8: fills a body slot from a body record (the appearance/hit state of a body hit); `kind` picks the
 * work byte copied and its sub-code, the second argument is stored, not read. */
void body_set(_BODY_W* body, _BODY_DATA* data, u8 kind, u32 work, u8 mode)
{
    _BODY_DATA* work_rec = (_BODY_DATA*)work;

    body->field_0x005 = 1;
    body->field_0x006 = 0;
    body->kind = kind;
    body->field_0x010 = work;
    body->field_0x00A = 0;
    body->data = (u32)data;
    body->source_kind = 0;
    body->field_0x014 = 0;
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
