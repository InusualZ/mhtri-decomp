/*
 * DWCi/dwc_init.cpp - DWC library init/shutdown and the DWC memory functions with their GameSpy callbacks.
 *
 * RANGE. `.text` 0x805074B0..0x805078F0 (11 functions); `.data` 0x8062FD68..0x8062FE10 (the version and init
 *   strings); `.sdata` 0x807941F8..0x80794200 ("clear"); `.sbss` 0x807957B8..0x807957C8 (game id, init flag,
 *   free/alloc callbacks). Both data edges are seams: the target pads the jump table before these strings
 *   (0x64 -> 0x68) and pads them 0x10C -> 0x110 before the report prefixes, which one object does not emit.
 *   The SDK's dwc_init/dwc_memfunc pair has no data seam between them (the memory half owns only two `.sbss`
 *   words at an 8-aligned start), so it stays one unit.
 * FLAGS. `cflags_base` (the retail functions start 16-byte aligned, `gap_*` words between them); docs/network.md.
 * NAMES. Nintendo DWC SDK names read off the bodies (`DWC_SetMemFunc`/`DWC_Alloc`/`DWC_AllocEx`/`DWC_Free`, the
 *   four GameSpy memory callbacks `DWCi_Gs*` handed to `gsiMemoryCallbacksSet`); GUESS on the `.sbss` word names
 *   and on the file name (the SDK's `dwc_init.c`; no `__FILE__` string).
 * SHAPES. `DWC_Alloc`, `DWCi_GsMalloc` and `DWCi_GsMemalign` carry the tagged-block allocation inline (the
 *   `DWCi_AllocBlock` helper); `DWC_AllocEx` is the same code written out, which is what gives its different register
 *   choice. `DWCi_GsRealloc` is a direct body whose locals are declared new block, old size, header (the colouring
 *   order, first declared = highest number) and whose free is written through the named `header` under the block's
 *   null test: with that second direct use the frontend keeps `header` a named local numbered below the temporaries
 *   (as an inline helper's local or a single-use local it is substituted by its `block - 1` temporary, which then
 *   colours first and takes r31), and the inlined `DWC_AllocEx` result is coalesced into `newBlock`.
 * RESIDUALS. none in `.text`. The target names the `.sdata` "clear" string `DWCi_clearCommand` where ours emits an
 *   anonymous literal (same bytes).
 */
#include "DWCi/dwc_init.h"
#include "DWCi/dwc_report.h"
#include "DWCi/dwc_nonport.h"           /* DWCi_Np_CPUCopyFast, DWCi_GetConsoleFriendCode */
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/natneg.h"
#include "DWCi/gt2Utility.h"
#include "unsplit/DWCi.h"
#include "unsplit/OS.h"
#include "MSL_C/alloc.h"

extern "C" {

/* The header `DWC_AllocEx` puts in front of every block it hands out. */
typedef struct DWCMemHeader {
    /* +0x00 */ u32 tag;
    /* +0x04 */ u32 size;
    /* +0x08 */ u8 pad_0x08[0x18];
} DWCMemHeader; /* size: 0x20 */

#define DWC_MEM_TAG 0x4457434D /* 'DWCM' */
#define DWC_ALLOCTYPE_GS 9

u32 stDwcGameId;
s32 stDwcInitialized;
DWCFreeExFunc stDwcFreeFunc;
DWCAllocExFunc stDwcAllocFunc;

/* 0x805074B0 (0xF8): registers the version, installs the allocators, starts the auth layer and picks the
 * stats server for the given server type. */
s32 DWC_Init(u32 serverType, u32 gameName, u32 gameId, u32 allocFunc, u32 freeFunc) {
    OSRegisterVersion("<< RVL_SDK - DWC \trelease build: May 14 2009 19:45:15 (0x4302_145) >>");
    DWC_SetMemFunc((DWCAllocExFunc)allocFunc, (DWCFreeExFunc)freeFunc);
    if (DWCi_npStart(serverType) == 0) {
        DWC_Printf(8, "Failed to initialize auth interface.\n");
        return -1;
    }
    stDwcGameId = gameId;
    gsiMemoryCallbacksSet(DWCi_GsMalloc, DWCi_GsFree, DWCi_GsRealloc, DWCi_GsMemalign);
    strcpy(DWCi_gameName, (const char*)gameName);
    if (serverType == 0) {
        strcpy(DWCi_statsServerHostname, "sdkdev.gamespy.com");
    } else {
        strcpy(DWCi_statsServerHostname, "gamestats.gs.nintendowifi.net");
    }
    stDwcInitialized = 1;
    DWCi_GetConsoleFriendCode();
    return 0;
}

/* 0x805075B0 (0x54): restores the GameSpy allocators, clears the host cache and the request list. */
void DWC_Shutdown(void) {
    gsiMemoryCallbacksSet(DWCi_GsMalloc, DWCi_GsFree, DWCi_GsRealloc, DWCi_GsMemalign);
    DWCi_socketLookupHost("clear");
    DWCi_FreeList();
    stDwcInitialized = 0;
}

/* 0x80507610 (0xC): installs the application's allocator pair. */
void DWC_SetMemFunc(DWCAllocExFunc allocFunc, DWCFreeExFunc freeFunc) {
    stDwcAllocFunc = allocFunc;
    stDwcFreeFunc = freeFunc;
}

/* Allocates `size` bytes behind a tagged header; NULL when the application allocator fails. */
/* untyped: caller-owned payload - a raw block */
static inline void* DWCi_AllocBlock(u32 name, u32 size, u32 align) {
    DWCMemHeader* header = (DWCMemHeader*)stDwcAllocFunc(name, size + sizeof(DWCMemHeader), align);
    void* block;
    if (header == NULL) {
        block = NULL;
    } else {
        header->tag = DWC_MEM_TAG;
        header->size = size;
        block = header + 1;
    }
    return block;
}

/* Releases a block `DWCi_AllocBlock` returned. */
/* untyped: caller-owned payload - a raw block */
static inline void DWCi_FreeBlock(u32 name, void* block, u32 size) {
    if (block != NULL) {
        stDwcFreeFunc(name, (DWCMemHeader*)block - 1, size);
    }
}

/* 0x80507620 (0x64): allocates a 32-byte aligned block. */
/* untyped: caller-owned payload - a raw block */
void* DWC_Alloc(u32 name, u32 size) {
    return DWCi_AllocBlock(name, size, 32);
}

/* 0x80507690 (0x5C): allocates a block with the given alignment. */
/* untyped: caller-owned payload - a raw block */
void* DWC_AllocEx(u32 name, u32 size, u32 align) {
    DWCMemHeader* header = (DWCMemHeader*)stDwcAllocFunc(name, size + sizeof(DWCMemHeader), align);
    if (header == NULL) {
        return NULL;
    }
    header->tag = DWC_MEM_TAG;
    header->size = size;
    return header + 1;
}

/* 0x805076F0 (0x1C): releases a block. */
/* untyped: caller-owned payload - a raw block */
void DWC_Free(u32 name, void* block, u32 size) {
    if (block == NULL) {
        return;
    }
    stDwcFreeFunc(name, (DWCMemHeader*)block - 1, size);
}

/* 0x80507710 (0x68): the GameSpy `malloc` callback. */
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsMalloc(u32 size) {
    return DWCi_AllocBlock(DWC_ALLOCTYPE_GS, size, 32);
}

/* 0x80507780 (0xD0): the GameSpy `realloc` callback: a new block, the old contents copied, the old block freed. */
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsRealloc(void* block, u32 size) {
    void* newBlock;
    u32 oldSize;
    DWCMemHeader* header;
    newBlock = DWC_AllocEx(DWC_ALLOCTYPE_GS, size, 32);
    if (newBlock == NULL) {
        newBlock = NULL;
    } else if (block != NULL) {
        header = (DWCMemHeader*)block - 1;
        oldSize = header->size;
        DWCi_Np_CPUCopyFast((u8*)newBlock, (const u8*)block, oldSize > size ? size : oldSize);
        if (block != NULL) {
            stDwcFreeFunc(DWC_ALLOCTYPE_GS, header, oldSize);
        }
    }
    return newBlock;
}

/* 0x80507850 (0x24): the GameSpy `free` callback. */
/* untyped: caller-owned payload - a raw block */
void DWCi_GsFree(void* block) {
    DWCi_FreeBlock(DWC_ALLOCTYPE_GS, block, 0);
}

/* 0x80507880 (0x68): the GameSpy `memalign` callback. */
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsMemalign(u32 align, u32 size) {
    return DWCi_AllocBlock(DWC_ALLOCTYPE_GS, size, align);
}

}
