/* WPAD/wpad.cpp - the Wii remote driver (the SDK's WPAD.c): control block, connect/disconnect, sampling, speaker, stream
 *   data and the command queue builders.
 * RANGE. .text 0x804EB510-0x804F2FF0 (84 functions); .rodata 0x80573C80-0x80573CC0; .data 0x8062BF30-0x8062C358;
 *   .bss 0x8075B2A0-0x8075EA00; .sdata 0x80794198-0x807941C8; .sbss 0x807956A8-0x80795700; .sdata2 0x8079D3C8-0x8079D3D0.
 *   Left edge: the "<< RVL_SDK - WPAD ... Jun 22 2009 ... (0x4302_145) >>" build string (.data 0x8062BF30) and the
 *   `WBCReadDummy`/`WBCSetZEROPointDummy`/`WBCGetTGCWeightDummy` print strings follow it before any other string, so
 *   the nine 8-byte stubs and the three dummies at 0x804EB510..0x804EB630 belong to this TU.  Right edge: the seam with
 *   WPAD/WPADHIDParser.cpp at 0x804F2FF0 / 0x8062C358 (evidence in that unit's header).  The library's other files follow
 *   in SDK order: WPAD/WPADHIDParser.cpp, WPAD/WPADEncrypt.cpp, WPAD/WPADClamp.cpp, WPAD/WPADMem.cpp, WPAD/debug_msg.cpp.
 * FLAGS. `cflags_base` (-O4,p, 16-byte function alignment): every function start is 16-aligned.  `WPADSetAutoSamplingBuf`
 *   sits in a scoped `#pragma opt_strength_reduction off`: the original multiplies the index by the record size on every
 *   iteration instead of stepping a pointer.
 * NAMES. the map's names (`WPADInit`, `WPADDisconnect`, `WPADProbe`, `WPADControlSpeaker`, `WPADSendStreamData`,
 *   `WPADiSendWriteData`, `WPADiSendSetReportType`, `WPADiClearQueue`, ...).  GUESS names: `WPADiNullCallbackA/B`,
 *   `WPADiGetReserved0..3`, `WPADiReturnZeroA..C`, `WBCReadDummy`, `WBCSetZEROPointDummy`, `WBCGetTGCWeightDummy` (the last
 *   three from their print strings); public-API-shaped guesses `WPADStartSyncDevice`, `WPADStartClearDevice`,
 *   `WPADSetSyncDeviceCallback`, `WPADSetClearDeviceCallback`, `WPADRegisterAllocator`, `WPADGetStatus`,
 *   `WPADGetSensorBarPosition`, `WPADGetAccGravityUnit`, `WPADSetAutoSleepTime`, `WPADGetDataFormat`, `WPADSetDataFormat`,
 *   `WPADGetDpdSensitivity`, `WPADIsDpdEnabled`, `WPADSetAutoSamplingBuf` (the thunks forward to the WUD functions whose log
 *   strings name them; the others from the field they read or write); driver internals `wpadShutdownCallback`, `wpadSendCommand`,
 *   `wpadUpdateRadioSensitivity`, `wpadCountIdleReports`, `wpadPollSample`, `wpadAlarmHandler`, `wpadResetChannel`, `wpadStart`,
 *   `wpadInitDoneCallback`, `wpadAbortCallback`, `wpadHidDataCallback`, `wpadAssignChannel`, `wpadHidOpenCallback`,
 *   `wpadCloseLinkCallback`, `wpadInfoCallback`, `wpadApplyDpdFormat`, `wpadSendWriteByte`, `wpadSendReadMemory`, `wpadQueueHasRoom`,
 *   `wpadCopyCurrentSample`, `wpadFilterButtons`, `wpadStoreSample`, `wpadGetAppType`, `wpadGetGameName`, `wpadSendWbcCommand`,
 *   `wpadSampleChanged`, `wpadSamplingFiber`, `wpadInitSequence`.  GUESS statics `wpadMoveHoldCount`/`wpadMoveDecayPeriod`
 *   (.sdata 0x807941A0/0x807941A8: the per-group hold and decay limits `wpadSampleChanged` compares the WPADCB counters
 *   in `moveCount`/`moveDecay` against).  `BTA_DmSendHciReset` (BTE/bta_dm_api.h; the map's `fn_8047DAC4`: it posts
 *   event 0x10C, whose bta_dm action-table slot 12 is `bta_dm_send_hci_reset`).  GUESS fields `WPADCB::used` (set when
 *   `wpadHidOpenCallback` opens the channel; the sampling fiber polls only used channels and `wpadAssignChannel` refuses
 *   a second board on channel 3) and `WPADCB::extStatusTimer` (the countdown the sampling fiber runs before it clears
 *   `extStatus`).
 *   GUESS `WPADControlDpd` (0x804F14A0: the SDK name by shape - KPAD's sampling callback passes a channel, a dpd command
 *   byte and a callback, and the body reads the dpd-enabled flag and the wanted format); GUESS `.rodata`
 *   `wpadDpdSensitivityBlockA/B` (0x80573C80/0x80573CB0: the camera sensitivity blocks it writes, indexed by the SC
 *   setting).  Struct fields are named from use (`WPADSample::balance.press` is `WPADBLStatus::press` at +0x2A: format 12
 *   compares four `lhz` load cells); the `unused_0xNN` fields (+0x8C4, +0x8C9, +0x910, +0x982) are only
 *   cleared by the written bodies.
 *   Statics are named from use (the four `wpadInitHook0..3` hooks `wpadStart` calls are separate `.sbss` words; the map
 *   rows `wpadInitHook0..3` are one word each); the `.sbss` rows `wpadChanStateA/B` are not understood
 *   yet.  Foreign names given here: WUD (`WUDInit`, `WUDShutdown`, `WUDGetStatus`,
 *   `WUDRegisterAllocator`, `WUDSet*Callback`, `WUDStart*`, `WUDStop*`, `WUDSetHidRecvCallback`, `WUDSetHidConnCallback`,
 *   `WUDIsSyncing`, `WUDIsHistoryAddr`, from the `WUD...()` log strings where one exists), SC (`SCGetWpadMotorMode`,
 *   `SCGetWpadSensorBarPosition`, `SCGetWpadSpeakerVolume`), `OSReturnToMenuPending`.
 * RESIDUALS. `WPADControlDpd`: the zero byte and the report id swap r12/r31 in two of its queued reports.
 *   `WPADControlSpeaker` (the first two one-byte writes of the "off" path) and `WPADControlDpd` (its three one-byte
 *   writes): the value/size bytes and the command and its by-value copy take each other's stack slots (alternate
 *   builder orders and a wrapper builder move the whole frame instead; every declaration order of a per-site one-byte
 *   builder, with the push inline or written out, leaves them).
 *   Register numbering only (the colouring order is read with tools/mwcc-debugger, playbook idea 244):
 *   `WPADiSendWriteData` (the push written out: `address` and `count` take each other's stack slots; every declaration
 *   order and a function-scope `item`/`level` leave it or move the frame);
 *   `wpadHidOpenCallback` (the push's `used` takes r28 for the original's r31, and the inlined pop's and peek's
 *   `level`/`used` swap r27/r28; a named `used`, split declarations and a caller-written push leave them); `wpadCountIdleReports` (`format` is a local or frontend temporary numbered below the
 *   backend `chan * 4`, which takes r3; the original gives r3 to `format`); `wpadFilterButtons` (the back-sample
 *   pointer takes r5 before the extension-code loads, the original r6); `wpadUpdateRadioSensitivity` (`average`
 *   takes r3, the original r5).
 *   `wpadSamplingFiber`: the loop's callee-saved registers and command stack slots are numbered differently.
 *   `wpadSampleChanged`: five callee-saved registers where the original uses four, and the original truncates `changed`
 *   after every debounce.  `changed = changed | wpadDebounceMove(...)` at all four sites reproduces the size and every
 *   instruction but colours `changed` r7 instead of r10 and scores lower; `=` at the first site and `|=` at the others
 *   scores best (8 bytes short); in the four-site form most of the macro's frontend temporaries colour differently
 *   (tools/mwcc-debugger/regsim.py search reaches 3 of 11 callee-saved targets with one edit).
 *   Flip blocker: .text (the partial rows).
 *   The data sections otherwise differ only in their aligned tails.
 * SHAPES. A command is a local `WPADCommand` passed by value to an inline push (`wpadQueuePush`): the 0x30-byte stack copy
 *   and the `memset` + `memcpy` into the ring are the by-value parameter.  The frame says where the command was built: one
 *   built in the function's own body lies above its copy, one built inside an inline builder (`wpadQueueLed`,
 *   `wpadQueueInfoRequest`, `wpadQueueStreamData`, `wpadQueueWriteByte`) usually lies below it (the residuals list the
 *   exceptions).  The one-byte builders store the report id first.  `wpadSendWriteByte` forwards to `wpadQueueWriteByte`;
 *   `wpadSendReadMemory` carries its own copy of the read body.  `wpadSendReadMemory`, `WPADiSendWriteData` and
 *   `WPADiSendSetReportType` (and the inline builders `wpadQueueReadMemoryCopy`/`wpadQueueDpdEnableCopy`, which
 *   `wpadHidOpenCallback` and `wpadInitSequence` use) write the push out in a block (`WPADCommand item = cmd;` is the by-value copy) so that
 *   `level` is a named local of the caller: the inline's `level` is a frontend temporary that colours after the stalled
 *   `queue` parameter.  `__wpadIsBusyStream` reads the queued and not-acked sizes through the inline
 *   `wpadGetQueuedSize`/`wpadGetNotAckedSize` (the first result is unused) and declares `level` before `cb`.  `wpadQueueCount` (an `s8` count, re-truncated after the
 *   wrap) is the shape the push, peek and pop share.  `wpadApplyDpdFormat` is queued as a completion callback, so its
 *   one-parameter prototype is cast to `WPADCallback`.  `u32 level = OSDisableInterrupts(); ... OSRestoreInterrupts(level);`
 *   brackets every field read; `-1 & ~((-a | a) >> 31)` is `a != 0 ? 0 : -1`.  `WPADDisconnect` re-reads the status
 *   through `wpadGetStatus`.  The sampling fiber's started/running dispatch is `if (status != 3) { if (status == 2) ... }
 *   else ...` (a switch compares 2 first and branches over a `b end`).  The movement test is the `WPAD_MOVED_BY` macro,
 *   not an inline: inline calls in an or-chain are expanded last-term first, while the macro's conditionals keep the
 *   original's order (second term, first, then the rest).  The report parser picks the back sample with
 *   `wpadSpareSample` (WPAD/wpad.h, `(u8)(index == 0)`); this file's own `wpadBackSample` spells the index
 *   `(u8)(index == 0 ? 1 : 0)`, which keeps the byte mask the original has here.  Register numbers follow the
 *   declarations (playbook idea 244): a second critical section takes its own block-local level
 *   (`reconnectLevel` in `wpadShutdownCallback`, `sendLevel` in `wpadSendWbcCommand`), `wpadDropLink` copies the handle into an `s8` (the
 *   field's type), `wpadPollSample` spells the back sample in place, `wpadUpdateRadioSensitivity` accumulates `sum` in
 *   two statements and `wpadInitSequence` picks `calSize` with `?:`.
 */

#include "types.h"
#include "WPAD/wpad.h"
#include "WPAD/WPADHIDParser.h"
#include "WPAD/WPADMem.h"
#include "WPAD/debug_msg.h"
#include "WUD/wud.h"
#include "BTE/bta_dm_api.h"
#include "BTE/bta_hh_api.h"
#include "BTE/gki_buffer.h"
#include "BTE/gki_time.h"
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
#include "OS/DCInvalidateRange.h"
#include "SC/SCSystemConfig.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"

/* The Wi-Fi channel the system leaves in low memory; the Bluetooth radio avoids it. */
#define WPAD_WIFI_CHANNEL (*(u8*)0x800031A2)

/* .bss */
static OSAlarm wpadAlarm;
WPADCB* wpadCBTable[4];
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
static void (*wpadInitHook0)(void);
static void (*wpadInitHook1)(void);
static void (*wpadInitHook2)(void);
static void (*wpadInitHook3)(void);
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
static u8 wpadSleepMinutes;
static char* wpadGameName;
static s8 wpadAppType;

/* .sdata */
static const char* wpadVersionString = "<< RVL_SDK - WPAD \trelease build: Jun 22 2009 18:33:21 (0x4302_145) >>";
static s32 wpadReconnectMode = -1;

/* The number of moving samples before an input group counts as changed: accelerometer, pointer, extension. */
static u16 wpadMoveHoldCount[4] = { 6, 4, 6 };

/* The number of still samples that decay a group's moving count by one. */
static u16 wpadMoveDecayPeriod[4] = { 30, 30, 30 };

/* Returns the number of commands waiting in `queue`. */
static inline s8 wpadQueueCount(WPADCmdQueue* queue)
{
    u32 level = OSDisableInterrupts();
    s8 used = queue->tail - queue->head;
    if (used < 0) {
        used = used + queue->capacity;
    }
    OSRestoreInterrupts(level);
    return used;
}

/* Returns the sample the next report fills (the one not currently published). */
static inline WPADSample* wpadBackSample(WPADCB* cb)
{
    return &cb->samples[(u8)(cb->sampleIndex == 0 ? 1 : 0)];
}

/* Pushes `cmd` onto `queue`; FALSE when the ring is full. */
static inline BOOL wpadQueuePush(WPADCmdQueue* queue, WPADCommand cmd)
{
    u32 level = OSDisableInterrupts();
    s8 used = wpadQueueCount(queue);

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

s32 WPADiReturnZeroC(u8 battery)
{
    return 0;
}

/* The balance board read stub: logs and fails. */
s32 WBCReadDummy(WPADBLStatus* status, f64* weight, s32 count)
{
    WPADiDebugPrint("WBCReadDummy\n");
    return -1;
}

s32 WBCSetZEROPointDummy(f64* zero, s32 count)
{
    WPADiDebugPrint("WBCSetZEROPointDummy\n");
    return -1;
}

s32 WBCGetTGCWeightDummy(f64 weight, f64* corrected, WPADBLStatus* status)
{
    WPADiDebugPrint("WBCGetTGCWeightDummy\n");
    return -1;
}

/* Queues the pointer camera's enable report (0x13) with the flag byte `value`. */
static inline BOOL wpadQueueDpdEnable(WPADCmdQueue* queue, u8 value, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x13;
    cmd.length = 1;
    cmd.data[0] = value;
    cmd.callback = callback;
    return wpadQueuePush(queue, cmd);
}

/* Queues the pointer camera's second enable report (0x1A) with the flag byte `value`. */
static inline BOOL wpadQueueDpdEnable2(WPADCmdQueue* queue, u8 value, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x1A;
    cmd.length = 1;
    cmd.data[0] = value;
    cmd.callback = callback;
    return wpadQueuePush(queue, cmd);
}

/* Queues the speaker enable report (0x14) with the flag byte `value`. */
static inline BOOL wpadQueueSpeakerEnable(WPADCmdQueue* queue, u8 value, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x14;
    cmd.length = 1;
    cmd.data[0] = value;
    cmd.callback = callback;
    return wpadQueuePush(queue, cmd);
}

/* Queues the speaker mute report (0x19) with the flag byte `value`. */
static inline BOOL wpadQueueSpeakerMute(WPADCmdQueue* queue, u8 value, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x19;
    cmd.length = 1;
    cmd.data[0] = value;
    cmd.callback = callback;
    return wpadQueuePush(queue, cmd);
}

/* Queues an LED command (report 0x11): `leds` in the high nibble. */
static inline BOOL wpadQueueLed(WPADCmdQueue* queue, u8 leds, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x11;
    cmd.length = 1;
    cmd.data[0] = (leds << 4) & 0xF0;
    cmd.callback = callback;
    return wpadQueuePush(queue, cmd);
}

/* Queues a status request (report 0x15). */
static inline BOOL wpadQueueStatusRequest(WPADCmdQueue* queue, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x15;
    cmd.length = 1;
    cmd.data[0] = 0;
    cmd.callback = callback;
    cmd.param = NULL;
    return wpadQueuePush(queue, cmd);
}

/* Queues a status request (report 0x15) whose reply fills `info`. */
static inline BOOL wpadQueueInfoRequest(WPADCmdQueue* queue, void* info, WPADCallback callback) /* untyped: the caller-owned status record */
{
    WPADCommand cmd;

    cmd.length = 1;
    cmd.data[0] = 0;
    cmd.reportId = 0x15;
    cmd.callback = callback;
    cmd.param = info;
    return wpadQueuePush(queue, cmd);
}

/* Queues one packet of speaker data (report 0x18): the byte count in the top five bits of the first byte. */
static inline BOOL wpadQueueStreamData(WPADCmdQueue* queue, const void* data, u32 length) /* untyped: byte range */
{
    WPADCommand cmd;

    cmd.reportId = 0x18;
    cmd.length = 0x15;
    cmd.data[0] = (length * 8) & 0xF8;
    cmd.callback = NULL;
    memcpy(&cmd.data[1], data, length);
    return wpadQueuePush(queue, cmd);
}

/* Queues a one-byte write of `value` to the remote address `addr`. */
static inline BOOL wpadQueueWriteByte(WPADCmdQueue* queue, u8 value, u32 addr, WPADCallback callback)
{
    WPADCommand cmd;
    u8 byteValue = value;
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

/* Queues a write of `size` bytes from `src` to the remote address `addr`. */
static inline BOOL wpadQueueWriteData(WPADCmdQueue* queue, const u8* src, s32 size, u32 addr, WPADCallback callback)
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
static inline s32 wpadQueueReadMemory(WPADCmdQueue* queue, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCommand cmd;
    u32 address = addr;
    u16 length = size;

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

/* Queues the same read as `wpadQueueReadMemory`, pushing a block-local copy of the command. */
static inline s32 wpadQueueReadMemoryCopy(WPADCmdQueue* queue, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCommand cmd;
    u32 address = addr;
    u16 length = size;

    cmd.reportId = 0x17;
    cmd.length = 6;
    cmd.callback = callback;
    memcpy(&cmd.data[0], &address, 4);
    memcpy(&cmd.data[4], &length, 2);
    cmd.argB = length;
    cmd.argA = (u32)dest;
    cmd.argC = address;
    {
        WPADCommand item = cmd;
        u32 level = OSDisableInterrupts();

        if (queue->capacity - 1 == (u32)wpadQueueCount(queue)) {
            OSRestoreInterrupts(level);
            return FALSE;
        }
        memset(&queue->items[queue->tail], 0, sizeof(WPADCommand));
        memcpy(&queue->items[queue->tail], &item, sizeof(WPADCommand));
        queue->tail = (queue->tail == queue->capacity - 1) ? 0 : queue->tail + 1;
        OSRestoreInterrupts(level);
        return TRUE;
    }
}

/* Queues the 0x1A dpd-enable report, pushing a block-local copy of the command. */
static inline BOOL wpadQueueDpdEnableCopy(WPADCmdQueue* queue, u8 value, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x1A;
    cmd.length = 1;
    cmd.data[0] = value;
    cmd.callback = callback;
    {
        WPADCommand item = cmd;
        u32 level = OSDisableInterrupts();

        if (queue->capacity - 1 == (u32)wpadQueueCount(queue)) {
            OSRestoreInterrupts(level);
            return FALSE;
        }
        memset(&queue->items[queue->tail], 0, sizeof(WPADCommand));
        memcpy(&queue->items[queue->tail], &item, sizeof(WPADCommand));
        queue->tail = (queue->tail == queue->capacity - 1) ? 0 : queue->tail + 1;
        OSRestoreInterrupts(level);
        return TRUE;
    }
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
        u32 reconnectLevel = OSDisableInterrupts();
        if (wpadInitialized == 0) {
            OSRestoreInterrupts(reconnectLevel);
        } else if (wpadShutdownRequested != 0) {
            OSRestoreInterrupts(reconnectLevel);
        } else {
            wpadShutdownRequested = 1;
            wpadReconnectMode = 1;
            WPADiDebugPrint("Wait for %d ms until start reconnect!\n", wpadReconnectDelayMs);
            OSRestoreInterrupts(reconnectLevel);
        }
    }
    return FALSE;
}

/* The shutdown record the OS list links. */
static OSShutdownFunctionInfo wpadShutdownInfo = { wpadShutdownCallback, 127, NULL, NULL };

/* Drops the Bluetooth link of the channel's device when it is connected. */
static inline void wpadDropLink(s32 chan)
{
    if (wpadGetStatus(chan) != -1) {
        WPADCB* cb = wpadCBTable[chan];
        u32 level = OSDisableInterrupts();
        s8 handle = cb->devHandle;
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
void wpadSendCommand(s32 chan, WPADCommand cmd)
{
    BT_HDR* buf;
    u32 level;
    s32 rumble;
    s8 handle;
    WPADCB* cb;
    u8* payload;
    u8 reportId;
    u8* data;
    s32 prevStatus;
    u16 length;

    length = cmd.length;
    cb = wpadCBTable[chan];
    reportId = (u8)cmd.reportId;
    data = cmd.data;
    level = OSDisableInterrupts();
    handle = cb->devHandle;
    prevStatus = cb->status;
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
            cb->replyB = cmd.argC;
            cb->replyD = cmd.argB;
            cb->replyA = cmd.argA;
            break;
        case 0x15:
            cb->status = prevStatus;
            cb->infoOut = cmd.param;
            cb->statusRequested = 1;
            break;
        default:
            data[0] = data[0] | 2;
            break;
        }
        cb->cmdCallback = cmd.callback;
        cb->lastReportId = reportId;
        cb->reconnectTime = __OSGetSystemTime() + (OS_BUS_CLOCK / 4) * 2;
        cb->unused_0x910 = 0;
    }
    OSRestoreInterrupts(level);
    WPADiDebugPrint("handle = %d, repid = %02x\n", handle, reportId);
    buf = GKI_getbuf((u8)(length + 0x12));
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
        u16 sum = (u16)(previous * 9);
        sum += (u16)((u32)(received * 100) >> 1);
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

/* 1 when two readings differ by more than `limit`, else 0. */
#define WPAD_MOVED_BY(a, b, limit) (((((a) - (b)) < 0 ? ((b) - (a)) : ((a) - (b))) > (limit)) ? 1 : 0)

/* Debounces one input group: TRUE once it moved on more consecutive samples than its hold count; a still sample
 * decays the count once per decay period. */
static inline BOOL wpadDebounceMove(WPADCB* cb, s32 moved, s32 group)
{
    if (moved) {
        if (++cb->moveCount[group] > wpadMoveHoldCount[group]) {
            cb->moveCount[group] = 0;
            cb->moveDecay[group] = 0;
            return TRUE;
        }
    } else {
        cb->moveDecay[group] = (cb->moveDecay[group] + 1) % wpadMoveDecayPeriod[group];
        if (cb->moveDecay[group] == wpadMoveDecayPeriod[group] - 1 && cb->moveCount[group] != 0) {
            cb->moveCount[group]--;
        }
    }
    return FALSE;
}

/* Compares a new sample with the last one: buttons, the accelerometer, the pointer and the extension's inputs. */
BOOL wpadSampleChanged(WPADCB* cb, WPADSample* sample, WPADSample* last)
{
    u8 changed = FALSE;
    s32 i;

    if (cb->dataFormat != 14 && (sample->extensionError == 0 || sample->extensionError == -7)
        && (last->extensionError == 0 || last->extensionError == -7)) {
        changed |= sample->buttons != last->buttons;
        changed = changed | wpadDebounceMove(cb,
                                    WPAD_MOVED_BY(sample->accX, last->accX, 12) | WPAD_MOVED_BY(sample->accY, last->accY, 12)
                                        | WPAD_MOVED_BY(sample->accZ, last->accZ, 12),
                                    0);
        changed |= wpadDebounceMove(cb,
                                    WPAD_MOVED_BY(sample->dpd[0].x, last->dpd[0].x, 2)
                                        | WPAD_MOVED_BY(sample->dpd[0].y, last->dpd[0].y, 2)
                                        | WPAD_MOVED_BY(sample->dpd[1].x, last->dpd[1].x, 2)
                                        | WPAD_MOVED_BY(sample->dpd[1].y, last->dpd[1].y, 2)
                                        | WPAD_MOVED_BY(sample->dpd[2].x, last->dpd[2].x, 2)
                                        | WPAD_MOVED_BY(sample->dpd[2].y, last->dpd[2].y, 2)
                                        | WPAD_MOVED_BY(sample->dpd[3].x, last->dpd[3].x, 2)
                                        | WPAD_MOVED_BY(sample->dpd[3].y, last->dpd[3].y, 2),
                                    1);
    }
    if (sample->extensionError == 0 && last->extensionError == 0) {
        switch (cb->dataFormat) {
        case 3:
        case 4:
        case 5:
            changed |= wpadDebounceMove(cb,
                                        WPAD_MOVED_BY(sample->nunchuk.accX, last->nunchuk.accX, 12)
                                            | WPAD_MOVED_BY(sample->nunchuk.accY, last->nunchuk.accY, 12)
                                            | WPAD_MOVED_BY(sample->nunchuk.accZ, last->nunchuk.accZ, 12),
                                        2);
            changed |= WPAD_MOVED_BY(sample->nunchuk.stickX, last->nunchuk.stickX, 1);
            changed |= WPAD_MOVED_BY(sample->nunchuk.stickY, last->nunchuk.stickY, 1);
            break;
        case 6:
        case 7:
        case 8:
        case 11:
        case 15: {
            s32 stickDiv;
            s32 rightDiv;
            s32 triggerDiv;

            switch (cb->extensionSubType) {
            case 1:
                stickDiv = 16;
                rightDiv = 32;
                triggerDiv = 8;
                break;
            case 3:
                stickDiv = 4;
                rightDiv = 4;
                triggerDiv = 1;
                break;
            default:
                stickDiv = 1;
                rightDiv = 1;
                triggerDiv = 1;
                break;
            }
            changed |= sample->classic.buttons != last->classic.buttons;
            changed |= WPAD_MOVED_BY(sample->classic.leftX / stickDiv, last->classic.leftX / stickDiv, 1);
            changed |= WPAD_MOVED_BY(sample->classic.leftY / stickDiv, last->classic.leftY / stickDiv, 1);
            changed |= WPAD_MOVED_BY(sample->classic.rightX / rightDiv, last->classic.rightX / rightDiv, 1);
            changed |= WPAD_MOVED_BY(sample->classic.rightY / rightDiv, last->classic.rightY / rightDiv, 1);
            changed |= WPAD_MOVED_BY(sample->classic.leftTrigger / triggerDiv, last->classic.leftTrigger / triggerDiv, 1);
            changed |= WPAD_MOVED_BY(sample->classic.rightTrigger / triggerDiv, last->classic.rightTrigger / triggerDiv, 1);
            break;
        }
        case 10:
            changed |= sample->simple.buttons != last->simple.buttons;
            changed |= WPAD_MOVED_BY(sample->simple.stickX, last->simple.stickX, 1);
            changed |= WPAD_MOVED_BY(sample->simple.stickY, last->simple.stickY, 1);
            break;
        case 12:
            changed |= wpadDebounceMove(cb,
                                        WPAD_MOVED_BY(sample->balance.press[0], last->balance.press[0], 50)
                                            | WPAD_MOVED_BY(sample->balance.press[1], last->balance.press[1], 50)
                                            | WPAD_MOVED_BY(sample->balance.press[2], last->balance.press[2], 50)
                                            | WPAD_MOVED_BY(sample->balance.press[3], last->balance.press[3], 50),
                                        2);
            break;
        case 13:
        case 14:
            changed |= sample->board.refC != last->board.refC;
            changed |= WPAD_MOVED_BY(sample->board.refB, last->board.refB, 32);
            for (i = 0; i < 5; i++) {
                changed |= WPAD_MOVED_BY(sample->board.weight[i], sample->board.weightRef[i], 32);
            }
            break;
        }
    }
    return changed;
}

/* Counts consecutive reports with no input and clears the flag once the count passes the limit. */
void wpadCountIdleReports(s32 chan, WPADSample* sample)
{
    u32 format;
    WPADCB* cb;
    s32 idle;
    u16 count;

    cb = wpadCBTable[chan];
    idle = 0;
    format = cb->dataFormat;
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
        WPADSample* sample = &cb->samples[(u8)(cb->sampleIndex == 0 ? 1 : 0)];
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

/* Copies the oldest command of `queue` into `cmd`; FALSE when the queue is empty. */
static inline BOOL wpadQueuePeek(WPADCmdQueue* queue, WPADCommand* cmd)
{
    u32 level;

    if (wpadQueueCount(queue) == 0) {
        return FALSE;
    }
    level = OSDisableInterrupts();
    memcpy(cmd, &queue->items[queue->head], sizeof(WPADCommand));
    OSRestoreInterrupts(level);
    return TRUE;
}

/* Drops the oldest command of `queue`; FALSE when the queue is empty. */
static inline BOOL wpadQueuePop(WPADCmdQueue* queue)
{
    u32 level = OSDisableInterrupts();

    if (wpadQueueCount(queue) == 0) {
        OSRestoreInterrupts(level);
        return FALSE;
    }
    memset(&queue->items[queue->head], 0, sizeof(WPADCommand));
    queue->head = (queue->head == queue->capacity - 1) ? 0 : queue->head + 1;
    OSRestoreInterrupts(level);
    return TRUE;
}

/* Sends `cmd` when the channel is idle and drops the link of a remote that stays busy; returns the channel status. */
static inline s32 wpadSendWhenIdle(s32 chan, WPADCommand cmd)
{
    WPADCB* cb = wpadCBTable[chan];
    s32 status;
    u32 level = OSDisableInterrupts();

    if (cb->statusRequested != 0) {
        status = -2;
    } else {
        status = wpadGetStatus(chan);
    }
    OSRestoreInterrupts(level);
    if (status == 0) {
        wpadSendCommand(chan, cmd);
    }
    if (status == -2) {
        if ((s32)((__OSGetSystemTime() - cb->reconnectTime) / (OS_BUS_CLOCK >> 2)) > 1 && wpadSleepMinutes != 0) {
            cb->reconnectTime = __OSGetSystemTime();
            wpadDropLink(chan);
        }
    }
    return status;
}

/* Sends the oldest command of `queue` and drops it once sent; TRUE when a command went out. */
static inline BOOL wpadSendQueued(s32 chan, WPADCmdQueue* queue)
{
    WPADCommand cmd;

    if (wpadQueuePeek(queue, &cmd)) {
        if (wpadSendWhenIdle(chan, cmd) == 0) {
            wpadQueuePop(queue);
            return TRUE;
        }
    }
    return FALSE;
}

/* Counts down the reconnect delay after a shutdown request and restarts the stack when it expires; FALSE when no
 * reconnect is pending. */
static inline BOOL wpadReconnectPending(void)
{
    s32 mode = wpadReconnectMode;

    if (mode >= 0) {
        if (--wpadReconnectDelayMs <= 0) {
            WPADiDebugPrint("Reconnect Start!!\n");
            BTA_DmSendHciReset();
            OSCancelAlarm(&wpadAlarm);
            WUDSetHidRecvCallback(NULL);
            WUDShutdown(mode);
        }
        return TRUE;
    }
    return FALSE;
}

/* The periodic sampling work: starts the stack, follows the system settings, sends one queued command per channel
 * and samples every connected remote. */
void wpadSamplingFiber(void)
{
    s32 chan;
    s32 status;

    status = WUDGetStatus();
    if (status != 3) {
        if (status == 2) {
            if (wpadStackStarted == 0) {
                wpadStackStarted = 1;
                wpadReconnectDelayMs = 50;
                WUDSetHidConnCallback((WUDHidConnCallback)wpadHidOpenCallback);
                WUDSetHidRecvCallback(wpadHidDataCallback);
            }
        }
    } else if (!wpadReconnectPending()) {
        if (wpadWifiCheckTick == 60000) {
            DCInvalidateRange(&WPAD_WIFI_CHANNEL, 1);
            WPADiDebugPrint("Check the update of WiFi using channel\n");
            WPADiDebugPrint("WiFi uses channel = %d\n", WPAD_WIFI_CHANNEL);
            if (wpadWifiChannel != WPAD_WIFI_CHANNEL) {
                u32 level = OSDisableInterrupts();
                wpadWifiChannel = WPAD_WIFI_CHANNEL;
                OSRestoreInterrupts(level);
                WUDSetDisableChannel(wpadWifiChannel);
            }
        }
        if (SCCheckStatus() == 0) {
            WUDiFlushDeviceInfo();
            if (wpadWifiCheckEnabled != 0) {
                u8 sensitivity = SCGetBtDpdSensibility();
                u8 setting;
                u8 volume;

                if (sensitivity < 1) {
                    sensitivity = 1;
                }
                if (sensitivity > 5) {
                    sensitivity = 5;
                }
                wpadDpdSensitivity = sensitivity;
                wpadSensorBarTop = SCGetWpadSensorBarPosition() == 1;
                wpadMotorEnabled = SCGetWpadMotorMode() == 1;
                setting = SCGetWpadSpeakerVolume();
                volume = setting;
                if (setting <= 0) {
                    volume = 0;
                }
                if (setting >= 127) {
                    volume = 127;
                }
                wpadSpeakerVolume = volume;
                wpadWifiCheckEnabled = 0;
            }
        }
        for (chan = 0; chan < 4; chan++) {
            WPADCB* cb = wpadCBTable[chan];

            if (cb->used != 0) {
                BOOL sentAux = FALSE;
                BOOL sent;

                if (wpadChanStateA[chan] == 5) {
                    WPADCommand cmd;

                    if (wpadQueuePeek(&cb->auxQueue, &cmd) && (cmd.reportId == 0x12 || cb->flags.extensionAttached != 0)) {
                        if (wpadSendWhenIdle(chan, cmd) == 0) {
                            wpadQueuePop(&cb->auxQueue);
                            sentAux = TRUE;
                            wpadChanStateA[chan] = 0;
                        }
                    }
                }
                wpadChanStateA[chan] = (wpadChanStateA[chan] == 5) ? wpadChanStateA[chan] : wpadChanStateA[chan] + 1;
                {
                    WPADCB* mainCb = wpadCBTable[chan];
                    if (sentAux) {
                        sent = TRUE;
                    } else {
                        sent = wpadSendQueued(chan, &mainCb->cmdQueue);
                    }
                }
                if ((sentAux | sent) == TRUE || wpadQueueCount(&wpadCBTable[chan]->cmdQueue) > 0) {
                    wpadCBTable[chan]->motorDirty = 0;
                } else if (wpadChanStateB[chan] == 5) {
                    WPADCommand cmd;

                    cmd.reportId = 0x10;
                    cmd.length = 1;
                    wpadCBTable[chan]->motorDirty = 0;
                    cmd.data[0] = 0;
                    cmd.callback = NULL;
                    wpadSendCommand(chan, cmd);
                }
                wpadChanStateB[chan] = (wpadCBTable[chan]->motorDirty != 0) ? wpadChanStateB[chan] + 1 : 0;
                wpadPollSample(chan);
                wpadUpdateRadioSensitivity(chan);
            }
            if (wpadCBTable[chan]->extStatus != 0) {
                if (wpadCBTable[chan]->extStatusTimer-- < 0) {
                    wpadCBTable[chan]->extStatus = 0;
                }
            }
        }
        wpadRadioTick = (wpadRadioTick == 10) ? 0 : wpadRadioTick + 1;
        wpadPollState = (wpadPollState == 5) ? 0 : wpadPollState + 1;
        wpadWifiCheckTick = (wpadWifiCheckTick == 60000) ? 0 : wpadWifiCheckTick + 1;
        BTA_HhGetAclQueueInfo();
    }
}

/* The periodic alarm handler: runs the sampling fiber on its own stack. */
void wpadAlarmHandler(OSAlarm* alarm, OSContext* context)
{
    OSSwitchFiberEx((u32)alarm, (u32)context, 0, 0, wpadSamplingFiber, wpadFiberStack + sizeof(wpadFiberStack));
}

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
    cb->moveCount[0] = 0;
    cb->moveDecay[0] = 0;
    cb->moveCount[1] = 0;
    cb->moveDecay[1] = 0;
    cb->moveCount[2] = 0;
    cb->moveDecay[2] = 0;
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
    cb->used = 0;
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
    cb->extStatus = 0;
    cb->extStatusTimer = 0;
    memset(&cb->flags, 0, sizeof(cb->flags));
    memset(cb->extId, 0, 0x40);
    memset(&cb->calA, 0, sizeof(cb->calA));
    memset(&cb->calB, 0, sizeof(cb->calB));
    memset(cb->key, 0, 0x10);
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
    WPADiClearMemBlock(chan);
    wpadChanStateA[chan] = 0;
    wpadChanStateB[chan] = 0;
}

/* Brings the driver up: powers the sensor bar, resets the four channels, reads the SC settings and arms the poll alarm. */
void wpadStart(void)
{
    u32 level = OSDisableInterrupts();
    s32 i;
    u8 sensitivity;
    u8 setting;
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
    setting = SCGetWpadSpeakerVolume();
    volume = setting;
    if (setting <= 0) {
        volume = 0;
    }
    if (setting >= 0x7F) {
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
    if (wpadInitHook0 != NULL) {
        wpadInitHook0();
    }
    if (wpadInitHook1 != NULL) {
        wpadInitHook1();
    }
    if (wpadInitHook2 != NULL) {
        wpadInitHook2();
    }
    if (wpadInitHook3 != NULL) {
        wpadInitHook3();
    }
    OSCreateAlarm(&wpadAlarm);
    OSSetPeriodicAlarm(&wpadAlarm, OSGetTime(), (OS_BUS_CLOCK >> 2) / 1000, wpadAlarmHandler);
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

/* Queues the connect sequence of a remote: the report type, the LEDs, the two memory blocks and the calibration
 * block the firmware generation selects, then a status request. */
void wpadInitSequence(s32 chan, s32 result)
{
    u16 calSize;
    WPADCB* cb = wpadCBTable[chan];
    u32 calAddr;
    u8 leds;
    u32 level;

    if (result == -1) {
        return;
    }
    level = OSDisableInterrupts();
    cb->calibrationPhase = result == 0;
    cb->status = 0;
    OSRestoreInterrupts(level);
    calSize = (result == 0) ? 20 : 42;
    calAddr = (result == 0) ? 0x176C : 0;
    WPADiDebugPrint(" ==>this error means that the firmware is for NDEV %s\n", cb->calibrationPhase != 0 ? "2.0" : "2.1 or later");
    leds = (cb->deviceType == 3) ? 1 : (1 << chan);
    WPADiSendSetReportType(&cb->cmdQueue, 0, cb->reportOnChange, wpadAbortCallback);
    wpadQueueDpdEnableCopy(&cb->cmdQueue, 0, wpadAbortCallback);
    wpadQueueLed(&cb->cmdQueue, leds, wpadAbortCallback);
    wpadQueueReadMemory(&cb->cmdQueue, cb->extId, 0x38, 0x2A, wpadAbortCallback);
    wpadQueueReadMemory(&cb->cmdQueue, cb->extId, 0x38, 0x62, wpadAbortCallback);
    wpadQueueReadMemory(&cb->cmdQueue, cb->extId, calSize, calAddr, wpadInitDoneCallback);
    wpadQueueStatusRequest(&cb->cmdQueue, NULL);
}

/* Picks the channel for a newly opened device: the balance board always takes channel 3, a remote takes its history slot. */
s32 wpadAssignChannel(WPADDeviceInfo* info)
{
    const u8* addr;
    s32 i;
    s32 chan = -1;

    addr = _WUDGetDevAddr(info->handle);
    if (memcmp(info, "Nintendo RVL-WBC", 0x10) == 0) {
        chan = 3;
        if (wpadCBTable[3]->used != 0) {
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
    WPADCB* cb;
    u8 handle;
    s32 chan;

    handle = info->handle;
    WPADiDebugPrint("connection is %s\n", connected != 0 ? "opened" : "closed");
    if (connected != 0) {
        chan = wpadAssignChannel(info);
        if (chan >= 0) {
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
            cb->used = 1;
            cb->status = 0;
            cb->radioSensitivity = 100;
            cb->disconnecting = 0;
            cb->extInitState = 0;
            cb->extensionResult = cb->deviceType;
            wpadQueueReadMemoryCopy(&cb->cmdQueue, cb->extId, 1, 0x1770, wpadInitSequence);
            __VIResetDimmingControlA();
        }
    } else {
        chan = wpadHandleToChan[handle];
        wpadHandleToChan[handle] = -1;
        if (chan != -1) {
            WPADCommand pending;
            cb = wpadCBTable[chan];
            cb->status = -1;
            if (cb->cmdCallback != NULL) {
                cb->cmdCallback(chan, -1);
            } else if (cb->bulkDoneCallback != NULL) {
                cb->bulkDoneCallback(chan, -1);
            }
            while (wpadQueuePeek(&cb->cmdQueue, &pending)) {
                if (pending.callback != NULL) {
                    pending.callback(chan, -1);
                }
                wpadQueuePop(&cb->cmdQueue);
            }
            WPADiDebugPrint("clean up command queue\n");
            if (cb->autoSampleBuf != NULL) {
                WPADSetAutoSamplingBuf(chan, cb->autoSampleBuf, cb->autoSampleCount);
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
void WPADGetAccGravityUnit(s32 chan, u32 type, WPADAccGravityUnit* unit)
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

        WUDSetDeviceHistory(chan, NULL);
        cb = wpadCBTable[chan];
        if (wpadGetStatus(chan) != -1) {
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
    s32 status;
    s32 ready;
    u32 current;

    ready = cb->ready;
    status = cb->status;
    current = cb->dataFormat;
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
            level = OSDisableInterrupts();
            cb->infoPending = 1;
            cb->infoCallback = callback;
            OSRestoreInterrupts(level);
            if (wpadQueueInfoRequest(&cb->cmdQueue, info, wpadInfoCallback)) {
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
    WPADCB* cb;
    u32 level;
    s32 ready;
    s32 status;

    cb = wpadCBTable[chan];
    level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else {
            status = wpadQueueLed(&cb->cmdQueue, leds, callback) ? 0 : -2;
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
    s32 motor;
    u8 volume;
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
    WPADSample* sample = wpadBackSample(cb);
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

#pragma push
#pragma opt_strength_reduction off
/* Points the channel at a ring of `count` samples that it fills after every report. */
void WPADSetAutoSamplingBuf(s32 chan, void* buf, u32 count) /* untyped: caller-owned ring of samples */
{
    s8 fill;
    s32 size;
    WPADCB* cb;
    u32 level;
    s32 i;

    cb = wpadCBTable[chan];
    level = OSDisableInterrupts();
    fill = -4;
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
#pragma pop

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
    WPADSample* sample = wpadBackSample(cb);
    u32 format;

    if ((sample->buttons & 3) == 3) {
        sample->buttons = sample->buttons & ~2;
    }
    if ((sample->buttons & 0xC) == 0xC) {
        sample->buttons = sample->buttons & ~4;
    }
    format = cb->dataFormat;
    if ((u32)(format - 6) <= 2 || format == 11 || format == 15) {
        if ((sample->extensionCode & 0x8002) == 0x8002) {
            sample->extensionCode = sample->extensionCode & 0x7FFF;
        }
        if ((sample->extensionCode & 0x4001) == 0x4001) {
            sample->extensionCode = sample->extensionCode & ~0x4000;
        }
    }
    if (cb->dataFormat == 10) {
        if ((sample->extensionCode & 0x8002) == 0x8002) {
            sample->extensionCode = sample->extensionCode & 0x7FFF;
        }
        if ((sample->extensionCode & 0x4001) == 0x4001) {
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
    WPADSample* sample = wpadBackSample(cb);
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

/* Drives the speaker: off (0), on with the configured volume (1, 5), mute on/off (2, 3) and the play flag (4). Reports
 * the result to `callback` unless the commands were queued. */
s32 WPADControlSpeaker(s32 chan, u32 command, WPADCallback callback)
{
    u8 config[7] = { 0x00, 0x00, 0xD0, 0x07, 0x40, 0x0C, 0x0E };
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 enabled;
    s32 ready;
    s32 status;

    enabled = cb->flags.speakerEnabled;
    status = cb->status;
    ready = cb->ready;

    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else if (command == 0) {
            if (enabled == 0) {
                status = 0;
            } else {
                level = OSDisableInterrupts();
                if ((u32)(wpadQueueCount(&cb->cmdQueue) + 5) <= (u32)(cb->cmdQueue.capacity - 1)) {
                    wpadQueueSpeakerMute(&cb->cmdQueue, 4, NULL);
                    wpadQueueWriteByte(&cb->cmdQueue, 1, 0x04A20001, NULL);
                    wpadQueueWriteByte(&cb->cmdQueue, 0, 0x04A20009, NULL);
                    wpadQueueSpeakerEnable(&cb->cmdQueue, 0, NULL);
                    wpadQueueStatusRequest(&cb->cmdQueue, callback);
                    OSRestoreInterrupts(level);
                    return 0;
                }
                status = -2;
                OSRestoreInterrupts(level);
            }
        } else {
            switch (command) {
            case 1:
            case 5:
                level = OSDisableInterrupts();
                if ((u32)(wpadQueueCount(&cb->cmdQueue) + 7) <= (u32)(cb->cmdQueue.capacity - 1)) {
                    wpadQueueSpeakerEnable(&cb->cmdQueue, 4, NULL);
                    wpadQueueSpeakerMute(&cb->cmdQueue, 4, NULL);
                    wpadQueueWriteByte(&cb->cmdQueue, 1, 0x04A20009, NULL);
                    wpadQueueWriteByte(&cb->cmdQueue, 0x80, 0x04A20001, NULL);
                    config[4] = wpadSpeakerVolume;
                    wpadQueueWriteData(&cb->cmdQueue, config, 7, 0x04A20001, NULL);
                    wpadQueueSpeakerMute(&cb->cmdQueue, 0, NULL);
                    wpadQueueStatusRequest(&cb->cmdQueue, callback);
                    OSRestoreInterrupts(level);
                    return 0;
                }
                status = -2;
                OSRestoreInterrupts(level);
                break;
            case 2:
                if (!wpadQueueSpeakerMute(&cb->cmdQueue, 4, callback)) {
                    status = -2;
                    break;
                }
                return 0;
            case 3:
                if (!wpadQueueSpeakerMute(&cb->cmdQueue, 0, callback)) {
                    status = -2;
                    break;
                }
                return 0;
            case 4:
                if (!wpadQueueWriteByte(&cb->cmdQueue, 1, 0x04A20008, callback)) {
                    status = -2;
                    break;
                }
                return 0;
            }
        }
    }
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
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
static inline u16 wpadGetQueuedSize(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s8 handle = cb->devHandle;
    OSRestoreInterrupts(level);
    if (status == -1) {
        return 0;
    }
    return _WUDGetQueuedSize(handle);
}

static inline u16 wpadGetNotAckedSize(s32 chan)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 status = cb->status;
    s8 handle = cb->devHandle;
    OSRestoreInterrupts(level);
    if (status == -1) {
        return 0;
    }
    return _WUDGetNotAckedSize(handle);
}

BOOL __wpadIsBusyStream(s32 chan)
{
    u32 level;
    WPADCB* cb = wpadCBTable[chan];
    u8 weakRadio;
    s8 queued;
    u8 bufferStatus;
    u8 deviceType;
    u16 queuedSize;
    u16 notAcked;
    u8 pending;
    u8 links;

    level = OSDisableInterrupts();
    weakRadio = cb->weakRadio;
    deviceType = cb->deviceType;
    bufferStatus = WUDGetBufferStatus();
    queued = wpadQueueCount(&cb->cmdQueue);
    queuedSize = wpadGetQueuedSize(chan);
    notAcked = wpadGetNotAckedSize(chan);
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
    WPADCB* cb;
    u32 level;
    s32 ready;
    s32 status;

    cb = wpadCBTable[chan];
    level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
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
    if (!wpadQueueStreamData(&cb->cmdQueue, data, length)) {
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

/* The pointer camera's sensitivity blocks written by `WPADControlDpd`, one row per sensitivity setting (1..5). */
static const u8 wpadDpdSensitivityBlockA[5][9] = {
    { 0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0x64, 0x00, 0xFE },
    { 0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0x96, 0x00, 0xB4 },
    { 0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0xAA, 0x00, 0x64 },
    { 0x02, 0x00, 0x00, 0x71, 0x01, 0x00, 0xC8, 0x00, 0x36 },
    { 0x07, 0x00, 0x00, 0x71, 0x01, 0x00, 0x72, 0x00, 0x20 },
};
static const u8 wpadDpdSensitivityBlockB[5][2] = {
    { 0xFD, 0x05 }, { 0xB3, 0x04 }, { 0x63, 0x03 }, { 0x35, 0x03 }, { 0x1F, 0x03 },
};

/* Turns the pointer camera off (`command` 0) or programs its sensitivity blocks and switches it to the format `command`;
 * reports the result to `callback` unless the commands were queued. */
s32 WPADControlDpd(s32 chan, u32 command, WPADCallback callback)
{
    WPADCB* cb = wpadCBTable[chan];
    u32 level = OSDisableInterrupts();
    s32 enabled;
    s32 ready;
    u8 wanted;
    s32 status;

    enabled = cb->flags.dpdEnabled;
    wanted = cb->dpdFormatWanted;
    status = cb->status;
    ready = cb->ready;

    OSRestoreInterrupts(level);
    if (status != -1) {
        if (ready == 0) {
            status = -2;
        } else if (command == 0) {
            if (enabled == 0) {
                status = 0;
            } else {
                level = OSDisableInterrupts();
                if ((u32)(wpadQueueCount(&cb->cmdQueue) + 3) <= (u32)(cb->cmdQueue.capacity - 1)) {
                    cb->dpdFormatWanted = command;
                    wpadQueueDpdEnable(&cb->cmdQueue, 0, NULL);
                    wpadQueueDpdEnable2(&cb->cmdQueue, 0, (WPADCallback)wpadApplyDpdFormat);
                    wpadQueueStatusRequest(&cb->cmdQueue, callback);
                    OSRestoreInterrupts(level);
                    return 0;
                }
                status = -2;
                OSRestoreInterrupts(level);
            }
        } else if (command != wanted) {
            level = OSDisableInterrupts();
            if ((u32)(wpadQueueCount(&cb->cmdQueue) + 8) <= (u32)(cb->cmdQueue.capacity - 1)) {
                cb->dpdFormatWanted = command;
                cb->dpdBlocked = 1;
                wpadQueueDpdEnable(&cb->cmdQueue, 4, NULL);
                wpadQueueDpdEnable2(&cb->cmdQueue, 4, NULL);
                wpadQueueWriteByte(&cb->cmdQueue, 1, 0x04B00030, NULL);
                wpadQueueWriteData(&cb->cmdQueue, wpadDpdSensitivityBlockA[wpadDpdSensitivity - 1], 9, 0x04B00000, NULL);
                wpadQueueWriteData(&cb->cmdQueue, wpadDpdSensitivityBlockB[wpadDpdSensitivity - 1], 2, 0x04B0001A, NULL);
                wpadQueueWriteByte(&cb->cmdQueue, command, 0x04B00033, NULL);
                wpadQueueWriteByte(&cb->cmdQueue, 8, 0x04B00030, (WPADCallback)wpadApplyDpdFormat);
                wpadQueueStatusRequest(&cb->cmdQueue, callback);
                OSRestoreInterrupts(level);
                return 0;
            }
            status = -2;
            OSRestoreInterrupts(level);
        }
    }
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}


/* Queues a set-data-reporting-mode command; FALSE when the queue is full. */
s32 WPADiSendSetReportType(WPADCmdQueue* queue, u32 type, s32 onChangeOnly, WPADCallback callback)
{
    WPADCommand cmd;

    cmd.reportId = 0x12;
    cmd.length = 2;
    cmd.data[0] = onChangeOnly != 0 ? 0 : 4;
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
    {
        WPADCommand item = cmd;
        u32 level = OSDisableInterrupts();

        if (queue->capacity - 1 == (u32)wpadQueueCount(queue)) {
            OSRestoreInterrupts(level);
            return FALSE;
        }
        memset(&queue->items[queue->tail], 0, sizeof(WPADCommand));
        memcpy(&queue->items[queue->tail], &item, sizeof(WPADCommand));
        queue->tail = (queue->tail == queue->capacity - 1) ? 0 : queue->tail + 1;
        OSRestoreInterrupts(level);
        return TRUE;
    }
}

/* Queues a one-byte write of `value` to the remote address `addr`. */
s32 wpadSendWriteByte(WPADCmdQueue* queue, u8 value, u32 addr, WPADCallback callback)
{
    return wpadQueueWriteByte(queue, value, addr, callback);
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
    {
        WPADCommand item = cmd;
        u32 level = OSDisableInterrupts();

        if (queue->capacity - 1 == (u32)wpadQueueCount(queue)) {
            OSRestoreInterrupts(level);
            return FALSE;
        }
        memset(&queue->items[queue->tail], 0, sizeof(WPADCommand));
        memcpy(&queue->items[queue->tail], &item, sizeof(WPADCommand));
        queue->tail = (queue->tail == queue->capacity - 1) ? 0 : queue->tail + 1;
        OSRestoreInterrupts(level);
        return TRUE;
    }
}

/* Queues a read of `size` bytes from the remote address `addr` into `dest`. */
s32 wpadSendReadMemory(WPADCmdQueue* queue, void* dest, u16 size, u32 addr, WPADCallback callback) /* untyped: caller-owned byte buffer */
{
    WPADCommand cmd;
    u32 address = addr;
    u16 length = size;

    cmd.reportId = 0x17;
    cmd.length = 6;
    cmd.callback = callback;
    memcpy(&cmd.data[0], &address, 4);
    memcpy(&cmd.data[4], &length, 2);
    cmd.argB = length;
    cmd.argA = (u32)dest;
    cmd.argC = address;
    {
        WPADCommand item = cmd;
        u32 level = OSDisableInterrupts();

        if (queue->capacity - 1 == (u32)wpadQueueCount(queue)) {
            OSRestoreInterrupts(level);
            return FALSE;
        }
        memset(&queue->items[queue->tail], 0, sizeof(WPADCommand));
        memcpy(&queue->items[queue->tail], &item, sizeof(WPADCommand));
        queue->tail = (queue->tail == queue->capacity - 1) ? 0 : queue->tail + 1;
        OSRestoreInterrupts(level);
        return TRUE;
    }
}

/* Whether `count` more commands fit in the queue. */
s32 wpadQueueHasRoom(WPADCmdQueue* queue, s8 count)
{
    s8 used = wpadQueueCount(queue);

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

/* Sends the balance-board handshake bytes to the extension register through the write queue. */
s32 wpadSendWbcCommand(s32 chan, s32 command, WPADCallback callback)
{
    u32 level;
    s32 status;
    s32 ready;
    WPADCB* cb;
    u8 data[7];

    cb = wpadCBTable[chan];
    level = OSDisableInterrupts();
    status = cb->status;
    ready = cb->ready;
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
            u32 sendLevel = OSDisableInterrupts();
            switch (command) {
            case 0xAA: {
                s8 used;
                u32 inner = OSDisableInterrupts();
                used = cb->cmdQueue.tail - cb->cmdQueue.head;
                if (used < 0) {
                    used = used + cb->cmdQueue.capacity;
                }
                OSRestoreInterrupts(inner);
                if ((u32)(used + 4) <= (u32)(cb->cmdQueue.capacity - 1)) {
                    WPADWriteExtReg(chan, data, 7, 0xF1, NULL);
                    WPADWriteExtReg(chan, data, 1, 0xF1, NULL);
                    WPADWriteExtReg(chan, data, 1, 0xF1, NULL);
                    WPADWriteExtReg(chan, data, 1, 0xF1, callback);
                    OSRestoreInterrupts(sendLevel);
                    return 0;
                }
                status = -2;
                break;
            }
            case 0x55:
                status = WPADWriteExtReg(chan, data, 7, 0xF1, callback);
                if (status == 0) {
                    OSRestoreInterrupts(sendLevel);
                    return 0;
                }
                break;
            case 0:
                status = WPADWriteExtReg(chan, data, 1, 0xF1, callback);
                if (status == 0) {
                    OSRestoreInterrupts(sendLevel);
                    return 0;
                }
                break;
            default:
                status = -2;
                break;
            }
            OSRestoreInterrupts(sendLevel);
        }
    }
    if (callback != NULL) {
        callback(chan, status);
    }
    return status;
}
