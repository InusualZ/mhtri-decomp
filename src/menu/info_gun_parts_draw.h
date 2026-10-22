/* menu/info_gun_parts_draw.h - leaf header (docs/plan.md 6.5 rule 2): `menu/menu_infomation.cpp`'s equipment detail rows, for `menu/movie.cpp`.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_INFO_GUN_PARTS_DRAW_H
#define MHTRI_MENU_INFO_GUN_PARTS_DRAW_H

#include "types.h"

struct _EQUIP;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8030A05C - draws the gun parts (`frame`, `barrel`, `stock`) at layout `id`. */
void info_gun_parts_draw(u16 id, struct _EQUIP* frame, struct _EQUIP* barrel, struct _EQUIP* stock);
/* 0x8030B5E8 - draws skill `skill`'s name row at layout `anchor` in `color`, offset by `off`. */
void skill_name_row_draw(u16 anchor, u16 skill, u32 color, const struct _mh_ivec2_* off);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_INFO_GUN_PARTS_DRAW_H */
