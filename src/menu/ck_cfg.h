/* menu/ck_cfg.h - leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s clamped option reads, for `menu/movie.cpp`.  C linkage (the map rows are plain names). */
#ifndef MHTRI_MENU_CK_CFG_H
#define MHTRI_MENU_CK_CFG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BEA6C - option `index` of the current mode (VS/arena or normal), clamped to its maximum. */
u8 ck_cfg(u8 player, u8 index);
/* 0x803BE994 - player `player`'s arena option `index`, clamped to its maximum. */
u8 ck_arena_cfg(u8 player, u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_CK_CFG_H */
