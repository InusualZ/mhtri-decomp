/*
 * fn_80423E74.h - views for the 0x80423E74 band (network work record + PatCamellia crypto).
 *
 * The band is the TU that owns the arena/base vectors of the `net_ctrl_wk` record and the PatCamellia
 * wrapper over the retail Camellia cipher.  `NetCtrlWk` itself lives in `Network/network_pat_control.h` (one definition,
 * docs/plan.md 6.5 rule 1) - this header only adds what this unit needs on top.
 *
 * A symbol with no registered owner is declared here rather than in the .cpp (docs/plan.md 6.5 rule 2's
 * unsplit gap; `stylelint` only checks `src/`).
 */
#ifndef MHTRI_FN_80423E74_H
#define MHTRI_FN_80423E74_H

#include "types.h"
#include "Network/network_pat_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Camellia's key schedule (owner: src/Camellia/camellia.c).  Declared here because the vendor header
 * sits beside its source and is not on the include path; a second consumer should promote it into
 * `include/` (rule 2).  `PatCamelliaKey` is the vendor's `KEY_TABLE_TYPE`. */
typedef unsigned int PatCamelliaKey[68];

void Camellia_Ekeygen(int keyBitLength, const unsigned char* rawKey, PatCamelliaKey keyTable);
void Camellia_EncryptBlock(int keyBitLength, const unsigned char* plaintext,
                           const PatCamelliaKey keyTable, unsigned char* cipherText);
void Camellia_DecryptBlock(int keyBitLength, const unsigned char* cipherText,
                           const PatCamelliaKey keyTable, unsigned char* plaintext);

/* The unit's Camellia key schedule (.bss 0x806D3670, 0x110 B) and the 64 x 0x400 arena base
 * (.sbss 0x80794CEC); neither has a registered owner. */
extern PatCamelliaKey lbl_806D3670;
extern u8* lbl_80794CEC;

/* Unsplit game callees. */
u8* fn_800404BC(u32 size);
/* The band's own entry points this neighbour calls (owner: src/fn_80423E74.cpp, rule 2). */
void resetNetSlots(NetCtrlWk* work);
/* 0x80427284 - queues a network command (1 = accepted): the command id, the caller's result byte, an
 * unused word, the argument count and the argument words (at most four). */
s32 queueNetCommand(u32 command, s8* result, s32 unused, s32 arg_count, const s32* args);
void syncScheduleClock(NetCtrlWk* work);
s32 fn_804C2380(u32 id);

/* The layer facade both network units drive.  `getNetworkLayerPat` and the holder type it takes are
 * declared in their owner's header, `Network/NetworkPat.h`, which this header reaches through
 * `Network/network_pat_control.h` (rule 2) - this unit only calls them. */

/* The slot mode-word source value (.sdata2 0x8079C888). */
extern f32 lbl_8079C888;

/* MSL primitives. */
void* memcpy(void* dst, const void* src, u32 size);
void* memset(void* dst, int value, u32 size);
char* strcpy(char* dst, const char* src);

/* ---- the pat-control band's callees in this range (`Network/network_pat_control.cpp`; GUESS on every
 * name below: they are derived from the caller's use) ---- */
struct PatTerms;
struct SystemWork;
/* 0x80424198 - points the record's arena vectors at the shared arena (allocating it on first use). */
void setupArenaVectors(NetCtrlWk* work);
/* 0x80429A68 - raises the network error (state 0x5A) for the work record. */
void setErrorHappened(NetCtrlWk* work);
/* 0x804295A4 - whether the terms object has reached its finished state. */
u32 isTermsCheckFinished(struct PatTerms* terms);
/* 0x80429994 - the per-frame timer tick of the control. */
void tickPatControl(void);
/* 0x80429A94 - stores `code` and sends the control to its shutdown state. */
void abortNetworkControl(NetCtrlWk* work, u8 code);
/* 0x80429AB0 - turns the current state into its failure state. */
void failNetworkControl(NetCtrlWk* work);
/* 0x80429990 - a stub (`blr`). */
void resetFailureState(NetCtrlWk* work);
/* 0x80429A40 - clears the refresh timeout. */
void clearRefreshTimeout(void);
/* 0x80429850 - resets the pat interface singletons. */
void resetPatInterfaces(void);
/* 0x804292B4 - allocates and clears the dialog record. */
void allocateDialogRecord(void);
/* 0x80425648 - repaints the server-select screen and counts the held-button frames. */
void refreshServerScreen(NetCtrlWk* work);
/* 0x80425790 - the message-pool state machine. */
void updateMessagePool(void);
/* 0x80426EA0 - resets the message pool. */
void resetMessagePool(void);
/* 0x80428CA8 - a stub (`blr`) taking the system record. */
void resetSystemState(struct SystemWork* system);
/* 0x8042968C - the reflect (page/event) callback the mediator is handed. */
/* untyped: caller-owned payload - the two trailing words are the mediator's own event payload words */
void patReflectCallback(s32 a, s32 b, s32 c, s32 d, void* e, void* f);
/* 0x80603858 - the per-language (group, group max) word pairs and 0x806038D8 - the server host name,
 * both in this range's `.data`. */
extern s32 language_group_table[10];
extern char pat_server_host[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FN_80423E74_H */
