/*
 * include/NHTTP/NHTTP_os_RVL.h - the NHTTP RVL platform unit (`src/NHTTP/NHTTP_os_RVL.c`, `.text`
 * 0x80515010..0x80515774).
 *
 * Rule 2: this unit owns `NHTTPi_InitRequestInfo` (and the RVL thread/receive-buffer helpers), so
 * `d_nhttp.c` includes this header instead of declaring them.
 */
#ifndef MHTRI_NHTTP_NHTTP_OS_RVL_H
#define MHTRI_NHTTP_NHTTP_OS_RVL_H

#include "types.h"
#include "NHTTP/d_nhttp.h"
#include "unsplit/OS.h"

/* one block of the receive ring's overflow list: the next block, then 508 bytes of stream */
typedef struct NHTTPRecvBlock {
    /* +0x000 */ struct NHTTPRecvBlock* next;
    /* +0x004 */ u8 data[0x1FC];
} NHTTPRecvBlock; /* size: 0x200 */

#ifdef __cplusplus
extern "C" {
#endif

void NHTTPi_InitRequestInfo(NHTTPRequestInfo* info);

/* 0x80515010 (0x98): the comm-thread guard - it panics with the TU's `%s:illegal thread` assert
 * when the caller is on / is not on the comm thread, per `onThread`.  Retail's call site in
 * `NHTTPi_CleanupAsync` passes 1. */
void NHTTPi_CheckCurrentThread(NHTTPThreadInfo* info, BOOL onThread);

/* The receive ring the RVL helpers walk: a 0x38-byte header whose 0x34 slot heads a list of
 * 512-byte blocks for the part of the stream past the first 1024 bytes, then the 1024 bytes the
 * header addresses directly.  Only the fields the helpers touch are modelled. size: 0x438 */
typedef struct NHTTPRecvBuf {
    /* +0x000 */ u8 pad_0x000[0x1C];
    /* +0x01C */ u32 length;                 /* bytes buffered (NHTTPi_isRecvBufFull) */
    /* +0x020 */ u8 pad_0x020[0x14];
    /* +0x034 */ NHTTPRecvBlock* blocks;     /* the 512-byte block list */
    /* +0x038 */ u8 data[0x400];             /* everything below offset 1024 lives here */
} NHTTPRecvBuf;

/* 0x805156F0 (0x1C): true when the ring holds at least `size` bytes. */
BOOL NHTTPi_isRecvBufFull(NHTTPRecvBuf* info, u32 size);

/* 0x805150A8 (0x24): the comm thread's OS entry point - it runs the request loop once and answers
 * 0.  Named from the only call site (`NHTTPi_createCommThread` hands it to OSCreateThread), a GUESS
 * like the rest of this band's names. */
/* untyped: opaque handle */
void* NHTTPi_commThreadMain(void* arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_OS_RVL_H */
