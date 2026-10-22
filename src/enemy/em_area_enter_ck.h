/* enemy/em_area_enter_ck.h - the declarations of `em_area_enter_ck` and `em_area_entry_enter_ck`, which
 * `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signatures are the ones their callers use (an enemy work
 * record or an area-table entry, then the map id and the area, answering 1 when it may enter). */
#ifndef MHTRI_ENEMY_EM_AREA_ENTER_CK_H
#define MHTRI_ENEMY_EM_AREA_ENTER_CK_H

#include "types.h"
#include "enemy/EmAreaEntry.h"

#ifdef __cplusplus
extern "C" {
#endif
u32 em_area_enter_ck(struct _ENEMY_WORK* work, u8 map, u8 area_no);
u32 em_area_entry_enter_ck(EmAreaEntry* entry, u8 map, u8 area_no);
#ifdef __cplusplus
}
#endif

#endif
