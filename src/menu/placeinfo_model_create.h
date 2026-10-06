/* Leaf header (docs/plan.md 6.5 rule 2): the `menu/menu_placeinfo.cpp` symbol the lobby interior loader calls.  C linkage
 * (the map row is a plain name). */
#ifndef MHTRI_MENU_PLACEINFO_MODEL_CREATE_H
#define MHTRI_MENU_PLACEINFO_MODEL_CREATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8043D0F8 - builds placed model `index` from the loaded resource file `name` (GUESS name: it opens the file's
 * resources and sets an `MHchar` frame up for the model). */
s32 placeinfo_model_create(u8 index, char* name);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PLACEINFO_MODEL_CREATE_H */
