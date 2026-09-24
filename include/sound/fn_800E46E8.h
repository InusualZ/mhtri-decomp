/*
 * Declarations owned by `sound/fn_800E46E8.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.  Keep it minimal.
 */
#ifndef MHTRI_SOUND_FN_800E46E8_H
#define MHTRI_SOUND_FN_800E46E8_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The depth-compare selector `set_zmode` maps a `_GXCompare` value through. */
s32 fn_800E46E8(u32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800E46E8_H */
