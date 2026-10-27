/* Leaf header (docs/plan.md 6.5 rule 2): `quest_element_item_use` (0x803B2C60), defined by `src/quest/quest_entry.cpp`. */
#ifndef MHTRI_QUEST_QUEST_ELEMENT_ITEM_USE_H
#define MHTRI_QUEST_QUEST_ELEMENT_ITEM_USE_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

u32 quest_element_item_use(struct _PLW* owner, u16 id, s32 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_QUEST_QUEST_ELEMENT_ITEM_USE_H */
