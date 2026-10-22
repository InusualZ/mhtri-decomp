/* `lb_list_draw_ex`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_LIST_DRAW_EX_H
#define MHTRI_LOBBY_LB_LIST_DRAW_EX_H

#include "types.h"

struct LbList;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws list `list` on panel `panel` with its sprites, rows and per-row titles. */
void lb_list_draw_ex(struct LbList* list, u16 panel, const u16* sprites, const u16* rows, u16 help, u8 open, const u16* titles);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_LIST_DRAW_EX_H */
