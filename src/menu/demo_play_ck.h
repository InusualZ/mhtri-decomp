/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_plsearch.cpp` symbols `quest/quest_entry.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_MENU_DEMO_PLAY_CK_H
#define MHTRI_MENU_DEMO_PLAY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8044FB98 - 1 while the local move work's +0x22E4 demo flag is set. */
u32 demo_play_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_DEMO_PLAY_CK_H */
