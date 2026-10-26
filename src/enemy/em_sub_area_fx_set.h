/* Leaf header (docs/plan.md 6.5 rule 2): `em_sub_area_fx_set`, `em_sub_area_fx2_set`, which `enemy/em_sub_state_prog` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_ENEMY_EM_SUB_AREA_FX_SET_H
#define MHTRI_ENEMY_EM_SUB_AREA_FX_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802F9994 - starts the area effect set `kind`. */
void em_sub_area_fx_set(u8 kind);

/* 0x802FA804 - starts the second area effect set `kind`. */
void em_sub_area_fx2_set(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_SUB_AREA_FX_SET_H */
