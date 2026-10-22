/* menu/quest_info_text_get.h - the declarations of `quest_info_text_get`, `quest_info_paid_ck`, `quest_info_reward_ck`, `quest_info_mode_get`, which `menu/menu_item_sub.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em020_prog.cpp` calls them through. */
#ifndef MHTRI_MENU_QUEST_INFO_TEXT_GET_H
#define MHTRI_MENU_QUEST_INFO_TEXT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
s8* quest_info_text_get(s32 mode, s32 line);
u32 quest_info_paid_ck(void);
u8 quest_info_reward_ck(s32 mode);
s32 quest_info_mode_get(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_QUEST_INFO_TEXT_GET_H */
