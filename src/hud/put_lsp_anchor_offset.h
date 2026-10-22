/* Leaf header (docs/plan.md 6.5 rule 2) for `put_lsp_anchor_offset` (0x802E06B0), the `hud/cockpit.cpp` layout
 * helper that places sprite `id` at its anchor shifted by `offset`.  C linkage (the map row is unmangled); the
 * offset is a by-value `_mh_ivec2_` (the quest info pages copy it to a stack temporary before every call). */
#ifndef MHTRI_HUD_PUT_LSP_ANCHOR_OFFSET_H
#define MHTRI_HUD_PUT_LSP_ANCHOR_OFFSET_H

#include "types.h"
#include "hud/layout_types.h"

#ifdef __cplusplus
extern "C" {
/* 0x802E06B0 - the anchor position of sprite `id` plus `offset`, into `pos`. */
s32 put_lsp_anchor_offset(u16 id, _mh_ivec2_* pos, _mh_ivec2_ offset);
}
#endif

#endif /* MHTRI_HUD_PUT_LSP_ANCHOR_OFFSET_H */
