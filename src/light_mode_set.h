/* Leaf header (docs/plan.md 6.5 rule 2): `light_mode_set`, `light_mode_rgb_set`, which `fn_80056F24` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_LIGHT_MODE_SET_H
#define MHTRI_LIGHT_MODE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80057810 - sets the light block's three mode bytes from three flags. */
void light_mode_set(u8 a, u8 b, u8 c);

/* 0x80057888 - sets the light block's three colour bytes. */
void light_mode_rgb_set(u8 r, u8 g, u8 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_MODE_SET_H */
