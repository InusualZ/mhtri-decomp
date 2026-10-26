/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmPanelAdvanceWord1A00`, the one
 * declaration `homebutton/hbm_kb_cursor.cpp`'s update needs from it. */
#ifndef MHTRI_HOMEBUTTON_HBMPANELADVANCEWORD1A00_H
#define MHTRI_HOMEBUTTON_HBMPANELADVANCEWORD1A00_H

#include "types.h"

#ifdef __cplusplus
class HbmPanelView;

extern "C" {
/* 0x80546648 (0x10): advances the panel's +0x1A00 word (the keyboard update's last step). */
void hbmPanelAdvanceWord1A00(HbmPanelView* self);
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMPANELADVANCEWORD1A00_H */
