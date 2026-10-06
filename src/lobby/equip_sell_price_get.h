/*
 * lobby/equip_sell_price_get.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_80212810.cpp`'s
 *   `equip_sell_price_get` (0x80217C68), which that unit has no header for yet.
 */
#ifndef MHTRI_LOBBY_EQUIP_SELL_PRICE_GET_H
#define MHTRI_LOBBY_EQUIP_SELL_PRICE_GET_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* What the equipment record `equip` sells for: its base price (0x80217B04 over the kind and item id) plus the
 * decorations' worth.  GUESS name, from the result screen's sell totals. */
s32 equip_sell_price_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_EQUIP_SELL_PRICE_GET_H */
