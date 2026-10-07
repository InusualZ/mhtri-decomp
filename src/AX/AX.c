/*
 * AX/AX.c - the AX audio library core: init, the init flag and the version registration.
 *
 * RANGE. .text 0x8046E3D0-0x8046E4F0 (4 functions, 0x120 B); .data 0x8060F8D8-0x8060F920; .sdata
 *    0x80793D10-0x80793D18; .sbss 0x80794EC8-0x80794ED0.  Cut from the old ARC/AX block; the left edge is
 *    `ARCCloseDir`'s end, the right edge 0x8046E4F0 is `__AXGetStackHead`, the first reader of the voice stacks.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `AXInit`, `AXIsInit` and the `__AX*` callees are the map's; `AXInitEx` (0x8046E430) and `AXQuit` (0x8046E490) are
 *    GUESSes from the public API (the first is `AXInit` with an output mode, the second tears the sub-modules down);
 *    `__AXVersion` and `__AXInitFlag` are GUESSes named from how they are used.
 * EVIDENCE. `.data` 0x8060F8D8 is the build string `<< RVL_SDK - AX release build ... (0x4302_145) >>`, reached
 *    through `.sdata` 0x80793D10 by `AXInit` and `AXInitEx`; `.sbss` 0x80794EC8 (the init flag) is read by
 *    these four functions only.
 * RESIDUALS. none recorded yet.
 * SHAPES. one static inline init body expanded into `AXInit` (mode 0) and `AXInitEx`.
 */

#include "types.h"

#include "AX/AX.h"
#include "AX/AXCL.h"
#include "AX/AXAlloc.h"
#include "AX/AXAux.h"
#include "AX/AXOut.h"
#include "AX/AXSPB.h"
#include "AX/AXVPB.h"
#include "OS/OS.h"

const char* __AXVersion = "<< RVL_SDK - AX \trelease build: Feb 27 2009 10:01:36 (0x4302_145) >>";

BOOL __AXInitFlag;

/* Registers the version and brings up every AX sub-module with the given output mode. */
static inline void __AXInit(u32 mode)
{
    if (__AXInitFlag == FALSE) {
        OSRegisterVersion(__AXVersion);
        __AXAllocInit();
        __AXVPBInit();
        __AXSPBInit();
        __AXAuxInit();
        __AXClInit();
        __AXOutInit(mode);
        __AXInitFlag = TRUE;
    }
}

/* 0x8046E3D0 (0x54): initializes AX with the default output mode. */
void AXInit(void)
{
    __AXInit(0);
}

/* 0x8046E430 (0x60): initializes AX with the given output mode. */
void AXInitEx(u32 mode)
{
    __AXInit(mode);
}

/* 0x8046E490 (0x48): shuts every AX sub-module down. */
void AXQuit(void)
{
    if (__AXInitFlag != FALSE) {
        __AXOutQuit();
        __AXAllocQuit();
        __AXVPBQuit();
        __AXSPBQuit();
        __AXAuxQuit();
        __AXClQuit();
        __AXInitFlag = FALSE;
    }
}

/* 0x8046E4E0 (0x8): returns whether AX has been initialized. */
BOOL AXIsInit(void)
{
    return __AXInitFlag;
}
