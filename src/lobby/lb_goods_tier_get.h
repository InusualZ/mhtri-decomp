/* `lb_goods_tier_get`, defined by `lobby/fn_801E7530.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_GOODS_TIER_GET_H
#define MHTRI_LOBBY_LB_GOODS_TIER_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E9DFC - the trade-goods tier of goods `id`: 0 below 4, 1 below 8, else 2. */
u8 lb_goods_tier_get(u8 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_GOODS_TIER_GET_H */
