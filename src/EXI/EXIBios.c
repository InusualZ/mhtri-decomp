/*
 * EXI/EXIBios.c - the Revolution SDK EXI bus driver (EXIBios.c).
 *
 * RANGE. `.text` 0x804AFED0-0x804B17D0 (19 functions / 0x1900 B); `.data` 0x8061A4F0-0x8061A538 (the version banner);
 *   `.bss` 0x80746B60-0x80746C20 (`Ecb`, 3 channel records of 0x40 B); `.sdata` 0x80793E30 (`__EXIVersion`);
 *   `.sbss` 0x80795198 (`IDSerialPort1`).
 *   - right edge: `EXI/ProbeBarnacle.c` starts at 0x804B17D0; left edge: `WriteUARTN` (the UART layer) calls into this
 *     range, so it is `EUART/EUART.c`'s
 *   - `__EXIVersion` is read by `EXIInit` only and `Ecb` / `IDSerialPort1` have no other referrer, so all three are
 *     this file's own
 *   - `__OSInIPL` (`.sbss` 0x807952C8) is the OS's global (16 readers across DVD and EXI): read through `OS/OS.h`
 * FLAGS. the `OS` lib block (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16: every start in the EXI band is
 *   16-aligned) plus `#pragma scheduling off` for the whole file (the byte pack loops and the epilogues are in
 *   program order).  Inlining follows definition order: `EXIGetID` inlines `EXIDetach`, `EXIUnlock` and
 *   `EXIAttachDevice`; the latter is `static inline` so no standalone copy is emitted.
 * NAMES. the SDK's own names for every symbol; the `EXIControl` fields (`exiCallback` .. `queue`), the `EXI_STATE_*`
 *   bits, `EXI_PROBE_TIME` (low memory 0x800030C0, two words) and `EXIAttachDevice` (the inline body shared by
 *   `EXIAttach` and `EXIGetID`) are GUESSes from their use.
 * RESIDUALS. `EXISync` and `EXISelect` differ only in register choice (`exi` / the hoisted `chan * 0x14` swap in
 *   `EXISync`; the `regs` temp and the address temp swap in `EXISelect`); locals order, one-expression and
 *   declaration-with-initialiser forms were measured.  flipcheck: `.text` is 8 B short of the claim (the pad after
 *   `EXIGetID`, where `ProbeBarnacle` starts 16-aligned) and the 0x48 B `.data` claim holds the 0x46 B banner.
 * SHAPES. the byte pack/unpack loops of `EXIImm` / `EXISync` / `TCIntrruptHandler` are plain `for` loops over a
 *   local length and a walking pointer; `__EXIRegs[3][5]` and `EXI_PROBE_TIME[2]` are placed at their hardware
 *   addresses with the `: address` declarator, which is what makes the compiler emit `lis` + `add` with no
 *   relocation (a cast constant folds to `addis`).
 */

#pragma scheduling off
#include "types.h"
#include "EXI/EXIBios.h"
#include "EXI/ProbeBarnacle.h"
#include "MSL_C/alloc.h"
#include "OS/OS.h"
#include "OS/OSContext.h"
#include "OS/OSInterrupt.h"
#include "OS/OSTime.h"

#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

/* The three channels' register blocks: control/status, DMA address, DMA length, transfer control, immediate data.
 * Placed at its hardware address (no relocation in the object). */
volatile u32 __EXIRegs[3][5] : 0xCD006800;

/* The per-channel probe timestamps the OS keeps in low memory (no relocation in the object), and the disc's device
 * code word. */
volatile s32 EXI_PROBE_TIME[2] : 0x800030C0;
#define OS_DEVICE_CODE (*(volatile u16*)0x800030E6)

/* `EXIControl::state` bits. */
#define EXI_STATE_DMA 0x01
#define EXI_STATE_IMM 0x02
#define EXI_STATE_BUSY (EXI_STATE_DMA | EXI_STATE_IMM)
#define EXI_STATE_SELECTED 0x04
#define EXI_STATE_ATTACHED 0x08
#define EXI_STATE_LOCKED 0x10

typedef struct EXIQueueEntry {
    /* +0x00 */ u32 dev;
    /* +0x04 */ EXICallback callback;
} EXIQueueEntry; /* size: 0x08 */

typedef struct EXIControl {
    /* +0x00 */ EXICallback exiCallback;
    /* +0x04 */ EXICallback tcCallback;
    /* +0x08 */ EXICallback extCallback;
    /* +0x0C */ volatile u32 state;
    /* +0x10 */ s32 immLen;
    /* +0x14 */ u8* immBuf;
    /* +0x18 */ u32 dev;
    /* +0x1C */ u32 id;
    /* +0x20 */ s32 idTime;
    /* +0x24 */ s32 items;
    /* +0x28 */ EXIQueueEntry queue[3];
} EXIControl; /* size: 0x40 */

static EXIControl Ecb[3];
static u32 IDSerialPort1;
static const char* __EXIVersion = "<< RVL_SDK - EXI \trelease build: Feb 27 2009 10:02:03 (0x4302_145) >>";

/* Masks or unmasks the channel's EXI interrupt according to whether a callback or a lock wants it. */
static void SetExiInterruptMask(EXIChannel chan, EXIControl* exi)
{
    EXIControl* exi2 = &Ecb[EXI_CHAN_2];

    switch (chan) {
    case EXI_CHAN_0:
        if ((exi->exiCallback == NULL && exi2->exiCallback == NULL) || (exi->state & EXI_STATE_LOCKED)) {
            __OSMaskInterrupts(0x410000);
        } else {
            __OSUnmaskInterrupts(0x410000);
        }
        break;
    case EXI_CHAN_1:
        if (exi->exiCallback == NULL || (exi->state & EXI_STATE_LOCKED)) {
            __OSMaskInterrupts(0x80000);
        } else {
            __OSUnmaskInterrupts(0x80000);
        }
        break;
    case EXI_CHAN_2:
        if (__OSGetInterruptHandler(25) == NULL || (exi->state & EXI_STATE_LOCKED)) {
            __OSMaskInterrupts(0x40);
        } else {
            __OSUnmaskInterrupts(0x40);
        }
        break;
    }
}

/* Starts an immediate transfer of up to four bytes on a selected channel. */
/* untyped: the byte range of a transfer */
BOOL EXIImm(EXIChannel chan, void* buf, s32 len, u32 type, EXICallback callback)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();
    s32 i;
    u32 data;

    if ((exi->state & EXI_STATE_BUSY) || !(exi->state & EXI_STATE_SELECTED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->tcCallback = callback;
    if (exi->tcCallback) {
        __EXIRegs[chan][0] = (__EXIRegs[chan][0] & 0x7F5) | 0x08;
        __OSUnmaskInterrupts(0x200000u >> (chan * 3));
    }

    exi->state |= EXI_STATE_IMM;

    if (type != EXI_READ) {
        data = 0;
        for (i = 0; i < len; i++) {
            data |= ((u8*)buf)[i] << ((3 - i) * 8);
        }
        __EXIRegs[chan][4] = data;
    }

    exi->immBuf = buf;
    exi->immLen = (type != EXI_WRITE) ? len : 0;

    __EXIRegs[chan][3] = (type << 2) | 0x01 | ((len - 1) << 4);

    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Transfers a buffer of any length through four-byte immediate transfers. */
/* untyped: the byte range of a transfer */
BOOL EXIImmEx(EXIChannel chan, void* buf, s32 len, u32 type)
{
    s32 xLen;

    while (len) {
        xLen = (len < 4) ? len : 4;
        if (!EXIImm(chan, buf, xLen, type, NULL)) {
            return FALSE;
        }
        if (!EXISync(chan)) {
            return FALSE;
        }
        buf = (u8*)buf + xLen;
        len -= xLen;
    }
    return TRUE;
}

/* Starts a DMA transfer on a selected channel. */
/* untyped: the byte range of a transfer */
BOOL EXIDma(EXIChannel chan, void* buf, s32 len, u32 type, EXICallback callback)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_BUSY) || !(exi->state & EXI_STATE_SELECTED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->tcCallback = callback;
    if (exi->tcCallback) {
        __EXIRegs[chan][0] = (__EXIRegs[chan][0] & 0x7F5) | 0x08;
        __OSUnmaskInterrupts(0x200000u >> (chan * 3));
    }

    exi->state |= EXI_STATE_DMA;

    __EXIRegs[chan][1] = (u32)buf & 0xFFFFFFE0;
    __EXIRegs[chan][2] = len;
    __EXIRegs[chan][3] = (type << 2) | 0x03;

    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Waits for the channel's transfer to finish and copies an immediate read back to its buffer. */
BOOL EXISync(EXIChannel chan)
{
    EXIControl* exi;
    BOOL rc;
    BOOL enabled;
    s32 i;
    u8* buf;
    s32 len;
    u32 data;

    exi = &Ecb[chan];
    rc = FALSE;
    while (exi->state & EXI_STATE_SELECTED) {
        if (!(__EXIRegs[chan][3] & 0x01)) {
            enabled = OSDisableInterrupts();
            if (exi->state & EXI_STATE_SELECTED) {
                if (exi->state & EXI_STATE_BUSY) {
                    if ((exi->state & EXI_STATE_IMM) && (len = exi->immLen)) {
                        buf = exi->immBuf;
                        data = __EXIRegs[chan][4];
                        for (i = 0; i < len; i++) {
                            *buf++ = (u8)(data >> ((3 - i) * 8));
                        }
                    }
                    exi->state &= ~EXI_STATE_BUSY;
                }
                if (__OSGetDIConfig() != 0xFF || (OSGetConsoleType() & 0xF0000000) == 0x20000000 ||
                    exi->immLen != 4 || (__EXIRegs[chan][0] & 0x70) ||
                    (__EXIRegs[chan][4] != 0x01010000 && __EXIRegs[chan][4] != 0x05070000 &&
                     __EXIRegs[chan][4] != 0x04220001) ||
                    OS_DEVICE_CODE == 0x8200) {
                    rc = TRUE;
                }
            }
            OSRestoreInterrupts(enabled);
            break;
        }
    }
    return rc;
}

/* Installs the channel's EXI interrupt callback and returns the previous one. */
EXICallback EXISetExiCallback(EXIChannel chan, EXICallback callback)
{
    EXIControl* exi = &Ecb[chan];
    EXICallback prev;
    BOOL enabled = OSDisableInterrupts();

    prev = exi->exiCallback;
    exi->exiCallback = callback;

    if (chan != EXI_CHAN_2) {
        SetExiInterruptMask(chan, exi);
    } else {
        SetExiInterruptMask(EXI_CHAN_0, &Ecb[EXI_CHAN_0]);
    }

    OSRestoreInterrupts(enabled);
    return prev;
}

/* Reports whether a device has been in the slot long enough to be considered present. */
static BOOL __EXIProbe(EXIChannel chan)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled;
    BOOL rc;
    u32 status;
    s32 t;

    if (chan == EXI_CHAN_2) {
        return TRUE;
    }

    rc = TRUE;
    enabled = OSDisableInterrupts();
    status = __EXIRegs[chan][0];
    if (!(exi->state & EXI_STATE_ATTACHED)) {
        if (status & 0x800) {
            __EXIRegs[chan][0] = (__EXIRegs[chan][0] & 0x7F5) | 0x800;
            exi->idTime = 0;
            EXI_PROBE_TIME[chan] = 0;
        }
        if (status & 0x1000) {
            t = (s32)(OSGetTime() / (OS_TIMER_CLOCK / 1000) / 100) + 1;
            if (EXI_PROBE_TIME[chan] == 0) {
                EXI_PROBE_TIME[chan] = t;
            }
            if (t - EXI_PROBE_TIME[chan] < 3) {
                rc = FALSE;
            }
        } else {
            exi->idTime = 0;
            EXI_PROBE_TIME[chan] = 0;
            rc = FALSE;
        }
    } else if (!(status & 0x1000) || (status & 0x800)) {
        exi->idTime = 0;
        EXI_PROBE_TIME[chan] = 0;
        rc = FALSE;
    }
    OSRestoreInterrupts(enabled);
    return rc;
}

/* Arms the card-detect interrupt of a channel when a device is present and not yet attached. */
static inline BOOL EXIAttachDevice(EXIChannel chan, EXICallback callback)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_ATTACHED) || !__EXIProbe(chan)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    __EXIRegs[chan][0] = (__EXIRegs[chan][0] & 0x7F5) | 0x02;
    exi->extCallback = callback;
    __OSUnmaskInterrupts(0x100000u >> (chan * 3));
    exi->state |= EXI_STATE_ATTACHED;

    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Enables the card-detect interrupt of a channel and records the callback to run on removal. */
BOOL EXIAttach(EXIChannel chan, EXICallback callback)
{
    EXIControl* exi = &Ecb[chan];
    u32 id;
    BOOL rc;
    BOOL enabled;

    if (__EXIProbe(chan) && exi->idTime == 0) {
        EXIGetID(chan, 0, &id);
    }

    enabled = OSDisableInterrupts();
    if (exi->idTime == 0) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    rc = EXIAttachDevice(chan, callback);
    OSRestoreInterrupts(enabled);
    return rc;
}

/* Disables the card-detect interrupt of a channel. */
BOOL EXIDetach(EXIChannel chan)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();

    if (!(exi->state & EXI_STATE_ATTACHED)) {
        OSRestoreInterrupts(enabled);
        return TRUE;
    }
    if ((exi->state & EXI_STATE_LOCKED) && exi->dev == 0) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state &= ~EXI_STATE_ATTACHED;
    __OSMaskInterrupts(0x500000u >> (chan * 3));
    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Asserts the chip select of a device on a locked channel. */
BOOL EXISelect(EXIChannel chan, u32 dev, u32 freq)
{
    u32 regs;
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();

    if ((exi->state & EXI_STATE_SELECTED) ||
        (chan != EXI_CHAN_2 &&
         ((dev == 0 && !(exi->state & EXI_STATE_ATTACHED) && !__EXIProbe(chan)) ||
          !(exi->state & EXI_STATE_LOCKED) || exi->dev != dev))) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state |= EXI_STATE_SELECTED;

    regs = __EXIRegs[chan][0];
    regs &= 0x405;
    regs |= (freq << 4);
    regs |= (1 << dev) << 7;
    __EXIRegs[chan][0] = regs;

    if (exi->state & EXI_STATE_ATTACHED) {
        switch (chan) {
        case EXI_CHAN_0:
            __OSMaskInterrupts(0x100000);
            break;
        case EXI_CHAN_1:
            __OSMaskInterrupts(0x20000);
            break;
        }
    }

    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Releases the chip select of the channel. */
BOOL EXIDeselect(EXIChannel chan)
{
    EXIControl* exi = &Ecb[chan];
    u32 regs;
    BOOL enabled = OSDisableInterrupts();

    if (!(exi->state & EXI_STATE_SELECTED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state &= ~EXI_STATE_SELECTED;

    regs = __EXIRegs[chan][0];
    __EXIRegs[chan][0] = regs & 0x405;

    if (exi->state & EXI_STATE_ATTACHED) {
        switch (chan) {
        case EXI_CHAN_0:
            __OSUnmaskInterrupts(0x100000);
            break;
        case EXI_CHAN_1:
            __OSUnmaskInterrupts(0x20000);
            break;
        }
    }

    OSRestoreInterrupts(enabled);

    if (chan != EXI_CHAN_2 && (regs & 0x80)) {
        if (__EXIProbe(chan)) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

/* The EXI interrupt of a channel: acknowledges it and runs the channel's callback under a scratch context. */
static void EXIIntrruptHandler(s16 interrupt, OSContext* context)
{
    EXIChannel chan;
    EXIControl* exi;
    EXICallback callback;

    chan = (EXIChannel)((interrupt - 9) / 3);
    __EXIRegs[chan][0] = (__EXIRegs[chan][0] & 0x7F5) | 0x02;

    exi = &Ecb[chan];
    callback = exi->exiCallback;
    if (callback) {
        OSContext exiContext;

        OSClearContext(&exiContext);
        OSSetCurrentContext(&exiContext);

        callback(chan, context);

        OSClearContext(&exiContext);
        OSSetCurrentContext(context);
    }
}

/* The transfer-complete interrupt of a channel: copies an immediate read back and runs the callback. */
static void TCIntrruptHandler(s16 interrupt, OSContext* context)
{
    EXIControl* exi;
    s32 i;
    u8* buf;
    s32 len;
    EXIChannel chan;
    EXICallback callback;
    u32 data;

    chan = (EXIChannel)((interrupt - 10) / 3);
    exi = &Ecb[chan];
    __OSMaskInterrupts(0x80000000 >> interrupt);
    __EXIRegs[chan][0] = (__EXIRegs[chan][0] & 0x7F5) | 0x08;

    callback = exi->tcCallback;
    if (callback) {
        OSContext exiContext;

        exi->tcCallback = NULL;

        if (exi->state & EXI_STATE_BUSY) {
            if ((exi->state & EXI_STATE_IMM) && (len = exi->immLen)) {
                buf = exi->immBuf;
                data = __EXIRegs[chan][4];
                for (i = 0; i < len; i++) {
                    *buf++ = (u8)(data >> ((3 - i) * 8));
                }
            }
            exi->state &= ~EXI_STATE_BUSY;
        }

        OSClearContext(&exiContext);
        OSSetCurrentContext(&exiContext);

        callback(chan, context);

        OSClearContext(&exiContext);
        OSSetCurrentContext(context);
    }
}

/* The card-detect interrupt of a channel: masks it, drops the attached state and runs the removal callback. */
static void EXTIntrruptHandler(s16 interrupt, OSContext* context)
{
    EXIChannel chan;
    EXIControl* exi;
    EXICallback callback;

    chan = (EXIChannel)((interrupt - 11) / 3);
    __OSMaskInterrupts(0x500000u >> (chan * 3));
    exi = &Ecb[chan];
    callback = exi->extCallback;
    exi->state &= ~EXI_STATE_ATTACHED;

    if (callback) {
        OSContext exiContext;

        OSClearContext(&exiContext);
        OSSetCurrentContext(&exiContext);

        exi->extCallback = NULL;
        callback(chan, context);

        OSClearContext(&exiContext);
        OSSetCurrentContext(context);
    }
}

/* Quiesces the bus, installs the interrupt handlers and enables the barnacle when a debugger is attached. */
void EXIInit(void)
{
    u32 id;

    while ((__EXIRegs[0][3] & 0x01) == 1 || (__EXIRegs[1][3] & 0x01) == 1 || (__EXIRegs[2][3] & 0x01) == 1) {
    }

    __OSMaskInterrupts(0x7F8000);

    __EXIRegs[0][0] = 0;
    __EXIRegs[1][0] = 0;
    __EXIRegs[2][0] = 0;

    __EXIRegs[0][0] = 0x2000;

    __OSSetInterruptHandler(9, EXIIntrruptHandler);
    __OSSetInterruptHandler(10, TCIntrruptHandler);
    __OSSetInterruptHandler(11, EXTIntrruptHandler);
    __OSSetInterruptHandler(12, EXIIntrruptHandler);
    __OSSetInterruptHandler(13, TCIntrruptHandler);
    __OSSetInterruptHandler(14, EXTIntrruptHandler);
    __OSSetInterruptHandler(15, EXIIntrruptHandler);
    __OSSetInterruptHandler(16, TCIntrruptHandler);

    EXIGetID(EXI_CHAN_0, EXI_DEV_NET, &IDSerialPort1);

    if (__OSInIPL) {
        EXI_PROBE_TIME[1] = 0;
        EXI_PROBE_TIME[0] = 0;
        Ecb[EXI_CHAN_1].idTime = 0;
        Ecb[EXI_CHAN_0].idTime = 0;
        __EXIProbe(EXI_CHAN_0);
        __EXIProbe(EXI_CHAN_1);
    } else if (EXIGetID(EXI_CHAN_0, EXI_DEV_EXT, &id) && id == 0x07010000) {
        __OSEnableBarnacle(EXI_CHAN_1, EXI_DEV_EXT);
    } else if (EXIGetID(EXI_CHAN_1, EXI_DEV_EXT, &id) && id == 0x07010000) {
        __OSEnableBarnacle(EXI_CHAN_0, EXI_DEV_NET);
    }

    OSRegisterVersion(__EXIVersion);
}

/* Takes the lock of a channel for a device, or queues the callback to run when it is free. */
BOOL EXILock(EXIChannel chan, u32 dev, EXICallback callback)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();
    s32 i;

    if (exi->state & EXI_STATE_LOCKED) {
        if (callback) {
            for (i = 0; i < exi->items; i++) {
                if (exi->queue[i].dev == dev) {
                    OSRestoreInterrupts(enabled);
                    return FALSE;
                }
            }
            exi->queue[exi->items].callback = callback;
            exi->queue[exi->items].dev = dev;
            exi->items++;
        }
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state |= EXI_STATE_LOCKED;
    exi->dev = dev;

    SetExiInterruptMask(chan, exi);

    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* Releases the lock of a channel and hands it to the first queued waiter. */
BOOL EXIUnlock(EXIChannel chan)
{
    EXIControl* exi = &Ecb[chan];
    BOOL enabled = OSDisableInterrupts();
    EXICallback unlockedCallback;

    if (!(exi->state & EXI_STATE_LOCKED)) {
        OSRestoreInterrupts(enabled);
        return FALSE;
    }

    exi->state &= ~EXI_STATE_LOCKED;

    SetExiInterruptMask(chan, exi);

    if (exi->items > 0) {
        unlockedCallback = exi->queue[0].callback;
        if (--exi->items > 0) {
            memmove(&exi->queue[0], &exi->queue[1], exi->items * sizeof(exi->queue[0]));
        }
        unlockedCallback(chan, 0);
    }

    OSRestoreInterrupts(enabled);
    return TRUE;
}

/* The lock-release callback `EXIGetID` queues: re-reads the id of the memory-card slot. */
static void UnlockedHandler(EXIChannel chan, OSContext* context)
{
    u32 id;

    EXIGetID(chan, 0, &id);
}

/* Reads the 32-bit device id of a channel/device pair, caching it for the slots 0 and 1. */
BOOL EXIGetID(EXIChannel chan, u32 dev, u32* id)
{
    EXIControl* exi = &Ecb[chan];
    BOOL err;
    u32 cid;
    s32 startTime;
    BOOL enabled;

    if (chan == EXI_CHAN_0 && dev == EXI_DEV_NET && IDSerialPort1 != 0) {
        *id = IDSerialPort1;
        return TRUE;
    }

    if (chan < EXI_CHAN_2 && dev == EXI_DEV_EXT) {
        if (!__EXIProbe(chan)) {
            return FALSE;
        }
        if (exi->idTime == EXI_PROBE_TIME[chan]) {
            *id = exi->id;
            return exi->idTime;
        }
        if (!EXIAttachDevice(chan, NULL)) {
            return FALSE;
        }
        startTime = EXI_PROBE_TIME[chan];
    }

    enabled = OSDisableInterrupts();

    err = !EXILock(chan, dev, (chan < EXI_CHAN_2 && dev == EXI_DEV_EXT) ? UnlockedHandler : NULL);
    if (!err) {
        err = !EXISelect(chan, dev, EXI_FREQ_1MHZ);
        if (!err) {
            cid = 0;
            err |= !EXIImm(chan, &cid, 2, EXI_WRITE, NULL);
            err |= !EXISync(chan);
            err |= !EXIImm(chan, id, 4, EXI_READ, NULL);
            err |= !EXISync(chan);
            err |= !EXIDeselect(chan);
        }
        EXIUnlock(chan);
    }

    OSRestoreInterrupts(enabled);

    if (chan < EXI_CHAN_2 && dev == EXI_DEV_EXT) {
        EXIDetach(chan);
        enabled = OSDisableInterrupts();
        err |= (startTime != EXI_PROBE_TIME[chan]);
        if (!err) {
            exi->id = *id;
            exi->idTime = startTime;
        }
        OSRestoreInterrupts(enabled);
        return err ? FALSE : exi->idTime;
    }

    return err ? FALSE : TRUE;
}
