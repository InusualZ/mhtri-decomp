/* Declarations `enemy/em_common.cpp` owns from its action/status band (0x8012BDF4-0x8012E968): the `em_*` checks and
 * the `fn_8012C*`/`fn_8012D*` helpers, in the signatures the consumers use.
 */
#ifndef MHTRI_ENEMY_FN_8012BDF4_H
#define MHTRI_ENEMY_FN_8012BDF4_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

s32 em_act_ck__FP11_ENEMY_WORKUcUc(struct _ENEMY_WORK* work, u8 a, u8 b);
/* `em_act_ck` must not be declared in this `extern "C"` block: the first declaration fixes the linkage, and the
 * map references the mangling `em_act_ck__FP11_ENEMY_WORKUcUc` (the C++ declaration at the bottom). */
u32 em_area_ck__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
u32 em_die_ck__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
void fn_8012BDF4(struct _ENEMY_WORK* work);
/* 0x8012CEB4 - r3 the work record, r4 the timer, r5 the slot index; the area-table timer arm the action counters
 * use. */
void fn_8012CEB4(struct _ENEMY_WORK* self, s16 timer, u8 index);
void fn_8012C600(struct _ENEMY_WORK* work);
void fn_8012C9AC(struct _ENEMY_WORK* work);
s32 fn_8012D0B4(struct _ENEMY_WORK* enemy, void* move);
s32 fn_8012D188(struct _ENEMY_WORK* enemy, struct _ENEMY_WORK* other);
u32 em_busy_ck(struct _ENEMY_WORK* work);
/* 0x8012E728 - ends the running action; `mode` is the caller's 0. */
void em_act_end(struct _ENEMY_WORK* work, s32 mode);
s32 fn_8012D1A8(u8 arg0);
u32 fn_8012D23C();
u8 fn_8012D3E0();
u32 fn_8012D7FC(struct _ENEMY_WORK* other);
u32 fn_8012E5A8(struct _ENEMY_WORK* self);
/* 0x8012E664 - one `self` argument, no return (clears the work record; `enemy/em016_prog.cpp`'s teardown step). */
void fn_8012E664(struct _ENEMY_WORK* self);
/* 0x8012E694 - one `self` argument, no return (the finish step `enemy/em016_prog.cpp` runs once
 * `em_mot_end_ck` reports done). */
void fn_8012E694(struct _ENEMY_WORK* self);
/* 0x8012D8D0 / 0x8012DB3C / 0x8012E21C - in the signatures their call sites set (r3 the work record / the two
 * byte-derived values). */
u32 fn_8012D8D0(struct _ENEMY_WORK* work);
u32 fn_8012DB3C(struct _ENEMY_WORK* work);
u32 fn_8012E21C(u32 state, u32 action);
/* 0x8012C300 / 0x8012C3C8 - the owner's own signatures: `(u32 team, u32 state_sub)` and
 * `(u32 team, u32 state_sub, void* ref, f32 radius)`. */
s32 fn_8012C300(u32 team, u32 state_sub);
s32 fn_8012C3C8(u32 team, u32 state_sub, void* ref, f32 radius);

/* Whether more than `seconds` have passed since the last frame stamp. */
s32 fn_8012E8F4(f32 seconds);



/* C++-only: the C consumers spell it with ABI-equivalent prototypes, and MWCC's C front-end rejects a fixed
 * prototype after one of those. */
#ifdef __cplusplus
void em_busy_set(struct _ENEMY_WORK* self);
#endif
/* 0x8012C220 / 0x8012C4E8 - the owner's own signatures (`s32 (u32 team, u32 state_sub)`; `void (u32 team,
 * u32 state_sub, s16 arg3, void* ref, f32 radius)`), which `enemy/em001_prog.cpp` calls them with. */
#ifdef __cplusplus
s32 fn_8012C220(u32 team, u32 state_sub);
void fn_8012C4E8(u32 team, u32 state_sub, s16 arg3, void* ref, f32 radius);
#endif
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* Defined at C++ scope; the target objects reference their manglings (`em_work_die_ck__FP11_ENEMY_WORK`). */
s32 em_work_die_ck(struct _ENEMY_WORK* enemy);
/* The C++ spelling of `em_die_ck__FP11_ENEMY_WORK`. */
s32 em_die_ck(struct _ENEMY_WORK* work);
/* The C++ spelling of `em_area_ck__FP11_ENEMY_WORK` (the `extern "C"` block above spells the mangling). */
u32 em_area_ck(struct _ENEMY_WORK* work);
/* The C++ spelling of `em_act_ck__FP11_ENEMY_WORKUcUc` (the `extern "C"` block above spells the mangling). */
s32 em_act_ck(struct _ENEMY_WORK* work, u8 a, u8 b);
#endif

#endif /* MHTRI_ENEMY_FN_8012BDF4_H */
