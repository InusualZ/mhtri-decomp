/*
 * PAD/pad.c - the RVL SDK PAD library: controller reset and probe state machine over SI, origin capture,
 *    status decoding for the three pad specifications, the shutdown hook and the sampling handler.
 * RANGE. .text 0x804D82D0-0x804D9B3C (16 functions); .data 0x80629B38-0x80629B90; .bss 0x8074E3B0-0x8074E460; .sdata
 *    0x80793FE0-0x80794000; .sbss 0x80795410-0x80795440. The build string "<< RVL_SDK - PAD ... (0x4302_145) >>"
 *    opens .data; the pad state is read only by these functions.
 * FLAGS. `cflags_base` in the `OS` lib block of configure.py (Wii/1.3). `#pragma auto_inline off` keeps the
 *    PADReset/PADRecalibrate calls the target makes from PADInit, PADRead and __PADShutdown.
 * NAMES. Function and data names follow the RVL SDK spelling (map rows); `__PADShutdown` is the SDK's `OnShutdown`,
 *    `PADTypes` its `Type`, `PADInitialized` its `Initialized` (GUESS spellings forced by taken map names);
 *    `PADUnusedStatus` (GUESS) is a 0x30 .bss block no kept function references; `__OSWirelessPadFixMode` (GUESS,
 *    SDK spelling) is the low-memory word 0x800030E0.
 * RESIDUALS. Relocation names only: PADTypeAndStatusCallback's .bss base is `...bss.0` (target `Origin`, same offset 0);
 *    __PADVersion points at `@143` (map `lbl_80629B38`).
 * SHAPES. PADEnable/PADDisable/DoReset/PADSetSpec/PADSync/PADClearSamplingCallback/ClampS8/ClampU8 are static inline
 *    helpers. An s8 recentre is `+= -128` (`-= 128` emits `addi +0x80`, the target `subi 0x80`). A cleared bit is
 *    `x = x & ~m` (`x &= ~m` on a narrow field emits `andi.`, the target `rlwinm`). The fix-mode word is an
 *    absolute-address variable (`u16 v : addr`): one load feeds the four unrolled CmdProbeDevice stores, where a
 *    pointer-cast define reloads per store. PrevStatus is declared `extern` and defined `static` after PADRead,
 *    which keeps PADRead addressing PADTypes/PrevStatus/Origin by symbol instead of a pooled `...bss.0` base;
 *    PADUnusedStatus is defined after it, which gives .bss the target's order (PADUnusedStatus +0x50, PrevStatus
 *    +0x80).
 *    PADInitialized is an 8-byte record (playbook 116.4), which gives .sbss the target's 0x30 bytes.
 *    __PADDisableRecalibration sits in a scoped `global_optimizer off` (playbook 120): with the global optimizer on
 *    the `lis 0x8000` base and the flag byte take r4/r5 swapped; `opt_common_subs off` gives the same bytes.
 */
#include "PAD/pad.h"
#include "PAD/PADClamp.h"
#include "OS/OSContext.h"
#include "OS/OS.h"
#include "OS/OSInterrupt.h"
#include "OS/OSReset.h"
#include "OS/OSRtc.h"
#include "OS/OSTime.h"
#include "SI/SIBios.h"
#include "VI/vi.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Runtime.PPCEABI.H/memcpy.h"

/* The low-memory wireless controller fix mode (14 bits) and its flag byte (bit 0x40: recalibration disabled). */
u16 __OSWirelessPadFixMode : 0x800030E0;
#define PAD_WIRELESS_FLAGS (*(u8*)0x800030E3)

#define PAD_CHAN0_BIT 0x80000000
#define PAD_ALL_CHAN_BITS 0xF0000000

#define PAD_SPEC_0 0
#define PAD_SPEC_1 1
#define PAD_SPEC_2 2
#define PAD_SPEC_3 3
#define PAD_SPEC_4 4
#define PAD_SPEC_5 5

#define PAD_ERR_NONE 0
#define PAD_ERR_NO_CONTROLLER -1
#define PAD_ERR_NOT_READY -2
#define PAD_ERR_TRANSFER -3

#define PAD_BUTTON_A 0x0100
#define PAD_BUTTON_B 0x0200
#define PAD_BUTTON_X 0x0400
#define PAD_BUTTON_Y 0x0800
#define PAD_BUTTON_START 0x1000
#define PAD_TRIGGER_R 0x0020
#define PAD_TRIGGER_L 0x0040
#define PAD_USE_ORIGIN 0x0080
#define PAD_ORIGIN_CHANGED 0x2000

#define SI_ERROR_UNDER_RUN 0x0001
#define SI_ERROR_OVER_RUN 0x0002
#define SI_ERROR_COLLISION 0x0004
#define SI_ERROR_NO_RESPONSE 0x0008
#define SI_ERROR_MASK (SI_ERROR_UNDER_RUN | SI_ERROR_OVER_RUN | SI_ERROR_COLLISION | SI_ERROR_NO_RESPONSE)

#define SI_TYPE_MASK 0x18000000
#define SI_TYPE_GC 0x08000000
#define SI_GC_WIRELESS 0x80000000
#define SI_GC_NOMOTOR 0x20000000
#define SI_GC_STANDARD 0x01000000
#define SI_WIRELESS_RECEIVED 0x40000000
#define SI_WIRELESS_IR 0x04000000
#define SI_WIRELESS_FIX_ID 0x00100000
#define SI_WIRELESS_CONT_MASK 0x00080000
#define SI_WIRELESS_LITE 0x00040000
#define SI_GC_CONTROLLER 0x09000000

#pragma auto_inline off

/* The function a sampling interrupt forwards to. */
typedef void (*PADSamplingCallback)(void);

/* Decodes one SI response pair into a status record. */
typedef void (*PADMakeStatus)(s32 chan, PADStatus* status, u32 data[2]);

static void PADTypeAndStatusCallback(s32 chan, u32 type);
static void SPEC0_MakeStatus(s32 chan, PADStatus* status, u32 data[2]);
static void SPEC1_MakeStatus(s32 chan, PADStatus* status, u32 data[2]);
static void SPEC2_MakeStatus(s32 chan, PADStatus* status, u32 data[2]);
static BOOL __PADShutdown(BOOL final, u32 event);
static void SamplingHandler(s16 interrupt, OSContext* context);

const char* __PADVersion = "<< RVL_SDK - PAD \trelease build: Feb 27 2009 10:04:43 (0x4302_145) >>";

/* The library's init flag and the word after it. size: 0x8 */
typedef struct PADInitState {
    /* +0x00 */ BOOL initialized;
    /* +0x04 */ u32 unused_0x04;
} PADInitState;

static PADInitState PADInitialized;
static u32 EnabledBits;
static u32 ResettingBits;
static s32 ResettingChan = 32;
static u32 RecalibrateBits;
static u32 WaitingBits;
static u32 CheckingBits;
static u32 PendingBits;
static u32 XPatchBits = PAD_ALL_CHAN_BITS;
static u32 AnalogMode = 0x00000300;
static u32 BarrelBits;
s32 __PADSpec;
static u32 Spec = PAD_SPEC_5;
static PADMakeStatus MakeStatus = SPEC2_MakeStatus;

static PADStatus Origin[4];
static u32 PADTypes[4];
static u32 CmdReadOrigin = 0x41 << 24;
static u32 CmdCalibrate = 0x42 << 24;
static u32 CmdProbeDevice[4];
extern PADStatus PrevStatus[4];

static OSShutdownFunctionInfo padShutdownFunctionInfo = {__PADShutdown, 127, NULL, NULL};

static PADSamplingCallback SamplingCallback;

/* Clamps a stick axis against its origin and returns it relative to the origin. */
static inline s8 ClampS8(s8 var, s8 org)
{
    if (0 < org) {
        s8 min = (s8)(-128 + org);
        if (var < min) {
            var = min;
        }
    } else if (org < 0) {
        s8 max = (s8)(127 + org);
        if (max < var) {
            var = max;
        }
    }
    return var -= org;
}

/* Clamps a trigger against its origin and returns it relative to the origin. */
static inline u8 ClampU8(u8 var, u8 org)
{
    if (var < org) {
        var = org;
    }
    return var -= org;
}

/* 0x804D82D0 (0x174): masks the analog bits the current mode does not report and centres the stored origin. */
static void UpdateOrigin(s32 chan)
{
    PADStatus* origin;
    u32 chanBit = PAD_CHAN0_BIT >> chan;

    origin = &Origin[chan];
    switch (AnalogMode & 0x00000700u) {
    case 0x00000000u:
    case 0x00000500u:
    case 0x00000600u:
    case 0x00000700u:
        origin->triggerL &= ~15;
        origin->triggerR &= ~15;
        origin->analogA &= ~15;
        origin->analogB &= ~15;
        break;
    case 0x00000100u:
        origin->substickX &= ~15;
        origin->substickY &= ~15;
        origin->analogA &= ~15;
        origin->analogB &= ~15;
        break;
    case 0x00000200u:
        origin->substickX &= ~15;
        origin->substickY &= ~15;
        origin->triggerL &= ~15;
        origin->triggerR &= ~15;
        break;
    case 0x00000300u:
        break;
    case 0x00000400u:
        break;
    }

    origin->stickX += -128;
    origin->stickY += -128;
    origin->substickX += -128;
    origin->substickY += -128;

    if (XPatchBits & chanBit) {
        if (64 < origin->stickX && (SIGetType(chan) & 0xFFFF0000) == SI_GC_CONTROLLER) {
            origin->stickX = 0;
        }
    }
}

/* Marks a channel enabled and starts polling it. */
static inline void PADEnable(s32 chan)
{
    u32 cmd;
    u32 chanBit;
    u32 data[2];

    chanBit = PAD_CHAN0_BIT >> chan;
    EnabledBits |= chanBit;
    SIGetResponse(chan, data);
    cmd = (0x40 << 16) | AnalogMode;
    SISetCommand(chan, cmd);
    SIEnablePolling(EnabledBits);
}

/* Stops polling a channel and forgets every state bit it had. */
static inline void PADDisable(s32 chan)
{
    BOOL enabled;
    u32 chanBit;

    enabled = OSDisableInterrupts();
    chanBit = PAD_CHAN0_BIT >> chan;
    SIDisablePolling(chanBit);
    EnabledBits &= ~chanBit;
    WaitingBits &= ~chanBit;
    CheckingBits &= ~chanBit;
    PendingBits &= ~chanBit;
    BarrelBits &= ~chanBit;
    OSSetWirelessID(chan, 0);
    OSRestoreInterrupts(enabled);
}

/* Starts the type probe of the next channel waiting for a reset. */
static inline void DoReset(void)
{
    u32 chanBit;

    ResettingChan = __cntlzw(ResettingBits);
    if (ResettingChan == 32) {
        return;
    }
    chanBit = PAD_CHAN0_BIT >> ResettingChan;
    ResettingBits &= ~chanBit;
    memset(&Origin[ResettingChan], 0, sizeof(PADStatus));
    SIGetTypeAsync(ResettingChan, PADTypeAndStatusCallback);
}

/* 0x804D8450 (0xC0): stores the origin of the channel being reset, enables it and moves on to the next one. */
static void PADOriginCallback(s32 chan, u32 error, OSContext* context)
{
    if (!(error & SI_ERROR_MASK)) {
        UpdateOrigin(ResettingChan);
        PADEnable(ResettingChan);
    }
    DoReset();
}

/* 0x804D8510 (0xC8): stores a re-read origin, or disables the channel when it stopped answering. */
static void PADOriginUpdateCallback(s32 chan, u32 error, OSContext* context)
{
    if (!(EnabledBits & (PAD_CHAN0_BIT >> chan))) {
        return;
    }
    if (!(error & SI_ERROR_MASK)) {
        UpdateOrigin(chan);
    }
    if (error & SI_ERROR_NO_RESPONSE) {
        PADDisable(chan);
    }
}

/* 0x804D85E0 (0xD4): enables a probed wireless channel, marks it waiting and moves on to the next reset. */
static void PADProbeCallback(s32 chan, u32 error, OSContext* context)
{
    if (!(error & SI_ERROR_MASK)) {
        PADEnable(ResettingChan);
        WaitingBits |= PAD_CHAN0_BIT >> ResettingChan;
    }
    DoReset();
}

/* 0x804D86C0 (0x320): classifies the channel being reset and starts its origin read, calibration or probe. */
static void PADTypeAndStatusCallback(s32 chan, u32 type)
{
    u32 chanBit;
    u32 recalibrate;
    BOOL rc = TRUE;
    u32 error;

    chanBit = PAD_CHAN0_BIT >> ResettingChan;
    error = type & 0xFF;
    recalibrate = RecalibrateBits & chanBit;
    RecalibrateBits &= ~chanBit;

    if (error & SI_ERROR_MASK) {
        DoReset();
        return;
    }

    type &= ~0xFF;
    PADTypes[ResettingChan] = type;

    if ((type & SI_TYPE_MASK) != SI_TYPE_GC || !(type & SI_GC_STANDARD)) {
        DoReset();
        return;
    }

    if (Spec < PAD_SPEC_2) {
        PADEnable(ResettingChan);
        DoReset();
        return;
    }

    if (!(type & SI_GC_WIRELESS) || (type & SI_WIRELESS_IR)) {
        if (recalibrate) {
            rc = SITransfer(ResettingChan, &CmdCalibrate, 3, &Origin[ResettingChan], 10, PADOriginCallback, 0);
        } else {
            rc = SITransfer(ResettingChan, &CmdReadOrigin, 1, &Origin[ResettingChan], 10, PADOriginCallback, 0);
        }
    } else if ((type & SI_WIRELESS_FIX_ID) && !(type & SI_WIRELESS_CONT_MASK) && !(type & SI_WIRELESS_LITE)) {
        if (type & SI_WIRELESS_RECEIVED) {
            rc = SITransfer(ResettingChan, &CmdReadOrigin, 1, &Origin[ResettingChan], 10, PADOriginCallback, 0);
        } else {
            rc = SITransfer(ResettingChan, &CmdProbeDevice[ResettingChan], 3, &Origin[ResettingChan], 8,
                            PADProbeCallback, 0);
        }
    }
    if (!rc) {
        PendingBits |= chanBit;
        DoReset();
        return;
    }
}

/* 0x804D89E0 (0x13C): reads the origin of a wireless controller that answered, or disables the channel. */
static void PADReceiveCheckCallback(s32 chan, u32 type)
{
    u32 error;
    u32 chanBit;

    chanBit = PAD_CHAN0_BIT >> chan;
    if (!(EnabledBits & chanBit)) {
        return;
    }

    error = type & 0xFF;
    type &= ~0xFF;

    WaitingBits &= ~chanBit;
    CheckingBits &= ~chanBit;

    if (!(error & SI_ERROR_MASK) && (type & SI_GC_WIRELESS) && (type & SI_WIRELESS_FIX_ID) &&
        (type & SI_WIRELESS_RECEIVED) && !(type & SI_WIRELESS_IR) && !(type & SI_WIRELESS_CONT_MASK) &&
        !(type & SI_WIRELESS_LITE)) {
        SITransfer(chan, &CmdReadOrigin, 1, &Origin[chan], 10, PADOriginUpdateCallback, 0);
    } else {
        PADDisable(chan);
    }
}

BOOL PADReset(u32 mask)
{
    BOOL enabled;
    u32 disableBits;

    enabled = OSDisableInterrupts();
    mask |= PendingBits;
    PendingBits = 0;
    mask &= ~(WaitingBits | CheckingBits);
    ResettingBits |= mask;
    disableBits = ResettingBits & EnabledBits;
    EnabledBits &= ~mask;
    BarrelBits &= ~mask;
    if (Spec == PAD_SPEC_4) {
        RecalibrateBits |= mask;
    }
    SIDisablePolling(disableBits);
    if (ResettingChan == 32) {
        DoReset();
    }
    OSRestoreInterrupts(enabled);
    return TRUE;
}

BOOL PADRecalibrate(u32 mask)
{
    BOOL enabled;
    u32 disableBits;

    enabled = OSDisableInterrupts();
    mask |= PendingBits;
    PendingBits = 0;
    mask &= ~(WaitingBits | CheckingBits);
    ResettingBits |= mask;
    disableBits = ResettingBits & EnabledBits;
    EnabledBits &= ~mask;
    BarrelBits &= ~mask;
    if (!(PAD_WIRELESS_FLAGS & 0x40)) {
        RecalibrateBits |= mask;
    }
    SIDisablePolling(disableBits);
    if (ResettingChan == 32) {
        DoReset();
    }
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Selects the status decoder of a pad specification. */
static inline void PADSetSpec(u32 spec)
{
    __PADSpec = 0;
    switch (spec) {
    case PAD_SPEC_0:
        MakeStatus = SPEC0_MakeStatus;
        break;
    case PAD_SPEC_1:
        MakeStatus = SPEC1_MakeStatus;
        break;
    case PAD_SPEC_2:
    case PAD_SPEC_3:
    case PAD_SPEC_4:
    case PAD_SPEC_5:
        MakeStatus = SPEC2_MakeStatus;
        break;
    }
    Spec = spec;
}

BOOL PADInit(void)
{
    s32 chan;

    if (PADInitialized.initialized) {
        return TRUE;
    }

    OSRegisterVersion(__PADVersion);

    if (__PADSpec) {
        PADSetSpec(__PADSpec);
    }

    PADInitialized.initialized = TRUE;

    if (__PADFixBits != 0) {
        s64 time = OSGetTime();

        __OSWirelessPadFixMode = (u16)((((time) & 0xFFFF) + ((time >> 16) & 0xFFFF) + ((time >> 32) & 0xFFFF) +
                                       ((time >> 48) & 0xFFFF)) &
                                      0x3FFF);
        RecalibrateBits = PAD_ALL_CHAN_BITS;
    }

    for (chan = 0; chan < 4; ++chan) {
        CmdProbeDevice[chan] = (0x4D << 24) | (chan << 22) | ((__OSWirelessPadFixMode & 0x3FFF) << 8);
    }

    SIRefreshSamplingRate();
    OSRegisterShutdownFunction(&padShutdownFunctionInfo);

    return PADReset(PAD_ALL_CHAN_BITS);
}

u32 PADRead(PADStatus* status)
{
    BOOL enabled;
    s32 chan;
    u32 data[2];
    u32 chanBit;
    u32 sr;
    u32 motor;
    s32 threshold;

    enabled = OSDisableInterrupts();

    motor = 0;
    for (chan = 0; chan < 4; chan++, status++) {
        chanBit = PAD_CHAN0_BIT >> chan;

        if (PendingBits & chanBit) {
            PADReset(0);
            status->err = PAD_ERR_NOT_READY;
            memset(status, 0, 10);
            continue;
        }

        if ((ResettingBits & chanBit) || ResettingChan == chan) {
            status->err = PAD_ERR_NOT_READY;
            memset(status, 0, 10);
            continue;
        }

        if (!(EnabledBits & chanBit)) {
            status->err = PAD_ERR_NO_CONTROLLER;
            memset(status, 0, 10);
            continue;
        }

        if (SIIsChanBusy(chan)) {
            status->err = PAD_ERR_TRANSFER;
            memset(status, 0, 10);
            continue;
        }

        sr = SIGetStatus(chan);
        if (sr & SI_ERROR_NO_RESPONSE) {
            SIGetResponse(chan, data);

            if (WaitingBits & chanBit) {
                status->err = PAD_ERR_NONE;
                memset(status, 0, 10);

                if (!(CheckingBits & chanBit)) {
                    CheckingBits |= chanBit;
                    SIGetTypeAsync(chan, PADReceiveCheckCallback);
                }
                continue;
            }

            PADDisable(chan);
            status->err = PAD_ERR_NO_CONTROLLER;
            memset(status, 0, 10);
            continue;
        }

        if (!(SIGetType(chan) & SI_GC_NOMOTOR)) {
            motor |= chanBit;
        }

        if (!SIGetResponse(chan, data)) {
            status->err = PAD_ERR_TRANSFER;
            memset(status, 0, 10);
            continue;
        }

        if (data[0] & 0x80000000) {
            status->err = PAD_ERR_TRANSFER;
            memset(status, 0, 10);
            continue;
        }

        MakeStatus(chan, status, data);

        if ((PADTypes[chan] & 0xFFFF0000) == SI_GC_CONTROLLER && ((status->button & 0x80) ^ 0x80)) {
            threshold = 10;
        } else {
            threshold = 3;
        }

        if (__abs(__abs(status->stickX) - __abs(PrevStatus[chan].stickX)) >= threshold ||
            __abs(__abs(status->stickY) - __abs(PrevStatus[chan].stickY)) >= threshold ||
            __abs(__abs(status->substickX) - __abs(PrevStatus[chan].substickX)) >= threshold ||
            __abs(__abs(status->substickY) - __abs(PrevStatus[chan].substickY)) >= threshold ||
            __abs(__abs(status->triggerL) - __abs(PrevStatus[chan].triggerL)) >= threshold ||
            __abs(__abs(status->triggerR) - __abs(PrevStatus[chan].triggerR)) >= threshold ||
            status->button != PrevStatus[chan].button) {
            __VIResetDimmingControlB();
        }
        memcpy(&PrevStatus[chan], status, sizeof(PADStatus));

        if (status->button & PAD_ORIGIN_CHANGED) {
            status->err = PAD_ERR_TRANSFER;
            memset(status, 0, 10);
            SITransfer(chan, &CmdReadOrigin, 1, &Origin[chan], 10, PADOriginUpdateCallback, 0);
            continue;
        }

        status->err = PAD_ERR_NONE;
        status->button = status->button & ~PAD_USE_ORIGIN;
    }

    OSRestoreInterrupts(enabled);
    return motor;
}

static PADStatus PrevStatus[4];
static PADStatus PADUnusedStatus[4];

/* 0x804D9360 (0x118): decodes a specification-0 response. */
static void SPEC0_MakeStatus(s32 chan, PADStatus* status, u32 data[2])
{
    status->button = 0;
    status->button |= ((data[0] >> 16) & 0x0008) ? PAD_BUTTON_A : 0;
    status->button |= ((data[0] >> 16) & 0x0020) ? PAD_BUTTON_B : 0;
    status->button |= ((data[0] >> 16) & 0x0100) ? PAD_BUTTON_X : 0;
    status->button |= ((data[0] >> 16) & 0x0001) ? PAD_BUTTON_Y : 0;
    status->button |= ((data[0] >> 16) & 0x0010) ? PAD_BUTTON_START : 0;
    status->stickX = (s8)(data[1] >> 16);
    status->stickY = (s8)(data[1] >> 24);
    status->substickX = (s8)(data[1]);
    status->substickY = (s8)(data[1] >> 8);
    status->triggerL = (u8)(data[0] >> 8);
    status->triggerR = (u8)(data[0] & 0xFF);
    status->analogA = 0;
    status->analogB = 0;
    if (170 <= status->triggerL) {
        status->button |= PAD_TRIGGER_L;
    }
    if (170 <= status->triggerR) {
        status->button |= PAD_TRIGGER_R;
    }
    status->stickX += -128;
    status->stickY += -128;
    status->substickX += -128;
    status->substickY += -128;
}

/* 0x804D9480 (0x118): decodes a specification-1 response. */
static void SPEC1_MakeStatus(s32 chan, PADStatus* status, u32 data[2])
{
    status->button = 0;
    status->button |= ((data[0] >> 16) & 0x0080) ? PAD_BUTTON_A : 0;
    status->button |= ((data[0] >> 16) & 0x0100) ? PAD_BUTTON_B : 0;
    status->button |= ((data[0] >> 16) & 0x0020) ? PAD_BUTTON_X : 0;
    status->button |= ((data[0] >> 16) & 0x0010) ? PAD_BUTTON_Y : 0;
    status->button |= ((data[0] >> 16) & 0x0200) ? PAD_BUTTON_START : 0;
    status->stickX = (s8)(data[1] >> 16);
    status->stickY = (s8)(data[1] >> 24);
    status->substickX = (s8)(data[1]);
    status->substickY = (s8)(data[1] >> 8);
    status->triggerL = (u8)(data[0] >> 8);
    status->triggerR = (u8)(data[0] & 0xFF);
    status->analogA = 0;
    status->analogB = 0;
    if (170 <= status->triggerL) {
        status->button |= PAD_TRIGGER_L;
    }
    if (170 <= status->triggerR) {
        status->button |= PAD_TRIGGER_R;
    }
    status->stickX += -128;
    status->stickY += -128;
    status->substickX += -128;
    status->substickY += -128;
}

/* 0x804D95A0 (0x40C): decodes a specification-2 response for the current analog mode and clamps it to the origin. */
static void SPEC2_MakeStatus(s32 chan, PADStatus* status, u32 data[2])
{
    PADStatus* origin;

    status->button = (u16)((data[0] >> 16) & 0x3FFF);
    status->stickX = (s8)(data[0] >> 8);
    status->stickY = (s8)(data[0]);

    switch (AnalogMode & 0x00000700) {
    case 0x00000000:
    case 0x00000500:
    case 0x00000600:
    case 0x00000700:
        status->substickX = (s8)(data[1] >> 24);
        status->substickY = (s8)(data[1] >> 16);
        status->triggerL = (u8)(((data[1] >> 12) & 0x0F) << 4);
        status->triggerR = (u8)(((data[1] >> 8) & 0x0F) << 4);
        status->analogA = (u8)(((data[1] >> 4) & 0x0F) << 4);
        status->analogB = (u8)(((data[1] >> 0) & 0x0F) << 4);
        break;
    case 0x00000100:
        status->substickX = (s8)(((data[1] >> 28) & 0x0F) << 4);
        status->substickY = (s8)(((data[1] >> 24) & 0x0F) << 4);
        status->triggerL = (u8)(data[1] >> 16);
        status->triggerR = (u8)(data[1] >> 8);
        status->analogA = (u8)(((data[1] >> 4) & 0x0F) << 4);
        status->analogB = (u8)(((data[1] >> 0) & 0x0F) << 4);
        break;
    case 0x00000200:
        status->substickX = (s8)(((data[1] >> 28) & 0x0F) << 4);
        status->substickY = (s8)(((data[1] >> 24) & 0x0F) << 4);
        status->triggerL = (u8)(((data[1] >> 20) & 0x0F) << 4);
        status->triggerR = (u8)(((data[1] >> 16) & 0x0F) << 4);
        status->analogA = (u8)(data[1] >> 8);
        status->analogB = (u8)(data[1] >> 0);
        break;
    case 0x00000300:
        status->substickX = (s8)(data[1] >> 24);
        status->substickY = (s8)(data[1] >> 16);
        status->triggerL = (u8)(data[1] >> 8);
        status->triggerR = (u8)(data[1] >> 0);
        status->analogA = 0;
        status->analogB = 0;
        break;
    case 0x00000400:
        status->substickX = (s8)(data[1] >> 24);
        status->substickY = (s8)(data[1] >> 16);
        status->triggerL = 0;
        status->triggerR = 0;
        status->analogA = (u8)(data[1] >> 8);
        status->analogB = (u8)(data[1] >> 0);
        break;
    }

    status->stickX += -128;
    status->stickY += -128;
    status->substickX += -128;
    status->substickY += -128;

    if ((PADTypes[chan] & 0xFFFF0000) == SI_GC_CONTROLLER && ((status->button & 0x80) ^ 0x80)) {
        BarrelBits |= PAD_CHAN0_BIT >> chan;
        status->stickX = 0;
        status->stickY = 0;
        status->substickX = 0;
        status->substickY = 0;
        return;
    } else {
        BarrelBits &= ~(PAD_CHAN0_BIT >> chan);
    }

    origin = &Origin[chan];
    status->stickX = ClampS8(status->stickX, origin->stickX);
    status->stickY = ClampS8(status->stickY, origin->stickY);
    status->substickX = ClampS8(status->substickX, origin->substickX);
    status->substickY = ClampS8(status->substickY, origin->substickY);
    status->triggerL = ClampU8(status->triggerL, origin->triggerL);
    status->triggerR = ClampU8(status->triggerR, origin->triggerR);
}

/* Removes the sampling callback and its SI polling handler. */
static inline void PADClearSamplingCallback(void)
{
    SamplingCallback = NULL;
    SIUnregisterPollingHandler(SamplingHandler);
}

/* Whether no controller reset is pending and SI is idle. */
static inline BOOL PADSync(void)
{
    return ResettingBits == 0 && ResettingChan == 32 && !SIBusy();
}

/* 0x804D99B0 (0xC8): the shutdown hook: drops the sampling callback and recalibrates once before a reset. */
static BOOL __PADShutdown(BOOL final, u32 event)
{
    static BOOL recalibrated = FALSE;
    BOOL sync;

    if (SamplingCallback) {
        PADClearSamplingCallback();
    }

    if (!final) {
        sync = PADSync();
        if (!recalibrated && sync) {
            recalibrated = PADRecalibrate(PAD_ALL_CHAN_BITS);
            return FALSE;
        }
        return sync;
    } else {
        recalibrated = FALSE;
    }
    return TRUE;
}

/* 0x804D9A80 (0x60): the SI polling handler: runs the sampling callback on a clean context. */
static void SamplingHandler(s16 interrupt, OSContext* context)
{
    OSContext exceptionContext;

    if (SamplingCallback) {
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(&exceptionContext);
        SamplingCallback();
        OSClearContext(&exceptionContext);
        OSSetCurrentContext(context);
    }
}

#pragma push
#pragma global_optimizer off
BOOL __PADDisableRecalibration(BOOL disable)
{
    BOOL enabled;
    BOOL prev;

    enabled = OSDisableInterrupts();
    prev = (PAD_WIRELESS_FLAGS & 0x40) ? TRUE : FALSE;
    PAD_WIRELESS_FLAGS = PAD_WIRELESS_FLAGS & ~0x40;
    if (disable) {
        PAD_WIRELESS_FLAGS |= 0x40;
    }
    OSRestoreInterrupts(enabled);
    return prev;
}
#pragma pop
