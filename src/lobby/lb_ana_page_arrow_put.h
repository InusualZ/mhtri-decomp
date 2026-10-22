/* `lb_ana_page_arrow_put`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_ANA_PAGE_ARROW_PUT_H
#define MHTRI_LOBBY_LB_ANA_PAGE_ARROW_PUT_H

#include "types.h"

struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Puts the stick page arrows with the held flag set (`LbPutAnaPageArrow`). */
void lb_ana_page_arrow_put(s32 lsp, s32 kind, s32 page, s32 pages, s32 moved, struct _mh_ivec2_* pos, bool active);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_ANA_PAGE_ARROW_PUT_H */
