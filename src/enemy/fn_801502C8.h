/* Declarations `enemy/em001_prog.cpp` owns from its 0x801502C8-0x801545B8 block, in its definitions' spellings;
 * `fn_801545B8` keeps the C consumers' `void*` under `#ifndef __cplusplus` (the definition takes `EmSpawnRec`, a C
 * caller passes its own `ShellParams` view of the same 0x18 bytes).
 */
#ifndef MHTRI_ENEMY_FN_801502C8_H
#define MHTRI_ENEMY_FN_801502C8_H

#include "types.h"

#ifdef __cplusplus
#include "enemy/fn_80147CE0.h" /* EmSpawnRec */
#endif

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80154784 - whether the work record's action is armed (its answer is compared against 1 -
 * `enemy/em030_prog.cpp`'s call site reads it unsigned, so the definition returns `u32`). */
u32 fn_80154784(struct _ENEMY_WORK* self);

/* 0x80154CA4 - clamps the record's +0x1AC counter after `fn_801303FC`. */
void fn_80154CA4(struct _ENEMY_WORK* self);

#ifdef __cplusplus
/* 0x801545B8 - fills the 0x18-byte spawn record and posts its effect. */
void fn_801545B8(EmSpawnRec* rec, u8 arg1, s16 arg2, s16 arg3);
#else
/* The C consumers' spelling (`enemy/em001_prog.cpp` passes its own `ShellParams` record). */
void fn_801545B8(void* rec, u32 a, u32 b, u32 c);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801502C8_H */
