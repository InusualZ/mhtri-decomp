/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `field_game_task_start`, which `lobby/lb_menu_pos_tbl.cpp` calls. */
#ifndef MHTRI_FIELD_GAME_TASK_START_H
#define MHTRI_FIELD_GAME_TASK_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80041A0C (0x10): hands task slot 4 to `field_game_task`. */
void field_game_task_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FIELD_GAME_TASK_START_H */
