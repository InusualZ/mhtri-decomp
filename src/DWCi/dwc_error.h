/* Declarations owned by `src/DWCi/dwc_error.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_DWCI_DWC_ERROR_H
#define MHTRI_DWCI_DWC_ERROR_H

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* the NATNEG half's own data (rule 2: the owner declares it) */
#include "DWCi/DWCi_Np_CPUCopyFast.h"    /* the Np unit's own data (rule 2) */

/* Declarations moved here from `unsplit/DWCi.h, Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

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

/* 0x805074B0 - register the DWC version string, start the Np layer with `npStartParam`, install the DWCi
 * allocators, copy `gameName` (a string address) and set the library's initialised flag; 0, or -1 when
 * the Np start fails (`sNetworkLibraryWii::init` logs "DWC_Init failed"). */
s32 DWC_Init(u32 npStartParam, u32 gameName, u32 configWord, u32 allocParamA, u32 allocParamB);

/* 0x805075B0 - the inverse of `DWC_Init`: reinstall the allocators, release the host lookup and the free
 * list and clear the initialised flag `DWC_Init` set.  GUESS on the name (the body and the shutdown
 * paths of `sNetworkLibraryWii::final`/`stop`, which call it right before `SOCleanup`). */
void DWC_Shutdown(void);

/* NHTTP / network utility layer */
s32 DWC_GetLastErrorEx(s32* code, s32* type);

void DWC_ClearError(void);

/* 0x805076F0 - the middle band's node pump `DWCi_FreeList` drains (its unit is unregistered, so the
 * map row is a rename of `fn_805076F0` and `src/DWCi/DWCi_Np_CPUCopyFast.c` names it from that call
 * site). */
void DWCi_freeNode(u32 kind, void* node, u32 arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_ERROR_H */
