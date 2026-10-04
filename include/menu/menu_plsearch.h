/* Declarations owned by `src/menu/menu_plsearch.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_MENU_MENU_PLSEARCH_H
#define MHTRI_MENU_MENU_PLSEARCH_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct PatTerms;

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

/* 0x80450F8C / 0x80451024 / 0x80451040 / 0x8045106C / 0x804513A8 - the terms object's (`struct PatTerms`,
 * `Network/network_pat_control.h`) open, check request, update request, update cancel and progress read, which
 * the mediator's terms wrappers forward to (GUESS names from the bodies: the open stores the buffer pair and sets
 * the ready byte, the check request clears it and raises +0x0E, the update request raises +0x0F while ready, the
 * cancel drops +0x0F and raises +0x10, the read returns the +0xE0 count). */
/* untyped: byte range - the MEM2 buffer handed to the terms object */
void initPatTerms(struct PatTerms* terms, void* buffer, u32 size);
void requestPatTermsCheck(struct PatTerms* terms);
void requestPatTermsUpdate(struct PatTerms* terms);
void cancelPatTermsUpdate(struct PatTerms* terms);
u16 getPatTermsProgress(struct PatTerms* terms);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_PLSEARCH_H */
