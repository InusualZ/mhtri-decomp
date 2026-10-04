/* Declarations owned by `src/menu/menu_plsearch.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_MENU_MENU_PLSEARCH_H
#define MHTRI_MENU_MENU_PLSEARCH_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80449968 / 0x8044996C / 0x80449918 / 0x8044991C - the boot and account loading steps. */
void startBootLoad(void);

s32 pollBootLoad(void);

void startAccountLoad(void);

s32 pollAccountLoad(s16 frame);

/* 0x8044F520 - the lobby mailbox inside the lobby state block (+0x1FC4); `Network/net_session_close.cpp`
 * appends received mail to it (GUESS name: the body returns that address).  `NetMailBox` is that consumer's. */
struct NetMailBox* getLobbyMailBox(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_PLSEARCH_H */
