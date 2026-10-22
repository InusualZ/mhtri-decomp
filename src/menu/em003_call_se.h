#ifndef EM003_CALL_SE_H
#define EM003_CALL_SE_H

#include "types.h"

/* Defined in `menu/menu_placeinfo.cpp`; declared here for the callers outside that unit (GUESS names, docs/pipeline.md 16.2). */
struct _ENEMY_WORK;
extern "C" void em003_call_se_reset(struct _ENEMY_WORK* self);
extern "C" void em003_call_se_step(struct _ENEMY_WORK* self, u8 call_kind, u8 variant);

#endif
