/*
 * DWCi/fn_805113B0.h - the declarations `src/DWCi/fn_805113B0.c` owns that other units call
 * (docs/plan.md 6.5 rule 2): the address helpers the GameSpy interface unit formats its connect
 * address with.  A consumer includes this header instead of declaring the symbols itself.  Keep it
 * minimal - the signatures are the ones `fn_805113B0.c` defines.
 */
#ifndef MHTRI_DWCI_FN_805113B0_H
#define MHTRI_DWCI_FN_805113B0_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The GT2 socket / connection core the `DWCi/dwc_nasfunc.cpp` GT2 entry points call (types in
 * `DWCi/dwc_nasfunc.h`). */
u32 DWCi_sendControlFrame(DWCiReq* req);
u32 DWCi_appendTransfer(DWCiReq* req, DWCiXfer* xfer);
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag);
void DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode);
void DWCi_destroyConnection(DWCiConn* conn);
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value);
u32 DWCi_createRequest(DWCiConn* conn, DWCiReq** out, u32 addr, u16 port);
void DWCi_removeRequest(DWCiReq* req);
/* untyped: byte range - the datagram */
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len);
u32 DWCi_connectionTick(DWCiConn* conn);
void DWCi_connectionFlushRequests(DWCiConn* conn);
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort);
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port);
void DWCi_connectionShutdown(DWCiConn* conn);

/* host-to-network byte order for a port */
u16 DWCi_htons(u16 port);
/* formats an IPv4 address and port as text into `buf` (or an internal buffer when `buf` is NULL) */
char* DWCi_formatAddress(u32 addr, u16 port, char* buf);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/DWCi.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x806308A8 - the GameSpy stats server host name, a 64-byte buffer `DWC_Init` rewrites for the server type
 * (the unit's `.data` run 0x806308A8..0x806308E8). GUESS on the name. */
extern char DWCi_statsServerHostname[64];

extern char DWCi_addressFormat[6];

extern char DWCi_addressFormatHost[3];

extern char DWCi_addressFormatPort[4];

/* 0x80795820 - the index of the two-buffer address-string ring `DWCi_formatAddress` writes into the
 * 0x807625C0 buffers (`DWCi_addressRing`, declared by the NATNEG unit's header - the word falls
 * inside its `.bss` run). */
extern u32 DWCi_addressRingIndex;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_FN_805113B0_H */
