/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s stage_area_cell_get */
#ifndef MHTRI_STAGE_STAGE_AREA_CELL_GET_H
#define MHTRI_STAGE_STAGE_AREA_CELL_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B0290 - area `area`'s word [i][j], the area-argument twin of `stage_cell_get` (GUESS name). */
u32 stage_area_cell_get(u8 i, u8 j, u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_AREA_CELL_GET_H */
