/* Leaf header (docs/plan.md 6.5 rule 2): the `lobby/lb_companion_ui.cpp` symbol `quest/quest_entry.cpp` calls,
 * for a consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_SUB18_SEND_H
#define MHTRI_LOBBY_LB_SUB18_SEND_H

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339A48 - sends the lobby's sub-18 message (the quest finish step's hand-off). */
void lb_sub18_send(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB18_SEND_H */
