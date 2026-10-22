/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft052.cpp`'s `eft052_page_take`, which `lobby/lb_server_sel_trans.cpp`
 * calls and the owner's header does not declare.  C linkage (the map row is a plain name). */
#ifndef MHTRI_EF_EFT052_PAGE_TAKE_H
#define MHTRI_EF_EFT052_PAGE_TAKE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80359164 - takes `count` of item `item` out of the player's item pages into the user data (`flag` 1 announces it). */
void eft052_page_take(struct _PLW* plw, u16 item, s32 count, s32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_PAGE_TAKE_H */
