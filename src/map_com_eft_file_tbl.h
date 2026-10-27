/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `map_com_eft_file_tbl` and `map_com_efm_file_tbl`, the per-map common effect
 * archive tables `draw_shape.cpp` owns (registry names of lane stage-stg-w; GUESS).  C linkage.
 */
#ifndef MHTRI_MAP_COM_EFT_FILE_TBL_H
#define MHTRI_MAP_COM_EFT_FILE_TBL_H

#include "types.h"
#include "draw_shape.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80589BC8 / 0x80588E60 - one common eft / efm archive record per map (null where the map has none). */
extern EftResFile* map_com_eft_file_tbl[23];
extern EftResFile* map_com_efm_file_tbl[24];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MAP_COM_EFT_FILE_TBL_H */
