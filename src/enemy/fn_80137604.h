/* Declarations `enemy/em_common.cpp` owns from its per-motion action/rotation set (0x80137604-0x8013791C), the
 * move-work slot picker `fn_801377D0` among them.
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
