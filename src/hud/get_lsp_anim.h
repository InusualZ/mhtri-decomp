/* hud/get_lsp_anim.h - leaf header for `get_lsp_anim`, which the `hud/cockpit` unit's range holds with C linkage
 * (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_HUD_GET_LSP_ANIM_H
#define MHTRI_HUD_GET_LSP_ANIM_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E0714 - the animation table of layout sprite `id` (its record's `anim`). */
const _SPR_ANIM_* get_lsp_anim(u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_GET_LSP_ANIM_H */
