/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `armor_color_rgba_get`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_ARMOR_COLOR_RGBA_GET_H
#define MHTRI_PL_ARMOR_COLOR_RGBA_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E774 - the actor's armour dye colour as an RGBA word. */
s32 armor_color_rgba_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ARMOR_COLOR_RGBA_GET_H */
