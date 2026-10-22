/* `userdata_equip_mask_get`, defined by `fn_8004CAD8.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_EQUIP_MASK_GET_H
#define MHTRI_USERDATA_EQUIP_MASK_GET_H

#include "types.h"

struct Q_UserData;

#ifdef __cplusplus
extern "C" {
#endif

/* The equipment-wear mask of user data `user`: 1 or 2 by the hunter's sex, with 8 for a gunner (kind 0xB) or 4. */
u8 userdata_equip_mask_get(struct Q_UserData* user);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_EQUIP_MASK_GET_H */
