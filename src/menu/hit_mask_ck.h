/* Leaf header for `hit_mask_ck`, which `menu/menu_item.cpp` owns (0x8029F51C): the owner's full header redefines
 * `_HIT_W` and pulls `pl.h`, so a consumer that holds a hit record without its type includes this instead (rule 2).
 */
struct _HIT_W;

#ifndef MHTRI_MENU_HIT_MASK_CK_H
#define MHTRI_MENU_HIT_MASK_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 when the hit record's mask byte has a bit of `mask` set. */
u32 hit_mask_ck(struct _HIT_W* hit, u32 mask);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_MASK_CK_H */
