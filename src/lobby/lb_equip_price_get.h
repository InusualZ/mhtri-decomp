/* `lb_equip_price_get`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_PRICE_GET_H
#define MHTRI_LOBBY_LB_EQUIP_PRICE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the shop price of equipment `id` of kind `kind` (twice the record's value). */
s32 lb_equip_price_get(s32 kind, s32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_PRICE_GET_H */
