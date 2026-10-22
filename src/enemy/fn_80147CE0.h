/* Declarations `enemy/em001_prog.cpp` owns from its action band (0x80147CE0..): the C consumers keep old-style
 * `()` declarations because two of them pass argument counts the real prototypes would reject.
 */
#ifndef MHTRI_ENEMY_FN_80147CE0_H
#define MHTRI_ENEMY_FN_80147CE0_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

/* The 0x18-byte spawn record the unit's `em_spawn_rec_init`/`em001_spawn_rec_fill` fill and `em001_spawn_step` builds on
 * its stack.  Same layout as `enemy/em001_prog.cpp`'s private `ShellParams` (its +0x12/+0x14 u16 pair
 * is the one that unit reads back): one record, two definitions.
 * size: 0x18 */
typedef struct EmSpawnRec {
    /* +0x00 */ u32 id;           /* the effect type `em001_spawn_rec_fill` stores (26) */
    /* +0x04 */ VEC3 pos;
    /* +0x10 */ u8 field_0x10;
    /* +0x12 */ u16 field_0x12;
    /* +0x14 */ u16 field_0x14;
} EmSpawnRec;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
void fn_801481FC(struct _ENEMY_WORK* self);
void em001_spawn_step(struct _ENEMY_WORK* self, u32 arg1, s32 arg2, u32 arg3);
EmSpawnRec* em_spawn_rec_init(EmSpawnRec* rec);
void em001_act3_0(struct _ENEMY_WORK* self);
void em001_act3_1(struct _ENEMY_WORK* self);
void em001_dive_step(struct _ENEMY_WORK* self);
void em001_act3_3(struct _ENEMY_WORK* self);
void em001_act3_4(struct _ENEMY_WORK* self);
#else
/* The consumers' old-style spellings (C only). */
void fn_801481FC();
void em001_spawn_step();
void em_spawn_rec_init();
void em001_act3_0();
void em001_act3_1();
void em001_dive_step();
void em001_act3_3();
void em001_act3_4();
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80147CE0_H */
