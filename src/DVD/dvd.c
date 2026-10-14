/*
 * DVD/dvd.c - the SDK DVD command layer (state machine, error recovery, ReadAbs/Inquiry/Cancel/Reset).
 *
 * RANGE. `.text` 0x804A6410-0x804AB040; `.data` 0x80618800-0x80618C10; `.bss` 0x80741A40-0x807466F0; `.sdata`
 *   0x80793DE0-0x80793DF8; `.sbss` 0x807950A0-0x80795138 (53 functions / 0x4AB8 B).
 *   - the DOL string `dvd.c` (.sdata 0x80793DEC) is the `__FILE__` of the FST-size panic; the version string `<< RVL_SDK -
 *     DVD ... >>` at `.data` 0x80618800 is registered by `DVDInit` through `__DVDVersion` (.sdata 0x80793DE0)
 *   - `.data`: the two strings, six jump tables (`stateCoverClosed`, `stateBusy` x2, `DVDCancelAsync` x2, `DVDCancel`)
 *     and `ImmCommandList`; `.bss` (ticket/TMD buffers, BB2, disc id, TOC, partition table, command blocks, alarms);
 *     `.sbss` (pause/fatal/motor/partition state)
 *   - the right edge is the queue helpers (`__DVDClearWaitingQueue`), whose only data is `WaitingQueue`
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16); the file
 *   states `#pragma inline_bottom_up off` (see SHAPES).
 * NAMES. runtime dump: `StampCommand`, `CategorizeError`, `cbForStoreErrorCode1/3`, `cbForUnrecoveredError(Retry)`,
 *   `stateCoverClosed`, `stateDownRotation`, `cbForStateCoverClosed`, `cbForPrepareCoverRegister`, `CoverAlarmHandler`,
 *   `stateReady`, `DummyCommandBlock`, `CoverAlarm`, `PauseFlag`, `PausingFlag`, `FatalErrorFlag`, `ResumeFromHere`,
 *   `WaitingForCoverOpen/Close`, `MotorStopped`, `CancelLastError`, `CurrCommand`, `executing`; the map's state and
 *   data names (`stateReadingFST` .. `cbForStateBusy`, `BB2`, `CurrDiskID`, `GameToc`, `PartInfo`, `PartitionInfo`,
 *   `PartInfoCursor`, `GamePartition`, `BreakRequested`, `ImmCommandList`, `DmaCommandList`).
 *   GUESS (callers and data, SDK scheme): `defaultOptionalCommandChecker`, `FatalAlarmHandler`, `cbForStateError`,
 *   `cbForStoreErrorCode2`, `cbForStateReset`, `DVDCancel`, `cbForCancelSync`, `cbForLowSync`, `cbForPrepareReset`,
 *   `__DVDStopMotorAsync`, `__DVDRestartMotor`; the data names `autoInvalidation`, `checkOptionalCommand`, `FatalAlarm`,
 *   `ResetAlarm`, `CommandInfoCounter`, `Canceling`, `NumInternalRetry`, `MediaChanged`, `PreparingCover`,
 *   `LowSyncResult`, `PrepareResetDone`, `ResetRequired`, `LastError`, `CancelCallback`, `LastState`; the types
 *   `DVDBB2`, `DVDPartitionInfo`, `DVDGameTOC`, `DVDPartitionParams` (its offsets from `stateBusy` commands 41/42) and
 *   the low-memory macros `DVD_DEVICE_CODE` (0x800030E6), `DVD_BOOT_PARTITION_TYPE/OFFSET` (0x80003194/8); the inlined
 *   sequences `issueCommand`, `DVDPause`, `SetCoverAlarm`, `CoverClosed`, `CheckCancel`, `stateGettingError`,
 *   `StampIntType`, `FinishCommand`, `IsDmaCommand`, `IsImmCommandWithResult`.
 * RESIDUALS. `__DVDPrepareReset` swaps r29/r30 between the outer interrupt level and `DVDResume`'s in the
 *   inlined `__DVDPrepareResetAsync` (6 bytes); local declaration order and a `DVDCancelAllAsync` inline helper were
 *   measured and fix only the standalone copy or break it.
 *   `.sbss` matches the target's layout (0x98 B, section alignment 32) through the `aligned(32)` `__DVDNumTmdBytes`
 *   (an IOS output word); the split object's `.sbss` is aligned 8, so a `DVD/dvdfs.c` flip needs `align:32` on this
 *   unit's `.sbss` split line (without it this unit's `.sbss` links 0x10 low, at 0x80795090).
 *   `__DVDStopMotorCommandBlock` is a 0x60 map row; the block is 0x30 and the trailing 0x30 is unreferenced (`.bss`
 *   0x4C80 vs 0x4CB0). `__ErrorInfo` (.bss 0x80746880) sits in `DVD/dvderror.c`'s run.
 *   Relocation names only (bytes identical): the jump tables, the two strings and the `.bss` base (`...bss.0` for
 *   `__DVDTicketViewBuffer`) are anonymous in our object.
 * SHAPES. `#pragma inline_bottom_up off`: an inline helper's own calls are not inlined again, which is why the
 *   error-code tail `stateGettingError` keeps `bl StampCommand` while a direct `StampCommand` call auto-inlines, and
 *   why `cbForStateCheckID2`'s inlined `stateReadingFST` keeps its `bl`; code the target inlines one level deeper
 *   (the cover-closed re-read in `ResetAlarmHandler`/`cbForStateDownRotation`, the partition open) is written out.
 *   `__DVDPrepareResetAsync` (with `cbForPrepareReset` and `DVDResume`) inlines whole into `__DVDPrepareReset` only
 *   under `#pragma inline_max_auto_size(512)`. `__DVDTmdBuffer` is aligned 64, which puts the `.bss` block at the
 *   target offsets the base-relative accesses use. `.sbss` is emitted in reverse definition order, `.bss` in first-use
 *   order. Volatile flags follow the reloads (`CommandInfoCounter`, `CurrCommand`, `ResumeFromHere`, ...);
 *   `MotorState` is plain. `DVDGetCommandBlockStatus` and `DVDCancelAsync` read `state` through a volatile view (the
 *   target reloads it after each compare); `DVDCancel` reads it once per pass and sleeps in the loop increment, so the
 *   switch cases leave the loop and one `OSSleepThread` serves both waits. `cbForStateBusy` spells its finishes out
 *   (one `finished` variable keeps r31).
 */

#include "types.h"
#pragma inline_bottom_up off
#include "DVD/dvd.h"
#include "DVD/__DVDTestAlarm.h"
#include "DVD/dvd_broadway.h"
#include "DVD/dvderror.h"
#include "DVD/dvdfs.h"
#include "DVD/dvdqueue.h"
#include "DVD/dvdFatal.h"
#include "DVD/dvdidutils.h"
#include "ESP/esp.h"
#include "OS/OS.h"
#include "OS/OSError.h"
#include "MSL_C/alloc.h"
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

/* The drive's device code from its inquiry, kept in low memory. */
#define DVD_DEVICE_CODE (*(u16*)0x800030E6)

#define DVD_TIMER_CLOCK (OS_BUS_CLOCK / 4)
#define DVD_MILLISECONDS_TO_TICKS(msec) ((msec) * (DVD_TIMER_CLOCK / 1000))

/* Rounds a byte count up to the 32-byte DMA granule. */
#define OSRoundUp32B(x) (((u32)(x) + 32 - 1) & ~(32 - 1))

/* A state function of the driver; `LastState` remembers the one to resume. */
typedef void (*DVDStateFunc)(DVDCommandBlock* block);

/* Issues a drive command the driver does not know itself; `callback` runs once it completes. */
typedef void (*DVDOptionalCommandChecker)(DVDCommandBlock* block, DVDLowCallback callback);

/* The low-memory copy of the boot partition the loader chose (zero offset when it chose none). */
#define DVD_BOOT_PARTITION_TYPE (*(u32*)0x80003194)
#define DVD_BOOT_PARTITION_OFFSET (*(u32*)0x80003198)

/* The disc header's second boot block, read at word offset 0x108: where the FST lives on the disc. */
typedef struct DVDBB2 {
    /* +0x00 */ u32 bootFilePosition;
    /* +0x04 */ u32 FSTPosition; /* disc offset of the FST, in the layout's address unit */
    /* +0x08 */ u32 FSTLength;   /* FST byte count, in the layout's address unit */
    /* +0x0C */ u32 FSTMaxLength;
    /* +0x10 */ u32 unused_0x10[4];
} DVDBB2; /* size: 0x20 */

/* One entry of the disc's partition table: where the partition starts and what it holds. */
typedef struct DVDPartitionInfo {
    /* +0x00 */ u32 offset; /* word offset of the partition */
    /* +0x04 */ u32 type;   /* 0 for the game partition */
} DVDPartitionInfo; /* size: 0x8 */

/* What a partition-parameter command (41, 42) reads and opens a partition with; the caller owns it. */
typedef struct DVDPartitionParams {
    /* +0x00000 */ ESTicket ticket;
    /* +0x002A4 */ u8 pad_0x2A4[0x1C];
    /* +0x002C0 */ ESTicketView ticketView;
    /* +0x00398 */ u8 pad_0x398[0x8];
    /* +0x003A0 */ u32 numTmdBytes; /* zero until the sizes are read */
    /* +0x003A4 */ u8 pad_0x3A4[0x1C];
    /* +0x003C0 */ u8 tmd[0x4A00];
    /* +0x04DC0 */ u32 numCertBytes; /* zero until the sizes are read */
    /* +0x04DC4 */ u8 pad_0x4DC4[0x1C];
    /* +0x04DE0 */ u8 certs[0x1000];
    /* +0x05DE0 */ u32 dataWordOffset;
    /* +0x05DE4 */ u8 pad_0x5DE4[0x1C];
    /* +0x05E00 */ u8 h3Table[0x18000];
} DVDPartitionParams; /* size: 0x1DE00 (approximation: the h3 table is sized to the Wii H3 table, the code reads only its start) */

/* The disc's partition-table header, read at word offset 0x10000. */
typedef struct DVDGameTOC {
    /* +0x00 */ u32 numGamePartitions; /* entries in the partition table */
    /* +0x04 */ u32 unused_0x04[7];
} DVDGameTOC; /* size: 0x20 */

void defaultOptionalCommandChecker(DVDCommandBlock* block, DVDLowCallback callback);

/* .data / .sdata */
const char* __DVDVersion = "<< RVL_SDK - DVD \trelease build: Feb 27 2009 10:01:59 (0x4302_145) >>";
static BOOL autoInvalidation = TRUE;
static DVDOptionalCommandChecker checkOptionalCommand = defaultOptionalCommandChecker;

/* .bss (emitted in first-use order) */
static ESTicketView __DVDTicketViewBuffer __attribute__((aligned(32)));
static u8 __DVDTmdBuffer[0x4A00] __attribute__((aligned(64)));
static DVDBB2 BB2 __attribute__((aligned(32)));
static DVDDiskID CurrDiskID __attribute__((aligned(32)));
static DVDGameTOC GameToc __attribute__((aligned(32)));
static DVDPartitionInfo PartInfo[4] __attribute__((aligned(32)));
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
static u32 __DVDNumTmdBytes __attribute__((aligned(32)));
static DVDGameTOC* PartitionInfo;
static DVDPartitionInfo* PartInfoCursor;
static DVDPartitionInfo* GamePartition;
static volatile BOOL PrepareResetDone;
static volatile u32 LowSyncResult;
static BOOL DVDInitialized;
volatile s32 __DVDLayoutFormat;
static volatile BOOL PreparingCover;
static volatile BOOL MediaChanged;
static volatile BOOL MotorStopped;
static volatile BOOL WaitingForCoverClose;
static volatile BOOL WaitingForCoverOpen;
static volatile BOOL BreakRequested;
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
void cbForUnrecoveredError(u32 intType);
void cbForStoreErrorCode3(s32 result, DVDCommandBlock* block);
static void cbForStoreErrorCode1(s32 result, DVDCommandBlock* block);
void cbForStoreErrorCode2(s32 result, DVDCommandBlock* block);
void cbForCancelSync(s32 result, DVDCommandBlock* block);
void ResetAlarmHandler(OSAlarm* alarm, OSContext* context);
void CoverAlarmHandler(OSAlarm* alarm, OSContext* context);
void stateReadingFST(void);
void cbForStateReadingFST(u32 intType);
void cbForStateReadingTOC(u32 intType);
void cbForStateReadingPartitionInfo(u32 intType);
void cbForStateOpenPartition(u32 intType);
void cbForStateOpenPartition2(u32 intType);
void cbForStateMotorStopped(u32 intType);
void cbForStateCheckID2(u32 intType);
void stateCoverClosed_CMD(void);
void cbForStateBusy(u32 intType);
void cbForStateCoverClosed(u32 intType);

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

/* Stores the interrupt type that ended the newest command in the error record. */
static inline void StampIntType(u32 intType)
{
    BOOL enabled = OSDisableInterrupts();

    if (CommandInfoCounter == 0) {
        __ErrorInfo.lastCommand[4].intType = intType;
    } else {
        __ErrorInfo.lastCommand[CommandInfoCounter - 1].intType = intType;
    }
    OSRestoreInterrupts(enabled);
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

/* Finishes the running command as cancelled when a cancel is pending; `resume` is where `stateReady` resumes. */
static inline BOOL CheckCancel(u32 resume)
{
    DVDCommandBlock* finished;

    if (Canceling) {
        ResumeFromHere = resume;
        Canceling = FALSE;
        finished = executing;
        executing = &DummyCommandBlock;
        finished->state = 10;
        if (finished->callback) {
            finished->callback(-3, finished);
        }
        if (CancelCallback) {
            CancelCallback(0, finished);
        }
        stateReady();
        return TRUE;
    }
    return FALSE;
}

/* Asks the drive for the error code behind a failed command. */
static inline void stateGettingError(void)
{
    StampCommand(0x27, 0, 0);
    DVDLowRequestError(cbForStateGettingError);
}

/* 0x804A64D0 (0x4): ignores a command the driver does not know. */
void defaultOptionalCommandChecker(DVDCommandBlock* block, DVDLowCallback callback)
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

/* 0x804A6640 (0x178): reads the new disc's FST to the address the boot block names. */
void stateReadingFST(void)
{
    LastState = (DVDStateFunc)stateReadingFST;
    if (bootInfo->FSTMaxLength < BB2.FSTLength) {
        OSPanic(__FILE__, 1090, "DVDChangeDisk(): FST in the new disc is too big.   ");
    }
    DVDLowClearCoverInterrupt(NULL);
    StampCommand(1, BB2.FSTPosition >> __DVDLayoutFormat, OSRoundUp32B(BB2.FSTLength << (~__DVDLayoutFormat & 2)));
    DVDLowRead(bootInfo->FSTLocation, OSRoundUp32B(BB2.FSTLength << (~__DVDLayoutFormat & 2)),
               BB2.FSTPosition >> __DVDLayoutFormat, cbForStateReadingFST);
}

/* 0x804A67C0 (0x184): finishes the disc change once the FST is read. */
void cbForStateReadingFST(u32 intType)
{
    DVDCommandBlock* finished;

    StampIntType(intType);
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
        __DVDFSInit();
        finished = executing;
        executing = &DummyCommandBlock;
        finished->state = 0;
        if (finished->callback) {
            finished->callback(0, finished);
        }
        stateReady();
        return;
    }
    StampCommand(0x27, 0, 0);
    DVDLowRequestError(cbForStateGettingError);
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

/* 0x804A6CA0 (0x504): reacts to the drive's error code: retries, waits for the cover, or stores the error. */
void cbForStateGettingError(u32 intType)
{
    u32 error;
    u32 status;
    u32 category;
    u32 resume;

    StampIntType(intType);
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
    error = DVDLowGetImmBufferReg();
    status = error & 0xFF000000;
    category = CategorizeError(error);
    if (category == 1) {
        __DVDStoreErrorCode(error, cbForStoreErrorCode1);
        return;
    }
    if (category == 2 || category == 3) {
        resume = 0;
    } else if (status == 0x01000000) {
        resume = 4;
    } else if (status == 0x02000000) {
        resume = 6;
    } else if (status == 0x03000000) {
        resume = 3;
    } else if (status == 0) {
        if (error == 0x53000) {
            resume = 1;
        } else {
            resume = 5;
        }
    } else {
        resume = 5;
    }
    if (CheckCancel(resume)) {
        return;
    }
    if (category == 2) {
        __DVDStoreErrorCode(error, cbForStoreErrorCode3);
        return;
    }
    if (category == 3) {
        if ((error & 0xFFFFFF) == 0x31100) {
            StampCommand(2, executing->offset, 0);
            DVDLowSeek(executing->offset, cbForUnrecoveredError);
        } else {
            LastState(executing);
        }
        return;
    }
    if (status == 0x01000000) {
        executing->state = 5;
        SetCoverAlarm();
        return;
    }
    if (status == 0x02000000) {
        executing->state = 3;
        stateCoverClosed();
        return;
    }
    if (status == 0x03000000) {
        executing->state = 4;
        SetCoverAlarm();
        return;
    }
    if (status == 0) {
        if (error == 0x53000) {
            StampCommand(0x10, 0, 0);
            DVDLowStopMotor(0, 0, cbForStateMotorStopped);
            return;
        }
        __DVDStoreErrorCode(0x1234567, cbForStoreErrorCode1);
        return;
    }
    __DVDStoreErrorCode(0x1234567, cbForStoreErrorCode1);
}

/* 0x804A71B0 (0x1E0): handles an unrecoverable command: stops the motor or reads the error. */
void cbForUnrecoveredError(u32 intType)
{
    StampIntType(intType);
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
    StampIntType(intType);
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

/* 0x804A7470 (0x240): waits for the cover to be opened once the motor has stopped for a retry. */
void cbForStateGoToRetry(u32 intType)
{
    StampIntType(intType);
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
    NumInternalRetry = 0;
    if (CurrCommand == 4 || CurrCommand == 5 || CurrCommand == 13 || CurrCommand == 33 || CurrCommand == 34 ||
        CurrCommand == 41 || CurrCommand == 42 || CurrCommand == 15 || CurrCommand == 37) {
        ResetRequired = TRUE;
    }
    if (CheckCancel(2)) {
        return;
    }
    executing->state = 11;
    SetCoverAlarm();
}

/* 0x804A76B0 (0x338): checks the disc id just read against the expected one, then reads the partition table. */
void stateCheckID(void)
{
    switch (CurrCommand) {
    case 3:
        MediaChanged = FALSE;
        if (DVDCompareDiskID(&CurrDiskID, executing->id)) {
            memcpy(IDShouldBe, &CurrDiskID, sizeof(DVDDiskID));
            executing->state = 1;
            DCInvalidateRange(&BB2, sizeof(DVDBB2));
            NumInternalRetry = 0;
            DVDLowClearCoverInterrupt(NULL);
            StampCommand(0x21, 0x10000, sizeof(DVDGameTOC));
            DVDLowUnencryptedRead(&GameToc, sizeof(DVDGameTOC), 0x10000, cbForStateReadingTOC);
        } else {
            StampCommand(0x10, 0, 0);
            DVDLowStopMotor(0, 0, cbForStateMotorStopped);
        }
        break;
    default:
        if (memcmp(&CurrDiskID, IDShouldBe, sizeof(DVDDiskID))) {
            StampCommand(0x10, 0, 0);
            DVDLowStopMotor(0, 0, cbForStateMotorStopped);
        } else {
            NumInternalRetry = 0;
            DVDLowClearCoverInterrupt(NULL);
            StampCommand(0x21, 0x10000, sizeof(DVDGameTOC));
            DVDLowUnencryptedRead(&GameToc, sizeof(DVDGameTOC), 0x10000, cbForStateReadingTOC);
        }
        break;
    }
}

/* 0x804A79F0 (0x1A0): reads the partition table once its header is read. */
void cbForStateReadingTOC(u32 intType)
{
    StampIntType(intType);
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
        PartitionInfo = &GameToc;
        DVDLowClearCoverInterrupt(NULL);
        StampCommand(0x21, 0x10008, sizeof(PartInfo));
        DVDLowUnencryptedRead(PartInfo, sizeof(PartInfo), 0x10008, cbForStateReadingPartitionInfo);
        return;
    }
    stateGettingError();
}

/* 0x804A7B90 (0x47C): finds the game partition in the table just read and opens it. */
void cbForStateReadingPartitionInfo(u32 intType)
{
    s16 i;

    StampIntType(intType);
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
        PartInfoCursor = PartInfo;
        GamePartition = NULL;
        if (DVD_BOOT_PARTITION_OFFSET) {
            GamePartition = PartInfoCursor;
            GamePartition->type = DVD_BOOT_PARTITION_TYPE;
            GamePartition->offset = DVD_BOOT_PARTITION_OFFSET;
        } else {
            for (i = 0; i < PartitionInfo->numGamePartitions; i++) {
                if (PartInfoCursor->type == DVD_BOOT_PARTITION_TYPE) {
                    GamePartition = PartInfoCursor;
                }
                PartInfoCursor++;
            }
        }
        if (GamePartition) {
            switch (CurrCommand) {
            case 3:
                NumInternalRetry = 0;
                DVDLowClearCoverInterrupt(NULL);
                StampCommand(0x22, GamePartition->offset, 0);
                if (DVD_DISC_APP_TYPE == 0x80) {
                    DVDLowOpenPartitionWithTmdAndTicketView(GamePartition->offset, &__DVDTicketViewBuffer, __DVDNumTmdBytes,
                                                            __DVDTmdBuffer, 0, NULL, cbForStateOpenPartition);
                } else {
                    DVDLowOpenPartition(GamePartition->offset, NULL, 0, NULL, __DVDTmdBuffer, cbForStateOpenPartition);
                }
                break;
            default:
                NumInternalRetry = 0;
                DVDLowClearCoverInterrupt(NULL);
                StampCommand(0x22, GamePartition->offset, 0);
                if (DVD_DISC_APP_TYPE == 0x80) {
                    DVDLowOpenPartitionWithTmdAndTicketView(GamePartition->offset, &__DVDTicketViewBuffer, __DVDNumTmdBytes,
                                                            __DVDTmdBuffer, 0, NULL, cbForStateOpenPartition2);
                } else {
                    DVDLowOpenPartition(GamePartition->offset, NULL, 0, NULL, __DVDTmdBuffer, cbForStateOpenPartition2);
                }
                break;
            }
        } else {
            if (CheckCancel(1)) {
                return;
            }
            executing->state = 6;
            SetCoverAlarm();
        }
        return;
    }
    stateGettingError();
}

/* 0x804A8010 (0x188): reads the boot block once the partition of a changed disc is open. */
void cbForStateOpenPartition(u32 intType)
{
    StampIntType(intType);
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
        DVDLowClearCoverInterrupt(NULL);
        StampCommand(1, 0x108, sizeof(DVDBB2));
        DVDLowRead(&BB2, sizeof(DVDBB2), 0x108, cbForStateCheckID2);
        return;
    }
    stateGettingError();
}

/* 0x804A81A0 (0x164): restarts the interrupted command once the partition is open again. */
void cbForStateOpenPartition2(u32 intType)
{
    StampIntType(intType);
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
        if (CheckCancel(0)) {
            return;
        }
        executing->state = 1;
        stateBusy(executing);
        return;
    }
    stateGettingError();
}

/* 0x804A8310 (0x1C8): waits for the cover to be opened once the motor has stopped for a wrong disc. */
void cbForStateMotorStopped(u32 intType)
{
    StampIntType(intType);
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
    NumInternalRetry = 0;
    if (CheckCancel(1)) {
        return;
    }
    executing->state = 6;
    SetCoverAlarm();
}

/* 0x804A84E0 (0x194): reads the new disc's FST once its boot block is read. */
void cbForStateCheckID2(u32 intType)
{
    StampIntType(intType);
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
        stateReadingFST();
        return;
    }
    stateGettingError();
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

/* 0x804A8750 (0x250): lowers the rotation speed of a drive that needs it, else re-reads the disc id after a reset. */
void ResetAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    if (DVD_DEVICE_CODE == 0x8003) {
        LastState = (DVDStateFunc)stateDownRotation;
        DVDLowClearCoverInterrupt(NULL);
        StampCommand(0x25, 0, 0);
        DVDLowSetMaximumRotation(0x20000, cbForStateDownRotation);
    } else {
        DCInvalidateRange(&CurrDiskID, sizeof(DVDDiskID));
        LastState = (DVDStateFunc)stateCoverClosed_CMD;
        if (CurrCommand == 40) {
            NumInternalRetry = 0;
            if (CheckCancel(0)) {
                return;
            }
            executing->state = 1;
            stateBusy(executing);
            return;
        }
        DVDLowClearCoverInterrupt(NULL);
        StampCommand(5, 0, sizeof(DVDDiskID));
        DVDLowReadDiskID(&CurrDiskID, cbForStateCoverClosed);
    }
}

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
    stateGettingError();
}

/* 0x804A8A90 (0xC8): lowers the drive's rotation speed. */
void stateDownRotation(DVDCommandBlock* block)
{
    DVDLowClearCoverInterrupt(NULL);
    StampCommand(0x25, 0, 0);
    DVDLowSetMaximumRotation(0x20000, cbForStateDownRotation);
}

/* 0x804A8B60 (0x240): re-reads the disc id once the rotation speed is lowered. */
void cbForStateDownRotation(u32 intType)
{
    StampIntType(intType);
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    if (intType & 1) {
        DCInvalidateRange(&CurrDiskID, sizeof(DVDDiskID));
        LastState = (DVDStateFunc)stateCoverClosed_CMD;
        if (CurrCommand == 40) {
            NumInternalRetry = 0;
            if (CheckCancel(0)) {
                return;
            }
            executing->state = 1;
            stateBusy(executing);
            return;
        }
        DVDLowClearCoverInterrupt(NULL);
        StampCommand(5, 0, sizeof(DVDDiskID));
        DVDLowReadDiskID(&CurrDiskID, cbForStateCoverClosed);
        return;
    }
    stateGettingError();
}

/* 0x804A8DA0 (0x178): re-reads the disc id, or restarts a command that needs no id check. */
void stateCoverClosed_CMD(void)
{
    if (CurrCommand == 40) {
        NumInternalRetry = 0;
        if (CheckCancel(0)) {
            return;
        }
        executing->state = 1;
        stateBusy(executing);
        return;
    }
    DVDLowClearCoverInterrupt(NULL);
    StampCommand(5, 0, sizeof(DVDDiskID));
    DVDLowReadDiskID(&CurrDiskID, cbForStateCoverClosed);
}

/* 0x804A8F20 (0xD8): checks the disc id once the cover has closed. */
void cbForStateCoverClosed(u32 intType)
{
    StampIntType(intType);
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
    stateGettingError();
}

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

/* Finishes the running command successfully and starts the next one. */
static inline void FinishCommand(void)
{
    DVDCommandBlock* finished;

    finished = executing;
    executing = &DummyCommandBlock;
    finished->state = 0;
    if (finished->callback) {
        finished->callback(0, finished);
    }
    stateReady();
}

/* 0x804A94E0 (0x5B0): issues the drive command `block` carries. */
void stateBusy(DVDCommandBlock* block)
{
    u32 size;
    DVDPartitionParams* params;
    DVDPartitionParams* openParams;

    LastState = stateBusy;
    switch (block->command) {
    case 2:
    case 3:
    case 5:
    case 13:
    case 14:
    case 15:
    case 16:
    case 34:
    case 37:
    case 41:
    case 42:
        StampCommand(block->command, block->offset, block->length);
        break;
    }
    switch (block->command) {
    case 5:
        DVDLowClearCoverInterrupt(NULL);
        block->currTransferSize = sizeof(DVDDiskID);
        DVDLowReadDiskID(block->addr, cbForStateBusy);
        break;
    case 1:
    case 4:
        if (block->length == 0) {
            FinishCommand();
        } else {
            DVDLowClearCoverInterrupt(NULL);
            size = block->length - block->transferredSize;
            if (size > 0x80000) {
                size = 0x80000;
            }
            block->currTransferSize = size;
            StampCommand(block->command, block->offset + (block->transferredSize >> 2), size);
            DVDLowRead((u8*)block->addr + block->transferredSize, block->currTransferSize,
                       block->offset + (block->transferredSize >> 2), cbForStateBusy);
        }
        break;
    case 2:
        DVDLowClearCoverInterrupt(NULL);
        DVDLowSeek(block->offset, cbForStateBusy);
        break;
    case 3:
        DVDLowStopMotor(0, 0, cbForStateBusy);
        break;
    case 15:
        DVDLowStopMotor(0, 0, cbForStateBusy);
        break;
    case 13:
        DVDLowClearCoverInterrupt(NULL);
        DVDLowAudioBufferConfig(block->offset, block->length, cbForStateBusy);
        break;
    case 14:
        DVDLowClearCoverInterrupt(NULL);
        block->currTransferSize = sizeof(DVDDriveInfo);
        DVDLowInquiry(block->addr, cbForStateBusy);
        break;
    case 16:
        DVDLowClearCoverInterrupt(NULL);
        DVDLowStopMotor(0, 0, cbForStateBusy);
        break;
    case 32:
        DVDLowSetSpinupFlag(1);
        DVDLowReset(cbForStateBusy);
        break;
    case 33:
        if (block->length == 0) {
            FinishCommand();
        } else {
            DVDLowClearCoverInterrupt(NULL);
            size = block->length - block->transferredSize;
            if (size > 0x80000) {
                size = 0x80000;
            }
            block->currTransferSize = size;
            StampCommand(block->command, block->offset + (block->transferredSize >> 2), size);
            DVDLowUnencryptedRead((u8*)block->addr + block->transferredSize, block->currTransferSize,
                                  block->offset + (block->transferredSize >> 2), cbForStateBusy);
        }
        break;
    case 34:
        DVDLowClearCoverInterrupt(NULL);
        DVDLowOpenPartition(block->offset, NULL, 0, NULL, block->addr, cbForStateBusy);
        break;
    case 35:
        DVDLowClearCoverInterrupt(NULL);
        DVDLowClosePartition(cbForStateBusy);
        break;
    case 38:
        DVDLowPrepareCoverRegister(cbForStateBusy);
        break;
    case 36:
        DVDLowPrepareCoverRegister(cbForStateBusy);
        break;
    case 37:
        DVDLowClearCoverInterrupt(NULL);
        DVDLowSetMaximumRotation(0x20000, cbForStateBusy);
        break;
    case 40:
        DVDLowClearCoverInterrupt(NULL);
        block->addr = &CurrDiskID;
        block->currTransferSize = sizeof(DVDDiskID);
        DVDLowReadDiskID(&CurrDiskID, cbForStateBusy);
        break;
    case 41:
        DVDLowClearCoverInterrupt(NULL);
        params = block->addr;
        if (params->numTmdBytes == 0 && params->numCertBytes == 0) {
            DVDLowGetNoDiscBufferSizes(block->offset, &params->numTmdBytes, &params->numCertBytes, cbForStateBusy);
        } else {
            DVDLowGetNoDiscOpenPartitionParams(block->offset, &params->ticket, &params->numTmdBytes, params->tmd,
                                               &params->numCertBytes, params->certs, &params->dataWordOffset,
                                               params->h3Table, cbForStateBusy);
        }
        break;
    case 42:
        DVDLowClearCoverInterrupt(NULL);
        openParams = block->addr;
        DVDLowOpenPartitionWithTmdAndTicketView(block->offset, &openParams->ticketView, openParams->numTmdBytes,
                                                openParams->tmd, openParams->numCertBytes, openParams->certs,
                                                cbForStateBusy);
        break;
    default:
        checkOptionalCommand(block, cbForStateBusy);
        break;
    }
}

/* Vendor drive commands that return their result in the immediate buffer register (none in this build). */
static u32 ImmCommandList[] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

/* Vendor drive commands that transfer data by DMA (none in this build). */
static u32 DmaCommandList[] = {0xFFFFFFFF};

/* Whether `command` transfers data by DMA (its byte count advances the block's transferred size). */
static inline BOOL IsDmaCommand(u32 command)
{
    u32 i;

    if (command == 1 || command == 4 || command == 5 || command == 33 || command == 14) {
        return TRUE;
    }
    for (i = 0; i < sizeof(DmaCommandList) / sizeof(DmaCommandList[0]); i++) {
        if (command == DmaCommandList[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Whether `command` returns its result in the immediate buffer register. */
static inline BOOL IsImmCommandWithResult(u32 command)
{
    u32 i;

    if (command == 9 || command == 10 || command == 11 || command == 12) {
        return TRUE;
    }
    for (i = 0; i < sizeof(ImmCommandList) / sizeof(ImmCommandList[0]); i++) {
        if (command == ImmCommandList[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

/* 0x804A9A90 (0x9E0): completes, continues or retries the running command once the drive answers. */
void cbForStateBusy(u32 intType)
{
    DVDCommandBlock* finished;
    u32 result;
    u32 reg;

    StampIntType(intType);
    if (intType == 16) {
        __DVDStoreErrorCode(0x1234568, cbForStoreErrorCode2);
        return;
    }
    if (intType == 32) {
        __DVDStoreErrorCode(0x1234569, cbForStoreErrorCode2);
        return;
    }
    if (CurrCommand == 3 || CurrCommand == 15) {
        if (intType & 2) {
            __DVDStoreErrorCode(0x1234567, cbForStoreErrorCode1);
            return;
        }
        NumInternalRetry = 0;
        if (CurrCommand == 15) {
            ResetRequired = TRUE;
        }
        if (CheckCancel(7)) {
            return;
        }
        if (MotorState != 2) {
            executing->state = 7;
            SetCoverAlarm();
        }
        return;
    }
    if (IsDmaCommand(CurrCommand)) {
        executing->transferredSize += (intType & 9) ? executing->currTransferSize : 0;
    }
    if (BreakRequested) {
        finished = executing;
        BreakRequested = FALSE;
        Canceling = FALSE;
        executing = &DummyCommandBlock;
        finished->state = 10;
        if (finished->callback) {
            finished->callback(-3, finished);
        }
        if (CancelCallback) {
            CancelCallback(0, finished);
        }
        stateReady();
        return;
    }
    if (intType & 1) {
        NumInternalRetry = 0;
        if (CurrCommand == 16) {
            if (executing->offset) {
                MotorState = 2;
            } else {
                MotorState = 1;
            }
            finished = executing;
            executing = &DummyCommandBlock;
            finished->state = 0;
            if (finished->callback) {
                finished->callback(0, finished);
            }
            stateReady();
            return;
        }
        if (CurrCommand == 32) {
            LastResetEnd = __OSGetSystemTime();
            ResetRequired = FALSE;
            ResumeFromHere = 0;
            finished = executing;
            executing = &DummyCommandBlock;
            finished->state = 0;
            if (finished->callback) {
                finished->callback(0, finished);
            }
            stateReady();
            return;
        }
        if (CheckCancel(0)) {
            return;
        }
        if (CurrCommand == 38) {
            reg = DVDLowGetCoverRegister();
            if (__OSGetSystemTime() - LastResetEnd < DVD_MILLISECONDS_TO_TICKS(100)) {
                result = 0;
            } else if (reg & 1) {
                result = 1;
            } else {
                result = 2;
            }
            finished = executing;
            executing = &DummyCommandBlock;
            finished->state = 0;
            finished->offset = result;
            if (finished->callback) {
                finished->callback(result, finished);
            }
            stateReady();
            return;
        }
        if (CurrCommand == 36) {
            reg = DVDLowGetCoverRegister();
            if (((reg >> 2) & 1) || (reg & 1)) {
                result = 0;
            } else if (ResumeFromHere) {
                result = 0;
            } else {
                result = 1;
            }
            finished = executing;
            executing = &DummyCommandBlock;
            finished->state = 0;
            finished->offset = result;
            if (finished->callback) {
                finished->callback(result, finished);
            }
            stateReady();
            return;
        }
        if (CurrCommand == 40) {
            if (DVDCompareDiskID(&CurrDiskID, executing->id)) {
                memcpy(IDShouldBe, &CurrDiskID, sizeof(DVDDiskID));
                finished = executing;
                executing = &DummyCommandBlock;
                finished->state = 0;
                finished->offset = 1;
                if (finished->callback) {
                    finished->callback(1, finished);
                }
                NumInternalRetry = 0;
                stateReady();
            } else {
                StampCommand(0x10, 0, 0);
                DVDLowStopMotor(0, 0, cbForStateMotorStopped);
            }
            return;
        }
        if (CurrCommand == 41) {
            if (((DVDPartitionParams*)executing->addr)->dataWordOffset == 0) {
                stateBusy(executing);
            } else {
                finished = executing;
                executing = &DummyCommandBlock;
                finished->state = 0;
                if (finished->callback) {
                    finished->callback(0, finished);
                }
                stateReady();
            }
            return;
        }
        if (IsDmaCommand(CurrCommand)) {
            if (executing->transferredSize != executing->length) {
                stateBusy(executing);
                return;
            }
            finished = executing;
            executing = &DummyCommandBlock;
            finished->state = 0;
            if (finished->callback) {
                finished->callback(finished->transferredSize, finished);
            }
            stateReady();
            return;
        }
        if (IsImmCommandWithResult(CurrCommand)) {
            if (CurrCommand == 11 || CurrCommand == 10) {
                result = DVDLowGetImmBufferReg() << 2;
            } else {
                result = DVDLowGetImmBufferReg();
            }
            finished = executing;
            executing = &DummyCommandBlock;
            finished->state = 0;
            if (finished->callback) {
                finished->callback(result, finished);
            }
            stateReady();
            return;
        }
        finished = executing;
        executing = &DummyCommandBlock;
        finished->state = 0;
        if (finished->callback) {
            finished->callback(0, finished);
        }
        stateReady();
        return;
    }
    if (CurrCommand == 14) {
        __DVDStoreErrorCode(0x1234567, cbForStoreErrorCode1);
        return;
    }
    if ((CurrCommand == 1 || CurrCommand == 4 || CurrCommand == 5 || CurrCommand == 33 || CurrCommand == 14) &&
        executing->transferredSize == executing->length) {
        if (CheckCancel(0)) {
            return;
        }
        finished = executing;
        executing = &DummyCommandBlock;
        finished->state = 0;
        if (finished->callback) {
            finished->callback(finished->transferredSize, finished);
        }
        stateReady();
        return;
    }
    stateGettingError();
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

/* 0x804AA810 (0x364): cancels `block` in whatever state it is; `callback` runs once the cancel completes. */
BOOL DVDCancelAsync(DVDCommandBlock* block, DVDCBCallback callback)
{
    BOOL enabled;
    volatile DVDCommandBlock* vblock = block;

    enabled = OSDisableInterrupts();
    switch (block->state) {
    case -1:
    case 0:
    case 10:
        if (callback) {
            callback(0, block);
        }
        break;
    case 1:
        if (Canceling) {
            OSRestoreInterrupts(enabled);
            return FALSE;
        }
        Canceling = TRUE;
        CancelCallback = callback;
        if (block->command == 4 || block->command == 33 || block->command == 34 || block->command == 41 ||
            block->command == 42 || block->command == 1) {
            BreakRequested = TRUE;
        }
        break;
    case 2:
        __DVDDequeueWaitingQueue(block);
        block->state = 10;
        if (block->callback) {
            block->callback(-3, block);
        }
        if (callback) {
            callback(0, block);
        }
        break;
    case 3:
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
            if (callback) {
                callback(0, block);
            }
            break;
        case 1:
        case 2:
            if (__OSInIPL) {
                executing = &DummyCommandBlock;
                block->state = 10;
                if (block->callback) {
                    block->callback(-3, block);
                }
                if (callback) {
                    callback(0, block);
                }
                stateReady();
                break;
            }
        default:
            if (Canceling) {
                OSRestoreInterrupts(enabled);
                return FALSE;
            }
            Canceling = TRUE;
            CancelCallback = callback;
            break;
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 11:
        if (!WaitingForCoverClose && !WaitingForCoverOpen) {
            OSRestoreInterrupts(enabled);
            return FALSE;
        }
        if (WaitingForCoverOpen) {
            OSCancelAlarm(&CoverAlarm);
            WaitingForCoverOpen = FALSE;
        }
        if (vblock->state == 4) {
            ResumeFromHere = 3;
        }
        if (vblock->state == 5) {
            ResumeFromHere = 4;
        }
        if (vblock->state == 6) {
            ResumeFromHere = 1;
        }
        if (vblock->state == 11) {
            ResumeFromHere = 2;
        }
        if (vblock->state == 7) {
            ResumeFromHere = 7;
        }
        executing = &DummyCommandBlock;
        block->state = 10;
        if (block->callback) {
            block->callback(-3, block);
        }
        if (callback) {
            callback(0, block);
        }
        stateReady();
        break;
    case 12:
        executing = &DummyCommandBlock;
        block->state = 10;
        if (block->callback) {
            block->callback(-3, block);
        }
        if (callback) {
            callback(0, block);
        }
        stateReady();
        break;
    }
    OSRestoreInterrupts(enabled);
    return TRUE;
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
    for (;; OSSleepThread(&__DVDThreadQueue)) {
        state = block->state;
        if (state == 0 || state == -1 || state == 10) {
            break;
        }
        if (state != 3) {
            continue;
        }
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
            continue;
        }
        break;
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
