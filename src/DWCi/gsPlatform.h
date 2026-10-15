/*
 * DWCi/gsPlatform.h - the declarations `src/DWCi/gsPlatform.c` owns (docs/plan.md 6.5 rule 2): the GameSpy socket
 *   layer over SO, the platform time / sleep helpers and the memory hooks.  The resolver records
 *   (`DWCiHostEntry`, `DWCiHostAddr`) come from `DWCi/dwc_nasfunc.h`.
 */
#ifndef MHTRI_DWCI_GSPLATFORM_H
#define MHTRI_DWCI_GSPLATFORM_H

#include "types.h"
#include "SO/soi.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The GameSpy SDK's memory hooks: the malloc / free / realloc / memalign callbacks. */
/* untyped: caller-owned payload - a raw block */
typedef void* (*GSIMallocFn)(u32 size);
/* untyped: caller-owned payload - a raw block */
typedef void (*GSIFreeFn)(void* block);
/* untyped: caller-owned payload - a raw block */
typedef void* (*GSIReallocFn)(void* block, u32 size);
/* untyped: caller-owned payload - a raw block */
typedef void* (*GSIMemalignFn)(u32 align, u32 size);

int DWCi_socketCreate(int domain, int type, int protocol);
int DWCi_socketClose(int sock);
int DWCi_socketBind(int sock, const SOSockAddrIn* addr, int len);
/* 0x8050B980 - receive one datagram: (socket, buffer, capacity, flags, sender address, its length in/out);
 * -1 on failure. */
int DWCi_socketRecvFrom(int sock, u8* buf, int len, u32 flags, SOSockAddrIn* from, int* fromLen);
/* untyped: byte range - the datagram */
int DWCi_socketSendTo(int sock, const void* buf, int len, u32 flags, const SOSockAddrIn* to, int toLen);
int DWCi_socketGetLocalName(int sock, SOSockAddrIn* addr, int* len);
u32 DWCi_socketResolveAddress(char* host);
int DWCi_socketGetLastError(int sock);
/* 0x8050BB10 - polls one socket for reading / writing / errors (each flag pointer may be NULL); -1 on failure. */
int DWCi_socketSelect(int sock, int* readFlag, int* writeFlag, int* exceptFlag);
DWCiHostEntry* DWCi_socketLookupHost(char* host);
/* 0x8050C1D0 - non-zero while the socket has a datagram waiting. */
int DWCi_socketHasData(int sock);
int DWCi_socketIsUsable(int sock);
/* 0x8050C270 - the resolver record of this machine's own host (NULL when it cannot be read). */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void);
/* 0x8050C450 - non-zero when the address the pointer names is a private one (10/8, 172.16/12, 192.168/16). */
int DWCi_hostAddressIsPrivate(DWCiHostAddr* host);
void SocketStartUp(void);
void SocketShutDown(void);
u32 current_time(void);
void msleep(u32 msec);
/* 0x8050C580 - installs the four callbacks. */
void gsiMemoryCallbacksSet(GSIMallocFn mallocFn, GSIFreeFn freeFn, GSIReallocFn reallocFn, GSIMemalignFn memalignFn);
/* untyped: caller-owned payload - a raw block */
void* gsimalloc(u32 size);
/* untyped: caller-owned payload - a raw block */
void* gsirealloc(void* block, u32 size);
/* untyped: caller-owned payload - a raw block */
void gsifree(void* block);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GSPLATFORM_H */
