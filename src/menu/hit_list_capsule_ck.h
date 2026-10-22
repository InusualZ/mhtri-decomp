/* menu/hit_list_capsule_ck.h - the declaration of `hit_list_capsule_ck`, which `menu/menu_item.cpp` owns (docs/plan.md
 * 6.5 rule 2); a leaf header because `menu/menu_item.h` does not see the `nw4r::math` capsule. */
#ifndef MHTRI_MENU_HIT_LIST_CAPSULE_CK_H
#define MHTRI_MENU_HIT_LIST_CAPSULE_CK_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8029E2F8 - walks the hit list at +0x08 (0x28-byte entries, -1 terminated) and tests each against the capsule
 * (GUESS name). */
s32 hit_list_capsule_ck(u8* hit, nw4r::math::CAPSULE* capsule, VEC3* pos, u16 flag);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_LIST_CAPSULE_CK_H */
