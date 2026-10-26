/* Leaf header (docs/plan.md 6.5 rule 2): `map_area_eft_list_tbl`, `map_area_eft_list2_tbl`, `map_land_grid_file_tbl`, `arena_a_land_grid_file_tbl`, `arena_b_land_grid_file_tbl`, which `draw_shape` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_MAP_AREA_EFT_LIST_TBL_H
#define MHTRI_MAP_AREA_EFT_LIST_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8058990C - the per-map tables of per-area effect lists. */
extern struct StageEftEntry** map_area_eft_list_tbl[];

/* 0x80588BF4 - the per-map tables of the second per-area effect lists. */
extern struct StageEftEntry** map_area_eft_list2_tbl[];

/* 0x805825B8 - the land-grid file of each map kind. */
extern struct StageResFile map_land_grid_file_tbl[];

/* 0x80582760 - the land-grid files of the arena map 21 areas. */
extern struct StageResFile arena_a_land_grid_file_tbl[];

/* 0x80582818 - the land-grid files of the arena map 22 areas. */
extern struct StageResFile arena_b_land_grid_file_tbl[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MAP_AREA_EFT_LIST_TBL_H */
