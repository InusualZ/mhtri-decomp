/* enemy/em_user_data_items_get.h - the declaration of `em_user_data_items_get`, which `enemy/fn_80138074.cpp` owns
 * (docs/plan.md 6.5 rule 2).  GUESS name (the owner's body). */
#ifndef MHTRI_ENEMY_EM_USER_DATA_ITEMS_GET_H
#define MHTRI_ENEMY_EM_USER_DATA_ITEMS_GET_H

#include "types.h"

struct _ENEMY_WORK;
struct UserDataItem;

#ifdef __cplusplus
extern "C" {
#endif
/* 0x8013A6C0: the enemy data's user-data item list (ended by an item whose node is -1), NULL without data. */
struct UserDataItem* em_user_data_items_get(struct _ENEMY_WORK* work);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_USER_DATA_ITEMS_GET_H */
