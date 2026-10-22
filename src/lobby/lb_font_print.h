/* lobby/lb_font_print.h - leaf header (docs/plan.md 6.5 rule 2): the lobby text helpers `lobby/fn_80212810.cpp`
 *   defines, for `menu/menu_placeinfo.cpp`.  C linkage (the map rows are plain names); the signatures follow the
 *   owner's bodies. */
#ifndef MHTRI_LOBBY_LB_FONT_PRINT_H
#define MHTRI_LOBBY_LB_FONT_PRINT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021537C: prints `str` at x,y in `color`, with a shadow under flag 4. */
void lb_font_print(s16 x, s16 y, s8* str, u32 color, u32 flags);
/* 0x80215A74: the page arrows of the lobby arrow table at x,y; `active` 0 dims them. */
void lb_page_arrow_put(s16 x, s16 y, s16 page, s16 pages, u16 moved, s32 active);
/* 0x8021800C: the speaker name for id `id`: the pig and cat names, the lobby string otherwise. */
s8* lb_speaker_name_get(u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_FONT_PRINT_H */
