/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s pop_map_kind_get */
#ifndef MHTRI_MENU_POP_MAP_KIND_GET_H
#define MHTRI_MENU_POP_MAP_KIND_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BE7F4 - the current map's kind, the map number itself when it has none (GUESS name). */
u8 pop_map_kind_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_POP_MAP_KIND_GET_H */
