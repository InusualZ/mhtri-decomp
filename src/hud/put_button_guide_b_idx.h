/* `put_button_guide_b_idx`, defined by `hud/cockpit.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_PUT_BUTTON_GUIDE_B_IDX_H
#define MHTRI_HUD_PUT_BUTTON_GUIDE_B_IDX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Puts button guide `index` at layout sprite `lsp` when the pointer is not in use. */
void put_button_guide_b_idx(u16 lsp, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_PUT_BUTTON_GUIDE_B_IDX_H */
