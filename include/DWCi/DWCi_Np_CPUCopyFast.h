/*
 * include/DWCi/DWCi_Np_CPUCopyFast.h - the DWCi state-machine unit (`src/DWCi/DWCi_Np_CPUCopyFast.c`,
 * `.text` 0x80507C40..0x80509DB0).
 *
 * Rule 2: this unit owns the two data runs below (`config/RMHE08/splits.txt`, claimed by the data
 * pass on 2026-09-28 - the `.sbss` words its own state machine stores and the `.sdata` name strings
 * it hands the session opener), so their declarations live here and every consumer includes this
 * header.  The band header `include/unsplit/DWCi.h` includes it, which is how this unit and
 * `src/DWCi/fn_805113B0.c` reach them; nothing defines them (playbook 29 - the bytes are the target
 * object's).
 *
 * The range's public entry points (`DWCi_AdvanceStatus`, `DWCi_IsStatusReady`, `DWCi_GetStatus`,
 * `DWCi_GetWorkBuffer`, `DWCi_SetResult`, ...) are map names on the unit's own functions and need no
 * declaration here.
 */
#ifndef MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H
#define MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H

#include "types.h"

struct DWCiRuntime;
struct DWCiListNode;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x807957E0 the free-list head and 0x807957F0 the runtime/result block the state machine publishes
 * into; 0x807957F4 the state-ladder word (retail reloads it, hence volatile); 0x807957EC and
 * 0x807957E8 the state-machine argument `DWCi_npStart` remembers and the flag that selects the
 * runtime block's stored host/port over the built-in default pair.  All five sit in the unit's
 * `.sbss` run 0x807957D0..0x807957F8 and every one is *stored* by this object and no other. */
extern struct DWCiListNode* DWCi_freeListHead;
extern struct DWCiRuntime* DWCi_runtime;
extern volatile s32 DWCi_state;
extern u32 DWCi_initArgument;
extern u32 DWCi_useStoredConfig;

/* 0x80794204/0x80794208 - the two empty (all-zero) name strings `DWCi_npSetup` hands the session
 * opener as the player-name / friend-code defaults.  Names are GUESSes (rule 7): the image keeps
 * only the 4 zero bytes each and the `strncpy` destinations they feed. */
extern char DWCi_npEmptyPlayerName[4];
extern char DWCi_npEmptyFriendCode[4];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H */
