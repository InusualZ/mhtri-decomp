/*
 * `_SPR_DATA_` - the sprite-data record the `draw_*` family takes by reference (`draw_itemicon_item_id(const _SPR_DATA_&, ...)`, `sprite_frame_apply`).
 * It has to be a global-scope type: the map's manglings of those callees name it, so a copy inside a namespace would change the symbol the call
 * binds.  The colour word +0x1C is the one `fn_801F60D4` overwrites; `sprite_frame_apply` initialises the rest of the block.
 */
#ifndef MHTRI_HUD_SPR_DATA_H
#define MHTRI_HUD_SPR_DATA_H

#include "types.h"

struct _SPR_ANIM_;

/* size: 0x24 */
typedef struct _SPR_DATA_ {
    /* +0x00 */ s16 pos_x;          /* the record's own anchor (`hud/layout_types.h`'s `pos`) */
    /* +0x02 */ s16 pos_y;
    /* +0x04 */ s16 ofs_x;          /* added to the anchor (`hud/layout_types.h`'s `ofs_x`/`ofs_y`) */
    /* +0x06 */ s16 ofs_y;
    /* +0x08 */ s16 width;          /* the sprite's rectangle (`hud/layout_types.h` names them alike) */
    /* +0x0A */ s16 height;
    /* +0x0C */ u16 u_scale;        /* the texture scale, in percent (`hud/layout_types.h`'s `u_scale`/`v_scale`) */
    /* +0x0E */ u16 v_scale;
    /* +0x10 */ u8 unused_0x10[0xC];
    /* +0x1C */ u32 color_0x1C;
    /* +0x20 */ const struct _SPR_ANIM_* anim_0x20; /* the animation table the frame setters walk */
} _SPR_DATA_;

#endif /* MHTRI_HUD_SPR_DATA_H */
