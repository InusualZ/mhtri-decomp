/* sound/snd_level_tbl.h - the declarations `sound/snd_level_tbl.cpp` owns that other units call (docs/plan.md 6.5 rule 2).  The signatures are the ones the callers use;
 * a function the unit has not written yet is declared from its callers. */
#ifndef MHTRI_SOUND_SND_LEVEL_TBL_H
#define MHTRI_SOUND_SND_LEVEL_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 fn_800E8E48(s16 idx);
#ifdef __cplusplus
}
#endif

#endif
