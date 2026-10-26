/* Leaf header (docs/plan.md 6.5 rule 2): `eft023_area_fx_set`, `eft023_area_fx2_set`, which `ef/eft023_fx` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_EFT023_AREA_FX_SET_H
#define MHTRI_EF_EFT023_AREA_FX_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80116770 - starts the eft023 area effect. */
void eft023_area_fx_set(void);

/* 0x80116DA4 - starts the second eft023 area effect. */
void eft023_area_fx2_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT023_AREA_FX_SET_H */
