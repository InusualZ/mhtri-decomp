/*
 * include/NHTTP/NHTTP_bgnend.h - the NHTTP begin/end unit (`src/NHTTP/NHTTP_bgnend.c`, `.text`
 * 0x805145B8..0x80515010).
 *
 * Rule 2: this unit owns the error/alloc/list/mutex/comm-thread entry points, so `d_nhttp.c`
 * includes this header instead of declaring them.  The NHTTP system-info block it operates on is
 * `d_nhttp.h`'s `NHTTPInfo` (rule 1: one definition, in the owner's header), and the RVL-side
 * helpers it calls (`NHTTPi_CheckCurrentThread`, `NHTTPi_InitRequestInfo`) come from
 * `NHTTP_os_RVL.h`, which the `.c` includes after this one.
 */
#ifndef MHTRI_NHTTP_NHTTP_BGNEND_H
#define MHTRI_NHTTP_NHTTP_BGNEND_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* `NHTTPAllocFn`/`NHTTPFreeFn`, the registered allocator pair `NHTTPi_Startup` installs, are declared
 * in `NHTTP/d_nhttp.h` (the response's callbacks are handed the same pair). */

/* The request record the request list holds.  `id` is the list's own monotonically increasing
 * handle (the list's second word is that counter, not a tail pointer). size: 0x18 */
typedef struct NHTTPRequestNode {
    /* +0x00 */ struct NHTTPRequestNode* next;
    /* +0x04 */ struct NHTTPRequestNode* prev;
    /* +0x08 */ s32 id;           /* the handle NHTTPi_insertRequest returns */
    /* +0x0C */ s32 request;      /* the request object NHTTPi_cancelRequest destroys */
    /* +0x10 */ s32 state;        /* -1 until a socket is opened; then that socket descriptor (`NHTTPi_connectSocket`) */
    /* +0x14 */ u32 unused_0x14;
} NHTTPRequestNode;

/* The request list: its head plus the next-id counter `NHTTPi_insertRequest` post-increments.
 * size: 0x08 */
typedef struct NHTTPRequestList {
    /* +0x00 */ NHTTPRequestNode* head;
    /* +0x04 */ s32 nextId;
} NHTTPRequestList;

/* The NHTTP request record `NHTTPi_createRequestObject` allocates (0x258 B, 4-aligned) and hangs off a
 * connection's +0x10 slot.  Both header lists are heads (the nodes are `NHTTPHeaderField`, 0x18 B
 * each); `response` is the 0x43C-byte record beside it.  Names are read off the bodies of `d_nhttp.c`
 * (the unit header lists the GUESSes); +0x0C..+0x28 hold the parsed URL. size: 0x258 */
typedef struct NHTTPRequest {
    /* +0x000 */ u32 cancelled;              /* 1 once the request was cancelled or shut down */
    /* +0x004 */ u32 field_0x04;             /* committed-header flag NHTTP_AddHeaderField tests */
    /* +0x008 */ u32 useSSL;                 /* 1 for an `https://` URL */
    /* +0x00C */ u32 useProxy;               /* 1 once `NHTTPSetProxy` gave the request a proxy */
    /* +0x010 */ u32 field_0x10;             /* "post data raw already added" flag */
    /* +0x014 */ s32 hostEnd;                /* offset in `url` of the `:` or `/` ending the host */
    /* +0x018 */ s32 pathStart;              /* offset in `url` of the `/` opening the path */
    /* +0x01C */ s32 method;                 /* 0 GET, 1 POST, 2 HEAD */
    /* +0x020 */ s32 port;                   /* 80, 443 or the URL's own */
    /* +0x024 */ char* url;                  /* the decoded copy of the URL */
    /* +0x028 */ char* host;                 /* the host name, its own copy */
    /* +0x02C */ NHTTPResponse* response;
    /* +0x030 */ NHTTPHeaderField* headerList;   /* NHTTP_AddHeaderField's field list */
    /* +0x034 */ NHTTPHeaderField* postDataList; /* NHTTPAddPostDataRaw's field list */
    /* +0x038 */ u16 boundaryDashes;         /* the "--" opening the multipart boundary */
    /* +0x03A */ char code[0x12];            /* the 18-byte code NHTTPAddPostDataRaw walks */
    /* +0x04C */ char basicAuth[0x5C];       /* base64 credentials of the `Authorization: Basic` header */
    /* +0x0A8 */ s32 basicAuthLength;
    /* +0x0AC */ s32 sslHandle;              /* the `SSLNew` context once connected, 0 or -1 otherwise */
    /* +0x0B0 */ u32 clientCert;
    /* +0x0B4 */ u32 clientCertLength;
    /* +0x0B8 */ u32 clientKey;
    /* +0x0BC */ u32 clientKeyLength;
    /* +0x0C0 */ u32 rootCA;
    /* +0x0C4 */ u32 rootCALength;
    /* +0x0C8 */ s32 builtinClientCert;      /* 1 to use the built-in client certificate `builtinClientCertId` */
    /* +0x0CC */ u32 verifyOption;
    /* +0x0D0 */ s32 postType;               /* 0 chooses by the fields, 2 multipart, anything else urlencoded */
    /* +0x0D4 */ u32 sslCallbackArg;
    /* +0x0D8 */ u32 builtinRootCAId;
    /* +0x0DC */ u32 builtinClientCertId;
    /* +0x0E0 */ char proxyHost[0x100];
    /* +0x1E0 */ s32 proxyPort;
    /* +0x1E4 */ char proxyAuth[0x5C];       /* base64 credentials of the `Proxy-Authorization` header */
    /* +0x240 */ s32 proxyAuthLength;
    /* +0x244 */ u32 recvBufferSize;         /* the socket's SO_RCVBUF option, 0 for the default */
    /* +0x248 */ u32 sendBufferSize;         /* and SO_SNDBUF */
    /* +0x24C */ char* rawBody;              /* the raw post body, null when the fields are posted */
    /* +0x250 */ s32 rawBodyLength;
    /* +0x254 */ s32 (*sendCallback)(u32 arg0, u32* arg1, u32* arg2, u32 arg3, s32 size);   /* phase-1 hook */
} NHTTPRequest; /* size: 0x258 */

/* the cancellation callback `NHTTPi_CleanupAsync` invokes when the caller passed one:
 * `NHTTPCompletionCallback` moved to `NHTTP/d_nhttp.h`, which this file includes - `NHTTPDestroy`
 * there is handed the same callback. */

void NHTTPi_SetError(NHTTPInfo* info, s32 err);
void NHTTPi_SetSSLError(NHTTPInfo* info, s32 err);
s32 NHTTPi_GetSSLError(NHTTPInfo* info);
s32 NHTTPi_GetError(NHTTPInfo* info);
void NHTTPi_InitSystemInfo(NHTTPInfo* info);
void NHTTPi_InitListInfo(NHTTPListInfo* info);
void NHTTPi_InitMutexInfo(NHTTPMutexInfo* info);
void NHTTPi_initLockReqList(NHTTPMutexInfo* mutex);
void NHTTPi_lockReqList(NHTTPMutexInfo* mutex);
void NHTTPi_unlockReqList(NHTTPMutexInfo* mutex);
void NHTTPi_exitLockReqList(void);

s32 NHTTPi_Startup(NHTTPInfo* info, NHTTPAllocFn alloc, NHTTPFreeFn free, u32 arg);
void NHTTPi_CleanupAsync(NHTTPInfo* info, NHTTPCompletionCallback callback);

s32 NHTTPi_SetHeaderField(NHTTPHeaderField** ppHead, NHTTPInfo* owner, const char* token,
                          const char* value);
u32 NHTTP_AddHeaderField(NHTTPRequest* request, NHTTPInfo* owner, const char* token,
                         const char* value);
s32 NHTTPAddPostDataRaw(NHTTPRequest* request, NHTTPInfo* owner, const char* token,
                        const char* value);
NHTTPHeaderField* NHTTPi_RemoveNode(NHTTPHeaderField** ppHead);
NHTTPHeaderField* NHTTPi_RemoveHeaderField(NHTTPHeaderField** ppHead);

u32 NHTTPi_insertRequest(NHTTPRequestList* list, s32 request);
s32 NHTTPi_cancelRequest(NHTTPRequestList* list, void* connection, s32 id); /* untyped: opaque handle */
void NHTTPi_cancelConnectionRequests(NHTTPRequestList* list, void* connection); /* untyped: opaque handle */

s32 NHTTPi_createCommThread(NHTTPThreadInfo* thread, u32 arg, u8* stack);
void NHTTPi_destroyCommThread(NHTTPThreadInfo* thread, NHTTPInfo* info);
BOOL NHTTPi_receiveMessage(OSMessageQueue* queue);
BOOL NHTTPi_sendQuitMessage(OSMessageQueue* queue);

/* untyped: byte range */
void* NHTTPi_alloc(u32 size, u32 align);
void NHTTPi_free(void* block); /* untyped: byte range */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_BGNEND_H */
