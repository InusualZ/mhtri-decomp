/* Leaf header (docs/plan.md 6.5 rule 2): `homebutton/hbm_text_panel.cpp`'s `hbmPanelIsReadyToCommit`, the one
 * declaration `homebutton/hbm_value.cpp` needs. */
#ifndef MHTRI_HOMEBUTTON_HBMPANELISREADYTOCOMMIT_H
#define MHTRI_HOMEBUTTON_HBMPANELISREADYTOCOMMIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HbmTextPanel;

/* 0x8054CA40 (0x108): Whether the panel's input helper is empty and ready and the helper its mode selects is idle. */
s32 hbmPanelIsReadyToCommit(struct HbmTextPanel* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HOMEBUTTON_HBMPANELISREADYTOCOMMIT_H */
