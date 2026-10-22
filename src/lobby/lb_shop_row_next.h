/* `lb_shop_row_next`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_SHOP_ROW_NEXT_H
#define MHTRI_LOBBY_LB_SHOP_ROW_NEXT_H

#include "types.h"

struct LbIdRow6;

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the index of the unlocked row of `table` that follows row `prev` (-1: the first) in the sort order `sort` gives equipment kind `kind`, or -1. */
s16 lb_shop_row_next(u8 kind, u8 sort, s32 prev, struct LbIdRow6* table);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SHOP_ROW_NEXT_H */
