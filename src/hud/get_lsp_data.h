/* Leaf header (docs/plan.md 6.5 rule 2): the `hud/cockpit.cpp` layout getter a lobby screen calls, on the
 * types of `hud/layout_types.h` (the page arrows and the pointer test are `hud/cockpit.h`'s). */
#ifndef MHTRI_HUD_GET_LSP_DATA_H
#define MHTRI_HUD_GET_LSP_DATA_H

#include "types.h"
#include "hud/layout_types.h"
#include "hud/ainpc_page_mode2_set.h"   /* the plain-C page helpers */

#ifdef __cplusplus
/* 0x802E0550 - the layout record of sprite `id` (its position into `out` when given). */
_SPR_DATA_* get_lsp_data(u16 id, _mh_ivec2_* out);
#endif

#endif /* MHTRI_HUD_GET_LSP_DATA_H */
