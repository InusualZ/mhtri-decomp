/*
 * EXI/ProbeBarnacle.c - the EXI barnacle probe and register writer (ProbeBarnacle, __OSEnableBarnacle, EXIWriteReg).
 *
 * RANGE. `.text` 0x804B17D0-0x804B1CA0 (3 functions / 0x4C0 B); `.sbss` 0x807951A0-0x807951B0.
 *   - the dump names the three functions ProbeBarnacle / __OSEnableBarnacle / EXIWriteReg; they call the EXI bus
 *     driver above them (EXILock, EXISelect, EXIImm, EXISync, EXIDeselect, EXIGetID)
 *   - `.sbss` 0x807951A0..0x807951B0 holds the four barnacle statics read by `__OSEnableBarnacle`; the next
 *     unit (`FS/fs.c`) starts at `ISFS_OpenLib`, whose data begins at 0x807951B0
 * FLAGS. the `OS` lib block of `configure.py` (`Wii/1.3`) with `cflags_base` (-O4,p, default alignment 16): every
 *   function start of the EXI band is 16-aligned; `-func_align 4` loses the layout and the pragma is redundant
 *   (byte-identical .text/.data/.sdata/.sdata2 with and without it).
 * NAMES. the four `.sbss` words are GUESSes from their use (`__OSEnableBarnacle` is the only writer, no reader in the DOL).
 * RESIDUALS. the three functions are byte-identical; `.text` is 0x4C8 of the 0x4D0 claimed (the 8-byte pad after
 *   `EXIWriteReg` is not emitted: flipcheck blocker).
 */

#include "types.h"
#include "EXI/ProbeBarnacle.h"

/* The statics the barnacle records its result in (`.sbss` 0x807951A0..0x807951B0, laid out in reverse definition
 * order): the channel/device it was last enabled on and the two `0xA5FF005A` flags. */
u32 __OSBarnacleChan;
u32 __OSBarnacleDev;
u32 __OSBarnacleMagicB;
u32 __OSBarnacleMagicA;

/* The device IDs the barnacle probe recognises (the SDK's EXIDeviceID). */
typedef enum {
    EXI_ID_MEMCARD_59 = 0x00000004,
    EXI_ID_MEMCARD_123 = 0x00000008,
    EXI_ID_MEMCARD_251 = 0x00000010,
    EXI_ID_MEMCARD_507 = 0x00000020,
    EXI_ID_USB_ADAPTER = 0x01010000,
    EXI_ID_BROADBAND_ADAPTER = 0x04020200,
    EXI_ID_INVALID = 0xFFFFFFFF
} EXIDeviceID;

/* Probe a device for the barnacle protocol: attach, lock, select, send the probe command word and read the
 * device's answer.  Returns whether the device answered with a valid (not `EXI_ID_INVALID`) ID. */
BOOL ProbeBarnacle(EXIChannel chan, u32 dev, u32* id)
{
    BOOL error;
    u32 cmd;

    if (chan != EXI_CHAN_2 && dev == EXI_DEV_EXT) {
        if (!EXIAttach(chan, NULL)) {
            return FALSE;
        }
    }

    error = !EXILock(chan, dev, NULL);
    if (!error) {
        error = !EXISelect(chan, dev, EXI_FREQ_1MHZ);
        if (!error) {
            cmd = 0x20011300;
            error = !EXIImm(chan, &cmd, sizeof(u32), EXI_WRITE, NULL);
            error |= !EXISync(chan);
            error |= !EXIImm(chan, id, sizeof(u32), EXI_READ, NULL);
            error |= !EXISync(chan);
            error |= !EXIDeselect(chan);
        }
        EXIUnlock(chan);
    }

    if (chan != EXI_CHAN_2 && dev == EXI_DEV_EXT) {
        EXIDetach(chan);
    }

    if (error) {
        return FALSE;
    }

    return *id != EXI_ID_INVALID;
}

/* Enable the barnacle on a channel/device pair the SDK already knows is present, unless the device is one
 * of the IDs that does not answer the probe protocol. */
void __OSEnableBarnacle(EXIChannel chan, u32 dev)
{
    u32 id;

    if (!EXIGetID(chan, dev, &id)) {
        return;
    }

    switch (id) {
    case EXI_ID_MEMCARD_59:
    case EXI_ID_MEMCARD_123:
    case EXI_ID_MEMCARD_251:
    case EXI_ID_MEMCARD_507:
    case EXI_ID_USB_ADAPTER:
    case 0x01020000:
    case 0x02020000:
    case 0x03010000:
    case 0x04020100:
    case EXI_ID_BROADBAND_ADAPTER:
    case 0x04020300:
    case 0x04040404:
    case 0x04060000:
    case 0x04120000:
    case 0x04130000:
    case 0x04220000:
    case 0x80000004:
    case 0x80000008:
    case 0x80000010:
    case 0x80000020:
    case EXI_ID_INVALID:
        break;
    default:
        if (ProbeBarnacle(chan, dev, &id)) {
            __OSBarnacleChan = chan;
            __OSBarnacleDev = dev;
            __OSBarnacleMagicB = 0xA5FF005A;
            __OSBarnacleMagicA = 0xA5FF005A;
        }
        break;
    }
}

/* Write a command plus a 1-, 2- or 4-byte big-endian word to a register, checking every EXI step. */
BOOL EXIWriteReg(EXIChannel chan, u32 dev, u32 cmd, const void* buf, s32 len)
{
    BOOL error = FALSE;
    u32 write_val;

    switch (len) {
    case 1:
        write_val = *(u8*)buf << 24;
        break;
    case 2:
        write_val = *(u16*)buf << 24 | (*(u16*)buf & 0xFF00) << 8;
        break;
    default:
        write_val = *(u32*)buf >> 24 & 0x000000FF | *(u32*)buf >> 8 & 0x0000FF00 |
                    *(u32*)buf << 8 & 0x00FF0000 | *(u32*)buf << 24 & 0xFF000000;
        break;
    }

    error |= !EXILock(chan, dev, NULL);
    if (error) {
        return FALSE;
    }

    error |= !EXISelect(chan, dev, EXI_FREQ_16MHZ);
    if (error) {
        EXIUnlock(chan);
        return FALSE;
    }

    error |= !EXIImm(chan, &cmd, sizeof(cmd), EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIImm(chan, &write_val, sizeof(write_val), EXI_WRITE, NULL);
    error |= !EXISync(chan);
    error |= !EXIDeselect(chan);
    error |= !EXIUnlock(chan);

    return error == FALSE;
}
