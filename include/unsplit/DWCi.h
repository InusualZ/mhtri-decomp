/*
 * DWCi-band declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The Nintendo Wi-Fi Connection (DWCi) band's transport/GameSpy half lives in
 * 0x80507C40..0x805145B8 (the registered units `DWCi/DWCi_Np_CPUCopyFast.c`, `DWCi/fn_805113B0.c`
 * and `DWCi/DWCi_NatNeg.c`), but the helpers those units call - the 0x8050A..-0x8050E.. socket /
 * list / address layer, `DWCi_freeNode` and the `.data` digit-class table - sit in bands no
 * registered unit covers.  stylelint's rule 2 resolves the 0x8050xxxx functions here (the nearest
 * registered ranges below and above both name `DWCi`), and the remaining `.sdata`/`.bss`/`.sbss`
 * objects come from the same band; none has an owner's header to move to, so this is their home.
 * Declared, never defined (playbook 29).
 *
 * The band's *owned* data is not here: the data pass of 2026-09-28 claimed the runs the two units
 * store into, so those declarations moved to their owners' headers and this header includes them -
 * `include/DWCi/DWCi_NatNeg.h` (`.sdata` 0x80794368..0x807943A0, `.sbss` 0x80795828..0x80795878,
 * `.bss` 0x807614D8..0x80762A20) and `include/DWCi/DWCi_Np_CPUCopyFast.h` (`.sdata`
 * 0x80794200..0x80794210, `.sbss` 0x807957D0..0x807957F8).  Rule 2 put them there, not here.
 *
 * The types the declarations reach are the units' own views, so only forward declarations are here
 * (`struct DWCiConn` / `struct DWCiReq` / `struct DWCiAddrKey` / `struct DWCiCType` are completed by
 * `src/DWCi/fn_805113B0.c`).
 *
 * Added with the networking conformance pass: `src/DWCi/fn_805113B0.c` declared 52 of these
 * locally, `DWCi_Np_CPUCopyFast.c` six and `DWCi_NatNeg.c` one.
 */
#ifndef MHTRI_UNSPLIT_DWCI_H
#define MHTRI_UNSPLIT_DWCI_H

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "DWCi/DWCi_Np_CPUCopyFast.h"    /* the Np unit's own data (rule 2) */

struct DWCiConn;
struct DWCiReq;
struct DWCiAddrKey;
struct DWCiCType;

/* The resolver records `DWCi_parseAddress` and the NATNEG unit read: an entry whose +0x0C reaches the
 * host's address words through one more indirection (the SDK's `hostent`: name, aliases, type, length,
 * then the address list). */
typedef struct DWCiHostAddr {
    /* +0x00 */ u32 addr;
} DWCiHostAddr; /* size: 0x04 */

typedef struct DWCiHostEntry {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ DWCiHostAddr** hosts;
} DWCiHostEntry; /* size: 0x10 */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the band's unowned data (`.sdata`, `.bss`, `.data`) -------------------------------------- */

/* 0x80794348 - the 2-byte 0xFEFE protocol constant; 0x80794358/0x80794360/0x80794364 are the
 * "%s:%d", "%s" and ":%d" format strings the address helpers sprintf through. */
extern u16 DWCi_protocolMagic;
extern char DWCi_addressFormat[6];
extern char DWCi_addressFormatHost[3];
extern char DWCi_addressFormatPort[4];

/* 0x8060EDB0 - the character-class record `DWCi_parseAddress` validates port digits against (a header
 * whose +0x38 pointer reaches the per-character u16 flags, bit 3 marking a decimal digit). */
extern struct DWCiCType DWCi_digitClassTable;

/* 0x80795818 - the result of the NAT-probe check `fn_8050C770` runs (1 = failed, 2 / 3 = the two
 * usable answers); the negotiator refuses to start while it reads 1.  Name and meaning are GUESSes
 * from that writer (it sends a probe, waits for its echo and stores 1, 2 or 3). */
extern s32 DWCi_natProbeStatus;

/* 0x80795820 - the index of the two-buffer address-string ring `DWCi_formatAddress` writes into the
 * 0x807625C0 buffers (`DWCi_addressRing`, declared by the NATNEG unit's header - the word falls
 * inside its `.bss` run). */
extern u32 DWCi_addressRingIndex;

/* 0x80760F00 - the DWCi state block (0x1D0 B, first word a status) and 0x807610D0 the 0x190-byte
 * work buffer. */
extern u32 DWCi_stateBlock[];
extern u8 DWCi_workBuffer[];

/* ---- the transport / socket helpers --------------------------------------------------------- */

void* DWCi_listCreate(u32 a, u32 b, void* cb);
void DWCi_listDestroy(void* list);
u32 DWCi_listCount(void* list);
void* DWCi_listItem(void* list, u32 index);
void DWCi_listRemove(void* list, u32 index);
void* DWCi_tableCreate(u32 a, u32 b, u32 c, u32 d);
void DWCi_tableDestroy(void* table);
void DWCi_tableInsert(void* table, struct DWCiReq** entry);
void* DWCi_tableLookup(void* table, struct DWCiAddrKey** key);
void DWCi_tableRemove(void* table, void* key);
u32 DWCi_tableWalk(void* table, void* callback, void* arg);
int DWCi_socketCreate(u32 a, u32 b, u32 c);
void DWCi_socketClose(int sock);
int DWCi_socketBind(int sock, void* buf, int len);
int DWCi_socketSendTo(int sock, void* buf, int len, u32 flags, void* sa, int salen);
int DWCi_socketGetLocalName(int sock, void* sa, int* salen);
/* 0x8050AC70 - append a copy of the fixed-size record at `item` to the list. */
/* untyped: opaque handle (the list) and byte range (the record copied in) */
void DWCi_listAppend(void* list, void* item);
/* 0x8050AF20 - delete the list's `index`th record. */
/* untyped: opaque handle (the list) */
void DWCi_listDeleteAt(void* list, u32 index);
u32 DWCi_socketResolveAddress(char* host);
int DWCi_socketGetLastError(int sock);
struct DWCiHostEntry* DWCi_socketLookupHost(char* host);
/* 0x8050B980 - receive one datagram: (socket, buffer, capacity, flags, sender address, its length in/out);
 * -1 on failure. */
int DWCi_socketRecvFrom(int sock, u8* buf, int len, u32 flags, DWCiSockAddrIn* sa, int* salen);
/* 0x8050C1D0 - non-zero while the socket has a datagram waiting. */
int DWCi_socketHasData(int sock);
/* 0x8050C270 - the resolver record of this machine's own host (NULL when it cannot be read). */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void);
/* 0x8050C450 - non-zero when the address the pointer names is a usable (public) one. */
int DWCi_hostAddressIsUsable(DWCiHostAddr* host);
int DWCi_socketIsUsable(int sock);
void DWCi_platformInit(void);
void DWCi_platformCleanup(void);
u32 DWCi_getTick(void);
void* DWCi_malloc(u32 size);
void DWCi_free(void* p);
/* 0x80520658 / 0x80520664 - the peer-id word at `msg+8` and the negotiator socket token a record's
 * +0x08 holds.  Retail's body for both is a bare `blr`: the argument comes back unchanged, so this
 * build has them as two 4-byte SO-band entries that do no conversion, and their names are taken from
 * how their call sites use the result (`DWCi_socketTokenFromPeer(token)`, the peer word read out of
 * a received `msg+8`, compared against a record's token; `DWCi_peerTokenFromSocket(rec->token)`,
 * written into `msg+8` of an outgoing one). */
s32 DWCi_socketTokenFromPeer(u32 peerId);
s32 DWCi_peerTokenFromSocket(s32 sock);
int DWCi_bufferAlloc(u8** dst, u32 size);
int DWCi_bufferFrame(u8** dst, u32 a, u32 b);
int DWCi_socketConnect(struct DWCiConn* conn);
int DWCi_requestSend(struct DWCiConn* conn, struct DWCiReq* req, u32 addr, u16 port, u32 a, void* buf,
                int len, u32 flag);
int DWCi_requestFrame(struct DWCiReq* req, void* p, u32 len);
int DWCi_requestIsTimedOut(struct DWCiReq* req, u32 now);
void DWCi_requestFree(void* p);
void DWCi_socketShutdown(struct DWCiConn* conn);
int DWCi_requestReconnect(struct DWCiConn* conn, u32 addr, u16 port);
int DWCi_requestRetry(struct DWCiConn* conn, u32 addr, u16 port, u32 flag);
int DWCi_requestBlockState(struct DWCiReq* req, u32 a, u32 b, u32* out);
int DWCi_requestFlush(struct DWCiReq* req);
/* 0x805076F0 - the middle band's node pump `DWCi_FreeList` drains (its unit is unregistered, so the
 * map row is a rename of `fn_805076F0` and `src/DWCi/DWCi_Np_CPUCopyFast.c` names it from that call
 * site). */
void DWCi_freeNode(u32 kind, void* node, u32 arg);

/* 0x80507690 - the tagged-block allocator `DWCi_freeNode` releases: one callback allocates
 * `size + 0x20` bytes (the DWCi allocator the runtime's initialiser registers), the block is stamped
 * with the 0x4457434D tag and its size and the payload is handed back (`NULL` when the callback
 * fails).  Name and shape are GUESSes from the body: `src/DWCi/DWCi_Np_CPUCopyFast.c`'s friend-code
 * getter calls it twice as `(3, 0x4000/0x8000, 0x20)`, and the `u8*` return is the payload, not a
 * typed object. */
u8* DWCi_allocNode(u32 kind, u32 size, u32 align);

/* 0x805078F0 - the DWCi report: a `printf`-style, category-filtered logger (the category argument
 * masks 0x1000000/0x8000000 against this band's own enable word at 0x807957C8 and selects the
 * prefix out of the 0x8062FE10 table before calling `OSReport`). */
void DWCi_report(u32 category, const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_DWCI_H */
