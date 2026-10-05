/* The declarations owned by `enemy/fn_801502C8.cpp` - the enemy band's 0x801502C8..0x801545B8 block
 * (docs/plan.md 6.5 rule 2: a consumer includes the owner's header, it never declares the symbol
 * itself).
 *
 * The three entry points below left `unsplit/enemy.h` when this unit registered the range -
 * they had been declared there from `enemy/fn_801B0010.cpp`'s and `enemy/fn_80147CE0.cpp`'s call
 * sites - and that band header includes this one now.  The spellings are this unit's definitions'.
 *
 * `fn_801545B8` keeps the C consumers' `void*` spelling under `#ifndef __cplusplus`: this unit defines
 * it over `EmSpawnRec` while `enemy/fn_8014A1BC.c` passes its own `ShellParams` view of the same 0x18
 * bytes.  The split is the one `enemy/fn_80147CE0.h` carries for the same reason.
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
 * `enemy/fn_801B0010.cpp`'s call site reads it unsigned, so the definition returns `u32`). */
u32 fn_80154784(struct _ENEMY_WORK* self);

/* 0x80154CA4 - clamps the record's +0x1AC counter after `fn_801303FC`. */
void fn_80154CA4(struct _ENEMY_WORK* self);

#ifdef __cplusplus
/* 0x801545B8 - fills the 0x18-byte spawn record and posts its effect. */
void fn_801545B8(EmSpawnRec* rec, u8 arg1, s16 arg2, s16 arg3);
#else
/* The C consumers' spelling (`enemy/fn_8014A1BC.c` passes its own `ShellParams` record). */
void fn_801545B8(void* rec, u32 a, u32 b, u32 c);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801502C8_H */
