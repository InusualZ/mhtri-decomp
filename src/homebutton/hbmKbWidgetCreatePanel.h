/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmKbWidgetCreatePanel`, the keyboard
 * widget's create that `homebutton/hbm_kb_cursor.cpp`'s keyboard create runs first. */
#ifndef MHTRI_HOMEBUTTON_HBMKBWIDGETCREATEPANEL_H
#define MHTRI_HOMEBUTTON_HBMKBWIDGETCREATEPANEL_H

#include "types.h"

#ifdef __cplusplus
class HbmPanelView;
class HbmTextBufferSet;
struct MEMAllocator;

extern "C" {
/* 0x8054D0AC (0x530): creates the keyboard widget (panel, pane listener, pane manager, font, scroll buttons). */
void hbmKbWidgetCreatePanel(HbmPanelView* self, MEMAllocator* allocator, HbmTextBufferSet* buffers);
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMKBWIDGETCREATEPANEL_H */
