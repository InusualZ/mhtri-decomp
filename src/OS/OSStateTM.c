/*
 * OS/OSStateTM.c - the OS state/STM event handling: STM init, shutdown-to-standby, hot reset and the state-event
 *    handler.
 * RANGE. .text 0x804D56B0-0x804D5E00 (12 functions); .data 0x80629518-0x806295E0; .bss 0x8074E0A0-0x8074E160; .sbss
 *    0x807953A8-0x807953D0.  Cut from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor
 *    "OSStateTM.c" at .data 0x80629540 and the "/dev/stm/*" device strings (0x80629518); the STM buffers (.bss
 *    0x8074E0A0..0x8074E160) and the `Stm*` state (.sbss 0x807953A8..0x807953D0) are read only here.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names; `OSSetResetCallback` and `OSSetPowerCallback` are the SDK names of the two handler setters;
 *    `__OSSetVIForceDimming` and `VIForceDimmingDone` are GUESSes (ioctl 0x5001 of /dev/stm/immediate, packed as
 *    enable<<7 | step<<3 | level; the callback only clears `StmVdInUse`); `StmImInBuf`, `StmImOutBuf`, `StmVdInBuf` and
 *    `StmVdOutBuf` are GUESSes (the 32-byte in/out blocks of the immediate channel and of the dimming request).
 * RESIDUALS. flipcheck: .sbss claims 0x28 but the object emits 0x20 (the byte at 0x807953C8 is `Debug_BBA`, defined by
 *    `Runtime.PPCEABI.H/__start.c`; the tail of the claim is link padding); .text 0x744 vs the claimed 0x750 pads only once the
 *    successor `OS/OSPlayRecord` is flipped; the string literals are pooled `@NNN` labels where the target names `lbl_80629540` etc.
 *    The .sbss and .bss variables are defined in reverse order of the target's layout because the compiler emits them reversed.
 * SHAPES. plain C; the event-hook registration is a static inline helper the compiler expands in place.
 */

#include "types.h"

#include "IPC/ipcclt.h"
#include "OS/OSCache.h"
#include "OS/OSError.h"
#include "OS/OSInterrupt.h"
#include "OS/OSStateTM.h"
#include "VI/vi.h"

#define STM_IOCTL_REGISTER_EVENT 0x1000
#define STM_IOCTL_HOT_RESET 0x2001
#define STM_IOCTL_SHUTDOWN_TO_STANDBY 0x2003
#define STM_IOCTL_UNREGISTER_EVENT 0x3002
#define STM_IOCTL_FORCE_DIMMING 0x5001
#define STM_IOCTL_SET_IDLE_MODE 0x6002
#define STM_EVENT_RESET 0xFFFE0000
#define STM_EVENT_POWER 0x800

/* The register that holds the reset button state (bit 16 clear when the button is down). */
#define OS_RESET_BUTTON_REGISTER (*(volatile u32*)0xCC003000)
/* The register the shutdown and reset paths clear before the console leaves the game. */
#define OS_STM_PRE_EXIT_REGISTER (*(volatile u16*)0xCC002002)

static void __OSDefaultResetCallback(void);
static void __OSDefaultPowerCallback(void);
static s32 __OSStateEventHandler(s32 result);
static s32 VIForceDimmingDone(s32 result);

__declspec(align(32)) u32 StmEhInBuf[8];
__declspec(align(32)) u32 StmEhOutBuf[8];
__declspec(align(32)) u32 StmImInBuf[8];
__declspec(align(32)) u32 StmImOutBuf[8];
__declspec(align(32)) u32 StmVdInBuf[8];
__declspec(align(32)) u32 StmVdOutBuf[8];

OSResetCallback ResetCallback;
OSPowerCallback PowerCallback;
s32 StmVdInUse;
s32 StmEhRegistered;
s32 StmEhDesc;
s32 StmImDesc;
s32 StmReady;
s32 ResetDown;

/* Posts the asynchronous state-event request; the handler re-posts it on every event. */
static inline void RegisterStateEvent(void)
{
    BOOL enabled = OSDisableInterrupts();

    if (IOS_IoctlAsync(StmEhDesc, STM_IOCTL_REGISTER_EVENT, StmEhInBuf, 0x20, StmEhOutBuf, 0x20, __OSStateEventHandler, NULL) == 0) {
        StmEhRegistered = TRUE;
    } else {
        StmEhRegistered = FALSE;
    }
    OSRestoreInterrupts(enabled);
}

/* Installs the reset button handler and makes sure the state-event request is posted. */
OSResetCallback OSSetResetCallback(OSResetCallback callback)
{
    BOOL enabled = OSDisableInterrupts();
    OSResetCallback previous;

    previous = ResetCallback;

    if (callback != NULL) {
        ResetCallback = callback;
    } else {
        ResetCallback = __OSDefaultResetCallback;
    }
    if (!StmEhRegistered) {
        RegisterStateEvent();
    }
    OSRestoreInterrupts(enabled);
    if (previous == __OSDefaultResetCallback) {
        return NULL;
    }
    return previous;
}

/* Installs the power button handler and makes sure the state-event request is posted. */
OSPowerCallback OSSetPowerCallback(OSPowerCallback callback)
{
    BOOL enabled = OSDisableInterrupts();
    OSPowerCallback previous;

    previous = PowerCallback;

    if (callback != NULL) {
        PowerCallback = callback;
    } else {
        PowerCallback = __OSDefaultPowerCallback;
    }
    if (!StmEhRegistered) {
        RegisterStateEvent();
    }
    OSRestoreInterrupts(enabled);
    if (previous == __OSDefaultPowerCallback) {
        return NULL;
    }
    return previous;
}

/* Opens the two STM channels, installs the default handlers and posts the first state-event request. */
BOOL __OSInitSTM(void)
{
    PowerCallback = __OSDefaultPowerCallback;
    ResetCallback = __OSDefaultResetCallback;
    ResetDown = FALSE;
    if (StmReady) {
        return TRUE;
    }
    StmVdInUse = FALSE;
    StmImDesc = IOS_Open("/dev/stm/immediate", 0);
    if (StmImDesc < 0) {
        StmReady = FALSE;
        return FALSE;
    }
    StmEhDesc = IOS_Open("/dev/stm/eventhook", 0);
    if (StmEhDesc < 0) {
        StmReady = FALSE;
        return FALSE;
    }
    RegisterStateEvent();
    StmReady = TRUE;
    return TRUE;
}

/* Tells the STM to power the console down to standby, then spins with the instruction cache flushed. */
void __OSShutdownToSBY(void)
{
    OS_STM_PRE_EXIT_REGISTER = 0;
    if (!StmReady) {
        OSPanic("OSStateTM.c", 348, "Error: The firmware doesn't support shutdown feature.\n");
    }
    StmImInBuf[0] = 0;
    IOS_Ioctl(StmImDesc, STM_IOCTL_SHUTDOWN_TO_STANDBY, StmImInBuf, 0x20, StmImOutBuf, 0x20);
    OSDisableInterrupts();
    ICFlashInvalidate();
    for (;;) {
    }
}

/* Tells the STM to hot reset the console, then spins with the instruction cache flushed. */
void __OSHotReset(void)
{
    OS_STM_PRE_EXIT_REGISTER = 0;
    if (!StmReady) {
        OSPanic("OSStateTM.c", 412, "Error: The firmware doesn't support reboot feature.\n");
    }
    IOS_Ioctl(StmImDesc, STM_IOCTL_HOT_RESET, StmImInBuf, 0x20, StmImOutBuf, 0x20);
    OSDisableInterrupts();
    ICFlashInvalidate();
    for (;;) {
    }
}

/* Posts one video-dimming request to the STM unless one is already in flight. */
s32 __OSSetVIForceDimming(u32 enable, u32 step, u32 level)
{
    BOOL enabled;
    s32 result;

    if (!StmReady) {
        return -10;
    }
    enabled = OSDisableInterrupts();
    if (StmVdInUse) {
        OSRestoreInterrupts(enabled);
        return 0;
    }
    StmVdInUse = TRUE;
    OSRestoreInterrupts(enabled);
    StmVdInBuf[0] = (step << 3) | level | (enable << 7);
    StmVdInBuf[1] = 0;
    StmVdInBuf[2] = 0;
    StmVdInBuf[3] = 0;
    StmVdInBuf[4] = 0;
    StmVdInBuf[5] = -1;
    StmVdInBuf[6] = 0xFFFF0000;
    StmVdInBuf[7] = 0;
    result = IOS_IoctlAsync(StmImDesc, STM_IOCTL_FORCE_DIMMING, StmVdInBuf, 0x20, StmVdOutBuf, 0x20, VIForceDimmingDone, NULL);
    if (result != 0) {
        return result;
    }
    return 1;
}

s32 SCSetIdleMode(u8 idleMode)
{
    if (!StmReady) {
        return -6;
    }
    StmImInBuf[0] = idleMode;
    return IOS_Ioctl(StmImDesc, STM_IOCTL_SET_IDLE_MODE, StmImInBuf, 0x20, StmImOutBuf, 0x20);
}

/* Withdraws the state-event request. */
s32 __OSUnRegisterStateEvent(void)
{
    s32 result;

    if (!StmEhRegistered) {
        return 0;
    }
    if (!StmReady) {
        return -6;
    }
    result = IOS_Ioctl(StmImDesc, STM_IOCTL_UNREGISTER_EVENT, StmImInBuf, 0x20, StmImOutBuf, 0x20);
    if (result == 0) {
        StmEhRegistered = FALSE;
    }
    return result;
}

/* Completion of the dimming request: the next one may be posted. */
static s32 VIForceDimmingDone(s32 result)
{
    StmVdInUse = FALSE;
    return 0;
}

static void __OSDefaultResetCallback(void)
{
}

static void __OSDefaultPowerCallback(void)
{
}

/* Handles one STM event: runs the reset or power handler and re-posts the request. */
static s32 __OSStateEventHandler(s32 result)
{
    OSResetCallback resetCallback;
    OSPowerCallback powerCallback;
    BOOL enabled;
    BOOL resetPressed;

    if (result != 0) {
        OSPanic("OSStateTM.c", 820, "Error on STM state event handler\n");
    }
    StmEhRegistered = FALSE;
    if (StmEhOutBuf[0] == 0x20000) {
        if (!(OS_RESET_BUTTON_REGISTER & 0x10000)) {
            resetPressed = TRUE;
        } else {
            resetPressed = FALSE;
        }
        if (resetPressed) {
            enabled = OSDisableInterrupts();
            resetCallback = ResetCallback;
            ResetDown = TRUE;
            ResetCallback = __OSDefaultResetCallback;
            resetCallback();
            OSRestoreInterrupts(enabled);
            VIResetDimmingCount();
        }
        RegisterStateEvent();
    }
    if (StmEhOutBuf[0] == STM_EVENT_POWER) {
        enabled = OSDisableInterrupts();
        powerCallback = PowerCallback;
        PowerCallback = __OSDefaultPowerCallback;
        powerCallback();
        OSRestoreInterrupts(enabled);
    }
    return 0;
}
