/*
 * `_SPR_DATA_` - the sprite-data record the `draw_*` family takes by reference (`draw_itemicon_item_id(const _SPR_DATA_&, ...)`, `sprite_frame_apply`).
 * It has to be a global-scope type: the map's manglings of those callees name it, so a copy inside a namespace would change the symbol the call
 * binds.  Only +0x1C - the colour word `fn_801F60D4` overwrites - is named; `sprite_frame_apply` initialises the rest of the block.
 */
#ifndef MHTRI_HUD_SPR_DATA_H
#define MHTRI_HUD_SPR_DATA_H

#include "types.h"

/* size: 0x20 (approximate: the frame `fn_801F60D4` reserves for it) */
typedef struct _SPR_DATA_ {
    /* +0x00 */ u8 unused_0x00[0x1C];
    /* +0x1C */ u32 color_0x1C;
} _SPR_DATA_;

#endif /* MHTRI_HUD_SPR_DATA_H */
