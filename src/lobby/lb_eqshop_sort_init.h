/* Leaf header (docs/plan.md 6.5 rule 2): the equipment shop's sort chooser, frame draws and sprite tables that
 * `lobby/fn_801E0ADC.cpp` defines and `lobby/lb_screen_step.cpp` reuses.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_EQSHOP_SORT_INIT_H
#define MHTRI_LOBBY_LB_EQSHOP_SORT_INIT_H

#include "types.h"

struct LbShopSortWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E48C0 - sets the sort chooser up for equipment kind `kind`. */
void lb_eqshop_sort_init(struct LbShopSortWork* sort, u8 kind);
/* 0x801E4CD8 - steps the sort chooser: 1 decided, 2 cancelled. */
s32 lb_eqshop_sort_step(struct LbShopSortWork* sort);
/* 0x801E63C4 - draws the sort chooser. */
void lb_eqshop_sort_draw(struct LbShopSortWork* sort, u8 kind);
/* 0x801E69C4 - draws the scroll list's frame. */
void lb_eqshop_scroll_frame_draw(u8 kind);

/* 0x805B7B60 - the goods window's frame sprites. */
extern u16 lb_eqshop_goods_frame[16];
/* 0x805B7B80 - the goods window's title sprite of each equipment kind. */
extern u16 lb_eqshop_goods_titles[16];
/* 0x805B7BA0 - the goods rows' layout ids. */
extern u16 lb_eqshop_goods_row_lsp[8];
/* 0x80791B60 - the goods window's second frame sprite run. */
extern u16 lb_eqshop_goods_frame_sub[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQSHOP_SORT_INIT_H */
