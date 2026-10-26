/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmTextPaneListenerOnEvent`, the one
 * declaration `homebutton/hbm_kb_cursor.cpp`'s pane handler needs from it. */
#ifndef MHTRI_HOMEBUTTON_HBMTEXTPANELISTENERONEVENT_H
#define MHTRI_HOMEBUTTON_HBMTEXTPANELISTENERONEVENT_H

#include "types.h"

#ifdef __cplusplus
class HbmTextPaneListener;
class HbmPaneComponentView;
struct HbmPointerInfo;

extern "C" {
/* 0x8054E894 (0x628): the text panel's pane listener (slot 0x18): scroll buttons, button states and the text box. */
void hbmTextPaneListenerOnEvent(HbmTextPaneListener* self, HbmPaneComponentView* component, u32 event,
                                HbmPointerInfo* info);
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMTEXTPANELISTENERONEVENT_H */
