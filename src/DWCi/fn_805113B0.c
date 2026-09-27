/*
 * the DWCi transport unit: `.text` 0x805113B0-0x80512490 (20 functions, 4320 B), the band of the
 * Nintendo Wi-Fi Connection (DWCi) SDK block the game links between its own code and the NHTTP
 * library.
 *
 * SEAM MOVED (recon lane, 2026-09-27).  This unit was registered as 0x805113B0..0x805124F4; the
 * right edge is now 0x80512490.  `tools/splits/tudiscover.py at 0x80512490` reports a STRONG x2
 * cut there (.sdata run jumps DWCi_addressFormatPort -> DWCi_emptyString and DWCi_emptyString -> 0x80794370
 * intersect at the single cut 18642), so a TU begins at 0x80512490 and the 0x64 overlap with the
 * NHTTP-side probe is exactly `DWCi_GetStringLength`.  That function's definition moved to the new
 * `DWCi/DWCi_NatNeg.c` (0x80512490..0x805145B8); the forward declaration below stays because the
 * bodies here still call it.  Re-measured before/after: 91.1543 % / 21 functions / 472 B matched
 * (before) -> 90.9439 % / 20 functions / 372 B matched (after) - the 100 B loss is exactly the
 * 100 %-matched `DWCi_GetStringLength` leaving the unit, not a regression in any remaining body.
 *
 * NAMING (rule 7, no exemption).  The symbol map carries only placeholder names for this band
 * (`tools/symbols/dumpmap.py lookup` answers `zz_`/`fn_` for every row of the range; the whole DOL's
 * string pool holds no source-file name this range references), so all 66 generated names the file
 * used are gone: the map rows, the definitions and the declarations were renamed through
 * `symedit.py rename` in one change.  The 20 functions the unit *defines* are named from their
 * bodies and their header comments (`DWCi_sendControlFrame`, `DWCi_appendTransfer`,
 * `DWCi_flushRequest`, `DWCi_requestTableHash`/`Compare`, `DWCi_requestListRelease`,
 * `DWCi_findRequest`, `DWCi_createConnection`, `DWCi_destroyConnection`,
 * `DWCi_setConnectionUserValue`, `DWCi_createRequest`, `DWCi_removeRequest`, `DWCi_sendTo`,
 * `DWCi_requestTick`, `DWCi_connectionTick`, `DWCi_connectionFlushRequests`,
 * `DWCi_connectionShutdown`, `DWCi_htons`, `DWCi_formatAddress`, `DWCi_parseAddress`); the 39
 * helpers it *calls* that no registered unit owns (`DWCi_list*`, `DWCi_table*`, `DWCi_socket*`,
 * `DWCi_malloc`/`free`, `DWCi_buffer*`, `DWCi_request*`, `DWCi_platform*`, `DWCi_getTick`, and the
 * two SO address helpers) and the 7 band data objects are named from this file's own call sites and
 * are declared in `include/unsplit/DWCi.h` / `include/unsplit/SO.h`.  Every one of those is a GUESS
 * and a marker for a later reconstruction to confirm - only the argument/return shapes at the call
 * sites, not a recovered SDK spelling, back them.
 *
 * Seam, module and language:
 *   - the range is a placeholder-name run discovery proposed (`--max-bytes` cut; no `__FILE__` string
 *     covers it), and `tools/splits/tudiscover.py at 0x805113B0` finds no closure edge, no must-link
 *     anchor and no labelled data the range references - the boundary is unconstrained, so the run
 *     is worked as one unit.  The registered neighbours by address are `RSO/runtime.c` below and
 *     `homebutton/keyboard.cpp` above.
 *   - module: the range sits inside the DWCi block of the retail link order - the only named symbol
 *     of that block is `DWCi_Np_CPUCopyFast` (0x80507C40) and the next block up is the NHTTP library
 *     (`NHTTPi_alloc`, 0x805145E8); every callee of this range (0x8050A-0x8050E) is inside that same
 *     DWCi block, and the block's `.rodata` holds the DWC library's own strings (`DWC_AUTH`,
 *     `https://naswii.nintendowifi.net/pr`, `dwc_auth_interface.c`).  No `__FILE__` string names the
 *     original file, so the module directory is `DWCi` (the library's own spelling) and the file name
 *     stays the map's stem - brief section 2, class 3+4.
 *   - language: C.  Every callee is a plain C symbol (`memcpy`, `sprintf`, `SOHtoNs`, the 0x8050xxxx
 *     helpers) and no defined name in the range is mangled.
 *
 * Sections: .text 0x805113B0..0x80512490 only.  The range loads three unowned data objects
 * (`DWCi_protocolMagic`'s 2-byte protocol constant, the `%s:%d`/`%s`/`:%d` format strings at
 * 0x80794358..0x80794368, and `.data` 0x8060EDB0 + the `.bss` counter 0x80795820 / ring buffers
 * 0x807625C0); none of them is claimed here - a data claim has to be measured before and after
 * (playbook 55).
 *
 * Registered NonMatching in configure.py, lib DWCi (Wii/1.3).  All 20 bodies are reconstructed and every
 * one of them is at or above the 80 % bar (unit fuzzy 90.94 % over the 4320 B of .text, 7 byte-identical,
 * 372 B matched; `DWCi_GetStringLength` is 100 % in its new home `DWCi/DWCi_NatNeg.c`):
 *
 *   100.00  DWCi_requestTableHash  DWCi_requestListRelease  DWCi_destroyConnection  DWCi_setConnectionUserValue  DWCi_connectionTick  DWCi_connectionShutdown
 *           DWCi_htons
 *    95.38  DWCi_requestTableCompare        94.92  DWCi_flushRequest        94.56  DWCi_formatAddress
 *    91.87  DWCi_sendTo        91.76  DWCi_removeRequest        91.32  DWCi_connectionFlushRequests
 *    90.74  DWCi_sendControlFrame        89.96  DWCi_appendTransfer        88.59  DWCi_requestTick
 *    87.83  DWCi_createConnection        87.10  DWCi_parseAddress        86.79  DWCi_createRequest
 *    99.79  DWCi_findRequest
 *
 * Residuals (all measured with `recompile.py` on this file `--measure <symbol>`; none is a seam or a
 * type error - they are codegen shapes):
 *   - DWCi_parseAddress (87.10): retail's parser uses goto-shaped early exits - the empty-string case and the
 *     colon-at-start case jump straight to the store and to the port parse.  Rule 8 forbids `goto`, so the
 *     conformant nested-if shape is used; it measures 87.10 (368 B) against the target's 400 B, while the
 *     earlier flat `if (str != 0 && *str != 0)` shape measured 82.30 (352 B).  Both numbers are here
 *     because the rule-8 shape is not the byte-identical one.
 *   - DWCi_createRequest (86.79) / DWCi_findRequest (99.79): the frames are smaller than retail's (0x30 vs 0x170 and
 *     0x20 vs 0xB0).  The original TU keeps a large scratch block in these two functions that no field of
 *     the recovered types accounts for; instruction counts are equal, so only the prologue/epilogue pairs
 *     differ.  DWCi_createRequest also builds the same key twice, as retail does.
 *   - DWCi_createConnection (87.83) / DWCi_sendTo (91.87) / DWCi_removeRequest (91.76) / DWCi_connectionFlushRequests (91.32) /
 *     DWCi_requestTick (88.59): register colouring and block order; instruction counts equal or within one.
 *   - DWCi_appendTransfer (89.96): the two header bytes go out as `extrwi`+`stbx` in retail and `srawi`+`stb`
 *     here (same bytes, different materialisation).
 *   - DWCi_sendControlFrame (90.74): the conn/port/addr load order and one `or`-based bool tail.
 *
 * Flags: `cflags_dwc = cflags_base + "-func_align 4"`, the same command-line shape as the sibling
 * `cflags_os`/`cflags_network`; nothing was tuned per function.  `tools/flags/infer.py` on the split object
 * reports peephole on, lmw_stmw off and func_align 16 - the 16-byte alignment is the split's own `gap_*`
 * padding symbols (15 of them, 116 B), the same situation configure.py's comment above `cflags_network`
 * documents.
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers (rule 2) */
#include "unsplit/SO.h"                  /* SOAddressToString / SOAddressToHostPort / SOHtoNs */
#include "unsplit/Runtime.PPCEABI.H.h"   /* sprintf / strchr / strlen / atoi */

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

typedef struct DWCiConn DWCiConn; /* size: 0x4C */
typedef struct DWCiReq DWCiReq;   /* size: 0xA0 */
typedef struct DWCiXfer DWCiXfer; /* one transfer record handed to DWCi_appendTransfer */

/* The address record the transport hands to the socket wrappers: one length byte, one family byte,
 * the network-order port and the network-order address (the same 8 bytes `bind`/`connect` take). */
typedef struct DWCiSockAddrIn {
    /* +0x00 */ u8 len;
    /* +0x01 */ u8 family;
    /* +0x02 */ u16 port;
    /* +0x04 */ u32 addr;
} DWCiSockAddrIn; /* size: 0x08 */

/* The `{ addr, port }` key the connection's request table hashes and compares on.  The table API
 * takes the key *indirectly* (a `DWCiAddrKey**`): DWCi_requestTableHash/DWCi_requestTableCompare dereference their first
 * argument once and only then read +0x00/+0x04.  A DWCiReq starts with exactly these six bytes, so
 * a request is its own key. */
typedef struct DWCiAddrKey {
    /* +0x00 */ u32 addr;
    /* +0x04 */ u16 port;
    /* +0x06 */ u16 pad_0x06;
} DWCiAddrKey; /* size: 0x08 */

/* The connection: one transport handle, the request table keyed by `{addr, port}`, the request list
 * the cleanup walk iterates, the two buffer sizes the per-request buffers are cut from, and the
 * state flags the teardown chain reads. */
struct DWCiConn {
    /* +0x00 */ int sock;
    /* +0x04 */ u32 addr;
    /* +0x08 */ u16 port;
    /* +0x0A */ u16 pad_0x0a;
    /* +0x0C */ void* table;  /* request table (DWCi_tableCreate(4, 32, 2, 0)) */
    /* +0x10 */ void* list;   /* request list (DWCi_listCreate(4, 4, DWCi_requestListRelease)) */
    /* +0x14 */ u32 field_0x14; /* set when the transport is being shut down */
    /* +0x18 */ u32 field_0x18; /* teardown-entered flag (DWCi_connectionShutdown) */
    /* +0x1C */ u32 field_0x1c; /* "shutdown while busy" (DWCi_destroyConnection / DWCi_sendTo) */
    /* +0x20 */ u32 field_0x20; /* DWCi_setConnectionUserValue's stored user value */
    /* +0x24 */ u32 field_0x24; /* DWCi_createConnection's fifth argument */
    /* +0x28 */ u32 field_0x28; /* "send buffered" flag DWCi_sendTo tests */
    /* +0x2C */ u8 pad_0x2c[0xC];
    /* +0x38 */ u32 size_0x38; /* per-request receive buffer size */
    /* +0x3C */ u32 size_0x3c; /* per-request send buffer size */
    /* +0x40 */ u32 mode;      /* 2/3 select the handshake shapes */
    /* +0x44 */ u32 field_0x44; /* header offset the send path adds */
    /* +0x48 */ u8 pad_0x48[0x4];
}; /* size: 0x4C */

/* A request: `{addr, port}` key, owner connection, protocol state, the two per-request buffers and
 * the four queues the transport layer walks. */
struct DWCiReq {
    /* +0x00 */ u32 addr;
    /* +0x04 */ u16 port;
    /* +0x06 */ u16 field_0x06;
    /* +0x08 */ DWCiConn* conn;
    /* +0x0C */ s32 field_0x0c; /* protocol state; 7 is the terminal one */
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u32 field_0x18;
    /* +0x1C */ u32 field_0x1c;
    /* +0x20 */ u32 field_0x20;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u32 field_0x28;
    /* +0x2C */ u32 field_0x2c;
    /* +0x30 */ u8 pad_0x30[0x14];
    /* +0x44 */ u8* buf_0x44;
    /* +0x48 */ u8 pad_0x48[0x8];
    /* +0x50 */ u8* buf_0x50;
    /* +0x54 */ u8 pad_0x54[0x8];
    /* +0x5C */ void* queue_0x5c;
    /* +0x60 */ void* queue_0x60;
    /* +0x64 */ u16 field_0x64;
    /* +0x66 */ u16 field_0x66;
    /* +0x68 */ u8 pad_0x68[0x20];
    /* +0x88 */ u32 field_0x88;
    /* +0x8C */ u32 field_0x8c;
    /* +0x90 */ u32 field_0x90;
    /* +0x94 */ u8 pad_0x94[0x4];
    /* +0x98 */ void* queue_0x98;
    /* +0x9C */ void* queue_0x9c;
}; /* size: 0xA0 */

/* One transfer record of the send path: the offset into the request's send buffer, the length, and
 * the timestamp the transport stamps into +0x0C. */
struct DWCiXfer {
    /* +0x00 */ u32 offset;
    /* +0x04 */ u32 length;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 stamp;
}; /* size: 0x10 */

/* --------------------------------------------------------------------------------------------- */
/* The band data this unit loads.  It is owned by nobody and the registered ranges bracketing it   */
/* name different modules, so it is declared in `include/unsplit/DWCi.h` (rule 2) and never        */
/* defined here (playbook 29).  The private records those declarations reach stay below, next to   */
/* the code that uses them.                                                                       */
/* --------------------------------------------------------------------------------------------- */

/* The character-class record DWCi_parseAddress validates the port digits against: a header whose +0x38
 * pointer reaches the per-character u16 flags, bit 3 marking a decimal digit.  Declared, never
 * defined - the `.data` run belongs to the split (playbook 29). */
typedef struct DWCiCharClass {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u16 bits[0x80];
} DWCiCharClass; /* size: 0x108 */

typedef struct DWCiCType {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ DWCiCharClass* cls;
    /* +0x3C */ u8 pad_0x3c[0xC];
} DWCiCType; /* size: 0x48 */


/* The resolver records DWCi_parseAddress falls back to: an entry whose +0x0C reaches the host's address
 * word through one more indirection. */
typedef struct DWCiHostAddr {
    /* +0x00 */ u32 addr;
} DWCiHostAddr; /* size: 0x04 */

typedef struct DWCiHostEntry {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ DWCiHostAddr** hosts;
} DWCiHostEntry; /* size: 0x10 */

/* The unit's own entry points, in address order. */
u32 DWCi_sendControlFrame(DWCiReq* req);
u32 DWCi_appendTransfer(DWCiReq* req, DWCiXfer* xfer);
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag);
u32 DWCi_requestTableHash(DWCiAddrKey** key, u32 bucket);
u32 DWCi_requestTableCompare(DWCiAddrKey** a, DWCiAddrKey** b);
void DWCi_requestListRelease(DWCiReq** slot);
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port);
void DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode);
void DWCi_destroyConnection(DWCiConn* conn);
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value);
void DWCi_removeRequest(DWCiReq* req);
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len);
u32 DWCi_requestTick(void** slot, u32* now);
u32 DWCi_connectionTick(DWCiConn* conn);
void DWCi_connectionFlushRequests(DWCiConn* conn);
void DWCi_connectionShutdown(DWCiConn* conn);
u16 DWCi_htons(u16 port);
char* DWCi_formatAddress(u32 addr, u16 port, char* buf);
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort);
void DWCi_GetStringLength(void** buf, int* len);

/* 0x805113B0 - the handshake write: { 0x0003 } when the connection is in mode 2, then the shared
 * 0xFEFE constant and the 0x68 terminator, handed to the send path. */
u32 DWCi_sendControlFrame(DWCiReq* req)
{
    DWCiConn* conn;
    u16 port;
    u32 addr;
    u8 buf[32];
    u16 word;
    int n;

    conn = req->conn;
    port = req->port;
    addr = req->addr;
    n = 0;
    if (conn->mode == 2) {
        word = 3;
        memcpy(buf, &word, 2);
        n = 2;
    }
    memcpy(buf + n, &DWCi_protocolMagic, 2);
    n += 2;
    buf[n] = 0x68;
    return DWCi_sendTo(conn, addr, port, buf, n + 1) != 0;
}

/* 0x80511470 - the send-buffer append: stamp the two header bytes at the connection's header offset,
 * hand the payload to the framer, then stamp the transfer's timestamp and, for a mode-2 request,
 * the "stamped" mark. */
u32 DWCi_appendTransfer(DWCiReq* req, DWCiXfer* xfer)
{
    DWCiConn* conn;
    u16 value;
    u8* buf;
    u32 off;

    conn = req->conn;
    value = req->field_0x66;
    buf = req->buf_0x50;
    off = xfer->offset + conn->field_0x44;
    buf[off + 5] = (u8)(value >> 8);
    buf[off + 6] = (u8)value;
    if (DWCi_requestFrame(req, req->buf_0x50 + xfer->offset, xfer->length) == 0) {
        return 0;
    }
    xfer->stamp = req->field_0x88;
    if (req->buf_0x50[xfer->offset + req->conn->field_0x44 + 2] == 2) {
        req->field_0x8c = req->field_0x88;
    }
    return 1;
}

/* 0x80511530 - the flush path: when the flag is clear, hand the request to DWCi_requestFlush; otherwise
 * look the block up by the size byte, and if it is not already complete, append it and re-frame the
 * last queue entry. */
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag)
{
    u32 out;
    u32 count;
    u32* item;
    u32 ret;

    if (flag != 0) {
        if (DWCi_requestBlockState(req, 0, b + 7, &out) == 0) {
            return 0;
        }
        if (out != 0) {
            return 1;
        }
        DWCi_bufferFrame(&req->buf_0x50, a, b);
        count = DWCi_listCount(req->queue_0x60);
        item = DWCi_listItem(req->queue_0x60, count - 1);
        ret = DWCi_requestFrame(req, req->buf_0x50 + item[0], item[1]);
        if (ret == 0) {
            return 0;
        }
        req->field_0x90 = 0;
        return 1;
    }
    return DWCi_requestFlush(req);
}

/* 0x80511620 - the request table's hash: (key.addr * key.port) modulo the bucket count. */
u32 DWCi_requestTableHash(DWCiAddrKey** key, u32 bucket)
{
    DWCiAddrKey* k = *key;

    return (k->addr * k->port) % bucket;
}

/* 0x80511640 - the request table's ordering: the address difference, or the signed port difference
 * when the addresses are equal. */
u32 DWCi_requestTableCompare(DWCiAddrKey** a, DWCiAddrKey** b)
{
    DWCiAddrKey* ka = *a;
    DWCiAddrKey* kb = *b;

    if (ka->addr != kb->addr) {
        return ka->addr - kb->addr;
    }
    return (u16)(ka->port - kb->port);
}

/* 0x80511680 - the request list's "release one slot" hook. */
void DWCi_requestListRelease(DWCiReq** slot)
{
    DWCi_requestFree(*slot);
}

/* 0x80511690 - look a request up by `{addr, port}`. */
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port)
{
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;

    key.addr = addr;
    key.port = port;
    pkey = &key;
    slot = DWCi_tableLookup(conn->table, &pkey);
    if (slot != 0) {
        return (u32)*slot;
    }
    return 0;
}

/* 0x805116E0 - build a connection around `host`: parse the address, allocate the 0x4C-byte record,
 * its request table and list, open the transport handle, then connect (or, in mode 3, only stamp
 * the address). */
void DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode)
{
    DWCiConn* conn;
    DWCiSockAddrIn sa;
    u32 addr;
    u16 port;
    int salen;

    DWCi_platformInit();
    if (bufSize2 == 0) {
        bufSize2 = 0x10000;
    }
    if (bufSize1 == 0) {
        bufSize1 = 0x10000;
    }
    if (DWCi_parseAddress(host, &addr, &port) == 0) {
        *out = (DWCiConn*)4;
        return;
    }
    conn = DWCi_malloc(0x4C);
    if (conn == 0) {
        *out = (DWCiConn*)1;
        return;
    }
    memset(conn, 0, 0x4C);
    conn->sock = -1;
    conn->size_0x3c = bufSize2;
    conn->size_0x38 = bufSize1;
    conn->field_0x24 = arg5;
    conn->table = DWCi_tableCreate(4, 32, 2, 0);
    if (conn->table == 0) {
        DWCi_free(conn);
        *out = (DWCiConn*)1;
        return;
    }
    conn->list = DWCi_listCreate(4, 4, DWCi_requestListRelease);
    if (conn->list == 0) {
        DWCi_tableDestroy(conn->table);
        DWCi_free(conn);
        *out = (DWCiConn*)1;
        return;
    }
    conn->sock = DWCi_socketCreate(2, 2, 17);
    conn->mode = mode;
    if (mode == 3) {
        conn->field_0x44 = 0;
    } else {
        conn->field_0x44 = mode;
    }
    if (conn->sock == -1) {
        DWCi_tableDestroy(conn->table);
        DWCi_listDestroy(conn->list);
        DWCi_free(conn);
        *out = (DWCiConn*)3;
        return;
    }
    memset(&sa, 0, 8);
    sa.family = 2;
    sa.addr = addr;
    sa.port = SOHtoNs(port);
    if (mode != 3) {
        if (DWCi_socketBind(conn->sock, &sa, 8) == -1) {
            DWCi_socketClose(conn->sock);
            DWCi_tableDestroy(conn->table);
            DWCi_listDestroy(conn->list);
            DWCi_free(conn);
            *out = (DWCiConn*)3;
            return;
        }
    }
    salen = 8;
    DWCi_socketGetLocalName(conn->sock, &sa, &salen);
    conn->addr = sa.addr;
    conn->port = SOAddressToHostPort(sa.port);
    *out = conn;
}

/* 0x80511920 - close the connection object itself: mark "shutdown while busy", or release the
 * transport handle, the table, the list and the record. */
void DWCi_destroyConnection(DWCiConn* conn)
{
    if (conn->field_0x1c != 0) {
        conn->field_0x14 = 1;
        return;
    }
    DWCi_socketClose(conn->sock);
    DWCi_tableDestroy(conn->table);
    DWCi_listDestroy(conn->list);
    DWCi_free(conn);
    DWCi_platformCleanup();
}

/* 0x80511990 - store the connection's user value. */
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value)
{
    conn->field_0x20 = value;
}

u32 DWCi_createRequest(DWCiConn* conn, DWCiReq** out, u32 addr, u16 port)
{
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;
    DWCiReq* req;
    DWCiReq* entry;
    DWCiReq** found;

    key.addr = addr;
    key.port = port;
    pkey = &key;
    found = DWCi_tableLookup(conn->table, &pkey);
    req = 0;
    if (found != 0) {
        req = *found;
    }
    if (req != 0) {
        return 5;
    }
    req = DWCi_malloc(0xA0);
    if (req != 0) {
        memset(req, 0, 0xA0);
        req->addr = addr;
        req->port = port;
        req->conn = conn;
        req->field_0x1c = DWCi_getTick();
        req->field_0x88 = req->field_0x1c;
        req->field_0x64 = 0;
        req->field_0x66 = 0;
        if (DWCi_bufferAlloc(&req->buf_0x44, conn->size_0x3c) != 0) {
            if (DWCi_bufferAlloc(&req->buf_0x50, conn->size_0x38) != 0) {
                req->queue_0x5c = DWCi_listCreate(16, 64, 0);
                if (req->queue_0x5c != 0) {
                    req->queue_0x60 = DWCi_listCreate(16, 64, 0);
                    if (req->queue_0x60 != 0) {
                        req->queue_0x98 = DWCi_listCreate(4, 2, 0);
                        if (req->queue_0x98 != 0) {
                            req->queue_0x9c = DWCi_listCreate(4, 2, 0);
                            if (req->queue_0x9c != 0) {
                                DWCi_tableInsert(conn->table, &req);
                                key.addr = addr;
                                key.port = port;
                                pkey = &key;
                                slot = DWCi_tableLookup(conn->table, &pkey);
                                entry = 0;
                                if (slot != 0) {
                                    entry = *slot;
                                }
                                *out = entry;
                                if (entry != 0) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
        DWCi_free(req->buf_0x44);
        DWCi_free(req->buf_0x50);
        if (req->queue_0x5c != 0) {
            DWCi_listDestroy(req->queue_0x5c);
        }
        if (req->queue_0x60 != 0) {
            DWCi_listDestroy(req->queue_0x60);
        }
        if (req->queue_0x98 != 0) {
            DWCi_listDestroy(req->queue_0x98);
        }
        if (req->queue_0x9c != 0) {
            DWCi_listDestroy(req->queue_0x9c);
        }
        DWCi_free(req);
    }
    return 1;
}

/* 0x80511C20 - drop a request from its owner: nothing to do while it is still flagged busy, else
 * remove it from the request list (state 7) or from the request table, by its own key. */
void DWCi_removeRequest(DWCiReq* req)
{
    DWCiReq* key;
    u32 count;
    u32 i;

    key = req;
    if (req->field_0x14 != 0) {
        return;
    }
    if (req->field_0x24 != 0) {
        return;
    }
    if (req->field_0x0c == 7) {
        count = DWCi_listCount(req->conn->list);
        for (i = 0; i < count; i++) {
            if (*(DWCiReq**)DWCi_listItem(req->conn->list, i) == req) {
                DWCi_listRemove(req->conn->list, i);
                return;
            }
        }
    } else {
        DWCi_tableRemove(req->conn->table, &key);
    }
}

/* 0x80511CF0 - the send path: default the buffer/length pair, refuse while the transport is not
 * usable, address the peer, hand the frame to the socket layer and dispatch the socket error to the
 * reconnect/retry entry points. */
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len)
{
    DWCiSockAddrIn sa;
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;
    DWCiReq* req;
    int err;

    DWCi_GetStringLength(&buf, &len);
    if (conn->mode != 3 && DWCi_socketIsUsable(conn->sock) == 0) {
        return 1;
    }
    memset(&sa, 0, 8);
    sa.family = 2;
    sa.addr = addr;
    sa.port = SOHtoNs(port);
    if (DWCi_socketSendTo(conn->sock, buf, len, 0, &sa, 8) == -1) {
        err = DWCi_socketGetLastError(conn->sock);
        if (err == -15) {
            if (DWCi_requestReconnect(conn, addr, port) != 0) {
                return 1;
            }
            return 0;
        }
        if (err == -23) {
            if (DWCi_requestRetry(conn, addr, port, 1) != 0) {
                return 1;
            }
            return 0;
        }
        if (err == -42 || err == -6) {
            return 1;
        }
        if (err == -35) {
            return 1;
        }
        if (conn->field_0x18 == 0) {
            conn->field_0x18 = 1;
            DWCi_socketShutdown(conn);
            if (DWCi_socketConnect(conn) != 0) {
                if (conn->field_0x1c != 0) {
                    conn->field_0x14 = 1;
                } else {
                    DWCi_socketClose(conn->sock);
                    DWCi_tableDestroy(conn->table);
                    DWCi_listDestroy(conn->list);
                    DWCi_free(conn);
                    DWCi_platformCleanup();
                }
            }
        }
        return 0;
    }
    if (conn->field_0x28 != 0) {
        key.addr = addr;
        key.port = port;
        pkey = &key;
        slot = DWCi_tableLookup(conn->table, &pkey);
        req = 0;
        if (slot != 0) {
            req = *slot;
        }
        if (DWCi_requestSend(conn, req, addr, port, 0, buf, len, 1) == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x80511F10 - the table walk's per-request step: skip a request that is neither terminal nor
 * timed out, then drop the terminal ones through the same remove path DWCi_removeRequest uses. */
u32 DWCi_requestTick(void** slot, u32* now)
{
    DWCiReq* req;
    DWCiReq* key;
    u32 count;
    u32 i;

    req = *slot;
    if (req->field_0x0c != 7 && DWCi_requestIsTimedOut(req, *now) == 0) {
        return 0;
    }
    if (req->field_0x0c == 7 && req->field_0x14 == 0 && req->field_0x24 == 0) {
        key = req;
        if (req->field_0x0c == 7 && req->field_0x14 == 0 && req->field_0x24 == 0) {
            if (req->field_0x0c == 7) {
                count = DWCi_listCount(req->conn->list);
                for (i = 0; i < count; i++) {
                    if (*(DWCiReq**)DWCi_listItem(req->conn->list, i) == req) {
                        DWCi_listRemove(req->conn->list, i);
                        return 1;
                    }
                }
            } else {
                DWCi_tableRemove(req->conn->table, &key);
            }
        }
    }
    return 1;
}

/* 0x80512030 - the periodic walk: hand the current tick to the table's per-request step. */
u32 DWCi_connectionTick(DWCiConn* conn)
{
    u32 now;

    now = DWCi_getTick();
    return DWCi_tableWalk(conn->table, DWCi_requestTick, &now) == 0;
}

/* 0x80512080 - the teardown walk: every request of the owner's list that is terminal is removed
 * from itself, back to front. */
void DWCi_connectionFlushRequests(DWCiConn* conn)
{
    int i;

    for (i = DWCi_listCount(conn->list) - 1; i >= 0; i--) {
        DWCiReq* req = *(DWCiReq**)DWCi_listItem(conn->list, i);
        DWCiReq* key;

        if (req->field_0x14 != 0) {
            continue;
        }
        if (req->field_0x24 != 0) {
            continue;
        }
        if (req->field_0x0c == 7) {
            u32 count = DWCi_listCount(req->conn->list);
            u32 j;

            for (j = 0; j < count; j++) {
                if (*(DWCiReq**)DWCi_listItem(req->conn->list, j) == req) {
                    DWCi_listRemove(req->conn->list, j);
                    break;
                }
            }
        } else {
            key = req;
            DWCi_tableRemove(req->conn->table, &key);
        }
    }
}

/* 0x80512170 - the single-shot teardown: run the transport's shutdown hooks once, then either mark
 * "shutdown while busy" or release the connection. */
void DWCi_connectionShutdown(DWCiConn* conn)
{
    if (conn->field_0x18 != 0) {
        return;
    }
    conn->field_0x18 = 1;
    DWCi_socketShutdown(conn);
    if (DWCi_socketConnect(conn) == 0) {
        return;
    }
    if (conn->field_0x1c != 0) {
        conn->field_0x14 = 1;
        return;
    }
    DWCi_socketClose(conn->sock);
    DWCi_tableDestroy(conn->table);
    DWCi_listDestroy(conn->list);
    DWCi_free(conn);
    DWCi_platformCleanup();
}

/* 0x80512200 - the host-to-network short wrapper the transport clients call. */
u16 DWCi_htons(u16 port)
{
    return SOAddressToHostPort(port);
}

/* 0x80512210 - format an address for the log/text layer: "addr:port", "addr", ":port" or the empty
 * string, into the caller's buffer or into the two-buffer ring when the caller passes none. */
char* DWCi_formatAddress(u32 addr, u16 port, char* buf)
{
    if (buf == 0) {
        DWCi_addressRingIndex ^= 1;
        buf = (char*)(DWCi_addressRing + DWCi_addressRingIndex * 0x16);
    }
    if (addr != 0) {
        u32 withPort;
        u32 withoutPort;

        if (port != 0) {
            withPort = addr;
            sprintf(buf, DWCi_addressFormat, SOAddressToString(&withPort), port);
        } else {
            withoutPort = addr;
            sprintf(buf, DWCi_addressFormatHost, SOAddressToString(&withoutPort));
        }
    } else {
        if (port != 0) {
            sprintf(buf, DWCi_addressFormatPort, port);
        } else {
            *buf = 0;
        }
    }
    return buf;
}

/* 0x80512300 - parse "host:port": split at the colon, validate the port digits against the band's
 * character-class table, convert with atoi, resolve the host name and hand both halves back. */
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort)
{
    char buf[0x108];
    char* colon;
    char* p;
    u32 addr;
    u16 port;
    u32 value;

    if (str == 0 || *str == 0) {
        addr = 0;
        port = 0;
    } else {
        colon = strchr(str, ':');
        if (colon == 0) {
            port = 0;
        } else {
            if (colon == str) {
                str = 0;
                addr = 0;
            } else {
                memcpy(buf, str, colon - str);
                buf[colon - str] = 0;
                str = buf;
            }
            p = colon + 1;
            while (*p != 0) {
                if ((u32)(s8)*p > 0xFFu ||
                    (DWCi_digitClassTable.cls->bits[(u8)*p] & 8) == 0) {
                    return 0;
                }
                p++;
            }
            value = atoi(colon + 1);
            if (value > 0xFFFF) {
                return 0;
            }
            port = (u16)value;
        }
        if (str != 0) {
            addr = DWCi_socketResolveAddress(str);
            if (addr == (u32)-1) {
                DWCiHostEntry* entry = DWCi_socketLookupHost(str);

                if (entry == 0) {
                    return 0;
                }
                addr = (*entry->hosts)->addr;
            }
        }
    }
    if (outAddr != 0) {
        *outAddr = addr;
    }
    if (outPort != 0) {
        *outPort = port;
    }
    return 1;
}

/* 0x80512490 moved to `DWCi/DWCi_NatNeg.c` with the seam (see the header): the body was
 * byte-identical here and is kept there verbatim, so this unit now ends at 0x80512490. */
