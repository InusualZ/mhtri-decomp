/*
 * DVD/dvd.c - the SDK DVD command layer (state machine, error recovery, ReadAbs/Inquiry/Cancel/Reset).
 *
 * RANGE. `.text` 0x804A6410-0x804AB040; `.data` 0x80618800-0x80618C10; `.bss` 0x80741A40-0x807466F0; `.sdata`
 *   0x80793DE0-0x80793DF8; `.sbss` 0x807950A0-0x80795138 (53 functions / 0x4AB8 B).
 *   - the DOL string `dvd.c` (.sdata 0x80793DEC) is the `__FILE__` of its asserts; the version string `<< RVL_SDK -
 *     DVD ... >>` at `.data` 0x80618800 is registered by `DVDInit` through `__DVDVersion` (.sdata 0x80793DE0)
 *   - `.data` jump tables for the state functions, `.bss` 0x80741A40..0x807466F0 (ticket/TMD buffers, command blocks,
 *     the alarms), `.sbss` 0x807950A0..0x80795138 (pause/fatal/motor state)
 *   - the right edge is the queue helpers (`__DVDClearWaitingQueue`), whose only data is `WaitingQueue`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16).
 * NAMES. runtime dump: `StampCommand`, `CategorizeError`, `cbForStoreErrorCode1/3`, `cbForUnrecoveredError(Retry)`,
 *   `stateCoverClosed`, `stateDownRotation`, `cbForStateCoverClosed`, `cbForPrepareCoverRegister`, `CoverAlarmHandler`,
 *   `stateReady`, `DummyCommandBlock`, `CoverAlarm`, `PauseFlag`, `PausingFlag`, `FatalErrorFlag`, `ResumeFromHere`,
 *   `WaitingForCoverOpen/Close`, `MotorStopped`, `CancelLastError`, `CurrCommand`, `executing`.
 *   GUESS (callers and data, SDK scheme): `defaultOptionalCommandChecker`, `FatalAlarmHandler`, `cbForStateError`,
 *   `cbForStoreErrorCode2`, `cbForStateReset`, `DVDCancel`, `cbForCancelSync`, `cbForLowSync`, `cbForPrepareReset`,
 *   `__DVDStopMotorAsync`, `__DVDRestartMotor`; the data names `autoInvalidation`, `checkOptionalCommand`, `FatalAlarm`,
 *   `ResetAlarm`, `CommandInfoCounter`, `Canceling`, `NumInternalRetry`, `MediaChanged`, `PreparingCover`,
 *   `LowSyncResult`, `PrepareResetDone`, `ResetRequired`, `LastError`, `CancelCallback`, `LastState`; the
 *   `DVDOptionalCommandChecker` signature `void (*)(void)` (its only caller, `stateBusy`, is unwritten); the inlined
 *   sequences `issueCommand`, `DVDPause`, `SetCoverAlarm`, `CoverClosed` and the macro `STAMP_INT_TYPE`.
 * RESIDUALS. `DVDCancel` is 204 B vs the target's 192 B (both 51 instructions aligned): it carries a second
 *   `OSSleepThread` tail (+0xC B, 3 instructions) where the target's `bne` (state != 3) and the switch default's `bgt`
 *   share one sleep and the switch cases leave the loop straight to its single exit; its jump table's exit entries are
 *   +0xC as a result. Goto-free alternatives measured: return in the case 208 B (+0x10), returns inside an infinite
 *   loop 212 B (+0x14), a flag variable 204 B (+0xC, one more compare), an inline predicate 220 B (+0x1C); the kept
 *   shape (case `break`, default sleeps and `continue`s) is the closest.
 *   `__DVDPrepareReset` swaps r29/r30 between the outer interrupt level and `DVDResume`'s in the inlined
 *   `__DVDPrepareResetAsync` (local declaration order fixes the standalone copy only).
 *   `__DVDStopMotorCommandBlock` is a 0x60 map row; the block is 0x30 and the trailing 0x30 is unreferenced; `.bss`
 *   also has 0x20 unreferenced bytes before `__DVDTmdBuffer`. `__ErrorInfo` (.bss 0x80746880) sits in
 *   `DVD/dvderror.c`'s run.
 *   Unwritten (17 functions): 0x804A6640-0x804A6944 (stateReadingFST, cbForStateReadingFST), 0x804A6CA0-0x804A71A4
 *   (cbForStateGettingError), 0x804A7470-0x804A8674 (cbForStateGoToRetry .. cbForStateCheckID2), 0x804A8750-0x804A89A0
 *   (ResetAlarmHandler), 0x804A8B60-0x804A8F18 (cbForStateDownRotation, stateCoverClosed_CMD), 0x804A94E0-0x804AA470
 *   (stateBusy, cbForStateBusy), 0x804AA810-0x804AAB74 (DVDCancelAsync). Their data is not defined yet: the `dvd.c`
 *   and FST-panic strings, five jump tables, `ImmCommandList`, `DmaCommandList`, `BB2`, `CurrDiskID`, `GameToc`,
 *   `PartInfo`, `PartitionInfo`, `GamePartition`, `PartInfoCursor`, `BreakRequested`.
 * SHAPES. `StampCommand` is auto-inlined except in the `cbFor*` tails that request the error code, which keep the `bl`:
 *   `#pragma dont_inline on` scopes those functions, so the interrupt-type stamp they share is the macro
 *   `STAMP_INT_TYPE`. `__DVDPrepareResetAsync` (with `cbForPrepareReset` and `DVDResume`) inlines whole into
 *   `__DVDPrepareReset` only under `#pragma inline_max_auto_size(512)`. `.sbss` is emitted in reverse definition order,
 *   `.bss` in first-use order. Volatile flags follow the reloads (`CommandInfoCounter`, `CurrCommand`,
 *   `ResumeFromHere`, ...); `MotorState` is plain (its load is scheduled ahead of the cover-flag stores).
 *   `DVDGetCommandBlockStatus` and `DVDCancel` read `state` through a volatile view; the target reloads it after each
 *   compare.
 */

#include "types.h"
#include "DVD/dvd.h"
#include "DVD/__DVDTestAlarm.h"
#include "DVD/dvd_broadway.h"
#include "DVD/dvderror.h"
#include "DVD/dvdfs.h"
#include "DVD/dvdqueue.h"
#include "DVD/dvdFatal.h"
#include "ESP/esp.h"
#include "OS/OS.h"
#include "OS/OSAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSSetPeriodicAlarm.h"
#include "OS/OSInterrupt.h"
#include "OS/OSThread.h"
#include "OS/OSTime.h"
#include "OS/__OSGetSystemTime.h"
#include "OS/DCInvalidateRange.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The disc's application type byte in low memory (0x80 for a disc title). */
#define DVD_DISC_APP_TYPE (*(u8*)0x80003187)

#define DVD_TIMER_CLOCK (OS_BUS_CLOCK / 4)
#define DVD_MILLISECONDS_TO_TICKS(msec) ((msec) * (DVD_TIMER_CLOCK / 1000))

/* A state function of the driver; `LastState` remembers the one to resume. */
typedef void (*DVDStateFunc)(DVDCommandBlock* block);

/* A hook that may veto an optional drive command. */
typedef void (*DVDOptionalCommandChecker)(void);

void defaultOptionalCommandChecker(void);

/* .data / .sdata */
const char* __DVDVersion = "<< RVL_SDK - DVD \trelease build: Feb 27 2009 10:01:59 (0x4302_145) >>";
static BOOL autoInvalidation = TRUE;
static DVDOptionalCommandChecker checkOptionalCommand = defaultOptionalCommandChecker;

/* .bss (emitted in first-use order) */
static ESTicketView __DVDTicketViewBuffer __attribute__((aligned(32)));
static u8 __DVDTmdBuffer[0x4A00] __attribute__((aligned(32)));
static DVDCommandBlock DummyCommandBlock;
static OSAlarm FatalAlarm;
static OSAlarm CoverAlarm;
static OSAlarm ResetAlarm;
DVDCommandBlock __DVDStopMotorCommandBlock;

/* .sbss (emitted in reverse definition order) */
static DVDStateFunc LastState;
static DVDCommandBlock* executing;
static DVDDiskID* IDShouldBe;
static OSBootInfo* bootInfo;
static volatile u32 CurrCommand;
static DVDCBCallback CancelCallback;
static u32 CancelLastError;
static u32 LastError;
static volatile BOOL ResetRequired;
static u32 MotorState;
static volatile s64 LastResetEnd;
static u32 __DVDNumTmdBytes;
static volatile BOOL PrepareResetDone;
static volatile u32 LowSyncResult;
static BOOL DVDInitialized;
s32 __DVDLayoutFormat;
static volatile BOOL PreparingCover;
static volatile BOOL MediaChanged;
static volatile BOOL MotorStopped;
static volatile BOOL WaitingForCoverClose;
static volatile BOOL WaitingForCoverOpen;
static BOOL FirstTimeInBootrom;
static volatile s32 NumInternalRetry;
static volatile u32 ResumeFromHere;
static volatile BOOL Canceling;
static volatile BOOL FatalErrorFlag;
static volatile BOOL PausingFlag;
static volatile BOOL PauseFlag;
static volatile u32 CommandInfoCounter;

void stateReady(void);
static void stateCoverClosed(void);
void stateDownRotation(DVDCommandBlock* block);
void stateCheckID(void);
void stateBusy(DVDCommandBlock* block);
void cbForStateError(u32 intType);
void cbForStateGettingError(u32 intType);
void cbForStateGoToRetry(u32 intType);
void cbForStateReset(u32 intType);
void cbForStateDownRotation(u32 intType);
void cbForUnrecoveredErrorRetry(u32 intType);
static void cbForStoreErrorCode1(s32 result, DVDCommandBlock* block);
void cbForStoreErrorCode2(s32 result, DVDCommandBlock* block);
void cbForCancelSync(s32 result, DVDCommandBlock* block);
void ResetAlarmHandler(OSAlarm* alarm, OSContext* context);

/* 0x804A6410 (0xB4): records a drive command and its arguments in the error record's command ring. */
void StampCommand(u32 command, u32 param1, u32 param2)
{
    BOOL enabled = OSDisableInterrupts();

    if (CommandInfoCounter >= 5) {
        CommandInfoCounter = 0;
    }
    __ErrorInfo.lastCommand[CommandInfoCounter].command = command;
    __ErrorInfo.lastCommand[CommandInfoCounter].param1 = param1;
    __ErrorInfo.lastCommand[CommandInfoCounter].param2 = param2;
    __ErrorInfo.lastCommand[CommandInfoCounter].tick = OSGetTick();
    CommandInfoCounter++;
    OSRestoreInterrupts(enabled);
}

/* 0x804A64D0 (0x4): accepts every optional command. */
void defaultOptionalCommandChecker(void)
{
}

/* 0x804A64E0 (0x154): initialises the DVD library, the drive interface and the error record. */
void DVDInit(void)
{
    s32 result;
    DVDDiskID* diskId;

    if (DVDInitialized) {
        return;
    }
    OSRegisterVersion(__DVDVersion);
    DVDInitialized = TRUE;
    DVDLowInit();
    if (!__OSInIPL && DVD_DISC_APP_TYPE == 0x80) {
        result = ESP_InitLib();
        if (result == 0) {
            result = ESP_DiGetTicketView(NULL, &__DVDTicketViewBuffer);
        }
        if (result == 0) {
            result = ESP_DiGetTmd(NULL, &__DVDNumTmdBytes);
        }
        if (result == 0) {
            ESP_DiGetTmd(__DVDTmdBuffer, &__DVDNumTmdBytes);
        }
        ESP_CloseLib();
    }
    __DVDFSInit();
    __DVDClearWaitingQueue();
    MotorState = 0;
    bootInfo = (OSBootInfo*)0x80000000;
    IDShouldBe = (DVDDiskID*)0x80000000;
    OSInitThreadQueue(&__DVDThreadQueue);
    DVDLowUnmaskStatusInterrupts();
    DVDLowMaskCoverInterrupt();
    if (bootInfo->magic != 0xE5207C22 && bootInfo->magic != 0x0D15EA5E) {
        FirstTimeInBootrom = TRUE;
    }
    memset(&__ErrorInfo, 0, sizeof(DVDErrorInfo));
    diskId = (DVDDiskID*)0x80000000;
    memcpy(__ErrorInfo.gameName, diskId->gameName, 4);
    __ErrorInfo.diskNumber = diskId->diskNumber;
    __ErrorInfo.gameVersion = diskId->gameVersion;
    __DVDLayoutFormat = 0;
    DVDSetAutoFatalMessaging(TRUE);
}

/* 0x804A6950 (0x4): prints the fatal-error message once the fatal alarm fires. */
void FatalAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    __DVDPrintFatalMessage();
}

/* 0x804A6960 (0x110): puts the driver in the fatal state and fails the running command. */
void cbForStateError(u32 intType)
{
    DVDCommandBlock* finished;

    if (__DVDGetAutoFatalMessaging()) {
        OSCreateAlarm(&FatalAlarm);
        OSSetAlarm(&FatalAlarm, 1, FatalAlarmHandler);
        return;
    }
    executing->state = -1;
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    FatalErrorFlag = TRUE;
    finished = executing;
    executing = &DummyCommandBlock;
    if (finished->callback) {
        finished->callback(-1, finished);
    }
    if (Canceling) {
        Canceling = FALSE;
        if (CancelCallback) {
            CancelCallback(0, finished);
        }
    }
    stateReady();
}

/* 0x804A6A70 (0x14): stops the motor once the error code is stored, then enters the error state. */
static void cbForStoreErrorCode1(s32 result, DVDCommandBlock* block)
{
    DVDLowStopMotor(0, 0, cbForStateError);
}

/* 0x804A6A90 (0x3C): resets the drive without spin-up once the error code is stored. */
void cbForStoreErrorCode2(s32 result, DVDCommandBlock* block)
{
    DVDLowSetSpinupFlag(0);
    DVDLowReset(cbForStateError);
    ResetRequired = FALSE;
    ResumeFromHere = 0;
}

/* 0x804A6AD0 (0xF4): classifies a drive error: 0 fatal, 1 retry, 2 move on, 3 retry the command. */
s32 CategorizeError(u32 error)
{
    if (error == 0x20400) {
        LastError = error;
        return 1;
    }
    error &= 0xFFFFFF;
    if (error == 0x62800 || error == 0x23A00 || error == 0x53000 || error == 0xB5A01) {
        return 0;
    }
    if (error == 0x52000 && (executing->command == 37 || LastState == stateDownRotation)) {
        return 0;
    }
    ++NumInternalRetry;
    if (NumInternalRetry == 2) {
        if (error == LastError) {
            LastError = error;
            return 1;
        } else {
            LastError = error;
            return 2;
        }
    }
    LastError = error;
    if (error == 0x31100 || executing->command == 5) {
        return 2;
    }
    return 3;
}

/* 0x804A6BD0 (0xC4): stops the motor before a retry once the error code is stored. */
void cbForStoreErrorCode3(s32 result, DVDCommandBlock* block)
{
    StampCommand(0x10, 0, 0);
    DVDLowStopMotor(0, 0, cbForStateGoToRetry);
}

/* Stores the interrupt type that ended the newest command in the error record. */
#define STAMP_INT_TYPE(intType)                                                  \
    do {                                                                         \
        BOOL enabled_ = OSDisableInterrupts();                                   \
        if (CommandInfoCounter == 0) {                                           \
            __ErrorInfo.lastCommand[4].intType = (intType);                      \
        } else {                                                                 \
            __ErrorInfo.lastCommand[CommandInfoCounter - 1].intType = (intType); \
        }                                                                        \
        OSRestoreInterrupts(enabled_);                                           \
    } while (0)

/* 0x804A71B0 (0x1E0): handles an unrecoverable command: stops the motor or reads the error. */
void cbForUnrecoveredError(u32 intType)
{
    STAMP_INT_TYPE(intType);
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    if (intType & 1) {
        StampCommand(0x10, 0, 0);
        DVDLowStopMotor(0, 0, cbForStateGoToRetry);
        return;
    }
    StampCommand(0x27, 0, 0);
    DVDLowRequestError(cbForUnrecoveredErrorRetry);
}

/* 0x804A7390 (0xD4): stores the drive's error code once the error request completes. */
void cbForUnrecoveredErrorRetry(u32 intType)
{
    STAMP_INT_TYPE(intType);
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    if (intType & 2) {
        __DVDStoreErrorCode(0x1234567, cbForStoreErrorCode1);
        return;
    }
    __DVDStoreErrorCode(DVDLowGetImmBufferReg(), cbForStoreErrorCode1);
}

/* 0x804A8680 (0xC4): resumes the command that was running when the cover closed. */
static void stateCoverClosed(void)
{
    DVDCommandBlock* finished;

    MotorState = 1;
    switch (CurrCommand) {
    case 4:
    case 5:
    case 13:
    case 15:
    case 33:
    case 34:
    case 37:
    case 41:
    case 42:
        __DVDClearWaitingQueue();
        finished = executing;
        executing = &DummyCommandBlock;
        if (finished->callback) {
            finished->callback(-4, finished);
        }
        stateReady();
        break;
    case 32:
        MotorState = 0;
    case 35:
    case 36:
    case 38:
        executing->state = 1;
        stateBusy(executing);
        break;
    case 1:
    case 2:
        if (__OSInIPL) {
            break;
        }
    default:
        MotorState = 0;
        DVDLowSetSpinupFlag(1);
        DVDLowReset(cbForStateReset);
        break;
    }
}

#pragma push
#pragma dont_inline on
/* 0x804A89A0 (0xE4): arms the reset alarm once the drive reset completes. */
void cbForStateReset(u32 intType)
{
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    if (intType & 1) {
        LastResetEnd = __OSGetSystemTime();
        ResetRequired = FALSE;
        ResumeFromHere = 0;
        OSCreateAlarm(&ResetAlarm);
        OSSetAlarm(&ResetAlarm, DVD_MILLISECONDS_TO_TICKS(100), ResetAlarmHandler);
        return;
    }
    StampCommand(0x27, 0, 0);
    DVDLowRequestError(cbForStateGettingError);
}
#pragma pop

/* 0x804A8A90 (0xC8): lowers the drive's rotation speed. */
void stateDownRotation(DVDCommandBlock* block)
{
    DVDLowClearCoverInterrupt(NULL);
    StampCommand(0x25, 0, 0);
    DVDLowSetMaximumRotation(0x20000, cbForStateDownRotation);
}

#pragma push
#pragma dont_inline on
/* 0x804A8F20 (0xD8): checks the disc id once the cover has closed. */
void cbForStateCoverClosed(u32 intType)
{
    STAMP_INT_TYPE(intType);
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    if (intType & 1) {
        NumInternalRetry = 0;
        stateCheckID();
        return;
    }
    StampCommand(0x27, 0, 0);
    DVDLowRequestError(cbForStateGettingError);
}
#pragma pop

/* Resumes the stopped command once the cover is closed again. */
static inline void CoverClosed(void)
{
    WaitingForCoverClose = FALSE;
    if (CurrCommand == 3) {
        MediaChanged = TRUE;
    }
    if (MotorState == 2) {
        if (executing) {
            executing->state = 12;
        }
    } else {
        DVDLowMaskCoverInterrupt();
        if (executing) {
            executing->state = 3;
            stateCoverClosed();
        } else {
            ResumeFromHere = 7;
        }
    }
}

/* 0x804A9000 (0x188): tracks the cover opening and closing from the cover register. */
void cbForPrepareCoverRegister(u32 intType)
{
    PreparingCover = FALSE;
    if (WaitingForCoverClose) {
        if (!(DVDLowGetCoverRegister() & 1)) {
            OSCancelAlarm(&CoverAlarm);
            WaitingForCoverClose = FALSE;
            CoverClosed();
        }
    } else if (DVDLowGetCoverRegister() & 1) {
        WaitingForCoverOpen = FALSE;
        WaitingForCoverClose = TRUE;
        if (MotorState == 2) {
            executing->state = 12;
        } else {
            executing->state = 5;
        }
    } else if (DVDLowGetCoverRegister() & 4) {
        OSCancelAlarm(&CoverAlarm);
        WaitingForCoverOpen = FALSE;
        DVDLowClearCoverInterrupt(NULL);
        CoverClosed();
    }
}

/* 0x804A9190 (0x24): polls the cover register while no poll is in flight. */
void CoverAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    if (PreparingCover) {
        return;
    }
    PreparingCover = TRUE;
    DVDLowPrepareCoverRegister(cbForPrepareCoverRegister);
}

/* Starts polling the cover every 100 ms unless a poll is already armed. */
static inline void SetCoverAlarm(void)
{
    MotorState = 1;
    if (!WaitingForCoverOpen && !WaitingForCoverClose) {
        WaitingForCoverOpen = TRUE;
        OSCreateAlarm(&CoverAlarm);
        OSSetPeriodicAlarm(&CoverAlarm, OSGetTick(), DVD_MILLISECONDS_TO_TICKS(100), CoverAlarmHandler);
    }
}

/* 0x804A91C0 (0x31C): starts the next queued command, or resumes the one an error interrupted. */
void stateReady(void)
{
    DVDCommandBlock* finished;

    if (PauseFlag) {
        PausingFlag = TRUE;
        executing = NULL;
        return;
    }
    if (!__DVDCheckWaitingQueue()) {
        executing = NULL;
        return;
    }
    executing = __DVDPopWaitingQueue();
    if (FatalErrorFlag) {
        executing->state = -1;
        finished = executing;
        executing = &DummyCommandBlock;
        if (finished->callback) {
            finished->callback(-1, finished);
        }
        stateReady();
        return;
    }
    CurrCommand = executing->command;
    if (CurrCommand == 32 || CurrCommand == 14 || CurrCommand == 35) {
        ResumeFromHere = 0;
    }
    if (ResumeFromHere) {
        switch (ResumeFromHere) {
        case 2:
            executing->state = 11;
            SetCoverAlarm();
            break;
        case 3:
            executing->state = 4;
            SetCoverAlarm();
            break;
        case 4:
            executing->state = 5;
            SetCoverAlarm();
            break;
        case 1:
        case 6:
        case 7:
            executing->state = 3;
            stateCoverClosed();
            break;
        case 5:
            __DVDStoreErrorCode(CancelLastError, cbForStoreErrorCode1);
            break;
        }
        ResumeFromHere = 0;
        return;
    }
    switch (MotorState) {
    case 2:
        if (MotorStopped) {
            executing->state = 12;
        } else {
            executing->state = 3;
            stateCoverClosed();
        }
        break;
    case 0:
        executing->state = 1;
        stateBusy(executing);
        break;
    default:
        stateCoverClosed();
        break;
    }
}

/* Invalidates the destination of a read, queues `block` and starts it when the driver is idle. */
static inline BOOL issueCommand(s32 prio, DVDCommandBlock* block)
{
    BOOL level;
    BOOL result;

    if (autoInvalidation &&
        (block->command == 1 || block->command == 4 || block->command == 5 || block->command == 33 ||
         block->command == 14)) {
        DCInvalidateRange(block->addr, block->length);
    }
    level = OSDisableInterrupts();
    block->state = 2;
    result = __DVDPushWaitingQueue(prio, block);
    if (executing == NULL && PauseFlag == FALSE) {
        stateReady();
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x804AA470 (0xE4): queues an asynchronous read of `length` bytes at `offset` into `addr`. */
/* untyped: byte range the read fills */
s32 DVDReadAbsAsyncPrio(DVDCommandBlock* block, void* addr, s32 length, s32 offset, DVDCBCallback callback, s32 prio)
{
    block->command = 1;
    block->addr = addr;
    block->length = length;
    block->offset = offset;
    block->transferredSize = 0;
    block->callback = callback;
    return issueCommand(prio, block);
}

/* 0x804AA560 (0xD8): queues a drive inquiry that fills `info`. */
s32 DVDInquiryAsync(DVDCommandBlock* block, DVDDriveInfo* info, DVDCBCallback callback)
{
    block->command = 14;
    block->addr = info;
    block->length = sizeof(DVDDriveInfo);
    block->transferredSize = 0;
    block->callback = callback;
    return issueCommand(2, block);
}

/* 0x804AA640 (0xC8): returns the state of a queued command as the caller sees it. */
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* block)
{
    BOOL enabled;
    s32 retVal;
    DVDCommandBlock* next;
    const volatile DVDCommandBlock* vblock = block;

    enabled = OSDisableInterrupts();
    if (vblock->state == 3) {
        retVal = 1;
    } else if (vblock->state == 5) {
        retVal = 4;
    } else if (executing == &__DVDStopMotorCommandBlock) {
        next = __DVDGetNextWaitingQueue();
        if (next) {
            if (block == next) {
                retVal = 1;
            } else {
                retVal = vblock->state;
            }
        } else if (block == &__DVDStopMotorCommandBlock) {
            retVal = 0;
        } else {
            retVal = vblock->state;
        }
    } else {
        retVal = vblock->state;
    }
    OSRestoreInterrupts(enabled);
    return retVal;
}

/* 0x804AA710 (0x9C): returns the drive's state. */
s32 DVDGetDriveStatus(void)
{
    BOOL enabled;
    s32 retVal;

    enabled = OSDisableInterrupts();
    if (FatalErrorFlag) {
        retVal = -1;
    } else if (PausingFlag) {
        retVal = 8;
    } else if (executing == NULL) {
        retVal = 0;
    } else if (executing == &DummyCommandBlock) {
        retVal = 0;
    } else {
        retVal = DVDGetCommandBlockStatus(executing);
    }
    OSRestoreInterrupts(enabled);
    return retVal;
}

/* 0x804AA7B0 (0x10): sets the read invalidation flag and returns its previous value. */
BOOL DVDSetAutoInvalidation(BOOL enable)
{
    BOOL prev = autoInvalidation;

    autoInvalidation = enable;
    return prev;
}

/* Stops the driver from starting new commands. */
static inline void DVDPause(void)
{
    BOOL level = OSDisableInterrupts();

    PauseFlag = TRUE;
    if (executing == NULL) {
        PausingFlag = TRUE;
    }
    OSRestoreInterrupts(level);
}

/* 0x804AA7C0 (0x50): lets the driver start commands again. */
void DVDResume(void)
{
    BOOL level = OSDisableInterrupts();

    PauseFlag = FALSE;
    if (PausingFlag) {
        PausingFlag = FALSE;
        stateReady();
    }
    OSRestoreInterrupts(level);
}

/* 0x804AAB80 (0xC0): cancels `block` and sleeps until the cancel completes. */
s32 DVDCancel(DVDCommandBlock* block)
{
    BOOL enabled;
    s32 state;

    if (DVDCancelAsync(block, cbForCancelSync) == FALSE) {
        return -1;
    }
    enabled = OSDisableInterrupts();
    for (;;) {
        state = ((volatile DVDCommandBlock*)block)->state;
        if (state == 0 || state == -1 || state == 10) {
            break;
        }
        if (state == 3) {
            switch (block->command) {
            case 4:
            case 5:
            case 13:
            case 15:
            case 33:
            case 34:
            case 37:
            case 41:
            case 42:
                break;
            default:
                OSSleepThread(&__DVDThreadQueue);
                continue;
            }
            break;
        }
        OSSleepThread(&__DVDThreadQueue);
    }
    OSRestoreInterrupts(enabled);
    return 0;
}

/* 0x804AAC40 (0x8): wakes the threads waiting in `DVDCancel`. */
void cbForCancelSync(s32 result, DVDCommandBlock* block)
{
    OSWakeupThread(&__DVDThreadQueue);
}

/* 0x804AAC50 (0x8): returns the disc header at 0x80000000. */
DVDDiskID* DVDGetCurrentDiskID(void)
{
    return (DVDDiskID*)0x80000000;
}

/* 0x804AAC60 (0x8): records the result of a synchronous low-level command. */
void cbForLowSync(u32 result)
{
    LowSyncResult = result;
}

/* 0x804AAC70 (0xCC): reads the cover register: 0 unknown or too soon after a reset, 1 open, 2 closed. */
u32 __DVDGetCoverStatus(void)
{
    u32 reg;

    LowSyncResult = 0;
    DVDLowPrepareCoverRegister(cbForLowSync);
    while (LowSyncResult == 0) {
    }
    if (!(LowSyncResult & 1)) {
        return 0;
    }
    reg = DVDLowGetCoverRegister();
    if (__OSGetSystemTime() - LastResetEnd < DVD_MILLISECONDS_TO_TICKS(100)) {
        return 0;
    }
    return (reg & 1) ? 1 : 2;
}

/* 0x804AAD40 (0x44): resets the drive without spin-up and waits for the reset to complete. */
void DVDResetDriveSync(void)
{
    LowSyncResult = 0;
    DVDLowSetSpinupFlag(0);
    DVDLowReset(cbForLowSync);
    while (LowSyncResult == 0) {
    }
}

#pragma push
#pragma inline_max_auto_size(512)
/* 0x804AAD90 (0x11C): cancels every queued and running command and calls `callback` when the drive is idle. */
void __DVDPrepareResetAsync(DVDCBCallback callback)
{
    BOOL level;
    BOOL enabled;
    DVDCommandBlock* block;

    enabled = OSDisableInterrupts();
    __DVDClearWaitingQueue();
    if (Canceling) {
        CancelCallback = callback;
    } else {
        if (executing) {
            executing->callback = NULL;
        }
        level = OSDisableInterrupts();
        DVDPause();
        while ((block = __DVDPopWaitingQueue()) != NULL) {
            DVDCancelAsync(block, NULL);
        }
        if (executing) {
            DVDCancelAsync(executing, callback);
        } else if (callback) {
            callback(0, NULL);
        }
        DVDResume();
        OSRestoreInterrupts(level);
    }
    OSRestoreInterrupts(enabled);
}

/* 0x804AAEB0 (0xC): marks the reset preparation as done. */
void cbForPrepareReset(s32 result, DVDCommandBlock* block)
{
    PrepareResetDone = TRUE;
}

/* 0x804AAEC0 (0x138): quiesces the drive ahead of a reset and waits until it is idle. */
void __DVDPrepareReset(void)
{
    OSDisableInterrupts();
    PrepareResetDone = FALSE;
    __DVDPrepareResetAsync(cbForPrepareReset);
    OSEnableInterrupts();
    while (PrepareResetDone != TRUE) {
    }
}

#pragma pop

/* 0x804AB000 (0x20): nonzero when `alarm` is one of the DVD driver's own alarms. */
BOOL __DVDTestAlarm(OSAlarm* alarm)
{
    if (alarm == &ResetAlarm) {
        return TRUE;
    }
    return DVDLowTestAlarm(alarm);
}

/* 0x804AB020 (0x8): reports success without stopping the motor. */
BOOL __DVDStopMotorAsync(DVDCommandBlock* block, DVDCBCallback callback)
{
    return TRUE;
}

/* 0x804AB030 (0x4): does nothing. */
void __DVDRestartMotor(void)
{
}
