/*
 * SI/SIBios.c - the SI (serial interface) library: controller polling, transfers, the type probe and the sampling
 *    rate.
 *
 * RANGE. .text 0x804DD9C0-0x804DF0A0 (21 functions, 0x16E0 B); .data 0x80629F98-0x8062A0A0; .bss
 *    0x80757608-0x80757828; .sdata 0x80794138-0x80794140; .sbss 0x80795470-0x80795488.  Cut from the old SC block
 *    between `SEQ/seq.c` (0x804DD9C0) and `SYN/syn.c` (0x804DF0A0).
 * FLAGS. the `OS` lib's group with `cflags_base` (configure.py `cflags=`): every function start of the band is
 *    16-aligned (`-O4,p` default alignment), not the 4-byte packing `cflags_os` gives.
 * NAMES. `SIBusy`, `SIInit`, `SITransfer`, `SIGetType`, `SISetSamplingRate`, ... are the map's names; the file name
 *    `SIBios.c` is a GUESS.  `CompleteTransfer`, `SIEnablePollingInterrupt`, `SIUnregisterPollingHandler`,
 *    `SIGetResponse`, `SIRefreshSamplingRate` and the statics `XferTime`, `Alarm`, `InputBuffer`, `InputBufferValid`,
 *    `InputBufferVcount`, `RDSTHandler`, `cmdFixedDevice`, `TypeCallback`, `FixedDeviceMask` are GUESSes derived from
 *    what each does.
 * EVIDENCE. `.data` 0x80629F98 is the build string `<< RVL_SDK - SI release build ... (0x4302_145) >>` read through
 *    `.sdata` 0x80794138 by `SIInit`; `Si`, `Type`, the 0x30 B `XYNTSC` table and the 0x30 B table after it
 *    with the `SISetSamplingRate: unknown TV format` string follow (0x8062A008, 0x8062A038).  The two status helpers
 *    are `static inline` and defined after `CompleteTransfer`: the statics are allocated in first-use order, and
 *    the target inlines them into `SIInterruptHandler`, `SIGetStatus` and `SIGetResponse` without emitting them.
 *    `InputBufferVcount` is volatile (the target reloads it between the zero test and the sum).
 * RESIDUALS. `__SITransfer` 0x804DE340: identical instruction stream, the six parameter copies are numbered
 *    r26..r31 in another order.  `GetTypeCallback` 0x804DEA20 (callback-loop registers, relocation names against
 *    `Packet`), `SIInterruptHandler` 0x804DDD20 (register numbering, one `lwzx` scheduled later in the target) and
 *    `SISetSamplingRate` (relocations name `XYNTSC` in the target, the section in ours) are all 94..99.9 %.
 *    flipcheck: `.bss` order (`cmdFixedDevice` precedes `TypeCallback` in the target, the first reference decides
 *    ours), the tail padding of `.data`/`.sdata`/`.sbss`, the `...bss.0`/`...data.0` relocation names, and
 *    `cmdFixedDevice` force-active in the target's `.comment`.
 * SHAPES. the SI registers are the absolute array `__SIRegs : 0xCD006400`; COMCSR is a bitfield union so the
 *    transfer start composes with bare `rlwimi`s; the tick conversions are 32-bit (`OS_USEC_TO_TICKS(65)`).
 */

#include "types.h"

#include "SI/SIBios.h"
#include "OS/OS.h"
#include "OS/OSAlarm.h"
#include "OS/OSInterrupt.h"
#include "OS/OSRtc.h"
#include "OS/OSSetAlarm.h"
#include "OS/__OSGetSystemTime.h"
#include "OS/OSError.h"
#include "VI/vi.h"

#define SI_MAX_CHAN 4

volatile u32 __SIRegs[64] : 0xCD006400;

/* The SI register block and the shared transfer buffer inside it (32-bit words). */
#define SI_REGS __SIRegs
#define SI_POLL 12
#define SI_COMCSR 13
#define SI_SR 14
#define SI_BUF 32

#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)
#define OS_MSEC_TO_TICKS(msec) ((msec) * (OS_TIMER_CLOCK / 1000))
#define OS_USEC_TO_TICKS(usec) (((usec) * (OS_TIMER_CLOCK / 125000)) / 8)

#define SI_ERROR_NO_RESPONSE 0x08
#define SI_ERROR_COLLISION 0x04
#define SI_ERROR_BUSY 0x80
#define SI_STATUS_RDST 0x20

/* The COMCSR register bits, most significant first. */
typedef struct SICommControlBits {
    /* +0x00 */ u32 tcint : 1;
    /* +0x00 */ u32 tcintmsk : 1;
    /* +0x00 */ u32 comerr : 1;
    /* +0x00 */ u32 rdstint : 1;
    /* +0x00 */ u32 rdstintmsk : 1;
    /* +0x00 */ u32 pad_26_23 : 4;
    /* +0x00 */ u32 outlen : 7;
    /* +0x00 */ u32 pad_15 : 1;
    /* +0x00 */ u32 inlen : 7;
    /* +0x00 */ u32 pad_7_3 : 5;
    /* +0x00 */ u32 channel : 2;
    /* +0x00 */ u32 tstart : 1;
} SICommControlBits; /* size: 0x04 */

/* The COMCSR register as a word or as its bit fields. */
typedef union SICommControl {
    /* +0x00 */ u32 val;
    /* +0x00 */ SICommControlBits f;
} SICommControl; /* size: 0x04 */

/* One queued transfer: its channel (-1 when the slot is free), buffers, callback and earliest start time. */
typedef struct SIPacket {
    /* +0x00 */ s32 chan;
    /* +0x04 */ void* output; /* untyped: the byte range of a transfer */
    /* +0x08 */ u32 outBytes;
    /* +0x0C */ void* input; /* untyped: the byte range of a transfer */
    /* +0x10 */ u32 inBytes;
    /* +0x14 */ SICallback callback;
    /* +0x18 */ s64 fire;
} SIPacket; /* size: 0x20 */

/* The transfer in flight: its channel (-1 when idle), the polling word, the response buffer and the callback. */
typedef struct SIControl {
    /* +0x00 */ s32 chan;
    /* +0x04 */ u32 poll;
    /* +0x08 */ u32 inBytes;
    /* +0x0C */ void* input; /* untyped: the byte range of a transfer */
    /* +0x10 */ SICallback callback;
} SIControl; /* size: 0x14 */

/* One sampling-rate row: the polling line and the sample count. */
typedef struct SISamplingXY {
    /* +0x00 */ u16 line;
    /* +0x02 */ u8 count;
    /* +0x03 */ u8 pad_0x03;
} SISamplingXY; /* size: 0x04 */


const char* __SIVersion = "<< RVL_SDK - SI \trelease build: Feb 27 2009 10:04:44 (0x4302_145) >>";

static SIControl Si = {-1, 0, 0, 0, 0};
static u32 Type[SI_MAX_CHAN] = {SI_ERROR_NO_RESPONSE, SI_ERROR_NO_RESPONSE, SI_ERROR_NO_RESPONSE, SI_ERROR_NO_RESPONSE};

static SISamplingXY XYNTSC[12] = {
    {0x00F6, 2, 0}, {0x000E, 19, 0}, {0x001E, 9, 0}, {0x002C, 6, 0}, {0x0034, 5, 0}, {0x0041, 4, 0},
    {0x0057, 3, 0}, {0x0057, 3, 0}, {0x0057, 3, 0}, {0x0083, 2, 0}, {0x0083, 2, 0}, {0x0083, 2, 0},
};
static SISamplingXY XYPAL[12] = {
    {0x0128, 2, 0}, {0x000F, 21, 0}, {0x001D, 11, 0}, {0x002D, 7, 0}, {0x0034, 6, 0}, {0x003F, 5, 0},
    {0x004E, 4, 0}, {0x0068, 3, 0}, {0x0068, 3, 0}, {0x0068, 3, 0}, {0x0068, 3, 0}, {0x009C, 2, 0},
};

static s32 SamplingRate;
static u32 FixedDeviceMask;

static SITypeCallback TypeCallback[SI_MAX_CHAN][SI_MAX_CHAN];
static u32 cmdFixedDevice[SI_MAX_CHAN];
static SIPollingHandler RDSTHandler[SI_MAX_CHAN];
static volatile u32 InputBufferVcount[SI_MAX_CHAN];
static u32 InputBufferValid[SI_MAX_CHAN];
static u32 InputBuffer[SI_MAX_CHAN][2];
static OSAlarm Alarm[SI_MAX_CHAN];
static s64 TypeTime[SI_MAX_CHAN];
static s64 XferTime[SI_MAX_CHAN];
static SIPacket Packet[SI_MAX_CHAN];

static void GetTypeCallback(s32 chan, u32 error, OSContext* context);

BOOL SIBusy(void)
{
    return Si.chan != -1 ? TRUE : FALSE;
}

BOOL SIIsChanBusy(s32 chan)
{
    return (Packet[chan].chan != -1 || Si.chan == chan) ? TRUE : FALSE;
}

/* Finishes the transfer in flight: copies its response out, records its time and returns the status nibble. */
u32 CompleteTransfer(void)
{
    u32 sr;
    u32 i;
    u32 rLen;
    u8* input;
    u32 data;

    sr = SI_REGS[SI_SR];
    SI_REGS[SI_COMCSR] = (SI_REGS[SI_COMCSR] | 0x80000000) & 0xFFFFFFFE;
    if (Si.chan != -1) {
        XferTime[Si.chan] = __OSGetSystemTime();
        input = Si.input;
        rLen = Si.inBytes / 4;
        for (i = 0; i < rLen; i++) {
            *(u32*)input = SI_REGS[SI_BUF + i];
            input += 4;
        }
        rLen = Si.inBytes & 3;
        if (rLen != 0) {
            data = SI_REGS[SI_BUF + i];
            for (i = 0; i < rLen; i++) {
                *input++ = data >> ((3 - i) * 8);
            }
        }
        if (SI_REGS[SI_COMCSR] & 0x20000000) {
            sr >>= (3 - Si.chan) * 8;
            sr &= 0xF;
            if ((sr & SI_ERROR_NO_RESPONSE) && !(Type[Si.chan] & SI_ERROR_BUSY)) {
                Type[Si.chan] = SI_ERROR_NO_RESPONSE;
            }
            if (sr == 0) {
                sr = SI_ERROR_COLLISION;
            }
        } else {
            TypeTime[Si.chan] = __OSGetSystemTime();
            sr = 0;
        }
        Si.chan = -1;
    }
    return sr;
}

/* Returns the status byte of `chan`, marking the channel's type as no-response when the hardware reports it. */
static inline u32 GetStatus(s32 chan)
{
    BOOL enabled;
    u32 sr;

    enabled = OSDisableInterrupts();
    sr = SI_REGS[SI_SR];
    sr >>= (3 - chan) * 8;
    if ((sr & SI_ERROR_NO_RESPONSE) && !(Type[chan] & SI_ERROR_BUSY)) {
        Type[chan] = SI_ERROR_NO_RESPONSE;
    }
    OSRestoreInterrupts(enabled);
    return sr;
}

/* Copies the polled response of `chan` out of the hardware when it carries the read-data flag. */
static inline BOOL GetResponseRaw(s32 chan)
{
    u32 status;

    status = GetStatus(chan);
    if (status & SI_STATUS_RDST) {
        InputBuffer[chan][0] = SI_REGS[chan * 3 + 1];
        InputBuffer[chan][1] = SI_REGS[chan * 3 + 2];
        InputBufferValid[chan] = TRUE;
        return TRUE;
    }
    return FALSE;
}

/* Handles the transfer-complete and read-data interrupts: completes the transfer, starts the next queued one and
 * runs the polling handlers. */
void SIInterruptHandler(s16 interrupt, OSContext* context)
{
    static u32 cmdTypeAndStatus;
    u32 reg;

    reg = SI_REGS[SI_COMCSR];
    if ((reg & 0xC0000000) == 0xC0000000) {
        s32 chan;
        u32 sr;
        SICallback callback;
        s32 i;
        s32 next;

        chan = Si.chan;
        sr = CompleteTransfer();
        callback = Si.callback;
        Si.callback = NULL;

        next = chan;
        for (i = 0; i < SI_MAX_CHAN; i++) {
            next++;
            next %= SI_MAX_CHAN;
            if (Packet[next].chan != -1 && Packet[next].fire <= __OSGetSystemTime()) {
                if (__SITransfer(Packet[next].chan, Packet[next].output, Packet[next].outBytes, Packet[next].input,
                                 Packet[next].inBytes, Packet[next].callback)) {
                    OSCancelAlarm(&Alarm[next]);
                    Packet[next].chan = -1;
                }
                break;
            }
        }

        if (callback) {
            callback(chan, sr, context);
        }

        SI_REGS[SI_SR] &= 0x0F000000 >> (chan * 8);
        if (Type[chan] == SI_ERROR_BUSY && !SIIsChanBusy(chan)) {
            SITransfer(chan, &cmdTypeAndStatus, 1, &Type[chan], 3, GetTypeCallback, OS_USEC_TO_TICKS(65));
        }
    }

    if ((reg & 0x18000000) == 0x18000000) {
        s32 i;
        u32 vcount;
        u32 curr;

        curr = VIGetCurrentLine() + 1;
        vcount = (Si.poll >> 16) & 0x3FF;
        for (i = 0; i < SI_MAX_CHAN; i++) {
            if (GetResponseRaw(i)) {
                InputBufferVcount[i] = curr;
            }
        }

        for (i = 0; i < SI_MAX_CHAN; i++) {
            if (Si.poll & (0x80000000 >> (24 + i))) {
                if (InputBufferVcount[i] != 0 && InputBufferVcount[i] + (vcount / 2) >= curr) {
                    continue;
                }
                return;
            }
        }

        for (i = 0; i < SI_MAX_CHAN; i++) {
            InputBufferVcount[i] = 0;
        }
        for (i = 0; i < SI_MAX_CHAN; i++) {
            if (RDSTHandler[i]) {
                RDSTHandler[i](interrupt, context);
            }
        }
    }
}

BOOL SIEnablePollingInterrupt(BOOL enable)
{
    BOOL enabled;
    BOOL rc;
    u32 reg;
    s32 i;

    enabled = OSDisableInterrupts();
    reg = SI_REGS[SI_COMCSR];
    rc = (reg & 0x08000000) ? TRUE : FALSE;
    if (enable) {
        for (i = 0; i < SI_MAX_CHAN; i++) {
            InputBufferVcount[i] = 0;
        }
        reg |= 0x08000000;
    } else {
        reg &= ~0x08000000;
    }
    reg &= 0x7FFFFFFE;
    SI_REGS[SI_COMCSR] = reg;
    OSRestoreInterrupts(enabled);
    return rc;
}

BOOL SIUnregisterPollingHandler(SIPollingHandler handler)
{
    BOOL enabled;
    s32 i;

    enabled = OSDisableInterrupts();
    for (i = 0; i < SI_MAX_CHAN; i++) {
        if (RDSTHandler[i] == handler) {
            RDSTHandler[i] = NULL;
            for (i = 0; i < SI_MAX_CHAN; i++) {
                if (RDSTHandler[i]) {
                    break;
                }
            }
            if (i == SI_MAX_CHAN) {
                SIEnablePollingInterrupt(FALSE);
            }
            OSRestoreInterrupts(enabled);
            return TRUE;
        }
    }
    OSRestoreInterrupts(enabled);
    return FALSE;
}

void SIInit(void)
{
    static BOOL Initialized;

    if (Initialized) {
        return;
    }
    OSRegisterVersion(__SIVersion);

    Packet[0].chan = Packet[1].chan = Packet[2].chan = Packet[3].chan = -1;
    Si.poll = 0;
    SISetSamplingRate(0);

    while (SI_REGS[SI_COMCSR] & 1) {
    }
    SI_REGS[SI_COMCSR] = 0x80000000;

    __OSSetInterruptHandler(0x14, SIInterruptHandler);
    __OSUnmaskInterrupts(0x800);

    SIGetType(0);
    SIGetType(1);
    SIGetType(2);
    SIGetType(3);

    Initialized = TRUE;
}

/* untyped: the byte ranges of a transfer */
BOOL __SITransfer(s32 chan, void* output, u32 outBytes, void* input, u32 inBytes, SICallback callback)
{
    BOOL enabled;
    SICommControl comcsr;
    u32 i;

    enabled = OSDisableInterrupts();
    if (Si.chan != -1) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    SI_REGS[SI_SR] &= 0x0F000000 >> (chan * 8);

    Si.chan = chan;
    Si.callback = callback;
    Si.inBytes = inBytes;
    Si.input = input;

    for (i = 0; i < (outBytes + 3) / 4; i++) {
        SI_REGS[SI_BUF + i] = ((u32*)output)[i];
    }

    comcsr.val = SI_REGS[SI_COMCSR];
    comcsr.f.tcint = 1;
    comcsr.f.tcintmsk = callback ? 1 : 0;
    comcsr.f.outlen = (outBytes == 128) ? 0 : outBytes;
    comcsr.f.inlen = (inBytes == 128) ? 0 : inBytes;
    comcsr.f.channel = chan;
    comcsr.f.tstart = 1;
    SI_REGS[SI_COMCSR] = comcsr.val;

    OSRestoreInterrupts(enabled);
    return TRUE;
}

u32 SIGetStatus(s32 chan)
{
    return GetStatus(chan);
}

void SISetCommand(s32 chan, u32 command)
{
    SI_REGS[chan * 3] = command;
}

u32 SISetXY(u32 x, u32 y)
{
    BOOL enabled;
    u32 poll;

    poll = x << 16;
    poll |= y << 8;
    enabled = OSDisableInterrupts();
    Si.poll &= 0xFC0000FF;
    Si.poll |= poll;
    poll = Si.poll;
    SI_REGS[SI_POLL] = poll;
    OSRestoreInterrupts(enabled);
    return poll;
}

u32 SIEnablePolling(u32 poll)
{
    BOOL enabled;
    u32 enable;

    if (poll == 0) {
        return Si.poll;
    }
    enabled = OSDisableInterrupts();
    poll >>= 24;
    enable = (poll >> 4) & 0xF;
    poll &= 0x03FFFFF0 | enable;
    Si.poll &= ~enable;
    Si.poll |= poll;
    poll = Si.poll;
    SI_REGS[SI_SR] = 0x80000000;
    SI_REGS[SI_POLL] = poll;
    OSRestoreInterrupts(enabled);
    return poll;
}

u32 SIDisablePolling(u32 poll)
{
    BOOL enabled;
    u32 disable;

    if (poll == 0) {
        return Si.poll;
    }
    enabled = OSDisableInterrupts();
    disable = (poll >> 24) & 0xF0;
    poll = Si.poll & ~disable;
    SI_REGS[SI_POLL] = poll;
    Si.poll = poll;
    OSRestoreInterrupts(enabled);
    return poll;
}

BOOL SIGetResponse(s32 chan, u32* data)
{
    BOOL enabled;
    BOOL rc;

    enabled = OSDisableInterrupts();
    GetResponseRaw(chan);
    rc = InputBufferValid[chan];
    InputBufferValid[chan] = FALSE;
    if (rc) {
        data[0] = InputBuffer[chan][0];
        data[1] = InputBuffer[chan][1];
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

/* Starts a queued transfer once its alarm fires. */
void AlarmHandler(OSAlarm* alarm, OSContext* context)
{
    s32 chan;
    SIPacket* packet;

    chan = alarm - Alarm;
    packet = &Packet[chan];
    if (packet->chan != -1) {
        if (__SITransfer(packet->chan, packet->output, packet->outBytes, packet->input, packet->inBytes,
                         packet->callback)) {
            packet->chan = -1;
        }
    }
}

/* untyped: the byte ranges of a transfer */
BOOL SITransfer(s32 chan, void* output, u32 outBytes, void* input, u32 inBytes, SICallback callback, s64 delay)
{
    BOOL enabled;
    SIPacket* packet;
    s64 now;
    s64 fire;

    packet = &Packet[chan];
    enabled = OSDisableInterrupts();
    if (packet->chan != -1 || Si.chan == chan) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    now = __OSGetSystemTime();
    if (delay == 0) {
        fire = now;
    } else {
        fire = XferTime[chan] + delay;
    }

    if (now < fire) {
        OSSetAlarm(&Alarm[chan], fire - now, AlarmHandler);
    } else if (__SITransfer(chan, output, outBytes, input, inBytes, callback)) {
        OSRestoreInterrupts(enabled);
        return TRUE;
    }

    packet->chan = chan;
    packet->output = output;
    packet->outBytes = outBytes;
    packet->input = input;
    packet->inBytes = inBytes;
    packet->callback = callback;
    packet->fire = fire;
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Finishes the type probe of `chan`: derives the device type word, repairs the wireless ID and runs the waiting
 * callbacks. */
static inline void RunTypeCallbacks(s32 chan)
{
    u32 type;
    s32 i;

    type = Type[chan];
    for (i = 0; i < SI_MAX_CHAN; i++) {
        if (TypeCallback[chan][i]) {
            SITypeCallback callback = TypeCallback[chan][i];
            TypeCallback[chan][i] = NULL;
            callback(chan, type);
        }
    }
}

static void GetTypeCallback(s32 chan, u32 error, OSContext* context)
{
    u32 type;
    u32 id;
    u32 fixed;
    u32 mask;

    Type[chan] &= ~SI_ERROR_BUSY;
    Type[chan] |= error;
    TypeTime[chan] = __OSGetSystemTime();
    type = Type[chan];

    mask = 0x80000000 >> chan;
    fixed = FixedDeviceMask & mask;
    FixedDeviceMask &= ~mask;

    if ((error & 0xF) || (type & 0x18000000) != 0x08000000 || !(type & 0x80000000) || (type & 0x04000000)) {
        OSSetWirelessID(chan, 0);
        RunTypeCallbacks(chan);
        return;
    }

    id = OSGetWirelessID(chan) << 8;
    if (fixed && (id & 0x00100000)) {
        cmdFixedDevice[chan] = (id & 0x00CFFF00) | 0x4E100000;
        Type[chan] = SI_ERROR_BUSY;
        SITransfer(chan, &cmdFixedDevice[chan], 3, &Type[chan], 3, GetTypeCallback, 0);
        return;
    }

    if (type & 0x00100000) {
        if ((id & 0x00CFFF00) != (type & 0x00CFFF00)) {
            if (!(id & 0x00100000)) {
                id = (type & 0x00CFFF00) | 0x00100000;
                OSSetWirelessID(chan, (id >> 8) & 0xFFFF);
            }
            cmdFixedDevice[chan] = id | 0x4E000000;
            Type[chan] = SI_ERROR_BUSY;
            SITransfer(chan, &cmdFixedDevice[chan], 3, &Type[chan], 3, GetTypeCallback, 0);
            return;
        }
    } else if (type & 0x40000000) {
        id = (type & 0x00CFFF00) | 0x00100000;
        OSSetWirelessID(chan, (id >> 8) & 0xFFFF);
        cmdFixedDevice[chan] = id | 0x4E000000;
        Type[chan] = SI_ERROR_BUSY;
        SITransfer(chan, &cmdFixedDevice[chan], 3, &Type[chan], 3, GetTypeCallback, 0);
        return;
    } else {
        OSSetWirelessID(chan, 0);
    }

    RunTypeCallbacks(chan);
}

u32 SIGetType(s32 chan)
{
    static s32 cmdTypeAndStatus;
    BOOL enabled;
    u32 type;
    s64 diff;

    enabled = OSDisableInterrupts();
    type = Type[chan];
    diff = __OSGetSystemTime() - TypeTime[chan];
    if (Si.poll & (0x80 >> chan)) {
        if (type != SI_ERROR_NO_RESPONSE) {
            TypeTime[chan] = __OSGetSystemTime();
            OSRestoreInterrupts(enabled);
            return type;
        }
        Type[chan] = SI_ERROR_BUSY;
        type = SI_ERROR_BUSY;
    } else if (diff <= OS_MSEC_TO_TICKS(50) && type != SI_ERROR_NO_RESPONSE) {
        OSRestoreInterrupts(enabled);
        return type;
    } else if (diff <= OS_MSEC_TO_TICKS(75)) {
        Type[chan] = SI_ERROR_BUSY;
    } else {
        Type[chan] = SI_ERROR_BUSY;
        type = SI_ERROR_BUSY;
    }

    TypeTime[chan] = __OSGetSystemTime();
    SITransfer(chan, &cmdTypeAndStatus, 1, &Type[chan], 3, GetTypeCallback, OS_USEC_TO_TICKS(65));
    OSRestoreInterrupts(enabled);
    return type;
}

u32 SIGetTypeAsync(s32 chan, SITypeCallback callback)
{
    BOOL enabled;
    u32 type;
    s32 i;

    enabled = OSDisableInterrupts();
    type = SIGetType(chan);
    if (Type[chan] & SI_ERROR_BUSY) {
        for (i = 0; i < SI_MAX_CHAN; i++) {
            if (TypeCallback[chan][i] == callback) {
                break;
            }
            if (TypeCallback[chan][i] == NULL) {
                TypeCallback[chan][i] = callback;
                break;
            }
        }
    } else {
        callback(chan, type);
    }
    OSRestoreInterrupts(enabled);
    return type;
}

#pragma dont_inline on
void SISetSamplingRate(u32 msec)
{
    BOOL enabled;
    SISamplingXY* xy;

    if (msec > 11) {
        msec = 11;
    }
    enabled = OSDisableInterrupts();
    SamplingRate = msec;
    switch (VIGetTvFormat()) {
    case 0:
    case 2:
    case 5:
        xy = XYNTSC;
        break;
    case 1:
        xy = XYPAL;
        break;
    default:
        OSReport("SISetSamplingRate: unknown TV format. Use default.");
        msec = 0;
        xy = XYNTSC;
        break;
    }
    SISetXY(((*(volatile u16*)0xCC00206C & 1) + 1) * xy[msec].line, xy[msec].count);
    OSRestoreInterrupts(enabled);
}

#pragma dont_inline reset

void SIRefreshSamplingRate(void)
{
    SISetSamplingRate(SamplingRate);
}
