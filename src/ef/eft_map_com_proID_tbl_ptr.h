/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `eft_map_com_proID_tbl_ptr` and `efm_map_com_proID_tbl_ptr`, the per-map
 * common effect proID tables `ef/eft_res.cpp` owns.  C linkage.
 */
#ifndef MHTRI_EF_EFT_MAP_COM_PROID_TBL_PTR_H
#define MHTRI_EF_EFT_MAP_COM_PROID_TBL_PTR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794A58 / 0x80794A54 - the per-map common eft and efm proID runs (0xFF ends each run). */
extern u8** eft_map_com_proID_tbl_ptr;
extern u8** efm_map_com_proID_tbl_ptr;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_MAP_COM_PROID_TBL_PTR_H */
