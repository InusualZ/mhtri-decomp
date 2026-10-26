/* `put_itemicon`, defined by `hud/layout.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_PUT_ITEMICON_H
#define MHTRI_HUD_PUT_ITEMICON_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E1288 - puts the item-icon square at (`x`, `y`) with no texture flag (`put_itemicon_tex` with 0). */
void put_itemicon(s16 x, s16 y, s16 size, u32 color, u8 tex_idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PUT_ITEMICON_H */
