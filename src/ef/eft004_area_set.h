/* Leaf header (docs/plan.md 6.5 rule 2): `eft004_area_set`, which `ef/eft004_fx` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_EFT004_AREA_SET_H
#define MHTRI_EF_EFT004_AREA_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80101C74 - starts the eft004 area effect of kind `kind`. */
void eft004_area_set(s8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT004_AREA_SET_H */
