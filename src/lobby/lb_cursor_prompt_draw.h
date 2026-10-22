/* lobby/lb_cursor_prompt_draw.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/fn_80212810.cpp`'s
 *   `lb_cursor_prompt_draw`, for a consumer that cannot include the lobby band.  C linkage (the map row is a plain
 *   name); the signature is the one `unsplit/lobby.h` already spells. */
#ifndef MHTRI_LOBBY_LB_CURSOR_PROMPT_DRAW_H
#define MHTRI_LOBBY_LB_CURSOR_PROMPT_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021565C: draws the cursor prompt `kind` at the position `pos` points at (its x and y halfwords). */
s32 lb_cursor_prompt_draw(u8 kind, s16* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CURSOR_PROMPT_DRAW_H */
