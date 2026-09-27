/*
 * NWC24/nwc24_io.c - the second translation unit of the WiiConnect24 (NWC24) SDK library the game
 * links: .text 0x8051E068-0x8051E864 (12 functions, 2044 B).  It is the library's device/utility half -
 * the `/dev/net/kd/*` fd + ioctl wrappers with the async command slot, the user-id CRC/unscramble pair,
 * the RTC pair, and the shutdown pair they drive.  Its sibling `NWC24/nwc24_msg.c`
 * (0x8051D710-0x8051E068) is the message-library half.
 *
 * NAME (a marked guess).  `nwc24_io` is descriptive, not recovered: the image carries no `__FILE__`
 * string for the range and the runtime dump answers only `zz_`/`fn_` placeholders for every row (checked
 * 0x8051D710-0x8051E864; the bare source-file names in the neighbouring SDK pools are `ncdsystem.c`,
 * `dwc_auth_interface.c`, `NHTTP_bgnend.c`, `NHTTP_os_RVL.c`, `d_nhttp.c`, none of them referenced by
 * this band).  Six of its twelve functions are the library's device layer - the wrappers every
 * `/dev/net/kd/*` user in both halves calls (NWC24iSetScriptMode, NWC24iRequestGenerateUserId,
 * NWC24iRequestIoctl, NWC24iSetRtcCounter, NWC24iPrepareShutdown) - and the seam that makes this a second
 * TU at all is the second `/dev/net/kd/request` copy at `.data` 0x80631200 (see the sibling's header for
 * the proof).  Its only real API names are the shutdown pair it ends with; the SDK's own file name is not
 * in the image.
 * The code is C (`IOS_*`/`OS*`/`SC*` callees only, nothing mangled) with `-Cpp_exceptions off`, so
 * `.text` is the only code section claimed (no extab/extabindex for the band).
 *
 * NAMING.  0x807958C0 `0x807958C0`, the two-word `.sbss` in-flight slot inside the unit's own
 * `.sbss` claim, is `sAsyncIoctlSlot` (a GUESS: the async wrapper sets it, the completion callback
 * clears it and `NWC24iIsAsyncIoctlBusy` reads it - the image carries no spelling for it).
 * Every generated name this unit owns was renamed through
 * `python tools/symbols/symedit.py rename` (18 rows for the two units, `.pi/notes/net-nwc24.md`); no
 * naming escape is needed, because the unit now defines and references only real names:
 *   0x8051E384 -> NWC24iSetRtcCounter        passes its own name string "NWC24iSetRtcCounter"
 *            (`.data` 0x806311D4) to its error path.
 *   0x8051E560 -> NWC24iOpenFd               IOS_Open with the NWC24 error map (-3 no out
 *            slot, -0x1D on -6, -0x1A on -8, -0x2A otherwise); its callers pass "/dev/net/kd/request"
 *            (0x80631178, 0x80631200) or "/dev/net/kd/time" (0x806311C0).
 *   0x8051E5D8 -> NWC24iCloseFd              IOS_Close, -0x2A on failure.
 *   0x8051E60C -> NWC24iIoctl                IOS_Ioctl, -0x2A on failure.
 *   0x8051E654 -> NWC24iIoctlAsync           IOS_IoctlAsync with NWC24iAsyncIoctlCallback as
 *            the callback; on success it sets the `.sbss` 0x807958C0 in-flight slot.
 *   0x8051E6B0 -> NWC24iIsAsyncIoctlBusy     returns that slot; its only caller is
 *            NWC24iRequestShutdown, which polls it for the async completion.
 *   0x8051E6B8 -> NWC24iAsyncIoctlCallback   the IOS_IoctlAsync callback: stores the result
 *            word to *out and clears the in-flight slot (marked guess: the slot's only writers are this
 *            callback and the async wrapper, so the name follows the pair).
 *   0x8051E794 -> NWC24iRequestShutdown      passes its own name string "NWC24iRequestShutdown"
 *            (`.data` 0x80631214) to its error path.
 *
 * SEAM.  `tools/splits/tudiscover.py at 0x8051D710` reports the seam between the two halves as its only
 * weak-cut cluster (0x8051E068 codegen, 0x8051E100, 0x8051E384), and the `.sdata` must-link anchor
 * `0x80794440` (span 400 B) binds NWC24iPrepareShutdown to NWC24iRequestShutdown at the band's top;
 * the split position and its window are documented in the sibling's header.
 *
 * Bodies written and measured: `NWC24iOpenFd`, `NWC24iCloseFd`, `NWC24iIoctl`, `NWC24iIoctlAsync`,
 * `NWC24iIsAsyncIoctlBusy`, `NWC24iAsyncIoctlCallback`.  Still forward declarations only:
 * `NWC24iCheckUserIdCRC`/`getUnScrambleId` (with the 16-byte table at `.rodata` 0x80574E00),
 * `NWC24iSetRtcCounter`/`NWC24iSynchronizeRtcCounter`, `NWC24iPrepareShutdown`/`NWC24iRequestShutdown` -
 * their pools (`.data` 0x806311C0-0x8063122A, `.bss` 0x80766B00-0x80766C40) stay unclaimed until those
 * bodies emit them (playbook 23).
 */

#include "types.h"
#include "unsplit/IOS.h"   /* IOS_Open / IOS_Close / IOS_Ioctl / IOS_IoctlAsync (rule 2 band) */

/* The in-flight async command slot (`.sbss` 0x807958C0, inside the unit's own `.sbss` range).  The
   map sizes the object at 8 B with no label at +0x4, so it is a two-word slot; the band's code only
   ever touches word 0.  NAME (a GUESS, see the header): the async wrapper sets it, the completion
   callback clears it, and `NWC24iIsAsyncIoctlBusy` reads it. */
static u32 sAsyncIoctlSlot[2];

/* owned by this unit; declared here until their bodies land */
int NWC24iSetRtcCounter(u32 value, u32 flag);
u32 getUnScrambleId(void);
int NWC24iCheckUserIdCRC(void);
int NWC24iAsyncIoctlCallback(u32 value, u32* out);

int NWC24iOpenFd(u32 unused, const char* path, s32* fd, u32 mode)
{
    s32 error;

    if (fd == 0) {
        return -3;
    }
    error = IOS_Open(path, mode);
    *fd = error;
    if (error < 0) {
        if (error == -6) {
            return -0x1D;
        }
        if (error == -8) {
            return -0x1A;
        }
        return -0x2A;
    }
    return 0;
}

int NWC24iCloseFd(u32 unused, s32 fd)
{
    s32 error = IOS_Close(fd);

    if (error >= 0) {
        return 0;
    }
    return -0x2A;
}

int NWC24iIoctl(u32 unused, s32 fd, u32 command, void* in, u32 inLen, void* out, u32 outLen)
{
    s32 error = IOS_Ioctl(fd, command, in, inLen, out, outLen);

    if (error >= 0) {
        return 0;
    }
    return -0x2A;
}

int NWC24iIoctlAsync(u32 unused, s32 fd, u32 command, void* in, u32 inLen, void* out, u32 outLen,
                     void* userData)
{
    s32 error = IOS_IoctlAsync(fd, command, in, inLen, out, outLen,
                              (void (*)(u32, u32*))NWC24iAsyncIoctlCallback, userData);

    if (error < 0) {
        return -0x2A;
    }
    sAsyncIoctlSlot[0] = 1;
    return 0;
}

u32 NWC24iIsAsyncIoctlBusy(void)
{
    return sAsyncIoctlSlot[0];
}

int NWC24iAsyncIoctlCallback(u32 value, u32* out)
{
    int result = 0;

    if (out != 0) {
        *out = value;
    }
    sAsyncIoctlSlot[0] = 0;
    return result;
}
