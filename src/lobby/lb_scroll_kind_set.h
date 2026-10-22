/* `lb_scroll_kind_set`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SCROLL_KIND_SET_H
#define MHTRI_LOBBY_LB_SCROLL_KIND_SET_H

#include "types.h"

struct LbScrollWork;

#ifdef __cplusplus
extern "C" {
#endif

/* Sets the scroll list's kind and rebuilds its rows. */
void lb_scroll_kind_set(struct LbScrollWork* scroll, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCROLL_KIND_SET_H */
