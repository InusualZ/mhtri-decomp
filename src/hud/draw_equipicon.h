/* `draw_equipicon`, defined by `hud/layout.cpp` (leaf header, rule 2).  C++ linkage: the map spells it
 * `draw_equipicon__FRC10_SPR_DATA_P6_EQUIPPC10_mh_ivec2_`. */
#ifndef MHTRI_HUD_DRAW_EQUIPICON_H
#define MHTRI_HUD_DRAW_EQUIPICON_H

#include "types.h"

struct _SPR_DATA_;
struct _EQUIP;
struct _mh_ivec2_;

/* Draws the equipment record's icon with sprite block `spr` at `pos`. */
void draw_equipicon(const struct _SPR_DATA_& spr, struct _EQUIP* equip, const struct _mh_ivec2_* pos);

#endif /* MHTRI_HUD_DRAW_EQUIPICON_H */
