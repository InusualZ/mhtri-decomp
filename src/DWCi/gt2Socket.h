/*
 * DWCi/gt2Socket.h - the declarations `src/DWCi/gt2Socket.c` owns that other units call (docs/plan.md 6.5 rule 2):
 *   the GT2 socket / connection core the `DWCi/dwc_nasfunc.c` GT2 entry points call (types in `DWCi/dwc_nasfunc.h`).
 */
#ifndef MHTRI_DWCI_GT2SOCKET_H
#define MHTRI_DWCI_GT2SOCKET_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

void DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode);
void DWCi_destroyConnection(DWCiConn* conn);
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value);
u32 DWCi_createRequest(DWCiConn* conn, DWCiReq** out, u32 addr, u16 port);
void DWCi_removeRequest(DWCiReq* req);
/* untyped: byte range - the datagram */
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len);
u32 DWCi_connectionTick(DWCiConn* conn);
void DWCi_connectionFlushRequests(DWCiConn* conn);
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port);
void DWCi_connectionShutdown(DWCiConn* conn);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2SOCKET_H */
