/*
 * OS/OSRtc.c - the OS real-time clock and SRAM access: SRAM init/sync, wireless ID and the RTC flags.
 * RANGE. .text 0x804D2B90-0x804D3640 (9 functions); .bss 0x8074D640-0x8074D698.  Cut from the OS core band
 *    0x804C1760-0x804D9B4C.  Evidence: the 0x54-byte control block `Scb` (.bss 0x8074D640) is read by
 *    `WriteSramCallback`, `__OSInitSram`, `UnlockSram`, `__OSSyncSram`, `OSGetWirelessID` and `OSSetWirelessID`, and by
 *    nothing outside the range.
 * FLAGS. `cflags_base` per object (configure.py).
 * NAMES. `__OSInitSram`, `UnlockSram`, `__OSSyncSram`, `OSGetWirelessID`, ... are the map's names; GUESS: `WriteSramCallback`
 *    (the write-back retry the EXI lock runs), `__OSReadROM` (the boot-ROM read `OSFont` calls), `statusBits` (the SRAM word
 *    at +0x3C that `__OSInitSram` repairs).
 * RESIDUALS. `UnlockSram` (93.6 %): the checksum loop keeps a `clrlwi` after each `nor` and the Scb base is formed earlier than in the target;
 *    `__OSInitSram` (99.0 %): the second LockSram result takes r3 where the target uses r5. No source spelling tried changed either.
 * SHAPES. the EXI write sequence is an inline helper expanded in `WriteSramCallback` and `UnlockSram`; lock/unlock of the
 *    control block are inline helpers.
 */

#include "types.h"

#include "EXI/EXIBios.h"
#include "OS/DCInvalidateRange.h"
#include "OS/OSInterrupt.h"
#include "OS/OSRtc.h"

/* size: 0x14 - the SRAM header */
typedef struct OSSram {
    /* +0x00 */ u16 checkSum;
    /* +0x02 */ u16 checkSumInv;
    /* +0x04 */ u32 ead0;
    /* +0x08 */ u32 ead1;
    /* +0x0C */ u32 counterBias;
    /* +0x10 */ s8 displayOffsetH;
    /* +0x11 */ u8 ntd;
    /* +0x12 */ u8 language;
    /* +0x13 */ u8 flags;
} OSSram; /* size: 0x14 */

/* size: 0x2C - the SRAM extension that follows the header */
typedef struct OSSramEx {
    /* +0x00 */ u8 flashID[2][12];
    /* +0x18 */ u32 wirelessKeyboardID;
    /* +0x1C */ u16 wirelessPadID[4];
    /* +0x24 */ u8 dvdErrorCode;
    /* +0x25 */ u8 pad_0x25;
    /* +0x26 */ u8 flashIDCheckSum[2];
    /* +0x28 */ u16 statusBits;
    /* +0x2A */ u8 pad_0x2A[2];
} OSSramEx; /* size: 0x2C */

/* size: 0x50 - the SRAM image and its lock state */
typedef struct SramControl {
    /* +0x00 */ u8 sram[0x40];
    /* +0x40 */ u32 offset;   /* first byte not yet written back */
    /* +0x44 */ BOOL enabled; /* the interrupt state to restore on unlock */
    /* +0x48 */ BOOL locked;
    /* +0x4C */ BOOL sync;    /* the last write-back's result */
} SramControl;

static SramControl Scb __attribute__((aligned(32)));

#define SRAM_EX_OFFSET 0x14

static void WriteSramCallback(EXIChannel chan, struct OSContext* context);

/* Writes `size` bytes at SRAM offset `offset` through the EXI bus; returns whether every step succeeded. */
/* untyped: the byte range of a transfer */
static inline BOOL WriteSram(void* buffer, u32 offset, u32 size)
{
    BOOL err;
    u32 cmd;

    if (!EXILock(EXI_CHAN_0, EXI_DEV_INT, WriteSramCallback)) {
        return FALSE;
    }
    if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, EXI_FREQ_8MHZ)) {
        EXIUnlock(EXI_CHAN_0);
        return FALSE;
    }
    offset <<= 6;
    cmd = (offset + 0x100) | 0xA0000000;
    err = FALSE;
    err |= !EXIImm(EXI_CHAN_0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIImmEx(EXI_CHAN_0, buffer, size, EXI_WRITE);
    err |= !EXIDeselect(EXI_CHAN_0);
    EXIUnlock(EXI_CHAN_0);
    return !err;
}

/* Retries the SRAM write-back when the EXI lock becomes available. */
static void WriteSramCallback(EXIChannel chan, struct OSContext* context)
{
    Scb.sync = WriteSram(Scb.sram + Scb.offset, Scb.offset, 0x40 - Scb.offset);
    if (Scb.sync) {
        Scb.offset = 0x40;
    }
}

/* Takes the control block and returns the SRAM bytes from `offset`, or NULL when it is already taken. */
static inline u8* LockSram(u32 offset)
{
    BOOL enabled = OSDisableInterrupts();

    if (Scb.locked) {
        OSRestoreInterrupts(enabled);
        return NULL;
    }
    Scb.enabled = enabled;
    Scb.locked = TRUE;
    return Scb.sram + offset;
}

/* Reads the whole SRAM into the control block. */
/* untyped: the byte range of a transfer */
static inline BOOL ReadSram(void* buffer)
{
    BOOL err;
    u32 cmd;

    DCInvalidateRange(buffer, 0x40);
    if (!EXILock(EXI_CHAN_0, EXI_DEV_INT, NULL)) {
        return FALSE;
    }
    if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, EXI_FREQ_8MHZ)) {
        EXIUnlock(EXI_CHAN_0);
        return FALSE;
    }
    cmd = 0x20000100;
    err = FALSE;
    err |= !EXIImm(EXI_CHAN_0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIDma(EXI_CHAN_0, buffer, 0x40, EXI_READ, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIDeselect(EXI_CHAN_0);
    EXIUnlock(EXI_CHAN_0);
    return !err;
}

/* Recomputes the checksums when `commit` is set, writes the changed bytes back and releases the control block. */
static BOOL UnlockSram(BOOL commit, u32 offset)
{
    u16* p;

    if (commit) {
        OSSram* sram = (OSSram*)Scb.sram;

        if (offset == 0) {
            if ((u32)(sram->flags & 3) > 2) {
                sram->flags &= ~3;
            }
            sram->checkSumInv = sram->checkSum = 0;
            for (p = (u16*)&sram->counterBias; p < (u16*)((u8*)sram + SRAM_EX_OFFSET); p++) {
                sram->checkSum += *p;
                sram->checkSumInv += ~*p;
            }
        }
        if (offset < Scb.offset) {
            Scb.offset = offset;
        }
        if (Scb.offset <= SRAM_EX_OFFSET) {
            OSSramEx* ex = (OSSramEx*)(Scb.sram + SRAM_EX_OFFSET);

            if ((u32)(ex->statusBits & 0x7C00) == 0x5000 || (u32)(ex->statusBits & 0xC0) == 0xC0) {
                ex->statusBits = 0;
            }
        }
        Scb.sync = WriteSram(Scb.sram + Scb.offset, Scb.offset, 0x40 - Scb.offset);
        if (Scb.sync) {
            Scb.offset = 0x40;
        }
    }
    Scb.locked = FALSE;
    OSRestoreInterrupts(Scb.enabled);
    return Scb.sync;
}

/* Reads the SRAM and repairs an invalid status word. */
void __OSInitSram(void)
{
    OSSramEx* ex;
    u32 status;

    Scb.enabled = FALSE;
    Scb.locked = FALSE;
    Scb.sync = ReadSram(Scb.sram);
    Scb.offset = 0x40;

    ex = (OSSramEx*)LockSram(SRAM_EX_OFFSET);
    status = ex->statusBits;
    UnlockSram(FALSE, SRAM_EX_OFFSET);
    if ((u32)(status & 0x7C00) == 0x5000 || (u32)(status & 0xC0) == 0xC0) {
        status = 0;
    }
    ex = (OSSramEx*)LockSram(SRAM_EX_OFFSET);
    if ((u16)status == ex->statusBits) {
        UnlockSram(FALSE, SRAM_EX_OFFSET);
        return;
    }
    ex->statusBits = status;
    UnlockSram(TRUE, SRAM_EX_OFFSET);
}

/* Returns the result of the last SRAM write-back. */
BOOL __OSSyncSram(void)
{
    return Scb.sync;
}

/* Reads `length` bytes at `offset` of the boot ROM into `buffer` through the EXI bus. */
/* untyped: the byte range of a transfer */
BOOL __OSReadROM(void* buffer, s32 length, s32 offset)
{
    BOOL err;
    u32 cmd;

    DCInvalidateRange(buffer, length);
    if (!EXILock(EXI_CHAN_0, EXI_DEV_INT, NULL)) {
        return FALSE;
    }
    if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, EXI_FREQ_8MHZ)) {
        EXIUnlock(EXI_CHAN_0);
        return FALSE;
    }
    cmd = offset << 6;
    err = FALSE;
    err |= !EXIImm(EXI_CHAN_0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIDma(EXI_CHAN_0, buffer, length, EXI_READ, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIDeselect(EXI_CHAN_0);
    EXIUnlock(EXI_CHAN_0);
    return !err;
}

/* Returns the wireless pad ID stored for `channel`. */
u16 OSGetWirelessID(s32 channel)
{
    OSSramEx* ex = (OSSramEx*)LockSram(SRAM_EX_OFFSET);
    u16 id = ex->wirelessPadID[channel];

    UnlockSram(FALSE, SRAM_EX_OFFSET);
    return id;
}

/* Stores the wireless pad ID for `channel`, writing the SRAM back when it changed. */
void OSSetWirelessID(s32 channel, u16 id)
{
    OSSramEx* ex = (OSSramEx*)LockSram(SRAM_EX_OFFSET);

    if (id != ex->wirelessPadID[channel]) {
        ex->wirelessPadID[channel] = id;
        UnlockSram(TRUE, SRAM_EX_OFFSET);
        return;
    }
    UnlockSram(FALSE, SRAM_EX_OFFSET);
}

/* Reads the RTC flag register. */
BOOL __OSGetRTCFlags(u32* flags)
{
    BOOL err;
    u32 cmd;

    if (!EXILock(EXI_CHAN_0, EXI_DEV_INT, NULL)) {
        return FALSE;
    }
    if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, EXI_FREQ_8MHZ)) {
        EXIUnlock(EXI_CHAN_0);
        return FALSE;
    }
    cmd = 0x21000800;
    err = FALSE;
    err |= !EXIImm(EXI_CHAN_0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIImm(EXI_CHAN_0, &cmd, 4, EXI_READ, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIDeselect(EXI_CHAN_0);
    EXIUnlock(EXI_CHAN_0);
    *flags = cmd;
    return !err;
}

/* Clears the RTC flag register. */
BOOL __OSClearRTCFlags(void)
{
    BOOL err;
    u32 cmd;
    u32 data;

    data = 0;
    if (!EXILock(EXI_CHAN_0, EXI_DEV_INT, NULL)) {
        return FALSE;
    }
    if (!EXISelect(EXI_CHAN_0, EXI_DEV_INT, EXI_FREQ_8MHZ)) {
        EXIUnlock(EXI_CHAN_0);
        return FALSE;
    }
    cmd = 0xA1000800;
    err = FALSE;
    err |= !EXIImm(EXI_CHAN_0, &cmd, 4, EXI_WRITE, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIImm(EXI_CHAN_0, &data, 4, EXI_WRITE, NULL);
    err |= !EXISync(EXI_CHAN_0);
    err |= !EXIDeselect(EXI_CHAN_0);
    EXIUnlock(EXI_CHAN_0);
    return !err;
}
