/* `place_model_load` and its two siblings, defined by `menu/menu_placeinfo.cpp` (leaf header, docs/plan.md 6.5 rule 2).
 * C linkage (plain map rows). */
#ifndef MHTRI_MENU_PLACE_MODEL_LOAD_H
#define MHTRI_MENU_PLACE_MODEL_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043CCBC - loads interior `id` into place slot `index` of (`map`, `area`). */
void place_model_load(u8 mode, u8 index, u8 id, u8 map, u8 area);
/* 0x8043CD98 - releases place slot `index`. */
void place_model_release(u8 index);
/* 0x8043D37C - non-zero once every requested place model has loaded. */
u32 place_model_ready_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACE_MODEL_LOAD_H */
