/*
 * OS/OSReset.c - the OS reset: shutdown functions, `OSRestart`, the return-to-menu family and `OSGetResetCode`.
 * RANGE. .text 0x804D21F0-0x804D2B90 (13 functions); .data 0x8061D410-0x8061D678; .sbss 0x80795380-0x80795390.  Cut
 *    from the OS core band 0x804C1760-0x804D9B4C.  Evidence: the `__FILE__` anchor "OSReset.c" at .data
 *    0x8061D410 opens its strings (read by `OSRestart`, `__OSReturnToMenu`, ...); `ShutdownFunctionQueue` (.sbss
 *    0x80795388) is read by the register/call/shutdown trio; `WriteSramCallback` (OSRtc) reads the RTC control block and opens
 *    the next unit.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. map names; GUESS: `OSShutdownSystem` (shuts down to standby or launches the menu on the idle-mode setting),
 *    `OSReturnToDataManager` (names the "OSReturnToDataManager()" failure text), `__OSDoHotReset` (the hot-reset tail
 *    `OSRestart` and the menu launchers share), `__OSGetDiscState` (the 1/2/3 disc state the state-flags record keeps; it has no
 *    caller in the DOL's OS core but `fn_804D7A80`), `BootDol` (0x80795380: the DOL offset `OSRestart` hands `__OSReboot`).
 * RESIDUALS. data: the object's string pool also holds the texts of the obsolete functions the linker stripped
 *    (`OSReturnToSetting` and the data-manager page names, `OSSetBootDol`); no function of this source references them, so
 *    the pool is shorter than the target's.
 * SHAPES. plain C; the shutdown-function walk is an inline helper expanded in `__OSCallShutdownFunctions` and
 *    `__OSShutdownDevices`.
 */

#include "types.h"

#include "DVD/dvd.h"
#include "ESP/esp.h"
#include "OS/OSArena.h"
#include "OS/OSAudioSystem.h"
#include "OS/OSContext.h"
#include "OS/OS.h"
#include "OS/OSError.h"
#include "OS/OSExec.h"
#include "OS/OSInterrupt.h"
#include "OS/OSLaunch.h"
#include "OS/OSPlayRecord.h"
#include "OS/OSPlayTime.h"
#include "OS/OSReboot.h"
#include "OS/OSReset.h"
#include "OS/OSRtc.h"
#include "OS/OSStateFlags.h"
#include "OS/OSStateTM.h"
#include "OS/OSThread.h"
#include "OS/LCEnable.h"
#include "PAD/pad.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "SC/sc.h"
#include "VI/vi3in1.h"
#include "unsplit/SC.h"

#define OS_ACTIVE_THREAD_QUEUE (*(OSThreadQueue*)0x800000DC)

#define STATE_TYPE_RETURN_TO_MENU 3
#define DISC_STATE_READY 1
#define DISC_STATE_INSERTED 2
#define DISC_STATE_COVER_OPEN 3

typedef struct ShutdownQueue {
    /* +0x00 */ OSShutdownFunctionInfo* head;
    /* +0x04 */ OSShutdownFunctionInfo* tail;
} ShutdownQueue; /* size: 0x08 */

static ShutdownQueue ShutdownFunctionQueue;
s32 OSReturnToMenuPending;
static u32 BootDol;

/* Inserts `info` into the shutdown list ahead of the first entry of higher priority number. */
void OSRegisterShutdownFunction(OSShutdownFunctionInfo* info)
{
    OSShutdownFunctionInfo* prev;
    OSShutdownFunctionInfo* next;

    for (next = ShutdownFunctionQueue.head; next && next->priority <= info->priority; next = next->next) {
    }
    if (next == NULL) {
        prev = ShutdownFunctionQueue.tail;
        if (prev == NULL) {
            ShutdownFunctionQueue.head = info;
        } else {
            prev->next = info;
        }
        info->prev = prev;
        info->next = NULL;
        ShutdownFunctionQueue.tail = info;
        return;
    }
    info->next = next;
    prev = next->prev;
    next->prev = info;
    info->prev = prev;
    if (prev == NULL) {
        ShutdownFunctionQueue.head = info;
        return;
    }
    prev->next = info;
}

/* Runs every shutdown function of the first priority group that still succeeds; returns whether all succeeded. */
static inline BOOL CallShutdownFunctions(BOOL final, u32 event)
{
    BOOL err = FALSE;
    u32 priority = 0;
    OSShutdownFunctionInfo* info;

    for (info = ShutdownFunctionQueue.head; info != NULL; info = info->next) {
        if (err && priority != info->priority) {
            break;
        }
        err |= !info->func(final, event);
        priority = info->priority;
    }
    err |= !__OSSyncSram();
    return !err;
}

/* The same walk for the device shutdown; its locals are declared in the order that reproduces the target's registers. */
static inline BOOL CallShutdownFunctionsForDevices(BOOL final, u32 event)
{
    OSShutdownFunctionInfo* info;
    u32 priority = 0;
    BOOL err = FALSE;

    for (info = ShutdownFunctionQueue.head; info != NULL; info = info->next) {
        if (err && priority != info->priority) {
            break;
        }
        err |= !info->func(final, event);
        priority = info->priority;
    }
    err |= !__OSSyncSram();
    return !err;
}

/* Calls the shutdown functions for `event`. */
BOOL __OSCallShutdownFunctions(BOOL final, u32 event)
{
    return CallShutdownFunctions(final, event);
}

/* Shuts down every device: shutdown functions in two passes, the LC, the pad calibration and all live threads. */
void __OSShutdownDevices(u32 event)
{
    OSThread* thread;
    OSThread* next;
    BOOL padState;
    BOOL keepPadState;

    if ((u32)(event - 5) <= 1 || event == 0) {
        keepPadState = FALSE;
    } else {
        keepPadState = TRUE;
    }
    __OSStopAudioSystem();
    if (!keepPadState) {
        padState = __PADDisableRecalibration(TRUE);
    }
    while (!CallShutdownFunctionsForDevices(FALSE, event)) {
    }
    while (!__OSSyncSram()) {
    }
    OSDisableInterrupts();
    CallShutdownFunctionsForDevices(TRUE, event);
    LCDisable();
    if (!keepPadState) {
        __PADDisableRecalibration(padState);
    }
    for (thread = OS_ACTIVE_THREAD_QUEUE.head; thread; thread = next) {
        next = thread->linkActive.next;
        if ((s32)thread->state == OS_THREAD_STATE_READY || (s32)thread->state == OS_THREAD_STATE_WAITING) {
            OSCancelThread(thread);
        }
    }
}

/* Returns the disc state the state-flags record keeps: cover open, disc ready after a flagged RTC, or inserted. */
u32 __OSGetDiscState(u32 discState)
{
    u32 flags;

    if (__DVDGetCoverStatus() != 2) {
        return DISC_STATE_COVER_OPEN;
    }
    if (discState == 1 && __OSGetRTCFlags(&flags) && flags == 0) {
        return DISC_STATE_READY;
    }
    return DISC_STATE_INSERTED;
}

/* Shuts the console down: to standby, or to the system menu when the idle mode asks for it. */
void OSShutdownSystem(void)
{
    u8 idleMode[2];
    OSStateFlags state;
    OSIOSRev iosRev;

    memset(idleMode, 0, 2);
    SCInit();
    while (SCCheckStatus() == 1) {
    }
    SCGetIdleMode((SCIdleModeInfo*)idleMode);
    __OSStopPlayRecord();
    __OSUnRegisterStateEvent();
    __DVDPrepareReset();
    __OSReadStateFlags(&state);
    state.discState = __OSGetDiscState(state.discState);
    if (idleMode[0] == 1) {
        state.type = 5;
    } else {
        state.type = 1;
    }
    __OSClearRTCFlags();
    __OSWriteStateFlags(&state);
    __OSGetIOSRev(&iosRev);
    if (idleMode[0] == 1) {
        OSReturnToMenuPending = TRUE;
        OSDisableScheduler();
        __OSShutdownDevices(5);
        OSEnableScheduler();
        __OSLaunchMenu();
        return;
    }
    OSDisableScheduler();
    __OSShutdownDevices(2);
    __OSShutdownToSBY();
}

/* Restarts into the same title (or reboots the DOL) when started from a channel, otherwise hot-resets. */
void OSRestart(u32 resetCode)
{
    u8 appType = OSGetAppType();

    __OSStopPlayRecord();
    __OSUnRegisterStateEvent();
    if (appType == 0x81) {
        OSDisableScheduler();
        __OSShutdownDevices(4);
        OSEnableScheduler();
        __OSRelaunchTitle(resetCode);
    } else if (appType == 0x80) {
        OSDisableScheduler();
        __OSShutdownDevices(4);
        OSEnableScheduler();
        __OSReboot(resetCode, BootDol);
    }
    OSDisableScheduler();
    __OSShutdownDevices(1);
    if (__OSInNandBoot || __OSInReboot) {
        __OSInitSTM();
    }
    __OSHotReset();
    OSPanic("OSReset.c", 1034, "__OSHotReset(): Falied to reset system.\n");
}

/* Records an error exit in the state flags and launches the system menu; does not return. */
static inline void ReturnToMenuForError(void)
{
    OSStateFlags state;

    __OSReadStateFlags(&state);
    state.discState = 2;
    state.type = STATE_TYPE_RETURN_TO_MENU;
    __OSClearRTCFlags();
    __OSWriteStateFlags(&state);
    __OSLaunchMenu();
    OSDisableScheduler();
    __VISetRGBModeImm();
    if (__OSInNandBoot || __OSInReboot) {
        __OSInitSTM();
    }
    __OSHotReset();
    OSPanic("OSReset.c", 1034, "__OSHotReset(): Falied to reset system.\n");
    OSPanic("OSReset.c", 1010, "__OSReturnToMenu(): Falied to boot system menu.\n");
}

/* Returns to the system menu with `returnToMenu` recorded in the state flags; does not return. */
void __OSReturnToMenu(u8 returnToMenu)
{
    OSStateFlags state;
    u32 played;
    u32 remaining;
    ESTicketView* ticketView;

    __OSStopPlayRecord();
    __OSUnRegisterStateEvent();
    __DVDPrepareReset();
    __OSReadStateFlags(&state);
    state.discState = __OSGetDiscState(state.discState);
    state.type = STATE_TYPE_RETURN_TO_MENU;
    state.returnToMenu = returnToMenu;
    __OSClearRTCFlags();
    __OSWriteStateFlags(&state);
    OSSetArenaLo((void*)0x81280000);
    OSSetArenaHi((void*)0x812F0000);
    if (ESP_InitLib() != 0) {
        ReturnToMenuForError();
    }
    ticketView = (ESTicketView*)OSAllocFromMEM1ArenaLo(0xE0, 0x20);
    if (ticketView == NULL) {
        ReturnToMenuForError();
    }
    memset(ticketView, 0, 0xE0);
    if (ESP_DiGetTicketView(NULL, ticketView) == 0 && OSPlayTimeIsLimited()) {
        played = 0;
        remaining = -1;
        __OSGetPlayTime(ticketView, (s32*)&played, (u32*)&remaining);
        if (remaining == 0) {
            __OSWriteExpiredFlagIfSet();
        }
    }
    OSDisableScheduler();
    __OSShutdownDevices(5);
    OSEnableScheduler();
    __OSLaunchMenu();
    OSDisableScheduler();
    __VISetRGBModeImm();
    if (__OSInNandBoot || __OSInReboot) {
        __OSInitSTM();
    }
    __OSHotReset();
    OSPanic("OSReset.c", 1034, "__OSHotReset(): Falied to reset system.\n");
}

/* Returns to the system menu; a failure to do so is fatal. */
void OSReturnToMenu(void)
{
    __OSReturnToMenu(0);
    OSPanic("OSReset.c", 895, "OSReturnToMenu(): Falied to boot system menu.\n");
}

/* Returns to the data manager; a failure to do so is fatal. */
void OSReturnToDataManager(void)
{
    __OSReturnToMenu(1);
    OSPanic("OSReset.c", 913, "OSReturnToDataManager(): Falied to boot system menu.\n");
}

/* Records an error exit in the state flags and launches the system menu; does not return. */
void __OSReturnToMenuForError(void)
{
    ReturnToMenuForError();
}

/* Hot-resets the console; a failure to do so is fatal. */
void __OSDoHotReset(void)
{
    if (__OSInNandBoot || __OSInReboot) {
        __OSInitSTM();
    }
    __OSHotReset();
    OSPanic("OSReset.c", 1034, "__OSHotReset(): Falied to reset system.\n");
}

/* Returns the reset code: the pending reboot's, else the hardware's. */
u32 OSGetResetCode(void)
{
    if (__OSRebootParams.valid != 0) {
        return __OSRebootParams.resetCode | 0x80000000;
    }
    return *(u32*)0xCC003024 >> 3;
}

/* Obsolete; always fails. */
void OSResetSystem(s32 reset, u32 resetCode, s32 forceMenu)
{
    OSPanic("OSReset.c", 1185, "OSResetSystem() is obsoleted. It doesn't work any longer.\n");
}
