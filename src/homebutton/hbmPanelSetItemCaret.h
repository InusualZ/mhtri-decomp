/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmPanelSetItemCaret`, the one declaration
 * `homebutton/hbm_kb_cursor.cpp` needs from it. */
#ifndef MHTRI_HOMEBUTTON_HBMPANELSETITEMCARET_H
#define MHTRI_HOMEBUTTON_HBMPANELSETITEMCARET_H

#include "types.h"

#ifdef __cplusplus
class HbmPanelView;
class HbmPanelItemView;

extern "C" {
/* 0x80542C7C (0x110): moves the item's caret to `position` and books the move with the panel. */
void hbmPanelSetItemCaret(HbmPanelView* self, HbmPanelItemView* item, s32 position);
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMPANELSETITEMCARET_H */
