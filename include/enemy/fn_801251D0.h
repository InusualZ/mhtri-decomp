/* The enemy control unit `enemy/fn_801251D0.cpp` (0x801251D0..0x8012BA00): the enemy work block's
 * per-motion dispatch, its action/status helpers and the `get_enemy_data` accessor.
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_ENEMY_FN_801251D0_H
#define MHTRI_ENEMY_FN_801251D0_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void fn_801252DC(struct _ENEMY_WORK* work);
s32 fn_80126098(struct _ENEMY_WORK* work);
s32 fn_801260BC(struct _ENEMY_WORK* work);
s32 fn_801260E0(struct _ENEMY_WORK* work);
s32 fn_80126104(struct _ENEMY_WORK* work);
void fn_80126278(u16 id, nw4r::math::VEC3* out);
void (*fn_801264BC(struct _ENEMY_WORK* work, s32 index))(struct _ENEMY_WORK*);
u16 fn_80127E78(struct _ENEMY_WORK* work);
void fn_801281EC(struct _ENEMY_WORK* work);
void fn_801281F8(struct _ENEMY_WORK* work);
u32 fn_80128204(struct _ENEMY_WORK* work);
void fn_80128308(struct _ENEMY_WORK* work);
void fn_80128BF8(struct _ENEMY_WORK* work, s32 arg1);
void fn_8012987C(struct _ENEMY_WORK* work);
void fn_8012A3B4(struct _ENEMY_WORK* work);
void fn_8012A414(struct _ENEMY_WORK* work);
void fn_8012A658(struct _ENEMY_WORK* work, s32 arg1);
void fn_8012B64C(struct _ENEMY_WORK* work);

void fn_801251D0(u32 a, u32 b, u32 c);
void fn_801251D8(u32 a, u32 b, u32 c);
void fn_801252C0(struct _ENEMY_WORK* self, u8 a);
void fn_8012554C(struct _ENEMY_WORK* self);
u8 fn_80125F88(u32 idx);
u32 fn_80125F9C(u32 a, u32 b);
u32 fn_80125FF0(u32 a, u32 b);
u8* fn_80126044(struct _ENEMY_WORK* self);
void* fn_80126704(struct _ENEMY_WORK* self);
void fn_80126898(struct _ENEMY_WORK* self);
void fn_80129668(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80129864(struct _ENEMY_WORK* self);
void fn_80129984(struct _ENEMY_WORK* self);
u32 fn_8012B5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012B604(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* `get_enemy_data` is the unit's one genuinely mangled symbol
 * (`get_enemy_data__FP11_ENEMY_WORK`), so it is defined and declared at C++ scope (docs/plan.md 6.5
 * rule 9: a caller never spells the mangling). */
struct EnemyData;
EnemyData* get_enemy_data(struct _ENEMY_WORK* work);
#endif

#endif /* MHTRI_ENEMY_FN_801251D0_H */
