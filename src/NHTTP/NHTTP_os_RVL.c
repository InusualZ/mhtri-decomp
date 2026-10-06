/*
 * NHTTP_os_RVL.c - the Revolution SDK NHTTP library's RVL platform TU, `.text`
 * 0x80515010..0x80515774 (10 functions, 1892 B).
 *
 * REGISTRATION.  Left edge 0x80515010 is `NHTTPi_CheckCurrentThread`, the
 * first referrer of the TU's private string, and the `.sdata` run-jump interval
 * (`0x80794394 -> 0x807943A0`, cuts [18653,18686]) admits it.  Right edge 0x80515774 is
 * `0x80515774`, the first referrer of the next TU's `.data` fragment (0x80630B28, `"https://"`,
 * `"CONNECT "`), so the split honours the per-TU data-fragment order; the `.sdata` run jump
 * `0x807943A0 -> 0x807943A8` (cuts [18686,18739]) admits it too.
 *
 * SOURCE FILE.  Real name recovered from the pool: `NHTTPi_CheckCurrentThread` loads
 * `.data:0x80630B18` = `"NHTTP_os_RVL.c"` (group 0x80630AE8, with the `"NHTTPi_CheckCurrentThread"`
 * and `"%s:illegal thread"` asserts).  The range is the RVL-side thread / receive-buffer helpers
 * (`NHTTPi_CheckCurrentThread`, `NHTTPi_isRecvBufFull`, `0x805150CC`/`0x805152C4`/`0x805153BC`/
 * `0x805155AC`); the runtime dump names none of them (`zz_0515xxx_`).
 *
 * LIBRARY.  `NHTTP` - the TU's own `__FILE__` string says NHTTP, and its callees are
 * `NHTTPi_lockReqList`/`0x80514EFC` (same library).
 *
 * SECTIONS.  `.text`, plus the `.data` assert strings and the `.sdata` halt message the comm-thread guard
 *   emits as literals (`__FUNCTION__` first, then the format and the file name).
 *
 * FLAGS.  `cflags_nhttp`, as `NHTTP_bgnend.c`.
 *
 * BODY.  All ten functions are reconstructed; six are byte-identical and the four open rows are:
 *
 *    100.00  NHTTPi_RecvBufCopy         (324/324)  the ring's read half
 *    100.00  NHTTPi_CheckCurrentThread  (152/152)
 *    100.00  NHTTPi_SocRecvFromOffset   (32/32)
 *    100.00  NHTTPi_commThreadMain      (36/36)
 *    100.00  NHTTPi_isRecvBufFull       (28/28)
 *    100.00  NHTTPi_InitRequestInfo     (12/12)
 *     96.60  NHTTPi_RecvBufFindSpace   (248)
 *     90.35  NHTTPi_RecvBufFindUpper   (496)
 *     97.16  NHTTPi_RecvBufFindLine    (504)
 *     79.67  NHTTPi_SocRecvOffsetRange (60/56)
 *
 * NAMING (rule 7).  The six `fn_` rows this pass renamed in the map, and what each name rests on:
 * `0x805150CC` -> `NHTTPi_RecvBufFindLine` and `0x805152C4` -> `NHTTPi_RecvBufFindSpace`, from the
 * reconstructed walks' own stop characters (`'\r'`/`'\n'` and `' '`); `0x805155AC` ->
 * `NHTTPi_RecvBufCopy`, the ring's read half; `0x8051570C` -> `NHTTPi_SocRecvFromOffset` and
 * `0x8051572C` -> `NHTTPi_SocRecvOffsetRange`, named for what they do to `NHTTPi_SocRecv`'s
 * arguments (the wrappers forward their own unchanged, which is what pins the register mapping
 * their declarations use); and `0x805153BC` -> `NHTTPi_RecvBufFindUpper`, a **GUESS** from the
 * `'A'..'Z'` test that folds both characters by 32 before the `cmpw`: a case-insensitive compare of
 * the ring stream against a caller string.
 *
 * The ring's layout, read off these bodies: `+0x00` is the ring's total byte count, `+0x34` heads a
 * singly-linked list of blocks and `+0x38` is the first 1024 bytes of stream.  A ring offset below
 * 1024 is `data[offset]`; from 1024 on it is `blocks[(offset - 1024) / 512]->data[(offset - 1024) &
 * 511]`, so one block carries 512 bytes behind its 4-byte link.  The three walkers share a
 * stateful byte reader (a `static` MWCC inlines at every site, so it has no symbol): it keeps the
 * current block and offset and advances the offset past the byte it answers.
 *
 * Load-bearing source shapes (each measured):
 *   - `NHTTPi_RecvBufCopy` declares `chunk` before `block` (the allocator gives the first-declared
 *     local the higher callee-saved register, which is what pairs `ring`/`chunk` with retail's
 *     r30/r31) and spells each clamp `chunk = length; if (chunk > <container left>) chunk = ...`
 *     rather than `min`; `offset += chunk; offset &= 511;` as two statements is what keeps the
 *     add in place instead of through a temporary.  Its outer test is the *negated* one
 *     (`if (offset + length <= ring->capacity) { ... return 1; } return 0;`), which is what puts
 *     retail's two exit blocks where they are.
 *   - The block-list walk is `while (n != 0) { block = block->next; n--; }`: that is the shape MWCC
 *     unrolls by 8 (`srwi. r0,r3,3; mtctr; 8x; bdnz; andi. r3,r3,7; mtctr; 1x; bdnz`).  A
 *     `for (i = 0; i < n; i++)` over a separate counter emits a signed guard instead and does not
 *     pair.
 *   - `NHTTPi_RecvBufFindSpace` declares `index` before `block` for the same register reason;
 *     `NHTTPi_RecvBufFindUpper` declares `block` first.
 *   - The byte reader post-increments the offset inside each subscript (`data[(*index)++]`).
 *   - `NHTTPi_RecvBufFindUpper` folds case with `((c >= 'A') & (c <= 'Z')) ? c + 32 : c` (the bitwise
 *     `&` is what gives retail's carry-arithmetic booleans), compares `lower(c) == lower(*name)` as a
 *     `while` condition (the rotated loop with its entry branch) and guards with the positive form.
 *   - Both walkers guard their range with the **positive** form, `if (start < end) { ...body... }
 *     return -1;`, not `if (start >= end) { return -1; } ...`: the negative form lays the `-1` out
 *     inline (`blt` over `li r3,-1` / `blr`) where retail's `bge` reaches a `-1` tail at the
 *     function's end and the loop exit shares it.  Measured: FindSpace 88.39 -> 91.77, FindLine
 *     76.27 -> 77.94.
 *
 * Residuals:
 *   - `NHTTPi_RecvBufFindUpper` (90.35): retail sign-extends the byte as it reads it and reloads the
 *     index into the shared block arm (`li r9,0`, then the shared `data[index++]`), ours folds the
 *     first block read to `li r9,1`; the remaining rows are register numbering.
 *   - `NHTTPi_RecvBufFindLine` (97.16) and `NHTTPi_RecvBufFindSpace` (96.60) share the byte reader:
 *     retail *specialises* its first arm (its own `addi <index>,<index>,1` plus an `extsb`) where ours
 *     shares one tail. FindLine answers on the byte after a CR whatever it is (2 in `*flag` when it is
 *     the LF), and compares the raw byte through `(char)` casts, which is what keeps retail's two saved
 *     registers and its per-test `extsb`.
 *   - NHTTPi_SocRecvOffsetRange (79.67): retail re-loads `sock->length` after the range check
 *     (`lwz r0,0x1c(r10)` at the test, `lwz r9,0x1c(r10)` again in the body) where ours reuses the
 *     first load.  The check, the clamp and the `sock->base + offset` computation all pair.
 *   - `NHTTPi_SocRecv`'s body and its six-parameter signature belong to `d_nhttp.c` (still
 *     unwritten); the wrappers here forward their own arguments unchanged, which is what pins the
 *     register mapping the declaration uses.
 * RESIDUALS. `NHTTPi_CheckCurrentThread`: retail relocates its strings against the map labels `NHTTPi_threadCheckMessages`
 *   / `NHTTPi_haltMessage`, ours against the anonymous literals the source emits (same bytes, same offsets).
 */
#include "types.h"
#include "NHTTP/NHTTP_os_RVL.h"   /* this unit's own types (rule 1) */



/* The receive ring's byte reader the walkers below share: `*index` is the ring offset and `*block`
 * is the 512-byte block the offset currently lives in (null while the offset is still inside the
 * ring's own first 1024 bytes).  Both are advanced past the byte the call returns.  MWCC inlines it
 * at every call site, so it has no symbol of its own. */
static u8 NHTTPi_RecvBufNextByte(NHTTPRecvBuf* ring, NHTTPRecvBlock** block, s32* index) {
    u8 value;

    if (*block == 0) {
        if (*index < 1024) {
            value = ring->data[(*index)++];
        } else {
            *block = ring->blocks;
            *index = 0;
            value = (*block)->data[(*index)++];
        }
    } else {
        if (*index == 512) {
            *index = 0;
            *block = (*block)->next;
        }
        value = (*block)->data[(*index)++];
    }
    return value;
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
    s32 seenCr;
    s32 result;
    s32 index;
    NHTTPRecvBlock* block;
    s32 i;
    s32 n;

    if (offset != 0) {
        *offset = -1;
    }
    if (start < end) {
        seenCr = 0;
        result = -1;
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
            u8 c = NHTTPi_RecvBufNextByte(ring, &block, &index);

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

/* 0x805152C4 (0xF8): the index of the first space in the ring's stream between `start` and `end`,
 * or -1 when there is none. */
s32 NHTTPi_RecvBufFindSpace(NHTTPRecvBuf* ring, s32 start, s32 end) {
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
            if ((char)NHTTPi_RecvBufNextByte(ring, &block, &index) == ' ') {
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
    char c;

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
        c = (char)NHTTPi_RecvBufNextByte(ring, &block, &index);
        while (NHTTPI_TOLOWER(c) == NHTTPI_TOLOWER(*name)) {
            if (*name == '\0' || *name == ' ' || *name == terminator || start == end - 1) {
                return 0;
            }
            c = (char)NHTTPi_RecvBufNextByte(ring, &block, &index);
            start++;
            name++;
        }
    }
    return -1;
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

    if (sock->length <= (u32)offset) {
        return -1003;
    }
    buf = sock->base + offset;
    if (length > (s32)(sock->length - offset)) {
        length = sock->length - offset;
    }
    return NHTTPi_SocRecv(handle, conn, flags, buf, length, arg);
}

/* 0x805156F0 (0x1C): true once the receive ring holds at least `size` bytes. */
BOOL NHTTPi_isRecvBufFull(NHTTPRecvBuf* info, u32 size) {
    return info->length <= size;
}

/* 0x80515768 (0xC): clear the request-info record. */
void NHTTPi_InitRequestInfo(NHTTPRequestInfo* info) {
    info->active = NULL;
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
