/* menu/put_frame_dialog_title.h - leaf header (docs/plan.md 6.5 rule 2): the frame and message-window helpers
 *   `menu/menu_message.cpp` defines, for `menu/menu_placeinfo.cpp` (the owner's header does not declare them yet).
 *   C linkage (the map rows are plain names); the parameter kinds follow the owner's prologues (`extsh` for the
 *   coordinates, a stack word for the title's ninth argument). */
#ifndef MHTRI_MENU_PUT_FRAME_DIALOG_TITLE_H
#define MHTRI_MENU_PUT_FRAME_DIALOG_TITLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802A7CC8: a titled dialog frame at x,y (shifted by the wide-screen offset `wide` picks); returns the content x. */
s32 put_frame_dialog_title(s16 x, s16 y, s16 width, s16 height, u32 color, u32 frame_color, u32 title_color, s8* title,
                           u32 wide);
/* 0x802A8218: the screen's header band with `text`. */
s16 put_frame_header(s16 x, s16 y, u32 color, s8* text, u32 wide);
/* 0x802A83E8: a column label box at x,y. */
void put_frame_label(s16 x, s16 y, s16 width, u32 color, s16 text_color, s8* text);
/* 0x802A8578: a row separator at x,y. */
void put_frame_separator(s16 x, s16 y, s16 width, s16 height, u32 flags);
/* 0x802A8788: the cursor bar over a row at x,y. */
void put_frame_cursor_bar(s16 x, s16 y, s16 width, s16 height, u32 color, u32 flags);
/* 0x802A8A08: the yes/no message window with `msg` and the two answers; `sel` is the highlighted answer. */
void put_message_window(s8* msg, s8* yes, s8* no, s32 sel, s16 size, u32 color);
/* 0x802A8DF4: steps `value` over `count` with the two masks of `keys`, wrapping, and reports the move in `moved`. */
s32 menu_cursor_wrap(s32 value, s32 count, u16 keys, u16 dec_mask, u16 inc_mask, s32 sfx, u16* moved);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_PUT_FRAME_DIALOG_TITLE_H */
