/* `equip_worn_records_refresh`, defined by `src/userdata_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_USERDATA_ITEM_EQUIP_WORN_RECORDS_REFRESH_H
#define MHTRI_USERDATA_ITEM_EQUIP_WORN_RECORDS_REFRESH_H

#include "types.h"

struct LbPage;

#ifdef __cplusplus
extern "C" {
#endif

/* Copies the worn equipment records into the player snapshot and refreshes page record `page`. */
/* untyped: the save block, seen by each caller through its own record view */
void equip_worn_records_refresh(struct LbPage* page, void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_EQUIP_WORN_RECORDS_REFRESH_H */
