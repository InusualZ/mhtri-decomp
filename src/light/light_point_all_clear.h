/* Leaf header (docs/plan.md 6.5 rule 2): `light/light.cpp`'s `light_point_all_clear`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_LIGHT_LIGHT_POINT_ALL_CLEAR_H
#define MHTRI_LIGHT_LIGHT_POINT_ALL_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C0728 (0xAC): clears every point light (the map's name). */
void light_point_all_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_POINT_ALL_CLEAR_H */
