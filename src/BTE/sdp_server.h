/* BTE/sdp_server.h - the server request entry point `BTE/sdp_server.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_SDP_SERVER_H
#define MHTRI_BTE_SDP_SERVER_H

#include "BTE/sdp_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804A2D64 - validates a client request and dispatches it to its handler. */
void sdp_server_handle_client_req(tCONN_CB* p_ccb, BT_HDR* p_msg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_SDP_SERVER_H */
