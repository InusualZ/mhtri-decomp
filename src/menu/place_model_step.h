/* Leaf header (docs/plan.md 6.5 rule 2): `place_model_work_init`, `place_model_area_load`, `place_model_step`, `place_model_create_pending`, which `menu/menu_placeinfo` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_MENU_PLACE_MODEL_STEP_H
#define MHTRI_MENU_PLACE_MODEL_STEP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043C7AC - resets the placed-model work. */
void place_model_work_init(void);

/* 0x8043C840 - loads the placed models of area `areano` of map `mapno`. */
void place_model_area_load(u8 mapno, u8 areano);

/* 0x8043CFC0 - steps the placed models. */
void place_model_step(void);

/* the placed-model creation of the pending area. */
void place_model_create_pending(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACE_MODEL_STEP_H */
