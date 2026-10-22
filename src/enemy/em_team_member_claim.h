#ifndef EM_TEAM_MEMBER_CLAIM_H
#define EM_TEAM_MEMBER_CLAIM_H

#include "types.h"

/* Defined in `enemy/em_common.cpp`; declared here for the callers outside that unit (GUESS names, docs/pipeline.md 16.2). */
struct _ENEMY_WORK;
extern "C" s32 em_team_member_claim(u32 team, u32 area_no);

#endif
