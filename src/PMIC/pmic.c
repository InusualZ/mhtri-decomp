/*
 * PMIC/pmic.c - the PMIC library: the USB device bring-up (`PMICInit`) with its firmware image, the transfer state
 *    machine and the sample filter.
 *
 * RANGE. .text 0x80522E00-0x80526C90 (53 functions, 0x3E90 B); .data 0x80631468-0x806492D8; .bss
 *    0x8078FA38-0x8078FF40; .sdata 0x80794460-0x80794498; .sbss 0x80795938-0x807959F0; .sdata2
 *    0x8079D528-0x8079D550.  Cut from the old VF block between `TRK/exi2_comm.c` (0x80522E00) and `KPR/kpr.c`
 *    (0x80526C90).  COARSE: the library's source files are not separated.
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. only the library name `PMIC` is known (the build string); `PMICInit` is a GUESS for 0x80522E00 (it is the
 *    registrant of the build string) and every other `PMIC*` name, the state, request, ring and filter names are
 *    GUESSES from what the bodies do; the file name `pmic.c` is a GUESS.
 * EVIDENCE. `.data` 0x80631468..0x806492D8 is one image (0x17CC8 B, `pmic_firmware.inc`) whose address `PMICInit`
 *    keeps, followed by the build string `<< RVL_SDK - PMIC release build ... (0x4302_145) >>` (read through
 *    `.sdata` 0x80794460 by `OSRegisterVersion`), the `PMICInit() : too busy` and `/dev/usb/oh0` strings and the
 *    device error string; `.sbss` 0x80795938..0x807959F0 (the state word 0x80795938 is read by 35 functions) and
 *    `.bss` 0x8078FA38..0x8078FF40 are read here only; the entries at 0x80526890 and 0x80526980 are called by
 *    `menu/menu_plsearch.cpp`.
 * RESIDUALS. register numbering only, no instruction differences: `PMICSetBuffers` (the copy of `size` and the
 *    aligned base swap r28/r29; declaration order and statement order were measured), `PMICSetGain` and
 *    `PMICGetParam` (the callback and argument copies swap r30/r31), `PMICInit` (the stack and register order of the
 *    buffer carving chain; the request pool loop and the state tests match), `PMICWrite`, `PMICPopPlay` and
 *    `PMICPushCapture` (the ring base register), `PMICOnQueryDone`, `PMICStartAsync`.  `PMICFilterSamples` keeps explicit
 *    `frsp` roundings of the two level followers where this source emits none (`f64` locals measured worse), and
 *    `fp_contract off` is needed for its unfused `fmuls`+`fadds`.  flipcheck blockers: the .text differences above
 *    (the object is 16 B short of the split); `.data` and `.sdata` end 4 B short of the split (trailing alignment
 *    pad the compiler does not emit, as in KPR/kpr.c).
 * SHAPES. the request helpers `PMICPopFree`, `PMICPushQueue` and `PMICPushFree` are `static inline` and expand into
 *    their callers, while the real entries (`PMICRequestAlloc`, `PMICRequestDone`, `PMICRequestEnqueue`,
 *    `PMICGetState` and the ring helpers) are defined under `#pragma dont_inline on` with hand-written bodies;
 *    `.sbss` words are defined in reverse address order (the compiler lays them out backwards); unrolled ring copy
 *    loops hoist the capacity and buffer into locals; the isochronous callbacks need the loop and flag locals
 *    declared in the order `i`, `offset`, `word`, `length`, `status`, flags; a state switch of one or two cases is an
 *    if chain, a polling `if (state == n)` ladder rather than a `switch` (`PMICPollRequests`).
 */

#include "types.h"
#include "USB/usb.h"
#include "IPC/ipcclt.h"
#include "OS/OS.h"
#include "OS/OSAlarm.h"
#include "OS/OSSetAlarm.h"
#include "OS/OSCache.h"
#include "OS/OSError.h"
#include "OS/OSThread.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* size: 0x34 - one queued control request: the command word, its operands and the completion callbacks. */
typedef struct PMICRequest PMICRequest;
typedef void (*PMICRequestDoneFn)(s32 result, PMICRequest* request);
typedef void (*PMICUserCallback)(s32 result, s32 arg);

struct PMICRequest {
    /* +0x00 */ PMICRequest* next;
    /* +0x04 */ u16 command;
    /* +0x06 */ u16 value0;
    /* +0x08 */ u16 value1;
    /* +0x0A */ u16 value2;
    /* +0x0C */ u16 unused_0x0C;
    /* +0x0E */ u8 pad_0x0E[2];
    /* +0x10 */ u32 unused_0x10;
    /* +0x14 */ u32* reply;
    /* +0x18 */ PMICRequestDoneFn done;
    /* +0x1C */ PMICUserCallback callback;
    /* +0x20 */ s32 callbackArg;
    /* +0x24 */ u8 keep;
    /* +0x25 */ u8 pad_0x25[3];
    /* +0x28 */ OSThreadQueue queue;
    /* +0x30 */ s32 retries;
}; /* size: 0x34 */

/* size: 0x0A - the control packet that carries a request to the device and its answer. */
typedef struct PMICPacket {
    /* +0x00 */ u16 command;
    /* +0x02 */ u16 value0;
    /* +0x04 */ u16 value1;
    /* +0x06 */ u16 value2;
    /* +0x08 */ u16 sequence;
} PMICPacket;

/* size: 0x40 - the capture ring (+0x00) and the playback ring (+0x18) with their counters. */
typedef struct PMICRings {
    /* +0x00 */ s32 captureCapacity;
    /* +0x04 */ s32 captureCount;
    /* +0x08 */ s32 captureWrite;
    /* +0x0C */ s32 captureRead;
    /* +0x10 */ s16* captureBuffer;
    /* +0x14 */ s32 captureOverflow;
    /* +0x18 */ s32 playCapacity;
    /* +0x1C */ s32 playCount;
    /* +0x20 */ s32 playWrite;
    /* +0x24 */ s32 playRead;
    /* +0x28 */ s32 playPrimed;
    /* +0x2C */ s16* playBuffer;
    /* +0x30 */ s32 playUnderrun;
    /* +0x34 */ s32 playOverrun;
    /* +0x38 */ s32 playFault;
    /* +0x3C */ u8 pad_0x3C[4];
} PMICRings;

/* size: 0x28 - the two scratch areas the capture path keeps beside the rings. */
typedef struct PMICScratch {
    /* +0x00 */ s32 unused_0x00;
    /* +0x04 */ s32 mask;
    /* +0x08 */ u8* areaA;
    /* +0x0C */ u8* areaB;
    /* +0x10 */ u8 unused_0x10[0x18];
} PMICScratch;

typedef void (*PMICIsoCallback)(s32 result, s32 arg);

/* The firmware image the device is loaded with; the address and size are kept by `PMICInit`. */
u32 PMICFirmware[0x17CC8 / 4] = {
#include "PMIC/pmic_firmware.inc"
};

const char* __PMICVersion = "<< RVL_SDK - PMIC \trelease build: Jun  2 2009 09:30:47 (0x4302_145) >>";

char PMICBusName[] = "oh0";

u16 PMICParamTable[4] = {0x2001, 0x2004, 0x200D, 0x0000};

f32 PMICSilenceRatio = 0.6666f;
f32 PMICSilenceLevel = 0.014252141f;
f32 PMICGainMinA = 0.5011872f;
f32 PMICGainUpA = 0.1000061f;
f32 PMICGainDownA = 0.0005493164f;
f32 PMICGainMinB = 0.5011872f;
f32 PMICGainUpB = 0.1000061f;
f32 PMICGainDownB = 0.0005493164f;
f32 PMICGainTarget = 0.5011872f;

PMICRequest PMICRequestPool[20];
OSAlarm PMICOpAlarm;
OSAlarm PMICRequestAlarm;
OSAlarm PMICPollAlarm;
PMICRings PMICRingState;
PMICScratch PMICScratchState;

s32 PMICFilterFlagB;
s32 PMICFilterFlagA;
f32 PMICFilterGainB;
f32 PMICFilterGainA;
f32 PMICFilterLevelB;
f32 PMICFilterLevelA;
s32 PMICBuffersReady[2];
s32 PMICIsoCount;
s32 PMICDevice;
USBCallback PMICOpPending;
USBCallback PMICRequestPending;
s32 PMICStatusPolls;
OSThreadQueue PMICSyncQueue;
s32 PMICSyncResult;
PMICUserCallback PMICOpCallback;
s32 PMICOpCallbackArg;
u8* PMICControlBuffer;
u8* PMICChunkBuffer;
u8* PMICFirmwareCursor;
u32 PMICFirmwareLeft;
u8* PMICFirmwareData;
u32 PMICFirmwareSize;
USBIsoRequest* PMICCaptureIso[2];
USBIsoRequest* PMICPlayIso[2];
s32 PMICIsoPending;
s32 PMICCaptureErrors;
s32 PMICPlayErrors;
PMICIsoCallback PMICIsoDone;
s32 PMICIsoCallbackArg;
PMICPacket* PMICPacketBufferHead;
PMICPacket* PMICPacketBuffer;
u16 PMICRequestSeq;
s32 PMICRequestState;
s32 PMICRequestResult;
PMICRequest* PMICFree;
PMICRequest* PMICQueue;
PMICRequest* PMICCurrent;
s32 PMICInsertionPending;
s32 PMICState;

#define PMIC_TICKS_PER_MICROSECOND (OS_BUS_CLOCK / 4 / 125000)
#define PMIC_TICKS_PER_MILLISECOND (OS_BUS_CLOCK / 4 / 1000)

/* Pops a request from the free list. */
static inline PMICRequest* PMICPopFree(void)
{
    PMICRequest* request = NULL;
    if (PMICFree != NULL) {
        request = PMICFree;
        PMICFree = PMICFree->next;
    }
    return request;
}

/* Appends a request to the queue. */
static inline void PMICPushQueue(PMICRequest* request)
{
    PMICRequest* tail = PMICQueue;
    if (tail == NULL) {
        PMICQueue = request;
        request->next = NULL;
    } else {
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = request;
        request->next = NULL;
    }
}

/* Returns a request to the free list once its owner is done with it. */
static inline void PMICPushFree(PMICRequest* request)
{
    request->command = 0;
    request->value0 = 0;
    request->value1 = 0;
    request->value2 = 0;
    request->unused_0x0C = 0;
    request->unused_0x10 = 0;
    request->reply = NULL;
    request->done = NULL;
    request->callback = NULL;
    request->callbackArg = 0;
    request->keep = 0;
    request->next = PMICFree;
    PMICFree = request;
}

/* Reports a result to the operation callback and clears it. */
#define PMIC_REPORT_OP(result)                                                                                         \
    do {                                                                                                               \
        if (PMICOpCallback != NULL) {                                                                                  \
            PMICOpCallback((result), PMICOpCallbackArg);                                                               \
        }                                                                                                              \
        PMICOpCallback = NULL;                                                                                         \
        PMICOpCallbackArg = 0;                                                                                         \
    } while (0)


/* untyped: caller-owned payload (unused) */
void PMICOnInsertion(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnDeviceOpened(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnConfigSet(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnConfigRead(s32 result, void* arg);
s32 PMICOnDeviceRemoved(s32 result);
/* untyped: caller-owned payload (unused) */
void PMICOnDeviceClosed(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnStartAck(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnStopAck(s32 result, void* arg);
s32 PMICStartIso(void);
void PMICOnIsoStopped(s32 result);
/* untyped: caller-owned payload (unused) */
void PMICOnCaptureIso(s32 result, USBIsoRequest* iso, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnPlayIso(s32 result, USBIsoRequest* iso, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnStatusRead(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnChunkWritten(s32 result, void* arg);
/* untyped: caller-owned payload (unused) */
void PMICOnReadyPoll(s32 result, void* arg);
void PMICOnOpTimeout(OSAlarm* alarm, OSContext* context);
void PMICOnRequestTimeout(OSAlarm* alarm, OSContext* context);
void PMICOnOpRetryTimeout(OSAlarm* alarm, OSContext* context);
void PMICOnRequestRetryTimeout(OSAlarm* alarm, OSContext* context);
void PMICPollRequests(OSAlarm* alarm, OSContext* context);
void PMICDisableBuffers(void);
void PMICResetCaptureRing(void);
void PMICResetPlayRing(void);
void PMICResetPlayRingAndCounters(void);
u8* PMICSetBuffers(u8* area, u32 size);
void PMICWakeSync(s32 result, s32 arg);
void PMICOnRecordStart0(s32 result, PMICRequest* request);
void PMICOnRecordStop0(s32 result, PMICRequest* request);
void PMICOnRecordStart1(s32 result, PMICRequest* request);
void PMICOnRecordStop1(s32 result, PMICRequest* request);
void PMICOnQueryDone(s32 result, PMICRequest* request);
void PMICOnQueryRetry(s32 result, PMICRequest* request);

/* 0x80522E00 (0x310): brings the library up once, laying its transfer buffers out in the caller's work area. */
s32 PMICInit(u8* work, s32 size)
{
    u32 level;
    u8* base;
    u8* chunk;
    u8* capData0;
    u8* capSizes0;
    u8* capIso1;
    u8* capData1;
    u8* capSizes1;
    u8* playIso0;
    u8* playData0;
    u8* playSizes0;
    u8* playIso1;
    u8* playData1;
    u8* playSizes1;
    u8* packet;
    s32 i;
    s32 result;

    level = OSDisableInterrupts();
    if (PMICState == -3) {
        OSRestoreInterrupts(level);
        return -100;
    }
    if (PMICState == 1) {
        OSRestoreInterrupts(level);
        return -5;
    }
    if (PMICState != 0) {
        OSRestoreInterrupts(level);
        return 0;
    }
    {
        PMICState = 1;
        OSRestoreInterrupts(level);
        OSRegisterVersion(__PMICVersion);
        if (work == NULL || size <= 0) {
            return -1;
        }
        if (IPCCltInit() < 0) {
            OSReport("PMICInit() : too busy, can't work stably.\n");
            PMICState = -3;
        } else if (IUSB_OpenLib() < 0) {
            OSReport("PMICInit() : too busy, can't work stably.\n");
            PMICState = -3;
        } else {
            PMICFirmwareData = (u8*)PMICFirmware;
            PMICFirmwareSize = 0x17CC8;
            PMICOpCallback = NULL;
            PMICOpCallbackArg = 0;
            PMICIsoPending = 0;
            PMICCaptureErrors = -1;
            PMICPlayErrors = -1;
            PMICIsoDone = NULL;
            PMICIsoCallbackArg = 0;
            OSInitThreadQueue(&PMICSyncQueue);
            base = (u8*)(((u32)work + 0x1F) & ~0x1F);
            PMICControlBuffer = base;
            PMICCaptureIso[0] = (USBIsoRequest*)(base + 0x420);
            capData0 = base + 0x440;
            PMICChunkBuffer = base + 0x20;
            capSizes0 = capData0 + 0x100;
            ((USBIsoRequest*)(base + 0x420))->packetCount = 8;
            capIso1 = capSizes0 + 0x20;
            capData1 = capIso1 + 0x20;
            capSizes1 = capData1 + 0x100;
            playIso0 = capSizes1 + 0x20;
            PMICCaptureIso[0]->data = capData0;
            playData0 = playIso0 + 0x20;
            playSizes0 = playData0 + 0x200;
            playIso1 = playSizes0 + 0x20;
            playData1 = playIso1 + 0x20;
            PMICCaptureIso[0]->packetSizes = (u16*)capSizes0;
            playSizes1 = playData1 + 0x200;
            packet = playSizes1 + 0x20;
            PMICCaptureIso[1] = (USBIsoRequest*)capIso1;
            ((USBIsoRequest*)capIso1)->packetCount = 8;
            PMICCaptureIso[1]->data = capData1;
            PMICCaptureIso[1]->packetSizes = (u16*)capSizes1;
            PMICPlayIso[0] = (USBIsoRequest*)playIso0;
            ((USBIsoRequest*)playIso0)->packetCount = 8;
            PMICPlayIso[0]->data = playData0;
            PMICPlayIso[0]->packetSizes = (u16*)playSizes0;
            PMICPlayIso[1] = (USBIsoRequest*)playIso1;
            ((USBIsoRequest*)playIso1)->packetCount = 8;
            PMICPlayIso[1]->data = playData1;
            PMICPlayIso[1]->packetSizes = (u16*)playSizes1;
            PMICPacketBufferHead = (PMICPacket*)packet;
            PMICSetBuffers(packet + 0x20, size);
            PMICPacketBuffer = (PMICPacket*)PMICControlBuffer;
            PMICFree = PMICRequestPool;
            for (i = 0; i < 19; i++) {
                memset(&PMICRequestPool[i], 0, sizeof(PMICRequest));
                PMICRequestPool[i].next = &PMICRequestPool[i + 1];
                OSInitThreadQueue(&PMICRequestPool[i].queue);
            }
            memset(&PMICRequestPool[i], 0, sizeof(PMICRequest));
            PMICRequestPool[i].next = NULL;
            OSInitThreadQueue(&PMICRequestPool[i].queue);
            PMICQueue = NULL;
            PMICRequestPending = NULL;
            PMICOpPending = NULL;
            OSCreateAlarm(&PMICOpAlarm);
            OSCreateAlarm(&PMICRequestAlarm);
            level = OSDisableInterrupts();
            PMICRequestState = 0;
            OSCreateAlarm(&PMICPollAlarm);
            OSSetAlarm(&PMICPollAlarm, (PMIC_TICKS_PER_MICROSECOND * 1050) / 8, PMICPollRequests);
            PMICState = 3;
            OSRestoreInterrupts(level);
        }
        result = 0;
        if (PMICState == -3) {
            result = -100;
        }
        return result;
    }
}

/* 0x80523110 (0xEC): tears the library down when no transfer is in flight. */
s32 PMICEnd(void)
{
    u32 level = OSDisableInterrupts();
    s32 result;
    if (PMICState == -3) {
        result = -100;
    } else if (PMICState == 0) {
        result = 0;
    } else if (PMICState == 1 || PMICState == 4 || PMICState == 6 || PMICState == 2) {
        result = -5;
    } else if (PMICState >= 7) {
        result = -4;
    } else {
        OSCancelAlarm(&PMICPollAlarm);
        if (PMICState == 5) {
            PMICState = 2;
            OSRestoreInterrupts(level);
            IUSB_Ioctl1D(PMICDevice);
            level = OSDisableInterrupts();
            IUSB_CloseDevice(PMICDevice);
        }
        PMICDisableBuffers();
        PMICState = 0;
        result = 0;
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x80523200 (0xD0): asks the USB stack to report the device when it is plugged in. */
s32 PMICStartSearch(void)
{
    u32 level = OSDisableInterrupts();
    s32 result;
    s32 status;
    if (PMICState == -3) {
        result = -100;
    } else if (PMICState < 3) {
        result = -4;
    } else {
        result = 0;
        if (PMICState < 5) {
            result = -2;
        }
        if (PMICInsertionPending == 0) {
            PMICInsertionPending = 1;
            status = IUSB_RegisterInsertionNotifyAsync("/dev/usb/oh0", 0x57E, 0x308, PMICOnInsertion, NULL);
            if (status < 0) {
                if (status == -3) {
                    PMICState = -3;
                    result = -100;
                } else {
                    PMICInsertionPending = 0;
                }
            }
        }
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x805232D0 (0x128): starts the firmware upload to the opened device and reports the outcome to `callback`. */
s32 PMICStartAsync(PMICUserCallback callback, s32 arg)
{
    u32 level = OSDisableInterrupts();
    s32 result;
    if (PMICState == -3) {
        result = -100;
    } else if (PMICState == 6) {
        result = -5;
    } else if (PMICState != 5) {
        result = -4;
    } else {
        PMICOpCallbackArg = arg;
        PMICOpCallback = callback;
        if (IUSB_WriteCtrlMsgAsync(PMICDevice, 0x41, 0, 0, 0, 0, NULL, PMICOnStartAck, NULL) < 0) {
            PMICOpCallback = NULL;
            result = -6;
            PMICOpCallbackArg = 0;
        } else {
            PMICState = 6;
            PMICOpPending = PMICOnStartAck;
            OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
            result = 0;
        }
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x80523400 (0x90): asks a running device to stop and reports the outcome to `callback`. */
s32 PMICStopAsync(PMICUserCallback callback, s32 arg)
{
    s32 result;
    u32 level = OSDisableInterrupts();
    if (PMICState == -3) {
        result = -100;
    } else if (PMICState <= 5) {
        result = -4;
    } else if (PMICState == 6 || PMICState == 8) {
        result = -5;
    } else {
        PMICOpCallback = callback;
        result = 0;
        PMICOpCallbackArg = arg;
        PMICState = 6;
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x80523490 (0x74): stops the device and waits until it has stopped. */
s32 PMICStop(void)
{
    u32 level = OSDisableInterrupts();
    s32 result = PMICStopAsync(PMICWakeSync, 0);
    if (result != 0) {
        OSRestoreInterrupts(level);
        return result;
    }
    OSSleepThread(&PMICSyncQueue);
    OSRestoreInterrupts(level);
    return PMICSyncResult;
}

/* 0x80523510 (0x118): queues the request that starts recording once the device runs. */
s32 PMICStartRecordingAsync(PMICUserCallback callback, s32 arg)
{
    PMICRequest* request;
    s32 result;
    u32 level = OSDisableInterrupts();
    if (PMICState == -3) {
        result = -100;
    } else if (PMICState == 8) {
        result = -5;
    } else if (PMICState != 7) {
        result = -4;
    } else {
        request = PMICPopFree();
        if (request == NULL) {
            result = -5;
        } else {
            PMICState = 8;
            request->command = 0x9A00;
            request->value0 = 1;
            request->done = PMICOnRecordStart0;
            request->callback = callback;
            request->callbackArg = arg;
            request->keep = 0;
            PMICPushQueue(request);
            result = 0;
        }
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x80523630 (0x118): queues the request that stops recording while the device runs. */
s32 PMICStopRecordingAsync(PMICUserCallback callback, s32 arg)
{
    PMICRequest* request;
    s32 result;
    u32 level = OSDisableInterrupts();
    if (PMICState == -3) {
        result = -100;
    } else if (PMICState == 8) {
        result = -5;
    } else if (PMICState != 9) {
        result = -4;
    } else {
        request = PMICPopFree();
        if (request == NULL) {
            result = -5;
        } else {
            PMICState = 8;
            request->command = 0x9A0C;
            request->value0 = 1;
            request->done = PMICOnRecordStop0;
            request->callback = callback;
            request->callbackArg = arg;
            request->keep = 0;
            PMICPushQueue(request);
            result = 0;
        }
    }
    OSRestoreInterrupts(level);
    return result;
}

#pragma dont_inline on

/* 0x80523750 (0x20): takes a request from the free list. */
PMICRequest* PMICRequestAlloc(void)
{
    PMICRequest* request = NULL;
    if (PMICFree != NULL) {
        request = PMICFree;
        PMICFree = PMICFree->next;
    }
    return request;
}

/* 0x80523770 (0x88): reports a finished request to its callback and recycles it. */
void PMICRequestDone(s32 result, PMICRequest* request)
{
    PMICUserCallback callback = request->callback;
    if (callback != NULL) {
        callback(result, request->callbackArg);
    }
    if (request->keep == 0) {
        request->command = 0;
        request->value0 = 0;
        request->value1 = 0;
        request->value2 = 0;
        request->unused_0x0C = 0;
        request->unused_0x10 = 0;
        request->reply = NULL;
        request->done = NULL;
        request->callback = NULL;
        request->callbackArg = 0;
        request->keep = 0;
        request->next = PMICFree;
        PMICFree = request;
    }
}

/* 0x80523800 (0x44): appends a request to the queue. */
s32 PMICRequestEnqueue(PMICRequest* request)
{
    PMICRequest* tail = PMICQueue;
    if (tail == NULL) {
        PMICQueue = request;
        request->next = NULL;
    } else {
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = request;
        request->next = NULL;
    }
    return 0;
}

/* 0x80523850 (0x8): returns the library state. */
s32 PMICGetState(void)
{
    return PMICState;
}

#pragma dont_inline reset

/* 0x80523860 (0xFC): opens the device once the USB stack reports it. */
/* untyped: caller-owned payload (unused) */
void PMICOnInsertion(s32 result, void* arg)
{
    if (PMICState != 4 && PMICState != -3) {
        if ((u32)PMICState <= 1) {
            PMICInsertionPending = 0;
            return;
        }
        if (result == -6) {
            PMICInsertionPending = 0;
            return;
        }
        if (result < 0) {
            PMICState = -3;
            return;
        }
        if (IUSB_OpenDeviceIdsAsync(PMICBusName, 0x57E, 0x308, PMICOnDeviceOpened, NULL) < 0) {
            OSReport("PMIC lib : too busy, can't work stably.\n");
            PMICState = -3;
            return;
        }
        PMICState = 4;
        PMICOpPending = PMICOnDeviceOpened;
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
    }
}

/* 0x80523960 (0x154): configures the opened device and starts the configuration reads. */
/* untyped: caller-owned payload (unused) */
void PMICOnDeviceOpened(s32 result, void* arg)
{
    if (PMICOpPending != NULL) {
        OSCancelAlarm(&PMICOpAlarm);
        PMICOpPending = NULL;
    }
    PMICDevice = result;
    if (PMICState != -3) {
        if (result == -6) {
            PMICState = 3;
            PMICInsertionPending = 0;
            return;
        }
        if (result < 0) {
            PMICState = -3;
            return;
        }
        if (IUSB_DeviceRemovalNotifyAsync(result, (USBCallback)PMICOnDeviceRemoved, NULL) < 0) {
            OSReport("PMIC lib : too busy, can't work stably.\n");
            PMICState = -3;
            return;
        }
        if (IUSB_WriteCtrlMsgAsync(PMICDevice, 1, 0xB, 1, 0, 0, NULL, PMICOnConfigSet, NULL) < 0) {
            OSReport("PMIC lib : too busy, can't work stably.\n");
            PMICState = -3;
            return;
        }
        PMICOpPending = PMICOnConfigSet;
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
    }
}

/* 0x80523AC0 (0x150): reads the device's configuration back once it is set. */
/* untyped: caller-owned payload (unused) */
void PMICOnConfigSet(s32 result, void* arg)
{
    if (PMICOpPending != NULL) {
        OSCancelAlarm(&PMICOpAlarm);
        PMICOpPending = NULL;
    }
    if (PMICState != -3 && PMICState != 3 && result != -6 && result != -4) {
        if (result < 0) {
            OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
            return;
        }
        DCInvalidateRange(PMICControlBuffer, 0x20);
        if (IUSB_ReadCtrlMsgAsync(PMICDevice, 0x81, 0xA, 0, 0, 1, PMICControlBuffer, PMICOnConfigRead, NULL) < 0) {
            OSReport("PMIC lib : too busy, can't work stably.\n");
            PMICState = -3;
            return;
        }
        PMICOpPending = PMICOnConfigRead;
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
    }
}

/* 0x80523C10 (0xDC): accepts the configuration the device reports and marks it ready. */
/* untyped: caller-owned payload (unused) */
void PMICOnConfigRead(s32 result, void* arg)
{
    if (PMICOpPending != NULL) {
        OSCancelAlarm(&PMICOpAlarm);
        PMICOpPending = NULL;
    }
    if (PMICState != -3 && PMICState != 3 && result != -6 && result != -4) {
        if (result < 0) {
            OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
            return;
        }
        if (*PMICControlBuffer != 1) {
            PMICState = -3;
            return;
        }
        PMICState = 5;
        PMICRequestSeq = 0;
    }
}

/* 0x80523CF0 (0x88): clears the rings when the device goes away and closes it. */
s32 PMICOnDeviceRemoved(s32 result)
{
    PMICResetCaptureRing();
    PMICResetPlayRingAndCounters();
    if (PMICState != -3) {
        if (PMICState == 2) {
            PMICInsertionPending = 0;
        } else if (IUSB_CloseDeviceAsync(PMICDevice, PMICOnDeviceClosed, NULL) < 0) {
            OSReport("PMIC lib : too busy, can't work stably.\n");
            PMICState = -3;
        }
    }
    return result;
}

/* 0x80523D80 (0x100): finishes the removal and reports it to the waiting callers. */
/* untyped: caller-owned payload (unused) */
void PMICOnDeviceClosed(s32 result, void* arg)
{
    if (PMICState != -3) {
        PMICState = 3;
        PMICInsertionPending = 0;
    }
    OSCancelAlarm(&PMICOpAlarm);
    if (PMICState == 3) {
        if (PMICOpCallback != NULL) {
            PMICOpCallback(-2, PMICOpCallbackArg);
        }
    } else {
        if (PMICOpCallback != NULL) {
            PMICOpCallback(-100, PMICOpCallbackArg);
        }
        PMICState = -3;
    }
    PMICOpCallback = NULL;
    PMICOpCallbackArg = 0;
    OSCancelAlarm(&PMICRequestAlarm);
    if (PMICRequestState == 5) {
        if (PMICState == 3 || (u32)(PMICState - 5) <= 1) {
            PMICRequestState = -1;
            PMICRequestResult = -2;
            return;
        }
        PMICRequestState = -1;
        PMICRequestResult = -100;
        PMICState = -3;
    }
}

/* 0x80524AD0 (0xC): stores the stop result and wakes the thread waiting in the synchronous call. */
void PMICWakeSync(s32 result, s32 arg)
{
    PMICSyncResult = result;
    OSWakeupThread(&PMICSyncQueue);
}

/* 0x80526010 (0x30): fails the library when the pending operation does not answer in time. */
void PMICOnOpTimeout(OSAlarm* alarm, OSContext* context)
{
    USBCallback pending = PMICOpPending;
    PMICState = -3;
    if (pending == NULL) {
        return;
    }
    PMICOpPending = NULL;
    pending(0, NULL);
}

/* 0x80526040 (0x30): fails the library when the pending request does not answer in time. */
void PMICOnRequestTimeout(OSAlarm* alarm, OSContext* context)
{
    USBCallback pending = PMICRequestPending;
    PMICState = -3;
    if (pending == NULL) {
        return;
    }
    PMICRequestPending = NULL;
    pending(0, NULL);
}

/* 0x80526070 (0x8C): reports a failed operation after the retry delay. */
void PMICOnOpRetryTimeout(OSAlarm* alarm, OSContext* context)
{
    if (alarm == NULL) {
        OSCancelAlarm(&PMICOpAlarm);
    }
    if (PMICState == 3) {
        if (PMICOpCallback != NULL) {
            PMICOpCallback(-2, PMICOpCallbackArg);
        }
    } else {
        if (PMICOpCallback != NULL) {
            PMICOpCallback(-100, PMICOpCallbackArg);
        }
        PMICState = -3;
    }
    PMICOpCallback = NULL;
    PMICOpCallbackArg = 0;
}

/* 0x80526100 (0x80): reports a failed request after the retry delay. */
void PMICOnRequestRetryTimeout(OSAlarm* alarm, OSContext* context)
{
    if (alarm == NULL) {
        OSCancelAlarm(&PMICRequestAlarm);
    }
    if (PMICRequestState == 5) {
        if (PMICState == 3 || (u32)(PMICState - 5) <= 1) {
            PMICRequestState = -1;
            PMICRequestResult = -2;
            return;
        }
        PMICRequestState = -1;
        PMICRequestResult = -100;
        PMICState = -3;
    }
}

#pragma dont_inline on

/* 0x805263B0 (0x10C): lays the capture ring, the playback ring and the scratch areas out in `area`. */
u8* PMICSetBuffers(u8* area, u32 size)
{
    u8* capture;
    u8* play;
    capture = (u8*)(((u32)area + 0x1F) & ~0x1F);
    PMICRingState.captureCapacity = size >> 1;
    PMICRingState.captureCount = 0;
    PMICRingState.captureWrite = 0;
    PMICRingState.captureRead = 0;
    PMICRingState.captureOverflow = 0;
    PMICRingState.captureBuffer = (s16*)capture;
    memset(capture, 0, size);
    PMICRingState.playCapacity = 0x600;
    play = capture + ((size + 0x1F) & ~0x1F);
    PMICRingState.playCount = 0;
    PMICRingState.playWrite = 0;
    PMICRingState.playRead = 0;
    PMICRingState.playPrimed = 0;
    PMICRingState.playUnderrun = 0;
    PMICRingState.playOverrun = 0;
    PMICRingState.playFault = 0;
    PMICRingState.playBuffer = (s16*)play;
    memset(play, 0, 0xC00);
    PMICScratchState.unused_0x00 = 0;
    PMICScratchState.mask = 0x3F;
    PMICScratchState.areaA = play + 0xC00;
    memset(play + 0xC00, 0, 0x80);
    PMICScratchState.areaB = play + 0x1E80;
    memset(play + 0x1E80, 0, 0x80);
    PMICBuffersReady[0] = 1;
    return play + 0x3100;
}

/* 0x805264C0 (0xC): marks the rings as unusable. */
void PMICDisableBuffers(void)
{
    PMICBuffersReady[0] = 0;
}

/* 0x805264D0 (0x30): empties the capture ring. */
void PMICResetCaptureRing(void)
{
    PMICRingState.captureCount = 0;
    PMICRingState.captureWrite = 0;
    PMICRingState.captureRead = 0;
    PMICRingState.captureOverflow = 0;
    memset(PMICRingState.captureBuffer, 0, PMICRingState.captureCapacity * 2);
}

/* 0x80526500 (0x30): empties the playback ring. */
void PMICResetPlayRing(void)
{
    PMICRingState.playCount = 0;
    PMICRingState.playWrite = 0;
    PMICRingState.playRead = 0;
    PMICRingState.playPrimed = 0;
    PMICRingState.playFault = 0;
    memset(PMICRingState.playBuffer, 0, 0xC00);
}

/* 0x80526530 (0x64): empties the playback ring and clears its error counters. */
void PMICResetPlayRingAndCounters(void)
{
    PMICRingState.playCount = 0;
    PMICRingState.playWrite = 0;
    PMICRingState.playRead = 0;
    PMICRingState.playPrimed = 0;
    PMICRingState.playFault = 0;
    memset(PMICRingState.playBuffer, 0, 0xC00);
    PMICRingState.playUnderrun = 0;
    PMICRingState.playOverrun = 0;
}

#pragma dont_inline reset

/* Cancels the operation alarm once the pending operation has answered. */
#define PMIC_CLEAR_OP_PENDING()                                                                                        \
    do {                                                                                                               \
        if (PMICOpPending != NULL) {                                                                                   \
            OSCancelAlarm(&PMICOpAlarm);                                                                               \
            PMICOpPending = NULL;                                                                                      \
        }                                                                                                              \
    } while (0)

/* 0x80523E80 (0x1B8): reads the device status after the start command was acknowledged. */
/* untyped: caller-owned payload (unused) */
void PMICOnStartAck(s32 result, void* arg)
{
    PMIC_CLEAR_OP_PENDING();
    if (PMICState == -3) {
        PMIC_REPORT_OP(-100);
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMIC_REPORT_OP(-2);
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
        return;
    }
    DCInvalidateRange(PMICControlBuffer, 0x20);
    if (IUSB_ReadCtrlMsgAsync(PMICDevice, 0xC1, 6, 0, 0, 1, PMICControlBuffer, PMICOnStatusRead, NULL) < 0) {
        PMIC_REPORT_OP(-6);
        PMICState = 5;
        return;
    }
    PMICOpPending = PMICOnStatusRead;
    OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
}

/* 0x80524040 (0x238): sends the first firmware chunk once the device reports itself idle. */
/* untyped: caller-owned payload (unused) */
void PMICOnStatusRead(s32 result, void* arg)
{
    u32 chunk;
    PMIC_CLEAR_OP_PENDING();
    if (PMICState == -3) {
        PMIC_REPORT_OP(-100);
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMIC_REPORT_OP(-2);
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
        return;
    }
    if (*PMICControlBuffer != 0) {
        PMIC_REPORT_OP(-100);
        PMICState = -3;
        return;
    }
    chunk = 0x400;
    PMICFirmwareCursor = PMICFirmwareData;
    PMICFirmwareLeft = PMICFirmwareSize;
    if (PMICFirmwareLeft <= 0x400) {
        chunk = PMICFirmwareLeft;
    }
    memcpy(PMICChunkBuffer, PMICFirmwareCursor, chunk);
    DCFlushRange(PMICChunkBuffer, chunk);
    if (IUSB_WriteBlkMsgAsync(PMICDevice, 2, chunk, PMICChunkBuffer, PMICOnChunkWritten, NULL) < 0) {
        PMIC_REPORT_OP(-6);
        PMICState = 5;
        return;
    }
    PMICFirmwareCursor += chunk;
    PMICFirmwareLeft -= chunk;
    PMICOpPending = PMICOnChunkWritten;
    OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
}

/* 0x80524280 (0x2B4): sends the next firmware chunk, then polls the device status once the image is out. */
/* untyped: caller-owned payload (unused) */
void PMICOnChunkWritten(s32 result, void* arg)
{
    u32 chunk;
    PMIC_CLEAR_OP_PENDING();
    if (PMICState == -3) {
        PMIC_REPORT_OP(-100);
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMIC_REPORT_OP(-2);
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
        return;
    }
    if (PMICFirmwareLeft != 0) {
        chunk = 0x400;
        if (PMICFirmwareLeft <= 0x400) {
            chunk = PMICFirmwareLeft;
        }
        memcpy(PMICChunkBuffer, PMICFirmwareCursor, chunk);
        DCFlushRange(PMICChunkBuffer, chunk);
        if (IUSB_WriteBlkMsgAsync(PMICDevice, 2, chunk, PMICChunkBuffer, PMICOnChunkWritten, NULL) < 0) {
            PMIC_REPORT_OP(-6);
            PMICState = 5;
            return;
        }
        PMICFirmwareCursor += chunk;
        PMICFirmwareLeft -= chunk;
        PMICOpPending = PMICOnChunkWritten;
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
        return;
    }
    DCInvalidateRange(PMICControlBuffer, 0x20);
    if (IUSB_ReadCtrlMsgAsync(PMICDevice, 0xC1, 6, 0, 0, 1, PMICControlBuffer, PMICOnReadyPoll, NULL) < 0) {
        PMIC_REPORT_OP(-6);
        PMICState = 5;
        return;
    }
    PMICStatusPolls = 0;
    PMICOpPending = PMICOnReadyPoll;
    OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
}

/* 0x80524540 (0x288): polls the device until it reports itself running, then starts the isochronous streams. */
/* untyped: caller-owned payload (unused) */
void PMICOnReadyPoll(s32 result, void* arg)
{
    PMIC_CLEAR_OP_PENDING();
    if (PMICState == -3) {
        PMIC_REPORT_OP(-100);
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMIC_REPORT_OP(-2);
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
        return;
    }
    if (*PMICControlBuffer == 0) {
        PMICStatusPolls++;
        if (PMICStatusPolls > 0x104) {
            PMIC_REPORT_OP(-100);
            PMICState = -3;
            return;
        }
        DCInvalidateRange(PMICControlBuffer, 0x20);
        if (IUSB_ReadCtrlMsgAsync(PMICDevice, 0xC1, 6, 0, 0, 1, PMICControlBuffer, PMICOnReadyPoll, NULL) < 0) {
            PMIC_REPORT_OP(-6);
            PMICState = 5;
            return;
        }
        PMICOpPending = PMICOnReadyPoll;
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
        return;
    }
    if (PMICStartIso() != 0) {
        PMIC_REPORT_OP(0);
        PMICResetPlayRingAndCounters();
        PMICState = 7;
        PMICIsoCount = 0;
        return;
    }
    PMIC_REPORT_OP(-6);
    PMICState = 5;
}

/* 0x805247D0 (0x2F8): sizes the isochronous packets, clears the playback data and submits the four transfers. */
s32 PMICStartIso(void)
{
    s32 ok = 1;
    s32 i;
    for (i = 0; i < 8; i++) {
        PMICCaptureIso[0]->packetSizes[i] = 0x20;
        PMICCaptureIso[1]->packetSizes[i] = 0x20;
        PMICPlayIso[0]->packetSizes[i] = 0x40;
        PMICPlayIso[1]->packetSizes[i] = 0x40;
    }
    memset(PMICPlayIso[0]->data, 0, 0x200);
    memset(PMICPlayIso[1]->data, 0, 0x200);
    DCFlushRange(PMICPlayIso[0]->data, 0x200);
    DCFlushRange(PMICPlayIso[1]->data, 0x200);
    if (IUSB_IsoMsgAsync(PMICDevice, 0x81, PMICCaptureIso[0], PMICOnCaptureIso, NULL) < 0) {
        PMICIsoPending = 0;
        ok = 0;
    } else if (IUSB_IsoMsgAsync(PMICDevice, 0x81, PMICCaptureIso[1], PMICOnCaptureIso, NULL) < 0) {
        PMICIsoPending = 1;
        ok = 0;
    } else if (IUSB_IsoMsgAsync(PMICDevice, 3, PMICPlayIso[0], PMICOnPlayIso, NULL) < 0) {
        PMICIsoPending = 2;
        ok = 0;
    } else if (IUSB_IsoMsgAsync(PMICDevice, 3, PMICPlayIso[1], PMICOnPlayIso, NULL) < 0) {
        PMICIsoPending = 3;
        ok = 0;
    } else {
        PMICIsoPending = 4;
        PMICCaptureErrors = -1;
        PMICPlayErrors = -1;
    }
    return ok;
}

/* 0x80524AE0 (0x160): clears the rings after the streams stopped and sends the stop command. */
void PMICOnIsoStopped(s32 result)
{
    PMICResetCaptureRing();
    PMICResetPlayRingAndCounters();
    if (PMICState == -3) {
        PMIC_REPORT_OP(-100);
        return;
    }
    if (PMICState == 3 || result == -4 || result == -6 || result == -0x1B6E) {
        PMIC_REPORT_OP(-2);
        return;
    }
    if (IUSB_WriteCtrlMsgAsync(PMICDevice, 0x41, 0, 0, 0, 0, NULL, PMICOnStopAck, NULL) < 0) {
        PMIC_REPORT_OP(-6);
        PMICState = 5;
        return;
    }
    PMICOpPending = PMICOnStopAck;
    OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpTimeout);
}

/* 0x80524C40 (0x130): reports the stop once the device acknowledged it. */
/* untyped: caller-owned payload (unused) */
void PMICOnStopAck(s32 result, void* arg)
{
    PMIC_CLEAR_OP_PENDING();
    if (PMICState == -3) {
        PMIC_REPORT_OP(-100);
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMIC_REPORT_OP(-2);
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICOpAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnOpRetryTimeout);
        return;
    }
    PMIC_REPORT_OP(0);
    PMICState = 5;
}

/* 0x80525A00 (0x100): chains the second command of the recording start, or reports the failure. */
void PMICOnRecordStart0(s32 result, PMICRequest* request)
{
    PMICUserCallback callback;
    if (result == 0) {
        request->command = 0x9A0C;
        request->value0 = 0;
        request->done = PMICOnRecordStart1;
        PMICPushQueue(request);
        return;
    }
    callback = request->callback;
    if (callback != NULL) {
        callback(result, request->callbackArg);
    }
    if (request->keep == 0) {
        PMICPushFree(request);
    }
    if (PMICState == 8) {
        PMICState = 7;
    }
}

/* 0x80525B00 (0xC0): finishes the recording start and clears the capture ring. */
void PMICOnRecordStart1(s32 result, PMICRequest* request)
{
    PMICUserCallback callback = request->callback;
    if (callback != NULL) {
        callback(result, request->callbackArg);
    }
    if (request->keep == 0) {
        PMICPushFree(request);
    }
    if (PMICState == 8) {
        if (result == 0) {
            PMICResetCaptureRing();
            PMICState = 9;
            return;
        }
        PMICState = 7;
    }
}

/* 0x80525BC0 (0x100): chains the second command of the recording stop, or reports the failure. */
void PMICOnRecordStop0(s32 result, PMICRequest* request)
{
    PMICUserCallback callback;
    if (result == 0) {
        request->command = 0x9A00;
        request->value0 = 0;
        request->done = PMICOnRecordStop1;
        PMICPushQueue(request);
        return;
    }
    callback = request->callback;
    if (callback != NULL) {
        callback(result, request->callbackArg);
    }
    if (request->keep == 0) {
        PMICPushFree(request);
    }
    if (PMICState == 8) {
        PMICState = 9;
    }
}

/* 0x80525CC0 (0xB4): finishes the recording stop. */
void PMICOnRecordStop1(s32 result, PMICRequest* request)
{
    PMICUserCallback callback = request->callback;
    if (callback != NULL) {
        callback(result, request->callbackArg);
    }
    if (request->keep == 0) {
        PMICPushFree(request);
    }
    if (PMICState == 8) {
        s32 next = 9;
        if (result == 0) {
            next = 7;
        }
        PMICState = next;
    }
}

/* 0x80525D80 (0x178): issues the follow-up query command, retrying a busy device up to 100 times. */
void PMICOnQueryDone(s32 result, PMICRequest* request)
{
    if (result == 0) {
        request->value0 = 0;
        request->command = 0x9A14;
        request->done = PMICOnQueryRetry;
        request->keep = 0;
        request->retries = 0;
        PMICPushQueue(request);
        return;
    }
    if (result == -6) {
        s32 retries = request->retries;
        if (retries > 100) {
            PMICPushFree(request);
            PMICState = -3;
            return;
        }
        request->command = 0x9A14;
        request->value0 = 1;
        request->done = PMICOnQueryDone;
        request->keep = 0;
        request->retries = retries + 1;
        PMICPushQueue(request);
        return;
    }
    PMICPushFree(request);
}

/* 0x80525F00 (0x108): repeats the query command while the device is busy. */
void PMICOnQueryRetry(s32 result, PMICRequest* request)
{
    if (result == -6) {
        s32 retries = request->retries;
        if (retries > 100) {
            PMICPushFree(request);
            PMICState = -3;
            return;
        }
        request->command = 0x9A14;
        request->value0 = 0;
        request->done = PMICOnQueryRetry;
        request->keep = 0;
        request->retries = retries + 1;
        PMICPushQueue(request);
        return;
    }
    PMICPushFree(request);
}

/* 0x80526180 (0xE8): copies up to `count` captured samples out of the capture ring. */
s32 PMICRead(s16* dst, s32 count)
{
    u32 level;
    s32 n;
    s32 read;
    s32 capacity;
    s16* buffer;
    s32 i;
    if (PMICBuffersReady[0] == 0) {
        return -1;
    }
    if (dst == NULL || count < 0) {
        return -1;
    }
    level = OSDisableInterrupts();
    n = PMICRingState.captureCount;
    if (n >= count) {
        n = count;
    }
    capacity = PMICRingState.captureCapacity;
    read = PMICRingState.captureRead;
    buffer = PMICRingState.captureBuffer;
    for (i = n; i > 0; i--) {
        *dst++ = buffer[read++];
        if (read >= capacity) {
            read = 0;
        }
    }
    PMICRingState.captureRead = read;
    PMICRingState.captureCount -= n;
    OSRestoreInterrupts(level);
    return n;
}

/* 0x80526270 (0x140): appends `count` samples to the playback ring. */
s32 PMICWrite(s16* src, s32 count)
{
    u32 level;
    s32 overflow = 0;
    s32 write;
    s32 capacity;
    s16* buffer;
    s32 i;
    if (PMICBuffersReady[0] == 0) {
        return -1;
    }
    if (src == NULL || count < 0 || count > PMICRingState.playCapacity) {
        return -1;
    }
    if (PMICGetState() < 7) {
        return 0;
    }
    level = OSDisableInterrupts();
    capacity = PMICRingState.playCapacity;
    write = PMICRingState.playWrite;
    buffer = PMICRingState.playBuffer;
    if (count > capacity - PMICRingState.playCount) {
        overflow = 1;
    }
    for (i = count; i > 0; i--) {
        buffer[write++] = *src++;
        if (write >= capacity) {
            write = 0;
        }
    }
    if (overflow != 0) {
        PMICRingState.playCount = capacity;
        PMICRingState.playWrite = write;
        PMICRingState.playRead = write;
        PMICRingState.playUnderrun = 1;
        PMICRingState.playFault = 1;
    } else {
        PMICRingState.playWrite = write;
        PMICRingState.playCount += count;
    }
    OSRestoreInterrupts(level);
    return count;
}

/* 0x805265A0 (0xFC): appends captured samples to the capture ring while recording. */
s32 PMICPushCapture(s16* src, s32 bytes)
{
    s32 overflow = 0;
    s32 write;
    s32 n;
    s32 capacity;
    s16* buffer;
    s32 i;
    if (PMICGetState() != 9) {
        return 1;
    }
    n = bytes / 2;
    capacity = PMICRingState.captureCapacity;
    write = PMICRingState.captureWrite;
    buffer = PMICRingState.captureBuffer;
    if (n > capacity - PMICRingState.captureCount) {
        overflow = 1;
    }
    for (i = n; i > 0; i--) {
        buffer[write++] = *src++;
        if (write >= capacity) {
            write = 0;
        }
    }
    if (overflow != 0) {
        PMICRingState.captureCount = capacity;
        PMICRingState.captureWrite = write;
        PMICRingState.captureRead = write;
        PMICRingState.captureOverflow = 1;
    } else {
        PMICRingState.captureWrite = write;
        PMICRingState.captureCount += n;
    }
    return 1;
}

/* 0x805266A0 (0x10C): moves playback samples into an isochronous packet, swapping each pair. */
s32 PMICPopPlay(s16* dst, s32 bytes)
{
    s32 n = bytes / 2;
    s16* first;
    s16* second;
    s32 read;
    s32 capacity;
    s16* buffer;
    s32 i;
    if (PMICRingState.playCount >= n) {
        second = dst + 1;
        first = dst;
        read = PMICRingState.playRead;
        capacity = PMICRingState.playCapacity;
        buffer = PMICRingState.playBuffer;
        for (i = n / 2; i > 0; i--) {
            *second = buffer[read++];
            *first = buffer[read++];
            second += 2;
            first += 2;
            if (read >= capacity) {
                read = 0;
            }
        }
        PMICRingState.playRead = read;
        PMICRingState.playCount -= n;
        if (PMICRingState.playPrimed < 0x10) {
            PMICRingState.playPrimed++;
        }
    } else {
        if (PMICRingState.playPrimed < 0x10) {
            PMICRingState.playPrimed = 0;
        } else {
            PMICRingState.playOverrun = 1;
            PMICRingState.playFault = 1;
        }
        memset(dst, 0, bytes);
    }
    return PMICRingState.playFault == 0;
}

/* 0x805267B0 (0xD8): queues the command that selects one of four gain steps. */
s32 PMICSetGain(u16 unit, PMICUserCallback callback, s32 arg)
{
    PMICRequest* request;
    s32 state;
    s32 result;
    u32 level = OSDisableInterrupts();
    state = PMICGetState();
    if (state == -3) {
        result = -100;
    } else if (unit > 3) {
        result = -1;
    } else if (state < 7) {
        result = -4;
    } else {
        request = PMICRequestAlloc();
        if (request == NULL) {
            result = -5;
        } else {
            request->command = 0x9A02;
            request->value0 = unit;
            request->done = PMICRequestDone;
            request->callback = callback;
            request->callbackArg = arg;
            request->keep = 0;
            result = PMICRequestEnqueue(request);
        }
    }
    OSRestoreInterrupts(level);
    return result;
}

/* 0x80526890 (0xEC): queues the command that reads one device parameter into `reply`. */
s32 PMICGetParam(u32 index, u32* reply, PMICUserCallback callback, s32 arg)
{
    PMICRequest* request;
    s32 state;
    s32 result;
    u32 level = OSDisableInterrupts();
    state = PMICGetState();
    if (state == -3) {
        result = -100;
    } else if (index > 2) {
        result = -1;
    } else if (reply == NULL) {
        result = -1;
    } else if (state < 7) {
        result = -4;
    } else {
        request = PMICRequestAlloc();
        if (request == NULL) {
            result = -5;
        } else {
            request->command = 0x9A39;
            request->value0 = PMICParamTable[index];
            request->reply = reply;
            request->done = PMICRequestDone;
            request->callback = callback;
            request->callbackArg = arg;
            request->keep = 0;
            result = PMICRequestEnqueue(request);
        }
    }
    OSRestoreInterrupts(level);
    return result;
}

/* Reports a result to the iso callback. */
#define PMIC_REPORT_ISO(result)                                                                                        \
    do {                                                                                                               \
        if (PMICIsoDone != NULL) {                                                                                     \
            PMICIsoDone((result), PMICIsoCallbackArg);                                                                 \
        }                                                                                                              \
    } while (0)

/* 0x80524D70 (0x2AC): takes the captured packets of one transfer, resubmits it and tracks the stream errors. */
/* untyped: caller-owned payload (unused) */
void PMICOnCaptureIso(s32 result, USBIsoRequest* iso, void* arg)
{
    s32 i;
    s32 offset;
    u16 word;
    s32 length;
    s32 status;
    s32 bad;
    s32 cancelled;
    s32 resubmitFailed;
    s32 tooMany;
    if (PMICState >= 7 && PMICIsoPending == 4) {
        bad = 0;
        cancelled = 0;
        resubmitFailed = 0;
        tooMany = 0;
        if (result == 0 || result == -0x1B60 || result == -0x1B61 || (u32)(result + 0x1B67) <= 1) {
            offset = 0;
            for (i = 0; i < 8; i++) {
                word = iso->packetSizes[i];
                length = word & 0xFFF;
                status = (word >> 12) & 0xF;
                if ((length == 0x20 && status == 0) || (length == 0x16 && status == 9) || (length == 0x18 && status == 9) ||
                    (length == 0x10 && status == 9)) {
                    PMICPushCapture((s16*)((u8*)iso->data + offset), length);
                } else {
                    bad = 1;
                }
                offset += 0x20;
                iso->packetSizes[i] = 0x20;
                if (status != 0 && status != 8 && status != 9 && status != 14 && status != 15) {
                    PMICCaptureErrors = 0;
                }
            }
        } else if (result == -6 || result == -0x1B6E) {
            cancelled = 1;
        } else {
            if (PMICCaptureErrors < 0) {
                PMICCaptureErrors = 0;
            }
            bad = 1;
        }
        if (PMICCaptureErrors >= 0) {
            PMICCaptureErrors++;
            if (PMICCaptureErrors > 200) {
                tooMany = 1;
            }
        }
        if (cancelled == 0 && tooMany == 0 && IUSB_IsoMsgAsync(PMICDevice, 0x81, iso, PMICOnCaptureIso, NULL) < 0) {
            resubmitFailed = 1;
        }
        if (cancelled != 0 || resubmitFailed != 0 || tooMany != 0) {
            PMICIsoPending--;
        }
        if (tooMany != 0) {
            PMIC_REPORT_ISO(-100);
            PMICState = -3;
        } else if (resubmitFailed != 0) {
            PMIC_REPORT_ISO(-6);
            PMICState = 6;
        } else if (cancelled != 0) {
            PMICState = 6;
        } else if (bad != 0) {
            PMIC_REPORT_ISO(-7);
        }
    } else {
        PMICIsoPending--;
    }
    if (PMICIsoPending == 0) {
        PMICOnIsoStopped(result);
    }
}

/* 0x80525020 (0x354): fills the playback packets of one transfer, resubmits it and tracks the stream errors. */
/* untyped: caller-owned payload (unused) */
void PMICOnPlayIso(s32 result, USBIsoRequest* iso, void* arg)
{
    s32 i;
    s32 offset;
    u16 word;
    s32 status;
    s32 bad;
    s32 cancelled;
    s32 resubmitFailed;
    s32 tooMany;
    PMICRequest* request;
    if (PMICState >= 7 && PMICIsoPending == 4) {
        bad = 0;
        cancelled = 0;
        resubmitFailed = 0;
        tooMany = 0;
        if (result == 0 || result == -0x1B60 || result == -0x1B61 || (u32)(result + 0x1B67) <= 1) {
            offset = 0;
            for (i = 0; i < 8; i++) {
                word = iso->packetSizes[i];
                status = (word >> 12) & 0xF;
                if (PMICPopPlay((s16*)((u8*)iso->data + offset), 0x40) == 0) {
                    bad = 1;
                }
                offset += 0x40;
                iso->packetSizes[i] = 0x40;
                if (status != 0) {
                    if (status != 8 && status != 9 && status != 14 && status != 15) {
                        PMICPlayErrors = 0;
                    }
                    bad = 1;
                }
            }
        } else if (result == -6 || result == -0x1B6E) {
            cancelled = 1;
        } else {
            if (PMICPlayErrors < 0) {
                PMICPlayErrors = 0;
            }
            bad = 1;
        }
        if (PMICPlayErrors >= 0) {
            PMICPlayErrors++;
            if (PMICPlayErrors > 200) {
                tooMany = 1;
            }
        }
        if (cancelled == 0 && tooMany == 0) {
            iso->packetCount = 8;
            DCFlushRange(iso->data, 0x200);
            if (IUSB_IsoMsgAsync(PMICDevice, 3, iso, PMICOnPlayIso, NULL) < 0) {
                resubmitFailed = 1;
            }
        }
        if (cancelled != 0 || resubmitFailed != 0 || tooMany != 0) {
            PMICIsoPending--;
        }
        if (tooMany != 0) {
            PMIC_REPORT_ISO(-100);
            PMICState = -3;
        } else if (resubmitFailed != 0) {
            PMIC_REPORT_ISO(-6);
            PMICState = 6;
        } else if (cancelled != 0) {
            PMICState = 6;
        } else if (bad != 0) {
            request = PMICPopFree();
            if (request != NULL) {
                request->command = 0x9A14;
                request->value0 = 1;
                request->done = PMICOnQueryDone;
                request->callback = NULL;
                request->callbackArg = 0;
                request->keep = 0;
                request->retries = 0;
                PMICPushQueue(request);
                PMICResetPlayRing();
                PMIC_REPORT_ISO(-7);
            } else {
                PMIC_REPORT_ISO(-6);
                PMICState = 6;
            }
        }
    } else {
        PMICIsoPending--;
    }
    if (PMICIsoPending == 0) {
        PMICOnIsoStopped(result);
    }
    PMICIsoCount += 8;
}

/* 0x80525840 (0xE0): moves the request machine on once the command packet was written. */
/* untyped: caller-owned payload (unused) */
void PMICOnPacketSent(s32 result, void* arg)
{
    if (PMICRequestPending != NULL) {
        OSCancelAlarm(&PMICRequestAlarm);
        PMICRequestPending = NULL;
    }
    if (PMICState == -3) {
        PMICRequestState = -1;
        PMICRequestResult = -100;
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMICRequestState = -1;
        PMICRequestResult = -2;
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICRequestAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnRequestRetryTimeout);
        PMICRequestState = 5;
        return;
    }
    PMICRequestState = 2;
}

/* 0x80525920 (0xE0): moves the request machine on once the answer packet was read. */
/* untyped: caller-owned payload (unused) */
void PMICOnPacketRead(s32 result, void* arg)
{
    if (PMICRequestPending != NULL) {
        OSCancelAlarm(&PMICRequestAlarm);
        PMICRequestPending = NULL;
    }
    if (PMICState == -3) {
        PMICRequestState = -1;
        PMICRequestResult = -100;
        return;
    }
    if (PMICState == 3 || result == -6) {
        PMICRequestState = -1;
        PMICRequestResult = -2;
        return;
    }
    if (result < 0) {
        OSSetAlarm(&PMICRequestAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnRequestRetryTimeout);
        PMICRequestState = 5;
        return;
    }
    PMICRequestState = 4;
}

/* Pops the oldest request from the queue. */
static inline PMICRequest* PMICPopQueue(void)
{
    PMICRequest* request = NULL;
    if (PMICQueue != NULL) {
        request = PMICQueue;
        PMICQueue = PMICQueue->next;
    }
    return request;
}

/* 0x80525380 (0x4B8): the periodic task that runs the request machine: sends a queued request, reads its answer and
 * completes it. */
void PMICPollRequests(OSAlarm* alarm, OSContext* context)
{
    PMICRequest* request;
    PMICUserCallback callback;
    PMICRequestDoneFn done;
    u16* out;
    if (PMICRequestState == 0) {
        request = PMICPopQueue();
        PMICCurrent = request;
        if (PMICState < 7) {
            while (request != NULL) {
                callback = request->callback;
                if (callback != NULL) {
                    if (PMICState == -3) {
                        callback(-100, request->callbackArg);
                    } else {
                        callback(-4, request->callbackArg);
                    }
                }
                if (PMICCurrent->keep == 0) {
                    PMICPushFree(PMICCurrent);
                }
                request = PMICPopQueue();
                PMICCurrent = request;
            }
        } else if (request != NULL) {
            PMICPacketBuffer->command = request->command;
            PMICPacketBuffer->value0 = request->value0;
            PMICPacketBuffer->value1 = request->value1;
            PMICPacketBuffer->value2 = request->value2;
            PMICPacketBuffer->sequence = PMICRequestSeq++;
            DCFlushRange(PMICPacketBuffer, 0xA);
            if (IUSB_WriteCtrlMsgAsync(PMICDevice, 0x41, 1, 0, 0, 0xA, PMICPacketBuffer, PMICOnPacketSent, NULL) < 0) {
                done = PMICCurrent->done;
                if (done != NULL) {
                    done(-6, PMICCurrent);
                }
            } else {
                PMICRequestState = 1;
                PMICRequestPending = PMICOnPacketSent;
                OSSetAlarm(&PMICRequestAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnRequestTimeout);
            }
        }
    } else if (PMICRequestState == 2) {
        if (PMICState < 7) {
            callback = PMICCurrent->callback;
            if (callback != NULL) {
                if (PMICState == -3) {
                    callback(-100, PMICCurrent->callbackArg);
                } else {
                    callback(-4, PMICCurrent->callbackArg);
                }
            }
            if (PMICCurrent->keep == 0) {
                PMICPushFree(PMICCurrent);
            }
            PMICRequestState = 0;
        } else {
            DCInvalidateRange(PMICPacketBuffer, 0xA);
            if (IUSB_ReadCtrlMsgAsync(PMICDevice, 0xC1, 2, 0, 0, 0xA, PMICPacketBuffer, PMICOnPacketRead, NULL) < 0) {
                done = PMICCurrent->done;
                if (done != NULL) {
                    done(-6, PMICCurrent);
                }
                PMICRequestState = 0;
            } else {
                PMICRequestState = 3;
                PMICRequestPending = PMICOnPacketRead;
                OSSetAlarm(&PMICRequestAlarm, (OS_BUS_CLOCK / 4 / 1000) * 2000, PMICOnRequestTimeout);
            }
        }
    } else if (PMICRequestState == 4) {
        request = PMICCurrent;
        request->value0 = PMICPacketBuffer->value0;
        request->value1 = PMICPacketBuffer->value1;
        request->value2 = PMICPacketBuffer->value2;
        out = (u16*)request->reply;
        if (out != NULL) {
            switch (request->command) {
            case 0x9A01:
            case 0x9A03:
            case 0x9A07:
            case 0x9A09:
            case 0x9A0B:
            case 0x9A0D:
            case 0x9A0F:
            case 0x9A11:
            case 0x9A13:
            case 0x9A15:
            case 0x9A33:
                *out = request->value0;
                break;
            case 0x9A05:
                *out = (u8)request->value0;
                break;
            case 0x9A35:
            case 0x9A39:
                *out = request->value1;
                break;
            case 0x9A37:
            case 0x9A3B:
                *(u32*)out = (request->value1 << 16) | request->value2;
                break;
            case 0x9A3D:
                *out = request->value2;
                break;
            }
        }
        done = request->done;
        if (done != NULL) {
            done(0, request);
        }
        PMICRequestState = 0;
    } else if (PMICRequestState == -1) {
        if (PMICCurrent != NULL) {
            done = PMICCurrent->done;
            if (done != NULL) {
                done(PMICRequestResult, PMICCurrent);
            }
        }
        PMICRequestState = 0;
    }
    OSSetAlarm(alarm, (PMIC_TICKS_PER_MICROSECOND * 1050) / 8, PMICPollRequests);
}

#pragma fp_contract off

/* 0x80526980 (0x30C): runs the two-channel level follower over `count` sample pairs, scaling each channel by its
 * gain; reports through `silent` whether most of the block was gated. */
s32 PMICFilterSamples(s16* left, s16* right, s32 count, s32 reset, s32* silent)
{
    f32 inputL;
    f32 inputR;
    f32 magL;
    f32 magR;
    f32 levelA;
    f32 levelB;
    f32 gainA;
    f32 gainB;
    f32 diff;
    f32 target;
    s32 gated;
    s32 attack;
    s32 i;
    if (left == NULL || right == NULL || count < 0) {
        return -1;
    }
    if (reset != 0) {
        PMICFilterLevelA = 0.0f;
        PMICFilterLevelB = 0.0f;
        PMICFilterGainA = 0.0f;
        PMICFilterGainB = 0.0f;
        PMICFilterFlagB = 0;
        PMICFilterFlagA = 0;
    }
    gated = 0;
    for (i = count; i > 0; i--) {
        inputL = (f32)*left;
        magL = inputL / 32768.0f;
        inputR = (f32)*right;
        if (magL < 0.0f) {
            magL *= -1.0f;
        }
        levelA = PMICFilterLevelA + 0.0078125f * (magL - PMICFilterLevelA);
        if (levelA > 1.0f) {
            levelA = 1.0f;
        } else if (levelA < 0.0f) {
            levelA = 0.0f;
        }
        magR = inputR / 32768.0f;
        PMICFilterLevelA = levelA;
        if (magR < 0.0f) {
            magR *= -1.0f;
        }
        diff = magR - PMICFilterLevelB;
        if (diff > 0.0f) {
            levelB = PMICFilterLevelB + 0.0078125f * diff;
        } else {
            levelB = PMICFilterLevelB + 0.00048828125f * diff;
        }
        if (levelB > 1.0f) {
            levelB = 1.0f;
        } else if (levelB < 0.0f) {
            levelB = 0.0f;
        }
        PMICFilterLevelB = levelB;
        if (levelB >= PMICSilenceLevel) {
            if (levelB * PMICSilenceRatio > levelA) {
                attack = 1;
                PMICFilterFlagB = 1;
            } else {
                attack = 0;
                PMICFilterFlagB = 0;
            }
        } else {
            attack = 0;
            PMICFilterFlagB = 0;
        }
        if (attack == 1) {
            gainB = PMICFilterGainB - PMICGainDownA;
            if (gainB <= PMICGainMinA) {
                gainB = PMICGainMinA;
            }
        } else {
            gainB = PMICFilterGainB + PMICGainUpA;
            if (gainB >= 1.0f) {
                gainB = 1.0f;
            }
        }
        PMICFilterGainB = gainB;
        target = PMICGainTarget / gainB;
        if (target > 1.0f) {
            target = 1.0f;
        } else if (target < 0.0f) {
            target = 0.0f;
        }
        if (target > PMICFilterGainA) {
            attack = 0;
            PMICFilterFlagA = 0;
            gated++;
        } else {
            attack = 1;
            PMICFilterFlagA = 1;
        }
        if (attack == 1) {
            gainA = PMICFilterGainA - PMICGainDownB;
            if (gainA <= PMICGainMinB) {
                gainA = PMICGainMinB;
            }
        } else {
            gainA = PMICFilterGainA + PMICGainUpB;
            if (gainA >= 1.0f) {
                gainA = 1.0f;
            }
        }
        PMICFilterGainA = gainA;
        *left = (s16)(0.5f + inputL * gainB);
        left++;
        *right = (s16)(0.5f + inputR * gainA);
        right++;
    }
    if (silent != NULL) {
        if ((f32)gated >= 0.8f * (f32)count) {
            *silent = 1;
        } else {
            *silent = 0;
        }
    }
    return 0;
}

#pragma fp_contract reset
