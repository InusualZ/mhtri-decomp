/* Leaf header (docs/plan.md 6.5 rule 2): the declaration of `em_area_entry_tbl` (`.bss` 0x806A54E0), the enemy area
 * entry table `enemy/enemy_control.cpp` owns; the shape is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_AREA_ENTRY_TBL_H
#define MHTRI_ENEMY_EM_AREA_ENTRY_TBL_H

#include "types.h"
#include "enemy/EmAreaEntry.h"

#ifdef __cplusplus
extern "C" {
#endif
extern EmAreaEntry em_area_entry_tbl[32][4];
#ifdef __cplusplus
}
#endif

#endif
