/* WPAD/wpad.cpp - the Wii remote driver: control block, connect/disconnect, speaker, stream data and the report queue.
 * RANGE. .text 0x804EB510-0x804F9B30 (131 functions); .rodata 0x80573C80-0x80573CD8; .data 0x8062BF30-0x8062DBF0;
 *   .bss 0x8075B2A0-0x8075EAA0; .sdata 0x80794198-0x807941C8; .sbss 0x807956A8-0x80795720; .sdata2 0x8079D3C8-0x8079D478.
 *   Edges: the "<< RVL_SDK - WPAD ... Jun 22 2009 ... (0x4302_145) >>" build string (.data 0x8062BF30) and the
 *   `WBCReadDummy`/`WBCSetZEROPointDummy`/`WBCGetTGCWeightDummy` print strings follow it before any other string, so
 *   the nine 8-byte stubs and the three dummies at 0x804EB510..0x804EB630 belong to this TU; the right edge 0x804F9B30
 *   is the first reader of the WUD state (.sbss 0x80795738, .bss 0x8075EAA0).
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment): every function start is 16-aligned.  `#pragma dont_inline`
 *   brackets the memory read/write helpers at the end: in retail their callees are out-of-line (see RESIDUALS, two TUs).
 * NAMES. the map's names (`WPADInit`, `WPADDisconnect`, `WPADProbe`, `WPADControlSpeaker`, `WPADSendStreamData`,
 *   `WPADiSendWriteData`, `WPADiSendSetReportType`, `WPADiClearQueue`, `WPADWriteExtReg`, ...).  GUESS names: `WPADiDebugPrint`,
 *   `WPADiNullCallbackA/B`, `WPADiGetReserved0..3`, `WPADiReturnZeroA..C`, `WBCReadDummy`, `WBCSetZEROPointDummy`,
 *   `WBCGetTGCWeightDummy` (the last three from their print strings); public-API-shaped guesses `WPADStartSyncDevice`,
 *   `WPADStartClearDevice`, `WPADSetSyncDeviceCallback`, `WPADSetClearDeviceCallback`, `WPADRegisterAllocator`, `WPADGetStatus`,
 *   `WPADGetSensorBarPosition`, `WPADGetAccGravityUnit`, `WPADSetAutoSleepTime`, `WPADGetDataFormat`, `WPADSetDataFormat`,
 *   `WPADGetDpdSensitivity`, `WPADIsDpdEnabled`, `WPADSetAutoSamplingBuf` (the thunks forward to the WUD functions whose log
 *   strings name them; the others from the field they read or write); driver internals `wpadShutdownCallback`, `wpadSendCommand`,
 *   `wpadUpdateRadioSensitivity`, `wpadCountIdleReports`, `wpadPollSample`, `wpadAlarmHandler`, `wpadResetChannel`, `wpadStart`,
 *   `wpadInitDoneCallback`, `wpadAbortCallback`, `wpadHidDataCallback`, `wpadAssignChannel`, `wpadHidOpenCallback`,
 *   `wpadCloseLinkCallback`, `wpadInfoCallback`, `wpadApplyDpdFormat`, `wpadSendWriteByte`, `wpadSendReadMemory`, `wpadQueueHasRoom`,
 *   `wpadWriteMemory`, `wpadReadMemory`, `wpadReadMemoryGuarded`, `wpadBulkDoneCallback`, `wpadCopyCurrentSample`,
 *   `wpadFilterButtons`, `wpadStoreSample`, `wpadReportButtons`, `wpadReportButtonsAccel`, `wpadDecryptExtension`,
 *   `wpadGetDpdCalibration`, `wpadSetDpdStatusA/B`, `wpadResetSpeakerState`, `wpadGetAppType`, `wpadGetGameName`,
 *   `wpadDispatchReport`, `wpadReportStatus/ReadData/Ack/Ignored`, `wpadReportButtonsExt8`, `wpadReportButtonsAccelDpd12`,
 *   `wpadReportExt19`, `wpadReportButtonsAccelExt16`, `wpadReportButtonsDpd10Ext9`, `wpadReportButtonsAccelDpd10Ext6`,
 *   `wpadReportExt21`, `wpadReportInterleavedA/B` (the report ids are 0x20..0x3F; the names read the payload layout),
 *   `wpadDecodeDpd`, `wpadDecodeDpdInterleaved`, `wpadDecodeExtDualStick`, `wpadDecodeExtBoard` (the extension layouts are
 *   GUESSes from the decode: Nunchuk-style, Classic-style, a byte-stick type and a balance-board-like 21-byte type),
 *   `wpadExtInitCallback`, `wpadDpdCalibrationReadDone`, `wpadSendWbcCommand`, `wpadBulkWrite`, `wpadBulkWriteNext`,
 *   `wpadBulkWriteFirst`, `wpadCopyTitleString` (the title string copy); named but unwritten: `wpadSampleChanged` (0x804EBAF0),
 *   `wpadSamplingFiber` (0x804EC650), `wpadInitSequence` (0x804ED880), `wpadAccCalibrationReadDone` (0x804F3150),
 *   `wpadExtCalibrationReadDone` (0x804F3C40), `wpadExtIdReadDone` (0x804F4180).  GUESS names `wpadStickDeadzoneSquare`, `wpadStickDeadzoneCircle`
 *   (the dead zone of a stick pair on 8-bit or 16-bit axes) and `wpadClampAxes` (the accelerometer axes against a full-scale limit).
 *   `fn_` names remain on the 0x804F14A0,
 *   0x804F72C0, 0x804F79B0, 0x804F8380 and 0x804F84E0 rows.  Struct fields
 *   are named from use; a `unused_0xNN` field is written but never read by a written body.  Statics are named from use; the
 *   `.sbss` rows `wpadChanStateA/B` and the `.bss` rows at 0x8075EA00.. are not understood yet.  Foreign names given here:
 *   WUD (`WUDInit`, `WUDShutdown`, `WUDGetStatus`, `WUDRegisterAllocator`, `WUDSet*Callback`, `WUDStart*`, `WUDStop*`,
 *   `WUDSetHidRecvCallback`, `WUDSetHidConnCallback`, `WUDIsSyncing`, `WUDIsHistoryAddr`, from the `WUD...()` log strings
 *   where one exists), SC (`SCGetWpadMotorMode`, `SCGetWpadSensorBarPosition`, `SCGetWpadSpeakerVolume`), `OSReturnToMenuPending`.
 * RESIDUALS. Written: 119 of 131 rows, 59 at 100 %; 12 rows are not attempted (0x804EBAF0, 0x804EC650, 0x804ED880, 0x804EF900
 *   `WPADControlSpeaker`, 0x804F14A0, 0x804F3150, 0x804F3C40, 0x804F4180, 0x804F72C0, 0x804F79B0, 0x804F8380, 0x804F84E0).
 *   `wpadStickDeadzoneSquare` keeps r31 for the sign copy where the target does not, and `wpadClampAxes` differs in the float register
 *   numbering and in the .sdata2 constant names.  The new report decoders re-read the sample pointer per statement in the original
 *   (`wpadDecodeExtBoard` 2 %, `wpadDecodeDpdInterleaved` 53 %: the source's statement order differs and the original keeps no
 *   stack frame); `wpadReportReadData`, `wpadReportStatus` and `wpadBulkWrite*` differ in register numbering and one reload.
 *   Register-numbering differences only (the variable order) remain in `WPADDisconnect`, `wpadAssignChannel`,
 *   `wpadHidOpenCallback`, `wpadUpdateRadioSensitivity`, `wpadReportButtonsAccel` (the original reloads the control block
 *   pointer) and `wpadDecryptExtension` (loop unrolling); `WPADSetAutoSamplingBuf` strength-reduces the index multiply the
 *   original keeps.  The queue push, clear and read builders are inline helpers; the original shares their bodies with the
 *   out-of-line copies.  Flip blockers: .text (15 unwritten rows), .data (the strings and jump tables of the unwritten bodies; the
 *   report handler table is emitted), .rodata, .sdata, .sbss, .bss (ours is 0x10 over the target: the 0x8075EA00 float arrays
 *   and `wpadTitleBuf` are emitted, the cause of the extra 16 bytes is not found) and .sdata2 (the float pool the unwritten bodies and
 *   `wpadDecodeDpd` load).  Calls that the target makes out of line are bracketed by `#pragma dont_inline` in the written bodies
 *   (`wpadSendWriteByte`, `wpadSendReadMemory`, `wpadQueueHasRoom`, `wpadGetGameName`, `wpadGetAppType` are one TU away).  `WPADiDebugPrint` is an empty varargs body by design.  The TU is probably
 *   two: the .sdata2 pool holds the double 4330000080000000 twice (0x8079D418 read by 0x804F3150/0x804F5050/0x804F54F0,
 *   0x8079D460 read by 0x804F84E0/0x804F8C00/0x804F8D30) and the float 0 twice (0x8079D3E0, 0x8079D46C); the second TU starts
 *   after 0x804F5768 and no later than 0x804F8380, and `WPADWriteExtReg` (0x804F9A10), `wpadWriteMemory` and `wpadReadMemory`
 *   call `WPADiSendWriteData` (0x804F2B60) and `wpadSendReadMemory` (0x804F2D20) out of line, which one TU would have
 *   inlined (measured: inlined they are 556 B against the original's 204 B); the seam is not pinned and the unit is kept whole.
 * SHAPES. A command is a local `WPADCommand` passed by value to an inline push (`wpadQueuePush`): the 0x30-byte stack copy
 *   and the `memset` + `memcpy` into the ring are the by-value parameter; `u32 level = OSDisableInterrupts(); ...
 *   OSRestoreInterrupts(level);` brackets every field read; `-1 & ~((-a | a) >> 31)` is `a != 0 ? 0 : -1`.
 */

#include "types.h"
#include "WPAD/wpad.h"
#include "WUD/wud.h"
#include "BTE/gki_buffer.h"
#include "SC/SCGetWpadMotorMode.h"
#include "SC/sc.h"
#include "VI/vi.h"
#include "OS/OS.h"
#include "OS/OSAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSSetPeriodicAlarm.h"
#include "OS/OSSwitchFiberEx.h"
#include "OS/OSInitThreadQueue.h"
#include "OS/OSReset.h"
#include "OS/OSError.h"
#include "OS/OSTime.h"
#include "OS/__OSGetSystemTime.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"
#include "MSL/s_cos.h"
#include "MSL/sqrt.h"
#include "MSL/s_sin.h"

/* .bss */
static OSAlarm wpadAlarm;
static WPADCB* wpadCBTable[4];
static u8 wpadFiberStack[0x1000];
static s8 wpadHandleToChan[0x20];
static WPADCB wpadCBs[4];
static f32 wpadDpdCenterX[4];
static f32 wpadDpdCenterY[4];
static f32 wpadDpdPivotX[4];
static f32 wpadDpdPivotY[4];
static f32 wpadDpdRoll[4];
static u8 wpadExtRaw[0x18];
static u16 wpadTitleBuf[0x1C];

/* .sbss */
static u8 wpadShutdownRegistered;
static u16 wpadRadioTick;
static u8 wpadPollState;
static u16 wpadWifiCheckTick;
static u8 wpadChanStateA[4];
static u8 wpadChanStateB[4];
static s32 wpadInitialized;
static s32 wpadReconnectDelayMs;
static void (*wpadInitHook[4])(void);
static u32 wpadReserved0;
static u32 wpadReserved1;
static u32 wpadReserved2;
static u32 wpadReserved3;
static s32 wpadKpadOwnsCallbacks;
static s32 wpadStackStarted;
static u8 wpadChanAllocated[4];
static s8 wpadWifiChannel;
static u8 wpadShutdownRequested;
static u8 wpadWifiCheckEnabled;
static u8 wpadSpeakerVolume;
static s32 wpadMotorEnabled;
static u8 wpadSensorBarTop;
static u8 wpadDpdSensitivity;
static s8 wpadSleepMinutes;
static char* wpadGameName;
static s8 wpadAppType;
static u8 wpadInterleavedA[4];
static u8 wpadInterleavedB[4];
static u8 wpadExtInitRetries[4];
static u8 wpadSticksZeroed[4];

/* .rodata */
static const u8 wpadExtZero[0x18] = { 0 };

/* .sdata */
static const char* wpadVersionString = "<< RVL_SDK - WPAD \trelease build: Jun 22 2009 18:33:21 (0x4302_145) >>";
static s32 wpadReconnectMode = -1;

/* The shutdown record the OS list links. */
static OSShutdownFunctionInfo wpadShutdownInfo = { wpadShutdownCallback, 127, NULL, NULL };

/* Pushes `cmd` onto `queue`; FALSE when the ring is full. */
static inline BOOL wpadQueuePush(WPADCmdQueue* queue, WPADCommand cmd)
{
    u32 level = OSDisableInterrupts();
    s32 used;
    {
        u32 inner = OSDisableInterrupts();
        used = (s8)(queue->tail - queue->head);
        if (used < 0) {
            used += queue->capacity;
        }
        OSRestoreInterrupts(inner);
    }
    if (queue->capacity - 1 == (u32)used) {
        OSRestoreInterrupts(level);
        return FALSE;
    }
    memset(&queue->items[queue->tail], 0, sizeof(WPADCommand));
    memcpy(&queue->items[queue->tail], &cmd, sizeof(WPADCommand));
    queue->tail = (queue->tail == queue->capacity - 1) ? 0 : queue->tail + 1;
    OSRestoreInterrupts(level);
    return TRUE;
}

/* The null callback pair handed to the WUD layer when the stack starts. */
void* WPADiNullCallbackA(u32 size) /* untyped: caller-owned block */
{
    return NULL;
}

s32 WPADiNullCallbackB(void* block) /* untyped: the caller-owned block */
{
    return 0;
}

u32 WPADiGetReserved0(void)
{
    return wpadReserved0;
}

u32 WPADiGetReserved1(void)
{
    return wpadReserved1;
}

u32 WPADiGetReserved2(void)
{
    return wpadReserved2;
}

u32 WPADiGetReserved3(void)
{
    return wpadReserved3;
}

s32 WPADiReturnZeroA(void)
{
    return 0;
}

s32 WPADiReturnZeroB(void)
{
    return 0;
}

s32 WPADiReturnZeroC(void)
{
    return 0;
}

/* The balance board read stub: logs and fails. */
s32 WBCReadDummy(void)
{
    WPADiDebugPrint("WBCReadDummy\n");
    return -1;
}

s32 WBCSetZEROPointDummy(void)
{
    WPADiDebugPrint("WBCSetZEROPointDummy\n");
    return -1;
}

s32 WBCGetTGCWeightDummy(void)
{
    WPADiDebugPrint("WBCGetTGCWeightDummy\n");
    return -1;
}

/* Queues a read of `size` bytes from the remote address `addr` into `dest`. */
static inline s32 wpadQueueReadMemory(WPADCmdQueue* queue, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCommand cmd;
    u16 length = size;
    u32 address = addr;

    cmd.reportId = 0x17;
    cmd.length = 6;
    cmd.callback = callback;
    memcpy(&cmd.data[0], &address, 4);
    memcpy(&cmd.data[4], &length, 2);
    cmd.argB = length;
    cmd.argA = (u32)dest;
    cmd.argC = address;
    return wpadQueuePush(queue, cmd);
}

/* Reads the status word of channel `chan` with interrupts off. */
static inline s32 wpadGetStatus(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    OSRestoreInterrupts(level);
    return status;
}

/* The shutdown callback the OS runs: stops the stack and logs the delayed reconnect. */
BOOL wpadShutdownCallback(BOOL final, u32 event)
{
    s32 status = WUDGetStatus();
    BOOL stop;
    s32 i;
    u32 level;

    if (final != 0 || status == 0) {
        return TRUE;
    }
    if (status == 4 || (u32)(status - 1) <= 1) {
        return FALSE;
    }
    if (WUDIsSyncing() != 0) {
        WUDStopSyncDevice();
        return FALSE;
    }
    switch (event) {
    case 0:
        WUDRegisterAllocator(WPADiNullCallbackA, WPADiNullCallbackB);
    case 2:
    case 3:
        stop = TRUE;
        break;
    case 1:
    case 4:
    case 6:
        stop = FALSE;
        break;
    case 5:
        stop = OSReturnToMenuPending != 0;
        break;
    }
    if (stop) {
        level = OSDisableInterrupts();
        if (wpadShutdownRequested != 0) {
            OSRestoreInterrupts(level);
        } else {
            wpadShutdownRequested = 1;
            WUDSetVisibility(0, 0);
            for (i = 0; i < 4; i++) {
                WUDSetDeviceHistory(i, NULL);
            }
            OSCancelAlarm(&wpadAlarm);
            WUDSetHidRecvCallback(NULL);
            WUDShutdown(0);
            OSRestoreInterrupts(level);
        }
    } else {
        level = OSDisableInterrupts();
        if (wpadInitialized == 0) {
            OSRestoreInterrupts(level);
        } else if (wpadShutdownRequested != 0) {
            OSRestoreInterrupts(level);
        } else {
            wpadShutdownRequested = 1;
            wpadReconnectMode = 1;
            WPADiDebugPrint("Wait for %d ms until start reconnect!\n", wpadReconnectDelayMs);
            OSRestoreInterrupts(level);
        }
    }
    return FALSE;
}

/* Returns the sample the next report fills (the one not currently published). */
static inline WPADSample* wpadSpareSample(WPADCB* cb)
{
    return &cb->samples[(u8)(cb->sampleIndex == 0)];
}

/* Drops the Bluetooth link of the channel's device when it is connected. */
static inline void wpadDropLink(s32 chan)
{
    if (wpadGetStatus(chan) != -1) {
        WPADCB* cb = wpadCBTable[chan];
        u32 level = OSDisableInterrupts();
        u8 handle = cb->devHandle;
        const u8* addr;
        u8 bdAddr[6];
        OSRestoreInterrupts(level);
        addr = _WUDGetDevAddr(handle);
        if (addr != NULL) {
            memcpy(bdAddr, addr, 6);
        } else {
            memset(bdAddr, 0, 6);
        }
        btm_remove_acl(bdAddr);
    }
}

/* Empties a command queue. */
static inline void wpadQueueClear(WPADCmdQueue* queue)
{
    u32 level = OSDisableInterrupts();
    queue->head = 0;
    queue->tail = 0;
    memset(queue->items, 0, queue->capacity * sizeof(WPADCommand));
    OSRestoreInterrupts(level);
}

/* Sends one queued command to the remote and arms its reply timeout. */
void wpadSendCommand(s32 chan, WPADCommand* cmd)
{
    u16 length = cmd->length;
    WPADCB* cb = wpadCBTable[chan];
    u8 reportId = (u8)cmd->reportId;
    u8* data = cmd->data;
    u32 level = OSDisableInterrupts();
    s8 handle = cb->devHandle;
    s32 prevStatus = cb->status;
    s32 rumble;
    BT_HDR* buf;
    u8* payload;

    if (handle < 0) {
        OSRestoreInterrupts(level);
        return;
    }
    cb->status = -2;
    rumble = cb->motorOn & wpadMotorEnabled;
    if (reportId == 0x10) {
        cb->status = prevStatus;
    } else if (reportId == 0x18) {
        cb->status = prevStatus;
        cb->pendingStreamPackets = cb->pendingStreamPackets - 1;
    } else {
        switch (reportId) {
        case 0x16:
            break;
        case 0x17:
            cb->replyC = 0;
            cb->replyB = cmd->argC;
            cb->replyD = cmd->argB;
            cb->replyA = cmd->argA;
            break;
        case 0x15:
            cb->status = prevStatus;
            cb->infoOut = cmd->param;
            cb->statusRequested = 1;
            break;
        default:
            data[0] = data[0] | 2;
            break;
        }
        cb->cmdCallback = cmd->callback;
        cb->lastReportId = reportId;
        cb->reconnectTime = __OSGetSystemTime() + (OS_BUS_CLOCK / 4) * 2;
        cb->unused_0x910 = 0;
    }
    OSRestoreInterrupts(level);
    WPADiDebugPrint("handle = %d, repid = %02x\n", handle, reportId);
    buf = GKI_getbuf(length + 0x12);
    buf->len = (u8)(length + 1);
    buf->offset = 10;
    payload = ((BT_BUF*)buf)->data + buf->offset;
    payload[0] = reportId;
    memcpy(payload + 1, data, length);
    if (rumble != 0) {
        payload[1] = payload[1] | 1;
    } else {
        payload[1] = payload[1] & 0xFE;
    }
    BTA_HhSendData(handle, buf);
}

/* Folds the received-report count into the running radio average and updates the weak-radio flag. */
void wpadUpdateRadioSensitivity(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];

    if (wpadRadioTick == 10) {
        u16 received = cb->sampleCount;
        u8 previous = cb->radioSensitivity;
        u16 sum = (u16)(previous * 9) + (u16)((u32)(received * 100) >> 1);
        u16 average = sum / 10;
        u16 level = (average <= 100) ? average : 100;
        cb->radioSensitivity = level;
        cb->sampleCount = 0;
        if (cb->weakRadio != 0) {
            if (level > 85) {
                cb->weakRadio = 0;
                cb->weakRadioHysteresis = 0;
                return;
            }
            if (level > 80) {
                u8 count = cb->weakRadioHysteresis + 1;
                cb->weakRadioHysteresis = count;
                if (count >= 20) {
                    cb->weakRadio = 0;
                    cb->weakRadioHysteresis = 0;
                }
            }
        } else {
            if (level < 75) {
                cb->weakRadio = 1;
                cb->weakRadioHysteresis = 0;
                return;
            }
            if (level < 80) {
                u8 count = cb->weakRadioHysteresis + 1;
                cb->weakRadioHysteresis = count;
                if (count >= 1) {
                    cb->weakRadio = 1;
                    cb->weakRadioHysteresis = 0;
                }
            }
        }
    }
}

/* Counts consecutive reports with no input and clears the flag once the count passes the limit. */
void wpadCountIdleReports(s32 chan, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 idle = 0;
    u32 format = cb->dataFormat;
    u16 count;

    if ((u32)(format - 3) <= 2 && sample->buttons == 0x1C10) {
        idle = 1;
    }
    if ((u32)(format - 6) <= 2 && (sample->buttons == 0x1C10 || (sample->extensionCode == 0x1450 && sample->extensionError == 0))) {
        idle = 1;
    }
    count = cb->idleCount + idle;
    cb->idleCount = count;
    if (count > 600) {
        WPADCB* again = wpadCBTable[chan];
        u32 level = OSDisableInterrupts();
        again->centerValid = 0;
        again->idleCount = 0;
        OSRestoreInterrupts(level);
    }
}

/* Reads the newest sample, refreshes the last-input time and drops the link after the idle limit. */
void wpadPollSample(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    BOOL changed = FALSE;

    if (wpadPollState == 5) {
        u32 level = OSDisableInterrupts();
        WPADSample* sample = wpadSpareSample(cb);
        BOOL different;
        OSRestoreInterrupts(level);
        different = wpadSampleChanged(cb, sample, &cb->lastSample);
        wpadCountIdleReports(chan, sample);
        if (different) {
            changed = TRUE;
            cb->lastReportTime = __OSGetSystemTime();
            memcpy(&cb->lastSample, sample, sizeof(WPADSample));
        } else if (wpadSleepMinutes != 0) {
            s64 elapsed = __OSGetSystemTime() - cb->lastReportTime;
            if ((s32)(elapsed / (OS_BUS_CLOCK >> 2)) > wpadSleepMinutes * 60) {
                wpadDropLink(chan);
            }
        }
        if (cb->lastSample.extensionError != 0 && cb->lastSample.extensionError != -7) {
            memcpy(&cb->lastSample, sample, sizeof(WPADSample));
        }
        if (changed) {
            __VIResetDimmingControlA();
        }
    }
}

/* The periodic alarm handler: runs the sampling fiber on its own stack. */
void wpadAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, wpadSamplingFiber, wpadFiberStack + sizeof(wpadFiberStack));
}

/* The periodic alarm's fiber entry; the sampling work itself. */

/* Resets every field of one channel to the disconnected state and empties its command queues. */
void wpadResetChannel(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];

    cb->infoOut = NULL;
    cb->motorOn = 0;
    cb->cmdCallback = NULL;
    cb->extensionCallback = NULL;
    cb->samplingCallback = NULL;
    cb->autoSampleBuf = NULL;
    cb->latestIndex = 0;
    cb->autoSampleCount = 0;
    cb->dataFormat = 0;
    cb->status = -1;
    cb->deviceType = 0xFD;
    cb->extensionSubType = 0;
    cb->centerValid = 0;
    cb->idleCount = 0;
    cb->statusRequested = 0;
    cb->reportRate = 12;
    cb->dpdFormat = 0;
    cb->dpdFormatWanted = 0;
    cb->unused_0x8F8[0] = 0;
    cb->unused_0x8F8[3] = 0;
    cb->unused_0x8F8[1] = 0;
    cb->unused_0x8F8[4] = 0;
    cb->unused_0x8F8[2] = 0;
    cb->unused_0x8F8[5] = 0;
    cb->lastReportTime = __OSGetSystemTime();
    cb->reconnectTime = __OSGetSystemTime();
    cb->unused_0x910 = 0;
    cb->unused_0x8C9 = 0;
    cb->unused_0x8C4 = 0;
    cb->replyA = 0;
    cb->replyB = 0;
    cb->replyD = 0;
    cb->replyC = 0;
    cb->devHandle = -1;
    cb->unused_0x8D8 = 0;
    cb->ready = 0;
    cb->calibrationPhase = 0;
    cb->weakRadio = 1;
    cb->weakRadioHysteresis = 0;
    cb->pendingStreamPackets = 0;
    cb->unused_0x982 = 0;
    cb->radioSensitivity = 0;
    cb->sampleCount = 0;
    cb->disconnecting = 1;
    cb->infoPending = 0;
    cb->infoCallback = NULL;
    cb->reportOnChange = 0;
    cb->extBatteryLevel = 4;
    cb->extensionResult = 0xFD;
    cb->unused_0x991 = 0;
    cb->unused_0x992 = 0;
    memset(&cb->flags, 0, sizeof(cb->flags));
    memset(cb->extId, 0, 0x40);
    memset(&cb->calA, 0, sizeof(cb->calA));
    memset(&cb->calB, 0, sizeof(cb->calB));
    memset(cb->pad_0x914, 0, 0x10);
    memset(cb->keyAdd, 0, 8);
    memset(cb->keyXor, 0, 8);
    memset(cb, 0, 0x38);
    cb->sampleIndex = 0;
    memset(cb->samples, 0, sizeof(cb->samples));
    cb->samples[0].extensionError = -1;
    cb->samples[1].extensionError = -1;
    memcpy(&cb->lastSample, &cb->samples[0], sizeof(WPADSample));
    cb->dpdStatusA = -1;
    cb->dpdStatusB = -1;
    cb->cmdQueue.items = cb->cmdStorage;
    cb->cmdQueue.capacity = 0x18;
    cb->auxQueue.items = cb->auxStorage;
    cb->auxQueue.capacity = 0xC;
    wpadQueueClear(&cb->cmdQueue);
    wpadQueueClear(&cb->auxQueue);
    wpadResetSpeakerState(chan);
    wpadChanStateA[chan] = 0;
    wpadChanStateB[chan] = 0;
}

/* Brings the driver up: powers the sensor bar, resets the four channels, reads the SC settings and arms the poll alarm. */
void wpadStart(void)
{
    u32 level = OSDisableInterrupts();
    s32 i;
    u8 sensitivity;
    u8 volume;
    u8 bar;
    u8 motor;

    *(volatile u32*)0xCD0000C0 |= 0x100;
    OSRestoreInterrupts(level);
    for (i = 0; i < 16; i++) {
        wpadHandleToChan[i] = -1;
    }
    WPADiDebugPrint("WPADInit()\n");
    for (i = 0; i < 4; i++) {
        wpadCBTable[i] = &wpadCBs[i];
        wpadChanAllocated[i] = 0;
        wpadCBs[i].connectCallback = NULL;
        wpadResetChannel(i);
        OSInitThreadQueue(&wpadCBs[i].threadQueue);
        wpadChanStateA[i] = 0;
        wpadChanStateB[i] = 0;
    }
    wpadSleepMinutes = 5;
    wpadGameName = OSGetAppGamename();
    wpadAppType = OSGetAppType();
    sensitivity = SCGetBtDpdSensibility();
    if (sensitivity < 1) {
        sensitivity = 1;
    }
    if (sensitivity > 5) {
        sensitivity = 5;
    }
    wpadDpdSensitivity = sensitivity;
    bar = SCGetWpadSensorBarPosition();
    wpadSensorBarTop = (bar == 1);
    motor = SCGetWpadMotorMode();
    wpadMotorEnabled = (motor == 1);
    volume = SCGetWpadSpeakerVolume();
    if (volume == 0) {
        volume = 0;
    }
    if (volume >= 0x7F) {
        volume = 0x7F;
    }
    wpadSpeakerVolume = volume;
    wpadRadioTick = 0;
    wpadPollState = 0;
    wpadWifiCheckTick = 0;
    wpadShutdownRequested = 0;
    wpadWifiCheckEnabled = 1;
    wpadWifiChannel = -1;
    wpadKpadOwnsCallbacks = 0;
    OSRegisterVersion(wpadVersionString);
    if (wpadInitHook[0] != NULL) {
        wpadInitHook[0]();
    }
    if (wpadInitHook[1] != NULL) {
        wpadInitHook[1]();
    }
    if (wpadInitHook[2] != NULL) {
        wpadInitHook[2]();
    }
    if (wpadInitHook[3] != NULL) {
        wpadInitHook[3]();
    }
    OSCreateAlarm(&wpadAlarm);
    OSSetPeriodicAlarm(&wpadAlarm, OSGetTime(), OS_BUS_CLOCK / 4000, wpadAlarmHandler);
}

/* Registers the shutdown record, starts the Bluetooth stack and the driver. */
void WPADInit(void)
{
    wpadInitialized = 1;
    if (wpadShutdownRegistered == 0) {
        OSRegisterShutdownFunction(&wpadShutdownInfo);
        wpadShutdownRegistered = 1;
    }
    if (WUDInit() != 0) {
        wpadStackStarted = 0;
        wpadReconnectMode = -1;
        wpadReconnectDelayMs = 50;
        wpadStart();
    }
}

void WPADStartSyncDevice(void)
{
    WUDStartSyncDevice();
}

void WPADStartFastSimpleSync(void)
{
    WUDStartFastSyncSimple();
}

void WPADStopSimpleSync(void)
{
    WUDStopSyncSimple();
}

void WPADStartClearDevice(void)
{
    WUDStartClearDevice();
}

void WPADSetSyncDeviceCallback(void (*callback)(s32 result))
{
    WUDSetSyncDeviceCallback(callback);
}

void WPADSetSimpleSyncCallback(void (*callback)(s32 result))
{
    WUDSetSyncSimpleCallback(callback);
}

void WPADSetClearDeviceCallback(void (*callback)(s32 result))
{
    WUDSetClearDeviceCallback(callback);
}

void WPADRegisterAllocator(void* (*alloc)(u32 size), s32 (*dealloc)(void* block)) /* untyped: the caller-owned block */
{
    WUDRegisterAllocator(alloc, dealloc);
}

s32 WPADGetStatus(void)
{
    return WUDGetStatus();
}

/* Returns the radio signal strength of the channel. */
u8 WPADGetRadioSensitivity(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u8 value = cb->radioSensitivity;
    OSRestoreInterrupts(level);
    return value;
}

u8 WPADGetSensorBarPosition(void)
{
    u32 level = OSDisableInterrupts();
    u8 value = wpadSensorBarTop;
    OSRestoreInterrupts(level);
    return value;
}

/* The reply to the first status request: marks the remote ready, or drops the link on failure. */
void wpadInitDoneCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];

    if (result == 0) {
        cb->ready = 1;
        if (cb->connectCallback != NULL) {
            cb->connectCallback(chan, result);
        }
    } else {
        wpadDropLink(chan);
    }
}

/* Empties the command queue and drops the link when a command fails. */
void wpadAbortCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];

    if (result != 0) {
        wpadQueueClear(&cb->cmdQueue);
        wpadDropLink(chan);
    }
}

/* Picks the channel for a newly opened device: the balance board always takes channel 3, a remote takes its history slot. */
s32 wpadAssignChannel(WPADDeviceInfo* info)
{
    s32 chan = -1;
    const u8* addr = _WUDGetDevAddr(info->handle);
    s32 i;

    if (memcmp(info, "Nintendo RVL-WBC", 0x10) == 0) {
        chan = 3;
        if (wpadCBTable[3]->unused_0x8D8 != 0) {
            btm_remove_acl((u8*)addr);
            return -1;
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (WUDIsHistoryAddr(i, addr) != 0 && wpadChanAllocated[i] == 0) {
                chan = i;
                break;
            }
            if (wpadChanAllocated[i] == 0 && chan < 0) {
                chan = i;
            }
        }
    }
    if (WUDIsHistoryAddr(chan, addr) == 0) {
        WUDSetDeviceHistory(chan, addr);
    }
    wpadChanAllocated[chan] = 1;
    return chan;
}

/* The HID open/close callback: binds a connected remote to a channel and starts its handshake, or tears a channel down. */
void wpadHidOpenCallback(WPADDeviceInfo* info, s32 connected)
{
    u8 handle = info->handle;

    WPADiDebugPrint("connection is %s\n", connected != 0 ? "opened" : "closed");
    if (connected != 0) {
        s32 chan = wpadAssignChannel(info);
        if (chan >= 0) {
            WPADCB* cb;
            wpadHandleToChan[handle] = chan;
            wpadResetChannel(chan);
            cb = wpadCBTable[chan];
            if (memcmp(info, "Nintendo RVL-CNT", 0x10) == 0) {
                cb->deviceType = 0;
                cb->dataFormat = 0;
            } else if (memcmp(info, "Nintendo RVL-WBC", 0x10) == 0 && WUDIsLinkedWBC() != 0) {
                cb->deviceType = 3;
                cb->dataFormat = 12;
            } else {
                cb->deviceType = 0xFB;
                cb->dataFormat = 0;
            }
            cb->devHandle = handle;
            cb->unused_0x8D8 = 1;
            cb->status = 0;
            cb->radioSensitivity = 100;
            cb->disconnecting = 0;
            cb->extInitState = 0;
            cb->extensionResult = cb->deviceType;
            wpadQueueReadMemory(&cb->cmdQueue, cb->extId, 1, 0x1770, wpadInitSequence);
            __VIResetDimmingControlA();
        }
    } else {
        s8 chan = wpadHandleToChan[handle];
        wpadHandleToChan[handle] = -1;
        if (chan != -1) {
            WPADCB* cb = wpadCBTable[chan];
            WPADCommand pending;
            cb->status = -1;
            if (cb->cmdCallback != NULL) {
                cb->cmdCallback(chan, -1);
            } else if (cb->bulkDoneCallback != NULL) {
                cb->bulkDoneCallback(chan, -1);
            }
            for (;;) {
                s8 used;
                u32 level;
                BOOL any;
                {
                    u32 inner = OSDisableInterrupts();
                    used = cb->cmdQueue.tail - cb->cmdQueue.head;
                    if (used < 0) {
                        used += cb->cmdQueue.capacity;
                    }
                    OSRestoreInterrupts(inner);
                }
                if (used == 0) {
                    any = FALSE;
                } else {
                    u32 inner = OSDisableInterrupts();
                    memcpy(&pending, &cb->cmdQueue.items[cb->cmdQueue.head], sizeof(WPADCommand));
                    OSRestoreInterrupts(inner);
                    any = TRUE;
                }
                if (!any) {
                    break;
                }
                if (pending.callback != NULL) {
                    pending.callback(chan, -1);
                }
                level = OSDisableInterrupts();
                {
                    s8 left;
                    u32 inner = OSDisableInterrupts();
                    left = cb->cmdQueue.tail - cb->cmdQueue.head;
                    if (left < 0) {
                        left += cb->cmdQueue.capacity;
                    }
                    OSRestoreInterrupts(inner);
                    if (left == 0) {
                        OSRestoreInterrupts(level);
                    } else {
                        memset(&cb->cmdQueue.items[cb->cmdQueue.head], 0, sizeof(WPADCommand));
                        cb->cmdQueue.head = (cb->cmdQueue.head == cb->cmdQueue.capacity - 1) ? 0 : cb->cmdQueue.head + 1;
                        OSRestoreInterrupts(level);
                    }
                }
            }
            WPADiDebugPrint("clean up command queue\n");
            if (cb->autoSampleBuf != NULL) {
                WPADSetAutoSamplingBuf(chan, NULL, cb->autoSampleCount);
            }
            wpadResetChannel(chan);
            wpadChanAllocated[chan] = 0;
            if (cb->connectCallback != NULL) {
                cb->connectCallback(chan, -1);
            }
        } else {
            WPADiDebugPrint("WARNING: disconnection for device handle not assigned to channel.\n");
        }
    }
}

/* The HID data callback: routes an input report to the channel that owns the device handle. */
void wpadHidDataCallback(u8 handle, u8* report, u16 length)
{
    u8 chan = (u8)wpadHandleToChan[handle];

    if (chan < 4) {
        s32 error = wpadDispatchReport(chan, report);
        if (error != 0) {
            WPADiDebugPrint("HID Parser reports: %d\n", error);
        }
    }
}

/* Copies the one-G minus zero-G accelerometer difference of the remote (type 0) or the extension (type 1). */
void WPADGetAccGravityUnit(s32 chan, s32 type, WPADAccGravityUnit* unit)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();

    if (unit != NULL) {
        switch (type) {
        case 0:
            unit->x = cb->calA.accOne[0] - cb->calA.accZero[0];
            unit->y = cb->calA.accOne[1] - cb->calA.accZero[1];
            unit->z = cb->calA.accOne[2] - cb->calA.accZero[2];
            break;
        case 1:
            unit->x = cb->calB.nunchuk.accOne[0] - cb->calB.nunchuk.accZero[0];
            unit->y = cb->calB.nunchuk.accOne[1] - cb->calB.nunchuk.accZero[1];
            unit->z = cb->calB.nunchuk.accOne[2] - cb->calB.nunchuk.accZero[2];
            break;
        }
    }
    OSRestoreInterrupts(level);
}

/* Closes the HID link once the LED command that parks the remote has been answered. */
void wpadCloseLinkCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];

    if (result != -1) {
        BTA_HhClose(cb->devHandle);
    }
}

/* Starts an orderly disconnect: forgets the device history and turns the LEDs off before closing the link. */
void WPADDisconnect(s32 chan)
{
    if (wpadGetStatus(chan) != -1) {
        WPADCB* cb;
        u32 level;
        s32 status;
        WUDSetDeviceHistory(chan, NULL);
        cb = wpadCBTable[chan];
        level = OSDisableInterrupts();
        status = cb->status;
        OSRestoreInterrupts(level);
        if (status != -1) {
            level = OSDisableInterrupts();
            if (cb->disconnecting != 0) {
                OSRestoreInterrupts(level);
                return;
            }
            cb->disconnecting = 1;
            OSRestoreInterrupts(level);
            WPADControlLed(chan, 0, wpadCloseLinkCallback);
        }
    }
}

/* Sets how many idle minutes pass before the remotes are dropped. */
void WPADSetAutoSleepTime(s8 minutes)
{
    u32 level = OSDisableInterrupts();
    wpadSleepMinutes = minutes;
    OSRestoreInterrupts(level);
}

/* Returns the channel status; `type` receives the attached device type. */
s32 WPADProbe(s32 chan, u32* type)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status;

    if (type != NULL) {
        *type = cb->deviceType;
    }
    status = cb->status;
    if (status != -1) {
        if (cb->deviceType == 0xFD) {
            status = -1;
        } else if (cb->ready == 0) {
            status = -2;
        }
    }
    OSRestoreInterrupts(level);
    return status;
}

/* Installs the per-sample callback and returns the previous one. */
WPADSamplingCallback WPADSetSamplingCallback(s32 chan, WPADSamplingCallback callback)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADSamplingCallback previous;
    u32 level;

    if (wpadKpadOwnsCallbacks != 0) {
        OSReport("WARNING: Overwritten the callback needed by KPAD.\n");
        OSReport("         Please call KPADSetSamplingCallback instead of WPADSetSamplingCallback.\n");
    }
    level = OSDisableInterrupts();
    previous = cb->samplingCallback;
    cb->samplingCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Installs the connect callback and returns the previous one. */
WPADCallback WPADSetConnectCallback(s32 chan, WPADCallback callback)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCallback previous;
    u32 level;

    if (wpadKpadOwnsCallbacks != 0) {
        OSReport("WARNING: Overwritten the callback needed by KPAD.\n");
        OSReport("         Please call KPADSetConnectCallback instead of WPADSetConnectCallback.\n");
    }
    level = OSDisableInterrupts();
    previous = cb->connectCallback;
    cb->connectCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Installs the extension callback and returns the previous one. */
WPADCallback WPADSetExtensionCallback(s32 chan, WPADCallback callback)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    WPADCallback previous = cb->extensionCallback;
    cb->extensionCallback = callback;
    OSRestoreInterrupts(level);
    return previous;
}

/* Returns the input data format of the channel. */
u32 WPADGetDataFormat(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u32 format = cb->dataFormat;
    OSRestoreInterrupts(level);
    return format;
}

/* Asks the remote to report in `format`; returns 0 or a negative error. */
s32 WPADSetDataFormat(s32 chan, u32 format)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 ready = cb->ready;
    s32 status = cb->status;
    u32 current = cb->dataFormat;
    OSRestoreInterrupts(level);
    if (status == -1) {
        return -1;
    }
    if (ready == 0) {
        return -2;
    }
    if (current == format) {
        return 0;
    }
    if (WPADiSendSetReportType(&cb->cmdQueue, format, cb->reportOnChange, NULL) != 0) {
        level = OSDisableInterrupts();
        cb->dataFormat = format;
        OSRestoreInterrupts(level);
        return 0;
    }
    return -2;
}

/* Runs the callback WPADGetInfoAsync stored, then clears it. */
void wpadInfoCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->infoCallback != NULL) {
        cb->infoCallback(chan, result);
    }
    cb->infoCallback = NULL;
    cb->infoPending = 0;
}

/* Requests the remote's status report; `callback` runs when it arrives. */
s32 WPADGetInfoAsync(s32 chan, void* info, WPADCallback callback) /* untyped: the caller-owned status record */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 ready = cb->ready;
    s32 status = cb->status;
    u8 pending = cb->infoPending;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0 || pending != 0) {
            status = -2;
        } else {
            WPADCommand cmd;
            level = OSDisableInterrupts();
            cb->infoPending = 1;
            cb->infoCallback = callback;
            OSRestoreInterrupts(level);
            cmd.length = 1;
            cmd.data[0] = 0;
            cmd.reportId = 0x15;
            cmd.callback = wpadInfoCallback;
            cmd.param = info;
            if (wpadQueuePush(&cb->cmdQueue, cmd)) {
                status = 0;
            } else {
                status = -2;
                level = OSDisableInterrupts();
                cb->infoPending = 0;
                cb->infoCallback = NULL;
                OSRestoreInterrupts(level);
            }
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Turns the rumble motor of a channel on or off. */
void WPADControlMotor(s32 chan, u32 command)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();

    if (cb->status == -1) {
        OSRestoreInterrupts(level);
        return;
    }
    if (wpadMotorEnabled == 0 && (command != 0 || cb->motorOn != 1)) {
        OSRestoreInterrupts(level);
        return;
    }
    if ((command == 0 && cb->motorOn == 0) || (command == 1 && cb->motorOn == 1)) {
        OSRestoreInterrupts(level);
        return;
    }
    cb->motorOn = command != 0;
    cb->motorDirty = 1;
    OSRestoreInterrupts(level);
}

/* Enables or disables rumble for all channels. */
void WPADEnableMotor(s32 enable)
{
    u32 level = OSDisableInterrupts();
    wpadMotorEnabled = enable;
    OSRestoreInterrupts(level);
}

s32 WPADIsMotorEnabled(void)
{
    u32 level = OSDisableInterrupts();
    s32 enabled = wpadMotorEnabled;
    OSRestoreInterrupts(level);
    return enabled;
}

/* Lights the player LEDs selected by `leds`; returns 0 or a negative error. */
s32 WPADControlLed(s32 chan, u8 leds, WPADCallback callback)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            WPADCommand cmd;
            cmd.data[0] = (leds << 4) & 0xF0;
            cmd.length = 1;
            cmd.reportId = 0x11;
            cmd.callback = callback;
            status = wpadQueuePush(&cb->cmdQueue, cmd) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Writes the speaker volume and rumble setting back to the console configuration. */
BOOL WPADSaveConfig(void (*callback)(s32 result))
{
    u32 level;
    u8 volume;
    s32 motor;
    s32 saved;

    if (SCCheckStatus() != 0) {
        return FALSE;
    }
    level = OSDisableInterrupts();
    volume = wpadSpeakerVolume;
    motor = wpadMotorEnabled != 0;
    OSRestoreInterrupts(level);
    saved = SCSetWpadSpeakerVolume(volume) & 1;
    saved = saved & SCSetWpadMotorMode(motor);
    if (saved != 0) {
        SCFlushAsync(callback);
    } else if (callback != NULL) {
        callback(2);
    }
    return saved;
}

/* Copies the newest sample, truncated to the size of the channel's data format, into `dest`. */
void wpadCopyCurrentSample(s32 chan, void* dest) /* untyped: caller-owned sample record */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    WPADSample* sample = wpadSpareSample(cb);
    s32 size;
    switch (cb->dataFormat) {
    case 3:
    case 4:
    case 5:
        size = 0x32;
        break;
    case 6:
    case 7:
    case 8:
    case 11:
    case 15:
        size = 0x36;
        break;
    case 10:
        size = 0x2E;
        break;
    case 12:
        size = 0x34;
        break;
    case 13:
    case 14:
        size = 0x4A;
        break;
    case 9:
        size = 0x5A;
        break;
    default:
        size = 0x2A;
        break;
    }
    if (sample->extensionError != 0) {
        size = 0x2A;
    }
    memcpy(dest, sample, size);
    OSRestoreInterrupts(level);
}

/* Points the channel at a ring of `count` samples that it fills after every report. */
void WPADSetAutoSamplingBuf(s32 chan, void* buf, u32 count) /* untyped: caller-owned ring of samples */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s8 fill = -4;
    s32 size;
    s32 i;

    if (cb->status == -1) {
        fill = -1;
    }
    switch (cb->dataFormat) {
    case 3:
    case 4:
    case 5:
        size = 0x32;
        break;
    case 6:
    case 7:
    case 8:
    case 11:
    case 15:
        size = 0x36;
        break;
    case 10:
        size = 0x2E;
        break;
    case 12:
        size = 0x34;
        break;
    case 13:
    case 14:
        size = 0x4A;
        break;
    case 9:
        size = 0x5A;
        break;
    default:
        size = 0x2A;
        break;
    }
    if (buf != NULL) {
        memset(buf, 0, size * count);
        for (i = 0; i < count; i++) {
            ((WPADSample*)((u8*)buf + i * size))->extensionError = fill;
        }
        cb->latestIndex = -1;
        cb->autoSampleCount = count;
    }
    cb->autoSampleBuf = buf;
    OSRestoreInterrupts(level);
}

/* Returns the index of the newest sample in the auto sampling buffer (0 before the first). */
s32 WPADGetLatestIndexInBuf(s32 chan)
{
    u32 level = OSDisableInterrupts();
    WPADCB* cb = wpadCBTable[chan];
    s32 index = cb->latestIndex;
    OSRestoreInterrupts(level);
    return index == -1 ? 0 : index;
}

/* Clears the button bits the remote reports when both halves of a pair are pressed. */
void wpadFilterButtons(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    WPADSample* sample = wpadSpareSample(cb);
    u32 format;

    if ((sample->buttons & 3) == 3) {
        sample->buttons = sample->buttons & ~2;
    }
    if ((sample->buttons & 0xC) == 0xC) {
        sample->buttons = sample->buttons & ~4;
    }
    format = cb->dataFormat;
    if ((u32)(format - 6) <= 2 || format == 11 || format == 15) {
        if ((s16)(sample->extensionCode & 0x8002) == (s16)0x8002) {
            sample->extensionCode = sample->extensionCode & 0x7FFF;
        }
        if ((s16)(sample->extensionCode & 0x4001) == 0x4001) {
            sample->extensionCode = sample->extensionCode & ~0x4000;
        }
    }
    if (cb->dataFormat == 10) {
        if ((s16)(sample->extensionCode & 0x8002) == (s16)0x8002) {
            sample->extensionCode = sample->extensionCode & 0x7FFF;
        }
        if ((s16)(sample->extensionCode & 0x4001) == 0x4001) {
            sample->extensionCode = sample->extensionCode & ~0x4000;
        }
    }
    OSRestoreInterrupts(level);
}

/* Stores the newest sample in the auto sampling ring, calls the sampling callback and counts the report. */
void wpadStoreSample(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    WPADSample* sample = wpadSpareSample(cb);
    s32 size;
    switch (cb->dataFormat) {
    case 3:
    case 4:
    case 5:
        size = 0x32;
        break;
    case 6:
    case 7:
    case 8:
    case 11:
    case 15:
        size = 0x36;
        break;
    case 10:
        size = 0x2E;
        break;
    case 12:
        size = 0x34;
        break;
    case 13:
    case 14:
        size = 0x4A;
        break;
    case 9:
        size = 0x5A;
        break;
    default:
        size = 0x2A;
        break;
    }
    if (cb->autoSampleBuf != NULL) {
        u32 index = cb->latestIndex + 1;
        void* dest;
        cb->latestIndex = index;
        if (index >= cb->autoSampleCount) {
            cb->latestIndex = 0;
        }
        dest = (u8*)cb->autoSampleBuf + cb->latestIndex * size;
        if (sample->extensionError != 0) {
            size = 0x2A;
        }
        memcpy(dest, sample, size);
    }
    if (cb->samplingCallback != NULL) {
        cb->samplingCallback(chan);
    }
    cb->sampleCount = cb->sampleCount + 1;
    OSRestoreInterrupts(level);
}

s32 WPADIsSpeakerEnabled(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 enabled = cb->flags.speakerEnabled;
    OSRestoreInterrupts(level);
    return enabled;
}

u8 WPADGetSpeakerVolume(void)
{
    u32 level = OSDisableInterrupts();
    u8 volume = wpadSpeakerVolume;
    OSRestoreInterrupts(level);
    return volume;
}

void WPADSetSpeakerVolume(u8 volume)
{
    u32 level = OSDisableInterrupts();
    u8 clamped = volume;
    if (volume == 0) {
        clamped = 0;
    }
    if (volume >= 0x7F) {
        clamped = 0x7F;
    }
    wpadSpeakerVolume = clamped;
    OSRestoreInterrupts(level);
}

/* Whether the audio stream cannot take another packet right now. */
BOOL __wpadIsBusyStream(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u8 weakRadio = cb->weakRadio;
    u32 level = OSDisableInterrupts();
    u8 deviceType = cb->deviceType;
    u8 bufferStatus = WUDGetBufferStatus();
    s8 queued;
    u16 notAcked;
    u8 pending;
    u8 links;
    {
        u32 inner = OSDisableInterrupts();
        queued = cb->cmdQueue.tail - cb->cmdQueue.head;
        if (queued < 0) {
            queued += cb->cmdQueue.capacity;
        }
        OSRestoreInterrupts(inner);
    }
    {
        WPADCB* again = wpadCBTable[chan];
        u32 inner = OSDisableInterrupts();
        s32 status = again->status;
        s8 handle = again->devHandle;
        OSRestoreInterrupts(inner);
        if (status != -1) {
            _WUDGetQueuedSize(handle);
        }
    }
    {
        WPADCB* again = wpadCBTable[chan];
        u32 inner = OSDisableInterrupts();
        s32 status = again->status;
        s8 handle = again->devHandle;
        OSRestoreInterrupts(inner);
        if (status == -1) {
            notAcked = 0;
        } else {
            notAcked = _WUDGetNotAckedSize(handle);
        }
    }
    pending = cb->pendingStreamPackets;
    links = _WUDGetLinkNumber();
    OSRestoreInterrupts(level);
    if (weakRadio != 0 || notAcked > 3 || bufferStatus == 10 || bufferStatus >= ((links * 2) & 0x1FE) + 2 || deviceType == 0xFF
        || queued >= 0x15 || pending >= 1) {
        return TRUE;
    }
    return FALSE;
}

/* Whether the channel can take a speaker data packet. */
BOOL WPADCanSendStreamData(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status == -1 || ready == 0 || __wpadIsBusyStream(chan) != 0) {
        return FALSE;
    }
    return TRUE;
}

/* Queues one packet of speaker data (up to 20 bytes); returns 0 or a negative error. */
s32 WPADSendStreamData(s32 chan, const void* data, u32 length) /* untyped: byte range */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    WPADCommand cmd;
    OSRestoreInterrupts(level);
    if (status == -1) {
        return -1;
    }
    if (ready == 0) {
        return -2;
    }
    if (__wpadIsBusyStream(chan) != 0) {
        return -2;
    }
    cmd.reportId = 0x18;
    cmd.length = 0x15;
    cmd.data[0] = (length * 8) & 0xF8;
    cmd.callback = NULL;
    memcpy(&cmd.data[1], data, length);
    if (!wpadQueuePush(&cb->cmdQueue, cmd)) {
        return -2;
    }
    level = OSDisableInterrupts();
    cb->pendingStreamPackets = cb->pendingStreamPackets + 1;
    OSRestoreInterrupts(level);
    return 0;
}

/* Returns the Bluetooth pointer sensitivity (1..5). */
u8 WPADGetDpdSensitivity(void)
{
    u32 level = OSDisableInterrupts();
    u8 value = wpadDpdSensitivity;
    OSRestoreInterrupts(level);
    return value;
}

s32 WPADIsDpdEnabled(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 enabled = cb->flags.dpdEnabled;
    OSRestoreInterrupts(level);
    return enabled;
}

u8 WPADGetDpdFormat(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u8 format;
    if (cb->flags.dpdEnabled != 0) {
        format = cb->dpdFormat;
    } else {
        format = 0;
    }
    OSRestoreInterrupts(level);
    return format;
}

/* Adopts the requested pointer format and updates the enabled flag from it. */
void wpadApplyDpdFormat(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];

    cb->dpdFormat = cb->dpdFormatWanted;
    cb->dpdBlocked = 0;
    cb->flags.dpdEnabled = cb->dpdFormatWanted != 0;
}

/* Queues a set-data-reporting-mode command; FALSE when the queue is full. */
s32 WPADiSendSetReportType(WPADCmdQueue* queue, u32 type, s32 onChangeOnly, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x12;
    cmd.length = 2;
    cmd.data[0] = 4 & ~-(onChangeOnly != 0);
    cmd.callback = callback;
    switch (type) {
    case 0:
        cmd.data[1] = 0x30;
        break;
    case 1:
        cmd.data[1] = 0x31;
        break;
    case 2:
        cmd.data[1] = 0x33;
        break;
    case 3:
        cmd.data[1] = 0x32;
        break;
    case 4:
        cmd.data[1] = 0x35;
        break;
    case 5:
        cmd.data[1] = 0x37;
        break;
    case 6:
        cmd.data[1] = 0x32;
        break;
    case 7:
        cmd.data[1] = 0x35;
        break;
    case 8:
        cmd.data[1] = 0x37;
        break;
    case 9:
        cmd.data[1] = 0x3E;
        break;
    case 10:
        cmd.data[1] = 0x32;
        break;
    case 11:
        cmd.data[1] = 0x37;
        break;
    case 15:
        cmd.data[1] = 0x37;
        break;
    case 12:
        cmd.data[1] = 0x34;
        break;
    case 13:
        cmd.data[1] = 0x35;
        break;
    case 14:
        cmd.data[1] = 0x3D;
        break;
    }
    return wpadQueuePush(queue, cmd);
}

/* Queues a one-byte write of `value` to the remote address `addr`. */
s32 wpadSendWriteByte(WPADCmdQueue* queue, s8 value, u32 addr, WPADCallback callback)
{
    WPADCommand cmd;
    s8 byteValue = value;
    u32 address = addr;
    u8 size = 1;

    cmd.reportId = 0x16;
    cmd.length = 0x15;
    cmd.callback = callback;
    memcpy(&cmd.data[0], &address, 4);
    memcpy(&cmd.data[4], &size, 1);
    memcpy(&cmd.data[5], &byteValue, 1);
    return wpadQueuePush(queue, cmd);
}

/* Queues a write of `size` bytes (at most 16) from `src` to the remote address `addr`. */
s32 WPADiSendWriteData(WPADCmdQueue* queue, const void* src, s32 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    WPADCommand cmd;
    u32 address = addr;
    s8 count = size & 0x1F;

    cmd.reportId = 0x16;
    cmd.length = 0x15;
    cmd.callback = callback;
    memcpy(&cmd.data[0], &address, 4);
    memcpy(&cmd.data[4], &count, 1);
    memcpy(&cmd.data[5], src, size);
    return wpadQueuePush(queue, cmd);
}

/* Queues a read of `size` bytes from the remote address `addr` into `dest`. */
s32 wpadSendReadMemory(WPADCmdQueue* queue, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    return wpadQueueReadMemory(queue, dest, size, addr, callback);
}

/* Whether `count` more commands fit in the queue. */
s32 wpadQueueHasRoom(WPADCmdQueue* queue, s8 count)
{
    u32 level = OSDisableInterrupts();
    s8 used = queue->tail - queue->head;

    if (used < 0) {
        used += queue->capacity;
    }
    OSRestoreInterrupts(level);
    if ((u32)(used + count) <= (u32)(queue->capacity - 1)) {
        return 1;
    }
    return 0;
}

/* Empties a command queue. */
void WPADiClearQueue(WPADCmdQueue* queue)
{
    u32 level = OSDisableInterrupts();
    queue->head = 0;
    queue->tail = 0;
    memset(queue->items, 0, queue->capacity * sizeof(WPADCommand));
    OSRestoreInterrupts(level);
}

void WPADSetCallbackByKPAD(s32 owned)
{
    wpadKpadOwnsCallbacks = owned;
}

u8 wpadGetAppType(void)
{
    return wpadAppType;
}

char* wpadGetGameName(void)
{
    return wpadGameName;
}

/* Applies a square dead zone to the stick (x, y) and scales the pair back inside the octagon of radius `outer`; `wide` selects 16-bit axes. */
void wpadStickDeadzoneSquare(WPADStickAxis* x, WPADStickAxis* y, s32 outer, s32 scale, s32 dead, s32 wide)
{
    s32 vx;
    s32 vy;
    s32 signX;
    s32 signY;
    s32 scaledX;
    s32 scaledY;
    s32 denominator;
    s32 numerator;

    if (wide == 0) {
        vx = x->narrow;
        vy = y->narrow;
    } else {
        vx = x->wide;
        vy = y->wide;
    }
    if (vx >= 0) {
        signX = 1;
    } else {
        signX = -1;
        vx = -vx;
    }
    if (vy >= 0) {
        signY = 1;
    } else {
        signY = -1;
        vy = -vy;
    }
    vx = vx <= dead ? 0 : vx - dead;
    vy = vy <= dead ? 0 : vy - dead;
    if (vx == 0 && vy == 0) {
        if (wide == 0) {
            x->narrow = 0;
            y->narrow = 0;
        } else {
            x->wide = 0;
            y->wide = 0;
        }
        return;
    }
    scaledX = scale * vx;
    scaledY = scale * vy;
    if (scaledY <= scaledX) {
        numerator = scale * outer;
        denominator = scaledX + vy * (outer - scale);
        if (numerator < denominator) {
            if (wide == 0) {
                vx = (s8)((vx * numerator) / denominator);
                vy = (s8)((vy * numerator) / denominator);
            } else {
                vx = (s16)((vx * numerator) / denominator);
                vy = (s16)((vy * numerator) / denominator);
            }
        }
    } else {
        numerator = scale * outer;
        denominator = scaledY + vx * (outer - scale);
        if (numerator < denominator) {
            if (wide == 0) {
                vx = (s8)((vx * numerator) / denominator);
                vy = (s8)((vy * numerator) / denominator);
            } else {
                vx = (s16)((vx * numerator) / denominator);
                vy = (s16)((vy * numerator) / denominator);
            }
        }
    }
    if (wide == 0) {
        x->narrow = signX * vx;
        y->narrow = signY * vy;
    } else {
        x->wide = signX * vx;
        y->wide = signY * vy;
    }
}

/* Applies a circular dead zone to the stick (x, y), shrinking the pair to the radius `limit` when it is longer; `wide` selects 16-bit axes. */
void wpadStickDeadzoneCircle(WPADStickAxis* x, WPADStickAxis* y, s32 limit, s32 dead, s32 wide)
{
    s32 vx;
    s32 vy;
    s32 lengthSq;

    if (wide == 0) {
        vx = x->narrow;
        vy = y->narrow;
    } else {
        vx = x->wide;
        vy = y->wide;
    }
    if (-dead < vx && vx < dead) {
        vx = 0;
    } else if (vx > 0) {
        vx = vx - dead;
    } else {
        vx = vx + dead;
    }
    if (-dead < vy && vy < dead) {
        vy = 0;
    } else if (vy > 0) {
        vy = vy - dead;
    } else {
        vy = vy + dead;
    }
    lengthSq = vx * vx + vy * vy;
    if (limit * limit < lengthSq) {
        s32 length = (s32)(f32)sqrt((f32)lengthSq);
        vx = (vx * limit) / length;
        vy = (vy * limit) / length;
    }
    if (wide == 0) {
        x->narrow = vx;
        y->narrow = vy;
    } else {
        x->wide = vx;
        y->wide = vy;
    }
}

/* Clamps the three accelerometer axes to `limit` times their full-scale value, keeping each sign. */
void wpadClampAxes(s16* x, s16* y, s16* z, WPADAxisScale* scale, f32 limit)
{
    s16 fullScaleX = scale->x;
    f32 nx = (f32)*x / (f32)fullScaleX;
    f32 signX = 1.0f;
    f32 signY = signX;
    f32 signZ = signX;
    f32 ny = (f32)*y / (f32)scale->y;
    f32 nz = (f32)*z / (f32)scale->z;

    if (nx < 0.0f) {
        signX = -1.0f;
        nx = -nx;
    }
    if (ny < 0.0f) {
        signY = -1.0f;
        ny = -ny;
    }
    if (nz < 0.0f) {
        signZ = -1.0f;
        nz = -nz;
    }
    if (nx > limit) {
        nx = limit;
    }
    if (ny > limit) {
        ny = limit;
    }
    if (nz > limit) {
        nz = limit;
    }
    *x = (s16)(nx * signX * (f32)fullScaleX);
    *y = (s16)(ny * signY * (f32)scale->y);
    *z = (s16)(nz * signZ * (f32)scale->z);
}

/* Clears the speaker state words of a channel. */
void wpadResetSpeakerState(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];

    cb->speakerStateA = 0;
    cb->speakerStateB = 0;
    cb->speakerStateC = 0;
    cb->speakerStateD = 0;
    cb->bulkDoneCallback = NULL;
}

/* The report handlers, indexed by report id - 0x20. */
typedef void (*WPADReportHandler)(u8 chan, u8* report, WPADSample* sample);
static WPADReportHandler wpadReportHandlers[32] = {
    wpadReportStatus, wpadReportReadData, wpadReportAck, wpadReportIgnored,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportButtons, wpadReportButtonsAccel, wpadReportButtonsExt8, wpadReportButtonsAccelDpd12,
    wpadReportExt19, wpadReportButtonsAccelExt16, wpadReportButtonsDpd10Ext9, wpadReportButtonsAccelDpd10Ext6,
    wpadReportIgnored, wpadReportIgnored, wpadReportIgnored, wpadReportIgnored,
    wpadReportIgnored, wpadReportExt21, wpadReportInterleavedA, wpadReportInterleavedB,
};

#pragma dont_inline on
/* Runs after an extension handshake step: retries the handshake or reports its failure. */
void wpadExtInitCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    s8 error;
    WPADCallback callback;

    if (result == 0) {
        return;
    }
    WPADiClearQueue(&cb->auxQueue);
    cb->extInitState = 0;
    if (result == -1) {
        error = -3;
    } else if (cb->flags.extensionAttached != 0) {
        u8 tries = wpadExtInitRetries[chan];
        wpadExtInitRetries[chan] = tries + 1;
        if (tries < 0x20) {
            WPADCB* again = wpadCBTable[chan];
            WPADiClearQueue(&again->auxQueue);
            WPADiSendSetReportType(&again->auxQueue, again->dataFormat, again->reportOnChange, wpadExtInitCallback);
            again->extInitState = 1;
            wpadSendWriteByte(&again->auxQueue, 0x55, 0x04A400F0, wpadExtInitCallback);
            wpadSendWriteByte(&again->auxQueue, 0, 0x04A400FB, wpadExtInitCallback);
            wpadSendReadMemory(&again->auxQueue, again->extId, 6, 0x04A400FA, wpadExtInitCallback);
            return;
        }
        error = -4;
    } else {
        WPADiDebugPrint("detaching extension during initialization.\n");
        WPADiSendSetReportType(&cb->auxQueue, cb->dataFormat, cb->reportOnChange, NULL);
        return;
    }
    cb->deviceType = error;
    cb->extensionResult = error;
    cb->unused_0x991 = 0;
    callback = cb->extensionCallback;
    if (callback != NULL) {
        callback(chan, error);
    }
}

#pragma dont_inline reset

/* Checks the checksum of the pointer calibration block just read and stores the block when it is intact. */
void wpadDpdCalibrationReadDone(s32 chan, s32 error, s32 which)
{
    WPADCB* cb = wpadCBTable[chan];
    u8* data = (u8*)cb->replyA;

    if (error == 0) {
        s32 sum = 0;
        s32 i;
        for (i = 0; i < 0x2F; i++) {
            sum += data[i];
        }
        if (data[0x2F] == (u8)(sum + 0x55)) {
            memcpy(cb->calBytes, data, sizeof(cb->calBytes));
            (&cb->dpdStatusA)[(u8)which] = 0;
        } else {
            (&cb->dpdStatusA)[(u8)which] = -4;
        }
    } else {
        (&cb->dpdStatusA)[(u8)which] = -4;
    }
}

/* Handles the input report of the dispatch table that carries nothing the driver reads. */
void wpadReportIgnored(u8 chan, u8* report, WPADSample* sample)
{
}

#pragma dont_inline on
/* Handles the status report: the battery, LED and extension flags, and starts the extension handshake. */
void wpadReportStatus(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 wasAttached = cb->flags.extensionAttached;
    u32 level;
    WPADCallback callback;

    WPADiDebugPrint("Received report 20\n");
    level = OSDisableInterrupts();
    if (cb->ready == 0) {
        OSRestoreInterrupts(level);
        return;
    }
    cb->flags.extensionAttached = (report[3] & 2) >> 1;
    cb->flags.batteryLow = report[3] & 1;
    cb->flags.ledMask = (report[3] >> 4) & 0xF;
    cb->flags.protect = 0;
    cb->flags.statusFlags = report[5] & 0xF0;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    cb->flags.dpdEnabled = (report[3] >> 3) & 1;
    cb->flags.speakerEnabled = (report[3] >> 2) & 1;
    if (cb->deviceType == 3) {
        cb->flags.batteryLevel = cb->extBatteryLevel;
    } else {
        u8 level7 = report[6];
        if (level7 >= 0x55) {
            cb->flags.batteryLevel = 4;
        } else if (level7 >= 0x44) {
            cb->flags.batteryLevel = 3;
        } else if (level7 >= 0x33) {
            cb->flags.batteryLevel = 2;
        } else if (level7 >= 3) {
            cb->flags.batteryLevel = 1;
        } else {
            cb->flags.batteryLevel = 0;
        }
    }
    if (cb->flags.extensionAttached != 0) {
        if (wasAttached == 0) {
            WPADCB* again;
            WPADiDebugPrint("initialize attachment\n");
            wpadExtInitRetries[chan] = 0;
            again = wpadCBTable[chan];
            callback = cb->extensionCallback;
            WPADiClearQueue(&again->auxQueue);
            WPADiSendSetReportType(&again->auxQueue, again->dataFormat, again->reportOnChange, wpadExtInitCallback);
            again->extInitState = 1;
            wpadSendWriteByte(&again->auxQueue, 0x55, 0x04A400F0, wpadExtInitCallback);
            wpadSendWriteByte(&again->auxQueue, 0, 0x04A400FB, wpadExtInitCallback);
            wpadSendReadMemory(&again->auxQueue, again->extId, 6, 0x04A400FA, wpadExtInitCallback);
            cb->deviceType = 0xFF;
            cb->extensionSubType = 0;
            if (callback != NULL) {
                callback(chan, 0xFF);
            }
        }
    } else {
        cb->deviceType = 0;
        cb->extensionSubType = 0;
        WPADiClearQueue(&cb->auxQueue);
        WPADiSendSetReportType(&cb->auxQueue, cb->dataFormat, cb->reportOnChange, NULL);
        if (wasAttached != 0) {
            cb->unused_0x991 = 1;
            cb->unused_0x992 = 0x12C;
            callback = cb->extensionCallback;
            if (callback != NULL) {
                callback(chan, 0);
            }
        }
    }
    if (cb->infoOut != NULL) {
        memcpy(cb->infoOut, &cb->flags, sizeof(WPADFlags));
        cb->infoOut = NULL;
    }
    memcpy(sample, wpadSpareSample(wpadCBTable[chan]), sizeof(WPADSample));
    sample->buttons = ((((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F) & ~0x6000) | (sample->buttons & 0x6000);
    if (sample->deviceType != cb->deviceType) {
        sample->deviceType = cb->deviceType;
        sample->extensionError = -4;
    }
    callback = cb->cmdCallback;
    if (callback != NULL && cb->statusRequested != 0) {
        callback(chan, 0);
        cb->cmdCallback = NULL;
    }
    cb->statusRequested = 0;
    OSRestoreInterrupts(level);
}

#pragma dont_inline reset

/* Handles the memory-read reply: copies the payload and runs the read's follow-up when the last block arrives. */
void wpadReportReadData(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u32 base;
    u16 offset;
    u32 high;
    u16 low;
    u8 size;
    s32 result;
    WPADCallback callback;

    if ((report[3] & 0xF) != 0) {
        WPADiDebugPrint("read error happens!\n");
        cb->replyC = -1;
        callback = cb->cmdCallback;
        if (callback != NULL) {
            if (cb->extensionCallback == NULL || cb->extensionCallback != callback) {
                callback(chan, -3);
            }
            cb->cmdCallback = NULL;
        }
        cb->status = 0;
    }
    base = cb->replyB;
    low = base;
    offset = (report[5] & ~0xFF00) | ((report[4] << 8) & 0xFF00);
    high = base >> 16;
    size = ((s32)report[3] >> 4) + 1;
    if (offset >= low && offset <= low + cb->replyD) {
        memcpy((u8*)cb->replyA + (s16)(offset - low), report + 6, size);
        if (low + cb->replyD == offset + size) {
            WPADiDebugPrint("base addr: %08x\n", cb->replyB);
            WPADiDebugPrint("length   : %d\n", (u32)cb->replyD);
            result = (-3) & ((s32)cb->replyC >> 31);
            WPADiDebugPrint("i2c = %04x\n", base >> 16);
            WPADiDebugPrint("enc = %d\n", (u32)cb->extInitState);
            if (high == 0x4A4) {
                WPADiDebugPrint("Access to extension register.\n");
                if ((u8)(cb->extInitState - 2) <= 1) {
                    WPADiDebugPrint("Decode!!!!\n");
                    WPADiDebugPrint("    len = %d, addr = %04x\n", (u32)size, (s32)(u16)base);
                    wpadDecryptExtension(chan, (u8*)cb->replyA, size, (u16)base);
                }
            }
            if ((cb->replyB == 0 && cb->calibrationPhase == 0) || (cb->replyB == 0x176C && cb->calibrationPhase == 1)) {
                wpadAccCalibrationReadDone(chan, result);
            }
            if (cb->replyB + 0xFB5C0000 == 0x20) {
                wpadExtCalibrationReadDone(chan, result);
            }
            if (cb->replyB + 0xFB5C0000 == 0xFA) {
                wpadExtIdReadDone(chan, result);
            }
            if (cb->replyB == 0x2A) {
                wpadDpdCalibrationReadDone(chan, result, 0);
            }
            if (cb->replyB == 0x62) {
                wpadDpdCalibrationReadDone(chan, result, 1);
            }
            callback = cb->cmdCallback;
            if (callback != NULL) {
                callback(chan, result);
                cb->cmdCallback = NULL;
            }
            cb->status = 0;
        }
    } else {
        WPADiDebugPrint("received data is out of range!\n");
    }
    memcpy(sample, wpadSpareSample(wpadCBTable[chan]), sizeof(WPADSample));
    sample->buttons = ((((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F) & ~0x6000) | (sample->buttons & 0x6000);
    if (sample->deviceType != cb->deviceType) {
        sample->deviceType = cb->deviceType;
        sample->extensionError = -4;
    }
    OSRestoreInterrupts(level);
}

/* Handles the acknowledge report: completes the pending command when the acknowledged id is the one in flight. */
void wpadReportAck(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    u8 ackedId;
    u8 error;
    s32 result;
    WPADCallback callback;

    WPADiDebugPrint("Received ack!\n");
    memcpy(sample, wpadSpareSample(wpadCBTable[chan]), sizeof(WPADSample));
    sample->buttons = ((((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F) & ~0x6000) | (sample->buttons & 0x6000);
    if (sample->deviceType != cb->deviceType) {
        sample->deviceType = cb->deviceType;
        sample->extensionError = -4;
    }
    ackedId = report[3];
    error = report[4];
    result = (error == 0) ? 0 : -3;
    WPADiDebugPrint("ack --> report ID = %02x, error code = %d\n", ackedId, error);
    if (cb->lastReportId == ackedId) {
        callback = cb->cmdCallback;
        if (callback != NULL) {
            callback(chan, result);
            cb->cmdCallback = NULL;
        }
        cb->status = 0;
    } else {
        WPADiDebugPrint("invalid ack!\n");
    }
    OSRestoreInterrupts(level);
}

/* Dispatches an input report to its handler and publishes the decoded sample; returns -1 for an unknown report id. */
s32 wpadDispatchReport(s32 chan, u8* report)
{
    WPADCB* cb = wpadCBTable[(u8)chan];
    s32 result = 0;

    if ((u8)(report[0] - 0x20) < 0x20) {
        WPADCB* current = wpadCBTable[(u8)chan];
        u32 level = OSDisableInterrupts();
        WPADSample* sample = &current->samples[current->sampleIndex];

        if (report[0] != 0x3E && report[0] != 0x3F) {
            memset(sample, 0, sizeof(WPADSample));
        }
        wpadReportHandlers[report[0] - 0x20]((u8)chan, report, sample);
        if (cb->ready == 0) {
            sample->extensionError = -4;
        }
        if (report[0] != 0x3E && report[0] != 0x3F) {
            WPADCB* next = wpadCBTable[(u8)chan];
            next->sampleIndex = (next->sampleIndex == 0);
        }
        OSRestoreInterrupts(level);
        wpadFilterButtons(chan);
        wpadStoreSample(chan);
    } else {
        result = -1;
    }
    return result;
}

/* Decodes the pointer objects of a basic (format 1) or extended (format 3) report and rotates them by the channel's roll. */
void wpadDecodeDpd(s32 chan, WPADSample** sample, u32 format, const u8* data, s32 length)
{
    u32 i;
    u8 slot;

    if (wpadCBTable[chan]->dpdBlocked != 0) {
        (*sample)->extensionError = -4;
        return;
    }
    if (format == 3) {
        for (slot = 0; slot < 4; slot++) {
            s32 offset = slot * 3;
            if (offset + 2 < length) {
                const u8* object = data + offset;
                u8 high = object[2];
                (*sample)->dpd[slot].x = object[0] | ((high << 4) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (object[1] | ((high << 2) & 0x300));
                (*sample)->dpd[slot].size = high & 0xF;
                (*sample)->dpd[slot].size = (u8)(3.141592f * ((f32)(*sample)->dpd[slot].size * (f32)(*sample)->dpd[slot].size));
                if ((*sample)->dpd[slot].size == 0 || (*sample)->dpd[slot].x == 0x3FF || (*sample)->dpd[slot].y == 0x2FF) {
                    (*sample)->dpd[slot].x = 0;
                    (*sample)->dpd[slot].y = 0x2FF;
                    (*sample)->dpd[slot].size = 0;
                }
                (*sample)->dpd[slot].slot = slot;
            } else {
                (*sample)->dpd[slot].x = 0;
                (*sample)->dpd[slot].y = 0x2FF;
                (*sample)->dpd[slot].size = 0;
                (*sample)->dpd[slot].slot = slot;
            }
        }
    } else if (format == 1) {
        for (slot = 0; slot < 4; slot++) {
            s32 pair = slot >> 1;
            const u8* object = data + slot * 3 - pair;
            u8 x = object[0];
            u8 y = object[1];
            u8 high = data[pair * 5 + 2];
            if (slot % 2 == 0) {
                (*sample)->dpd[slot].x = x | ((high << 4) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (y | ((high << 2) & 0x300));
            } else {
                (*sample)->dpd[slot].x = x | ((high << 8) & 0x300);
                (*sample)->dpd[slot].y = 0x2FF - (y | ((high << 6) & 0x300));
            }
            if ((*sample)->dpd[slot].x == 0x3FF || (*sample)->dpd[slot].y == 0x2FF) {
                (*sample)->dpd[slot].x = 0;
                (*sample)->dpd[slot].y = 0x2FF;
                (*sample)->dpd[slot].size = 0;
            } else {
                (*sample)->dpd[slot].size = 0xC;
            }
            (*sample)->dpd[slot].slot = slot;
        }
    }
    for (i = 0; i < 4; i++) {
        WPADDpdObject* object = &(*sample)->dpd[i];
        if (object->x != 0 || object->y != 0x2FF) {
            f32 px = (f32)object->x + wpadDpdCenterX[chan];
            f32 py = (f32)object->y + wpadDpdCenterY[chan];
            f32 dx = px - wpadDpdPivotX[chan];
            f32 dy = py - wpadDpdPivotY[chan];
            f32 sine = (f32)sin(-1.0f * wpadDpdRoll[chan]);
            object->x = wpadDpdPivotX[chan] + ((dx * (f32)cos(-1.0f * wpadDpdRoll[chan])) - (dy * sine));
            sine = (f32)cos(-1.0f * wpadDpdRoll[chan]);
            (*sample)->dpd[i].y = wpadDpdPivotY[chan] + ((dx * (f32)sin(-1.0f * wpadDpdRoll[chan])) + (dy * sine));
        }
    }
}

/* Decodes one 9-byte pointer record of an interleaved report into `slot` of the sample, plus its bounding box. */
void wpadDecodeDpdInterleaved(s32 chan, WPADSample** sample, u8 slot, const u8* data, s32 reserved)
{
    (*sample)->dpd[slot].x = data[0] | ((data[2] << 4) & 0x300);
    (*sample)->dpd[slot].y = 0x2FF - (data[1] | ((data[2] << 2) & 0x300));
    (*sample)->dpdExt[slot].intensity = (((s16)((data[7] << 8) & 0xFF00) | data[8]) << 6) & 0xFFC0;
    (*sample)->dpdExt[slot].sizeExtra = data[2] & 0xF;
    (*sample)->dpdExt[slot].left = data[3] != 0xFF ? data[3] : 0;
    (*sample)->dpdExt[slot].top = data[4] != 0xFF ? data[4] : 0;
    (*sample)->dpdExt[slot].right = data[5] != 0xFF ? data[5] : 0;
    (*sample)->dpdExt[slot].bottom = data[6] != 0xFF ? data[6] : 0;
    (*sample)->dpdExt[slot].left = (*sample)->dpdExt[slot].left * 8;
    (*sample)->dpdExt[slot].top = 0x2FF - (*sample)->dpdExt[slot].top * 8;
    (*sample)->dpdExt[slot].right = (*sample)->dpdExt[slot].right * 8;
    (*sample)->dpdExt[slot].bottom = 0x2FF - (*sample)->dpdExt[slot].bottom * 8;
    (*sample)->dpd[slot].size = 3.141592f * ((f32)(s8)(*sample)->dpdExt[slot].sizeExtra * (f32)(s8)(*sample)->dpdExt[slot].sizeExtra);
    if ((*sample)->dpd[slot].size == 0 || (*sample)->dpd[slot].x == 0x3FF || (*sample)->dpd[slot].y == 0x2FF || (*sample)->dpdExt[slot].sizeExtra == 0xF) {
        (*sample)->dpd[slot].x = 0;
        (*sample)->dpd[slot].y = 0x2FF;
        (*sample)->dpd[slot].size = 0;
        (*sample)->dpdExt[slot].intensity = 0;
        (*sample)->dpdExt[slot].sizeExtra = 0;
    }
    (*sample)->dpd[slot].slot = slot;
}

/* Decodes the sticks and triggers of a Classic-style extension payload and centres them on the first reading. */
void wpadDecodeExtDualStick(s32 chan, WPADSample** sample, u8 variant, const u8* data, u32 length)
{
    WPADCB* cb = wpadCBTable[chan];

    switch (variant) {
    case 2:
        (*sample)->classic.leftX = (s16)((s16)(data[0] * 4) & ~3) | (data[4] & 3);
        (*sample)->classic.rightX = (s16)((s16)(data[1] * 4) & ~3) | ((data[4] >> 2) & 3);
        (*sample)->classic.leftY = (s16)((s16)(data[2] * 4) & ~3) | ((data[4] >> 4) & 3);
        (*sample)->classic.rightY = (s16)((s16)(data[3] * 4) & ~3) | (s16)((s32)data[4] >> 6);
        (*sample)->classic.leftTrigger = data[5];
        (*sample)->classic.rightTrigger = (length < 9) ? 0 : data[6];
        (*sample)->classic.buttons = (length < 9) ? 0 : (((data[8] & ~0xFF00) | ((data[7] << 8) & 0xFF00)) ^ 0xFFFF);
        break;
    case 3:
        (*sample)->classic.leftX = (s16)data[0] * 4;
        (*sample)->classic.rightX = (s16)data[1] * 4;
        (*sample)->classic.leftY = (s16)data[2] * 4;
        (*sample)->classic.rightY = (s16)data[3] * 4;
        (*sample)->classic.leftTrigger = data[4];
        (*sample)->classic.rightTrigger = data[5];
        (*sample)->classic.buttons = (length < 8) ? 0 : (((data[7] & ~0xFF00) | ((data[6] << 8) & 0xFF00)) ^ 0xFFFF);
        break;
    default:
        (*sample)->classic.leftX = (data[0] * 0x10) & 0x3F0;
        (*sample)->classic.leftY = (data[1] * 0x10) & 0x3F0;
        (*sample)->classic.rightX = (s16)((s16)((s32)data[2] >> 7) | ((((data[1] >> 5) & 6) & ~0x18) | ((data[0] >> 3) & 0x18))) << 5;
        (*sample)->classic.rightY = (data[2] << 5) & 0x3E0;
        (*sample)->classic.leftTrigger = (u8)((((((s32)data[3] >> 5) & ~0x18) | (((s32)data[2] >> 2) & 0x18)) * 8) & 0xF8);
        (*sample)->classic.rightTrigger = (u8)((data[3] * 8) & 0xF8);
        (*sample)->classic.buttons = ((data[5] & ~0xFF00) | ((data[4] << 8) & 0xFF00)) ^ 0xFFFF;
        break;
    }
    if (cb->deviceType == 2) {
        (*sample)->classic.leftX = (*sample)->classic.leftX - 0x200;
        (*sample)->classic.leftY = (*sample)->classic.leftY - 0x200;
        (*sample)->classic.rightX = (*sample)->classic.rightX - 0x200;
        (*sample)->classic.rightY = (*sample)->classic.rightY - 0x200;
    } else {
        (*sample)->classic.leftX = (*sample)->classic.leftX - 0x200;
        (*sample)->classic.leftY = (*sample)->classic.leftY - 0x200;
    }
    if (cb->centerValid == 0) {
        cb->centerValid = 1;
        cb->calB.classic.leftX[0] = (*sample)->classic.leftX;
        cb->calB.classic.leftY[0] = (*sample)->classic.leftY;
        if (cb->deviceType == 2) {
            cb->calB.classic.rightX[0] = (*sample)->classic.rightX;
            cb->calB.classic.rightY[0] = (*sample)->classic.rightY;
            cb->calB.classic.leftTrigger = (*sample)->classic.leftTrigger;
            cb->calB.classic.rightTrigger = (*sample)->classic.rightTrigger;
        } else {
            cb->calB.classic.rightX[0] = 0;
            cb->calB.classic.rightY[0] = 0;
            cb->calB.classic.leftTrigger = 0;
            cb->calB.classic.rightTrigger = 0;
        }
    }
    {
        s16 value = (*sample)->classic.leftX - cb->calB.classic.leftX[0];
        (*sample)->classic.leftX = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
        value = (*sample)->classic.leftY - cb->calB.classic.leftY[0];
        (*sample)->classic.leftY = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
    }
    if (cb->deviceType == 2) {
        s16 value = (*sample)->classic.rightX - cb->calB.classic.rightX[0];
        (*sample)->classic.rightX = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
        value = (*sample)->classic.rightY - cb->calB.classic.rightY[0];
        (*sample)->classic.rightY = value < -0x200 ? -0x200 : (value > 0x1FF ? 0x1FF : value);
        value = (*sample)->classic.leftTrigger - cb->calB.classic.leftTrigger;
        (*sample)->classic.leftTrigger = value < 0 ? 0 : (value > 0xFF ? 0xFF : value);
        value = (*sample)->classic.rightTrigger - cb->calB.classic.rightTrigger;
        (*sample)->classic.rightTrigger = value < 0 ? 0 : (value > 0xFF ? 0xFF : value);
    }
    if (wpadSticksZeroed[chan] != 0) {
        (*sample)->classic.rightX = 0;
        (*sample)->classic.rightY = 0;
        (*sample)->classic.leftTrigger = 0;
        (*sample)->classic.rightTrigger = 0;
    }
}

/* Decodes the 21-byte payload of the board-style extension into the sample and its calibration. */
void wpadDecodeExtBoard(s32 chan, WPADSample** sample, u8 format, const u8* data, u32 length)
{
    (*sample)->board.batteryRef = data[0];
    (*sample)->board.weightRef[0] = (data[1] << 2) | ((s32)data[6] >> 6);
    (*sample)->board.weightRef[1] = (data[2] << 2) | ((s32)data[6] >> 4 & 3);
    (*sample)->board.weightRef[2] = (data[3] << 2) | ((s32)data[6] >> 2 & 3);
    (*sample)->board.weightRef[3] = (data[4] << 2) | (data[6] & 3);
    (*sample)->board.weightRef[4] = (data[5] << 2) | ((s32)data[7] >> 6);
    (*sample)->board.temperature = data[8];
    (*sample)->board.weight[0] = (data[9] << 2) | ((s32)data[14] >> 6);
    (*sample)->board.weight[1] = (data[10] << 2) | ((s32)data[14] >> 4 & 3);
    (*sample)->board.weight[2] = (data[11] << 2) | ((s32)data[14] >> 2 & 3);
    (*sample)->board.weight[3] = (data[12] << 2) | (data[14] & 3);
    (*sample)->board.weight[4] = (data[13] << 2) | ((s32)data[15] >> 6);
    {
        s32 reference = (data[7] << 4) & 0xF3F0;
        (*sample)->board.refA = (reference & ~0xF) | (((s32)data[15] >> 2) & 0xF);
        (*sample)->board.refC = data[15] & 3;
        if (reference < 0) {
            (*sample)->board.refB = 0;
            (*sample)->board.refD = 0;
            (*sample)->board.refE = 0;
            (*sample)->board.refF = 0;
            return;
        }
    }
    (*sample)->board.refB = (data[16] << 2) | ((s32)data[17] >> 6);
    (*sample)->board.refD = data[18] & 7;
    (*sample)->board.refE = data[19];
    (*sample)->board.refF = data[20];
}

/* Decodes the button word of the 0x32 report, 8 extension bytes follow. */
void wpadReportButtonsExt8(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat == 0 || cb->dataFormat == 3 || cb->dataFormat == 6 || cb->dataFormat == 10) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    memcpy(wpadExtRaw, report + 3, 8);
    wpadDecryptExtension(chan, report + 3, 8, 0);
    if (cb->flags.extensionAttached != 0) {
        switch (cb->deviceType) {
        case 1: {
            WPADCB* again = wpadCBTable[chan];
            s16 value;
            sample->nunchuk.stickX = report[3];
            sample->nunchuk.stickY = report[4];
            sample->nunchuk.accX = (s16)(((s16)(report[5] * 4) & ~3) | ((report[8] >> 2) & 3)) - again->calB.nunchuk.accZero[0];
            sample->nunchuk.accY = (s16)(((s16)(report[6] * 4) & ~3) | ((report[8] >> 4) & 3)) - again->calB.nunchuk.accZero[1];
            sample->nunchuk.accZ = (s16)(((s16)(report[7] * 4) & ~3) | (s16)((s32)report[8] >> 6)) - again->calB.nunchuk.accZero[2];
            sample->buttons = sample->buttons | ((~report[8] << 13) & 0x6000);
            if (again->centerValid == 0) {
                again->centerValid = 1;
                again->calB.nunchuk.stickX[0] = sample->nunchuk.stickX;
                again->calB.nunchuk.stickY[0] = sample->nunchuk.stickY;
            }
            value = sample->nunchuk.stickX - (u8)again->calB.nunchuk.stickX[0];
            sample->nunchuk.stickX = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            value = sample->nunchuk.stickY - (u8)again->calB.nunchuk.stickY[0];
            sample->nunchuk.stickY = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            break;
        }
        case 2:
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 3, 8);
            break;
        case 16:
            sample->simple.stickX = report[5];
            sample->simple.stickY = report[6];
            sample->simple.buttons = ((report[10] & ~0xFF00) | ((report[9] << 8) & 0xFF00)) ^ 0xFFFF;
            break;
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 8) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x33 report: buttons, accelerometer and 12 bytes of pointer data. */
void wpadReportButtonsAccelDpd12(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat <= 2) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
    wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 6, 12);
}

/* Decodes the 0x34 report: buttons and 19 extension bytes. */
void wpadReportExt19(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat == 12) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    memcpy(wpadExtRaw, report + 3, 0x13);
    wpadDecryptExtension(chan, report + 3, 0x13, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 3) {
            s32 doubled;
            sample->board.weight[0] = (report[4] & ~0xFF00) | ((report[3] << 8) & 0xFF00);
            sample->board.weight[1] = (report[6] & ~0xFF00) | ((report[5] << 8) & 0xFF00);
            sample->board.weight[2] = (report[8] & ~0xFF00) | ((report[7] << 8) & 0xFF00);
            sample->board.weight[3] = (report[10] & ~0xFF00) | ((report[9] << 8) & 0xFF00);
            sample->board.weight[4] = report[11];
            sample->board.temperature = report[13];
            doubled = sample->board.temperature * 2;
            if (doubled >= 0x104) {
                cb->extBatteryLevel = 4;
            } else if ((u32)(doubled - 0xFA) <= 9) {
                cb->extBatteryLevel = 3;
            } else if ((u32)(doubled - 0xF0) <= 9) {
                cb->extBatteryLevel = 2;
            } else if ((u32)(doubled - 0xD4) <= 0x1B) {
                cb->extBatteryLevel = 1;
            } else {
                cb->extBatteryLevel = 0;
            }
            if (cb->centerValid == 0) {
                cb->centerValid = 1;
            }
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 0x13) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x35 report: buttons, accelerometer and 16 extension bytes. */
void wpadReportButtonsAccelExt16(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || (u32)(cb->dataFormat - 3) <= 1 || (u32)(cb->dataFormat - 6) <= 1 || cb->dataFormat == 13) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
    memcpy(wpadExtRaw, report + 6, 0x10);
    wpadDecryptExtension(chan, report + 6, 0x10, 0);
    if (cb->flags.extensionAttached != 0) {
        switch (cb->deviceType) {
        case 1: {
            WPADCB* current = wpadCBTable[chan];
            s16 value;
            sample->nunchuk.stickX = report[6];
            sample->nunchuk.stickY = report[7];
            sample->nunchuk.accX = (s16)(((s16)(report[8] * 4) & ~3) | ((report[11] >> 2) & 3)) - current->calB.nunchuk.accZero[0];
            sample->nunchuk.accY = (s16)(((s16)(report[9] * 4) & ~3) | ((report[11] >> 4) & 3)) - current->calB.nunchuk.accZero[1];
            sample->nunchuk.accZ = (s16)(((s16)(report[10] * 4) & ~3) | (s16)((s32)report[11] >> 6)) - current->calB.nunchuk.accZero[2];
            sample->buttons = sample->buttons | ((~report[11] << 13) & 0x6000);
            if (current->centerValid == 0) {
                current->centerValid = 1;
                current->calB.nunchuk.stickX[0] = sample->nunchuk.stickX;
                current->calB.nunchuk.stickY[0] = sample->nunchuk.stickY;
            }
            value = sample->nunchuk.stickX - (u8)current->calB.nunchuk.stickX[0];
            sample->nunchuk.stickX = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            value = sample->nunchuk.stickY - (u8)current->calB.nunchuk.stickY[0];
            sample->nunchuk.stickY = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            break;
        }
        case 2:
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 6, 0x10);
            break;
        case 4:
            wpadDecodeExtBoard(chan, &sample, cb->dataFormat, report + 6, 0x10);
            break;
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 0x10) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x36 report: buttons, 10 bytes of pointer data and 9 extension bytes. */
void wpadReportButtonsDpd10Ext9(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->extensionError = -4;
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 3, 10);
    memcpy(wpadExtRaw, report + 13, 9);
    wpadDecryptExtension(chan, report + 13, 9, 0);
    if (cb->flags.extensionAttached != 0 && memcmp(wpadExtRaw, wpadExtZero, 9) == 0 && sample->extensionError == 0) {
        sample->extensionError = -7;
    }
}

/* Decodes the 0x37 report: buttons, accelerometer, 10 bytes of pointer data and 6 extension bytes. */
void wpadReportButtonsAccelDpd10Ext6(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 8 || cb->dataFormat == 11 || cb->dataFormat == 15) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
    wpadDecodeDpd(chan, &sample, cb->dpdFormat, report + 6, 10);
    memcpy(wpadExtRaw, report + 16, 6);
    wpadDecryptExtension(chan, report + 16, 6, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 1) {
            WPADCB* current = wpadCBTable[chan];
            s16 value;
            sample->nunchuk.stickX = report[16];
            sample->nunchuk.stickY = report[17];
            sample->nunchuk.accX = (s16)(((s16)(report[18] * 4) & ~3) | ((report[21] >> 2) & 3)) - current->calB.nunchuk.accZero[0];
            sample->nunchuk.accY = (s16)(((s16)(report[19] * 4) & ~3) | ((report[21] >> 4) & 3)) - current->calB.nunchuk.accZero[1];
            sample->nunchuk.accZ = (s16)(((s16)(report[20] * 4) & ~3) | (s16)((s32)report[21] >> 6)) - current->calB.nunchuk.accZero[2];
            sample->buttons = sample->buttons | ((~report[21] << 13) & 0x6000);
            if (current->centerValid == 0) {
                current->centerValid = 1;
                current->calB.nunchuk.stickX[0] = sample->nunchuk.stickX;
                current->calB.nunchuk.stickY[0] = sample->nunchuk.stickY;
            }
            value = sample->nunchuk.stickX - (u8)current->calB.nunchuk.stickX[0];
            sample->nunchuk.stickX = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
            value = sample->nunchuk.stickY - (u8)current->calB.nunchuk.stickY[0];
            sample->nunchuk.stickY = value < -0x80 ? -0x80 : (value > 0x7F ? 0x7F : value);
        } else if (cb->deviceType == 2 || (u8)(cb->deviceType - 0x11) <= 1) {
            wpadDecodeExtDualStick(chan, &sample, cb->extensionSubType, report + 16, 6);
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 6) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the 0x3D report: 21 extension bytes and no buttons. */
void wpadReportExt21(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->dataFormat == 14) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    memcpy(wpadExtRaw, report + 1, 0x15);
    wpadDecryptExtension(chan, report + 1, 0x15, 0);
    if (cb->flags.extensionAttached != 0) {
        if (cb->deviceType == 4) {
            wpadDecodeExtBoard(chan, &sample, cb->dataFormat, report + 1, 0x15);
        }
        if (memcmp(wpadExtRaw, wpadExtZero, 0x15) == 0 && sample->extensionError == 0) {
            sample->extensionError = -7;
        }
    }
}

/* Decodes the first half (slots 0 and 1) of an interleaved pointer report. */
void wpadReportInterleavedA(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level;

    if (wpadInterleavedA[chan] == 0 && wpadInterleavedB[chan] == 0) {
        memset(sample, 0, sizeof(WPADSample));
    }
    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || cb->dataFormat == 9) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = 0;
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | (s16)((report[1] >> 6) & 2)) - wpadCBTable[chan]->calA.accZero[0];
    sample->accZ = sample->accZ | (s16)(((s16)(report[2] * 8) & ~0xFF) | ((report[1] * 2) & 0xC0));
    wpadDecodeDpdInterleaved(chan, &sample, 0, report + 4, 0);
    wpadDecodeDpdInterleaved(chan, &sample, 1, report + 13, 0);
    level = OSDisableInterrupts();
    wpadInterleavedA[chan] = 1;
    if (wpadInterleavedB[chan] != 0) {
        sample->accZ = sample->accZ - cb->calA.accZero[2];
        cb->sampleIndex = (cb->sampleIndex == 0);
        wpadInterleavedB[chan] = 0;
        wpadInterleavedA[chan] = 0;
    }
    OSRestoreInterrupts(level);
}

/* Decodes the second half (slots 2 and 3) of an interleaved pointer report. */
void wpadReportInterleavedB(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level;

    if (wpadInterleavedA[chan] == 0 && wpadInterleavedB[chan] == 0) {
        memset(sample, 0, sizeof(WPADSample));
    }
    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    if (cb->dataFormat <= 1 || cb->dataFormat == 9) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    sample->deviceType = cb->deviceType;
    cb->flags.buttonByteTopBit = 0;
    sample->accY = (s16)(((s16)(report[3] * 4) & ~3) | (s16)((report[1] >> 6) & 2)) - wpadCBTable[chan]->calA.accZero[1];
    sample->accZ = sample->accZ | (s16)(((report[1] >> 3) & 0xC) | ((report[2] >> 1) & 0x30));
    wpadDecodeDpdInterleaved(chan, &sample, 2, report + 4, 0);
    wpadDecodeDpdInterleaved(chan, &sample, 3, report + 13, 0);
    level = OSDisableInterrupts();
    wpadInterleavedB[chan] = 1;
    if (wpadInterleavedA[chan] != 0 && wpadInterleavedB[chan] != 0) {
        sample->accZ = sample->accZ - cb->calA.accZero[2];
        cb->sampleIndex = (cb->sampleIndex == 0);
        wpadInterleavedB[chan] = 0;
        wpadInterleavedA[chan] = 0;
    }
    OSRestoreInterrupts(level);
}

/* Sends the balance-board handshake bytes to the extension register through the write queue. */
s32 wpadSendWbcCommand(s32 chan, s8 command, WPADCallback callback)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    s8 data[7];

    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0 || WUDIsLinkedWBC() == 0) {
            status = -2;
        } else {
            data[2] = 0xAA;
            data[1] = 0xAA;
            data[0] = 0xAA;
            data[3] = 0x55;
            data[6] = command;
            data[5] = command;
            data[4] = command;
            level = OSDisableInterrupts();
            switch (command) {
            case 0xAA: {
                s8 used;
                OSDisableInterrupts();
                used = cb->cmdQueue.tail - cb->cmdQueue.head;
                if (used < 0) {
                    used += cb->cmdQueue.capacity;
                }
                OSRestoreInterrupts(0);
                if ((u32)(used + 4) <= (u32)(cb->cmdQueue.capacity - 1)) {
                    WPADWriteExtReg(chan, data, 7, 0xF1, NULL);
                    WPADWriteExtReg(chan, data, 1, 0xF1, NULL);
                    WPADWriteExtReg(chan, data, 1, 0xF1, NULL);
                    WPADWriteExtReg(chan, data, 1, 0xF1, callback);
                    OSRestoreInterrupts(level);
                    return 0;
                }
                status = -2;
                break;
            }
            case 0x55:
                status = WPADWriteExtReg(chan, data, 7, 0xF1, callback);
                if (status == 0) {
                    OSRestoreInterrupts(level);
                    return 0;
                }
                break;
            case 0:
                status = WPADWriteExtReg(chan, data, 1, 0xF1, callback);
                if (status == 0) {
                    OSRestoreInterrupts(level);
                    return 0;
                }
                break;
            default:
                status = -2;
                break;
            }
            OSRestoreInterrupts(level);
        }
    }
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Continues a bulk write: sends the next 16-byte chunk, or reports completion when the data is spent or a chunk fails. */
void wpadBulkWriteNext(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    u16 remaining = cb->speakerStateC;
    u16 chunk;
    u32 level;
    WPADCallback done;

    if (remaining == 0) {
        cb->speakerStateA = 0;
        done = cb->bulkDoneCallback;
        cb->bulkDoneCallback = NULL;
        if (done != NULL) {
            done(chan, result);
        }
    } else {
        chunk = (remaining <= 0x10) ? remaining : 0x10;
        if (result == 0) {
            cb->speakerStateC = cb->speakerStateC - chunk;
            cb->speakerStateD = cb->speakerStateD + 0x10;
            cb->speakerStateB = cb->speakerStateB + 0x10;
            cb = wpadCBTable[chan];
            level = OSDisableInterrupts();
            wpadWriteMemory(chan, (const void*)cb->speakerStateB, chunk, cb->speakerStateD, wpadBulkWriteNext);
            OSRestoreInterrupts(level);
        } else if (result == -2) {
            level = OSDisableInterrupts();
            wpadWriteMemory(chan, (const void*)cb->speakerStateB, chunk, cb->speakerStateD, wpadBulkWriteNext);
            OSRestoreInterrupts(level);
        } else {
            cb->speakerStateA = 0;
            done = cb->bulkDoneCallback;
            cb->bulkDoneCallback = NULL;
            if (done != NULL) {
                done(chan, result);
            }
        }
    }
}

/* Finishes the calibration write and starts sending the bulk data, or reports the failure. */
void wpadBulkWriteFirst(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    u16 chunk;
    u16 remaining;
    u32 level;
    WPADCB* again;

    if (cb->dpdStatusB == 0) {
        cb->dpdStatusB = (result == 0) ? 0 : -3;
    }
    if (result == 0 && (cb->dpdStatusA == 0 || cb->dpdStatusB == 0)) {
        remaining = cb->speakerStateC;
        chunk = (remaining <= 0x10) ? remaining : 0x10;
        cb->speakerStateC = cb->speakerStateC - chunk;
        again = wpadCBTable[chan];
        level = OSDisableInterrupts();
        wpadWriteMemory(chan, (const void*)again->speakerStateB, chunk, again->speakerStateD, wpadBulkWriteNext);
        OSRestoreInterrupts(level);
        return;
    }
    if (cb->bulkDoneCallback != NULL) {
        cb->speakerStateA = 0;
        cb->bulkDoneCallback(chan, result);
        cb->bulkDoneCallback = NULL;
    }
}

#pragma dont_inline on
/* Stores the calibration block with the title code and checksum, then writes it to the remote followed by the bulk data. */
s32 wpadBulkWrite(s32 chan, const void* src, u16 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    WPADCB* cb = wpadCBTable[chan];
    s32 status = cb->status;
    u32 level = OSDisableInterrupts();

    if (status != -1) {
        if (cb->ready == 0) {
            status = -2;
        } else if (cb->speakerStateA == 0) {
            if (wpadQueueHasRoom(&cb->cmdQueue, 9) == 0) {
                status = -2;
            } else {
                u8 sum;
                u8 i;
                cb->speakerStateA = 1;
                cb->bulkDoneCallback = callback;
                cb->speakerStateC = size;
                cb->speakerStateD = addr + 0x9A;
                cb->speakerStateB = (u32)src;
                memcpy(cb->cal.titleCode, wpadGetGameName(), 4);
                memcpy(cb->cal.dpdCalibration, wpadTitleBuf, 0x22);
                cb->cal.timestamp = OSGetTime();
                cb->cal.appType = wpadGetAppType();
                cb->cal.checksum = 0;
                for (i = 0; i < 0x2F; i++) {
                    cb->cal.checksum = cb->cal.checksum + cb->calBytes[i];
                }
                cb->cal.checksum = cb->cal.checksum + 0x55;
                cb->dpdStatusA = 0;
                cb->dpdStatusB = 0;
                wpadWriteMemory(chan, cb->calBytes, 0x10, 0x2A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, &cb->calBytes[0x10], 0x10, 0x3A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, &cb->calBytes[0x20], 0x10, 0x4A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, &cb->calBytes[0x30], 8, 0x5A, wpadSetDpdStatusA);
                wpadWriteMemory(chan, cb->calBytes, 0x10, 0x62, wpadSetDpdStatusB);
                wpadWriteMemory(chan, &cb->calBytes[0x10], 0x10, 0x72, wpadSetDpdStatusB);
                wpadWriteMemory(chan, &cb->calBytes[0x20], 0x10, 0x82, wpadSetDpdStatusB);
                wpadWriteMemory(chan, &cb->calBytes[0x30], 8, 0x92, wpadBulkWriteFirst);
                OSRestoreInterrupts(level);
                return 0;
            }
        } else {
            status = -2;
        }
    }
    OSRestoreInterrupts(level);
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}

#pragma dont_inline reset

/* Copies the title string (at most 16 wide characters, zero terminated) into the title buffer. */
void wpadCopyTitleString(const u16* title)
{
    s32 i;

    for (i = 0; i < 0x10; i++) {
        wpadTitleBuf[i] = title[i];
        if (title[i] == 0) {
            break;
        }
    }
    wpadTitleBuf[0x10] = 0;
}

/* Decodes the report-0x30 payload: the button word only. */
void wpadReportButtons(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat == 0) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
}

/* Decodes the report-0x31 payload: the button word and the remote's accelerometer, relative to its zero point. */
void wpadReportButtonsAccel(u8 chan, u8* report, WPADSample* sample)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCB* again;

    sample->buttons = ((report[1] & ~0xFF00) | ((report[2] << 8) & 0xFF00)) & 0x9F1F;
    sample->deviceType = cb->deviceType;
    if (cb->dataFormat <= 1) {
        sample->extensionError = 0;
    } else {
        sample->extensionError = -4;
    }
    cb->flags.buttonByteTopBit = (report[1] >> 7) & 1;
    again = wpadCBTable[chan];
    sample->accX = (s16)(((s16)(report[3] * 4) & ~3) | ((report[1] >> 5) & 3)) - again->calA.accZero[0];
    sample->accY = (s16)(((s16)(report[4] * 4) & ~3) | (s16)((report[2] >> 4) & 2)) - again->calA.accZero[1];
    sample->accZ = (s16)(((s16)(report[5] * 4) & ~3) | (s16)((report[2] >> 5) & 2)) - again->calA.accZero[2];
}

/* Decrypts `length` bytes of extension data in place with the channel's key tables; `offset` is the position within the register block. */
void wpadDecryptExtension(s32 chan, u8* data, u16 length, s32 offset)
{
    WPADCB* cb = wpadCBTable[chan];
    u16 i;

    for (i = 0; i < length; i++) {
        s32 index = (offset + i) % 8;
        data[i] = cb->keyAdd[index] + (data[i] ^ cb->keyXor[index]);
    }
}

/* Runs the callback the bulk transfer stored. */
void wpadSpeakerDoneCallback(s32 chan, s32 result)
{
    WPADCB* cb = wpadCBTable[chan];
    WPADCallback callback;

    cb->speakerStateA = 0;
    callback = cb->bulkDoneCallback;
    cb->bulkDoneCallback = NULL;
    if (callback != NULL) {
        callback(chan, result);
    }
}

/* Records the outcome of the first pointer-calibration read (non-zero `error` = failed). */
void wpadSetDpdStatusA(s32 chan, s32 error)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->dpdStatusA == 0) {
        cb->dpdStatusA = (error == 0) ? 0 : -3;
    }
}

void wpadSetDpdStatusB(s32 chan, s32 error)
{
    WPADCB* cb = wpadCBTable[chan];

    if (cb->dpdStatusB == 0) {
        cb->dpdStatusB = (error == 0) ? 0 : -3;
    }
}

/* Hands out the pointer-calibration block once both calibration reads have finished; returns 0 or a negative error. */
s32 wpadGetDpdCalibration(s32 chan, u8** out)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 missing = 0;
    s32 status;

    if (cb->dpdStatusA == 0 || cb->dpdStatusB == 0) {
        missing = 1;
    }
    status = missing != 0 ? 0 : -4;
    if (status == 0) {
        *out = cb->cal.dpdCalibration;
    } else {
        *out = NULL;
    }
    OSRestoreInterrupts(level);
    return status;
}

#pragma dont_inline on
/* Starts a guarded read of the remote's memory once the extension signature matches the title's game code. */
s32 wpadReadMemoryGuarded(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCB* cb = wpadCBTable[chan];
    s32 status = cb->status;
    u32 level = OSDisableInterrupts();

    if (status != -1) {
        if (cb->ready == 0) {
            status = -2;
        } else if (cb->speakerStateA == 0) {
            s32 missing = 0;
            if (cb->dpdStatusA == 0 || cb->dpdStatusB == 0) {
                missing = 1;
            }
            status = missing != 0 ? 0 : -6;
            if (status == 0) {
                if (memcmp(cb->cal.titleCode, wpadGetGameName(), 4) == 0) {
                    cb->speakerStateA = 1;
                    cb->bulkDoneCallback = callback;
                    OSRestoreInterrupts(level);
                    return wpadReadMemory(chan, dest, size, addr + 0x9A, wpadSpeakerDoneCallback);
                }
                status = -5;
            }
        } else {
            status = -2;
        }
    }
    OSRestoreInterrupts(level);
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Queues a memory read of the remote; returns 0 or a negative error. */
s32 wpadReadMemory(s32 chan, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = wpadSendReadMemory(&cb->cmdQueue, dest, size, addr, callback) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Queues a memory write to the remote; returns 0 or a negative error. */
s32 wpadWriteMemory(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = WPADiSendWriteData(&cb->cmdQueue, src, size, addr, callback) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}

/* Writes to the extension register space (the remote adds the 0x04A4 prefix). */
s32 WPADWriteExtReg(s32 chan, const void* src, s32 size, u32 addr, WPADCallback callback) /* untyped: byte range */
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s32 ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = WPADiSendWriteData(&cb->cmdQueue, src, size, addr | 0x04A40000, callback) ? 0 : -2;
        }
    }
    if (status != 0 && callback != NULL) {
        callback(chan, status);
    }
    return status;
}
#pragma dont_inline reset

/* Takes the format of a log line and discards it; every WPAD log call goes through here. */
void WPADiDebugPrint(const char* format, ...)
{
}
