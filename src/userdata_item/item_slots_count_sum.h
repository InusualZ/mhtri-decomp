/* `item_slots_count_sum`, defined by `src/userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_ITEM_SLOTS_COUNT_SUM_H
#define MHTRI_USERDATA_ITEM_ITEM_SLOTS_COUNT_SUM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The total count of item `id` over `count` slots. */
/* untyped: the caller's slot run, seen through its own record view */
s32 item_slots_count_sum(u16 id, void* slots, s32 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_ITEM_SLOTS_COUNT_SUM_H */
