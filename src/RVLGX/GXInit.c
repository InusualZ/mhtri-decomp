/*
 * RVLGX/GXInit.c - the SDK GX initialisation (`GXInit`, `__GXInitGX`, shutdown hook).
 *
 * RANGE. `.text` 0x804B32D0-0x804B4460 (6 functions, `__GXDefaultTexRegionCallback` .. `__GXInitGX`); `.data`
 *   0x8061A560-0x8061A7A0 (the version string, `DefaultTexData`, `GXDefaultVATList`, `GXDefaultProjData`,
 *   `GXTexRegionAddrTable`, `GXShutdownFuncInfo`); `.bss` 0x80746C20-0x807472A0 (`FifoObj`, `gxData`); `.sdata`
 *   0x80793E48-0x80793E50 (`__GXVersion`); `.sbss` 0x807951C8-0x807951F0 (the four register windows, the
 *   `__GXShutdown` statics, `shutdownFuncRegistered`); `.sdata2` 0x8079D0F0-0x8079D118 (`__GXData`, then the pool).
 *   The first function follows `ISFS_ShutdownAsync`; the right edge is where `GXFifo` begins.
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p); no codegen pragma.
 * NAMES. SDK names throughout: the data statics carry the map's spellings, and the function-local statics compile to
 *   the map's `@LOCAL@__GXShutdown__FiUl@peCount` / `@time@0` / `@calledOnce@1` and
 *   `@LOCAL@GXInit__FPvUl@shutdownFuncRegistered`. The `.data` head is the version-string literal, a compiler-local
 *   `@NNN` that the map names `lbl_8061A560`; `__GXInitGX` reaches the three statics after it through that one base.
 *   `__GXReadMEMCounterU32` and `EnableWriteGatherPipe` are inlined helpers with no symbol of their own.
 * RESIDUALS. none in `.text`, `.data`, `.bss`, `.sbss` or `.sdata2`; the pool labels are compiler-local (`@NNN`)
 *   where the map names `@2712`, `@2713` and `lbl_8079D0FC`..`lbl_8079D110`: same offsets and bytes. The `.sdata`
 *   claim ends with 0x4 bytes of alignment fill after `__GXVersion`.
 * SHAPES. `__GXInitGX` switches on `(s32)VIGetTvFormat()`: `VI/vi.h` declares it `u32` (its other readers compare
 *   unsigned) while the target's binary-tree compare is signed. `GXInit` sets `__piReg` before `__cpReg`: the
 *   other order swaps r4/r5 in the register-window stores. The BP opcode inserts are `GX_SET_REG` on the `GXData`
 *   words; the revision bits build their XF/BP words with `GX_SET_REG` from a zeroed scalar.
 */

#include "types.h"
#include "gx.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXAttr.h"
#include "RVLGX/GXBump.h"
#include "RVLGX/GXFifo.h"
#include "RVLGX/GXFrameBuf.h"
#include "RVLGX/GXGeometry.h"
#include "RVLGX/GXLight.h"
#include "RVLGX/GXMisc.h"
#include "RVLGX/GXPerf.h"
#include "RVLGX/GXPixel.h"
#include "RVLGX/GXSetZCompLoc.h"
#include "RVLGX/GXTev.h"
#include "RVLGX/GXTexture.h"
#include "RVLGX/GXTransform.h"
#include "VI/vi.h"
#include "OS/OS.h"
#include "OS/OSReset.h"
#include "OS/OSTime.h"
#include "OS/PPCArch.h"

/* Writes `value` to a BP (raster) register through the write-gather pipe. */
#define GX_WRITE_RAS_REG(value) (GXWGFifo.u8 = 0x61, GXWGFifo.u32 = (u32)(value))

/* Writes `value` to the CP register `addr` through the write-gather pipe. */
#define GX_WRITE_CP_REG(addr, value) \
    (GXWGFifo.u8 = 0x08, GXWGFifo.u8 = (u8)(addr), GXWGFifo.u32 = (u32)(value))

/* Writes `value` to the XF register `addr` through the write-gather pipe. */
#define GX_WRITE_XF_REG(addr, value) \
    (GXWGFifo.u8 = 0x10, GXWGFifo.u32 = (u32)(addr), GXWGFifo.u32 = (u32)(value))

/* The memory-interface counter pair of the pixel-engine requests (`__memReg` halfwords 0x27 high, 0x28 low). */
#define MEM_PE_REQCOUNTH 0x27
#define MEM_PE_REQCOUNTL 0x28

const char* __GXVersion = "<< RVL_SDK - GX \trelease build: Feb 27 2009 10:04:13 (0x4302_145) >>";

/* The 4x4 IA8 texture `__GXInitGX` loads into every texture map. */
static u8 DefaultTexData[32] __attribute__((aligned(32))) = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

/* The vertex attribute format every vertex format starts with (position, normal, two colours, eight texture
 * coordinates). */
static GXVtxAttrFmtList GXDefaultVATList[13] = {
    {9, 1, 4, 0},
    {10, 0, 4, 0},
    {11, 1, 5, 0},
    {12, 1, 5, 0},
    {13, 1, 4, 0},
    {14, 1, 4, 0},
    {15, 1, 4, 0},
    {16, 1, 4, 0},
    {17, 1, 4, 0},
    {18, 1, 4, 0},
    {19, 1, 4, 0},
    {20, 1, 4, 0},
    {255, 0, 0, 0},
};

/* The default perspective projection `GXSetProjectionv` loads (type word, then the six parameters). */
static f32 GXDefaultProjData[7] = {1.0f, 0.0f, 1.0f, 0.0f, -1.0f, -2.0f, 0.0f};

/* The TMEM addresses of the default texture cache regions: even then odd banks of region sets 0, 1 and 2. */
static u32 GXTexRegionAddrTable[48] = {
    0x00000, 0x10000, 0x20000, 0x30000, 0x40000, 0x50000, 0x60000, 0x70000,
    0x08000, 0x18000, 0x28000, 0x38000, 0x48000, 0x58000, 0x68000, 0x78000,
    0x00000, 0x90000, 0x20000, 0xB0000, 0x40000, 0x98000, 0x60000, 0xB8000,
    0x80000, 0x10000, 0xA0000, 0x30000, 0x88000, 0x50000, 0xA8000, 0x70000,
    0x00000, 0x90000, 0x20000, 0xB0000, 0x40000, 0x90000, 0x60000, 0xB0000,
    0x80000, 0x10000, 0xA0000, 0x30000, 0x80000, 0x50000, 0xA0000, 0x70000,
};

static BOOL __GXShutdown(BOOL final, u32 event);
void __GXInitGX(void);

/* The shutdown record `GXInit` registers once. */
static OSShutdownFunctionInfo GXShutdownFuncInfo = {__GXShutdown, 127};

static GXFifoObj FifoObj;
static GXData gxData;
GXData* const __GXData = &gxData;

volatile u32* __piReg = NULL;
volatile u16* __cpReg = NULL;
volatile u16* __peReg = NULL;
volatile u16* __memReg = NULL;

/* 0x804B32D0 (0xE4): picks the default texture cache region of map `id` for the object's format and mipmapping. */
static GXTexRegion* __GXDefaultTexRegionCallback(const GXTexObj* obj, s32 id)
{
    s32 fmt;
    u8 mm;

    fmt = GXGetTexObjFmt(obj);
    mm = GXGetTexObjMipMap(obj);
    id = id % 8;

    switch (fmt) {
    case 6:
        if (mm) {
            return &__GXData->texRegions2[id];
        }
        return &__GXData->texRegions1[id];
    case 8:
    case 9:
    case 10:
        return &__GXData->texRegions0[id];
    default:
        if (mm) {
            return &__GXData->texRegions1[id];
        }
        return &__GXData->texRegions0[id];
    }
}

/* 0x804B33C0 (0x24): returns the default TLUT region `idx`, or NULL past the twenty regions. */
static GXTlutRegion* __GXDefaultTlutRegionCallback(u32 idx)
{
    if (idx >= 20) {
        return NULL;
    }
    return &__GXData->tlutRegions[idx];
}

/* Reads a 32-bit memory-interface counter from its two halfwords, re-reading until the high half is stable. */
static inline u32 __GXReadMEMCounterU32(u32 addrH, u32 addrL)
{
    u32 hiStart;
    u32 hiNew;
    u32 lo;

    hiStart = __memReg[addrH];
    do {
        hiNew = hiStart;
        lo = __memReg[addrL];
        hiStart = __memReg[addrH];
    } while (hiStart != hiNew);
    return (hiStart << 16) | lo;
}

/* 0x804B33F0 (0x170): waits for the pixel engine to go idle before a shutdown, then stops the FIFO and aborts GX. */
static BOOL __GXShutdown(BOOL final, u32 event)
{
    static u32 peCount;
    static s64 time;
    static u32 calledOnce = 0;
    u32 peCountNew;
    s64 timeNew;

    if (!final) {
        if (!calledOnce) {
            peCount = __GXReadMEMCounterU32(MEM_PE_REQCOUNTH, MEM_PE_REQCOUNTL);
            time = OSGetTime();
            calledOnce = 1;
            return FALSE;
        }

        timeNew = OSGetTime();
        peCountNew = __GXReadMEMCounterU32(MEM_PE_REQCOUNTH, MEM_PE_REQCOUNTL);
        if (timeNew - time < 10) {
            return FALSE;
        }
        if (peCountNew != peCount) {
            peCount = peCountNew;
            time = timeNew;
            return FALSE;
        }
    } else {
        GXSetBreakPtCallback(NULL);
        GXSetDrawSyncCallback(NULL);
        GXSetDrawDoneCallback(NULL);
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        GXWGFifo.u32 = 0;
        PPCSync();
        __cpReg[1] = 0;
        __cpReg[2] = 3;
        __GXData->abtWaitPECopy = TRUE;
        __GXAbort();
    }
    return TRUE;
}

/* 0x804B3560 (0x130): sets the revision-dependent vertex format bits and the XF/BP defaults of this GPU revision. */
void __GXInitRevisionBits(void)
{
    u32 i;

    for (i = 0; i < 8; i++) {
        GX_SET_REG(__GXData->vatA[i].word, 1, 1, 1);
        GX_SET_REG(__GXData->vatB[i].word, 1, 0, 0);
        GX_WRITE_CP_REG(0x80 | i, __GXData->vatB[i].word);
    }
    {
        u32 reg1 = 0;
        u32 reg2 = 0;
        u32 reg3 = 0;

        GX_SET_REG(reg1, 1, 31, 31);
        GX_SET_REG(reg1, 1, 30, 30);
        GX_SET_REG(reg1, 1, 29, 29);
        GX_SET_REG(reg1, 1, 28, 28);
        GX_SET_REG(reg1, 1, 27, 27);
        GX_SET_REG(reg1, 1, 26, 26);
        GX_WRITE_XF_REG(0x1000, reg1);

        GX_SET_REG(reg2, 1, 31, 31);
        GX_WRITE_XF_REG(0x1012, reg2);

        GX_SET_REG(reg3, 1, 31, 31);
        GX_SET_REG(reg3, 1, 30, 30);
        GX_SET_REG(reg3, 1, 29, 29);
        GX_SET_REG(reg3, 1, 28, 28);
        GX_SET_REG(reg3, 0x58, 0, 7);
        GX_WRITE_RAS_REG(reg3);
    }
}

/* Turns the write-gather pipe on at the GX FIFO window. */
static inline void EnableWriteGatherPipe(void)
{
    u32 hid2;

    hid2 = PPCMfhid2();
    PPCMtwpar(0x0C008000);
    hid2 |= 0x40000000;
    PPCMthid2(hid2);
}

/* 0x804B3690 (0x504): initialises GX on the FIFO buffer `base`..`base + size` and returns the FIFO record. */
GXFifoObj* GXInit(void* base, u32 size) /* untyped: byte range */
{
    static BOOL shutdownFuncRegistered = FALSE;
    u32 i;
    u32 freqBase;
    u32 reg;

    OSRegisterVersion(__GXVersion);
    __GXData->inDispList = FALSE;
    __GXData->dlSaveContext = TRUE;
    __GXData->abtWaitPECopy = TRUE;
    __GXData->tcsManEnab = 0;
    __GXData->tevTcEnab = 0;
    GXSetMisc(1, 0);

    __piReg = (volatile u32*)0xCC003000;
    __cpReg = (volatile u16*)0xCC000000;
    __peReg = (volatile u16*)0xCC001000;
    __memReg = (volatile u16*)0xCC004000;
    __GXFifoInit();
    GXInitFifoBase(&FifoObj, base, size);
    GXSetCPUFifo(&FifoObj);
    GXSetGPFifo(&FifoObj);

    if (!shutdownFuncRegistered) {
        OSRegisterShutdownFunction(&GXShutdownFuncInfo);
        shutdownFuncRegistered = TRUE;
    }

    __GXPEInit();
    EnableWriteGatherPipe();

    __GXData->genMode.word = 0;
    __GXData->bpMask.word = 255;
    GX_SET_REG(__GXData->bpMask.word, 0x0F, 0, 7);
    __GXData->lpSize.word = 0;
    GX_SET_REG(__GXData->lpSize.word, 0x22, 0, 7);

    for (i = 0; i < 16; i++) {
        __GXData->tevc[i].word = 0;
        __GXData->teva[i].word = 0;
        __GXData->tref[i / 2].word = 0;
        __GXData->texmapId[i] = 0xFF;
        GX_SET_REG(__GXData->tevc[i].word, 0xC0 + i * 2, 0, 7);
        GX_SET_REG(__GXData->teva[i].word, 0xC1 + i * 2, 0, 7);
        GX_SET_REG(__GXData->tevKsel[i / 2].word, 0xF6 + i / 2, 0, 7);
        GX_SET_REG(__GXData->tref[i / 2].word, 0x28 + i / 2, 0, 7);
    }

    __GXData->iref.word = 0;
    GX_SET_REG(__GXData->iref.word, 0x27, 0, 7);

    for (i = 0; i < 8; i++) {
        __GXData->suTs0[i].word = 0;
        __GXData->suTs1[i].word = 0;
        GX_SET_REG(__GXData->suTs0[i].word, 0x30 + i * 2, 0, 7);
        GX_SET_REG(__GXData->suTs1[i].word, 0x31 + i * 2, 0, 7);
    }

    GX_SET_REG(__GXData->scissorTopLeft.word, 0x20, 0, 7);
    GX_SET_REG(__GXData->scissorBottomRight.word, 0x21, 0, 7);
    GX_SET_REG(__GXData->colorMode.word, 0x41, 0, 7);
    GX_SET_REG(__GXData->dstAlpha.word, 0x42, 0, 7);
    GX_SET_REG(__GXData->zMode.word, 0x40, 0, 7);
    GX_SET_REG(__GXData->pixelCtrl.word, 0x43, 0, 7);
    GX_SET_REG(__GXData->cpTex.word, 0, 23, 24);

    __GXData->viewportZScale = 16777216.0f;
    __GXData->viewportZOffset = 0.0f;
    __GXData->dirtyState = 0;
    __GXData->dirtyVAT = FALSE;

    freqBase = OS_BUS_CLOCK / 500;
    __GXFlushTextureState();
    reg = (freqBase >> 11) | 0x400 | 0x69000000;
    GX_WRITE_RAS_REG(reg);
    __GXFlushTextureState();
    reg = (freqBase / 4224) | 0x200 | 0x46000000;
    GX_WRITE_RAS_REG(reg);

    __GXInitRevisionBits();

    for (i = 0; i < 8; i++) {
        GXInitTexCacheRegion(&__GXData->texRegions0[i], FALSE, GXTexRegionAddrTable[i], 0, GXTexRegionAddrTable[i + 8],
                             0);
        GXInitTexCacheRegion(&__GXData->texRegions1[i], FALSE, GXTexRegionAddrTable[i + 16], 0,
                             GXTexRegionAddrTable[i + 24], 0);
        GXInitTexCacheRegion(&__GXData->texRegions2[i], TRUE, GXTexRegionAddrTable[i + 32], 0,
                             GXTexRegionAddrTable[i + 40], 0);
    }
    for (i = 0; i < 16; i++) {
        GXInitTlutRegion(&__GXData->tlutRegions[i], 0xC0000 + 0x2000 * i, 16);
    }
    for (i = 0; i < 4; i++) {
        GXInitTlutRegion(&__GXData->tlutRegions[i + 16], 0xE0000 + 0x8000 * i, 64);
    }

    __cpReg[3] = 0;
    GX_SET_REG(__GXData->perfSel.word, 0, 24, 27);
    GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
    GX_WRITE_XF_REG(0x1006, 0);
    GX_WRITE_RAS_REG(0x23000000);
    GX_WRITE_RAS_REG(0x24000000);
    GX_WRITE_RAS_REG(0x67000000);

    __GXSetIndirectMask(0);
    __GXSetTmemConfig(2);
    __GXInitGX();
    return &FifoObj;
}

/* 0x804B3BA0 (0x8C0): loads the default state of every GX block for the console's video format. */
void __GXInitGX(void)
{
    GXRenderModeObj* rmode;
    GXTexObj texObj;
    f32 identityMtx[3][4];
    GXColor clear = {64, 64, 64, 255};
    GXColor black = {0, 0, 0, 0};
    GXColor white = {255, 255, 255, 255};
    u32 i;

    switch ((s32)VIGetTvFormat()) {
    case 0:
        rmode = &GXNtsc480IntDf;
        break;
    case 1:
        rmode = &GXPal528IntDf;
        break;
    case 5:
        rmode = &GXEurgb60Hz480IntDf;
        break;
    case 2:
        rmode = &GXMpal480IntDf;
        break;
    default:
        rmode = &GXNtsc480IntDf;
        break;
    }

    GXSetCopyClear(clear, 0xFFFFFF);
    GXSetTexCoordGen2(0, 1, 4, 60, 0, 125);
    GXSetTexCoordGen2(1, 1, 5, 60, 0, 125);
    GXSetTexCoordGen2(2, 1, 6, 60, 0, 125);
    GXSetTexCoordGen2(3, 1, 7, 60, 0, 125);
    GXSetTexCoordGen2(4, 1, 8, 60, 0, 125);
    GXSetTexCoordGen2(5, 1, 9, 60, 0, 125);
    GXSetTexCoordGen2(6, 1, 10, 60, 0, 125);
    GXSetTexCoordGen2(7, 1, 11, 60, 0, 125);
    GXSetNumTexGens(1);
    GXClearVtxDesc();
    GXInvalidateVtxCache();

    for (i = 9; i <= 24; i++) {
        GXSetArray(i, __GXData, 0);
    }
    for (i = 0; i < 8; i++) {
        GXSetVtxAttrFmtv(i, GXDefaultVATList);
    }

    GXSetLineWidth(6, 0);
    GXSetPointSize(6, 0);
    GXEnableTexOffsets(0, 0, 0);
    GXEnableTexOffsets(1, 0, 0);
    GXEnableTexOffsets(2, 0, 0);
    GXEnableTexOffsets(3, 0, 0);
    GXEnableTexOffsets(4, 0, 0);
    GXEnableTexOffsets(5, 0, 0);
    GXEnableTexOffsets(6, 0, 0);
    GXEnableTexOffsets(7, 0, 0);

    identityMtx[0][0] = 1.0f;
    identityMtx[0][1] = 0.0f;
    identityMtx[0][2] = 0.0f;
    identityMtx[0][3] = 0.0f;
    identityMtx[1][0] = 0.0f;
    identityMtx[1][1] = 1.0f;
    identityMtx[1][2] = 0.0f;
    identityMtx[1][3] = 0.0f;
    identityMtx[2][0] = 0.0f;
    identityMtx[2][1] = 0.0f;
    identityMtx[2][2] = 1.0f;
    identityMtx[2][3] = 0.0f;
    GXLoadPosMtxImm(identityMtx, 0);
    GXLoadNrmMtxImm(identityMtx, 0);
    GXSetCurrentMtx(0);
    GXLoadTexMtxImm(identityMtx, 60, 0);
    GXLoadTexMtxImm(identityMtx, 125, 0);

    GXSetViewport(0.0f, 0.0f, rmode->fbWidth, rmode->xfbHeight, 0.0f, 1.0f);
    GXSetProjectionv(GXDefaultProjData);
    __GXSetGenMode();
    GXSetCoPlanar(0);
    GXSetCullMode(2);
    GXSetClipMode(0);
    GXSetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    GXSetScissorBoxOffset(0, 0);

    GXSetNumChans(0);
    GXSetChanCtrl(4, 0, 0, 1, 0, 0, 2);
    GXSetChanAmbColor(4, black);
    GXSetChanMatColor(4, white);
    GXSetChanCtrl(5, 0, 0, 1, 0, 0, 2);
    GXSetChanAmbColor(5, black);
    GXSetChanMatColor(5, white);

    GXInvalidateTexAll();
    GXSetTexRegionCallback(__GXDefaultTexRegionCallback);
    GXSetTlutRegionCallback(__GXDefaultTlutRegionCallback);
    GXInitTexObj(&texObj, DefaultTexData, 4, 4, 3, 0, 0, 0);
    GXLoadTexObj(&texObj, 0);
    GXLoadTexObj(&texObj, 1);
    GXLoadTexObj(&texObj, 2);
    GXLoadTexObj(&texObj, 3);
    GXLoadTexObj(&texObj, 4);
    GXLoadTexObj(&texObj, 5);
    GXLoadTexObj(&texObj, 6);
    GXLoadTexObj(&texObj, 7);

    GXSetTevOrder(0, 0, 0, 4);
    GXSetTevOrder(1, 1, 1, 4);
    GXSetTevOrder(2, 2, 2, 4);
    GXSetTevOrder(3, 3, 3, 4);
    GXSetTevOrder(4, 4, 4, 4);
    GXSetTevOrder(5, 5, 5, 4);
    GXSetTevOrder(6, 6, 6, 4);
    GXSetTevOrder(7, 7, 7, 4);
    GXSetTevOrder(8, 255, 255, 255);
    GXSetTevOrder(9, 255, 255, 255);
    GXSetTevOrder(10, 255, 255, 255);
    GXSetTevOrder(11, 255, 255, 255);
    GXSetTevOrder(12, 255, 255, 255);
    GXSetTevOrder(13, 255, 255, 255);
    GXSetTevOrder(14, 255, 255, 255);
    GXSetTevOrder(15, 255, 255, 255);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 3);
    GXSetAlphaCompare(7, 0, 0, 7, 0);
    GXSetZTexture(0, 17, 0);
    for (i = 0; i < 16; i++) {
        GXSetTevKColorSel(i, 6);
        GXSetTevKAlphaSel(i, 0);
        GXSetTevSwapMode(i, 0, 0);
    }
    GXSetTevSwapModeTable(0, 0, 1, 2, 3);
    GXSetTevSwapModeTable(1, 0, 0, 0, 3);
    GXSetTevSwapModeTable(2, 1, 1, 1, 3);
    GXSetTevSwapModeTable(3, 2, 2, 2, 3);
    for (i = 0; i < 16; i++) {
        GXSetTevDirect(i);
    }
    GXSetNumIndStages(0);
    GXSetIndTexCoordScale(0, 0, 0);
    GXSetIndTexCoordScale(1, 0, 0);
    GXSetIndTexCoordScale(2, 0, 0);
    GXSetIndTexCoordScale(3, 0, 0);

    GXSetFog(0, 0.0f, 1.0f, 0.1f, 1.0f, black);
    GXSetFogRangeAdj(0, 0, NULL);
    GXSetBlendMode(0, 4, 5, 0);
    GXSetColorUpdate(1);
    GXSetAlphaUpdate(1);
    GXSetZMode(1, 3, 1);
    GXSetZCompLoc(1);
    GXSetDither(1);
    GXSetDstAlpha(0, 0);
    GXSetPixelFmt(0, 0);
    GXSetFieldMask(1, 1);
    GXSetFieldMode(rmode->field_rendering, rmode->viHeight == rmode->xfbHeight * 2 ? 1 : 0);
    GXSetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GXSetDispCopyDst(rmode->fbWidth, rmode->efbHeight);
    GXSetDispCopyYScale((f32)rmode->xfbHeight / (f32)rmode->efbHeight);
    GXSetCopyClamp(3);
    GXSetCopyFilter(rmode->aa, rmode->sample_pattern, 1, rmode->vfilter);
    GXSetDispCopyGamma(0);
    GXSetDispCopyFrame2Field(0);
    GXClearBoundingBox();

    GXPokeColorUpdate(1);
    GXPokeAlphaUpdate(1);
    GXPokeDither(0);
    GXPokeBlendMode(0, 0, 1, 15);
    GXPokeAlphaMode(7, 0);
    GXPokeAlphaRead(1);
    GXPokeDstAlpha(0, 0);
    GXPokeZMode(1, 7, 1);
    GXSetGPMetric(35, 22);
    GXClearGPMetric();
}
