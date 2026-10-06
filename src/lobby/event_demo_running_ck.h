/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp` symbols `quest/quest_entry.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_LOBBY_EVENT_DEMO_RUNNING_CK_H
#define MHTRI_LOBBY_EVENT_DEMO_RUNNING_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C482C - non-zero while an event demo runs. */
u32 event_demo_running_ck(void);
/* 0x803C4AA0 - marks the event demo running. */
void demo_set_running(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_EVENT_DEMO_RUNNING_CK_H */
