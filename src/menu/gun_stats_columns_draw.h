/* `gun_stats_columns_draw`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_GUN_STATS_COLUMNS_DRAW_H
#define MHTRI_MENU_GUN_STATS_COLUMNS_DRAW_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the second detail page of an equipment set at layout sprite `lsp`. */
void gun_stats_columns_draw(struct _PLW* plw, struct _EQUIP* recs, u16 lsp);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_GUN_STATS_COLUMNS_DRAW_H */
