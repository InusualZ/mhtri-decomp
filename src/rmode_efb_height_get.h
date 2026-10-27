/* Leaf header (docs/plan.md 6.5 rule 2): `main.cpp`'s `rmode_efb_height_get`, for `font/flfnt.cpp`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_RMODE_EFB_HEIGHT_GET_H
#define MHTRI_RMODE_EFB_HEIGHT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80040280 (0xC): returns the render mode's EFB height (`Rmode->efbHeight`). */
u16 rmode_efb_height_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RMODE_EFB_HEIGHT_GET_H */
