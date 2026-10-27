/*
 * homebutton/hbm_kb_scroller.h - the band's scroller class, shared by homebutton/hbm_kb_widget.cpp and
 * homebutton/hbm_text_panel.cpp (the owner of its class table).
 */
#ifndef HOMEBUTTON_HBM_KB_SCROLLER_H
#define HOMEBUTTON_HBM_KB_SCROLLER_H

#include "types.h"

#ifdef __cplusplus
class HbmKbScrollListener;

/* The scroller of the prediction-candidate list (homebutton/hbm_kb_widget.cpp, +0x18 of the list) and of the text
 * panel (homebutton/hbm_text_panel.cpp, +0x1964); its class table (0x8064F338) is in homebutton/hbm_text_panel.cpp's
 * .data.  Slot 0x08 starts a scroll, 0x0C steps it, 0x10 reads its offset, 0x14 whether it moves, 0x20 resets it.
 * Its constructor only stores the table.
 * size: 0x10 (approximation: only the vptr is touched) */
class HbmKbScrollerView {
public:
    /* slot +0x008 */ virtual void start(HbmKbScrollListener* listener, f32 from, f32 to, s32 words, f32 frames);
    /* slot +0x00C */ virtual void step();
    /* slot +0x010 */ virtual f32 getOffset();
    /* slot +0x014 */ virtual bool isMoving();
    /* slot +0x018 */ virtual void v_0x018();
    /* slot +0x01C */ virtual void v_0x01C();
    /* slot +0x020 */ virtual void reset();

    /* +0x04 */ u8 unused_0x04[0xC];
}; /* size: 0x10 */
#endif

#endif
