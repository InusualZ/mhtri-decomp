/* `gun_level_bonus_tbl`, defined by `Pl/pl_act_step_data.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_GUN_LEVEL_BONUS_TBL_H
#define MHTRI_PL_GUN_LEVEL_BONUS_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805BFCE8 - the bowgun upgrade bonus table: rows of seven steps, one per bonus row a part record names (indexed `row * 7 + step`). */
extern s16 gun_level_bonus_tbl[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_GUN_LEVEL_BONUS_TBL_H */
