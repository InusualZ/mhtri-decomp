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
 * BODY (third pass 2026-09-28).  Eight of the twelve functions are reconstructed and FIVE are
 * byte-identical: `NWC24iOpenFd`, `NWC24iIsAsyncIoctlBusy`, `NWC24iCloseFd`, `NWC24iIoctl` and
 * `NWC24iCheckUserIdCRC` (the last three added by the second pass, the file-wide `#pragma dont_inline
 * on` by the third).  The unit measures 39.66 % fuzzy over its 2044 B (31.35 % without the pragma,
 * 23.23 % at registration; 128 -> 404 B matched).  The definitions run in ADDRESS order (flip
 * requirement), so a body written later slots into its own address, not at the end.  `.sbss` 0x807958C0
 * is claimed and emitted; the device-path literals (`.data` 0x806311C0-0x8063122A) and the work block
 * (`.bss` 0x80766B00) belong to no registered unit and are declared `extern` in
 * `include/unsplit/NWC24.h` (playbook 29/58).
 *
 * LOAD-BEARING SHAPES (second pass, each measured):
 *   - `NWC24iCheckUserIdCRC`: `for (i = 0; i < 43; i++) if ((id >> (53 - (i + 1))) & 1) id ^=
 *     0x635ULL << (42 - i);` over `getUnScrambleId()`'s u64 (the declaration's return type had to
 *     become `u64` - the object returns r3:r4 and the shifts lower to `__shr2u`/`__shl2i`), answering
 *     -37 when anything is left.  The `53 - (i + 1)` spelling is load-bearing: the folded `52 - i`
 *     loses the `addi r0,r29,0x1` the target carries and measures 97.08 % instead of 100 %.
 *   - `NWC24iCloseFd` / `NWC24iIoctl`: retail keeps the branch (`cmpwi`/`bge` + two `li r3`), so the
 *     bodies assign a `result` local under `if (error < 0)` and return it; the two-early-return shape
 *     the file used before was if-converted to `srwi`/`subi`/`andc` and measured 74.92 % / 81.89 %.
 *   - `NWC24iGetUserId`'s siblings in `nwc24_msg.c` also drive `NWC24iCheckUserIdCRC`, so the two
 *     files are one dependency pair of this lib.
 *   - file-wide `#pragma dont_inline on`: retail's `NWC24iSetRtcCounter` keeps three `bl`s
 *     (`NWC24iOpenFd` at target `.text` +0x3DC, `NWC24iIoctl` +0x410, `NWC24iCloseFd` +0x430 - the
 *     target object's own relocations) where `-inline auto` folds all three wrappers into it; the
 *     pragma reclaims them and lifts the row 40.50 -> 92.24.  A scoped pair around
 *     `NWC24iSetRtcCounter` measures exactly the same (92.24390 / unit 39.65558), and the file-wide
 *     form was kept: the pragma is a TU-wide setting (playbook 33) and its sibling `nwc24_msg.c`
 *     carries the same line for the same reason.
 *
 * NOT reconstructed, with the reason:
 *   - `getUnScrambleId` (0x8051E100, 644 B): the 64-bit obfuscation transform `NWC24iCheckUserIdCRC`
 *     runs on.  It is 416 B of `rlwimi`/`rotlwi`/`clrlslwi`/`extrwi` over four `lbzx` S-box lookups
 *     into `.rodata` 0x80574E00 - a hand-scheduled bit permutation with no loop and no source shape to
 *     anchor on, and no reference implementation in the image, so writing it blind would bake a wrong
 *     body into the unit.  Recoverable only from the DWC/NWC24 SDK source.
 *   - `NWC24iPrepareShutdown` / `NWC24iRequestShutdown`: they build an `OSShutdownFunctionInfo` in
 *     `.bss` 0x80766BE0 and drive the async ioctl slot plus `SCCheckStatus`/`SCGetIdleMode`/
 *     `OSGetAppType`/`OSRegisterShutdownFunction`; the shutdown-info layout and the SC entry points
 *     have no owner in this tree yet.
 *   - `NWC24iSynchronizeRtcCounter`: needs `SCCheckStatus`, `OSGetTime`, `__div2i` and the
 *     `fn_804DCC60` tick source, whose 64-bit return convention the object shows but whose owner is
 *     outside this band.
 * `NWC24iSetRtcCounter`'s own residual (92.24 %, frame 0x30 both sides, all three `bl`s kept): retail's
 * thread guard is an if-assignment into `error` that is re-tested (`li r3,-1` / `li r3,0` /
 * `cmpwi r3,0` / `bge <body>` / `b <epilogue>`), four instructions ours does not carry because ours
 * returns -1 straight to the epilogue; the faithful spelling of that shape -
 * `error = OSGetCurrentThread() == 0 ? -1 : 0;` plus `if (error >= 0) { ...body... } return error;` -
 * measured 87.55 % and was rejected (the body restructure costs more than the seam is worth).  What is
 * left besides it is register colouring (retail keeps the disabled-interrupts level in r31 and the
 * open/fd error in r28 where ours uses r29: `mr r31,r3` vs `mr r29,r3`, `stw r28,0x4(r6)` vs
 * `stw r28,0xa4(r30)`) and one scheduling swap (retail materialises the in-buffer base `addi r6,r30,0xa0`
 * before the `stw r27,0xa0(r30)`; ours after).
 */

#include "types.h"
#include "NWC24/nwc24_io.h"   /* this unit's own API (rule 2) */
#include "NWC24/nwc24_msg.h"  /* NWC24iSetScriptMode / NWC24iRegisterVersion (rule 2) */
#include "unsplit/IOS.h"      /* IOS_Open / IOS_Close / IOS_Ioctl / IOS_IoctlAsync (rule 2 band) */
#include "unsplit/NWC24.h"    /* the band's unowned work blocks and literals (rule 2) */
#include "unsplit/OS.h"       /* OSDisable(Interrupts) / OSRestoreInterrupts / OSInitMutex */
#include "Runtime.PPCEABI.H/memset.h"

#pragma dont_inline on

/* The in-flight async command slot (`.sbss` 0x807958C0, inside the unit's own `.sbss` range).  The
   map sizes the object at 8 B with no label at +0x4, so it is a two-word slot; the band's code only
   ever touches word 0.  NAME (a GUESS, see the header): the async wrapper sets it, the completion
   callback clears it, and `NWC24iIsAsyncIoctlBusy` reads it. */
static u32 sAsyncIoctlSlot[2];

/* 0x8051E068 (0x98): validate the cached user id - unscramble it and fold bit `52 - i` of the
 * running value back into it 42 + i positions up, 43 times; anything left over is the -37 answer. */
int NWC24iCheckUserIdCRC(void)
{
    u64 id = getUnScrambleId();
    int i;

    for (i = 0; i < 43; i++) {
        if ((id >> (53 - (i + 1))) & 1) {
            id ^= 0x635ULL << (42 - i);
        }
    }
    if (id != 0) {
        return -37;
    }
    return 0;
}

/* 0x8051E384 (0x148): write the RTC counter through the time device - the value pair travels in the
 * work block's 32-byte input buffer and the device's answer comes back in the output one. */
int NWC24iSetRtcCounter(u32 value, u32 flag)
{
    NWC24RtcWork* work = &sNwc24RtcWork;
    s32 fd;
    s32 error;

    if (OSGetCurrentThread() == 0) {
        return -1;
    }
    if (sNwc24RtcWorkInit == 0) {
        BOOL level = OSDisableInterrupts();

        if (sNwc24RtcWorkInit == 0) {
            OSInitMutex(&work->mutex);
            memset(work->inBuffer, 0, 32);
            memset(work->outBuffer, 0, 32);
            sNwc24RtcWorkInit = 1;
        }
        OSRestoreInterrupts(level);
    }
    OSLockMutex(&work->mutex);
    error = NWC24iOpenFd((u32)Nwc24SetRtcName, Nwc24TimePath, &fd, 0);
    if (error >= 0) {
        work->inBuffer[0] = value;
        work->inBuffer[1] = flag;
        error = NWC24iIoctl((u32)Nwc24SetRtcName, fd, 23, work->inBuffer, 32, work->outBuffer, 32);
        if (error >= 0) {
            error = (s32)work->outBuffer[0];
        }
        {
            s32 closeError = NWC24iCloseFd((u32)Nwc24SetRtcName, fd);

            if (error >= 0) {
                error = closeError;
            }
        }
    }
    OSUnlockMutex(&work->mutex);
    return error;
}

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
    s32 result = 0;

    if (error < 0) {
        result = -0x2A;
    }
    return result;
}

int NWC24iIoctl(u32 unused, s32 fd, u32 command, u32* in, u32 inLen, u32* out, u32 outLen)
{
    s32 error = IOS_Ioctl(fd, command, in, inLen, out, outLen);
    s32 result = 0;

    if (error < 0) {
        result = -0x2A;
    }
    return result;
}

int NWC24iIoctlAsync(u32 unused, s32 fd, u32 command, u32* in, u32 inLen, u32* out,
                     u32 outLen, u32* userData)
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
