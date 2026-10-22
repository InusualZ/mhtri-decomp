/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `sound/fn_800E46E8.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C++ linkage (the map row carries the mangling). */
#ifndef MHTRI_SOUND_SET_STR_SE_MAIN_VOL_H
#define MHTRI_SOUND_SET_STR_SE_MAIN_VOL_H

#include "types.h"

/* 0x800E48A8 - sets the SE stream's main volume. */
void set_str_se_main_vol(f32 vol);

#endif /* MHTRI_SOUND_SET_STR_SE_MAIN_VOL_H */
