/* ef/ef_animcurve.h - the symbols `ef/ef_animcurve.cpp` owns that other units call (C linkage): the curve random
 * generator's step and the name hash that seeds it. */
#ifndef MHTRI_EF_EF_ANIMCURVE_H
#define MHTRI_EF_EF_ANIMCURVE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009EEDC - one step of the curve random generator's LCG (`seed * 0x343FD + 0x269EC3`). */
u32 ef_anim_rand_next(u32 seed);

/* 0x8009EEF4 - the four-word hash a random draw is seeded from (the seed, the curve's id, the key and the
 * division). */
u32 ef_anim_name_hash(u16 a, u16 b, u16 c, u32 d);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_ANIMCURVE_H */
