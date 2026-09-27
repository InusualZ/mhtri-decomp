/*
 * DWCi-band declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The Nintendo Wi-Fi Connection (DWCi) band's transport/GameSpy half lives in
 * 0x80507C40..0x805145B8 (the registered units `DWCi/DWCi_Np_CPUCopyFast.c`, `DWCi/fn_805113B0.c`
 * and `DWCi/DWCi_NatNeg.c`), but the helpers those units call - the 0x8050A..-0x8050E.. socket /
 * list / address layer, `DWCi_freeNode`, the `.sdata` strings and the `.bss`/`.sbss` work blocks -
 * sit in bands no registered unit covers.  stylelint's rule 2 resolves the 0x8050xxxx functions
 * here (the nearest registered ranges below and above both name `DWCi`), and the `.bss`/`.sbss`/
 * `.sdata`/`.data` objects come from the same band; none has an owner's header to move to, so this
 * is their home.  Declared, never defined (playbook 29).
 *
 * The types the declarations reach are the units' own views, so only forward declarations are here
 * (`struct DWCiConn` / `struct DWCiReq` / `struct DWCiAddrKey` / `struct DWCiCType` are completed by
 * `src/DWCi/fn_805113B0.c`, `struct DWCiRuntime` / `struct DWCiListNode` by
 * `src/DWCi/DWCi_Np_CPUCopyFast.c`).
 *
 * Added with the networking conformance pass: `src/DWCi/fn_805113B0.c` declared 52 of these
 * locally, `DWCi_Np_CPUCopyFast.c` six and `DWCi_NatNeg.c` one.
 */
#ifndef MHTRI_UNSPLIT_DWCI_H
#define MHTRI_UNSPLIT_DWCI_H

#include "types.h"

struct DWCiConn;
struct DWCiReq;
struct DWCiAddrKey;
struct DWCiHostEntry;
struct DWCiCType;
struct DWCiRuntime;
struct DWCiListNode;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the band's data (`.sdata`, `.sbss`, `.bss`, `.data`) ------------------------------------ */

/* 0x80794348 - the 2-byte 0xFEFE protocol constant; 0x80794358/0x80794360/0x80794364 are the
 * "%s:%d", "%s" and ":%d" format strings the address helpers sprintf through; 0x80794368 is the
 * empty string `DWCi_GetStringLength` defaults a NULL buffer to. */
extern u16 DWCi_protocolMagic;
extern char DWCi_addressFormat[6];
extern char DWCi_addressFormatHost[3];
extern char DWCi_addressFormatPort[4];
extern u8 DWCi_emptyString[8];

/* 0x8060EDB0 - the character-class record `DWCi_parseAddress` validates port digits against (a header
 * whose +0x38 pointer reaches the per-character u16 flags, bit 3 marking a decimal digit). */
extern struct DWCiCType DWCi_digitClassTable;

/* 0x80795820 - the index of the two-buffer address-string ring `DWCi_formatAddress` writes into the
 * 0x807625C0 buffers. */
extern u32 DWCi_addressRingIndex;
extern u8 DWCi_addressRing[];

/* 0x80760F00 - the DWCi state block (0x1D0 B, first word a status) and 0x807610D0 the 0x190-byte
 * work buffer; 0x807957E0 the free-list head and 0x807957F0 the runtime/result block the state
 * machine publishes into; 0x807957F4 the state-ladder word (retail reloads it, hence volatile). */
extern u32 DWCi_stateBlock[];
extern u8 DWCi_workBuffer[];
extern struct DWCiListNode* DWCi_freeListHead;
extern struct DWCiRuntime* DWCi_runtime;
extern volatile s32 DWCi_state;

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
u32 DWCi_socketResolveAddress(char* host);
int DWCi_socketGetLastError(int sock);
struct DWCiHostEntry* DWCi_socketLookupHost(char* host);
int DWCi_socketIsUsable(int sock);
void DWCi_platformInit(void);
void DWCi_platformCleanup(void);
u32 DWCi_getTick(void);
void* DWCi_malloc(u32 size);
void DWCi_free(void* p);
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
/* 0x805076F0 - the middle band's node pump `DWCi_FreeList` drains (its unit is unregistered, so it
 * keeps the map's `fn_` placeholder; `src/DWCi/DWCi_Np_CPUCopyFast.c` records the rule 7 deferral). */
void DWCi_freeNode(u32 kind, void* node, u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_DWCI_H */
