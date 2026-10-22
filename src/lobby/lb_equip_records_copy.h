/* Leaf header (docs/plan.md 6.5 rule 2): the equipment-record helpers `lobby/fn_80219260.cpp` defines and
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map rows are plain names). */
#ifndef MHTRI_LOBBY_LB_EQUIP_RECORDS_COPY_H
#define MHTRI_LOBBY_LB_EQUIP_RECORDS_COPY_H

#include "types.h"

struct _PLW;
struct _EQUIP;
struct LbScrollWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8021AA78 - copies the player's equipment records to or from the three buffers (mode 0 save, 1 restore, 2 keep). */
void lb_equip_records_copy(struct _PLW* plw, u8 mode, struct _EQUIP* records, struct _EQUIP* record,
                           struct _EQUIP* records_b);
/* 0x80219530 - copies `equip` into the scroll list's compared record. */
void lb_equip_compare_set(struct LbScrollWork* scroll, struct _EQUIP* equip);
/* 0x80219590 - copies `equip` into the scroll list's previewed record. */
void lb_equip_base_set(struct LbScrollWork* scroll, struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_RECORDS_COPY_H */
