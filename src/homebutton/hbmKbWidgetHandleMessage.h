/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmKbWidgetHandleMessage`, the keyboard
 * widget's message handler that `homebutton/hbm_kb_cursor.cpp`'s overrides fall back to. */
#ifndef MHTRI_HOMEBUTTON_HBMKBWIDGETHANDLEMESSAGE_H
#define MHTRI_HOMEBUTTON_HBMKBWIDGETHANDLEMESSAGE_H

#include "types.h"

#ifdef __cplusplus
class HbmPanelView;

extern "C" {
/* 0x8054E1C8 (0x35C): the keyboard widget's message handler (slot 0x18). */
void hbmKbWidgetHandleMessage(HbmPanelView* self, s32 id, u32* arg);
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMKBWIDGETHANDLEMESSAGE_H */
