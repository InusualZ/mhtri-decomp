/* The equipment sway unit `Pl/pl_yure.cpp` - its sway record and the entry points the player, enemy and
 * arena units call (docs/plan.md 6.5 rules 1-5).
 *
 * `.text` 0x802ABD28-0x802AD9C0.  The unit's notes, extent and residuals live in the source file's header
 * comment.
 */
#ifndef MHTRI_PL_PL_YURE_H
#define MHTRI_PL_PL_YURE_H

#include "types.h"
#include "nw4r/math.h"
#include "Pl/yure_joint_apply.h"

#ifdef __cplusplus

/* The number of joints one sway record chains (the record keeps `count_0x08` of them, at most this). */
#define YURE_JOINT_MAX 2

/* One sway (spring) record: a short chain of joints whose positions are integrated each frame so a
 * hanging piece of equipment lags behind the body.  Three live in an enemy work record (+0x590), eight in
 * a player work record (+0x674).
 * size: 0x90 */
struct YureRec {
    /* +0x00 */ u8 active_0x00;            /* nonzero while the record is in use */
    /* +0x01 */ u8 shape_0x01;             /* 1 and 2 pick the two spring models, anything else the chain model */
    /* +0x02 */ u8 param_0x02;             /* the row of the spring constant table */
    /* +0x03 */ u8 state_0x03;             /* 0 until the first step has seated the chain */
    /* +0x04 */ u8 pin_0x04;               /* the pin-joint row of the collision table, 0 for none */
    /* +0x05 */ u8 slot_0x05;              /* this record's row in the source table */
    /* +0x06 */ u8 is_enemy_0x06;         /* 1 for an enemy record, 0 for a player one */
    /* +0x07 */ u8 pad_0x07;
    /* +0x08 */ u32 count_0x08;            /* the chain length */
    /* +0x0C */ s32 root_joint_0x0C;       /* the joint the chain hangs from */
    /* +0x10 */ s32 joint_0x10[YURE_JOINT_MAX];   /* the joint each chain point follows */
    /* +0x18 */ nw4r::math::VEC3 pos_0x18[YURE_JOINT_MAX + 1];   /* the chain's current points */
    /* +0x3C */ nw4r::math::VEC3 vel_0x3C[YURE_JOINT_MAX];       /* each point's velocity */
    /* +0x54 */ nw4r::math::VEC3 rest_0x54[YURE_JOINT_MAX];      /* each point's rest offset from its joint */
    /* +0x6C */ nw4r::math::VEC3 anchor_0x6C;                    /* the root's previous world position */
    /* +0x78 */ u32 pitch_0x78;            /* the angle words `yure_joint_apply` hands `rotMatrixX`/`rotMatrixZ` */
    /* +0x7C */ u32 pad_0x7C;
    /* +0x80 */ u32 roll_0x80;
    /* +0x84 */ f32 length_0x84[YURE_JOINT_MAX + 1];             /* each segment's length */
};

/* The part of the enemy work record (`get_move_work_adrs(3)`, stride 0xB18) the sway unit reads: the
 * record's type byte and the three sway records.  `include/enemy/ENEMY_WORK.h`'s `_ENEMY_WORK` carries
 * the same bytes (`field_0x608`/`field_0x610` are sway record 0's angle words).
 * size: 0xB18 */
struct YureEnemyWork {
    /* +0x000 */ u8 active_0x000;
    /* +0x001 */ u8 pad_0x001[2];
    /* +0x003 */ u8 type_0x003;            /* the row of the sway table */
    /* +0x004 */ u8 pad_0x004[0x590 - 0x004];
    /* +0x590 */ YureRec yure_0x590[3];
    /* +0x740 */ u8 pad_0x740[0xB18 - 0x740];
};

/* The part of the player work record (`get_move_work_adrs(2)`, stride 0xB20) the sway unit reads: the
 * eight sway records (four hair, four cloth).  `include/pl.h`'s `_PLW` stops at +0x668.
 * size: 0xB20 */
struct YurePlayerWork {
    /* +0x000 */ u8 active_0x000;
    /* +0x001 */ u8 pad_0x001[0x674 - 0x001];
    /* +0x674 */ YureRec yure_0x674[8];
    /* +0xAF4 */ u8 pad_0xAF4[0xB20 - 0xAF4];
};

#define YURE_PLAYER_REC_MAX 8

/* The sway world: the gravity vector (+0x00) the chains fall along and 0x1C bytes of tail no function of
 * this unit touches.  size: 0x28 */
struct YureWorld {
    /* +0x00 */ nw4r::math::VEC3 gravity_0x00;
    /* +0x0C */ u8 unused_0x0C[0x1C];
};

extern YureWorld yure_world;

extern "C" void yure_static_init(void);
extern "C" void yure_enemy_init(YureEnemyWork* enemy);
extern "C" void yure_player_init(YurePlayerWork* plw);
extern "C" void yure_player_init_kind(YurePlayerWork* plw, u8 kind);
extern "C" void yure_player_clear_flags(YurePlayerWork* plw);
extern "C" void yure_player_move(YurePlayerWork* plw);
extern "C" void yure_step(void* owner /* untyped: caller-owned payload - a YureEnemyWork or a YurePlayerWork, by `is_enemy` */, u8 is_player, s32 index, s32 kind);
void yure_move(void);

#endif /* __cplusplus */

#endif /* MHTRI_PL_PL_YURE_H */
