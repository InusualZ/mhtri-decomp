/* `LbPutAnaPageArrow`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2).  C++ linkage: the map spells it
 * `LbPutAnaPageArrow__FUsUsssUsP10_mh_ivec2_bb`. */
#ifndef MHTRI_LOBBY_LBPUTANAPAGEARROW_H
#define MHTRI_LOBBY_LBPUTANAPAGEARROW_H

#include "types.h"

struct _mh_ivec2_;

/* Puts the stick page arrows of page `page` of `pages` at layout sprite `lsp`. */
void LbPutAnaPageArrow(u16 lsp, u16 kind, s16 page, s16 pages, u16 moved, struct _mh_ivec2_* pos, bool active,
                       bool flag);

#endif /* MHTRI_LOBBY_LBPUTANAPAGEARROW_H */
