/* `lb_item_info_draw`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_ITEM_INFO_DRAW_H
#define MHTRI_LOBBY_LB_ITEM_INFO_DRAW_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws an item info panel: rare colour, name, count (of `max`) and icon. */
void lb_item_info_draw(u16 id, s16 count, s32 max, u8 flag, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ITEM_INFO_DRAW_H */
