/* Leaf header (docs/plan.md 6.5 rule 2): `pl_land_grid_a_load`, `pl_land_grid_b_load`, which `Pl/pl_coll` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_PL_PL_LAND_GRID_A_LOAD_H
#define MHTRI_PL_PL_LAND_GRID_A_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802914CC - loads land grid A of area `area` from the pack entry `data`. */
/* untyped: byte range - the pack entry */
void pl_land_grid_a_load(u8* data, u8 area);

/* 0x8029135C - loads land grid B of area `area` from the pack entry `data`. */
/* untyped: byte range - the pack entry */
void pl_land_grid_b_load(u8* data, u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_LAND_GRID_A_LOAD_H */
