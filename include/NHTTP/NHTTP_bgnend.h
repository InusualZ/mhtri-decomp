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

/* The registered allocator pair `NHTTPi_Startup` installs: the allocator takes a size and an
 * alignment and returns the block, the free takes the block back.  Both are untyped at the block
 * (the info block stores them as bare words). */
typedef void* (*NHTTPAllocFn)(u32 size, u32 align); /* untyped: byte range */
typedef void (*NHTTPFreeFn)(void* block); /* untyped: byte range */

/* The request record the request list holds.  `id` is the list's own monotonically increasing
 * handle (the list's second word is that counter, not a tail pointer). size: 0x18 */
typedef struct NHTTPRequestNode {
    /* +0x00 */ struct NHTTPRequestNode* next;
    /* +0x04 */ struct NHTTPRequestNode* prev;
    /* +0x08 */ s32 id;           /* the handle NHTTPi_insertRequest returns */
    /* +0x0C */ s32 request;      /* the request object NHTTPi_cancelRequest destroys */
    /* +0x10 */ s32 state;        /* -1 until the request completes */
    /* +0x14 */ u32 unused_0x14;
} NHTTPRequestNode;

/* The request list: its head plus the next-id counter `NHTTPi_insertRequest` post-increments.
 * size: 0x08 */
typedef struct NHTTPRequestList {
    /* +0x00 */ NHTTPRequestNode* head;
    /* +0x04 */ s32 nextId;
} NHTTPRequestList;

/* The NHTTP request record; only the fields this unit's header path touches are modelled.  Both
 * header lists are heads (the nodes are `NHTTPHeaderField`, 0x18 B each). size: 0x4C */
typedef struct NHTTPRequest {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;              /* committed-header flag NHTTP_AddHeaderField tests */
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;              /* "post data raw already added" flag */
    /* +0x14 */ u8 pad_0x14[0x1C];
    /* +0x30 */ NHTTPHeaderField* headerList;   /* NHTTP_AddHeaderField's field list */
    /* +0x34 */ NHTTPHeaderField* postDataList; /* NHTTPAddPostDataRaw's field list */
    /* +0x38 */ u16 unused_0x38;
    /* +0x3A */ char code[0x12];             /* the 18-byte code NHTTPAddPostDataRaw walks */
} NHTTPRequest; /* size: 0x4C */

/* the cancellation callback `NHTTPi_CleanupAsync` invokes when the caller passed one */
typedef void (*NHTTPCompletionCallback)(void);

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
