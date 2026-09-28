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

/* The connection object `NHTTPi_Request2Connection` hands back and `NHTTPi_CompleteCallback` takes.
 * `NHTTPi_cancelRequest` marks its +0x04 word with 8 before completing the request and the
 * `NHTTPi_SocRecv*` wrappers read the socket at +0x2C.  Only the fields the reconstructed bodies
 * touch are modelled. size: 0x30 (approximate) */
typedef struct NHTTPConnection {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;   /* 8 once the request was cancelled */
    /* +0x08 */ u8 pad_0x08[0x24];
    /* +0x2C */ NHTTPSock* sock;  /* the socket `NHTTPi_SocRecvOffsetRange` reads */
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
    /* +0x348 */ u32 field_0x348;
} NHTTPThreadInfo;

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
void NHTTPi_cancelAllRequests(NHTTPInfo* info);
void NHTTPi_CompleteCallback(void* connection, void* response); /* untyped: opaque handle */
NHTTPConnection* NHTTPi_Request2Connection(void* connection, s32 request); /* untyped: opaque handle */

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
