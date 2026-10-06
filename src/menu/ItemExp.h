/*
 * menu/ItemExp.h - leaf header (docs/plan.md 6.5 rule 2) for two of `menu/menu_item.cpp`'s C++ free functions; the
 *   owner's full header pulls `pl.h`, whose `MHchar` cannot sit beside `sound/mhchar.h`.
 */
#ifndef MHTRI_MENU_ITEMEXP_H
#define MHTRI_MENU_ITEMEXP_H

#include "types.h"

#ifdef __cplusplus
struct _mh_ivec2_;

/* 0x8029F654 - item `id`'s description text (`ItemExp__FUs`). */
u32 ItemExp(u16 id);
/* 0x802A2564 - draws the menu cursor sprites `rows` at `pos` (`put_menu_cursor__FPUsUsPC10_mh_ivec2_`). */
void put_menu_cursor(u16* rows, u16 index, const struct _mh_ivec2_* pos);
#endif

#endif /* MHTRI_MENU_ITEMEXP_H */
