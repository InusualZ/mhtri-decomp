/* `skill_table_page_count`, defined by `menu/menu_infomation.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_SKILL_TABLE_PAGE_COUNT_H
#define MHTRI_MENU_SKILL_TABLE_PAGE_COUNT_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8030A1DC - the number of skill-table pages of the player `plw` wearing `equip` with the bowgun parts `part_a`/`part_b`. */
s32 skill_table_page_count(struct _PLW* plw, struct _EQUIP* equip, struct _EQUIP* part_a, struct _EQUIP* part_b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_SKILL_TABLE_PAGE_COUNT_H */
