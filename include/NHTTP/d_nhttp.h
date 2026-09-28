/*
 * include/NHTTP/d_nhttp.h - the NHTTP core unit (`src/NHTTP/d_nhttp.c`, `.text`
 * 0x80515774..0x8051B7FC) and the NHTTP system-info block it owns.
 *
 * Rule 2: this unit owns `NHTTPi_GetSystemInfoP` and the info-block layout, so the consumers
 * (`NHTTP_bgnend.c`, `NHTTP_os_RVL.c`) include this header instead of declaring them.  The two data
 * objects have no registered owner, so they are declared in the band header
 * `include/unsplit/NHTTP.h`, which this file includes.
 *
 * The block is `NHTTPi_systemInfo` (`.bss`); `NHTTPi_GetSystemInfoP` lazily points `NHTTPi_systemInfoP` at it.
 * Only the fields the reconstructed accessors touch are modelled, with `pad_0xNN` filling the gaps;
 * the sub-records sit at the offsets `NHTTPi_Get*InfoP` return (0x800/0x808/0x80C/0x840).
 */
#ifndef MHTRI_NHTTP_D_NHTTP_H
#define MHTRI_NHTTP_D_NHTTP_H

#include "types.h"
#include "unsplit/OS.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The header-field list node (the request's header list is circular and doubly linked). size: 0x18 */
typedef struct NHTTPHeaderField {
    /* +0x00 */ struct NHTTPHeaderField* next;
    /* +0x04 */ struct NHTTPHeaderField* prev;
    /* +0x08 */ const char* token;
    /* +0x0C */ const char* value;
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
} NHTTPHeaderField;

/* The socket record the `NHTTPi_SocRecv*` wrappers read through a connection's +0x2C slot: the base
 * of the receive area and the number of bytes still available at it.  Only those two fields are
 * touched by the reconstructed bodies. size: 0x2C (approximate) */
typedef struct NHTTPSock {
    /* +0x000 */ u8 pad_0x000[0x1C];
    /* +0x01C */ u32 length;   /* bytes still available at `base` */
    /* +0x020 */ u8 pad_0x020[0x8];
    /* +0x028 */ u8* base;     /* the receive area's base */
} NHTTPSock;

/* The completion callback `NHTTPi_CleanupAsync` runs when the caller passed one, and the one
 * `NHTTPDestroy` forwards: no arguments, no return. */
typedef void (*NHTTPCompletionCallback)(void);

/* The response object a connection's +0x14 slot holds.  Only the fields the request/response
 * callbacks exchange with the caller are modelled. size: 0x43C (approximate) */
typedef struct NHTTPResponse {
    /* +0x000 */ u32 field_0x00;
    /* +0x004 */ u32 field_0x04;   /* the third word the phase-2/3 callback exchanges */
    /* +0x008 */ u8 pad_0x008[0x14];
    /* +0x01C */ u32 field_0x1C;   /* the second word the phase-2/3 callback exchanges */
    /* +0x020 */ u8 pad_0x020[0x8];
    /* +0x028 */ u32 field_0x28;   /* the first word the phase-2/3 callback exchanges */
    /* +0x02C */ u8 pad_0x02C[0x40C];
    /* +0x438 */ u32 field_0x438;
} NHTTPResponse;

struct NHTTPConnection;
struct NHTTPRequest;   /* defined by `NHTTP/NHTTP_bgnend.h`, which includes this file */

/* The four words a request/response callback is handed, and may update in place. size: 0x10 */
typedef struct NHTTPCallbackArgs {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
} NHTTPCallbackArgs;

/* The completion callback `NHTTPCreateConnection` installs at +0x1C: the connection, the phase it
 * has reached, and that phase's own four words.  Phase 1 (`NHTTPi_PostSendCallback`) answers the
 * value its caller returns, the others are ignored. */
typedef s32 (*NHTTPConnectionCallback)(struct NHTTPConnection* connection, s32 phase,
                                       NHTTPCallbackArgs* args);

/* The connection object `NHTTPCreateConnection` allocates (0x8060 B, 32-byte aligned).  Its size is
 * measured twice in that body: the allocation's own immediate (`lis r30,1` + `addi r3,r30,-32672` =
 * 0x8060) and the `memclr` that zeroes the whole record before anything else.  The record is handed
 * back by `NHTTPi_Request2Connection` and taken by `NHTTPi_CompleteCallback`; `NHTTPi_cancelRequest`
 * marks its +0x04 word with 8 before completing the request, and the `NHTTPi_SocRecv*` wrappers read
 * the socket at +0x2C.  The receive buffer is the 0x8000 B at +0x40, so the words after it are that
 * buffer's own bookkeeping and the extent the bodies address is 0x8048.  Only the fields the
 * reconstructed bodies touch are named. size: 0x8060 */
typedef struct NHTTPConnection {
    /* +0x000 */ u32 field_0x00;
    /* +0x004 */ u32 field_0x04;     /* 8 once the request was cancelled */
    /* +0x008 */ u32 unused_0x08;
    /* +0x00C */ u32 field_0x0C;     /* 1 after creation, 0 once the request was notified */
    /* +0x010 */ struct NHTTPRequest* request;   /* the object `fn_80515774` builds */
    /* +0x014 */ NHTTPResponse* response;
    /* +0x018 */ s32 field_0x18;
    /* +0x01C */ NHTTPConnectionCallback callback;
    /* +0x020 */ struct NHTTPConnection* next;   /* the request list's link */
    /* +0x024 */ u32 field_0x24;     /* the first word the phase-1 callback exchanges */
    /* +0x028 */ u32 field_0x28;     /* the second word the phase-1 callback exchanges */
    /* +0x02C */ NHTTPSock* sock;    /* the socket `NHTTPi_SocRecvOffsetRange` reads */
    /* +0x030 */ u32 unused_0x30;
    /* +0x034 */ u32 unused_0x34;
    /* +0x038 */ u32 unused_0x38;
    /* +0x03C */ u32 unused_0x3C;
    /* +0x040 */ u8 recvBuffer[0x8000];   /* 0x8000 B, ends at +0x8040 */
    /* +0x8040 */ u32 recvOffset;        /* where the unread bytes start in recvBuffer */
    /* +0x8044 */ u32 recvUnread;        /* bytes unread at recvBuffer + recvOffset */
    /* +0x8048 */ u8 pad_0x8048[0x18];
} NHTTPConnection;

/* The list-info record (`NHTTPi_InitListInfo` zeroes head/tail). size: 0x08 */
typedef struct NHTTPListInfo {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
} NHTTPListInfo;

/* The request-info record. size: 0x04 */
typedef struct NHTTPRequestInfo {
    /* +0x00 */ u32 field_0x00;
} NHTTPRequestInfo;

/* The request-list mutex info: `created` is the lazy-init flag. size: 0x1C */
typedef struct NHTTPMutexInfo {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ u32 created;
} NHTTPMutexInfo;

/* The comm-thread info.  The queue (0x20 B), the 2-slot message array the queue is told holds 3
 * (0x10 B), the OS thread block (0x318 B) and the ready flag `NHTTPi_InitThreadInfo` clears /
 * `NHTTPi_markCommThreadReady` sets. size: 0x34C */
typedef struct NHTTPThreadInfo {
    /* +0x000 */ OSMessageQueue queue;
    /* +0x020 */ OSMessage msgArray[2];
    /* +0x030 */ OSThread thread;
    /* +0x348 */ u32 ready;   /* NHTTPi_markCommThreadReady / NHTTPi_isCommThreadReady */
} NHTTPThreadInfo;

/* The 8-byte OS thread queue the completion record ends in (the Dolphin `OSThreadQueue`: the head and
 * tail links `OSInitThreadQueue`/`OSWakeupThread` operate on).  Declared beside its one user until a
 * band header carries the OS type. size: 0x8 */
typedef struct NHTTPThreadQueue {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
} NHTTPThreadQueue;

/* The synchronisation record `NHTTPi_NotifyCompletion` lazily initialises (`NHTTPi_completionSync`,
 * `.bss` 0x80762C20): the +0x00 init flag, the mutex the connection's notified flag is cleared under,
 * and the queue that call wakes.  The 0x1C tail is untouched by the reconstructed bodies, so it stays
 * padding.  The record's name is a GUESS (see the unit header). size: 0x40 (0x24 of it used) */
typedef struct NHTTPiCompletionSync {
    /* +0x00 */ u32 initialized;
    /* +0x04 */ OSMutex mutex;
    /* +0x1C */ NHTTPThreadQueue queue;
    /* +0x24 */ u8 pad_0x24[0x1C];
} NHTTPiCompletionSync;

/* The NHTTP system-info block. size: 0xB8C */
typedef struct NHTTPInfo {
    /* +0x000 */ u8 pad_0x000[0x7C4];
    /* +0x7C4 */ void* alloc_fn;   /* registered allocator (NHTTPi_alloc) */
    /* +0x7C8 */ void* free_fn;    /* registered free (NHTTPi_free) */
    /* +0x7CC */ u32 field_0x7CC;   /* 1 once NHTTPi_Startup has finished */
    /* +0x7D0 */ s32 field_0x7D0;   /* the socket NHTTPi_Startup opened, -1 when none */
    /* +0x7D4 */ s32 ssl_error;     /* NHTTPi_SetSSLError / NHTTPi_GetSSLError */
    /* +0x7D8 */ s32 error;         /* NHTTPi_SetError / NHTTPi_GetError */
    /* +0x7DC */ u32 field_0x7DC;   /* the comm thread's quit flag */
    /* +0x7E0 */ void* comm_stack;  /* the 8192-byte block NHTTPi_Startup gave the comm thread */
    /* +0x7E4 */ u32 field_0x7E4;
    /* +0x7E8 */ u8 pad_0x7E8[0x18];
    /* +0x800 */ NHTTPListInfo list;
    /* +0x808 */ NHTTPRequestInfo request;
    /* +0x80C */ NHTTPMutexInfo mutex;
    /* +0x828 */ u8 pad_0x828[0x18];
    /* +0x840 */ NHTTPThreadInfo thread;
} NHTTPInfo;

/* `NHTTPi_GetSystemInfoP`'s singleton slot and the block it points at. */
#include "unsplit/NHTTP.h"

/* The data this unit owns: the `.sbss` run 0x80795878..0x80795888 (the connection list's head plus the
 * three words beside it, `NHTTPi_systemInfoP` included) and the `.bss` completion record 0x80762C20,
 * both claimed in `splits.txt` (rule 12).  `NHTTPi_systemInfoP` used to be declared in the band header
 * `unsplit/NHTTP.h`, which rule 2 forbids once the unit owns its range. */
extern u32 NHTTPi_versionRegistered;                 /* 0x80795878: the NHTTP banner was registered */
extern u32 NHTTPi_createVersionRegistered;           /* 0x8079587C: the NHTTPCREATE one was */
extern NHTTPConnection* NHTTPi_connectionListHead;   /* 0x80795880: the connection list's head */
extern NHTTPInfo* NHTTPi_systemInfoP;                /* 0x80795884: the lazily-set info-block slot */
extern NHTTPiCompletionSync NHTTPi_completionSync;   /* 0x80762C20: the completion mutex/queue */

NHTTPInfo* NHTTPi_GetSystemInfoP(void);
void* NHTTPi_GetBgnEndInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetListInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetReqInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetThreadInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetMutexInfoP(NHTTPInfo* info); /* untyped: opaque handle */
/* untyped: byte range */
void* NHTTPi_memcpy(void* dst, const void* src, u32 n);
/* untyped: byte range */
void* NHTTPi_memclr(void* dst, u32 n);
void NHTTPi_InitThreadInfo(NHTTPThreadInfo* info);

/* The library-internal helpers `NHTTP/NHTTP_bgnend.c`'s bodies call.  None of them has a body yet
 * (this unit is registered `.text`-only and most of its range is still unwritten), so their names
 * are derived from their own bodies: a `strlen` tail, a bounded substring search, and the comm
 * thread's ready flag plus the loop it runs. */
u32 NHTTPi_strlen(const char* s);
s32 NHTTPi_containsString(const char* haystack, s32 haystackLen, const char* needle, s32 needleLen);
void NHTTPi_markCommThreadReady(NHTTPThreadInfo* info);
s32 NHTTPi_isCommThreadReady(NHTTPThreadInfo* info);
void NHTTPi_commThreadLoop(void);
int NHTTPi_strcmp(const char* a, const char* b);
s32 NHTTPi_strnicmp(const char* s1, const char* s2, s32 n);
s32 NHTTPi_encodeUrlChar(u8* dst, char c);
s32 NHTTPi_urlEncodedLength(const char* s);
s32 NHTTPi_urlEncodedLengthN(const char* s, s32 n);
s32 NHTTPi_compareToken(const char* a, const char* b);
s32 NHTTPi_GetConnectionListLength(void);
void NHTTPi_InitConnectionList(void);
/* The public entry points at 0x8051A300..0x8051A5CC.  `NHTTPDestroy` is the teardown DWCi calls
 * with its own command callback; the two getters answer the library's last error. */
s32 NHTTPi_StartRequest(NHTTPConnection* connection);
s32 NHTTPi_GetResponseSize(NHTTPConnection* connection);
s32 NHTTPi_GetConnectionStatus(NHTTPConnection* connection);
void NHTTPDestroy(NHTTPCompletionCallback callback);
s32 NHTTPGetError(void);
s32 NHTTPGetSSLError(void);

void NHTTPi_cancelAllRequests(NHTTPInfo* info);
void NHTTPi_CompleteCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
NHTTPConnection* NHTTPi_Request2Connection(NHTTPMutexInfo* mutex, s32 request);

/* The connection-list API (0x8051B0D8..0x8051B79C).  The list is this unit's own: one singly-linked
 * chain of `NHTTPConnection` records, and the five `mode`s below say what an operation does to it.
 *
 * `NHTTPi_ControlConnectionList` is the one body that walks it; every other entry point is a
 * one-call wrapper over it.  The key it looks a record up by is the connection itself in the get,
 * add and omit modes and the request/response record the connection holds in the two lookup modes,
 * so it stays an opaque handle here (`NHTTPi_Request2Connection` is handed the request list's own
 * `s32` handle and the `NHTTPRequestNode.request` field it is read out of).
 */
#define NHTTPI_LIST_GET         0  /* find the connection `key` is */
#define NHTTPI_LIST_BY_REQUEST  1  /* find the connection whose request is `key` */
#define NHTTPI_LIST_BY_RESPONSE 2  /* find the connection whose response is `key` */
#define NHTTPI_LIST_ADD         3  /* put the connection at the head of the list */
#define NHTTPI_LIST_OMIT        4  /* take the connection out of the list */

/* untyped: opaque handle - the connection itself, or the request/response it holds */
NHTTPConnection* NHTTPi_ControlConnectionList(NHTTPMutexInfo* mutex, void* key, s32 mode);
NHTTPConnection* NHTTPi_GetConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
s32 NHTTPi_AddConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
s32 NHTTPi_OmitConnectionList(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
struct NHTTPRequest* NHTTPi_Connection2Request(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
NHTTPResponse* NHTTPi_Connection2Response(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
NHTTPConnection* NHTTPi_Response2Connection(NHTTPMutexInfo* mutex, s32 response);
/* The two get modes look the connection up by itself, so both take it typed; only the *return* is
 * untyped - it is the record the connection holds, or that same key handed back when the lookup
 * misses (the miss signal). */
/* untyped: opaque handle - the record the connection holds, or the key handed back on a miss */
void* NHTTPi_GetRequest(NHTTPMutexInfo* mutex, NHTTPConnection* key);
/* untyped: opaque handle - the record the connection holds, or the key handed back on a miss */
void* NHTTPi_GetResponse(NHTTPMutexInfo* mutex, NHTTPConnection* key);
void NHTTPi_SetSock(NHTTPConnection* connection, NHTTPSock* sock);
NHTTPSock* NHTTPi_GetSock(NHTTPConnection* connection);

/* The three request/response callbacks `NHTTPCreateConnection` can install (phases 1, 2/3 and 4).
 * `phase` 3's body is the map's `NHTTPi_RecvCallback` and is named for the receive half it reports. */
s32 NHTTPi_PostSendCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection, u32 arg1, u32 arg2);
void NHTTPi_BufferFullCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
void NHTTPi_RecvCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
void NHTTPi_NotifyCompletion(NHTTPConnection* connection);

/* The raw socket receive the RVL wrappers end in.  Its body belongs to `d_nhttp.c` (still
 * unwritten); the six-parameter signature is a GUESS - only the register mapping is proven, from
 * `NHTTPi_SocRecvFromOffset`/`NHTTPi_SocRecvOffsetRange`, which forward their own arguments
 * unchanged. */
s32 NHTTPi_SocRecv(s32 handle, NHTTPConnection* conn, s32 flags, u8* buf, s32 length, s32 arg);
void NHTTPi_destroyRequestObject(void* connection, s32 request); /* untyped: opaque handle */

/* 0x8051A4E8 - bring the HTTP layer up with the caller's two command callbacks and one command id:
 * the body registers this library's version once and hands all three on to `NHTTPi_Startup`,
 * answering 0 when it comes up and -1 when it does not.  The DWCi runtime initialiser calls it that
 * way (its command callbacks plus command 17), and that call site is where the name came from - a
 * GUESS the NHTTP lane may refine when it writes the body. */
s32 NHTTPi_RegisterCallbacks(void (*commandCallback)(u32), void (*commandCallbackEx)(u32), u32 command);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_D_NHTTP_H */
