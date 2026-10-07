/*
 * DWCi/dwc_error.cpp - the DWC error state, library init/shutdown, memory functions and report printer.
 *
 * RANGE. `.text` 0x805073C0..0x80507C40 (14 functions); `.data` 0x8062FD00..0x8062FF90 (the error-type jump
 *   table, the init strings, the report prefixes); `.sdata` 0x807941F8..0x80794200 ("clear"); `.sbss`
 *   0x807957B0..0x807957D0 (error, code, game id, init flag, free/alloc callbacks, report level).
 * FLAGS. `cflags_base` (the retail functions start 16-byte aligned, `gap_*` words between them; the lib's
 *   `cflags_dwc` packs on 4); measurement in docs/network.md.
 * NAMES. Nintendo DWC SDK names read off the bodies (error pair, `DWC_SetMemFunc`/`DWC_Alloc`/`DWC_AllocEx`/
 *   `DWC_Free`, the four GameSpy memory callbacks `DWCi_Gs*` handed to `gsiMemoryCallbacksSet`, `DWC_Printf`);
 *   GUESS on `DWCi_SetError` (the setter twin of `DWC_ClearError`) and on the `.sbss` word names.
 * SHAPES. `DWC_Alloc`, `DWCi_GsMalloc`, `DWCi_GsMemalign` and `DWCi_GsRealloc` carry the tagged-block
 *   allocation inline (the `DWCi_AllocBlock` helper); `DWC_AllocEx` is the same code written out, which is what
 *   gives its different register choice.
 * RESIDUALS. `DWCi_GsRealloc` swaps r30/r31 between the new block and the old size (no source order of the
 *   locals moved it). `DWC_Init`/`DWC_Printf` differ only in the string relocations (ours `.data`+offset, the
 *   target's dtk labels). The object is three retail TUs (dwc_error | dwc_init+memfunc | dwc_report): the
 *   target `.data` has a 4-byte pad after the jump table (0x64 -> 0x68) and the sections end padded
 *   (`.data` 0x283/0x290, `.sdata` 6/8, `.sbss` 0x1C/0x20), which one TU does not emit - a flip blocker.
 */
#include "DWCi/dwc_error.h"
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/DWCi_NatNeg.h"
#include "DWCi/fn_805113B0.h"
#include "NAND/nand.h"
#include "OS/OSVReport.h"
#include "stdarg.h"
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
#define DWC_ERROR_FATAL 9

s32 stDwcLastError;
s32 stDwcErrorCode;
u32 stDwcGameId;
s32 stDwcInitialized;
DWCFreeExFunc stDwcFreeFunc;
DWCAllocExFunc stDwcAllocFunc;
u32 stDwcReportLevel;

/* 0x805073C0 (0xAC): returns the last error, its code and the error type the dialog layer shows. */
s32 DWC_GetLastErrorEx(s32* code, s32* type) {
    if (code != NULL) {
        *code = stDwcErrorCode;
    }
    if (type != NULL) {
        switch ((u32)stDwcLastError) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 8:
        case 14:
        case 16:
        case 18:
        case 21:
            *type = 6;
            break;
        case 6:
            *type = 3;
            break;
        case 7:
            *type = 4;
            break;
        case 13:
            *type = 5;
            break;
        case 22:
            *type = 8;
            break;
        case 10:
        case 11:
        case 12:
        case 23:
            *type = 1;
            break;
        case 15:
        case 17:
        case 19:
        case 20:
        case 24:
            *type = 2;
            break;
        case 1:
        case 9:
            *type = 7;
            break;
        default:
            *type = 0;
            break;
        }
    }
    return stDwcLastError;
}

/* 0x80507470 (0x1C): clears the error state unless it is fatal. */
void DWC_ClearError(void) {
    if (stDwcLastError != DWC_ERROR_FATAL) {
        stDwcLastError = 0;
        stDwcErrorCode = 0;
    }
}

/* 0x80507490 (0x18): records an error and its code unless a fatal error is already pending. */
void DWCi_SetError(s32 error, s32 code) {
    if (stDwcLastError != DWC_ERROR_FATAL) {
        stDwcLastError = error;
        stDwcErrorCode = code;
    }
}

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

/* Moves a block into a new allocation of `size` bytes, copying what fits and freeing the old block. */
/* untyped: caller-owned payload - a raw block */
static inline void* DWCi_ReallocBlock(u32 name, void* block, u32 oldSize, u32 size, u32 align) {
    void* newBlock;
    DWCMemHeader* header;
    newBlock = DWC_AllocEx(name, size, align);
    if (newBlock == NULL) {
        return NULL;
    }
    if (block != NULL) {
        header = (DWCMemHeader*)block - 1;
        oldSize = header->size;
        DWCi_Np_CPUCopyFast((u8*)newBlock, (const u8*)block, oldSize > size ? size : oldSize);
        DWC_Free(name, block, oldSize);
    }
    return newBlock;
}

/* 0x80507780 (0xD0): the GameSpy `realloc` callback: a new block, the old contents copied, the old block freed. */
/* untyped: caller-owned payload - a raw block */
void* DWCi_GsRealloc(void* block, u32 size) {
    return DWCi_ReallocBlock(DWC_ALLOCTYPE_GS, block, 0, size, 32);
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

/* 0x805078F0 (0x348): prints a report line with its category prefix when the category is enabled. */
void DWC_Printf(u32 level, const char* format, ...) {
    va_list args;
    if (level & stDwcReportLevel) {
        switch ((s32)level) {
        case 0x1:
            OSReport("DWC_INFO     :");
            break;
        case 0x2:
            OSReport("++DWC_ERROR  :");
            break;
        case 0x4:
            OSReport("DWC_DEBUG    :");
            break;
        case 0x8:
            OSReport("DWC_WARN     :");
            break;
        case 0x10:
            OSReport("DWC_ACHECK   :");
            break;
        case 0x20:
            OSReport("DWC_LOGIN    :");
            break;
        case 0x40:
            OSReport("DWC_MATCH_NN :");
            break;
        case 0x80:
            OSReport("DWC_MATCH_GT2:");
            break;
        case 0x100:
            OSReport("DWC_TRANSPORT:");
            break;
        case 0x200:
            OSReport("DWC_QR2_REQ  :");
            break;
        case 0x400:
            OSReport("DWC_SB_UPDATE:");
            break;
        case 0x8000:
            OSReport("DWC_SEND     :");
            break;
        case 0x10000:
            OSReport("DWC_RECV     :");
            break;
        case 0x20000:
            OSReport("DWC_UPDATE_SV:");
            break;
        case 0x40000:
            OSReport("DWC_CONNECTINET:");
            break;
        case 0x1000000:
            OSReport("DWC_AUTH     :");
            break;
        case 0x2000000:
            OSReport("DWC_AC       :");
            break;
        case 0x4000000:
            OSReport("DWC_BM       :");
            break;
        case 0x8000000:
            OSReport("DWC_UTIL     :");
            break;
        case 0x10000000:
            OSReport("DWC_OPTION_CF:");
            break;
        case 0x20000000:
            OSReport("DWC_OPTION_CONNTEST:");
            break;
        case (s32)0x80000000:
            OSReport("DWC_GAMESPY  :");
            break;
        default:
            OSReport("DWC_UNKNOWN  :");
            break;
        }
        va_start(args, format);
        OSVReport(format, args);
        va_end(args);
    }
}

}
