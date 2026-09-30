/*
 * sound/fn_800EF7D8.h - the declarations `src/sound/fn_800EF7D8.cpp` owns (docs/plan.md 6.5 rule 2).  The
 * two loaders are C++ free functions - the map's `title_se_load__Fv` (0x800F18F0) and `title_bgm_load__Fv`
 * (0x800F2228) - loading the title's sound-effect bank and its BGM stream.  Added with
 * `quest/arenatask.cpp`, the first consumer.
 */
#ifndef MHTRI_SOUND_FN_800EF7D8_H
#define MHTRI_SOUND_FN_800EF7D8_H

#include "types.h"

#ifdef __cplusplus
void title_se_load(void);
void title_bgm_load(void);
#endif

#endif /* MHTRI_SOUND_FN_800EF7D8_H */
