/* Leaf header (docs/plan.md 6.5 rule 2): `mhchar_motion_speed_set`, which `sound/mhchar` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_SOUND_MHCHAR_MOTION_SPEED_SET_H
#define MHTRI_SOUND_MHCHAR_MOTION_SPEED_SET_H

#include "types.h"
#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E1640 - sets the model's motion speed. */
void mhchar_motion_speed_set(MHchar* chr, f32 rate);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MOTION_SPEED_SET_H */
