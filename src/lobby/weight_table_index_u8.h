/* lobby/weight_table_index_u8.h - leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_npc.cpp`'s
 *   `weight_table_index_u8` (0x80212724), for `lobby/fn_802FA9A0.cpp`.  C linkage; the signature is the owner's
 *   definition.  GUESS name (registry). */
#ifndef MHTRI_LOBBY_WEIGHT_TABLE_INDEX_U8_H
#define MHTRI_LOBBY_WEIGHT_TABLE_INDEX_U8_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80212724: the index of the first `table` weight whose running sum exceeds `value` (`count` when none does). */
s16 weight_table_index_u8(u8* table, s16 count, u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_WEIGHT_TABLE_INDEX_U8_H */
