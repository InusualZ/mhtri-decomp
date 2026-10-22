/* enemy/emc_delay_frames_get.h - the declarations of `emc_delay_frames_get`, which `enemy/enemy_control.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_ENEMY_EMC_DELAY_FRAMES_GET_H
#define MHTRI_ENEMY_EMC_DELAY_FRAMES_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s16 emc_delay_frames_get(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EMC_DELAY_FRAMES_GET_H */
