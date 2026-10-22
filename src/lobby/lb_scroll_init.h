/* `lb_scroll_init`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SCROLL_INIT_H
#define MHTRI_LOBBY_LB_SCROLL_INIT_H

#include "types.h"

struct _PLW;
struct LbScrollWork;

#ifdef __cplusplus
extern "C" {
#endif

/* Clears the 0x80-byte scroll list `scroll` and sets its owner, kind and mode. */
void lb_scroll_init(struct _PLW* owner, struct LbScrollWork* scroll, u8 kind, u8 mode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SCROLL_INIT_H */
