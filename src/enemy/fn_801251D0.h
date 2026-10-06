/* Declarations `enemy/em_common.cpp` owns (its 0x801251D0-0x8012BA00 accessors and `get_enemy_data`), in the
 * signatures the consumers use (the wider form where only the parameter spelling differed).
 */
#ifndef MHTRI_ENEMY_FN_801251D0_H
#define MHTRI_ENEMY_FN_801251D0_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

/* The per-enemy part limits `em_get_part_limit` returns: only the field the net sync tests is named.
 * size: 0x6 (approximate - the record is larger, its extent is not measured here) */
struct EmPartLimit {
    /* +0x0 */ u8 unused_0x0[0x4];
    /* +0x4 */ s16 break_limit_0x04;   /* part break data exists when this is positive */
};

#ifdef __cplusplus
extern "C" {
#endif

struct _HIT_W;
/* 0x801268B0 - the enemy's part limit record. */
struct EmPartLimit* em_get_part_limit(struct _ENEMY_WORK* work);
/* 0x80127158 - re-applies the special part selected by `selector`. */
void em_special_part_apply(struct _ENEMY_WORK* work, u8 selector);
/* 0x801285C0 - arms the action step; the net receiver passes the two bytes of the message and 4. */
void em_act_step_arm(struct _ENEMY_WORK* work, u8 a, u8 b, u32 c);
/* 0x8012555C - advances the running action to its next step; `mode` is 1 for a restart. */
void em_act_advance(struct _ENEMY_WORK* work, u8 mode);
/* 0x80129674 - registers the hit record `hit` (a shell's embedded record) with the enemy `self`. */
void em_hit_buff_apply(struct _ENEMY_WORK* self, struct _HIT_W* hit);
void fn_801252DC(struct _ENEMY_WORK* work);
s32 fn_80126098(struct _ENEMY_WORK* work);
s32 fn_801260BC(struct _ENEMY_WORK* work);
s32 fn_801260E0(struct _ENEMY_WORK* work);
s32 fn_80126104(struct _ENEMY_WORK* work);
/* 0x80126278 - three arguments, from the call sites (`enemy/em018_prog.cpp`'s `fn_8019D9BC` sets all three):
 * r3 the `_ENEMY_WORK`, r4 the 16-bit id, r5 the `VEC3*` fill target. */
void fn_80126278(struct _ENEMY_WORK* self, u16 id, nw4r::math::VEC3* out);
void (*fn_801264BC(struct _ENEMY_WORK* work, s32 index))(struct _ENEMY_WORK*);
u16 em_hit_mask_get(struct _ENEMY_WORK* work);
void fn_801281EC(struct _ENEMY_WORK* work);
void fn_801281F8(struct _ENEMY_WORK* work);
/* 0x80128A8C / 0x8012933C - `em_act_arm_unless_down` takes two u8 arguments (its body narrows both with `clrlwi` before the
 * tail call to `em_act_step_arm`; under the unit's `#pragma peephole off` a u8 parameter keeps that `clrlwi`, so the
 * spelling is load-bearing); `em_hit_window_set` narrows its second argument itself and its owner calls it
 * `(self, (u8)a, b, 0)`.  One declaration for `enemy/em001_prog.cpp`, `enemy/em008_prog.cpp` and
 * `enemy/em015_prog.cpp`. */
void em_act_arm_unless_down(struct _ENEMY_WORK* self, u8 a, u8 b);
/* 0x80126324 - the motion/area setter: r3 `self`, a byte r4 and a scalar r5 (it folds `self->area_no & 0xF` into
 * the id's high byte and passes `clrlwi r6,r31,24` on to 0x8012B380) plus the f32 blend f1 it stores at +0x384. */
void em_move_target_set(struct _ENEMY_WORK* self, u32 a, u32 b, f32 c);
void em_hit_window_set(struct _ENEMY_WORK* self, u8 a, u32 b, u32 c);
/* 0x8012B380 - r3 `self` and three scalars; the motion/state setter the action band calls after `em_mot_end_ck`
 * reports done. */
void fn_8012B380(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
/* 0x80127FE4 / 0x801280AC - one `self` argument, no return. */
void em_action_finish_fall(struct _ENEMY_WORK* self);
void em_action_finish_walk(struct _ENEMY_WORK* self);
/* 0x80128A70 / 0x80128AAC - r3 (`self`) and two u8 arguments (`clrlwi r4,r4,24` /
 * `clrlwi r5,r5,24`); 0x80128AAC supplies the constant third argument itself. */
void fn_80128A70(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128AAC(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_80128204(struct _ENEMY_WORK* work);
void fn_80128308(struct _ENEMY_WORK* work);
void em_target_pos_set(struct _ENEMY_WORK* work, VEC3* pos);
void fn_8012987C(struct _ENEMY_WORK* work);
void fn_8012A3B4(struct _ENEMY_WORK* work);
void fn_8012A414(struct _ENEMY_WORK* work);
void em_area_change(struct _ENEMY_WORK* work, s32 arg1);
void fn_8012B64C(struct _ENEMY_WORK* work);

/* 0x801251D0 - two views of one body, which forwards r3/r4/r5 to 0x80124C5C: the C consumers pass the table in r3
 * (`enemy/em001_prog.cpp`), the C++ ones (`enemy/fn_802F5138.cpp`) the work record, table, selector and id. */
#ifdef __cplusplus
void em_se_tbl_play(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);
#else
void em_se_tbl_play(u32 a, u32 b, u32 c);
#endif
u32 fn_80129A70(struct _ENEMY_WORK* self, u16 a);
u8 fn_80129DB8(struct _ENEMY_WORK* self);
u32 fn_8012A014(struct _ENEMY_WORK* self, u32 a, u32 b, u16 c, void* d, void* e);
u32 fn_8012A204(struct _ENEMY_WORK* self);
void fn_80128AEC(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128B80(struct _ENEMY_WORK* self);

/* 0x801251D8 - narrows r5 and tail-calls 0x80124C5C: C keeps the three-argument form `enemy/em001_prog.cpp` uses
 * (the table first), C++ the four-argument one `enemy/em003_prog.cpp` uses (`self`, table, selector, value). */
#ifdef __cplusplus
void em_se_tbl_play_alt(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);
#else
void em_se_tbl_play_alt(u32 a, u32 b, u32 c);
#endif
/* 0x80128030 - r3 `self` only; the action band's completion hook. */
void fn_80128030(struct _ENEMY_WORK* self);
/* 0x80129724 - r3 (`self`) and one scalar argument (every call site sets r4). */
void em_hit_window_clear(struct _ENEMY_WORK* self, u32 a);
/* 0x80127CC4 - installs the enemy data's default hit-part record for slot `slot`. */
void em_part_rec_reset(struct _ENEMY_WORK* self, u32 slot);
/* 0x80127D20 - installs the enemy data's alternate hit-part record `sel` for slot `slot`. */
void em_part_rec_alt_set(struct _ENEMY_WORK* self, u32 slot, u32 sel);

/* 0x80129744 - r3 `self` only; the action band's release hook (`enemy/em009_act.cpp`'s `fn_80389E1C` calls it when
 * its sub-state count reaches 4). */
void fn_80129744(struct _ENEMY_WORK* self);
void fn_801252C0(struct _ENEMY_WORK* self, u8 a);
void fn_8012554C(struct _ENEMY_WORK* self);
u8 fn_80125F88(u32 idx);
u32 enemy_kind_same_ck(u32 a, u32 b);
u32 fn_80125FF0(u32 a, u32 b);
/* 0x8012A9E8 - r3 the `_ENEMY_WORK`, r4/r5/r6 three byte pointers the effect spawner (`ef/eft_slot.cpp`) hands
 * over. */
void fn_8012A9E8(struct _ENEMY_WORK* self, u8* a, u8* b, u8* c);
u8* fn_80126044(struct _ENEMY_WORK* self);
void* fn_80126704(struct _ENEMY_WORK* self);
void fn_80126898(struct _ENEMY_WORK* self);
void fn_801280F4(struct _ENEMY_WORK* self);
/* 0x80128030 - one `self` argument, no return; the state machines of `enemy/em015_prog.cpp`, `enemy/em016_prog.cpp`,
 * `enemy/em018_prog.cpp` and `enemy/em020_prog.cpp` call it after `em_mot_end_ck` reports done. */
void fn_80128030(struct _ENEMY_WORK* self);
void em_hit_window_set_default(struct _ENEMY_WORK* self, u32 a, u32 b);
/* 0x80128AEC - r3 the work record, r4/r5 two scalars it hands to the 0x80128A3C slot helper (`enemy/em025_prog.cpp`'s
 * `fn_801A9724` asks the area table for action 13's slot through it). */
void fn_80128AEC(struct _ENEMY_WORK* self, u32 a, u32 b);
/* 0x80128A14 - r3 the work record and the two scalars its body narrows; the state machines call it after
 * `em_mot_end_ck` reports the motion done. */
void em_state_set(struct _ENEMY_WORK* self, u32 a, u32 b);
/* 0x80126454 - `get_enemy_data(self)->extra->table_0x1C` indexed by `self->field_0x38a` in
 * 0x10-byte steps; the caller (`enemy/em008_prog.cpp`'s `fn_8015EFAC`) reads the f32 at +0x4. */
f32* fn_80126454(struct _ENEMY_WORK* self);
void fn_80129864(struct _ENEMY_WORK* self);
void fn_80129984(struct _ENEMY_WORK* self);
u32 fn_8012B5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012B604(void);

/* 0x80125F54 - r3 is the caller's `EmSelRec`; the body zeroes its +0x08 vector (`VEC3_ctor(out + 8)`) and returns
 * the same pointer. */
void* em_ground_rec_clear(void* out);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* `get_enemy_data` is declared at C++ scope for its mangling `get_enemy_data__FP11_ENEMY_WORK`. */
struct EnemyData;
EnemyData* get_enemy_data(struct _ENEMY_WORK* work);
#endif

#endif /* MHTRI_ENEMY_FN_801251D0_H */
