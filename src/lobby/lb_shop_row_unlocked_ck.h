/* `lb_shop_row_unlocked_ck`, defined by `lobby/fn_801E0ADC.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SHOP_ROW_UNLOCKED_CK_H
#define MHTRI_LOBBY_LB_SHOP_ROW_UNLOCKED_CK_H

#include "types.h"

struct LbIdRow6;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E40A4 - whether the shop goods row `row` is on sale yet (1 when it is). */
s32 lb_shop_row_unlocked_ck(struct LbIdRow6* row);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SHOP_ROW_UNLOCKED_CK_H */
