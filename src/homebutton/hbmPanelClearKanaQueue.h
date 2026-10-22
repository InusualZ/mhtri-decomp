/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmPanelClearKanaQueue`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_HOMEBUTTON_HBMPANELCLEARKANAQUEUE_H
#define MHTRI_HOMEBUTTON_HBMPANELCLEARKANAQUEUE_H

#include "types.h"

struct HbmTextPanel;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8054CDC4 (0x8): clears the text panel's kana composition queue (+0x19C4) */
void hbmPanelClearKanaQueue(struct HbmTextPanel* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMPANELCLEARKANAQUEUE_H */
