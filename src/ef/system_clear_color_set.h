/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `system_clear_color_set` (registry name, map row `system_clear_color_set`). */
#ifndef MHTRI_EF_SYSTEM_CLEAR_COLOR_SET_H
#define MHTRI_EF_SYSTEM_CLEAR_COLOR_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CFBA0 (0x40): copies the RGBA colour `color` into `system_w`'s colour word (+0x7BC) and forces its alpha to 0xFF. */
void system_clear_color_set(u8* color);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_CLEAR_COLOR_SET_H */
