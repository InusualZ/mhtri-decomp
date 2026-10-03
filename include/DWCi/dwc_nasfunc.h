/* Declarations owned by `src/DWCi/dwc_nasfunc.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_DWCI_DWC_NASFUNC_H
#define MHTRI_DWCI_DWC_NASFUNC_H

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "DWCi/DWCi_Np_CPUCopyFast.h"    /* the Np unit's own data (rule 2) */

/* Declarations moved here from `include/unsplit/DWCi.h, Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct DWCiConn;
struct DWCiReq;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794348 - the 2-byte 0xFEFE protocol constant; 0x80794358/0x80794360/0x80794364 are the
 * "%s:%d", "%s" and ":%d" format strings the address helpers sprintf through. */
extern u16 DWCi_protocolMagic;

/* 0x80795818 - the result of the NAT-probe check `DWCi_natProbePoll` runs (1 = failed, 2 / 3 = the two
 * usable answers); the negotiator refuses to start while it reads 1.  Name and meaning are GUESSes
 * from that writer (it sends a probe, waits for its echo and stores 1, 2 or 3). */
extern s32 DWCi_natProbeStatus;

int DWCi_socketCreate(u32 a, u32 b, u32 c);

void DWCi_socketClose(int sock);

u32 DWCi_socketResolveAddress(char* host);

int DWCi_socketGetLastError(int sock);

/* 0x8050B980 - receive one datagram: (socket, buffer, capacity, flags, sender address, its length in/out);
 * -1 on failure. */
int DWCi_socketRecvFrom(int sock, u8* buf, int len, u32 flags, DWCiSockAddrIn* sa, int* salen);

/* 0x8050C1D0 - non-zero while the socket has a datagram waiting. */
int DWCi_socketHasData(int sock);

int DWCi_socketIsUsable(int sock);

void DWCi_platformInit(void);

void DWCi_platformCleanup(void);

u32 DWCi_getTick(void);

int DWCi_bufferAlloc(u8** dst, u32 size);

int DWCi_bufferFrame(u8** dst, u32 a, u32 b);

int DWCi_socketConnect(struct DWCiConn* conn);

int DWCi_requestIsTimedOut(struct DWCiReq* req, u32 now);

void DWCi_socketShutdown(struct DWCiConn* conn);

int DWCi_requestReconnect(struct DWCiConn* conn, u32 addr, u16 port);

int DWCi_requestRetry(struct DWCiConn* conn, u32 addr, u16 port, u32 flag);

int DWCi_requestBlockState(struct DWCiReq* req, u32 a, u32 b, u32* out);

int DWCi_requestFlush(struct DWCiReq* req);

/* GameSpy socket layer (DWC / GameSpyInterface) */
s32 DWC_NASLoginAsync(void);

s32 DWC_NASLoginProcess(void);

s32 DWC_SVLBegin(void);

void DWC_SVLEnd(void);

s32 DWC_SVLGetTokenAsync(const char* svl, s32 handle);

s32 DWC_SVLProcess(void);

void DWCi_natProbeStart(const char* gameName);

s32 DWCi_natProbePoll(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_NASFUNC_H */
