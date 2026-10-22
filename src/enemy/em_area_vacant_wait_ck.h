#ifndef EM_AREA_VACANT_WAIT_CK_H
#define EM_AREA_VACANT_WAIT_CK_H

#include "types.h"

/* Defined in `enemy/em_common.cpp`; declared here for its callers outside that unit (GUESS name, docs/pipeline.md 16.2). */
struct _ENEMY_WORK;
extern "C" s32 em_area_vacant_wait_ck(struct _ENEMY_WORK* self, u8 mode);

#endif
