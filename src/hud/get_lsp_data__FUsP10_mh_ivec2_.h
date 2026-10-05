/* `get_lsp_data`, owned by `hud/cockpit.cpp` (0x802E0550).  A leaf header (docs/plan.md 6.5 rule 2): the C
 * consumer `hud/fn_80324F7C.c` cannot take the C++ spelling, so it names the map's own symbol; the record it
 * fills is `struct _mh_ivec2_` (two s16).
 */
#ifndef MHTRI_HUD_GET_LSP_DATA_H
#define MHTRI_HUD_GET_LSP_DATA_H

#include "types.h"

/* `struct _mh_ivec2_` must be complete in the includer (the C consumer defines it before the include). */
void get_lsp_data__FUsP10_mh_ivec2_(u16 id, struct _mh_ivec2_* out);

#endif /* MHTRI_HUD_GET_LSP_DATA_H */
