/* Leaf header (docs/plan.md 6.5 rule 2): `lb_sub11_send`, which `lobby/lb_companion_ui` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_LOBBY_LB_SUB11_SEND_H
#define MHTRI_LOBBY_LB_SUB11_SEND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80339478 - sends area event `index`/`bit` to the partner. */
void lb_sub11_send(u8 index, u8 bit);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_SUB11_SEND_H */
