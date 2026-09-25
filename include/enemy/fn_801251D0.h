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
/* 0x80128A8C / 0x8012933C - this unit's own definitions (the first is defined here, the second is an
 * address inside its range).  Signatures are the owner's: `fn_80128A8C` takes two u32; `fn_8012933C`
 * narrows its second argument to u8 itself (`clrlwi r4,r4,24` on the way into `fn_801251D8`) and the
 * owner's own body calls it `(self, (u8)a, b, 0)`.  One declaration, hand-merged for the three
 * consumers that each moved it here (rule 2): `enemy/fn_80147CE0.cpp`, `enemy/fn_8015E854.cpp` and
 * `enemy/fn_80178378.cpp` - the sibling landings and this branch had put it in three times. */
void fn_80128A8C(struct _ENEMY_WORK* self, u32 a, u32 b);
/* 0x80126324 - the motion/area setter: r3 (`self`), a byte r4 and a scalar r5 (the body does
 * `clrlwi r4,r4,24`, folds `self->area_no (0x1E1) & 0xF` into the high byte of the id it builds,
 * and passes `clrlwi r6,r31,24` on to 0x8012B380) plus the f32 blend f1 it stores at +0x384.
 * Declared from the callee's own body (docs/plan.md 6.5 rule 6). */
void fn_80126324(struct _ENEMY_WORK* self, u32 a, u32 b, f32 c);
void fn_8012933C(struct _ENEMY_WORK* self, u8 a, u32 b, u32 c);
/* 0x80127FE4 / 0x801280AC - one `self` argument, no return.  Moved here from
 * `enemy/fn_801550FC.cpp` on landing (rule 2): this unit owns the addresses. */
void fn_80127FE4(struct _ENEMY_WORK* self);
void fn_801280AC(struct _ENEMY_WORK* self);
/* 0x80128A70 / 0x80128AAC - r3 (`self`) and two u8 arguments (`clrlwi r4,r4,24` /
 * `clrlwi r5,r5,24`); 0x80128AAC supplies the constant third argument itself. */
void fn_80128A70(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128AAC(struct _ENEMY_WORK* self, u32 a, u32 b);
u32 fn_80128204(struct _ENEMY_WORK* work);
void fn_80128308(struct _ENEMY_WORK* work);
void fn_80128BF8(struct _ENEMY_WORK* work, s32 arg1);
void fn_8012987C(struct _ENEMY_WORK* work);
void fn_8012A3B4(struct _ENEMY_WORK* work);
void fn_8012A414(struct _ENEMY_WORK* work);
void fn_8012A658(struct _ENEMY_WORK* work, s32 arg1);
void fn_8012B64C(struct _ENEMY_WORK* work);

void fn_801251D0(u32 a, u32 b, u32 c);
/* 0x801251D8 - r3, r4 and r5 (its body does `clrlwi r5,r5,24` then tail-calls 0x80124C5C).
 * `enemy/fn_8014A1BC.c` calls it with three arguments (the first is the table, not `self`);
 * `enemy/fn_801550FC.cpp` calls it with four (`self`, table, selector, value), and the target sets
 * both r5 and r6, so C keeps the three-argument form and C++ gets the four-argument one. */
#ifdef __cplusplus
void fn_801251D8(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);
#else
void fn_801251D8(u32 a, u32 b, u32 c);
#endif
/* 0x80128030 - r3 (`self`) only; the action band's completion hook.  Added by
 * `enemy/fn_80178378.cpp` (docs/plan.md 6.5 rule 2): this unit owns the address. */
void fn_80128030(struct _ENEMY_WORK* self);
/* 0x80129724 - r3 (`self`) and one scalar argument (every call site sets r4). */
void fn_80129724(struct _ENEMY_WORK* self, u32 a);
void fn_801252C0(struct _ENEMY_WORK* self, u8 a);
void fn_8012554C(struct _ENEMY_WORK* self);
u8 fn_80125F88(u32 idx);
u32 fn_80125F9C(u32 a, u32 b);
u32 fn_80125FF0(u32 a, u32 b);
u8* fn_80126044(struct _ENEMY_WORK* self);
void* fn_80126704(struct _ENEMY_WORK* self);
void fn_80126898(struct _ENEMY_WORK* self);
void fn_801280F4(struct _ENEMY_WORK* self);
/* 0x80128030 - one `self` argument, no return.  Added with its owner by
 * `enemy/fn_80182D5C.cpp`, whose state machines call it after `fn_8012F93C` reports done; the
 * existing consumers (`enemy/fn_80176C58.cpp`, `enemy/fn_80178128.cpp`) spell it the same way. */
void fn_80128030(struct _ENEMY_WORK* self);
void fn_80129668(struct _ENEMY_WORK* self, u32 a, u32 b);
/* 0x80126454 - `get_enemy_data(self)->extra->table_0x1C` indexed by `self->field_0x38a` in
 * 0x10-byte steps; the caller (`enemy/fn_8015E854.cpp`'s `fn_8015EFAC`) reads the f32 at +0x4. */
f32* fn_80126454(struct _ENEMY_WORK* self);
void fn_80129864(struct _ENEMY_WORK* self);
void fn_80129984(struct _ENEMY_WORK* self);
u32 fn_8012B5C4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012B604(void);

/* 0x80125F54 - r3 is the `EmSelRec` the caller owns; the body zeroes its +0x08 vector
 * (`fn_80043EA8(out + 8)`) and returns r3, so the return value is the same pointer.  Added with
 * `enemy/fn_801B7020.cpp` (rule 2: this unit owns the address). */
void* fn_80125F54(void* out);

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
