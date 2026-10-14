/* BTE/sdp_discovery.h - the client discovery entry points `BTE/sdp_discovery.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_DISCOVERY_H
#define MHTRI_BTE_SDP_DISCOVERY_H

#include "BTE/sdp_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A119C - starts the first request once the client channel is configured (GUESS name, the BTE source's). */
void sdp_disc_connected(tCONN_CB* p_ccb);

/* 0x804A11D0 - dispatches a server response to its parser. */
void sdp_disc_server_rsp(tCONN_CB* p_ccb, BT_HDR* p_msg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_DISCOVERY_H */
