/* enemy/em_team_area_alert_set.h - the declaration of `em_team_area_alert_set` (0x8012C300), which
 * `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2): sets `flags_0xA04` bit 1 on every live record of a team
 * standing in an area. */
#ifndef MHTRI_ENEMY_EM_TEAM_AREA_ALERT_SET_H
#define MHTRI_ENEMY_EM_TEAM_AREA_ALERT_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s32 em_team_area_alert_set(u32 team, u32 area);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_TEAM_AREA_ALERT_SET_H */
