/* Declarations owned by `src/DWCi/dwc_init.cpp` (docs/plan.md 6.5 rule 2): DWC init/shutdown and the DWC memory
 * functions with their GameSpy callbacks. */
#ifndef MHTRI_DWCI_DWC_INIT_H
#define MHTRI_DWCI_DWC_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The application's allocator pair `DWC_Init` installs: (allocation type, size, alignment) and
 * (allocation type, block, size). */
/* untyped: caller-owned payload - the application allocator returns raw memory */
typedef void* (*DWCAllocExFunc)(u32 name, u32 size, u32 align);
/* untyped: caller-owned payload - the block handed back to the application allocator */
typedef void (*DWCFreeExFunc)(u32 name, void* block, u32 size);

/* 0x807957B8..0x807957C8 - the game id `DWC_Init` stores, the initialised flag and the allocator pair. */
extern u32 stDwcGameId;
extern s32 stDwcInitialized;
extern DWCFreeExFunc stDwcFreeFunc;
extern DWCAllocExFunc stDwcAllocFunc;

/* 0x805074B0 - register the DWC version string, install the allocators, start the auth layer, copy the game
 * name and pick the stats server; 0, or -1 when the auth start fails.  The words are the caller's
 * configuration block (server type, game name address, game id, the allocator pair). */
s32 DWC_Init(u32 serverType, u32 gameName, u32 gameId, u32 allocFunc, u32 freeFunc);

/* 0x805075B0 - the inverse of `DWC_Init`. */
void DWC_Shutdown(void);

void DWC_SetMemFunc(DWCAllocExFunc allocFunc, DWCFreeExFunc freeFunc);
/* untyped: caller-owned payload - a raw block */
void* DWC_Alloc(u32 name, u32 size);
/* untyped: caller-owned payload - a raw block */
void* DWC_AllocEx(u32 name, u32 size, u32 align);
/* untyped: caller-owned payload - a raw block */
void DWC_Free(u32 name, void* block, u32 size);

/* The GameSpy memory callbacks (`gsiMemoryCallbacksSet`): every block is allocation type 9. */
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsMalloc(u32 size);
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsRealloc(void* block, u32 size);
/* untyped: caller-owned payload - a raw block */
void DWCi_GsFree(void* block);
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsMemalign(u32 align, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_DWC_INIT_H */
