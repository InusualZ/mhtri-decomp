/*
 * SO/soi.h - declarations of the symbols owned by `SO/soi.cpp` that other units call or read.
 */
#ifndef SO_SOI_H
#define SO_SOI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* untyped: opaque band object, typed by the callers' views */
u32 fn_80526F00(void* list, u32 arg1, u32 arg2);

/* helper entry points in the neighbouring (unsplit) subsystem TUs. */
/* untyped: opaque band object, typed by the callers' views */
u32 fn_80529430(u32 index, u32 value, void* callback, u32 arg);

void fn_80529B50(u8 index, u32 value);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `include/unsplit/DWCi.h, SO.h, VF.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x80520658 / 0x80520664 - the peer-id word at `msg+8` and the negotiator socket token a record's
 * +0x08 holds.  Retail's body for both is a bare `blr`: the argument comes back unchanged, so this
 * build has them as two 4-byte SO-band entries that do no conversion, and their names are taken from
 * how their call sites use the result (`DWCi_socketTokenFromPeer(token)`, the peer word read out of
 * a received `msg+8`, compared against a record's token; `DWCi_peerTokenFromSocket(rec->token)`,
 * written into `msg+8` of an outgoing one). */
s32 DWCi_socketTokenFromPeer(u32 peerId);

s32 DWCi_peerTokenFromSocket(s32 sock);

/* 0x80520604 - format the network-order address word at *addr as "a.b.c.d". */
char* SOAddressToString(u32* addr);

/* 0x8052065C - the port conversion `DWCi_sendControlFrame`/`DWCi_sendTo` store into a connection. */
u16 SOAddressToHostPort(u16 port);

/* 0x8051FAC0 - close the socket whose descriptor is inside the SO band's connection record; the
 * NHTTP library's async cleanup closes the one `NHTTPi_Startup` opened (`-1` when there is none). */
s32 SOClose(s32 fd);

/* 0x8051F8FC - open a socket (domain 2 = AF_INET, type 1 = stream); the descriptor, or a negative error. */
s32 __SOCreateSocket(s32 domain, s32 type, s32 protocol);

/* 0x80521054 - set one socket option; `value` is the option's own bytes. */
s32 SOSetSockOpt(s32 fd, s32 level, s32 option, u32* value, s32 length);

/* 0x805202E8 - shut the socket down (how 2 = both directions). */
s32 SOShutdown(s32 fd, s32 how);

/* 0x80520148 / 0x80520194 - receive into / send from a byte range; the byte count, or a negative error. */
s32 SORecv(s32 fd, void* buf, s32 length, s32 flags); /* untyped: byte range */

s32 SOSend(s32 fd, void* buf, s32 length, s32 flags); /* untyped: byte range */

/* 0x80521DE0 - 1 while the layers `VFipf2Init` brings up are up.  (The word it reads is the flag
 * `VFipf2Init` sets and `VFipf2Shutdown` clears.) */
u32 VFipf2IsInitialized(void);

/* 0x80521CA0 - under this band's own mutex, `VFSysInit(work, size)` followed by the disk manager,
 * prfile2 and dHash bring-up; the second argument is the work buffer's size (`DWCi_GetConsoleFriendCode`
 * hands it a freshly allocated 0x8000-byte block). */
void VFipf2Init(u8* work, u32 size);

/* 0x80521D70 - the matching take-down: finalize the file system and clear the flag. */
void VFipf2Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
