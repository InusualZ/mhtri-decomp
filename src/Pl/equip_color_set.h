/* `equip_color_set`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_COLOR_SET_H
#define MHTRI_PL_EQUIP_COLOR_SET_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Stores the RGB565 dye colour in the equipment record when its kind can be dyed. */
void equip_color_set(struct _EQUIP* equip, u16 color);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_COLOR_SET_H */
