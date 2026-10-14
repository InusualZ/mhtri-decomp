/*
 * RVLGX/GXPerf.c - the SDK GX performance counters (`GXSetGPMetric`, `GXClearGPMetric`).
 *
 * RANGE. `.text` 0x804BA9F0-0x804BB220 (2 functions / 0x82C B); `.data` 0x8061ACB8-0x8061ADA0: the jump tables of
 *   `GXSetGPMetric`'s two value switches, the counter-1 table (0x8061ACB8, 22 entries) before the counter-0 table
 *   (0x8061AD10, 35 entries); the last 4 bytes are alignment fill.
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (`-O4,p`); no float operations.
 * NAMES. SDK names: the RVL SDK GXPerf.c functions. The jump tables are `@NNN` locals here and `jumptable_8061ACB8` /
 *   `jumptable_8061AD10` in the map (relocation names only).
 * RESIDUALS. none.
 * SHAPES. `perf0`/`perf1` are `s32` (the old-value range tests end in a signed `cmpwi 34` / `cmpwi 21`). The case
 *   blocks follow the target's block order, which is not numeric: counter 0 runs 0..9, 34, 10..33 and counter 1
 *   runs 0..3, 8, 21, 4..7, 9..20; the `.data` relocation addends check it, objdiff's text score does not.
 */

#include "types.h"
#include "gx.h"
#include "RVLGX/GXInit.h"
#include "RVLGX/GXPerf.h"

/* Writes one XF register through the pipe. */
#define GX_WRITE_XF_REG(addr, value)     (GXWGFifo.u8 = 0x10, GXWGFifo.u32 = (u32)(addr), GXWGFifo.u32 = (u32)(value))
/* Writes one BP (raster) register word through the pipe. */
#define GX_WRITE_BP_REG(value) (GXWGFifo.u8 = 0x61, GXWGFifo.u32 = (u32)(value))
/* Writes one CP register through the pipe. */
#define GX_WRITE_CP_REG(addr, value)     (GXWGFifo.u8 = 0x08, GXWGFifo.u8 = (u8)(addr), GXWGFifo.u32 = (u32)(value))

/* 0x804BA9F0 (0x81C): selects the GP performance metrics of counters 0 and 1. */
void GXSetGPMetric(s32 perf0, s32 perf1)
{
    switch (__GXData->perf0) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 34:
        GX_WRITE_XF_REG(0x1006, 0);
        break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
        GX_WRITE_BP_REG(0x23000000);
        break;
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
        GX_WRITE_BP_REG(0x24000000);
        break;
    }

    switch (__GXData->perf1) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 21:
        GX_WRITE_BP_REG(0x67000000);
        break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        GX_SET_REG(__GXData->perfSel.word, 0, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 17:
    case 18:
    case 19:
    case 20:
        __cpReg[3] = 0;
        break;
    }

    __GXData->perf0 = perf0;
    switch (__GXData->perf0) {
    case 0:
        GX_WRITE_XF_REG(0x1006, 627);
        break;
    case 1:
        GX_WRITE_XF_REG(0x1006, 330);
        break;
    case 2:
        GX_WRITE_XF_REG(0x1006, 363);
        break;
    case 3:
        GX_WRITE_XF_REG(0x1006, 132);
        break;
    case 4:
        GX_WRITE_XF_REG(0x1006, 198);
        break;
    case 5:
        GX_WRITE_XF_REG(0x1006, 528);
        break;
    case 6:
        GX_WRITE_XF_REG(0x1006, 594);
        break;
    case 7:
        GX_WRITE_XF_REG(0x1006, 561);
        break;
    case 8:
        GX_WRITE_XF_REG(0x1006, 429);
        break;
    case 9:
        GX_WRITE_XF_REG(0x1006, 462);
        break;
    case 34:
        GX_WRITE_XF_REG(0x1006, 33);
        break;
    case 10:
        GX_WRITE_XF_REG(0x1006, 339);
        break;
    case 11:
        GX_WRITE_BP_REG(0x2300AE7F);
        break;
    case 12:
        GX_WRITE_BP_REG(0x23008E7F);
        break;
    case 13:
        GX_WRITE_BP_REG(0x23009E7F);
        break;
    case 14:
        GX_WRITE_BP_REG(0x23001E7F);
        break;
    case 15:
        GX_WRITE_BP_REG(0x2300AC3F);
        break;
    case 16:
        GX_WRITE_BP_REG(0x2300AC7F);
        break;
    case 17:
        GX_WRITE_BP_REG(0x2300ACBF);
        break;
    case 18:
        GX_WRITE_BP_REG(0x2300ACFF);
        break;
    case 19:
        GX_WRITE_BP_REG(0x2300AD3F);
        break;
    case 20:
        GX_WRITE_BP_REG(0x2300AD7F);
        break;
    case 21:
        GX_WRITE_BP_REG(0x2300ADBF);
        break;
    case 22:
        GX_WRITE_BP_REG(0x2300ADFF);
        break;
    case 23:
        GX_WRITE_BP_REG(0x2300AE3F);
        break;
    case 24:
        GX_WRITE_BP_REG(0x2300A27F);
        break;
    case 25:
        GX_WRITE_BP_REG(0x2300A67F);
        break;
    case 26:
        GX_WRITE_BP_REG(0x2300AA7F);
        break;
    case 27:
        GX_WRITE_BP_REG(0x2402C0C6);
        break;
    case 28:
        GX_WRITE_BP_REG(0x2402C16B);
        break;
    case 29:
        GX_WRITE_BP_REG(0x2402C0E7);
        break;
    case 30:
        GX_WRITE_BP_REG(0x2402C108);
        break;
    case 31:
        GX_WRITE_BP_REG(0x2402C129);
        break;
    case 32:
        GX_WRITE_BP_REG(0x2402C14A);
        break;
    case 33:
        GX_WRITE_BP_REG(0x2402C1AD);
        break;
    }

    __GXData->perf1 = perf1;
    switch (__GXData->perf1) {
    case 0:
        GX_WRITE_BP_REG(0x67000042);
        break;
    case 1:
        GX_WRITE_BP_REG(0x67000084);
        break;
    case 2:
        GX_WRITE_BP_REG(0x67000063);
        break;
    case 3:
        GX_WRITE_BP_REG(0x67000129);
        break;
    case 8:
        GX_WRITE_BP_REG(0x67000252);
        break;
    case 21:
        GX_WRITE_BP_REG(0x67000021);
        break;
    case 4:
        GX_WRITE_BP_REG(0x6700014B);
        break;
    case 5:
        GX_WRITE_BP_REG(0x6700018D);
        break;
    case 6:
        GX_WRITE_BP_REG(0x670001CF);
        break;
    case 7:
        GX_WRITE_BP_REG(0x67000211);
        break;
    case 9:
        GX_SET_REG(__GXData->perfSel.word, 2, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 10:
        GX_SET_REG(__GXData->perfSel.word, 3, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 11:
        GX_SET_REG(__GXData->perfSel.word, 4, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 12:
        GX_SET_REG(__GXData->perfSel.word, 5, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 13:
        GX_SET_REG(__GXData->perfSel.word, 6, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 14:
        GX_SET_REG(__GXData->perfSel.word, 7, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 15:
        GX_SET_REG(__GXData->perfSel.word, 9, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 16:
        GX_SET_REG(__GXData->perfSel.word, 8, 24, 27);
        GX_WRITE_CP_REG(0x20, __GXData->perfSel.word);
        break;
    case 17:
        __cpReg[3] = 2;
        break;
    case 18:
        __cpReg[3] = 3;
        break;
    case 19:
        __cpReg[3] = 4;
        break;
    case 20:
        __cpReg[3] = 5;
        break;
    }

    __GXData->xfWritePending = 0;
}

/* 0x804BB210 (0x10): clears the GP performance counters. */
void GXClearGPMetric(void)
{
    __cpReg[2] = 4;
}
