/*
 * `userdata_pl_look_set` (0x80049F7C), defined by `userdata_item.cpp` (leaf header, rule 2): copies the user
 * data's hunter look into a player work.
 */
#ifndef MHTRI_USERDATA_ITEM_USERDATA_PL_LOOK_SET_H
#define MHTRI_USERDATA_ITEM_USERDATA_PL_LOOK_SET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void userdata_pl_look_set(struct _PLW* plw);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_USERDATA_ITEM_USERDATA_PL_LOOK_SET_H */
