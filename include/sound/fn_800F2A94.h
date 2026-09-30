#ifndef MHTRI_SOUND_FN_800F2A94_H
#define MHTRI_SOUND_FN_800F2A94_H

#include "types.h"

/* Declarations for the symbols `src/sound/fn_800F2A94.cpp` owns (docs/plan.md 6.5 rule 2).
 * `sound/fn_800EF7D8.cpp` called fn_800F48F4 through its own extern until this unit registered - the two
 * ranges are adjacent, so the ownership only settled when both were landed.
 */
#ifdef __cplusplus
extern "C" {
#endif

extern "C" void fn_800F48F4(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/* 0x800F6334 - stops every BGM stream; the map's `bgm_stop_all__Fv`.  Added with `quest/arenatask.cpp`
 * (rule 2: this range owns the address). */
void bgm_stop_all(void);
#endif

#endif /* MHTRI_SOUND_FN_800F2A94_H */
