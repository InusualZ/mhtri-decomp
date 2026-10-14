/* BTE/sdp_main.h - the SDP control block and the connection entry points `BTE/sdp_main.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_MAIN_H
#define MHTRI_BTE_SDP_MAIN_H

#include "BTE/sdp_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x8073D360 - the SDP control block (GUESS name sdp_cb, the BTE source's). */
extern tSDP_CB sdp_cb;

/* 0x804A21F8 - clears the control block, registers SDP's security levels and its L2CAP PSM. */
void sdp_init(void);

/* 0x804A2A70 - allocates a CCB and opens the L2CAP channel to `p_bd_addr`; NULL when that fails. */
tCONN_CB* sdp_conn_originate(u8* p_bd_addr);

/* 0x804A2B84 - closes the CCB's channel (or reports `reason` at once while it is still connecting). */
void sdp_disconnect(tCONN_CB* p_ccb, u16 reason);

/* 0x804A2CDC - the inactivity timeout of a CCB: disconnects it and reports a failed connection. */
void sdp_conn_timeout(tCONN_CB* p_ccb);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_MAIN_H */
