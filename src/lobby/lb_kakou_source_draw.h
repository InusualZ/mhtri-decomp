/* `lb_kakou_source_draw`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_KAKOU_SOURCE_DRAW_H
#define MHTRI_LOBBY_LB_KAKOU_SOURCE_DRAW_H

#include "types.h"

struct _EQUIP;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Draws the upgrade source of `equips` (kind `kind`) or, with no source, the reason line for `state`. */
void lb_kakou_source_draw(struct _EQUIP* equips, u8 kind, u16 state, struct _EQUIP* source, struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_KAKOU_SOURCE_DRAW_H */
