#ifndef PL_AREA_BUFF_APPLY_H
#define PL_AREA_BUFF_APPLY_H

#include "types.h"

/* Defined in `Pl/pl_act.cpp`; declared here for the callers outside that unit (GUESS names, docs/pipeline.md 16.2). */
struct _ENEMY_WORK;
extern "C" void pl_area_buff_apply(u8 area_no, u8 mode);

#endif
