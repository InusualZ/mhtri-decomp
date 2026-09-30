/*
 * Leaf header (docs/plan.md 6.5 rule 2): `item_se_ck` (0x8004EB18, GUESS name), defined in the `src/fn_8004CAD8.cpp`
 * range: answers 1 when the item id is one whose pickup plays the extra sound effect.  Added with the net sync
 * (`hud/net_char_sync.cpp`), whose item-result receiver drives it like `Pl/fn_8025F088.cpp`.
 */
#ifndef MHTRI_FN_8004CAD8_ITEM_SE_CK_H
#define MHTRI_FN_8004CAD8_ITEM_SE_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 item_se_ck(u16 item_id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_8004CAD8_ITEM_SE_CK_H */
