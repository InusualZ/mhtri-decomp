#ifndef EM_ATTACK_FINISHED_CLEAR_H
#define EM_ATTACK_FINISHED_CLEAR_H

#include "types.h"

/* Defined in `enemy/em_common.cpp`; declared here for its callers outside that unit (GUESS name, docs/pipeline.md 16.2). */
struct _ENEMY_WORK;
extern "C" void em_attack_finished_clear(struct _ENEMY_WORK* record);

#endif
