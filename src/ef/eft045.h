/*
 * ef/eft045.h - `ef/eft045.cpp`'s view of the effect instance's `+0x38` work record, `MenuFxWork` (cast from
 *   `_EFT::work_0x38`); `ef/eft046.cpp` reads the same record.
 */
#ifndef MHTRI_EF_EFT045_H
#define MHTRI_EF_EFT045_H

#include "types.h"
#include "ef.h"
#include "pl.h"

/* The effect instance's `+0x38` work record, in the family view these updaters use: a model count, the
 * `MHchar` model pointers the count covers, the per-model s16 the state machine seeds, and a counter.
 * size: 0x18 lower bound (the highest offset the band touches is +0x16). */
struct MenuFxWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ MHchar* models[3];
    /* +0x10 */ s16 value_0x10[3];
    /* +0x16 */ s16 counter_0x16;
};
/* size: 0x18 (approximate) */

#endif /* MHTRI_EF_EFT045_H */
