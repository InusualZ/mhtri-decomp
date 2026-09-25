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
u32 em_area_ck__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
u32 em_die_ck__FP11_ENEMY_WORK(struct _ENEMY_WORK* work);
void fn_8012BDF4(struct _ENEMY_WORK* work);
void fn_8012C600(struct _ENEMY_WORK* work);
void fn_8012C9AC(struct _ENEMY_WORK* work);
s32 fn_8012D0B4(struct _ENEMY_WORK* enemy, void* move);
s32 fn_8012D188(struct _ENEMY_WORK* enemy, struct _ENEMY_WORK* other);
u32 fn_8012D1A0(struct _ENEMY_WORK* work);
s32 fn_8012D1A8(u8 arg0);
u32 fn_8012D23C();
u8 fn_8012D3E0();
u32 fn_8012D7FC(struct _ENEMY_WORK* other);
u32 fn_8012E5A8(struct _ENEMY_WORK* self);

/* Whether more than `seconds` have passed since the last frame stamp.  Declared here, with its owner
 * (rule 2), because `enemy/fn_8012E968.cpp` calls it and must not re-declare it locally. */
s32 fn_8012E8F4(f32 seconds);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
/* Declared for the C++ consumers (docs/plan.md 6.5 rule 2): this unit owns the symbol.  The C
 * consumers spell it themselves with ABI-equivalent prototypes, and MWCC's C front-end rejects a
 * fixed prototype after one of those, so the declaration is C++-only. */
#ifdef __cplusplus
void fn_8012CF20(struct _ENEMY_WORK* self);
#endif
/* 0x8012C220 / 0x8012C4E8 - this unit's own definitions, moved here from `include/unsplit/enemy.h`
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
#endif

#endif /* MHTRI_ENEMY_FN_8012BDF4_H */
