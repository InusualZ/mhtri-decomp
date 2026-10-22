/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `menu/sysmsg_text.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_MENU_NPC_TALK_NAME_TBL_H
#define MHTRI_MENU_NPC_TALK_NAME_TBL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8060DDA0 - per message language: the talk speakers' names, then the four choice strings.  GUESS name. */
extern char** npc_talk_name_tbl[6];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_NPC_TALK_NAME_TBL_H */
