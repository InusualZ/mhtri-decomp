/* The declarations owned by `enemy/fn_80147CE0.cpp` (docs/plan.md 6.5 rule 2): the enemy action band's
 * entry points that other units call.
 *
 * Moved here on that unit's landing from `unsplit/enemy.h`, which carried the consumers'
 * old-style spellings (`void fn_801493A8();`) while the symbols had no registered owner.  The
 * consumers (`enemy/fn_8014A1BC.c`, `enemy/fn_80149D6C.c`, `enemy/fn_80176C58.cpp`) now include this
 * header; their C call sites keep the old-style declarations below, because two of them pass argument
 * counts the real prototypes would reject (C allows it with `()`, C++ does not).
 */
#ifndef MHTRI_ENEMY_FN_80147CE0_H
#define MHTRI_ENEMY_FN_80147CE0_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

/* The 0x18-byte spawn record the unit's `em_spawn_rec_init`/`fn_801545B8` fill and `fn_801493A8` builds on
 * its stack.  Same layout as `enemy/fn_8014A1BC.c`'s private `ShellParams` (its +0x12/+0x14 u16 pair
 * is the one that unit reads back); the shared home for both is a rule-1 follow-up.
 * size: 0x18 */
typedef struct EmSpawnRec {
    /* +0x00 */ u32 id;           /* the effect type `fn_801545B8` stores (26) */
    /* +0x04 */ VEC3 pos;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ u16 field_0x14;
} EmSpawnRec;

#ifdef __cplusplus
extern "C" {
#endif

/* The helper's base constructor (the same 12-byte record `enemy/fn_80176C58.cpp` names
 * `Helper_80176E50`; the parameter stays `void*` until that type moves to a shared header). */
void* em_res_user_data_ctor(void* self);

#ifdef __cplusplus
void fn_801481FC(struct _ENEMY_WORK* self);
void fn_801493A8(struct _ENEMY_WORK* self, u32 arg1, s32 arg2, u32 arg3);
EmSpawnRec* em_spawn_rec_init(EmSpawnRec* rec);
void fn_801498C8(struct _ENEMY_WORK* self);
void fn_80149A08(struct _ENEMY_WORK* self);
void fn_80149AFC(struct _ENEMY_WORK* self);
void fn_80149C54(struct _ENEMY_WORK* self);
void fn_80149C58(struct _ENEMY_WORK* self);
#else
/* The consumers' old-style spellings (C only). */
void fn_801481FC();
void fn_801493A8();
void em_spawn_rec_init();
void fn_801498C8();
void fn_80149A08();
void fn_80149AFC();
void fn_80149C54();
void fn_80149C58();
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80147CE0_H */
