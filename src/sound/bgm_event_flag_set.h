/* sound/bgm_event_flag_set.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `bgm_event_flag_set` (0x800F63B4), which `sound/fn_800F2A94.cpp` owns;
 * it raises the BGM work's event flag (+0x1D8) with its 0x24 timer. */
#ifndef MHTRI_SOUND_BGM_EVENT_FLAG_SET_H
#define MHTRI_SOUND_BGM_EVENT_FLAG_SET_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

void bgm_event_flag_set(void);

#ifdef __cplusplus
}
#endif

#endif
