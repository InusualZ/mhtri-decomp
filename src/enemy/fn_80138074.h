/* Declarations `enemy/fn_80138074.c` owns, in the signatures its consumers use (the wider form where only the
 * parameter spelling differed).
 */
#ifndef MHTRI_ENEMY_FN_80138074_H
#define MHTRI_ENEMY_FN_80138074_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

/* The `ResUserDataAc` record `fn_8013A654` runs on (the owner spells the type `ResUserDataAc`): a
 * work pointer at +0x04 and a flag word at +0x08 (settled from `fn_8013A654`'s own body, which does
 * `lwz r31,4(r3)` and reads r31's +0x110/+0x13C).  `enemy/fn_801823C0` clears the work's part state
 * through it.  size: 0x0C */
struct EmUserData {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ struct _ENEMY_WORK* work_0x04;
    /* +0x08 */ u32 flags_0x08;
};

#ifdef __cplusplus
extern "C" {
#endif
/* 0x801394C0 - the track-selection helper `lobby/lb_equip_page.cpp` tail-calls (rule 2). */
void fn_801394C0(void* arg, u32 index);

/* 0x8013A9F4 - r3 `self`; the joint-effect release `enemy/em025_prog.cpp`'s `fn_801A9540` calls before it walks
 * the slots. */
void fn_8013A9F4(struct _ENEMY_WORK* self);
u32 fn_8013A884(struct _ENEMY_WORK* self, s32 value);
u32 fn_8013A8B4(struct _ENEMY_WORK* enemy, s32 a, s32 b);
u8 fn_8013A900(struct _ENEMY_WORK* enemy);
void fn_8013AAC4(struct _ENEMY_WORK* enemy);
u32 fn_8013AB74(struct _ENEMY_WORK *self, u32 a, u32 b);

/* The user-data accessors `enemy/fn_8013ACC4.cpp`'s interpreter drives (its call sites' arity). */
void em_userdata_state_exit(struct _ENEMY_WORK* self, u8 arg);
void em_userdata_state_reenter_alt(struct _ENEMY_WORK* self);
void em_userdata_motion_head_load(struct _ENEMY_WORK* self);
u32 em_userdata_roll(struct _ENEMY_WORK* self, u16 index); /* `ran_suu(1)`'s tail: the state's next roll */
u8 em_userdata_record_find_back(struct _ENEMY_WORK* self, u8 index, u8 key);

/* The user-data accessors `enemy/em015_prog.cpp` calls. */
s32 em_res_user_data_ck(struct _ENEMY_WORK* self);
void em_res_user_data_set(struct _ENEMY_WORK* self, void* arg1);
/* 0x8013A654 - r3 `self` (the owner spells it `ResUserDataAc*`) and r4, stored at +0x8 of the record at +0x4. */
void fn_8013A654(struct _ENEMY_WORK* self, u32 a);
/* 0x8013918C - r3 `self` and r4, the flag `enemy/em018_prog.cpp`'s `fn_8019E5A8` passes as 0 before it decides
 * whether to `operator delete` the record. */
void fn_8013918C(struct _ENEMY_WORK* self, u32 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80138074_H */
