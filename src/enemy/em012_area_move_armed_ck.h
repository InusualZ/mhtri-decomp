/* `enemy/em012_prog.cpp`'s area-move query, for the units that read a partner enemy's state (`enemy/em010_prog.cpp`). */
#ifndef MHTRI_ENEMY_EM012_AREA_MOVE_ARMED_CK_H
#define MHTRI_ENEMY_EM012_AREA_MOVE_ARMED_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80176AA8 */
u32 em012_area_move_armed_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM012_AREA_MOVE_ARMED_CK_H */
