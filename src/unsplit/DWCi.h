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
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/dwc_error.h"

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
int DWCi_socketBind(int sock, void* buf, int len);
int DWCi_socketSendTo(int sock, void* buf, int len, u32 flags, void* sa, int salen);
int DWCi_socketGetLocalName(int sock, void* sa, int* salen);
/* 0x8050AC70 - append a copy of the fixed-size record at `item` to the list. */
/* untyped: opaque handle (the list) and byte range (the record copied in) */
void DWCi_listAppend(void* list, void* item);
/* 0x8050AF20 - delete the list's `index`th record. */
/* untyped: opaque handle (the list) */
void DWCi_listDeleteAt(void* list, u32 index);
struct DWCiHostEntry* DWCi_socketLookupHost(char* host);
/* 0x8050C270 - the resolver record of this machine's own host (NULL when it cannot be read). */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void);
/* 0x8050C450 - non-zero when the address the pointer names is a usable (public) one. */
int DWCi_hostAddressIsUsable(DWCiHostAddr* host);
void* DWCi_malloc(u32 size);
void DWCi_free(void* p);
int DWCi_requestSend(struct DWCiConn* conn, struct DWCiReq* req, u32 addr, u16 port, u32 a, void* buf,
                int len, u32 flag);
int DWCi_requestFrame(struct DWCiReq* req, void* p, u32 len);
void DWCi_requestFree(void* p);
/* 0x805076F0 - the middle band's node pump `DWCi_FreeList` drains (its unit is unregistered, so the
 * map row is a rename of `fn_805076F0` and `src/DWCi/DWCi_Np_CPUCopyFast.c` names it from that call
 * site). */
void DWCi_freeNode(u32 kind, void* node, u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_DWCI_H */
