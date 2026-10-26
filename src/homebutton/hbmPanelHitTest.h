/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmPanelHitTest`, the one declaration
 * `homebutton/hbm_kb_cursor.cpp`'s hit-test override needs from it. */
#ifndef MHTRI_HOMEBUTTON_HBMPANELHITTEST_H
#define MHTRI_HOMEBUTTON_HBMPANELHITTEST_H

#include "types.h"

#ifdef __cplusplus
class HbmPanelView;

extern "C" {
/* 0x80547D00 (0x56C): returns the text position under the point (slot 0x184). */
u32 hbmPanelHitTest(HbmPanelView* self, f32 x, f32 y);
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMPANELHITTEST_H */
