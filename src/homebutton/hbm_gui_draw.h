/*
 * homebutton/hbm_gui_draw.h - the GX draw helpers owned by `homebutton/hbm_gui_draw.cpp`.
 */
#ifndef HOMEBUTTON_HBM_GUI_DRAW_H
#define HOMEBUTTON_HBM_GUI_DRAW_H

#include "types.h"
#include "gx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Draws a filled quad over the rectangle, scaled by `scale` about its centre at depth z. */
void hbmGuiDrawQuad(const GXColor& color, f32 left, f32 top, f32 right, f32 bottom, f32 z, f32 scale);

/* Draws a vertical (left == right) or horizontal segment as a quad width/6 thick at depth z. */
void hbmGuiDrawLine(u8 width, const GXColor& color, f32 left, f32 top, f32 right, f32 bottom, f32 z);

#ifdef __cplusplus
}
#endif

#endif
