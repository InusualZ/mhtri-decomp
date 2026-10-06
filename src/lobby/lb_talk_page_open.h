/*
 * lobby/lb_talk_page_open.h - leaf header (docs/plan.md 6.5 rule 2) for `lobby/fn_80219260.cpp`'s talk-page entry
 *   points; the owner has no header of its own and defines them inside its `extern "C"` blocks.
 */
#ifndef MHTRI_LOBBY_LB_TALK_PAGE_OPEN_H
#define MHTRI_LOBBY_LB_TALK_PAGE_OPEN_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021CBB0 - opens talk page `page` of the NPC page table (clears the page work and loads its entry).  GUESS. */
void lb_talk_page_open(s8 page);
/* 0x8021D160 - the page's current menu value.  GUESS name. */
u32 lb_talk_page_value_get(void);
/* 0x8021D5A8 - switches the page to menu mode `mode`.  GUESS name. */
s32 lb_talk_page_mode_set(s32 mode);
/* 0x8021D5BC - switches the page back to menu mode 1; 1 or 2 once the page has closed.  GUESS name. */
s32 lb_talk_page_mode_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_TALK_PAGE_OPEN_H */
