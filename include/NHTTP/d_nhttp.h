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

/* The comm-thread info. size: 0x34C */
typedef struct NHTTPThreadInfo {
    /* +0x000 */ u8 pad_0x000[0x348];
    /* +0x348 */ u32 field_0x348;
} NHTTPThreadInfo;

/* The NHTTP system-info block. size: 0xB8C */
typedef struct NHTTPInfo {
    /* +0x000 */ u8 pad_0x000[0x7C4];
    /* +0x7C4 */ void* alloc_fn;   /* registered allocator (NHTTPi_alloc) */
    /* +0x7C8 */ void* free_fn;    /* registered free (NHTTPi_free) */
    /* +0x7CC */ u32 field_0x7CC;
    /* +0x7D0 */ s32 field_0x7D0;
    /* +0x7D4 */ s32 ssl_error;     /* NHTTPi_SetSSLError / NHTTPi_GetSSLError */
    /* +0x7D8 */ s32 error;         /* NHTTPi_SetError / NHTTPi_GetError */
    /* +0x7DC */ u32 field_0x7DC;
    /* +0x7E0 */ u32 field_0x7E0;
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
void* NHTTPi_GetBgnEndInfoP(NHTTPInfo* info);
void* NHTTPi_GetListInfoP(NHTTPInfo* info);
void* NHTTPi_GetReqInfoP(NHTTPInfo* info);
void* NHTTPi_GetThreadInfoP(NHTTPInfo* info);
void* NHTTPi_GetMutexInfoP(NHTTPInfo* info);
void* NHTTPi_memcpy(void* dst, const void* src, u32 n);
void* NHTTPi_memclr(void* dst, u32 n);
void NHTTPi_InitThreadInfo(NHTTPThreadInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_D_NHTTP_H */
