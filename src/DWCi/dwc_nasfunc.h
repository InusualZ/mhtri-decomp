/* Declarations owned by `src/DWCi/dwc_nasfunc.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_DWCI_DWC_NASFUNC_H
#define MHTRI_DWCI_DWC_NASFUNC_H

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "DWCi/DWCi_Np_CPUCopyFast.h"    /* the Np unit's own data (rule 2) */

/* Declarations moved here from `unsplit/DWCi.h, Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct DWCiConn;
struct DWCiReq;
struct DWCiAddrKey;

/* The resolver records `DWCi_parseAddress` and the NATNEG unit read: an entry whose +0x0C reaches the
 * host's address words through one more indirection (the SDK's `hostent`: name, aliases, type, length,
 * then the address list). */
typedef struct DWCiHostAddr {
    /* +0x00 */ u32 addr;
} DWCiHostAddr; /* size: 0x04 */

typedef struct DWCiHostEntry {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ DWCiHostAddr** hosts;
} DWCiHostEntry; /* size: 0x10 */

/* the DWC callbacks are installed as unprototyped pointers and the callee casts them back */
typedef void (*NetworkCallback)();

/* GameSpy GT2 transport (the SDK's `gt2*` API): a socket and a connection are opaque handles, and
 * `gt2Accept` / `gt2Connect` take the four-callback set `GameSpyInterfaceThread` installs on a
 * connection.  The callbacks keep the flat parameter lists the retail bodies read (GUESS on every
 * parameter name: the GT2 header spells `connection, result, message, size`). */
typedef u32 GT2Socket;
typedef u32 GT2Connection;
typedef struct GT2ConnectionCallbacks {
    /* +0x00 */ void (*connected_00)(s32 connection, s32 result, s32 message, s32 timeout);
    /* +0x04 */ void (*received_04)(u32 connection, s32 message, s32 size);
    /* +0x08 */ void (*closed_08)(u32 connection, s32 reason);
    /* +0x0C */ void (*ping_0C)(void);
} GT2ConnectionCallbacks;   /* size: 0x10 */


/* The service-locator result `DWC_SVLGetTokenAsync` fills (the SDK's `DWCSvlResult`: a status, the host and the
 * token).  size: 0x174 - the mediator memsets 0x174 bytes at its +0x2BA8 before handing it over, and
 * `getNASToken` returns +0x2BA8 + 0x45 (`svltoken`). */
typedef struct DWCSvlResult {
    /* +0x000 */ s32  status;
    /* +0x004 */ char svlhost[65];
    /* +0x045 */ char svltoken[301];
    /* +0x172 */ u8   pad_172[0x02];
} DWCSvlResult;

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

s32 DWC_SVLGetTokenAsync(const char* svl, DWCSvlResult* result);

s32 DWC_SVLProcess(void);

void DWCi_natProbeStart(const char* gameName);

s32 DWCi_natProbePoll(void);

/* ---- the transport / socket helpers --------------------------------------------------------- */

void* DWCi_listCreate(u32 a, u32 b, void* cb);
void DWCi_listDestroy(void* list);
u32 DWCi_listCount(void* list);
void* DWCi_listItem(void* list, u32 index);
void DWCi_listRemove(void* list, u32 index);
void* DWCi_tableCreate(u32 a, u32 b, u32 c, u32 d);
void DWCi_tableDestroy(void* table);
void DWCi_tableInsert(void* table, struct DWCiReq** entry);
void* DWCi_tableLookup(void* table, struct DWCiAddrKey** key);
void DWCi_tableRemove(void* table, void* key);
u32 DWCi_tableWalk(void* table, void* callback, void* arg);
int DWCi_socketBind(int sock, void* buf, int len);
int DWCi_socketSendTo(int sock, void* buf, int len, u32 flags, void* sa, int salen);
int DWCi_socketGetLocalName(int sock, void* sa, int* salen);
/* 0x8050AC70 - append a copy of the fixed-size record at `item` to the list. */
/* untyped: opaque handle (the list) and byte range (the record copied in) */
void DWCi_listAppend(void* list, void* item);
/* 0x8050AF20 - delete the list's `index`th record. */
/* untyped: opaque handle (the list) */
void DWCi_listDeleteAt(void* list, u32 index);
struct DWCiHostEntry* DWCi_socketLookupHost(char* host);
/* 0x8050C270 - the resolver record of this machine's own host (NULL when it cannot be read). */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void);
/* 0x8050C450 - non-zero when the address the pointer names is a usable (public) one. */
int DWCi_hostAddressIsUsable(DWCiHostAddr* host);
void* DWCi_malloc(u32 size);
void DWCi_free(void* p);
int DWCi_requestSend(struct DWCiConn* conn, struct DWCiReq* req, u32 addr, u16 port, u32 a, void* buf,
                int len, u32 flag);
int DWCi_requestFrame(struct DWCiReq* req, void* p, u32 len);
void DWCi_requestFree(void* p);

s32 gt2CreateSocket(GT2Socket* socket, const char* localAddress, u32 outgoingBufferSize, u32 incomingBufferSize,
                    NetworkCallback socketErrorCallback);
void gt2CloseSocket(GT2Socket socket);
void gt2Think(GT2Socket socket);
void gt2Listen(GT2Socket socket, NetworkCallback connectAttemptCallback);
s32 gt2Accept(GT2Connection connection, const GT2ConnectionCallbacks* callbacks);
/* untyped: byte range - the reject message */
void gt2Reject(GT2Connection connection, const char* message, s32 length);
/* untyped: byte range - the connect message */
s32 gt2Connect(GT2Socket socket, u32* connection, const char* remoteAddress, const void* message, u32 length,
               u32 timeout, const GT2ConnectionCallbacks* callbacks, s32 blocking);
/* untyped: byte range - the datagram */
s32 gt2Send(GT2Connection connection, const void* message, u32 length, s32 reliable);
void gt2CloseConnection(GT2Connection connection);
s32 gt2GetSocketSOCKET(GT2Socket socket);
void gt2SetUnrecognizedMessageCallback(GT2Socket socket, NetworkCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_NASFUNC_H */
