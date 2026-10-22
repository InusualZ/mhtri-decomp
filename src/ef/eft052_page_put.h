/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft052.cpp`'s `eft052_page_put`, which `lobby/lb_screen_step.cpp` calls
 * and the owner's header does not declare.  C linkage (the map row is a plain name). */
#ifndef MHTRI_EF_EFT052_PAGE_PUT_H
#define MHTRI_EF_EFT052_PAGE_PUT_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8035921C - puts `count` of item `item` into the player's item pages (`flag` 1 announces it). */
void eft052_page_put(struct _PLW* plw, u16 item, s32 count, s32 flag);
/* 0x80358FF4 - half the item record's +0x0C value (the decoration make price). */
s32 eft052_item_half_get(u16 item);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT052_PAGE_PUT_H */
