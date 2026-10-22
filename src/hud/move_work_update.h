/* `move_work_update`, defined by `hud/move_work_update.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_MOVE_WORK_UPDATE_H
#define MHTRI_HUD_MOVE_WORK_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the lobby's move-work markers; registered by address with `subTransSetPrio`. */
void move_work_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_MOVE_WORK_UPDATE_H */
