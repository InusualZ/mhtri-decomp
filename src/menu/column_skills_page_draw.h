/* `column_skills_page_draw`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_COLUMN_SKILLS_PAGE_DRAW_H
#define MHTRI_MENU_COLUMN_SKILLS_PAGE_DRAW_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the skill list of an equipment set at layout sprite `lsp`. */
void column_skills_page_draw(struct _PLW* plw, struct _EQUIP* recs, u16 lsp);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_COLUMN_SKILLS_PAGE_DRAW_H */
