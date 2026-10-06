/* Leaf header (docs/plan.md 6.5 rule 2): the split-screen kill cut-in `camera/camera_main.cpp` owns, for the quest kill
 * bookkeeping.  C linkage (the owner defines it inside its file-wide `extern "C"` block). */
#ifndef MHTRI_CAMERA_CAMERA_KILL_CUT_START_SPLIT_H
#define MHTRI_CAMERA_CAMERA_KILL_CUT_START_SPLIT_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BEE3C - runs the kill cut-in on `enemy` for both split-screen players' banks. */
void camera_kill_cut_start_split(u8 id, struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_KILL_CUT_START_SPLIT_H */
