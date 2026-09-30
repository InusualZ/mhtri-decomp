/*
 * include/DWCi/DWCi_Np_CPUCopyFast.h - the DWCi state-machine unit (`src/DWCi/DWCi_Np_CPUCopyFast.c`,
 * `.text` 0x80507C40..0x80509DB0).
 *
 * Rule 2: this unit owns the data runs below (`config/RMHE08/splits.txt`: `.data`
 * 0x8062FF90..0x806302C8, `.sdata` 0x80794200..0x80794210, `.sbss` 0x807957D0..0x807957F8 - the
 * `.data` run was claimed by the data pass on 2026-09-28, the other two before it), so their
 * declarations live here and every consumer includes this header.  The band header
 * `include/unsplit/DWCi.h` includes it, which is how this unit and `src/DWCi/fn_805113B0.c` reach
 * them.  The `.data` run is *defined* by `src/DWCi/DWCi_Np_CPUCopyFast.c`; the `.sdata` and `.sbss`
 * objects are still declarations only (playbook 29 - the bytes are the target object's).  `.sdata`
 * cannot be defined at all: its two name strings are zero-initialised, and the compiler puts such an
 * object in `.sbss`/`.bss`, never in `.sdata`.  `.sbss` is 32 of the run's 40 B - the source models
 * seven objects in which `DWCi_friendCodeReady` and `DWCi_freeListHead` are 4-byte where the target's
 * rows are 8 bytes each - and a partial `.sbss` is what `flipcheck.py` refuses; an unwritten body is
 * not what holds it up.
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

/* 0x807957D0 - the flag that makes the console friend-code getter below a one-shot: the getter sets
 * it once it has brought the file system up and read the id, and every later call answers out of
 * 0x807957D8.  GUESS (rule 7): the map row covers 8 bytes, the code touches only the first word. */
extern s32 DWCi_friendCodeReady;

/* 0x807957D8 - the console's 64-bit friend code, the value `NWC24iGetUserId` is pointed at and the
 * one the getter returns and reports as `%016lld`.  Its second half is dtk's separate map row
 * 0x807957DC: the value is one object and both references are `@l`/`@l+4` off this symbol. */
extern u64 DWCi_consoleFriendCode;

/* 0x80794204/0x80794208 - the two empty (all-zero) name strings `DWCi_npSetup` hands the session
 * opener as the player-name / friend-code defaults.  Names are GUESSes (rule 7): the image keeps
 * only the 4 zero bytes each and the `strncpy` destinations they feed.  Still declarations: the
 * compiler puts a zero-initialised object in `.sbss`/`.bss`, never in `.sdata` (measured on this
 * host with the unit's own command line), so neither can be defined here without moving it. */
extern char DWCi_npEmptyPlayerName[4];
extern char DWCi_npEmptyFriendCode[4];

/* The unit's two entry points that have no body yet.  Both declarations are GUESSes, marked here as
 * such: `DWCi_Np_CPUCopyFast` is called dst/src/len in r3/r4/r5 from both of its call sites
 * (`fn_80507050` +0xE0, `fn_80507780` +0x90) with the result unused, so it is declared
 * memcpy-shaped with a `void` result - the real return type is not recoverable from the image; and
 * `DWCi_authDataTask` is called with no argument set up at all from the middle band's step
 * functions `DWC_NASLoginProcess`/`DWC_SVLProcess` (+0x3C / +0x100), so it takes none. */
void DWCi_Np_CPUCopyFast(u8* dst, const u8* src, u32 len);
void DWCi_authDataTask(void);

/* 0x80508630 - the unit's third written entry point: the console friend code, fetched once.  The
 * out-parameter `NWC24iGetUserId` writes into is `u32*` in that unit's declaration while the value is
 * 64 bits wide, so the call site casts. */
u64 DWCi_GetConsoleFriendCode(void);

/* The unit's `.data` run, 0x8062FF90..0x806302C8 - the objects the source file defines, in address
 * order (the layout, the two `aligned(8)`/size-rule facts and the claim's evidence are in
 * `src/DWCi/DWCi_Np_CPUCopyFast.c`).  They are exported because the unregistered band above this
 * unit (0x80509DB0..0x805113B0) reads the auth-data path and the two URL tables; rule 2 puts them
 * in the owner's header rather than in a band header.  The run's two switch tables
 * (`DWCi_npCopyFastTailTable`, `DWCi_authDataStateTable`) are `scope:local` in the map and stay
 * file-local here. */
extern char DWCi_reportFriendCodeFormat[];
extern char DWCi_reportFriendCodeFailedFormat[];
extern char DWCi_authDataPath[];
extern char DWCi_acUrlTest[];
extern char DWCi_acUrlProd[];
extern char DWCi_acUrlDev[];
extern char* DWCi_acUrlTable[3];
extern char DWCi_prUrlTest[];
extern char DWCi_prUrlProd[];
extern char DWCi_prUrlDev[];
extern char* DWCi_prUrlTable[3];
extern char DWCi_reportAuthProcessing[];
extern char DWCi_reportMemoryShortage[];
extern char DWCi_reportIfConfigQueryFailed[];
extern char DWCi_reportHttpStartFailed[];
extern char DWCi_reportHttpDestroy[];
extern char DWCi_reportUserIdRead[];
extern char DWCi_reportUserIdReadSize[];
extern char DWCi_reportUserIdDelete[];
extern char DWCi_reportAccountCreateTimeout[];
extern char DWCi_reportUserIdWriteSize[];
extern char DWCi_reportLoginTimeout[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWCI_NP_CPUCOPYFAST_H */
