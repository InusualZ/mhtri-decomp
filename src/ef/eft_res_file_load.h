/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `eft_res_file_load` (0x800F8634), the effect archive loader `ef/eft_res.cpp`
 * owns (a registry name of lane stage-stg-w; GUESS).  C linkage.
 */
#ifndef MHTRI_EF_EFT_RES_FILE_LOAD_H
#define MHTRI_EF_EFT_RES_FILE_LOAD_H

#include "types.h"
#include "draw_shape.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800F8634 - loads archive `file` into the effect slots `ids` (mode 0 eft, 3 efm), with the proID run `proids`
 * and the resource names `names`. */
void eft_res_file_load(EftResFile* file, u32 mode, u32 reserved, u8* ids, u8* proids, char** names);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_RES_FILE_LOAD_H */
