/* `sprite_ary_last_frame`, defined by `hud/layout.cpp` (leaf header, rule 2): the owner's `hud/layout.h` defines
 * `_mh_ivec2_` and cannot be included beside `menu/menu_item_page.h`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_HUD_SPRITE_ARY_LAST_FRAME_H
#define MHTRI_HUD_SPRITE_ARY_LAST_FRAME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E0C80 - the last animation frame of the sprite id run `ids`. */
u16 sprite_ary_last_frame(const u16* ids);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_SPRITE_ARY_LAST_FRAME_H */
