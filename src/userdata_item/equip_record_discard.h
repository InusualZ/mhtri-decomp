/* `equip_record_discard`, defined by `src/userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_EQUIP_RECORD_DISCARD_H
#define MHTRI_USERDATA_ITEM_EQUIP_RECORD_DISCARD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Clears box record `id` and the saved sets naming it unless it is worn; returns 1 when cleared. */
/* untyped: the save block, seen by each caller through its own record view */
u32 equip_record_discard(void* userdata, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_EQUIP_RECORD_DISCARD_H */
