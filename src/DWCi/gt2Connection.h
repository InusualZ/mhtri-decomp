/*
 * DWCi/gt2Connection.h - the declarations `src/DWCi/gt2Connection.c` owns (docs/plan.md 6.5 rule 2): the GT2
 *   connection setup, send, think and close functions the main, message and socket files call.  The GT2 records come
 *   from `DWCi/dwc_nasfunc.h`.
 */
#ifndef MHTRI_DWCI_GT2CONNECTION_H
#define MHTRI_DWCI_GT2CONNECTION_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 gti2NewOutgoingConnection(DWCiConn* socket, DWCiReq** connection, u32 ip, u16 port);
s32 gti2NewIncomingConnection(DWCiConn* socket, DWCiReq** connection, u32 ip, u16 port);
s32 gti2StartConnectionAttempt(DWCiReq* connection, u8* message, s32 len, const GT2ConnectionCallbacks* callbacks);
s32 gti2AcceptConnection(DWCiReq* connection, const GT2ConnectionCallbacks* callbacks);
void gti2RejectConnection(DWCiReq* connection, u8* message, s32 len);
/* untyped: byte range - the datagram */
int DWCi_requestFrame(struct DWCiReq* req, void* p, u32 len);
s32 gti2CheckTimeout(DWCiReq* connection, u32 now);
int DWCi_requestIsTimedOut(struct DWCiReq* req, u32 now);
void gti2CloseConnection(DWCiReq* connection, s32 hard);
void gti2ConnectionClosed(DWCiReq* connection);
/* untyped: caller-owned payload - the connection record */
void DWCi_requestFree(void* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2CONNECTION_H */
