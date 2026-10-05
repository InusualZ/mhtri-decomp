/* The enemy action/status unit `enemy/fn_8012BDF4.cpp` (0x8012BDF4..0x8012E968): its public `em_*` entry points and the `fn_8012C*`/`fn_8012D*` helpers its neighbours call.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
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
/* NOTE: the `s32 em_act_ck(...)` declaration that used to sit *here*, inside this
 * `extern "C"` block, made every consumer that includes this header emit the unmangled `em_act_ck`
 * while the map (and the retail objects) reference the mangling `em_act_ck__FP11_ENEMY_WORKUcUc`: the
 * first declaration of a name fixes its language linkage, so the C-linkage block above was winning
 * over the C++-scope declaration at the bottom of this file.  Removed - the C++ declaration below is
 * now the first, and callers emit the map's spelling (`em020_handlers.cpp` measured 98.76 -> 98.85 on
 * it, and it is what keeps the object linkable). */
u32 em_area_ck__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
u32 em_die_ck__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
void fn_8012BDF4(struct _ENEMY_WORK* work);
/* 0x8012CEB4 - r3 the work record, r4 the timer, r5 the slot index; the area-table timer arm
 * `enemy/fn_801A9540.cpp`'s counters use.  Added with that unit's registration (rule 2: this
 * range owns the address). */
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
/* 0x8012E664 - one `self` argument, no return (clear the work record).  Added with its owner by
 * `enemy/fn_80182D5C.cpp`, which calls it from the enemy-teardown step; the owner itself
 * declares and defines it with this signature. */
void fn_8012E664(struct _ENEMY_WORK* self);
/* 0x8012E694 - one `self` argument, no return (the finish step `enemy/fn_80182D5C.cpp` runs once
 * `em_mot_end_ck` reports done).  Added with its owner (rule 2). */
void fn_8012E694(struct _ENEMY_WORK* self);
/* 0x8012D8D0 / 0x8012DB3C / 0x8012E21C - this unit's own definitions, moved here from the consumer
 * `enemy/fn_8012EC74.cpp` (rule 2): the owner is this unit, and the signatures are the ones that
 * file's call sites set (r3 the work record / the two byte-derived values). */
u32 fn_8012D8D0(struct _ENEMY_WORK* work);
u32 fn_8012DB3C(struct _ENEMY_WORK* work);
u32 fn_8012E21C(u32 state, u32 action);
/* 0x8012C300 / 0x8012C3C8 - this unit's own definitions, added by `enemy/fn_801B0010.cpp` (rule 2).
 * Signatures are the owner's own definitions: the first is `(u32 team, u32 state_sub)` and the
 * second `(u32 team, u32 state_sub, void* ref, f32 radius)`. */
s32 fn_8012C300(u32 team, u32 state_sub);
s32 fn_8012C3C8(u32 team, u32 state_sub, void* ref, f32 radius);

/* Whether more than `seconds` have passed since the last frame stamp.  Declared here, with its owner
 * (rule 2), because `enemy/fn_8012E968.cpp` calls it and must not re-declare it locally. */
s32 fn_8012E8F4(f32 seconds);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
/* Declared for the C++ consumers (docs/plan.md 6.5 rule 2): this unit owns the symbol.  The C
 * consumers spell it themselves with ABI-equivalent prototypes, and MWCC's C front-end rejects a
 * fixed prototype after one of those, so the declaration is C++-only. */
#ifdef __cplusplus
void em_busy_set(struct _ENEMY_WORK* self);
#endif
/* 0x8012C220 / 0x8012C4E8 - this unit's own definitions, moved here from `unsplit/enemy.h`
 * (rule 2): the band header had them as if no unit owned the address.  Signatures are the owner's
 * definitions (`s32 fn_8012C220(u32 team, u32 state_sub)`; `void fn_8012C4E8(u32 team, u32 state_sub,
 * s16 arg3, void* ref, f32 radius)`); `enemy/fn_80147CE0.cpp` calls both with those argument types. */
#ifdef __cplusplus
s32 fn_8012C220(u32 team, u32 state_sub);
void fn_8012C4E8(u32 team, u32 state_sub, s16 arg3, void* ref, f32 radius);
#endif
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* The owner defines these at C++ scope and the target objects reference their C++ manglings
 * (`em_work_die_ck__FP11_ENEMY_WORK`), so they are declared with C++ linkage (relocaudit). */
s32 em_work_die_ck(struct _ENEMY_WORK* enemy);
/* The real C++ spelling of `em_die_ck__FP11_ENEMY_WORK` (mangle.py verified), so a caller
 * never spells the mangling (docs/plan.md 6.5 rule 9). */
s32 em_die_ck(struct _ENEMY_WORK* work);
/* The real C++ spelling of `em_area_ck__FP11_ENEMY_WORK`, added with `enemy/fn_8012EC74.cpp` (its
 * first consumer that must call it; the extern "C" block above spelled the mangling, which rule 9
 * forbids a caller from using). */
u32 em_area_ck(struct _ENEMY_WORK* work);
/* The C++ spelling of `em_act_ck__FP11_ENEMY_WORKUcUc` (the extern "C" block above spells the
 * mangling, which rule 9 forbids a caller from using); added with `enemy/fn_8019ED34.cpp`. */
s32 em_act_ck(struct _ENEMY_WORK* work, u8 a, u8 b);
#endif

#endif /* MHTRI_ENEMY_FN_8012BDF4_H */
