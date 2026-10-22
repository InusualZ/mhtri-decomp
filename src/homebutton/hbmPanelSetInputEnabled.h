/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmPanelSetInputEnabled`, the one declaration `menu/menu_sysmsg.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of menu/menu_sysmsg.cpp). */
#ifndef MHTRI_HOMEBUTTON_HBMPANELSETINPUTENABLED_H
#define MHTRI_HOMEBUTTON_HBMPANELSETINPUTENABLED_H

#include "types.h"

struct HbmTextPanel;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8054CDBC (0x8): stores the input-enabled word into the text panel's +0x1988 */
void hbmPanelSetInputEnabled(struct HbmTextPanel* self, s32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMPANELSETINPUTENABLED_H */
