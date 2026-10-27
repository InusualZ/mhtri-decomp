/* `monster_icon_cell_get`, defined by `hud/layout.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_MONSTER_ICON_CELL_GET_H
#define MHTRI_HUD_MONSTER_ICON_CELL_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E1978 - the monster texture index kind byte `index` maps to; negative means "no icon". */
u8 monster_icon_cell_get(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_MONSTER_ICON_CELL_GET_H */
