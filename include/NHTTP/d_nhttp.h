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

/* The registered allocator pair `NHTTPi_Startup` installs: the allocator takes a size and an
 * alignment and returns the block, the free takes the block back.  Both are untyped at the block
 * (the info block stores them as bare words). */
typedef void* (*NHTTPAllocFn)(u32 size, u32 align); /* untyped: byte range */
typedef void (*NHTTPFreeFn)(void* block); /* untyped: byte range */
struct NHTTPResponse;
struct NHTTPSock;

/* The caller's hooks on a response: the buffer-full callback is handed the buffer word (in and out),
 * the size word, the socket, the allocator pair and the response's own size word, and answers what
 * goes back into the first word of the phase-2 arguments; the free callback gets the buffer back; the
 * completion hook gets the connection's status, the response and its size word. */
typedef s32 (*NHTTPBufferFullCallback)(u32* buffer, u32* size, struct NHTTPSock* sock, NHTTPAllocFn alloc,
                                       NHTTPFreeFn free, s32 length);
typedef void (*NHTTPFreeBufferCallback)(u32 buffer, NHTTPFreeFn free, s32 size);
typedef void (*NHTTPCompleteCallback)(s32 status, struct NHTTPResponse* response, s32 size);

/* The response object a connection's +0x14 slot (a request's +0x2C) holds: the receive ring of
 * `NHTTP_os_RVL.h` (its 0x38-byte head, the 512-byte block list at +0x34 and the 0x400 bytes at +0x38)
 * followed by the caller's own word - which is why the sibling's `NHTTPRecvBuf` helpers take the same
 * pointer.  Names are read off the bodies of `d_nhttp.c`. size: 0x43C */
typedef struct NHTTPResponse {
    /* +0x000 */ s32 headerLength;      /* bytes of the response head buffered in the ring */
    /* +0x004 */ u32 received;          /* the third word the phase-2/3 callback exchanges */
    /* +0x008 */ u32 receivedTotal;
    /* +0x00C */ s32 contentLength;     /* the Content-Length header's value, -1 when it has none */
    /* +0x010 */ u32 completed;         /* 1 once the response ended without an error */
    /* +0x014 */ u32 headersParsed;     /* 1 once the status line and the fields were read */
    /* +0x018 */ s32 statusCode;
    /* +0x01C */ u32 bufferSize;        /* the second word the phase-2/3 callback exchanges */
    /* +0x020 */ void* auxBufferA;
    /* +0x024 */ void* auxBufferB;
    /* +0x028 */ u32 userBuffer;        /* the first word the phase-2/3 callback exchanges */
    /* +0x02C */ NHTTPBufferFullCallback bufferFullCallback;
    /* +0x030 */ NHTTPFreeBufferCallback freeCallback;
    /* +0x034 */ struct NHTTPRecvBlock* blocks;   /* the ring's 512-byte block list */
    /* +0x038 */ u8 data[0x400];
    /* +0x438 */ u32 userData;          /* the word the caller gave the request; the last argument of every phase callback */
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
    /* +0x000 */ u32 state;          /* 0 created, 1 queued, 2 sending, 3 receiving the head, 4 the body, 5 done */
    /* +0x004 */ u32 status;         /* 0xF at creation, 8 once cancelled, the request's own error once finished */
    /* +0x008 */ s32 sslStatus;      /* the last `SSLDoHandshake` answer */
    /* +0x00C */ u32 pending;        /* 1 after creation, 0 once the request was notified */
    /* +0x010 */ struct NHTTPRequest* request;   /* the object `NHTTPi_createRequestObject` builds */
    /* +0x014 */ NHTTPResponse* response;
    /* +0x018 */ s32 requestId;      /* the id `NHTTP_SendRequestAsync` answered, -1 before */
    /* +0x01C */ NHTTPConnectionCallback callback;
    /* +0x020 */ struct NHTTPConnection* next;   /* the request list's link */
    /* +0x024 */ u32 postData;       /* the first word the phase-1 callback exchanges: the piece it hands out */
    /* +0x028 */ u32 postLength;     /* the second: that piece's length, 0 when it is done */
    /* +0x02C */ NHTTPSock* sock;    /* the socket `NHTTPi_SocRecvOffsetRange` reads */
    /* +0x030 */ NHTTPCompleteCallback completeCallback;   /* the caller's phase-4 hook */
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

/* The request-info record: the request node the comm thread is serving now, null between requests
 * (`NHTTPi_dequeueRequest` sets it, `NHTTPi_finishRequest` frees and clears it). size: 0x04 */
struct NHTTPRequestNode;
typedef struct NHTTPRequestInfo {
    /* +0x00 */ struct NHTTPRequestNode* active;
} NHTTPRequestInfo;

/* The request-list mutex info: `created` is the lazy-init flag. size: 0x1C */
typedef struct NHTTPMutexInfo {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ u32 created;
} NHTTPMutexInfo;

/* The comm-thread info.  The queue (0x20 B), the 2-slot message array the queue is told holds 3
 * (0x10 B), the OS thread block (0x318 B), the ready flag `NHTTPi_InitThreadInfo` clears /
 * `NHTTPi_markCommThreadReady` sets, and the request send buffer. size: 0x460 */
typedef struct NHTTPThreadInfo {
    /* +0x000 */ OSMessageQueue queue;
    /* +0x020 */ OSMessage msgArray[2];
    /* +0x030 */ OSThread thread;
    /* +0x348 */ u32 ready;   /* NHTTPi_markCommThreadReady / NHTTPi_isCommThreadReady */
    /* +0x34C */ u8 pad_0x34C[0x14];
    /* +0x360 */ u8 sendBuffer[0x100];   /* the 256-byte buffer `NHTTPi_SaveBuf` fills before each send */
} NHTTPThreadInfo;


/* The synchronisation record `NHTTPi_NotifyCompletion` lazily initialises (`NHTTPi_completionSync`,
 * `.bss` 0x80762C20): the +0x00 init flag, the mutex the connection's notified flag is cleared under,
 * and the queue that call wakes.  The 0x1C tail is untouched by the reconstructed bodies, so it stays
 * padding.  The record's name is a GUESS (see the unit header). size: 0x40 (0x24 of it used) */
typedef struct NHTTPiCompletionSync {
    /* +0x00 */ u32 initialized;
    /* +0x04 */ OSMutex mutex;
    /* +0x1C */ OSThreadQueue queue;
    /* +0x24 */ u8 pad_0x24[0x1C];
} NHTTPiCompletionSync;

/* One proxy setting the network configuration holds (`NHTTPSetSystemProxy` reads the plain and the
 * https one): whether it is on, whether it carries credentials, the host, port and credentials.
 * size: 0x148 */
typedef struct NHTTPProxyConfig {
    /* +0x000 */ u8 enabled;
    /* +0x001 */ u8 authEnabled;
    /* +0x002 */ u8 pad_0x002[2];
    /* +0x004 */ char host[0x100];
    /* +0x104 */ u16 port;
    /* +0x106 */ char user[0x21];
    /* +0x127 */ char password[0x21];
} NHTTPProxyConfig;

/* The NHTTP system-info block. size: 0xCA0 */
typedef struct NHTTPInfo {
    /* +0x000 */ u8 pad_0x000[0x28];
    /* +0x028 */ NHTTPProxyConfig httpProxy;
    /* +0x170 */ NHTTPProxyConfig httpsProxy;
    /* +0x2B8 */ u8 pad_0x2B8[0x50C];
    /* +0x7C4 */ void* alloc_fn;   /* registered allocator (NHTTPi_alloc) */
    /* +0x7C8 */ void* free_fn;    /* registered free (NHTTPi_free) */
    /* +0x7CC */ u32 field_0x7CC;   /* 1 once NHTTPi_Startup has finished */
    /* +0x7D0 */ s32 socket;        /* the socket NHTTPi_Startup opened, -1 when none */
    /* +0x7D4 */ s32 ssl_error;     /* NHTTPi_SetSSLError / NHTTPi_GetSSLError */
    /* +0x7D8 */ s32 error;         /* NHTTPi_SetError / NHTTPi_GetError */
    /* +0x7DC */ u32 quitFlag;      /* the comm thread's quit flag */
    /* +0x7E0 */ void* comm_stack;  /* the 8192-byte block NHTTPi_Startup gave the comm thread */
    /* +0x7E4 */ void (*sslHook)(s32 ssl, u32 arg);   /* called with each new SSL context and the request's arg */
    /* +0x7E8 */ u8 pad_0x7E8[0x18];
    /* +0x800 */ NHTTPListInfo list;
    /* +0x808 */ NHTTPRequestInfo request;
    /* +0x80C */ NHTTPMutexInfo mutex;
    /* +0x828 */ u8 pad_0x828[0x18];
    /* +0x840 */ NHTTPThreadInfo thread;
} NHTTPInfo;

/* The comm thread's working state: `NHTTPi_commThreadLoop` keeps one on its stack and hands it to
 * every step of serving a request.  Names are read off the bodies.  size: 0x340 */
typedef struct NHTTPCommContext {
    /* +0x000 */ s32 requestId;         /* the request being served, -1 between requests */
    /* +0x004 */ char host[0x100];      /* the host the socket is connected to */
    /* +0x104 */ u8 recvScratch[0x200]; /* the response's spill buffer once the caller's is full */
    /* +0x304 */ u8 statusLine[0x10];   /* the first bytes of the response */
    /* +0x314 */ u32 address;           /* the resolved address of `host` */
    /* +0x318 */ u32 lastAddress;       /* and the one the socket is connected to */
    /* +0x31C */ u32 port;
    /* +0x320 */ u32 lastPort;
    /* +0x324 */ s32 sendUsed;          /* bytes queued in the request send buffer */
    /* +0x328 */ s32 recvCount;         /* bytes of the response head received */
    /* +0x32C */ s32 contentRemaining;  /* the body bytes still to come, or the current chunk's */
    /* +0x330 */ s32 error;             /* the request's error code, 0 when it is going well */
    /* +0x334 */ s32 again;             /* 1 to run the next round on the same socket */
    /* +0x338 */ s32 connected;         /* 1 while the socket is kept for the next request */
    /* +0x33C */ s32 chunked;           /* 1 for a chunked body */
} NHTTPCommContext;

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

/* The raw socket receive the RVL wrappers end in.  The sibling `NHTTP_os_RVL.c` declared the six
 * parameters as (handle, conn, flags, ...); the body reads them as the request-list mutex, the
 * REQUEST (its +0xAC SSL handle and +0x00 cancel flag), the socket, the buffer, its length and the
 * receive flags, so `conn` is a request and `handle` a mutex there (a shared-file request). */
s32 NHTTPi_SocRecv(s32 handle, NHTTPConnection* conn, s32 flags, u8* buf, s32 length, s32 arg);
s32 NHTTPi_destroyRequestObject(NHTTPMutexInfo* mutex, s32 request);

/* The socket layer (0x805163A4..0x80516C2C) - the raw SO wrappers with the request's SSL handle in
 * front: a send/receive goes through SSL when the request holds a handle, and the error mapping is
 * shared by both directions. */
s32 NHTTPi_SocClose(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd);
s32 NHTTPi_SocSSLConnect(NHTTPInfo* info, NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd);
s32 NHTTPi_SocRecv_sub(NHTTPConnection* connection, s32 fd, u8* buf, s32 length, s32 flags);
s32 NHTTPi_SocSend(struct NHTTPRequest* request, s32 fd, u8* buf, s32 length, s32 flags);
s32 NHTTPi_SocSend_sub(s32 fd, u8* buf, s32 length, s32 flags);
s32 NHTTPi_SocConnect(NHTTPInfo* info, NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd,
                      u32 address, u32 port);
void NHTTPi_SocShutdown(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd);
s32 NHTTPi_probeSocket(s32 fd);
s32 NHTTPi_openSocket(struct NHTTPRequest* request);
u32 NHTTPi_resolveHostName(struct NHTTPRequest* request, const char* name);

/* The string helpers (0x8051702C..0x80517634) and the connection/request API (0x8051A1B8..0x8051AFEC):
 * the public NHTTP calls take the request handle `NHTTPCreateRequest` answered (a request object) or,
 * for the response-side calls, the response handle. */
s32 NHTTPi_strToDec(const char* s, s32 n);
s32 NHTTPi_uintToStr(char* buf, u32 value);
s32 NHTTPi_isHeaderEnd(const char* window, s32 count);
NHTTPConnection* NHTTPCreateConnection(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                       NHTTPConnectionCallback callback, u32 userData);
s32 NHTTPGetBodyBuffer(NHTTPConnection* connection, u32* buffer, u32* size);
struct NHTTPRequest* __NHTTPCreateRequestEx(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                            NHTTPCompleteCallback completeCallback, u32 userData,
                                            NHTTPBufferFullCallback bufferFullCallback,
                                            NHTTPFreeBufferCallback freeCallback);
s32 NHTTPi_dispatchConnectionCallback(NHTTPConnection* connection, s32 phase, NHTTPCallbackArgs* args);
struct NHTTPRequest* NHTTPCreateRequest(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                        NHTTPCompleteCallback completeCallback, u32 userData);
s32 NHTTPSendRequestAsync(struct NHTTPRequest* request);
s32 NHTTPCancelRequest(s32 id);
void NHTTPDestroyResponse(NHTTPResponse* response);
s32 NHTTPGetBodyAll(NHTTPResponse* response, u32* buffer);
s32 NHTTPGetResultCode(NHTTPResponse* response);
s32 NHTTPSetVerifyOption(struct NHTTPRequest* handle, u32 option);
s32 NHTTPClearRootCA(struct NHTTPRequest* handle);
s32 NHTTPClearClientCert(struct NHTTPRequest* handle);
s32 NHTTPAddHeaderField(struct NHTTPRequest* handle, const char* token, const char* value);
s32 NHTTPAddPostDataAscii(struct NHTTPRequest* handle, const char* token, const char* value);

/* The request and response objects (0x80515774..0x805161A4) and the public API over them. */
struct NHTTPRequest* NHTTPi_createRequestObject(NHTTPInfo* info, const char* url, s32 method, u32 buffer,
                                                u32 bufferSize, u32 userData,
                                                NHTTPBufferFullCallback bufferFullCallback,
                                                NHTTPFreeBufferCallback freeCallback);
void NHTTP_DestroyRequest(NHTTPInfo* info, struct NHTTPRequest* request);
void NHTTP_DestroyResponse(NHTTPMutexInfo* mutex, NHTTPResponse* response);
s32 NHTTP_SendRequestAsync(NHTTPInfo* info, struct NHTTPRequest* request);
s32 NHTTPi_cancelRequestById(NHTTPInfo* info, s32 id);

/* The request writer (0x805176FC..0x80518308) and the rest of the public API. */
s32 NHTTPi_SaveBuf(struct NHTTPRequest* request, u8* buffer, s32 fd, s32* used, u8* data, s32 length);
s32 NHTTPi_queryPostFieldSize(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, const char* key, s32* total,
                              s32 mode);
s32 NHTTPi_sendPostField(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, u8* buffer, const char* key,
                         s32 fd, s32* used, s32 mode);
s32 NHTTPi_ensureRecvSpace(NHTTPMutexInfo* mutex, NHTTPResponse* response);
s32 NHTTPi_appendSendData(NHTTPCommContext* context, const char* data, s32 length);
s32 NHTTPi_sendHeaderFields(NHTTPCommContext* context);
s32 NHTTPSetProxy(struct NHTTPRequest* handle, const char* host, s32 port, const char* user,
                  const char* password);
s32 NHTTPSetSystemProxy(struct NHTTPRequest* handle);
s32 NHTTPi_Base64Encode(char* dst, const char* src);

/* The comm thread's steps (0x80518970..0x80519FB4): each takes the working state the loop keeps. */
s32 NHTTPi_dequeueRequest(NHTTPCommContext* context);
void NHTTPi_finishRequest(NHTTPCommContext* context);
s32 NHTTPi_prepareTarget(NHTTPCommContext* context);
s32 NHTTPi_connectSocket(NHTTPCommContext* context);
s32 NHTTPi_negotiateSSL(NHTTPCommContext* context);
s32 NHTTPi_sendProxyConnect(NHTTPCommContext* context);
s32 NHTTPi_recvProxyConnectReply(NHTTPCommContext* context);
s32 NHTTPi_sendRawPostBody(NHTTPCommContext* context);
s32 NHTTPi_sendMultipartBody(NHTTPCommContext* context);
s32 NHTTPi_sendUrlEncodedBody(NHTTPCommContext* context);
s32 NHTTPi_sendRequest(NHTTPCommContext* context);
s32 NHTTPi_recvResponseHeaders(NHTTPCommContext* context);
s32 NHTTPi_parseResponseHeaders(NHTTPCommContext* context);
s32 NHTTPi_recvResponseBody(NHTTPCommContext* context);
struct NHTTPRecvBuf;
s32 NHTTPi_findHeaderField(struct NHTTPRecvBuf* ring, const char* name, s32* offset);

/* 0x8051A4E8 - bring the HTTP layer up with the caller's two command callbacks and one command id:
 * the body registers this library's version once and hands all three on to `NHTTPi_Startup`,
 * answering 0 when it comes up and -1 when it does not.  The DWCi runtime initialiser calls it that
 * way (its command callbacks plus command 17), and that call site is where the name came from - a
 * GUESS the NHTTP lane may refine when it writes the body. */
s32 NHTTPi_RegisterCallbacks(void (*commandCallback)(u32), void (*commandCallbackEx)(u32), u32 command);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `include/unsplit/NHTTP.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80574CE8 (0x15 B, `.rodata`) - the 19-character code `NHTTPAddPostDataRaw` walks its request's
 * 18-byte code field up to, one character at a time, re-submitting after each step. */
extern const char NHTTPi_postDataRawCode[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_D_NHTTP_H */
