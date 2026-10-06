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
 *   exported entry point).
 * RESIDUALS. `yure_player_init_kind` (0x802ABF00, 744 B, the player records' builder, which reads the model's
 *   `_PLW_PHYSICS` table through the g3d name-handle helpers) and `yure_step` (0x802AC484, 4788 B, the spring
 *   integrator) are unwritten; until they are, the object lacks their constants and relocations.  `yure_enemy_init`
 *   and `yure_move` differ in register numbers (the target keeps `yure_move`'s span table base in r31).
 *  - flipcheck: `.text` 0x6FC, `.sdata2` 0x14, extab 0x38 and extabindex 0x54 against the claims 0x1C98, 0x38, 0x48
 *    and 0x6C; `.data` and `.sdata` are byte-equal to the target.
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
    src = &yure_enemy_src[start];
    for (i = 0; i < count; src++, i++, rec++) {
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

extern "C" void yure_player_clear_flags(YurePlayerWork* plw) {
    s32 i;

    for (i = 0; i < YURE_PLAYER_REC_MAX; i++) {
        if (plw->yure_0x674[i].active_0x00 != 0) {
            plw->yure_0x674[i].state_0x03 = 0;
        }
    }
}

void yure_move(void) {
    u16 enemy_max = get_move_work_max(3);
    YureEnemyWork* enemy = (YureEnemyWork*)get_move_work_adrs(3);
    s32 i;
    s32 j;
    u16 player_max;
    YurePlayerWork* player;

    for (i = 0; i < enemy_max; i++, enemy++) {
        if (enemy->active_0x000 != 0) {
            s8 count = yure_enemy_span[enemy->type_0x003].count_0x00;
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

extern "C" void yure_roll_calc(YureRec* rec, nw4r::math::MTX34* mtx, s32* out, u32 index) {
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
