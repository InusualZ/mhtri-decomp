/* menu/card_equip_ids_build.h - leaf header (docs/plan.md 6.5 rule 2): `menu/menu_friendlist.cpp`'s `card_equip_ids_build`, for
 *   `menu/menu_plsearch.cpp`.  C linkage (the map row is a plain name).  GUESS name (the registry carries it). */
#ifndef MHTRI_MENU_CARD_EQUIP_IDS_BUILD_H
#define MHTRI_MENU_CARD_EQUIP_IDS_BUILD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _PLW;

/* 0x80441300 (0xB8): writes the eight equipment ids `plw` shows (each flagged 0x4000/0x8000 by its item checks) to
 * `ids`. */
void card_equip_ids_build(struct _PLW* plw, u16* ids);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_CARD_EQUIP_IDS_BUILD_H */
