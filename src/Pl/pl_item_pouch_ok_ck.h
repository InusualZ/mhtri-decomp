/* `pl_item_pouch_ok_ck`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_PL_ITEM_POUCH_OK_CK_H
#define MHTRI_PL_PL_ITEM_POUCH_OK_CK_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* Whether the player may carry item `id` in the pouch now (always outside a quest). */
s32 pl_item_pouch_ok_ck(struct _PLW* plw, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ITEM_POUCH_OK_CK_H */
