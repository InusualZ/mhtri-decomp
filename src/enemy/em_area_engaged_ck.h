/* enemy/em_area_engaged_ck.h - the declaration of `em_area_engaged_ck` (0x801B0010), which `enemy/em030_prog.cpp`
 * owns (docs/plan.md 6.5 rule 2): whether a live kind-3 work record in the area answers its team's check. */
#ifndef MHTRI_ENEMY_EM_AREA_ENGAGED_CK_H
#define MHTRI_ENEMY_EM_AREA_ENGAGED_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_area_engaged_ck(u32 area);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_AREA_ENGAGED_CK_H */
