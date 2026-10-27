/* `lb_scroll_rows_build`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SCROLL_ROWS_BUILD_H
#define MHTRI_LOBBY_LB_SCROLL_ROWS_BUILD_H

#include "types.h"

struct LbScrollWork;

#ifdef __cplusplus
extern "C" {
#endif

/* Rebuilds the scroll list's two side tables from its kind and owner. */
void lb_scroll_rows_build(struct LbScrollWork* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCROLL_ROWS_BUILD_H */
