/*
 * DWCi/fn_805113B0.c - the 0x805113B0-0x80512490 band (20 functions, 4320 B) of the Nintendo
 * Wi-Fi Connection (DWCi) SDK block the game links between its own code and the NHTTP library.
 *
 * SEAM MOVED (recon lane, 2026-09-27).  This unit was registered as 0x805113B0..0x805124F4; the
 * right edge is now 0x80512490.  `tools/splits/tudiscover.py at 0x80512490` reports a STRONG x2
 * cut there (.sdata run jumps lbl_80794364 -> lbl_80794368 and lbl_80794368 -> lbl_80794370
 * intersect at the single cut 18642), so a TU begins at 0x80512490 and the 0x64 overlap with the
 * NHTTP-side probe is exactly `DWCi_GetStringLength`.  That function's definition moved to the new
 * `DWCi/DWCi_NatNeg.c` (0x80512490..0x805145B8); the forward declaration below stays because the
 * bodies here still call it.  Re-measured before/after: 91.1543 % / 21 functions / 472 B matched
 * (before) -> 90.9439 % / 20 functions / 372 B matched (after) - the 100 B loss is exactly the
 * 100 %-matched `DWCi_GetStringLength` leaving the unit, not a regression in any remaining body.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py lookup for every row of the range - all `zz_`/`fn_` placeholders - and
 * with a scan of the whole DOL's string pool: the only bare source-file names in the SDK band's
 * pools are WUD.c/vi.c/TPL.c/OS.c/dvd.c/dvdfs.c/arc.c (0x80793d08-0x807941d8), dwc_auth_interface.c
 * (0x806302d4), NHTTP_bgnend.c/NHTTP_os_RVL.c/d_nhttp.c/ncdsystem.c (0x80630a28-0x80631088) and
 * none of them is referenced by any address this range calls or loads), so the file keeps the map's
 * stem.
 *
 * Seam, module and language:
 *   - the range is a `fn_XXXXXXXX` run discovery proposed (`--max-bytes` cut; no `__FILE__` string
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
 * (`lbl_80794348`'s 2-byte protocol constant, the `%s:%d`/`%s`/`:%d` format strings at
 * 0x80794358..0x80794368, and `.data` 0x8060EDB0 + the `.bss` counter 0x80795820 / ring buffers
 * 0x807625C0); none of them is claimed here - a data claim has to be measured before and after
 * (playbook 55).
 *
 * Registered NonMatching in configure.py, lib DWCi (Wii/1.3).  All 20 bodies are reconstructed and every
 * one of them is at or above the 80 % bar (unit fuzzy 90.94 % over the 4320 B of .text, 7 byte-identical,
 * 372 B matched; `DWCi_GetStringLength` is 100 % in its new home `DWCi/DWCi_NatNeg.c`):
 *
 *   100.00  fn_80511620  fn_80511680  fn_80511920  fn_80511990  fn_80512030  fn_80512170
 *           fn_80512200
 *    95.38  fn_80511640        94.92  fn_80511530        94.56  fn_80512210
 *    91.87  fn_80511CF0        91.76  fn_80511C20        91.32  fn_80512080
 *    90.74  fn_805113B0        89.96  fn_80511470        88.59  fn_80511F10
 *    87.83  fn_805116E0        87.10  fn_80512300        86.79  fn_805119A0
 *    99.79  fn_80511690
 *
 * Residuals (all measured with `recompile.py DWCi/fn_805113B0.c --measure <symbol>`; none is a seam or a
 * type error - they are codegen shapes):
 *   - fn_80512300 (87.10): retail's parser uses goto-shaped early exits - the empty-string case and the
 *     colon-at-start case jump straight to the store and to the port parse.  Rule 8 forbids `goto`, so the
 *     conformant nested-if shape is used; it measures 87.10 (368 B) against the target's 400 B, while the
 *     earlier flat `if (str != 0 && *str != 0)` shape measured 82.30 (352 B).  Both numbers are here
 *     because the rule-8 shape is not the byte-identical one.
 *   - fn_805119A0 (86.79) / fn_80511690 (99.79): the frames are smaller than retail's (0x30 vs 0x170 and
 *     0x20 vs 0xB0).  The original TU keeps a large scratch block in these two functions that no field of
 *     the recovered types accounts for; instruction counts are equal, so only the prologue/epilogue pairs
 *     differ.  fn_805119A0 also builds the same key twice, as retail does.
 *   - fn_805116E0 (87.83) / fn_80511CF0 (91.87) / fn_80511C20 (91.76) / fn_80512080 (91.32) /
 *     fn_80511F10 (88.59): register colouring and block order; instruction counts equal or within one.
 *   - fn_80511470 (89.96): the two header bytes go out as `extrwi`+`stbx` in retail and `srawi`+`stb`
 *     here (same bytes, different materialisation).
 *   - fn_805113B0 (90.74): the conn/port/addr load order and one `or`-based bool tail.
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

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

typedef struct DWCiConn DWCiConn; /* size: 0x4C */
typedef struct DWCiReq DWCiReq;   /* size: 0xA0 */
typedef struct DWCiXfer DWCiXfer; /* one transfer record handed to fn_80511470 */

/* The address record the transport hands to the socket wrappers: one length byte, one family byte,
 * the network-order port and the network-order address (the same 8 bytes `bind`/`connect` take). */
typedef struct DWCiSockAddrIn {
    /* +0x00 */ u8 len;
    /* +0x01 */ u8 family;
    /* +0x02 */ u16 port;
    /* +0x04 */ u32 addr;
} DWCiSockAddrIn; /* size: 0x08 */

/* The `{ addr, port }` key the connection's request table hashes and compares on.  The table API
 * takes the key *indirectly* (a `DWCiAddrKey**`): fn_80511620/fn_80511640 dereference their first
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
    /* +0x0C */ void* table;  /* request table (fn_8050B460(4, 32, 2, 0)) */
    /* +0x10 */ void* list;   /* request list (fn_8050AAF0(4, 4, fn_80511680)) */
    /* +0x14 */ u32 field_0x14; /* set when the transport is being shut down */
    /* +0x18 */ u32 field_0x18; /* teardown-entered flag (fn_80512170) */
    /* +0x1C */ u32 field_0x1c; /* "shutdown while busy" (fn_80511920 / fn_80511CF0) */
    /* +0x20 */ u32 field_0x20; /* fn_80511990's stored user value */
    /* +0x24 */ u32 field_0x24; /* fn_805116E0's fifth argument */
    /* +0x28 */ u32 field_0x28; /* "send buffered" flag fn_80511CF0 tests */
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
/* The unregistered helpers and data this unit calls or loads.  Their addresses are outside this    */
/* unit's range and owned by nobody, and the registered bands bracketing them name different        */
/* modules (RSO below, homebutton above), so no `<module>.h` is sound - stylelint's rule 2 named    */
/* gap; they are declared here, never defined (playbook 29).                                       */
/* --------------------------------------------------------------------------------------------- */

extern u16 lbl_80794348; /* .sdata - the 2-byte 0xFEFE protocol constant */
extern char lbl_80794358[6]; /* .sdata - "%s:%d" */
extern char lbl_80794360[3]; /* .sdata - "%s" */
extern char lbl_80794364[4]; /* .sdata - ":%d" */
extern u8 lbl_80794368[8]; /* .sdata - the empty string */
/* The character-class record fn_80512300 validates the port digits against: a header whose +0x38
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

extern DWCiCType lbl_8060EDB0; /* .data - fn_80512300's digit-class table owner */
extern u32 lbl_80795820;  /* .sbss - the two-buffer ring's index (fn_80512210) */
extern u8 lbl_807625C0[]; /* .bss - the two 0x16-byte address-string buffers (fn_80512210) */

extern void* fn_8050C5A0(u32 size);
extern void fn_8050C5D0(void* p);
extern u32 fn_8050C4F0(void);
extern void fn_8050C4E0(void);
extern void fn_8050C4D0(void);
extern int fn_8050C220(int sock);
extern int fn_8050CD20(u8** dst, u32 size);
extern int fn_8050CDE0(u8** dst, u32 a, u32 b);
extern int fn_8050CEE0(DWCiConn* conn);
extern void* fn_8050B460(u32 a, u32 b, u32 c, u32 d);
extern void fn_8050B510(void* table);
extern void* fn_8050AAF0(u32 a, u32 b, void* cb);
extern void fn_8050AB90(void* list);
extern u32 fn_8050AC30(void* list);
extern void* fn_8050AC40(void* list, u32 index);
extern void fn_8050AFD0(void* list, u32 index);
extern void fn_8050B590(void* table, DWCiReq** entry);
extern void* fn_8050B6F0(void* table, DWCiAddrKey** key);
extern void fn_8050B640(void* table, void* key);
extern u32 fn_8050B800(void* table, void* callback, void* arg);
extern int fn_8050B880(u32 a, u32 b, u32 c);
extern void fn_8050B8C0(int sock);
extern int fn_8050B900(int sock, void* buf, int len);
extern int fn_8050B9E0(int sock, void* buf, int len, u32 flags, void* sa, int salen);
extern int fn_8050BA60(int sock, void* sa, int* salen);
extern int fn_8050BB00(int sock);
/* The resolver records fn_80512300 falls back to: an entry whose +0x0C reaches the host's address
 * word through one more indirection. */
typedef struct DWCiHostAddr {
    /* +0x00 */ u32 addr;
} DWCiHostAddr; /* size: 0x04 */

typedef struct DWCiHostEntry {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ DWCiHostAddr** hosts;
} DWCiHostEntry; /* size: 0x10 */

extern u32 fn_8050BAC0(char* host);
extern DWCiHostEntry* fn_8050BC40(char* host);
extern int fn_8050D5D0(DWCiConn* conn, DWCiReq* req, u32 addr, u16 port, u32 a, void* buf, int len,
                       u32 flag);
extern int fn_8050DA60(DWCiReq* req, void* p, u32 len);
extern int fn_8050DBC0(DWCiReq* req, u32 now);
extern void fn_8050DE10(void* p);
extern void fn_8050E270(DWCiConn* conn);
extern int fn_80510160(DWCiConn* conn, u32 addr, u16 port);
extern int fn_80510380(DWCiConn* conn, u32 addr, u16 port, u32 flag);
extern int fn_805108B0(DWCiReq* req, u32 a, u32 b, u32* out);
extern int fn_805110B0(DWCiReq* req);
extern char* fn_80520604(u32* addr);
extern u16 fn_8052065C(u16 port);
extern u16 SOHtoNs(u16 port);

/* The C runtime the SDK links (declared, like the rest of the band, because `-nosyspath` leaves the
 * prototypes to the unit). */
extern int sprintf(char* dst, const char* fmt, ...);
extern char* strchr(const char* s, int c);
extern u32 strlen(const char* s);
extern int atoi(const char* s);

/* The unit's own entry points, in address order. */
u32 fn_805113B0(DWCiReq* req);
u32 fn_80511470(DWCiReq* req, DWCiXfer* xfer);
u32 fn_80511530(DWCiReq* req, u32 a, u32 b, u32 flag);
u32 fn_80511620(DWCiAddrKey** key, u32 bucket);
u32 fn_80511640(DWCiAddrKey** a, DWCiAddrKey** b);
void fn_80511680(DWCiReq** slot);
u32 fn_80511690(DWCiConn* conn, u32 addr, u16 port);
void fn_805116E0(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode);
void fn_80511920(DWCiConn* conn);
void fn_80511990(DWCiConn* conn, u32 value);
void fn_80511C20(DWCiReq* req);
u32 fn_80511CF0(DWCiConn* conn, u32 addr, u16 port, void* buf, int len);
u32 fn_80511F10(void** slot, u32* now);
u32 fn_80512030(DWCiConn* conn);
void fn_80512080(DWCiConn* conn);
void fn_80512170(DWCiConn* conn);
u16 fn_80512200(u16 port);
char* fn_80512210(u32 addr, u16 port, char* buf);
u32 fn_80512300(char* str, u32* outAddr, u16* outPort);
void DWCi_GetStringLength(void** buf, int* len);

/* 0x805113B0 - the handshake write: { 0x0003 } when the connection is in mode 2, then the shared
 * 0xFEFE constant and the 0x68 terminator, handed to the send path. */
u32 fn_805113B0(DWCiReq* req)
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
    memcpy(buf + n, &lbl_80794348, 2);
    n += 2;
    buf[n] = 0x68;
    return fn_80511CF0(conn, addr, port, buf, n + 1) != 0;
}

/* 0x80511470 - the send-buffer append: stamp the two header bytes at the connection's header offset,
 * hand the payload to the framer, then stamp the transfer's timestamp and, for a mode-2 request,
 * the "stamped" mark. */
u32 fn_80511470(DWCiReq* req, DWCiXfer* xfer)
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
    if (fn_8050DA60(req, req->buf_0x50 + xfer->offset, xfer->length) == 0) {
        return 0;
    }
    xfer->stamp = req->field_0x88;
    if (req->buf_0x50[xfer->offset + req->conn->field_0x44 + 2] == 2) {
        req->field_0x8c = req->field_0x88;
    }
    return 1;
}

/* 0x80511530 - the flush path: when the flag is clear, hand the request to fn_805110B0; otherwise
 * look the block up by the size byte, and if it is not already complete, append it and re-frame the
 * last queue entry. */
u32 fn_80511530(DWCiReq* req, u32 a, u32 b, u32 flag)
{
    u32 out;
    u32 count;
    u32* item;
    u32 ret;

    if (flag != 0) {
        if (fn_805108B0(req, 0, b + 7, &out) == 0) {
            return 0;
        }
        if (out != 0) {
            return 1;
        }
        fn_8050CDE0(&req->buf_0x50, a, b);
        count = fn_8050AC30(req->queue_0x60);
        item = fn_8050AC40(req->queue_0x60, count - 1);
        ret = fn_8050DA60(req, req->buf_0x50 + item[0], item[1]);
        if (ret == 0) {
            return 0;
        }
        req->field_0x90 = 0;
        return 1;
    }
    return fn_805110B0(req);
}

/* 0x80511620 - the request table's hash: (key.addr * key.port) modulo the bucket count. */
u32 fn_80511620(DWCiAddrKey** key, u32 bucket)
{
    DWCiAddrKey* k = *key;

    return (k->addr * k->port) % bucket;
}

/* 0x80511640 - the request table's ordering: the address difference, or the signed port difference
 * when the addresses are equal. */
u32 fn_80511640(DWCiAddrKey** a, DWCiAddrKey** b)
{
    DWCiAddrKey* ka = *a;
    DWCiAddrKey* kb = *b;

    if (ka->addr != kb->addr) {
        return ka->addr - kb->addr;
    }
    return (u16)(ka->port - kb->port);
}

/* 0x80511680 - the request list's "release one slot" hook. */
void fn_80511680(DWCiReq** slot)
{
    fn_8050DE10(*slot);
}

/* 0x80511690 - look a request up by `{addr, port}`. */
u32 fn_80511690(DWCiConn* conn, u32 addr, u16 port)
{
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;

    key.addr = addr;
    key.port = port;
    pkey = &key;
    slot = fn_8050B6F0(conn->table, &pkey);
    if (slot != 0) {
        return (u32)*slot;
    }
    return 0;
}

/* 0x805116E0 - build a connection around `host`: parse the address, allocate the 0x4C-byte record,
 * its request table and list, open the transport handle, then connect (or, in mode 3, only stamp
 * the address). */
void fn_805116E0(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode)
{
    DWCiConn* conn;
    DWCiSockAddrIn sa;
    u32 addr;
    u16 port;
    int salen;

    fn_8050C4D0();
    if (bufSize2 == 0) {
        bufSize2 = 0x10000;
    }
    if (bufSize1 == 0) {
        bufSize1 = 0x10000;
    }
    if (fn_80512300(host, &addr, &port) == 0) {
        *out = (DWCiConn*)4;
        return;
    }
    conn = fn_8050C5A0(0x4C);
    if (conn == 0) {
        *out = (DWCiConn*)1;
        return;
    }
    memset(conn, 0, 0x4C);
    conn->sock = -1;
    conn->size_0x3c = bufSize2;
    conn->size_0x38 = bufSize1;
    conn->field_0x24 = arg5;
    conn->table = fn_8050B460(4, 32, 2, 0);
    if (conn->table == 0) {
        fn_8050C5D0(conn);
        *out = (DWCiConn*)1;
        return;
    }
    conn->list = fn_8050AAF0(4, 4, fn_80511680);
    if (conn->list == 0) {
        fn_8050B510(conn->table);
        fn_8050C5D0(conn);
        *out = (DWCiConn*)1;
        return;
    }
    conn->sock = fn_8050B880(2, 2, 17);
    conn->mode = mode;
    if (mode == 3) {
        conn->field_0x44 = 0;
    } else {
        conn->field_0x44 = mode;
    }
    if (conn->sock == -1) {
        fn_8050B510(conn->table);
        fn_8050AB90(conn->list);
        fn_8050C5D0(conn);
        *out = (DWCiConn*)3;
        return;
    }
    memset(&sa, 0, 8);
    sa.family = 2;
    sa.addr = addr;
    sa.port = SOHtoNs(port);
    if (mode != 3) {
        if (fn_8050B900(conn->sock, &sa, 8) == -1) {
            fn_8050B8C0(conn->sock);
            fn_8050B510(conn->table);
            fn_8050AB90(conn->list);
            fn_8050C5D0(conn);
            *out = (DWCiConn*)3;
            return;
        }
    }
    salen = 8;
    fn_8050BA60(conn->sock, &sa, &salen);
    conn->addr = sa.addr;
    conn->port = fn_8052065C(sa.port);
    *out = conn;
}

/* 0x80511920 - close the connection object itself: mark "shutdown while busy", or release the
 * transport handle, the table, the list and the record. */
void fn_80511920(DWCiConn* conn)
{
    if (conn->field_0x1c != 0) {
        conn->field_0x14 = 1;
        return;
    }
    fn_8050B8C0(conn->sock);
    fn_8050B510(conn->table);
    fn_8050AB90(conn->list);
    fn_8050C5D0(conn);
    fn_8050C4E0();
}

/* 0x80511990 - store the connection's user value. */
void fn_80511990(DWCiConn* conn, u32 value)
{
    conn->field_0x20 = value;
}

u32 fn_805119A0(DWCiConn* conn, DWCiReq** out, u32 addr, u16 port)
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
    found = fn_8050B6F0(conn->table, &pkey);
    req = 0;
    if (found != 0) {
        req = *found;
    }
    if (req != 0) {
        return 5;
    }
    req = fn_8050C5A0(0xA0);
    if (req != 0) {
        memset(req, 0, 0xA0);
        req->addr = addr;
        req->port = port;
        req->conn = conn;
        req->field_0x1c = fn_8050C4F0();
        req->field_0x88 = req->field_0x1c;
        req->field_0x64 = 0;
        req->field_0x66 = 0;
        if (fn_8050CD20(&req->buf_0x44, conn->size_0x3c) != 0) {
            if (fn_8050CD20(&req->buf_0x50, conn->size_0x38) != 0) {
                req->queue_0x5c = fn_8050AAF0(16, 64, 0);
                if (req->queue_0x5c != 0) {
                    req->queue_0x60 = fn_8050AAF0(16, 64, 0);
                    if (req->queue_0x60 != 0) {
                        req->queue_0x98 = fn_8050AAF0(4, 2, 0);
                        if (req->queue_0x98 != 0) {
                            req->queue_0x9c = fn_8050AAF0(4, 2, 0);
                            if (req->queue_0x9c != 0) {
                                fn_8050B590(conn->table, &req);
                                key.addr = addr;
                                key.port = port;
                                pkey = &key;
                                slot = fn_8050B6F0(conn->table, &pkey);
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
        fn_8050C5D0(req->buf_0x44);
        fn_8050C5D0(req->buf_0x50);
        if (req->queue_0x5c != 0) {
            fn_8050AB90(req->queue_0x5c);
        }
        if (req->queue_0x60 != 0) {
            fn_8050AB90(req->queue_0x60);
        }
        if (req->queue_0x98 != 0) {
            fn_8050AB90(req->queue_0x98);
        }
        if (req->queue_0x9c != 0) {
            fn_8050AB90(req->queue_0x9c);
        }
        fn_8050C5D0(req);
    }
    return 1;
}

/* 0x80511C20 - drop a request from its owner: nothing to do while it is still flagged busy, else
 * remove it from the request list (state 7) or from the request table, by its own key. */
void fn_80511C20(DWCiReq* req)
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
        count = fn_8050AC30(req->conn->list);
        for (i = 0; i < count; i++) {
            if (*(DWCiReq**)fn_8050AC40(req->conn->list, i) == req) {
                fn_8050AFD0(req->conn->list, i);
                return;
            }
        }
    } else {
        fn_8050B640(req->conn->table, &key);
    }
}

/* 0x80511CF0 - the send path: default the buffer/length pair, refuse while the transport is not
 * usable, address the peer, hand the frame to the socket layer and dispatch the socket error to the
 * reconnect/retry entry points. */
u32 fn_80511CF0(DWCiConn* conn, u32 addr, u16 port, void* buf, int len)
{
    DWCiSockAddrIn sa;
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;
    DWCiReq* req;
    int err;

    DWCi_GetStringLength(&buf, &len);
    if (conn->mode != 3 && fn_8050C220(conn->sock) == 0) {
        return 1;
    }
    memset(&sa, 0, 8);
    sa.family = 2;
    sa.addr = addr;
    sa.port = SOHtoNs(port);
    if (fn_8050B9E0(conn->sock, buf, len, 0, &sa, 8) == -1) {
        err = fn_8050BB00(conn->sock);
        if (err == -15) {
            if (fn_80510160(conn, addr, port) != 0) {
                return 1;
            }
            return 0;
        }
        if (err == -23) {
            if (fn_80510380(conn, addr, port, 1) != 0) {
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
            fn_8050E270(conn);
            if (fn_8050CEE0(conn) != 0) {
                if (conn->field_0x1c != 0) {
                    conn->field_0x14 = 1;
                } else {
                    fn_8050B8C0(conn->sock);
                    fn_8050B510(conn->table);
                    fn_8050AB90(conn->list);
                    fn_8050C5D0(conn);
                    fn_8050C4E0();
                }
            }
        }
        return 0;
    }
    if (conn->field_0x28 != 0) {
        key.addr = addr;
        key.port = port;
        pkey = &key;
        slot = fn_8050B6F0(conn->table, &pkey);
        req = 0;
        if (slot != 0) {
            req = *slot;
        }
        if (fn_8050D5D0(conn, req, addr, port, 0, buf, len, 1) == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x80511F10 - the table walk's per-request step: skip a request that is neither terminal nor
 * timed out, then drop the terminal ones through the same remove path fn_80511C20 uses. */
u32 fn_80511F10(void** slot, u32* now)
{
    DWCiReq* req;
    DWCiReq* key;
    u32 count;
    u32 i;

    req = *slot;
    if (req->field_0x0c != 7 && fn_8050DBC0(req, *now) == 0) {
        return 0;
    }
    if (req->field_0x0c == 7 && req->field_0x14 == 0 && req->field_0x24 == 0) {
        key = req;
        if (req->field_0x0c == 7 && req->field_0x14 == 0 && req->field_0x24 == 0) {
            if (req->field_0x0c == 7) {
                count = fn_8050AC30(req->conn->list);
                for (i = 0; i < count; i++) {
                    if (*(DWCiReq**)fn_8050AC40(req->conn->list, i) == req) {
                        fn_8050AFD0(req->conn->list, i);
                        return 1;
                    }
                }
            } else {
                fn_8050B640(req->conn->table, &key);
            }
        }
    }
    return 1;
}

/* 0x80512030 - the periodic walk: hand the current tick to the table's per-request step. */
u32 fn_80512030(DWCiConn* conn)
{
    u32 now;

    now = fn_8050C4F0();
    return fn_8050B800(conn->table, fn_80511F10, &now) == 0;
}

/* 0x80512080 - the teardown walk: every request of the owner's list that is terminal is removed
 * from itself, back to front. */
void fn_80512080(DWCiConn* conn)
{
    int i;

    for (i = fn_8050AC30(conn->list) - 1; i >= 0; i--) {
        DWCiReq* req = *(DWCiReq**)fn_8050AC40(conn->list, i);
        DWCiReq* key;

        if (req->field_0x14 != 0) {
            continue;
        }
        if (req->field_0x24 != 0) {
            continue;
        }
        if (req->field_0x0c == 7) {
            u32 count = fn_8050AC30(req->conn->list);
            u32 j;

            for (j = 0; j < count; j++) {
                if (*(DWCiReq**)fn_8050AC40(req->conn->list, j) == req) {
                    fn_8050AFD0(req->conn->list, j);
                    break;
                }
            }
        } else {
            key = req;
            fn_8050B640(req->conn->table, &key);
        }
    }
}

/* 0x80512170 - the single-shot teardown: run the transport's shutdown hooks once, then either mark
 * "shutdown while busy" or release the connection. */
void fn_80512170(DWCiConn* conn)
{
    if (conn->field_0x18 != 0) {
        return;
    }
    conn->field_0x18 = 1;
    fn_8050E270(conn);
    if (fn_8050CEE0(conn) == 0) {
        return;
    }
    if (conn->field_0x1c != 0) {
        conn->field_0x14 = 1;
        return;
    }
    fn_8050B8C0(conn->sock);
    fn_8050B510(conn->table);
    fn_8050AB90(conn->list);
    fn_8050C5D0(conn);
    fn_8050C4E0();
}

/* 0x80512200 - the host-to-network short wrapper the transport clients call. */
u16 fn_80512200(u16 port)
{
    return fn_8052065C(port);
}

/* 0x80512210 - format an address for the log/text layer: "addr:port", "addr", ":port" or the empty
 * string, into the caller's buffer or into the two-buffer ring when the caller passes none. */
char* fn_80512210(u32 addr, u16 port, char* buf)
{
    if (buf == 0) {
        lbl_80795820 ^= 1;
        buf = (char*)(lbl_807625C0 + lbl_80795820 * 0x16);
    }
    if (addr != 0) {
        u32 withPort;
        u32 withoutPort;

        if (port != 0) {
            withPort = addr;
            sprintf(buf, lbl_80794358, fn_80520604(&withPort), port);
        } else {
            withoutPort = addr;
            sprintf(buf, lbl_80794360, fn_80520604(&withoutPort));
        }
    } else {
        if (port != 0) {
            sprintf(buf, lbl_80794364, port);
        } else {
            *buf = 0;
        }
    }
    return buf;
}

/* 0x80512300 - parse "host:port": split at the colon, validate the port digits against the band's
 * character-class table, convert with atoi, resolve the host name and hand both halves back. */
u32 fn_80512300(char* str, u32* outAddr, u16* outPort)
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
                    (lbl_8060EDB0.cls->bits[(u8)*p] & 8) == 0) {
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
            addr = fn_8050BAC0(str);
            if (addr == (u32)-1) {
                DWCiHostEntry* entry = fn_8050BC40(str);

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
