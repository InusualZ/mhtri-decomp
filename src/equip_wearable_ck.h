/* `equip_wearable_ck`, defined by `src/fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EQUIP_WEARABLE_CK_H
#define MHTRI_EQUIP_WEARABLE_CK_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 when the player can wear the equipment record. */
/* untyped: the save block, seen by each caller through its own record view */
u32 equip_wearable_ck(void* userdata, const struct _EQUIP* rec);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EQUIP_WEARABLE_CK_H */
