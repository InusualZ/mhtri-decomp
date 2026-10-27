/* Leaf header (docs/plan.md 6.5 rule 2): `pl_rig_style_byte_get` (0x80223A70), owned by `lobby/lb_equip_page.cpp`.  C
 * linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_PL_RIG_STYLE_BYTE_GET_H
#define MHTRI_LOBBY_PL_RIG_STYLE_BYTE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80223A70 - the style byte of row `index` for set kind `kind` (4 hair, 5 face) of style set `table`, 0 otherwise. */
u8 pl_rig_style_byte_get(u8 table, s32 kind, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_PL_RIG_STYLE_BYTE_GET_H */
