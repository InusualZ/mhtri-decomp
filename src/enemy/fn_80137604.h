/* The enemy unit `enemy/fn_80137604.cpp` (0x80137604..0x80138074): the per-motion action/rotation
 * update set and the move-work slot picker.
 *
 * Declarations for the symbols the unit owns (docs/plan.md 6.5 rule 2: an extern lives with the TU
 * that owns the symbol).  `fn_801377D0` was added by `enemy/fn_801B0010.cpp`, whose em030 program
 * calls it.
 */
#ifndef MHTRI_ENEMY_FN_80137604_H
#define MHTRI_ENEMY_FN_80137604_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801377D0 - the move-work slot picker: r3 the mode and r4 the mask (the owner's own definition is
 * `u8* fn_801377D0(u8 mode, u8 mask)`, and it answers the picked record). */
u8* fn_801377D0(u8 mode, u8 mask);
/* 0x80137720 - latches the record's motion mode (declared with the owner's own `u8` parameter). */
void em_motion_mode_set(struct _ENEMY_WORK* self, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80137604_H */
