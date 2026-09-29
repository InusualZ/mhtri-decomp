/*
 * NHTTP_os_RVL.c - the Revolution SDK NHTTP library's RVL platform TU, `.text`
 * 0x80515010..0x80515774 (10 functions, 1892 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80515010 is `NHTTPi_CheckCurrentThread`, the
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
 * SECTIONS.  `.text` only.
 *
 * FLAGS.  `cflags_nhttp`, as `NHTTP_bgnend.c`.
 *
 * BODY.  Nine of the ten functions are reconstructed.  Five are byte-identical; the unit is
 * 63.08668 % fuzzy over its 1892 B and the five open rows are:
 *
 *    100.00  NHTTPi_RecvBufCopy         (324/324)  the ring's read half
 *    100.00  NHTTPi_SocRecvFromOffset   (32/32)
 *    100.00  NHTTPi_commThreadMain      (36/36)
 *    100.00  NHTTPi_isRecvBufFull       (28/28)
 *    100.00  NHTTPi_InitRequestInfo     (12/12)
 *     91.77  NHTTPi_RecvBufFindSpace   (248/244)
 *     79.67  NHTTPi_SocRecvOffsetRange (60/56)
 *     77.94  NHTTPi_RecvBufFindLine    (504/428)
 *     61.45  NHTTPi_CheckCurrentThread (152/176)
 *      0.00  NHTTPi_RecvBufFindUpper   (496 B, unwritten)
 *
 * NAMING (rule 7).  The six `fn_` rows this pass renamed in the map, and what each name rests on:
 * `0x805150CC` -> `NHTTPi_RecvBufFindLine` and `0x805152C4` -> `NHTTPi_RecvBufFindSpace`, from the
 * reconstructed walks' own stop characters (`'\r'`/`'\n'` and `' '`); `0x805155AC` ->
 * `NHTTPi_RecvBufCopy`, the ring's read half; `0x8051570C` -> `NHTTPi_SocRecvFromOffset` and
 * `0x8051572C` -> `NHTTPi_SocRecvOffsetRange`, named for what they do to `NHTTPi_SocRecv`'s
 * arguments (the wrappers forward their own unchanged, which is what pins the register mapping
 * their declarations use); and `0x805153BC` -> `NHTTPi_RecvBufFindUpper`, a **GUESS** - the row is
 * still unwritten, so the name is the registration lane's, from the target's `li r28,65` /
 * `li r31,90` seed: the `'A'..'Z'` test that folds a character by 32 before the `cmpw`, i.e. a
 * case-insensitive compare of the ring stream against a caller string.
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
 *   - `NHTTPi_RecvBufFindSpace` declares `index` before `block` for the same register reason.
 *   - Both walkers guard their range with the **positive** form, `if (start < end) { ...body... }
 *     return -1;`, not `if (start >= end) { return -1; } ...`: the negative form lays the `-1` out
 *     inline (`blt` over `li r3,-1` / `blr`) where retail's `bge` reaches a `-1` tail at the
 *     function's end and the loop exit shares it.  Measured: FindSpace 88.39 -> 91.77, FindLine
 *     76.27 -> 77.94.
 *
 * Residuals:
 *   - `NHTTPi_RecvBufFindLine` (77.94) and `NHTTPi_RecvBufFindSpace` (91.77) share the byte reader:
 *     retail *specialises* its first arm (its own `addi <index>,<index>,1` plus an `extsb`, then a
 *     branch to the shared `extsb`) where ours shares one tail; four spellings of the reader (`u8`
 *     value, `char` value and return, the `(char)` cast on the ring arm, post-increment in the
 *     subscript) were measured and all keep our shared tail, so ours stops 4 B short of retail's
 *     (244 B against 248).  Their *first* difference was the guard's polarity, which the positive
 *     form above closes.
 *   - `NHTTPi_RecvBufFindLine` alone carries a register residual: retail keeps two callee-saved
 *     registers (`r31` for the ring offset, `r30` for the seen-CR flag) with the `stw`/`lwz`
 *     prologue that implies, where ours allocates the same webs to caller-saved registers - there is
 *     no call anywhere in the function, so our allocator never needs to spill.  `FindSpace` has no
 *     such difference (its register file is r4/r5/r6/r7 with no prologue in either object).
 *     FindLine's own 76 B shortfall is separate: our build proves the LF arm's `result` assignment
 *     dead and answers with `beqlr` (two against retail's none), where retail materialises the
 *     result in a register and merges the exits; six declaration orders of the locals were measured
 *     (77.94 is the best, 75.28 the original).
 *   - NHTTPi_SocRecvOffsetRange (79.67): retail re-loads `sock->length` after the range check
 *     (`lwz r0,0x1c(r10)` at the test, `lwz r9,0x1c(r10)` again in the body) where ours reuses the
 *     first load.  The check, the clamp and the `sock->base + offset` computation all pair.
 *   - NHTTPi_CheckCurrentThread (61.45): the comm-thread guard.  Retail's `offThread` is true on the
 *     arm that panics when the caller IS the comm thread; the folded single-condition form measures
 *     61.45 % against the if/else forms' 55.00 %, and no C spelling of the two branches reproduces
 *     retail's `cmpwi r30,0` / `bne` / `cmplw` chain exactly.  The call site's *addressing* is open
 *     too, and decidable: ours materialises the OSPanic message with `lis`/`addi` where retail has
 *     the target object's **one** `SDA21` reloc, `li r5,NHTTPi_haltMessage@sda21`.  Giving the
 *     declaration its size (`extern const char NHTTPi_haltMessage[6];` in
 *     `include/unsplit/NHTTP.h`) reproduces that reloc, drops the function from 176 B to 172 B, and
 *     pairs its tail exactly (rows 36-47) - but the row *measures down* to 58.39474, because the
 *     removed `lis`/`addi` pair leaves one more row for objdiff's row-count normalisation.  The
 *     absolute form is kept while the comm-thread guard above is open; the sized declaration is the
 *     lever to take with it.
 *   - `NHTTPi_SocRecv`'s body and its six-parameter signature belong to `d_nhttp.c` (still
 *     unwritten); the wrappers here forward their own arguments unchanged, which is what pins the
 *     register mapping the declaration uses.
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
            value = ring->data[*index];
        } else {
            *block = ring->blocks;
            *index = 0;
            value = (*block)->data[*index];
        }
    } else {
        if (*index == 512) {
            *index = 0;
            *block = (*block)->next;
        }
        value = (*block)->data[*index];
    }
    (*index)++;
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
            char c = (char)NHTTPi_RecvBufNextByte(ring, &block, &index);

            if (c == ':' && offset != 0 && *offset < 0) {
                *offset = i;
            }
            if (seenCr != 0) {
                if (c == '\n') {
                    result = (i == end - 1) ? 0 : i + 1;
                    if (flag != 0) {
                        *flag = 2;
                    }
                    return result;
                }
            } else {
                if (c == '\r') {
                    seenCr = 1;
                    result = (i == end - 1) ? 0 : i + 1;
                    if (flag != 0) {
                        *flag = 1;
                    }
                }
                if (c == '\n') {
                    result = (i == end - 1) ? 0 : i + 1;
                    if (flag != 0) {
                        *flag = 1;
                    }
                    return result;
                }
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
    const char* messages = NHTTPi_threadCheckMessages;
    OSThread* current = OSGetCurrentThread();

    if (current != 0) {
        if (offThread ? (current == &info->thread) : (current != &info->thread)) {
            OSReport(messages + 0x1C, messages);
            OSPanic(messages + 0x30, 223, NHTTPi_haltMessage);
        }
    }
}
