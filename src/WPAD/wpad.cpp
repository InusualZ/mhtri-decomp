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
 *   `wpadGetDpdCalibration`, `wpadSetDpdStatusA/B`, `wpadResetSpeakerState`, `wpadGetAppType`, `wpadGetGameName`; named but
 *   unwritten: `wpadSampleChanged` (0x804EBAF0), `wpadSamplingFiber` (0x804EC650), `wpadInitSequence` (0x804ED880),
 *   `wpadDispatchReport` (0x804F4770).  `fn_` names remain on the 0x804F14A0.. and 0x804F2FF0..0x804F9220 runs.  Struct fields
 *   are named from use; a `unused_0xNN` field is written but never read by a written body.  Statics are named from use; the
 *   `.sbss` rows `wpadChanStateA/B` and the `.bss` rows at 0x8075EA00.. are not understood yet.  Foreign names given here:
 *   WUD (`WUDInit`, `WUDShutdown`, `WUDGetStatus`, `WUDRegisterAllocator`, `WUDSet*Callback`, `WUDStart*`, `WUDStop*`,
 *   `WUDSetHidRecvCallback`, `WUDSetHidConnCallback`, `WUDIsSyncing`, `WUDIsHistoryAddr`, from the `WUD...()` log strings
 *   where one exists), SC (`SCGetWpadMotorMode`, `SCGetWpadSensorBarPosition`, `SCGetWpadSpeakerVolume`), `OSReturnToMenuPending`.
 * RESIDUALS. Written: 85 of 131 rows, 56 at 100 %; 46 rows (0x804EBAF0, 0x804EC650, 0x804ED880..0x804EE754, 0x804EF900
 *   `WPADControlSpeaker`, 0x804F14A0..0x804F272C, 0x804F2FF0..0x804F8EC0 (33), 0x804F8F30..0x804F9198, 0x804F9220, 0x804F9950) are
 *   not attempted.  Register-numbering differences only (the variable order) remain in `WPADDisconnect`, `wpadAssignChannel`,
 *   `wpadHidOpenCallback`, `wpadUpdateRadioSensitivity`, `wpadReportButtonsAccel` (the original reloads the control block
 *   pointer) and `wpadDecryptExtension` (loop unrolling); `WPADSetAutoSamplingBuf` strength-reduces the index multiply the
 *   original keeps.  The queue push, clear and read builders are inline helpers; the original shares their bodies with the
 *   out-of-line copies.  Flip blockers: .text (0x3FE0 of 0xE620), .data (0x354 of 0x1CC0: the handler table 0x8062C358, the
 *   jump tables 0x8062C00C.., the strings the unwritten bodies print), .rodata 0x58, .sdata (0x17 of 0x30), .sbss (0x45 of
 *   0x78), .bss (0x3770 of 0x3800: 0x8075EA00..0x8075EAA0 belong to unwritten bodies) and .sdata2 0xB0 (the float pool the
 *   0x804F3150.. and 0x804F84E0.. bodies load).  `WPADiDebugPrint` is an empty varargs body by design.  The TU is probably
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

/* .bss */
static OSAlarm wpadAlarm;
static WPADCB* wpadCBTable[4];
static u8 wpadFiberStack[0x1000];
static s8 wpadHandleToChan[0x20];
static WPADCB wpadCBs[4];

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
s32 WPADiNullCallbackA(void)
{
    return 0;
}

s32 WPADiNullCallbackB(void)
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
        again->idleFlag = 0;
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
    cb->unused_0x8C2 = 0;
    cb->idleFlag = 0;
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
    cb->unused_0x8E0 = 0;
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
    cb->unused_0x98F = 4;
    cb->unused_0x990 = 0xFD;
    cb->unused_0x991 = 0;
    cb->unused_0x992 = 0;
    memset(&cb->flags, 0, sizeof(cb->flags));
    memset(cb->pad_0x934, 0, 0x40);
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

void WPADRegisterAllocator(s32 (*alloc)(void), s32 (*dealloc)(void))
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
            cb->unused_0x98D = 0;
            cb->unused_0x990 = cb->deviceType;
            wpadQueueReadMemory(&cb->cmdQueue, cb->pad_0x934, 1, 0x1770, wpadInitSequence);
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
void wpadHidDataCallback(u8 handle, u8* report)
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
            unit->x = cb->calB.accOne[0] - cb->calB.accZero[0];
            unit->y = cb->calB.accOne[1] - cb->calB.accZero[1];
            unit->z = cb->calB.accOne[2] - cb->calB.accZero[2];
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
    s32 links;
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
    cb->unused_0x994 = 0;
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
        *out = cb->dpdCalibration;
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
                if (memcmp(cb->titleCode, wpadGetGameName(), 4) == 0) {
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
