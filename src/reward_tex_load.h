/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape.cpp`'s `reward_tex_load`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_REWARD_TEX_LOAD_H
#define MHTRI_REWARD_TEX_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80055AC4 (0x198): loads the reward screen's textures. */
void reward_tex_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_REWARD_TEX_LOAD_H */
