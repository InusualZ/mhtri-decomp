/*
 * NHTTP/NHTTP_os_RVL.h - the NHTTP RVL platform unit (`src/NHTTP/NHTTP_os_RVL.c`, `.text`
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

/* one block of the receive ring's overflow list: the next block, then 512 bytes of stream.  The
 * helpers index `data` with the ring offset masked to 9 bits, so the block is 4 + 512 B. */
typedef struct NHTTPRecvBlock {
    /* +0x000 */ struct NHTTPRecvBlock* next;
    /* +0x004 */ u8 data[0x200];
} NHTTPRecvBlock; /* size: 0x204 */

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
    /* +0x000 */ s32 capacity;               /* the ring's total byte count (NHTTPi_RecvBufCopy) */
    /* +0x004 */ u8 pad_0x004[0x18];
    /* +0x01C */ u32 length;                 /* bytes buffered (NHTTPi_isRecvBufFull) */
    /* +0x020 */ u8 pad_0x020[0x14];
    /* +0x034 */ NHTTPRecvBlock* blocks;     /* the 512-byte block list */
    /* +0x038 */ u8 data[0x400];             /* everything below offset 1024 lives here */
} NHTTPRecvBuf;

/* 0x805155AC (0x144): copy `length` bytes out of the receive ring starting at `offset` - the first
 * 1024 bytes live in the ring itself, everything past them in the 512-byte block list, and each
 * chunk is clamped to what its container has left.  Answers 0 when the range runs past the ring's
 * capacity, 1 otherwise. */
BOOL NHTTPi_RecvBufCopy(NHTTPRecvBuf* ring, u8* dst, s32 offset, s32 length);

/* 0x805150CC (0x1F8): walk the ring from `start` to `end` for the end of a line, recording the
 * first `:` in `*offset` and the terminator it found (1 = LF, 2 = CRLF) in `*flag`.  Answers the
 * offset past the terminator (0 when the terminator ends the range), or -1 when there is none. */
s32 NHTTPi_RecvBufFindLine(NHTTPRecvBuf* ring, s32 start, s32 end, s32* offset, s32* flag);

/* 0x8051570C (0x20): hand the raw receive the socket buffer that starts `offset` bytes into the
 * connection's receive area, with the length left from there.  Retail keeps the tail call, so the
 * two are one pair. */
s32 NHTTPi_SocRecvFromOffset(s32 handle, NHTTPConnection* conn, s32 flags, s32 offset, s32 arg);

/* 0x8051572C (0x3C): the same, clamped to what the socket has left and refusing an offset past the
 * end with -1003. */
s32 NHTTPi_SocRecvOffsetRange(s32 handle, NHTTPConnection* conn, s32 flags, s32 offset, s32 length,
                              s32 arg);

/* 0x805152C4 (0xF8): the index of the first space in the ring's stream between `start` and `end`, or
 * -1 when there is none. */
s32 NHTTPi_RecvBufFindSpace(NHTTPRecvBuf* ring, s32 start, s32 end);

/* 0x805153BC (0x1F0): the case-insensitive search `NHTTPi_findHeaderField` names a header with: 0 when
 * the ring's stream between `start` and `end` holds `name` (each character compared upper-cased, the
 * `terminator` character also ending it), -1 when it does not.  Declared from its callers in
 * `d_nhttp.c`. */
s32 NHTTPi_RecvBufFindUpper(NHTTPRecvBuf* ring, s32 start, s32 end, const char* name, char terminator);

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

/* Declarations moved here from `unsplit/NHTTP.h` (docs/plan.md 6.5 rule 2: the owner declares). */

#endif /* MHTRI_NHTTP_NHTTP_OS_RVL_H */
