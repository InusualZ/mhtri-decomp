/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/fn_80212810.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_LIST_DIALOG_DRAW_H
#define MHTRI_LOBBY_LB_LIST_DIALOG_DRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80214B30 - draws `list` as a framed dialog titled `title` at (x, y), right-aligned when both are 0.  GUESS name. */
void lb_list_dialog_draw(struct LbList* list, s16 x, s16 y, s8* title, u32 active);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_LIST_DIALOG_DRAW_H */
