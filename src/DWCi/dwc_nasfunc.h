/* Declarations owned by `src/DWCi/dwc_nasfunc.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_DWCI_DWC_NASFUNC_H
#define MHTRI_DWCI_DWC_NASFUNC_H

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "SO/soi.h"
#include "DWCi/DWCi_Np_CPUCopyFast.h"    /* the Np unit's own data (rule 2) */

/* Declarations moved here from `unsplit/DWCi.h, Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct GTI2Buffer;
typedef struct DWCiConn DWCiConn; /* GT2's socket, size: 0x4C */
typedef struct DWCiReq DWCiReq;   /* GT2's connection, size: 0xA0 */

/* The resolver records `DWCi_parseAddress` and the NATNEG unit read: SO's `hostent` and its address words. */
typedef SOInAddr DWCiHostAddr;
typedef SOHostEnt DWCiHostEntry;

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

int DWCi_socketCreate(int domain, int type, int protocol);

int DWCi_socketClose(int sock);

u32 DWCi_socketResolveAddress(char* host);

int DWCi_socketGetLastError(int sock);

/* 0x8050B980 - receive one datagram: (socket, buffer, capacity, flags, sender address, its length in/out);
 * -1 on failure. */
int DWCi_socketRecvFrom(int sock, u8* buf, int len, u32 flags, SOSockAddrIn* from, int* fromLen);

/* 0x8050C1D0 - non-zero while the socket has a datagram waiting. */
int DWCi_socketHasData(int sock);

int DWCi_socketIsUsable(int sock);

void SocketStartUp(void);

void SocketShutDown(void);

u32 current_time(void);

void msleep(u32 msec);

int DWCi_bufferAlloc(struct GTI2Buffer* buffer, u32 size);

void DWCi_bufferFrame(struct GTI2Buffer* buffer, const u8* data, s32 len);

int DWCi_socketConnect(struct DWCiConn* conn);

int DWCi_requestIsTimedOut(struct DWCiReq* req, u32 now);

void DWCi_socketShutdown(struct DWCiConn* conn);

int DWCi_requestReconnect(struct DWCiConn* conn, u32 addr, u16 port);

int DWCi_requestRetry(struct DWCiConn* conn, u32 addr, u16 port, u32 flag);

int DWCi_requestBlockState(struct DWCiReq* req, u32 a, u32 b, u32* out);

int DWCi_requestFlush(struct DWCiReq* req, u8* message, s32 len);

/* GameSpy socket layer (DWC / GameSpyInterface) */
/* 0x80509DB0 - the completion callback of the auth request: parses the server's answer into the state block.
 * GUESS on the name. */
struct NHTTPResponse;
void DWCi_Auth_RequestCallback(s32 result, struct NHTTPResponse* response, s32 size);
/* 0x8050A710 - the NHTTP teardown callback: reports, tells the host the auth session is over and parks the
 * state machine in its terminal state. */
void DWCi_Auth_EndProcess(void);
/* 0x8050A770 - moves the auth state machine on after a NAND call: `success` when the result is 0, `retry` while the
 * NAND layer is busy (up to five times), `noExist`/`access` for those two errors (27 = none), and a fatal end
 * otherwise. GUESS on the name. */
void DWCi_Auth_CheckNandResult(s32 success, s32 retry, s32 noExist, s32 access);

/* 0x807957F8 - the NAS login state `DWC_NASLoginProcess` steps (4 running, 1 done, 3 failed, 2 cancelled) and
 * 0x80795800 the result record the service-locator request fills. GUESS on the names. */
extern s32 DWCi_nasLoginState;
extern DWCSvlResult* DWCi_svlResult;

s32 DWC_NASLoginAsync(void);

s32 DWC_NASLoginProcess(void);

s32 DWC_SVLBegin(void);

void DWC_SVLEnd(void);

s32 DWC_SVLGetTokenAsync(const char* svl, DWCSvlResult* result);

s32 DWC_SVLProcess(void);

void DWCi_natProbeStart(const char* gameName);

s32 DWCi_natProbePoll(void);

/* ---- the transport / socket helpers --------------------------------------------------------- */

/* The GameSpy dynamic array (`darray.c`): count, capacity, element size, growth step, element free function and
 * the element storage. */
/* untyped: caller-owned payload - the element */
typedef void (*ArrayElementFreeFn)(void* elem);
/* untyped: caller-owned payload - the two elements compared */
typedef int (*ArrayCompareFn)(const void* elem1, const void* elem2);
/* untyped: caller-owned payload - the element and the client data */
typedef void (*ArrayMapFn)(void* elem, void* clientData);
/* untyped: caller-owned payload - the element and the client data */
typedef int (*ArrayMapFn2)(void* elem, void* clientData);
struct DArrayImplementation {
    /* +0x00 */ int count;
    /* +0x04 */ int capacity;
    /* +0x08 */ int elemsize;
    /* +0x0C */ int growby;
    /* +0x10 */ ArrayElementFreeFn elemfreefn;
    /* +0x14 */ void* list;
}; /* size: 0x18 */
typedef struct DArrayImplementation* DArray;

/* The GameSpy hash table (`hashtable.c`): an array of darray buckets and the element callbacks. */
/* untyped: caller-owned payload - the element hashed */
typedef int (*TableHashFn)(const void* elem, int numBuckets);
typedef ArrayCompareFn TableCompareFn;
typedef ArrayElementFreeFn TableElementFreeFn;
typedef ArrayMapFn TableMapFn;
typedef ArrayMapFn2 TableMapFn2;
struct HashImplementation {
    /* +0x00 */ DArray* buckets;
    /* +0x04 */ int nbuckets;
    /* +0x08 */ TableElementFreeFn freefn;
    /* +0x0C */ TableHashFn hashfn;
    /* +0x10 */ TableCompareFn compfn;
}; /* size: 0x14 */
typedef struct HashImplementation* HashTable;

typedef struct DWCiXfer DWCiXfer; /* one transfer record handed to DWCi_appendTransfer */

/* The `{ addr, port }` key the connection's request table hashes and compares on.  The table API
 * takes the key *indirectly* (a `DWCiAddrKey**`): DWCi_requestTableHash/DWCi_requestTableCompare dereference their first
 * argument once and only then read +0x00/+0x04.  A DWCiReq starts with exactly these six bytes, so
 * a request is its own key. */
typedef struct DWCiAddrKey {
    /* +0x00 */ u32 addr;
    /* +0x04 */ u16 port;
    /* +0x06 */ u16 pad_0x06;
} DWCiAddrKey; /* size: 0x08 */


/* One transfer record of the send path: the offset into the request's send buffer, the length, and
 * the timestamp the transport stamps into +0x0C. */
struct DWCiXfer {
    /* +0x00 */ u32 offset;
    /* +0x04 */ u32 length;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 stamp;
}; /* size: 0x10 */

/* GT2's buffer (`gt2Buffer.c`): the storage, its size and the bytes in use. */
typedef struct GTI2Buffer {
    /* +0x00 */ u8* buffer;
    /* +0x04 */ int size;
    /* +0x08 */ int len;
} GTI2Buffer; /* size: 0x0C */

/* GT2's socket (`GTI2Socket`): the SO socket and its address, the connection table keyed by `{ip, port}` and the
 * list of closed connections waiting to be freed, the close/error flags and callback nesting level, the
 * callbacks, the two buffer sizes every connection is given and the protocol header. */
struct DWCiConn {
    /* +0x00 */ int sock;
    /* +0x04 */ u32 addr;
    /* +0x08 */ u16 port;
    /* +0x0A */ u16 pad_0x0a;
    /* +0x0C */ HashTable connections;
    /* +0x10 */ DArray closedConnections;
    /* +0x14 */ u32 close;
    /* +0x18 */ u32 error;
    /* +0x1C */ s32 callbackLevel;
    /* +0x20 */ u32 connectAttemptCallback;
    /* +0x24 */ u32 socketErrorCallback;
    /* +0x28 */ u32 sendDumpCallback;
    /* +0x2C */ u32 receiveDumpCallback;
    /* +0x30 */ u32 unrecognizedMessageCallback;
    /* +0x34 */ u32 data;
    /* +0x38 */ u32 outgoingBufferSize;
    /* +0x3C */ u32 incomingBufferSize;
    /* +0x40 */ u32 protocolType;
    /* +0x44 */ u32 protocolOffset;
    /* +0x48 */ u32 broadcastEnabled;
}; /* size: 0x4C */

/* GT2's connection (`GTI2Connection`): the `{ip, port}` key, the owner socket, the state and its result, the
 * timing, the callbacks, the initial message, the two buffers and their message lists, the serial numbers, the
 * challenge response, the ack bookkeeping and the filter lists. */
struct DWCiReq {
    /* +0x00 */ u32 addr;
    /* +0x04 */ u16 port;
    /* +0x06 */ u16 pad_0x06;
    /* +0x08 */ struct DWCiConn* conn;
    /* +0x0C */ s32 state;
    /* +0x10 */ u32 initiated;
    /* +0x14 */ u32 freeAtAcceptReject;
    /* +0x18 */ s32 connectionResult;
    /* +0x1C */ u32 startTime;
    /* +0x20 */ u32 timeout;
    /* +0x24 */ s32 callbackLevel;
    /* +0x28 */ GT2ConnectionCallbacks callbacks;
    /* +0x38 */ u8* initialMessage;
    /* +0x3C */ s32 initialMessageLength;
    /* +0x40 */ u32 data;
    /* +0x44 */ GTI2Buffer incomingBuffer;
    /* +0x50 */ GTI2Buffer outgoingBuffer;
    /* +0x5C */ DArray incomingBufferMessages;
    /* +0x60 */ DArray outgoingBufferMessages;
    /* +0x64 */ u16 serialNumber;
    /* +0x66 */ u16 expectedSerialNumber;
    /* +0x68 */ u8 response[0x20];
    /* +0x88 */ u32 lastSend;
    /* +0x8C */ u32 challengeTime;
    /* +0x90 */ u32 pendingAck;
    /* +0x94 */ u32 pendingAckTime;
    /* +0x98 */ DArray sendFilters;
    /* +0x9C */ DArray receiveFilters;
}; /* size: 0xA0 */

DArray ArrayNew(int elemSize, int numElemsToAllocate, ArrayElementFreeFn elemFreeFn);
void ArrayFree(DArray array);
int ArrayLength(DArray array);
/* untyped: caller-owned payload - the element */
void* ArrayNth(DArray array, int n);
/* untyped: caller-owned payload - the element copied in */
void ArrayAppend(DArray array, const void* newElem);
/* untyped: caller-owned payload - the element copied in */
void ArrayInsertSorted(DArray array, const void* newElem, ArrayCompareFn comparator);
void ArrayRemoveAt(DArray array, int n);
void ArrayDeleteAt(DArray array, int n);
/* untyped: caller-owned payload - the element copied in */
void ArrayReplaceAt(DArray array, const void* newElem, int n);
/* untyped: caller-owned payload - the key */
int ArraySearch(DArray array, const void* key, ArrayCompareFn comparator, int fromIndex, int isSorted);
/* untyped: caller-owned payload - the client data */
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void* clientData);
/* untyped: caller-owned payload - the client data and the element returned */
void* ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void* clientData);

HashTable TableNew2(int elemSize, int nBuckets, int nChains, TableHashFn hashFn, TableCompareFn compFn,
                    TableElementFreeFn freeFn);
void TableFree(HashTable table);
/* untyped: caller-owned payload - the element copied in */
void TableEnter(HashTable table, const void* newElem);
/* untyped: caller-owned payload - the key */
int TableRemove(HashTable table, const void* delElem);
/* untyped: caller-owned payload - the key and the element returned */
void* TableLookup(HashTable table, const void* elemKey);
/* untyped: caller-owned payload - the client data */
void TableMapSafe(HashTable table, TableMapFn fn, void* clientData);
/* untyped: caller-owned payload - the client data and the element returned */
void* TableMapSafe2(HashTable table, TableMapFn2 fn, void* clientData);

int DWCi_socketBind(int sock, const SOSockAddrIn* addr, int len);
/* untyped: byte range - the datagram */
int DWCi_socketSendTo(int sock, const void* buf, int len, u32 flags, const SOSockAddrIn* to, int toLen);
int DWCi_socketGetLocalName(int sock, SOSockAddrIn* addr, int* len);
/* 0x8050BB10 - polls one socket for reading / writing / errors (each flag pointer may be NULL); -1 on failure. */
int DWCi_socketSelect(int sock, int* readFlag, int* writeFlag, int* exceptFlag);
DWCiHostEntry* DWCi_socketLookupHost(char* host);
/* 0x8050C270 - the resolver record of this machine's own host (NULL when it cannot be read). */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void);
/* 0x8050C450 - non-zero when the address the pointer names is a private one (10/8, 172.16/12, 192.168/16). */
int DWCi_hostAddressIsPrivate(DWCiHostAddr* host);
/* untyped: caller-owned payload - a raw block */
void* gsimalloc(u32 size);
/* untyped: caller-owned payload - a raw block */
void* gsirealloc(void* block, u32 size);
/* untyped: caller-owned payload - a raw block */
void gsifree(void* block);
int DWCi_requestSend(struct DWCiConn* conn, struct DWCiReq* req, u32 addr, u16 port, u32 a, void* buf,
                int len, u32 flag);
/* untyped: byte range - the datagram */
int DWCi_requestFrame(struct DWCiReq* req, void* p, u32 len);
void DWCi_requestFree(void* p);

/* The GameSpy SDK's memory hooks: the malloc / free / realloc / memalign callbacks. */
/* untyped: caller-owned payload - a raw block */
typedef void* (*GSIMallocFn)(u32 size);
/* untyped: caller-owned payload - a raw block */
typedef void (*GSIFreeFn)(void* block);
/* untyped: caller-owned payload - a raw block */
typedef void* (*GSIReallocFn)(void* block, u32 size);
/* untyped: caller-owned payload - a raw block */
typedef void* (*GSIMemalignFn)(u32 align, u32 size);
/* 0x8050C580 - installs the four callbacks. */
void gsiMemoryCallbacksSet(GSIMallocFn mallocFn, GSIFreeFn freeFn, GSIReallocFn reallocFn, GSIMemalignFn memalignFn);

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
