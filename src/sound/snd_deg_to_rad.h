/* sound/snd_deg_to_rad.h - leaf: the degree-to-radian helper `sound/sound_obj.cpp` defines (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_SOUND_SND_DEG_TO_RAD_H
#define MHTRI_SOUND_SND_DEG_TO_RAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E7CD4 - converts `v` degrees to radians. */
f32 snd_deg_to_rad(f32 v);

#ifdef __cplusplus
}
#endif

#endif
