/* Leaf header for the two per-kind move-work accessors `ef/fn_800CDB2C.cpp` owns (0x800CFA90, 0x800CFAD0):
 * the owner's full header is included by units that carry their own spellings of these, so the
 * declarations stay out of it (rule 2).  The accessors hand out the per-kind work arrays (kind 2 players, kind 3 enemies).
 */
#ifndef MHTRI_EF_GET_MOVE_WORK_ADRS_H
#define MHTRI_EF_GET_MOVE_WORK_ADRS_H

#include "types.h"

#ifdef __cplusplus
/* The per-kind move-work array (enemy records for kind 3, player records for kind 2) and how many records
 * it holds; the map spells them `get_move_work_adrs__FUc` and `get_move_work_max__FUc`, so they are C++
 * free functions (rule 9). */
void* get_move_work_adrs(u8 index /* untyped: opaque handle - the base of a per-kind record array */);
u32 get_move_work_max(u8 index);
#endif

#endif /* MHTRI_EF_GET_MOVE_WORK_ADRS_H */
