/*
 * NHTTP_os_RVL.c - the Revolution SDK NHTTP library's RVL platform TU: the comm-thread guard and entry
 *   point, the receive ring's walkers and copier, and the two socket-receive wrappers.
 * RANGE. .text 0x80515010-0x80515774 (10 functions, 1892 B), in address order; .data 0x80630AE8-0x80630B28
 *   (the guard's assert strings) and .sdata 0x807943A0-0x807943A8 (its halt message).  Left edge: the
 *   `.sdata` run jump 0x80794394 -> 0x807943A0 and the first referrer of the TU's private strings.  Right
 *   edge 0x80515774: the first referrer of `d_nhttp.c`'s `.data` fragment (0x80630B28, `"https://"`).
 * SOURCE FILE. `NHTTPi_CheckCurrentThread` panics with `"NHTTP_os_RVL.c"` (`.data` 0x80630B18).
 * FLAGS. `cflags_nhttp` (GC/3.0a5.2), as `NHTTP_bgnend.c`.
 * NAMES. Map names derived from the bodies (the runtime dump has only `zz_0515xxx_`): `NHTTPi_RecvBufFindLine`,
 *   `NHTTPi_RecvBufCopy`, `NHTTPi_SocRecvFromOffset`, `NHTTPi_SocRecvOffsetRange`; GUESS:
 *   `NHTTPi_RecvBufFindUpper` (a case-insensitive compare of the stream against a caller string),
 *   `NHTTPi_RecvBufSkipSpaces` (the index of the first byte that is not a space; the map row was
 *   `NHTTPi_RecvBufFindSpace`, renamed by request net-nhttp#1).
 * LAYOUT. The ring: `+0x00` total byte count, `+0x34` the 512-byte block list, `+0x38` the first 1024
 *   bytes of stream; offset `o >= 1024` lives in `blocks[(o - 1024) / 512]->data[(o - 1024) & 511]`.
 * SHAPES. The three walkers share `NHTTPi_RecvBufNextByte` (`static inline`, no symbol): its first arm
 *   returns `(char)ring->data[i++]` and every other path falls into the shared `block->data[i++]`
 *   return, as an `s32`; each walker keeps the byte in an `s32` and compares `(char)c`, which is what
 *   gives retail's sign-extension in the first arm and again at every test.
 *   `NHTTPi_RecvBufFindLine` declares `result, n, i, block, index, seenCr` in that order and sets
 *   `result = -1` before `seenCr = 0` (the register colouring).  `NHTTPi_RecvBufCopy` declares `chunk`
 *   before `block`, spells each clamp `chunk = length; if (chunk > left) chunk = left;` and keeps
 *   `offset += chunk; offset &= 511;` as two statements.  The block-list walk is `while (n != 0) {
 *   block = block->next; n--; }` (MWCC unrolls it by 8).  Every walker guards with the positive form
 *   `if (start < end) { ... } return -1;`.  `NHTTPSock.length` is `volatile` (d_nhttp.h): retail reads it
 *   once for the range check and again for the clamp in `NHTTPi_SocRecvOffsetRange`.
 * RESIDUALS. `NHTTPi_CheckCurrentThread` relocates its strings against the anonymous literals the source
 *   emits where retail names `NHTTPi_threadCheckMessages` / `NHTTPi_haltMessage` (same bytes, same offsets;
 *   flipcheck lists the `.sdata` name, the trial flip links byte-identical).
 */
#include "types.h"
#include "NHTTP/NHTTP_os_RVL.h"   /* this unit's own types (rule 1) */
#include "NHTTP/NHTTP_request.h" /* NHTTPi_SocRecv */
#include "NHTTP/NHTTP_util.h"    /* NHTTPi_memcpy */
#include "NHTTP/NHTTP_thread.h"  /* NHTTPi_commThreadLoop */

/* The receive ring's byte reader the walkers below share: `*index` is the ring offset and `*block`
 * is the 512-byte block the offset currently lives in (null while the offset is still inside the
 * ring's own first 1024 bytes).  Both are advanced past the byte the call returns. */
static inline s32 NHTTPi_RecvBufNextByte(NHTTPRecvBuf* ring, NHTTPRecvBlock** block, s32* index) {
    if (*block == 0) {
        if (*index < 1024) {
            return (char)ring->data[(*index)++];
        }
        *block = ring->blocks;
        *index = 0;
    } else if (*index == 512) {
        *index = 0;
        *block = (*block)->next;
    }
    return (*block)->data[(*index)++];
}

/* 0x80515010 (0x98): assert the caller's thread.  `offThread` selects which side is legal - with
 * it set the caller must NOT be the comm thread (`NHTTPi_CleanupAsync` passes 1), with it clear the
 * caller must be on it.  Either way the failure path is the TU's `%s:illegal thread` assert. */
void NHTTPi_CheckCurrentThread(NHTTPThreadInfo* info, BOOL offThread) {
    OSThread* current = OSGetCurrentThread();
    OSThread* comm = &info->thread;

    if (current != 0) {
        if ((!offThread && current != comm) || (offThread && current == comm)) {
            OSReport("%s:illegal thread\n", __FUNCTION__);
            OSPanic("NHTTP_os_RVL.c", 223, "halt\n");
        }
    }
}

/* 0x805150A8 (0x24): the comm thread's OS entry point - the request loop runs once and the thread
 * answers 0.  Its argument is the thread's own payload and is not read. */
/* untyped: opaque handle */
void* NHTTPi_commThreadMain(void* arg) {
    (void)arg;
    NHTTPi_commThreadLoop();
    return 0;
}

/* 0x805150CC (0x1F8): walk the ring from `start` to `end` looking for the end of a line.  The
 * walk records the offset of the first `:` in `*offset` (left at -1 when there is none) and reports
 * the line ending it found in `*flag`: 1 for a bare LF and 2 for a CRLF pair.  The answer is the
 * offset just past the terminator, or 0 when the terminator is the last byte of the range; -1 when
 * the range holds no line ending. */
s32 NHTTPi_RecvBufFindLine(NHTTPRecvBuf* ring, s32 start, s32 end, s32* offset, s32* flag) {
    s32 result;
    s32 n;
    s32 i;
    NHTTPRecvBlock* block;
    s32 index;
    s32 seenCr;

    if (offset != 0) {
        *offset = -1;
    }
    if (start < end) {
        result = -1;
        seenCr = 0;
        if (start < 1024) {
            index = start;
            block = 0;
        } else {
            block = ring->blocks;
            n = (start - 1024) >> 9;
            while (n != 0) {
                block = block->next;
                n--;
            }
            index = (start - 1024) & 511;
        }
        for (i = start; i < end; i++) {
            s32 c = NHTTPi_RecvBufNextByte(ring, &block, &index);

            if ((char)c == ':' && offset != 0 && *offset < 0) {
                *offset = i;
            }
            if (seenCr != 0) {
                if ((char)c == '\n') {
                    result = (i == end - 1) ? 0 : i + 1;
                    if (flag != 0) {
                        *flag = 2;
                    }
                }
                return result;
            }
            if ((char)c == '\r') {
                seenCr = 1;
                result = (i == end - 1) ? 0 : i + 1;
                if (flag != 0) {
                    *flag = 1;
                }
            }
            if ((char)c == '\n') {
                s32 lf = (i == end - 1) ? 0 : i + 1;
                if (flag != 0) {
                    *flag = 1;
                }
                return lf;
            }
        }
    }
    return -1;
}

/* 0x805152C4 (0xF8): skips spaces - the index of the first byte between `start` and `end` that is not
 * a space, or -1 when there is none (a header value's start after its colon). */
s32 NHTTPi_RecvBufSkipSpaces(NHTTPRecvBuf* ring, s32 start, s32 end) {
    s32 index;
    NHTTPRecvBlock* block;
    s32 i;
    s32 n;

    if (start < end) {
        if (start < 1024) {
            index = start;
            block = 0;
        } else {
            block = ring->blocks;
            n = (start - 1024) >> 9;
            while (n != 0) {
                block = block->next;
                n--;
            }
            index = (start - 1024) & 511;
        }
        for (i = start; i < end; i++) {
            if ((char)NHTTPi_RecvBufNextByte(ring, &block, &index) != ' ') {
                return i;
            }
        }
    }
    return -1;
}

/* Lower-cases an ASCII letter. */
#define NHTTPI_TOLOWER(c) ((((c) >= 'A') & ((c) <= 'Z')) ? (c) + 32 : (c))

/* 0x805153BC (0x1F0): compares the ring bytes from `start` against `name` without case; 0 when they agree up to
 * the end of `name` (a NUL, a space or `terminator` also ending it) or up to `end`, -1 when they do not. */
s32 NHTTPi_RecvBufFindUpper(NHTTPRecvBuf* ring, s32 start, s32 end, const char* name, char terminator) {
    NHTTPRecvBlock* block;
    s32 index;
    s32 n;
    s32 c;

    if (start < end) {
        if (start < 1024) {
            index = start;
            block = 0;
        } else {
            block = ring->blocks;
            n = (start - 1024) >> 9;
            while (n != 0) {
                block = block->next;
                n--;
            }
            index = (start - 1024) & 511;
        }
        c = NHTTPi_RecvBufNextByte(ring, &block, &index);
        while (NHTTPI_TOLOWER((char)c) == NHTTPI_TOLOWER(*name)) {
            if (*name == '\0' || *name == ' ' || *name == terminator || start == end - 1) {
                return 0;
            }
            c = NHTTPi_RecvBufNextByte(ring, &block, &index);
            start++;
            name++;
        }
    }
    return -1;
}

/* 0x805155AC (0x144): copy `length` bytes out of the receive ring starting at `offset`.  The
 * first 1024 bytes live in the ring itself, everything past them in the 512-byte block list, and
 * each chunk is clamped to what its container has left; the block list is walked once to the block
 * the offset lands in, then followed one link per chunk. */
BOOL NHTTPi_RecvBufCopy(NHTTPRecvBuf* ring, u8* dst, s32 offset, s32 length) {
    s32 chunk;
    NHTTPRecvBlock* block;
    s32 n;

    if (offset + length <= ring->capacity) {
        if (length != 0) {
            if (offset < 1024) {
                chunk = length;
                if (chunk > 1024 - offset) {
                    chunk = 1024 - offset;
                }
                NHTTPi_memcpy(dst, ring->data + offset, chunk);
                offset += chunk;
                length -= chunk;
                dst += chunk;
            }
            if (length != 0) {
                block = ring->blocks;
                offset -= 1024;
                n = offset >> 9;
                offset &= 511;
                while (n != 0) {
                    block = block->next;
                    n--;
                }
                while (length != 0) {
                    chunk = length;
                    if (chunk > 512 - offset) {
                        chunk = 512 - offset;
                    }
                    NHTTPi_memcpy(dst, block->data + offset, chunk);
                    offset += chunk;
                    offset &= 511;
                    block = block->next;
                    length -= chunk;
                    dst += chunk;
                }
            }
        }
        return 1;
    }
    return 0;
}

/* 0x805156F0 (0x1C): true once the receive ring holds at least `size` bytes. */
BOOL NHTTPi_isRecvBufFull(NHTTPRecvBuf* info, u32 size) {
    return info->length <= size;
}

/* 0x8051570C (0x20): hand the raw receive the socket buffer that starts `offset` bytes into the
 * connection's receive area, with the length that is left from there. */
s32 NHTTPi_SocRecvFromOffset(s32 handle, NHTTPConnection* conn, s32 flags, s32 offset, s32 arg) {
    NHTTPSock* sock = conn->sock;

    return NHTTPi_SocRecv(handle, conn, flags, sock->base + offset, sock->length - offset, arg);
}

/* 0x8051572C (0x3C): the same, but it refuses an offset past the end of the socket's buffer with
 * -1003 and clamps the length to what the buffer has left. */
s32 NHTTPi_SocRecvOffsetRange(s32 handle, NHTTPConnection* conn, s32 flags, s32 offset, s32 length,
                              s32 arg) {
    NHTTPSock* sock = conn->sock;
    u8* buf;
    s32 left;

    if (sock->length <= (u32)offset) {
        return -1003;
    }
    left = sock->length - offset;
    buf = sock->base + offset;
    if (length > left) {
        length = left;
    }
    return NHTTPi_SocRecv(handle, conn, flags, buf, length, arg);
}

/* 0x80515768 (0xC): clear the request-info record. */
void NHTTPi_InitRequestInfo(NHTTPRequestInfo* info) {
    info->active = NULL;
}
