/*
 * DWCi/gt2Callback.h - the declarations `src/DWCi/gt2Callback.c` owns (docs/plan.md 6.5 rule 2): the GT2 callback
 *   wrappers the connection, main and message files call.  The GT2 records come from `DWCi/dwc_nasfunc.h`.
 */
#ifndef MHTRI_DWCI_GT2CALLBACK_H
#define MHTRI_DWCI_GT2CALLBACK_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

int DWCi_socketConnect(struct DWCiConn* conn);
s32 gti2ConnectAttemptCallback(DWCiConn* socket, DWCiReq* connection, u32 ip, u16 port, s32 latency, u8* message,
                               s32 len);
s32 gti2ConnectedCallback(DWCiReq* connection, s32 result, u8* message, s32 len);
s32 gti2ReceivedCallback(DWCiReq* connection, u8* message, s32 len, s32 reliable);
s32 gti2ClosedCallback(DWCiReq* connection, s32 reason);
s32 gti2PingCallback(DWCiReq* connection, s32 latency);
s32 gti2SendFilterCallback(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable);
s32 gti2ReceiveFilterCallback(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable);
/* untyped: byte range - the datagram */
int DWCi_requestSend(struct DWCiConn* conn, struct DWCiReq* req, u32 addr, u16 port, u32 a, void* buf,
                int len, u32 flag);
s32 gti2UnrecognizedMessageCallback(DWCiConn* socket, u32 ip, u16 port, u8* message, s32 len, s32* handled);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2CALLBACK_H */
