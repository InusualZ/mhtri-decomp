/*
 * DWCi/dwc_nasfunc.h - the declarations `src/DWCi/dwc_nasfunc.c` owns (docs/plan.md 6.5 rule 2), and the hub for
 *   the GameSpy / GT2 records the split GameSpy files share (`DWCiConn` = GTI2Socket, `DWCiReq` = GTI2Connection,
 *   `GTI2Buffer`, the GT2 handles and callback set).  Each split file's own declarations live in its header,
 *   included at the end of this one, so a consumer of this header sees the whole DWC/GameSpy surface.
 */
#ifndef MHTRI_DWCI_DWC_NASFUNC_H
#define MHTRI_DWCI_DWC_NASFUNC_H

#include "types.h"
#include "DWCi/natneg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "SO/soi.h"
#include "DWCi/dwc_auth_interface.h"    /* the Np unit's own data (rule 2) */
#include "DWCi/hashtable.h"             /* DArray / HashTable: the GT2 records hold them */

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

/* 0x8050A710 - the NHTTP teardown callback: reports, tells the host the auth session is over and parks the
 * state machine in its terminal state. */
void DWCi_Auth_EndProcess(void);
/* 0x8050A770 - moves the auth state machine on after a NAND call: `success` when the result is 0, `retry` while the
 * NAND layer is busy (up to five times), `noExist`/`access` for those two errors (27 = none), and a fatal end
 * otherwise. GUESS on the name. */
void DWCi_Auth_CheckNandResult(s32 success, s32 retry, s32 noExist, s32 access);

/* 0x807957F8 - the NAS login state `DWC_NASLoginProcess` steps (4 running, 1 done, 3 failed, 2 cancelled). GUESS on
 * the name. */
extern s32 DWCi_nasLoginState;

s32 DWC_NASLoginAsync(void);

s32 DWC_NASLoginProcess(void);

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
    /* +0x08 */ u16 serialNumber;
    /* +0x0A */ u16 pad_0x0A;
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
    /* +0x40 */ s32 protocolType;
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

#ifdef __cplusplus
}
#endif

/* The GT2 connection states (`GTI2Connection.state`). */
#define GTI2_STATE_AWAITING_SERVER_CHALLENGE 0
#define GTI2_STATE_AWAITING_CLIENT_CHALLENGE 2
#define GTI2_STATE_AWAITING_ACCEPT_REJECT 4
#define GTI2_STATE_CONNECTED 5
#define GTI2_STATE_CLOSING 6
#define GTI2_STATE_CLOSED 7

/* The split GT2 / GameSpy files' own declarations (each owner's header; they read the types above). */
#include "DWCi/dwc_svl.h"
#include "DWCi/gsPlatform.h"
#include "DWCi/gsAvailable.h"
#include "DWCi/gt2Auth.h"
#include "DWCi/gt2Buffer.h"
#include "DWCi/gt2Callback.h"
#include "DWCi/gt2Connection.h"
#include "DWCi/gt2Main.h"
#include "DWCi/gt2Message.h"

#endif /* MHTRI_DWCI_DWC_NASFUNC_H */
