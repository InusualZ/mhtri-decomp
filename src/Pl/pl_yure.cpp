/*
 * Pl/pl_yure.cpp - the equipment sway ("yure") band: short spring chains hung off a character's joints (hair, cloth,
 *   tails) that lag behind the body.  The records (`YureRec`, 0x90 B) live in the enemy work record (3 at +0x590) and
 *   the player work record (8 at +0x674); `yure_move` steps them all each frame through `yure_step`.
 * RANGE. .text 0x802ABD28-0x802AD9C0 (11 functions); .ctors 0x8056F378 (`yure_static_init`, which builds the gravity
 *   vector), .data 0x805CE638-0x805CED40 (the thirteen sway tables), .sdata 0x807922C0-0x807922E0 (the four `CL-0n`
 *   labels), .bss 0x806B8798-0x806B87C0 (`yure_world`), .sdata2 0x8079A410-0x8079A448, extab, extabindex.  The seam
 *   with `stage/shell.cpp` below: this TU has its own static initialiser and `.data` tables (read only by these
 *   functions) and no reference to the shell pool.
 * NAMES. The module and the file name are GUESSes from the behaviour (`yure` = sway; the map's `yure_move__Fv` is the
 *   exported entry point).  GUESS: `pl_parts_value_get` (0x80269508, registry name: it returns the part value
 *   `pl_parts_set` compares against); `YureEnemyWork::sway_mode_0x1E2` (2 picks the alternate spring constants).
 * RESIDUALS. Every row left differs only in register or stack-slot choice:
 *  - `yure_step`: `j` lands in r18-r21 where the target reuses `index`'s r16 for five of its loops; the chain loop's
 *    vel pointer and tip offset swap r24/r26; the `fsubs` of the pin push and the spring stretch is scheduled after
 *    the argument `addi`s instead of before them.
 *  - `yure_player_init_kind`: `src`/`label`/the inner-loop pointers take r20-r26 where the target uses r21-r27 (the
 *    best of the declaration orders measured); `yure_enemy_init` the same shift for `src` (r25 for r29).
 *  - `yure_move`: the enemy/player work pointers and counters take other non-volatile registers (r25-r30).
 *  - flipcheck: `.text` only; `pl_parts_value_get` (0x80269508) resolves once the registry name is in the map.
 *    `.sdata2` carries all thirteen pool values in the target's order (the target's fourteenth word is the link pad).
 * SHAPES. The four `CL-0n` labels are named `char` arrays in `.sdata` (a `const` array lands in `.sdata2`;
 *   `yure_label_3[8]` carries the pad of the target's 0x20-byte run), so the `.data` table's relocations name the
 *   map's rows.  `get_move_work_max` is declared `u32` in `ef/get_move_work_adrs.h`: the retail caller narrows the
 *   result with `clrlwi ...,16`.
 */

#include "types.h"

#include "Pl/pl_yure.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "ef.h"
#include "ef/get_move_work_adrs.h"
#include "enemy/joint_mtx_store.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_state.h"
#include "mh3_pad/vec3.h"
#include "unsplit/unknown.h"
#include "nw4r/fn_805012C4.h"
#include "MSL_C/alloc.h"
#include "Pl/pl_se_rig.h"
#include "Pl/pl_coll.h"
#include "Pl/pl_parts_value_get.h"
#include "lobby/pl_rig_style_byte_get.h"
#include "lobby/lb_equip_page.h"
#include "lobby/event_demo_running_ck.h"
#include "lobby/lb_server_sel_trans.h"
#include "g3d/res_node_get_parent.h"
#include "g3d/res_node_assign.h"
#include "g3d/fn_8005AA28.h"
#include "g3d/g3d_anmchr.h"
#include "g3d/g3d_resmat.h"
#include "g3d/vec3_ctor_return.h"
#include "g3d/g3d_calcview.h"
#include "g3d/g3d_calcworld.h"
#include "sound/mhchar_draw_callback_set.h"
#include "sound/mhchar_joint_mtx_get.h"
#include "enemy/em_res_node_trans_get.h"
#include "enemy/em_res_node_scale_get.h"
#include "enemy/fn_8012EC74.h"
#include "enemy/em_common.h"
#include "ef/effect.h"
#include "ef/mtx34_trans_get.h"
#include "ef/eft001.h"
#include "draw_shape/mtx34_copy.h"
#include "scaleMat34W.h"
#include "vec3_clamp_distance.h"

/* One row of a span table: how many sway records a type owns and the first one's row in the source table.
 * size: 0x2 */
struct YureSpan {
    /* +0x00 */ s8 count_0x00;
    /* +0x01 */ s8 start_0x01;
};

/* One row of a sway source table: the recipe one record is built from.  size: 0xE */
struct YureSrc {
    /* +0x00 */ u8 shape_0x00;
    /* +0x01 */ u8 param_0x01;           /* the row of the matching parameter table */
    /* +0x02 */ u8 count_0x02;           /* the chain length */
    /* +0x03 */ u8 matrix_joint_0x03;    /* the joint whose matrix the chain is expressed in */
    /* +0x04 */ u8 root_joint_0x04;      /* the joint the chain hangs from */
    /* +0x05 */ u8 joint_0x05[YURE_JOINT_MAX];   /* the joint each chain point follows (0xFF for none) */
    /* +0x07 */ u8 unused_0x07;
    /* +0x08 */ s16 rest_0x08[YURE_JOINT_MAX];   /* each point's row in `yure_rest_tbl` */
    /* +0x0C */ s16 pin_0x0C;            /* the row of `yure_pin_tbl` / `yure_pin_joint_tbl`, 0 for none */
};

/* One 0x18-byte row of the pin table: a direction and an offset.  size: 0x18 */
struct YurePin {
    /* +0x00 */ Vec dir_0x00;
    /* +0x0C */ Vec offset_0x0C;
};

/* One row of a sway parameter table: eight spring constants and the angle offset of a record type.  size: 0x24 */
struct YureParam {
    /* +0x00 */ f32 coeff_0x00[8];
    /* +0x20 */ s32 angle_offset_0x20;    /* added to the pitch word (0x8000 = half a turn), as a 32-bit load */
};

extern const char* yure_label_tbl[4];
extern Vec yure_rest_tbl[9];
extern YurePin yure_pin_tbl[13];
extern YureSpan yure_enemy_span[42];
extern YureSpan yure_player_span5[6];
extern YureSpan yure_player_span4[8];
extern YureSrc yure_enemy_src[13];
extern YureSrc yure_player_src5[5];
extern YureSrc yure_player_src4[10];
extern YureParam yure_enemy_param[14];
extern YureParam yure_player_param5[4];
extern YureParam yure_player_param4[5];
extern u8 yure_pin_joint_tbl[20];

extern "C" void yure_enemy_init(YureEnemyWork* enemy) {
    nw4r::math::VEC3 scratch_vec;
    nw4r::math::MTX34 scratch_mtx;
    YureSrc* src;
    s32 i;
    u32 j;
    s32 count;
    s32 start;
    YureRec* rec;

    VEC3_ctor(&scratch_vec);
    MTX34_ctor(&scratch_mtx);
    memset(enemy->yure_0x590, 0, sizeof(enemy->yure_0x590));
    rec = enemy->yure_0x590;
    start = yure_enemy_span[enemy->type_0x003].start_0x01;
    count = yure_enemy_span[enemy->type_0x003].count_0x00;
    for (i = 0, src = &yure_enemy_src[start]; i < count; src++, i++, rec++) {
        rec->active_0x00 = 1;
        rec->state_0x03 = 0;
        rec->is_enemy_0x06 = 1;
        rec->slot_0x05 = start + i;
        rec->shape_0x01 = src->shape_0x00;
        rec->param_0x02 = src->param_0x01;
        rec->count_0x08 = src->count_0x02;
        rec->root_joint_0x0C = src->root_joint_0x04;
        rec->pin_0x04 = src->pin_0x0C;
        for (j = 0; j < rec->count_0x08; j++) {
            rec->joint_0x10[j] = src->joint_0x05[j];
            vec_to_mh_vec3(&rec->rest_0x54[j], &yure_rest_tbl[src->rest_0x08[j]]);
            setVector3(&rec->vel_0x3C[j], 0.0f, 0.0f, 0.0f);
        }
        for (j = 0; j < rec->count_0x08 + 1; j++) {
            setVector3(&rec->pos_0x18[j], 0.0f, 0.0f, 0.0f);
        }
    }
}

extern "C" void yure_player_init(YurePlayerWork* plw) {
    yure_player_init_kind(plw, 5);
    yure_player_init_kind(plw, 4);
}

/* 0x802ABF00 (0x2E8): builds a player's sway records of one set (5 the face set, records 0-3; 4 the hair set,
 * records 4-7) from the style row the player's parts select, naming each chain point by its model node. */
extern "C" void yure_player_init_kind(YurePlayerWork* plw, u8 kind) {
    nw4r::math::VEC3 scratch_vec;
    nw4r::math::MTX34 scratch_mtx;
    YureSrc* src;
    u32 j;
    const char** label;
    YureSrc* srcs;
    s32 i;
    s32 count;
    s32 start;
    YureRec* rec;
    u8 row;

    VEC3_ctor(&scratch_vec);
    MTX34_ctor(&scratch_mtx);
    nw4r::g3d::ResNode node(NULL);
    nw4r::g3d::ResNode parent_node(NULL);
    switch (kind) {
    case 5:
        srcs = yure_player_src5;
        rec = &plw->yure_0x674[0];
        memset(rec, 0, sizeof(YureRec) * 4);
        row = pl_rig_style_byte_get(plw->style_set_0x014, kind, pl_parts_value_get((_PLW*)plw, kind));
        start = yure_player_span5[row].start_0x01;
        count = yure_player_span5[row].count_0x00;
        break;
    case 4:
        srcs = yure_player_src4;
        rec = &plw->yure_0x674[4];
        memset(rec, 0, sizeof(YureRec) * 4);
        row = pl_rig_style_byte_get(plw->style_set_0x014, kind, pl_parts_value_get((_PLW*)plw, kind));
        start = yure_player_span4[row].start_0x01;
        count = yure_player_span4[row].count_0x00;
        break;
    default:
        return;
    }
    for (i = 0, src = &srcs[start], label = yure_label_tbl; i < count; src++, i++, rec++) {
        rec->active_0x00 = 1;
        rec->state_0x03 = 0;
        rec->is_enemy_0x06 = 0;
        rec->slot_0x05 = start + i;
        rec->shape_0x01 = src->shape_0x00;
        rec->count_0x08 = src->count_0x02;
        rec->pin_0x04 = src->pin_0x0C;
        rec->param_0x02 = src->param_0x01;
        for (j = 0; j < rec->count_0x08; j++, label++) {
            nw4r::g3d::ResNode found =
                ((nw4r::g3d::ResMdl*)&plw->rig_0x13C->models_0x004[kind].model.field_0x114)->GetResNode(*label);
            res_node_assign((ResHandle*)&node, (const ResHandle*)&found);
            if (res_node_is_valid((const ResHandle*)&node) != 0) {
                rec->joint_0x10[j] = res_node_get_id((const ResHandle*)&node);
            } else {
                rec->joint_0x10[j] = 0;
            }
            vec_to_mh_vec3(&rec->rest_0x54[j], &yure_rest_tbl[src->rest_0x08[j]]);
            setVector3(&rec->vel_0x3C[j], 0.0f, 0.0f, 0.0f);
        }
        for (j = 0; j < rec->count_0x08 + 1; j++) {
            setVector3(&rec->pos_0x18[j], 0.0f, 0.0f, 0.0f);
        }
        if (rec->joint_0x10[0] != 0) {
            mhchar_res_node_get(&plw->rig_0x13C->models_0x004[kind].model, &node, rec->joint_0x10[0]);
            u32 parent = res_node_get_parent((ResHandle*)&node);
            res_node_assign((ResHandle*)&parent_node, (const ResHandle*)&parent);
            rec->root_joint_0x0C = res_node_get_id((const ResHandle*)&parent_node);
        } else {
            rec->root_joint_0x0C = 0;
        }
    }
}

extern "C" void yure_player_clear_flags(YurePlayerWork* plw) {
    s32 i;

    for (i = 0; i < YURE_PLAYER_REC_MAX; i++) {
        if (plw->yure_0x674[i].active_0x00 != 0) {
            plw->yure_0x674[i].state_0x03 = 0;
        }
    }
}

void yure_move(void) {
    s32 i;
    s32 j;
    u16 player_max;
    YurePlayerWork* player;
    u16 enemy_max = get_move_work_max(3);
    YureEnemyWork* enemy = (YureEnemyWork*)get_move_work_adrs(3);

    for (i = 0; i < enemy_max; i++, enemy++) {
        if (enemy->active_0x000 != 0) {
            s32 count = yure_enemy_span[enemy->type_0x003].count_0x00;
            for (j = 0; j < count; j++) {
                if (enemy->yure_0x590[j].active_0x00 != 0) {
                    yure_step(enemy, 0, j, 0);
                }
            }
        }
    }
    player_max = get_move_work_max(2);
    player = (YurePlayerWork*)get_move_work_adrs(2);
    for (i = 0; i < player_max; i++, player++) {
        if (player->active_0x000 != 0) {
            for (j = 0; j < 4; j++) {
                if (player->yure_0x674[j].active_0x00 != 0) {
                    yure_step(player, 1, j, 5);
                }
            }
            for (j = 4; j < 8; j++) {
                if (player->yure_0x674[j].active_0x00 != 0) {
                    yure_step(player, 1, j, 4);
                }
            }
        }
    }
}

extern "C" void yure_player_move(YurePlayerWork* plw) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (plw->yure_0x674[i].active_0x00 != 0) {
            yure_step(plw, 1, i, 5);
        }
    }
    for (i = 4; i < 8; i++) {
        if (plw->yure_0x674[i].active_0x00 != 0) {
            yure_step(plw, 1, i, 4);
        }
    }
}

/* 0x802AC484 (0x12B4): steps one sway record.  The first step seats the chain on the owner's joints and measures it;
 * later steps integrate the springs (shape 1 the pendulum, 2 the free-swinging point, anything else the chain),
 * keep the points on the pin plane's side and, for an enemy, recompute the joint angle words. */
extern "C" void yure_step(void* owner /* untyped: caller-owned payload - a YureEnemyWork or a YurePlayerWork, by `is_player` */,
                          u8 is_player, s32 index, s32 kind) {
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 work;
    nw4r::math::VEC3 force[YURE_JOINT_MAX];
    nw4r::math::VEC3 seg[YURE_JOINT_MAX];
    nw4r::math::VEC3 point;
    nw4r::math::VEC3 node_scale;
    nw4r::math::VEC3 root_pos;
    nw4r::math::VEC3 unused_vec;
    nw4r::math::VEC3 joint_pos[YURE_JOINT_MAX];
    nw4r::math::VEC3 pin_dir;
    nw4r::math::VEC3 pin_ofs;
    HitSphere probe;
    PlPlane plane;
    _CP_VECTOR rot;
    nw4r::math::VEC3 node_rot;
    nw4r::math::MTX34 pin_mtx;
    nw4r::math::MTX34 root_mtx;
    nw4r::math::MTX34 joint_mtx[YURE_JOINT_MAX];
    f32 length[YURE_JOINT_MAX];
    nw4r::math::VEC3 t_len_em;
    nw4r::math::VEC3 t_len_pl;
    nw4r::math::VEC3 t_tip_ofs;
    nw4r::math::VEC3 t_tip;
    nw4r::math::VEC3 t_pin0_rel;
    nw4r::math::VEC3 t_pin0_push;
    nw4r::math::VEC3 t1_pull;
    nw4r::math::VEC3 t1_rest;
    nw4r::math::VEC3 t1_tip;
    nw4r::math::VEC3 t1_mid;
    nw4r::math::VEC3 t1_mid_sum;
    nw4r::math::VEC3 t1_mid_ofs;
    nw4r::math::VEC3 t1_spring;
    nw4r::math::VEC3 t1_spring_d;
    nw4r::math::VEC3 t1_damp;
    nw4r::math::VEC3 t1_angle;
    nw4r::math::VEC3 t2_vel;
    nw4r::math::VEC3 t2_damp;
    nw4r::math::VEC3 t2_spring;
    nw4r::math::VEC3 t2_spring_d;
    nw4r::math::VEC3 t2_pull;
    nw4r::math::VEC3 t3_stiff;
    nw4r::math::VEC3 t3_grav;
    nw4r::math::VEC3 t3_link;
    nw4r::math::VEC3 t3_spring;
    nw4r::math::VEC3 t3_vel;
    nw4r::math::VEC3 t3_damp;
    nw4r::math::VEC3 t3_follow;
    nw4r::math::VEC3 t3_seg_d;
    nw4r::math::VEC3 t3_seg;
    nw4r::math::VEC3 t3_tip;
    nw4r::math::VEC3 t_pin1_rel;
    nw4r::math::VEC3 t_pin1_push;
    YureEnemyWork* enemy;
    YurePlayerWork* plw;
    YureRec* rec;
    YureSrc* srcs;
    YureParam* params;
    u32 j;
    s32 i;
    f32 scale;
    f32 damp;
    f32 spring;
    f32 stiff;
    f32 gravity;
    f32 seat_scale;
    f32 dist;
    f32 depth;

    VEC3_ctor(&dir);
    VEC3_ctor(&work);
    VEC3_ctor(&force[0]);
    VEC3_ctor(&force[1]);
    VEC3_ctor(&seg[0]);
    VEC3_ctor(&seg[1]);
    VEC3_ctor(&point);
    VEC3_ctor(&node_scale);
    VEC3_ctor(&root_pos);
    VEC3_ctor(&unused_vec);
    VEC3_ctor(&joint_pos[0]);
    VEC3_ctor(&joint_pos[1]);
    VEC3_ctor(&pin_dir);
    VEC3_ctor(&pin_ofs);
    eft_vec_ctor(&probe.pos);
    vec3_ctor_return(&plane.normal);
    VEC3_ctor(&node_rot);
    MTX34_ctor(&pin_mtx);
    MTX34_ctor(&root_mtx);
    MTX34_ctor(&joint_mtx[0]);
    MTX34_ctor(&joint_mtx[1]);
    nw4r::g3d::ResNode node(NULL);

    if (is_player == 0) {
        enemy = (YureEnemyWork*)owner;
        plw = NULL;
        rec = &enemy->yure_0x590[index];
        params = yure_enemy_param;
        srcs = yure_enemy_src;
    } else {
        plw = (YurePlayerWork*)owner;
        enemy = NULL;
        rec = &plw->yure_0x674[index];
        if (pl_eft_draw_ck((_PLW*)plw) == 0) {
            rec->state_0x03 = 0;
            return;
        }
        if ((u8)event_demo_running_ck() != 0 || event_demo_ck() != 0) {
            rec->state_0x03 = 0;
            return;
        }
        for (i = 0; i < 7; i++) {
            if (plw->rig_0x13C->models_0x004[i].model.field_0x34 == 0 || plw->rig_0x13C->field_0x9C0[i] == 0) {
                rec->state_0x03 = 0;
                return;
            }
        }
        switch (kind) {
        case 5:
            params = yure_player_param5;
            srcs = yure_player_src5;
            break;
        case 4:
            params = yure_player_param4;
            srcs = yure_player_src4;
            break;
        }
    }
    if (rec->count_0x08 == 0) {
        return;
    }
    switch (rec->state_0x03) {
    case 0:
        rec->state_0x03++;
        if (is_player == 0) {
            get_joint_wmat_em((_ENEMY_WORK*)enemy, srcs[rec->slot_0x05].matrix_joint_0x03, &root_mtx);
            if (rec->shape_0x01 == 2) {
                mtx34_trans_get(&root_mtx, &root_pos);
                for (j = 0; j < rec->count_0x08; j++) {
                    get_joint_wmat_em((_ENEMY_WORK*)enemy, srcs[rec->slot_0x05].matrix_joint_0x03, &joint_mtx[j]);
                    mtx34_trans_get(&joint_mtx[j], &joint_pos[j]);
                }
            } else {
                get_joint_wpos_em((_ENEMY_WORK*)enemy, rec->root_joint_0x0C, &root_pos);
                for (j = 0; j < rec->count_0x08; j++) {
                    get_joint_wmat_em((_ENEMY_WORK*)enemy, rec->joint_0x10[j], &joint_mtx[j]);
                    mtx34_trans_get(&joint_mtx[j], &joint_pos[j]);
                }
            }
            switch (rec->count_0x08) {
            case 1:
                rec->length_0x88[0] = 20.0f;
                break;
            case 2:
                subVec3(&t_len_em, &joint_pos[0], &joint_pos[1]);
                copyVec3(&work, &t_len_em);
                rec->length_0x88[0] = rec->length_0x88[1] = vec3_len(&work.x);
                break;
            }
            seat_scale = get_em_scale((_ENEMY_WORK*)enemy);
        } else {
            mhchar_joint_mtx_get(&plw->rig_0x13C->models_0x004[0].model, srcs[rec->slot_0x05].matrix_joint_0x03,
                                 &root_mtx);
            plw->rig_0x13C->models_0x004[0].model.get_joint_wpos(rec->root_joint_0x0C, &root_pos);
            for (j = 0; j < rec->count_0x08; j++) {
                mhchar_res_node_get(&plw->rig_0x13C->models_0x004[kind].model, &node, rec->joint_0x10[j]);
                copyVec3(&node_rot, (const nw4r::math::VEC3*)em_res_node_rot_get(&node));
                rot.x = (u16)(0.5f + (65536.0f * node_rot.x) / 360.0f);
                rot.y = (u16)(0.5f + (65536.0f * node_rot.y) / 360.0f);
                rot.z = (u16)(0.5f + (65536.0f * node_rot.z) / 360.0f);
                cpSetRotMatrix(&rot, &joint_mtx[j]);
                copyVec3(&node_scale, (const nw4r::math::VEC3*)em_res_node_scale_get(&node));
                scaleMat34W(&joint_mtx[j], &node_scale);
                copyVec3(&point, (const nw4r::math::VEC3*)em_res_node_trans_get(&node));
                mtx34_set_trans(&joint_mtx[j], &point);
                if (j == 0) {
                    mtx34_concat(&joint_mtx[j], &root_mtx, &joint_mtx[j]);
                } else {
                    mtx34_concat(&joint_mtx[j], &joint_mtx[j - 1], &joint_mtx[j]);
                }
                mtx34_trans_get(&joint_mtx[j], &joint_pos[j]);
            }
            switch (rec->count_0x08) {
            case 1:
                rec->length_0x88[0] = 20.0f;
                break;
            case 2:
                subVec3(&t_len_pl, &joint_pos[0], &joint_pos[1]);
                copyVec3(&work, &t_len_pl);
                rec->length_0x88[0] = rec->length_0x88[1] = vec3_len(&work.x);
                break;
            }
            seat_scale = 1.0f;
        }
        copyVec3(&rec->anchor_0x6C, &root_pos);
        for (j = 0; j < rec->count_0x08; j++) {
            copyVec3(&rec->pos_0x18[j], &joint_pos[j]);
            setVector3(&rec->vel_0x3C[j], 0.0f, 0.0f, 0.0f);
        }
        copyVec3(&dir, &rec->rest_0x54[j - 1]);
        mulVecMat(&dir, &joint_mtx[j - 1]);
        vec3_scale(&t_tip_ofs, &dir,
                   seat_scale * (rec->length_0x88[j - 1] + params[rec->param_0x02].coeff_0x00[j - 1]));
        copyVec3(&point, &t_tip_ofs);
        addVec3(&t_tip, &rec->pos_0x18[j - 1], &point);
        copyVec3(&rec->pos_0x18[j], &t_tip);
        if (rec->pin_0x04 != 0) {
            if (is_player == 1) {
                mhchar_joint_mtx_get(&plw->rig_0x13C->models_0x004[0].model, yure_pin_joint_tbl[rec->pin_0x04],
                                     &pin_mtx);
                plw->rig_0x13C->models_0x004[0].model.get_joint_wpos(rec->root_joint_0x0C, &point);
            } else {
                get_joint_wmat_em((_ENEMY_WORK*)enemy, srcs[rec->slot_0x05].matrix_joint_0x03, &pin_mtx);
                get_joint_wpos_em((_ENEMY_WORK*)enemy, yure_pin_joint_tbl[rec->pin_0x04], &point);
            }
            vec_to_mh_vec3(&pin_dir, &yure_pin_tbl[rec->pin_0x04].dir_0x00);
            mtx34_rotate_vec3(&dir, &pin_mtx, &pin_dir);
            vec3_normalize_into(&plane.normal, &dir);
            vec_to_mh_vec3(&pin_ofs, &yure_pin_tbl[rec->pin_0x04].offset_0x0C);
            mulVecMat(&pin_ofs, &pin_mtx);
            addVec3To(&point, &pin_ofs);
            plane.dist = vec3_dot(&point.x, &plane.normal.x);
            for (j = 0; j < rec->count_0x08; j++) {
                nw4r::math::VEC3* tip = &rec->pos_0x18[j + 1];

                copyVec3(&probe.pos, tip);
                probe.radius = seat_scale * params[rec->param_0x02].coeff_0x00[4];
                pl_plane_project(&probe.pos, &plane, &point);
                subVec3(&t_pin0_rel, &probe.pos, &point);
                copyVec3(&work, &t_pin0_rel);
                depth = vec3_dot(&plane.normal.x, &work.x);
                if (depth < probe.radius) {
                    vec3_scale(&t_pin0_push, &plane.normal, probe.radius - depth);
                    copyVec3(&work, &t_pin0_push);
                    addVec3To(tip, &work);
                }
            }
        }
        break;
    case 1:
        if (is_player == 0) {
            get_joint_wmat_em((_ENEMY_WORK*)enemy, srcs[rec->slot_0x05].matrix_joint_0x03, &root_mtx);
            mtx34_trans_get(&root_mtx, &root_pos);
            for (j = 0; j < rec->count_0x08; j++) {
                get_joint_wpos_em((_ENEMY_WORK*)enemy, rec->joint_0x10[j], &joint_pos[j]);
                length[j] = rec->length_0x88[j] + params[rec->param_0x02].coeff_0x00[j];
            }
            mtx34_copy(&pin_mtx, &root_mtx);
            scale = get_em_scale((_ENEMY_WORK*)enemy);
            if ((s32)rec->shape_0x01 == 2) {
                if (enemy->sway_mode_0x1E2 == 2) {
                    YureParam* param = &params[rec->param_0x02];
                    spring = param->coeff_0x00[7];
                    damp = param->coeff_0x00[3];
                    stiff = param->coeff_0x00[5];
                    gravity = 0.0f;
                } else {
                    YureParam* param = &params[rec->param_0x02];
                    spring = param->coeff_0x00[2];
                    damp = param->coeff_0x00[3];
                    stiff = param->coeff_0x00[5];
                    gravity = 0.0f;
                }
            } else if (enemy->sway_mode_0x1E2 == 2) {
                YureParam* param = &params[rec->param_0x02];
                spring = param->coeff_0x00[2];
                damp = param->coeff_0x00[3];
                stiff = param->coeff_0x00[5];
                gravity = param->coeff_0x00[6];
            } else {
                YureParam* param = &params[rec->param_0x02];
                spring = param->coeff_0x00[2];
                damp = param->coeff_0x00[3];
                stiff = param->coeff_0x00[5];
                gravity = param->coeff_0x00[7];
            }
        } else {
            mhchar_joint_mtx_get(&plw->rig_0x13C->models_0x004[0].model, srcs[rec->slot_0x05].matrix_joint_0x03,
                                 &root_mtx);
            plw->rig_0x13C->models_0x004[0].model.get_joint_wpos(rec->root_joint_0x0C, &root_pos);
            for (j = 0; j < rec->count_0x08; j++) {
                plw->rig_0x13C->models_0x004[kind].model.get_joint_wpos(rec->joint_0x10[j], &joint_pos[j]);
                length[j] = rec->length_0x88[j] + params[rec->param_0x02].coeff_0x00[j];
            }
            scale = 1.0f;
            YureParam* param = &params[rec->param_0x02];
            spring = param->coeff_0x00[2];
            if (plw->kind_0x009 == 3) {
                damp = 2.0f * param->coeff_0x00[3];
                stiff = 0.5f * param->coeff_0x00[5];
                gravity = param->coeff_0x00[7];
            } else {
                damp = param->coeff_0x00[3];
                stiff = param->coeff_0x00[5];
                gravity = param->coeff_0x00[6];
            }
        }
        copyVec3(&rec->pos_0x18[0], &joint_pos[0]);
        switch (rec->shape_0x01) {
        case 1:
            addVec3To(&rec->pos_0x18[1], &rec->vel_0x3C[0]);
            subVec3(&t1_pull, &rec->anchor_0x6C, &rec->pos_0x18[0]);
            copyVec3(&work, &t1_pull);
            vec3_scale_in_place(&work, params[rec->param_0x02].coeff_0x00[6]);
            addVec3To(&rec->vel_0x3C[0], &work);
            copyVec3(&dir, &rec->rest_0x54[0]);
            mulVecMat(&dir, &root_mtx);
            vec3_scale(&t1_rest, &dir, length[0] * scale);
            copyVec3(&point, &t1_rest);
            addVec3(&t1_tip, &rec->pos_0x18[0], &point);
            copyVec3(&rec->pos_0x18[2], &t1_tip);
            vec3_clamp_distance(&rec->pos_0x18[2], &rec->pos_0x18[1], scale * params[rec->param_0x02].coeff_0x00[1]);
            subVec3(&t1_mid, &rec->pos_0x18[1], &rec->pos_0x18[0]);
            copyVec3(&work, &t1_mid);
            if (vec3_length_sq(&work.x) > 0.001f) {
                vec3_normalize_into(&work, &work);
            }
            vec3_scale(&t1_mid_ofs, &work, length[0]);
            addVec3(&t1_mid_sum, &rec->pos_0x18[0], &t1_mid_ofs);
            copyVec3(&rec->pos_0x18[1], &t1_mid_sum);
            subVec3(&t1_spring_d, &rec->pos_0x18[2], &rec->pos_0x18[1]);
            vec3_scale(&t1_spring, &t1_spring_d, spring);
            addVec3To(&rec->vel_0x3C[0], &t1_spring);
            vec3_scale(&t1_damp, &rec->vel_0x3C[0], 1.0f - damp);
            copyVec3(&rec->vel_0x3C[0], &t1_damp);
            subVec3(&t1_angle, &rec->pos_0x18[0], &rec->pos_0x18[1]);
            copyVec3(&work, &t1_angle);
            mtx34_inverse(&pin_mtx, &root_mtx);
            mulVecMat(&work, &pin_mtx);
            rec->pitches_0x78[0] = (u16)(0.5f + (65536.0f * atan2f(work.z, work.y)) / 6.2831855f);
            rec->pitches_0x78[0] = rec->pitches_0x78[0] - 0x8000 + params[rec->param_0x02].angle_offset_0x20;
            rec->pitches_0x78[0] = (u16)rec->pitches_0x78[0];
            rec->rolls_0x80[0] = 0;
            copyVec3(&rec->anchor_0x6C, &root_pos);
            break;
        case 2:
            copyVec3(&rec->pos_0x18[0], &root_pos);
            addVec3To(&rec->pos_0x18[1], &rec->vel_0x3C[0]);
            vec3_clamp_distance(&rec->pos_0x18[0], &rec->pos_0x18[1], scale * params[rec->param_0x02].coeff_0x00[1]);
            subVec3(&t2_spring_d, &rec->pos_0x18[0], &rec->pos_0x18[1]);
            vec3_scale(&t2_spring, &t2_spring_d, spring);
            vec3_scale(&t2_damp, &rec->vel_0x3C[0], 1.0f - damp);
            addVec3(&t2_vel, &t2_damp, &t2_spring);
            copyVec3(&rec->vel_0x3C[0], &t2_vel);
            subVec3(&t2_pull, &rec->anchor_0x6C, &rec->pos_0x18[0]);
            copyVec3(&work, &t2_pull);
            vec3_scale_in_place(&work, 0.1f);
            if (vec3_len(&work.x) > 50.0f) {
                vec3_normalize_into(&work, &work);
                vec3_scale_in_place(&work, 50.0f);
            }
            addVec3To(&rec->vel_0x3C[0], &work);
            copyVec3(&rec->anchor_0x6C, &rec->pos_0x18[0]);
            break;
        default:
            for (j = 0; j < rec->count_0x08; j++) {
                nw4r::math::VEC3* tip = &rec->pos_0x18[j + 1];

                addVec3To(tip, &rec->vel_0x3C[j]);
                mtx34_rotate_vec3(&dir, &root_mtx, &rec->rest_0x54[j]);
                vec3_scale(&t3_stiff, &dir, stiff);
                copyVec3(&force[j], &t3_stiff);
                vec3_scale(&t3_grav, &yure_world.gravity_0x00, gravity);
                addVec3To(&force[j], &t3_grav);
                vec3_clamp_distance(&rec->pos_0x18[j], tip, scale * (10.0f + length[j]));
                subVec3(&t3_link, &rec->pos_0x18[j], tip);
                copyVec3(&work, &t3_link);
                dist = vec3_len(&work.x);
                if (dist > 0.001f) {
                    vec3_normalize_into(&work, &work);
                }
                vec3_scale(&t3_spring, &work, spring * (dist - scale * length[j]));
                addVec3To(&force[j], &t3_spring);
                vec3_scale(&t3_damp, &rec->vel_0x3C[j], 1.0f - damp);
                addVec3(&t3_vel, &t3_damp, &force[j]);
                copyVec3(&rec->vel_0x3C[j], &t3_vel);
                if (j == 0 && is_player == 1) {
                    subVec3(&t3_follow, &root_pos, &rec->anchor_0x6C);
                    addVec3To(&rec->pos_0x18[j], &t3_follow);
                }
                subVec3(&t3_seg_d, tip, &rec->pos_0x18[j]);
                copyVec3(&work, &t3_seg_d);
                if (vec3_len(&work.x) > 0.001f) {
                    vec3_normalize_into(&work, &work);
                }
                vec3_scale(&t3_seg, &work, scale * length[j]);
                copyVec3(&seg[j], &t3_seg);
                addVec3(&t3_tip, &rec->pos_0x18[j], &seg[j]);
                copyVec3(tip, &t3_tip);
            }
            if (rec->pin_0x04 != 0) {
                if (is_player == 1) {
                    mhchar_joint_mtx_get(&plw->rig_0x13C->models_0x004[0].model, yure_pin_joint_tbl[rec->pin_0x04],
                                         &pin_mtx);
                    plw->rig_0x13C->models_0x004[0].model.get_joint_wpos(rec->root_joint_0x0C, &point);
                } else {
                    get_joint_wmat_em((_ENEMY_WORK*)enemy, srcs[rec->slot_0x05].matrix_joint_0x03, &pin_mtx);
                    get_joint_wpos_em((_ENEMY_WORK*)enemy, yure_pin_joint_tbl[rec->pin_0x04], &point);
                }
                vec_to_mh_vec3(&pin_dir, &yure_pin_tbl[rec->pin_0x04].dir_0x00);
                mtx34_rotate_vec3(&dir, &pin_mtx, &pin_dir);
                vec3_normalize_into(&plane.normal, &dir);
                vec_to_mh_vec3(&pin_ofs, &yure_pin_tbl[rec->pin_0x04].offset_0x0C);
                mulVecMat(&pin_ofs, &pin_mtx);
                addVec3To(&point, &pin_ofs);
                plane.dist = vec3_dot(&point.x, &plane.normal.x);
                for (j = 0; j < rec->count_0x08; j++) {
                    nw4r::math::VEC3* tip = &rec->pos_0x18[j + 1];

                    copyVec3(&probe.pos, tip);
                    probe.radius = scale * params[rec->param_0x02].coeff_0x00[4];
                    pl_plane_project(&probe.pos, &plane, &point);
                    subVec3(&t_pin1_rel, &probe.pos, &point);
                    copyVec3(&work, &t_pin1_rel);
                    depth = vec3_dot(&plane.normal.x, &work.x);
                    if (depth < probe.radius) {
                        vec3_scale(&t_pin1_push, &plane.normal, probe.radius - depth);
                        copyVec3(&work, &t_pin1_push);
                        addVec3To(tip, &work);
                    }
                }
            }
            copyVec3(&rec->anchor_0x6C, &root_pos);
            for (j = 0; j < rec->count_0x08; j++) {
                if (is_player == 0) {
                    if ((s32)rec->shape_0x01 != 3) {
                        yure_angles_calc(rec, &root_mtx, &rec->pitches_0x78[j], &rec->rolls_0x80[j], j);
                    } else {
                        yure_roll_calc(rec, &root_mtx, &rec->rolls_0x80[j], j);
                        rec->rolls_0x80[j] += params[rec->param_0x02].angle_offset_0x20;
                    }
                }
            }
            break;
        }
        break;
    }
}

extern "C" void yure_joint_apply(YureRec* rec, MtxHolder* joint) {
    nw4r::math::MTX34 rot;
    nw4r::math::MTX34 joint_mtx;

    MTX34_ctor(&rot);
    MTX34_ctor(&joint_mtx);
    if (rec->state_0x03 >= 1) {
        joint_mtx_load(joint, &joint_mtx);
        mtx34_identity(&rot);
        rotMatrixX(rec->pitch_0x78, &rot);
        rotMatrixZ(rec->roll_0x80, &rot);
        mtx34_concat_assign(&joint_mtx, &rot);
        joint_mtx_store(joint, &joint_mtx);
    }
}

extern "C" void yure_angles_calc(YureRec* rec, nw4r::math::MTX34* mtx, u32* pitch, u32* roll, u32 index) {
    nw4r::math::VEC3 dir;
    nw4r::math::MTX34 inv;
    nw4r::math::VEC3 delta;

    VEC3_ctor(&dir);
    MTX34_ctor(&inv);
    mtx34_inverse(&inv, mtx);
    subVec3(&delta, &rec->pos_0x18[index], &rec->pos_0x18[index + 1]);
    copyVec3(&dir, &delta);
    mtx34_rotate_vec3(&dir, &inv, &dir);
    *pitch = (u16)(0.5f + (65536.0f * atan2f(dir.z, dir.y)) / 6.2831855f);
    f32 ysq = dir.y * dir.y;
    f32 zsq = dir.z * dir.z;
    f32 len = sqrt_f32(ysq + zsq);
    *roll = (u16)(0.5f + (65536.0f * atan2f(-dir.x, len)) / 6.2831855f);
}

extern "C" void yure_roll_calc(YureRec* rec, nw4r::math::MTX34* mtx, u32* out, u32 index) {
    nw4r::math::VEC3 dir;
    nw4r::math::MTX34 inv;
    nw4r::math::VEC3 delta;

    VEC3_ctor(&dir);
    MTX34_ctor(&inv);
    mtx34_inverse(&inv, mtx);
    subVec3(&delta, &rec->pos_0x18[index], &rec->pos_0x18[index + 1]);
    copyVec3(&dir, &delta);
    mtx34_rotate_vec3(&dir, &inv, &dir);
    *out = vec3_angle_xy(&dir.x);
}

extern "C" void yure_static_init(void) {
    setVec3(&yure_world.gravity_0x00, 0.0f, -1.0f, 0.0f);
}

/* The `.ctors` word (0x8056F378) the split assigns to this unit - it points at the static initialiser. */
__declspec(section ".ctors") void* const yure_ctor = (void*)yure_static_init;

YureWorld yure_world;

/* The sway tables (`.data` 0x805CE638-0x805CED40).  Names are from each table's use in this unit;
 * the four spring/pin tables' fields are the offsets the unwritten `yure_step` reads. */
char yure_label_0[] = "CL-00";
char yure_label_1[] = "CL-01";
char yure_label_2[] = "CL-02";
char yure_label_3[8] = "CL-03";
const char* yure_label_tbl[4] = {yure_label_0, yure_label_1, yure_label_2, yure_label_3};
Vec yure_rest_tbl[9] = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {0.0f, -0.9f, -0.1f},
    {-0.1f, -0.4f, -0.2f},
    {0.1f, -0.4f, -0.2f},
    {1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
};
YurePin yure_pin_tbl[13] = {
    {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{-0.2f, 0.1f, -0.9f}, {0.0f, 0.0f, -15.0f}},
    {{0.0f, 0.2f, -1.0f}, {0.0f, -40.0f, -15.0f}},
    {{0.2f, 0.5f, -0.5f}, {0.0f, 10.0f, 0.0f}},
    {{0.0f, -0.7f, -0.3f}, {0.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    {{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -10.0f}},
    {{0.0f, -0.7f, -0.3f}, {0.0f, 0.0f, 0.0f}},
};
YureSpan yure_enemy_span[42] = {
    {0, 0},
    {0, 0},
    {0, 0},
    {2, 3},
    {0, 0},
    {0, 0},
    {0, 0},
    {3, 8},
    {3, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {3, 5},
    {0, 0},
    {0, 0},
    {0, 0},
    {2, 11},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
};
YureSpan yure_player_span5[6] = {
    {0, 0},
    {1, 0},
    {2, 1},
    {1, 3},
    {1, 4},
    {0, 0},
};
YureSpan yure_player_span4[8] = {
    {0, 0},
    {2, 0},
    {2, 2},
    {2, 4},
    {1, 6},
    {1, 7},
    {1, 8},
    {1, 9},
};
YureSrc yure_enemy_src[13] = {
    {0, 1, 1, 0x03, 0x17, {0x18, 0xFF}, 0, {1, 0}, 5},
    {1, 2, 1, 0x03, 0x21, {0x22, 0xFF}, 0, {1, 0}, 0},
    {1, 3, 1, 0x03, 0x19, {0x1A, 0xFF}, 0, {1, 0}, 0},
    {1, 4, 1, 0x04, 0x04, {0x13, 0xFF}, 0, {7, 0}, 0},
    {1, 5, 1, 0x18, 0x18, {0x19, 0xFF}, 0, {7, 0}, 0},
    {2, 6, 1, 0x0F, 0x0F, {0x10, 0xFF}, 0, {0, 0}, 0},
    {2, 7, 1, 0x11, 0x11, {0x12, 0xFF}, 0, {0, 0}, 0},
    {2, 8, 1, 0x13, 0x13, {0x14, 0xFF}, 0, {0, 0}, 0},
    {0, 9, 1, 0x03, 0x03, {0x11, 0xFF}, 0, {1, 0}, 13},
    {1, 10, 1, 0x03, 0x25, {0x26, 0xFF}, 0, {1, 0}, 0},
    {1, 11, 1, 0x03, 0x1D, {0x1E, 0xFF}, 0, {1, 0}, 0},
    {3, 12, 1, 0x06, 0x06, {0x07, 0xFF}, 0, {5, 0}, 8},
    {3, 13, 1, 0x0B, 0x0B, {0x0C, 0xFF}, 0, {6, 0}, 9},
};
YureSrc yure_player_src5[5] = {
    {0, 0, 2, 0x0D, 0x02, {0x03, 0x04}, 0, {2, 2}, 2},
    {0, 1, 2, 0x0D, 0x02, {0x03, 0x04}, 0, {1, 1}, 4},
    {0, 1, 2, 0x0D, 0x02, {0x03, 0x04}, 0, {1, 1}, 4},
    {0, 1, 1, 0x0D, 0x02, {0x03, 0x04}, 0, {2, 2}, 2},
    {0, 2, 2, 0x0D, 0x02, {0x03, 0x04}, 0, {2, 2}, 0},
};
YureSrc yure_player_src4[10] = {
    {0, 0, 2, 0x0D, 0x02, {0x04, 0x05}, 0, {3, 3}, 3},
    {0, 0, 2, 0x0D, 0x02, {0x07, 0x08}, 0, {4, 4}, 3},
    {0, 0, 2, 0x0D, 0x02, {0x03, 0x04}, 0, {1, 1}, 11},
    {0, 0, 2, 0x0D, 0x02, {0x05, 0x06}, 0, {1, 1}, 3},
    {0, 1, 1, 0x0D, 0x02, {0x03, 0xFF}, 0, {1, 0}, 0},
    {0, 1, 1, 0x0D, 0x02, {0x04, 0xFF}, 0, {1, 0}, 0},
    {1, 3, 1, 0x0D, 0x02, {0x03, 0xFF}, 0, {7, 0}, 0},
    {1, 4, 1, 0x0D, 0x02, {0x03, 0xFF}, 0, {7, 0}, 0},
    {0, 2, 1, 0x0D, 0x02, {0x03, 0xFF}, 0, {1, 0}, 0},
    {0, 0, 2, 0x0D, 0x02, {0x04, 0x05}, 0, {1, 1}, 3},
};
YureParam yure_enemy_param[14] = {
    {{20.0f, 0.0f, 1.3f, 0.03f, 0.1f, 2.0f, 1.5f, 0.0f}, 0},
    {{20.0f, 0.0f, 0.7f, 0.05f, 30.0f, 3.0f, 1.0f, 0.0f}, 0},
    {{30.0f, 200.0f, 0.3f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f}, 0x8000},
    {{30.0f, 200.0f, 0.3f, 0.3f, 0.0f, 1.0f, 0.0f, 0.0f}, 0x8000},
    {{30.0f, 50.0f, 0.25f, 0.8f, 0.1f, 1.0f, 0.0f, 0.0f}, 0},
    {{30.0f, 50.0f, 0.3f, 0.5f, 0.1f, 1.0f, 0.0f, 0.0f}, 0},
    {{30.0f, 300.0f, 0.3f, 0.5f, 0.1f, 1.0f, 0.0f, 0.35f}, 0},
    {{30.0f, 300.0f, 0.15f, 0.5f, 0.1f, 1.0f, 0.0f, 0.15f}, 0},
    {{30.0f, 300.0f, 0.5f, 0.7f, 0.1f, 1.0f, 0.0f, 0.45f}, 0},
    {{20.0f, 0.0f, 3.0f, 0.15f, 30.0f, 3.0f, 1.0f, 0.0f}, 0},
    {{20.0f, 200.0f, 0.3f, 0.2f, 0.0f, 1.0f, 0.0f, 0.0f}, 0x8000},
    {{20.0f, 200.0f, 0.3f, 0.2f, 0.0f, 1.0f, 0.0f, 0.0f}, 0x8000},
    {{20.0f, 0.0f, 0.7f, 0.3f, 210.0f, 2.0f, 0.0f, 0.2f}, 0xC000},
    {{20.0f, 0.0f, 0.7f, 0.3f, 210.0f, 2.0f, 0.0f, 0.2f}, 0x4000},
};
YureParam yure_player_param5[4] = {
    {{13.0f, 13.0f, 1.0f, 0.15f, 0.15f, 2.0f, 1.0f, 0.0f}, 0},
    {{0.0f, 0.0f, 0.7f, 0.15f, 0.1f, 1.0f, 1.0f, 0.0f}, 0},
    {{0.0f, 0.0f, 0.5f, 0.15f, 0.1f, 1.0f, 2.0f, 0.0f}, 0},
    {{30.0f, 50.0f, 0.25f, 0.8f, 0.1f, 1.0f, 0.0f, 0.0f}, 0},
};
YureParam yure_player_param4[5] = {
    {{0.0f, 0.0f, 0.8f, 0.2f, 5.0f, 3.0f, 1.0f, 0.0f}, 0},
    {{0.0f, 0.0f, 2.5f, 0.2f, 7.0f, 3.0f, 1.0f, 0.0f}, 0},
    {{6.0f, 0.0f, 0.6f, 0.1f, 5.0f, 3.0f, 1.0f, 0.0f}, 0},
    {{10.0f, 30.0f, 0.3f, 0.5f, 0.1f, 1.0f, 0.0f, 0.0f}, 0},
    {{10.0f, 15.0f, 0.45f, 0.3f, 0.1f, 1.0f, 0.0f, 0.0f}, 0},
};
u8 yure_pin_joint_tbl[20] = {
    0x00, 0x11, 0x03, 0x0D, 0x0C, 0x18, 0x1A, 0x22, 0x06, 0x0B, 0x03, 0x03, 0x03, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
