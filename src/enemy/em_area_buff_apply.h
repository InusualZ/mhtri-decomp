#ifndef EM_AREA_BUFF_APPLY_H
#define EM_AREA_BUFF_APPLY_H

#include "types.h"

/* Defined in `enemy/em_common.cpp`; declared here for the callers outside that unit (GUESS names, docs/pipeline.md 16.2). */
struct _ENEMY_WORK;
extern "C" void em_area_buff_apply(struct _ENEMY_WORK* self, u8 area_no, u8 mode);

#endif
