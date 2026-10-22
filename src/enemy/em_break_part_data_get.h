/* enemy/em_break_part_data_get.h - the declarations of `em_break_part_data_get`, which `enemy/fn_80138074.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em_common.cpp` calls them through. */
#ifndef MHTRI_ENEMY_EM_BREAK_PART_DATA_GET_H
#define MHTRI_ENEMY_EM_BREAK_PART_DATA_GET_H

#include "types.h"

struct _ENEMY_WORK;
struct EmBreakPartData;

#ifdef __cplusplus
extern "C" {
#endif
struct EmBreakPartData* em_break_part_data_get(struct _ENEMY_WORK* self, u8 index);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_BREAK_PART_DATA_GET_H */
