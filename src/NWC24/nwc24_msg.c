/*
 * NWC24/nwc24_msg.c - the WiiConnect24 (NWC24) SDK library's message-library translation unit:
 * .text 0x8051D710-0x8051E068 (17 functions, 2392 B).  Its sibling `NWC24/nwc24_io.c`
 * (0x8051E068-0x8051E864) is the library's device/utility half; the seam between the two is the one
 * internal boundary the range's own data proves (see below).
 *
 * NAME (a marked guess).  `nwc24_msg` is descriptive, not recovered: the image carries no `__FILE__`
 * string for the range - the bare source-file names in the neighbouring SDK pools are `ncdsystem.c`
 * (0x80631088), `dwc_auth_interface.c`, `NHTTP_bgnend.c`, `NHTTP_os_RVL.c` and `d_nhttp.c`, none of them
 * referenced by this band - and the runtime dump (`DumpSymbols.zip`) answers only `zz_`/`fn_`
 * placeholders for every row of 0x8051D710-0x8051E864.  The unit is named for what it is: it owns the
 * whole message-library state API (`NWC24IsMsgLibOpened`, `NWC24IsMsgLibOpenedByTool`,
 * `NWC24BlockOpenMsgLib`), the scheduler pair, the script-mode/user-id requests, the version
 * registration and the `/dev/net/kd/request` command engine - the "MsgLib" half of the library, the
 * SDK's own token in all three public MsgLib names.  The original file name is not recoverable from
 * this image.  The code is C (`IOS_*`/`OS*` callees only, nothing mangled) with `-Cpp_exceptions off`,
 * so `.text` is the only code section claimed (no extab/extabindex for the band).
 *
 * NAMING.  Every generated name this unit owns was renamed through
 * `python tools/symbols/symedit.py rename` (18 rows, `.pi/notes/net-nwc24.md`); no naming
 * escape is needed, because the unit now defines and references only real names.  The three data
 * labels inside the unit's own `.sbss`/`.sdata` claims were renamed the same way, each a GUESS (the
 * image carries no spelling for them): 0x807958A0 `0x807958A0` -> `sMsgLibOpenState` (the four
 * values the public predicates compare: 0 closed / 1 opened by the game / 2 opened by a tool /
 * 3 open blocked), 0x807958A4 `0x807958A4` -> `sVersionRegistered` (the flag
 * `NWC24iRegisterVersion` sets around its one `OSRegisterVersion` call) and 0x80794438
 * `0x80794438` -> `sVersionTag` (the `NWC24VersionTag` whose string pointer is registered).
 * The evidence, per row:
 *   0x8051D710 -> NWC24iGetUserId           reads the 8-byte user id (work area, else the
 *            RTC shadow at 0x800031C0), CRC-checks it with NWC24iCheckUserIdCRC and generates + stores a
 *            fresh one; the public SDK spelling of the same read is `NWC24GetUserId` (marked guess).
 *   0x8051D878 -> NWC24iRegisterVersion      passes `.sdata` 0x80794438 (the pointer to the
 *            NWC24 version string) to OSRegisterVersion, once, behind the 0x807958A4 flag.
 *   0x8051D8D8 -> NWC24iIsMsgLibOpenBlocked  returns state == 3; NWC24BlockOpenMsgLib sets 3
 *            to block the open and callers treat a non-zero answer as the -0x1A error (marked guess:
 *            the runtime dump carries `nandIsInitialized` at this address, a Ghidra function-ID match
 *            for the `subi/cntlzw/srwi` getter shape that contradicts the state machine, so it was not
 *            taken).
 *   0x8051DB4C -> NWC24iSetScriptMode        passes its own name string "NWC24iSetScriptMode"
 *            (`.data` 0x8063118C) to its error path.
 *   0x8051DEA8 -> NWC24iRequestCommand6      tail-calls NWC24iRequestIoctl(0, 6, out); its
 *            only caller is 0x8051EB38 in the SO band, which retries while it answers -0x1D.
 *   0x8051DEB8 -> NWC24iRequestCommand7      tail-calls NWC24iRequestIoctl(0, 7, out); its
 *            only caller is 0x8051EF60, which maps its error through jumptable_80631310.
 *   0x8051DEC8 -> NWC24iLockSocket           tail-calls NWC24iRequestIoctl(0, 8, 0); its
 *            only caller is SOiPrepareTempRm, and command 9 is the dump-named `NWC24iUnlockSocket`
 *            called by SOiConcludeTempRm (marked guess: the lock half of that pair).
 *   0x8051DEE8 -> NWC24iRequestCommand1      tail-calls NWC24iRequestIoctl(0, 1, 0); its
 *            only caller is NWC24SuspendScheduler.
 *   0x8051DEF8 0x800B25F8 -> NWC24iRequestCommand3     tail-calls NWC24iRequestIoctl(0, 3, 0); its
 *            only caller is NWC24ResumeScheduler (the map's `loc_` stem was as generated as an `fn_`).
 *   0x8051DF08 -> NWC24iRequestIoctl         opens /dev/net/kd/request, IOS_Ioctls the
 *            caller's command with a 0x20-byte work buffer, copies one result word, closes, all under
 *            the work mutex.
 *
 * WHY A UNIT OF ITS OWN (registration evidence)
 *   - anchor: `tools/splits/tudiscover.py at 0x8051D8B0` closes on `NWC24IsMsgLibOpened` alone (a leaf),
 *     and its only STRONG cut is the right edge 0x8051E864 (`.sdata` pool run jump
 *     `0x80794440 -> 0x8079444C`, the NWC24 -> SO library seam).  The left edge is weak, so the band
 *     was read from the code and the pools instead: the two functions below 0x8051D8B0 are NWC24's own -
 *     `NWC24iRegisterVersion` (0x8051D878, see the `.sdata` 0x80794438 row above) and `NWC24iGetUserId`
 *     (0x8051D710, calls the whole NWC24 public API and reads the library's work pointer `.sbss`
 *     0x80795898).  Everything below 0x8051D710 belongs to other libraries - NETMemCpy/NETMemSet (the
 *     NET band, no data refs), the NCD band's `ncdsystem.c` pool (`.sbss` 0x80795894 is referenced only
 *     by 0x8051C554-0x8051CCE0) - so the weak cut at 0x8051CDD0 is inside the NCD/REX band, not here.
 *   - the band is TWO TUs, not one: the `.data` literal "/dev/net/kd/request" is emitted TWICE
 *     (0x80631178, referenced by 0x8051DB4C/0x8051DCEC/0x8051DF08, and 0x80631200, referenced by
 *     0x8051E6D4), and `-str reuse` merges identical literals inside one TU - verified on this host with
 *     `mwcceppc.exe -str reuse`, where the same two literals in one file land as one copy in `.data`.
 *     The proven boundary is therefore in (0x8051DF08, 0x8051E6D4]; this unit takes it at the first
 *     candidate past the proven extent (0x8051E068, tudiscover's weak codegen cut), which also keeps the
 *     device-path literal's two copies on opposite sides of the seam and leaves each unit a contiguous,
 *     leak-free data run.  A future re-split may refine the position inside that window - it is the one
 *     open question in this register.
 *
 * BODY (this pass).  All 17 functions are reconstructed and measured (10 byte-identical, unit 85.71 %
 * fuzzy over the 2392 B of `.text`, 2388 B ours); the unit's own pool
 * (`.data` 0x80631128-0x80631178, `.sdata` 0x80794438, `.sbss` 0x807958A0) is claimed and emitted, and
 * everything the bodies reference outside those ranges - the work block `.bss` 0x80766980, the
 * scheduler flags `.sbss` 0x807958A8-0x807958B4, the user-work pointer `.sbss` 0x80795898 and the
 * device-path literals `.data` 0x80631178+ - belongs to no registered unit and is declared `extern`
 * in `include/unsplit/NWC24.h` (playbook 29/58: declare, never define).
 *
 * Residuals:
 *   - the four device-request entry points (`NWC24iGetUserId`, `NWC24iSetScriptMode`,
 *     `NWC24iRequestGenerateUserId`, `NWC24iRequestIoctl`) repeat the same one-time work-block
 *     initialiser verbatim, because retail's object carries four inlined copies of it (the original
 *     source used a macro or a textually repeated block; a helper function would emit a `bl`).
 *   - `datagap.py` reports `target-extra .data 80 B (ours 74 B)`: the claimed range
 *     0x80631128-0x80631178 is the version string (74 B) plus the 6 bytes dtk's range boundary rounds
 *     it out to, i.e. `.data` alignment padding the source cannot emit as a definition (the bytes
 *     themselves are identical).  `.text` is 4 B short of the claim.
 *   - the user-id / error constants in `NWC24iGetUserId` (0x002386F2 / 0x6FC0FFFF) are transcribed as
 *     the raw immediate pairs the object carries; their meaning is not recoverable from this image.
 */

#include "types.h"
#include "NWC24/nwc24_msg.h"
#include "NWC24/nwc24_io.h"   /* NWC24iOpenFd / NWC24iIoctl / NWC24iCloseFd / NWC24iCheckUserIdCRC */
#include "unsplit/OS.h"       /* OSRegisterVersion / OSDisableInterrupts / OSGetCurrentThread */
#include "unsplit/NWC24.h"    /* the band's unowned data (rule 2) / memset */
#include "Runtime.PPCEABI.H/memset.h"

/* The library's state (`.sbss` 0x807958A0/0x807958A4, inside the unit's own `.sbss` range).  Both
   names are GUESSes - the original spellings are not in the image - and are derived from what the
   API does with them: the open state's four values (0 closed, 1 opened by the game, 2 opened by a
   tool, 3 open blocked) are exactly the four the public predicates compare against, and the second
   word is the 0/1 flag NWC24iRegisterVersion sets around its one OSRegisterVersion call. */
static u32 sMsgLibOpenState;   /* 0 closed, 1 opened by the game, 2 opened by a tool, 3 open blocked */
static u32 sVersionRegistered; /* NWC24 version-registration flag */

/* The library's version tag (`.sdata` 0x80794438, 8 B: the version-string pointer plus a zero word -
   the DOL's own bytes there are 0x80631128 then 0x00000000).  The literal itself is pooled into `.data`. */
typedef struct NWC24VersionTag {
    const char* version_00; /* +0x00 */
    u32 unused_04;          /* +0x04 */
} NWC24VersionTag; /* size: 0x8 */

/* NAME (a GUESS, see the header): the tag NWC24iRegisterVersion hands to OSRegisterVersion. */
static NWC24VersionTag sVersionTag = {
    "<< RVL_SDK - NWC24 \trelease build: Jun  9 2009 11:59:51 (0x4199_60831) >>",
    0,
};

/* The RTC shadow's user-id slot: the game keeps the id at the top of main memory and the NWC24
   library both reads and refreshes it there (a 32-byte DCStoreRange covers the two words). */
#define NWC24_RTC_USER_ID_SHADOW 0x800031C0

/* 0x8051D710 (0x168): the user-id read.  With the message library open it answers the cached id the
 * work structure holds; otherwise it takes the RTC shadow's copy, validates it with the CRC check
 * and, when that fails, suspends the scheduler, blocks the open, generates a fresh id, writes it
 * back through the cache flush and resumes the scheduler. */
int NWC24iGetUserId(u32* userId) {
    s32 error = 0;

    if (NWC24IsMsgLibOpened() || NWC24IsMsgLibOpenedByTool()) {
        u32* work = sNwc24UserWork;

        userId[0] = work[2];
        userId[1] = work[3];
        return error;
    }
    userId[0] = NWC24_RTC_USER_ID;
    userId[1] = NWC24_RTC_USER_ID_HI;
    if ((userId[0] | userId[1]) == 0) {
        error = -5;
    } else if (NWC24iCheckUserIdCRC() == 0) {
        return 0;
    }
    error = NWC24SuspendScheduler();
    if (error < 0) {
        return error;
    }
    error = NWC24BlockOpenMsgLib(1);
    if (error >= 0) {
        u32 ticket = 0;

        if (userId == 0) {
            error = -3;
        } else {
            userId[0] = 0x002386F2;
            userId[1] = 0x6FC0FFFF;
            error = NWC24iRequestGenerateUserId(userId, &ticket);
            NWC24_RTC_USER_ID = userId[0];
            NWC24_RTC_USER_ID_HI = userId[1];
            DCStoreRange((void*)NWC24_RTC_USER_ID_SHADOW, 32);
        }
        {
            s32 openError = NWC24BlockOpenMsgLib(0);

            if (error >= 0) {
                error = openError;
            }
        }
    }
    if ((u32)(error + 36) <= 1) {
        error = 0;
    }
    {
        s32 resumeError = NWC24ResumeScheduler();

        if (resumeError < 0 && error == 0) {
            error = resumeError;
        }
    }
    return error;
}

/* 0x8051D878 (0x3C): register the library's version string once. */
void NWC24iRegisterVersion(void)
{
    if (sVersionRegistered == 0) {
        OSRegisterVersion(sVersionTag.version_00);
        sVersionRegistered = 1;
    }
}

int NWC24IsMsgLibOpened(void)
{
    return sMsgLibOpenState == 1;
}

int NWC24IsMsgLibOpenedByTool(void)
{
    return sMsgLibOpenState == 2;
}

int NWC24iIsMsgLibOpenBlocked(void)
{
    return sMsgLibOpenState == 3;
}

int NWC24BlockOpenMsgLib(int block)
{
    int error = 0;
    BOOL wasBlocked;
    BOOL level;

    level = OSDisableInterrupts();
    if (block) {
        if (sMsgLibOpenState == 0) {
            sMsgLibOpenState = 3;
        } else if (sMsgLibOpenState == 1) {
            error = -10;
        } else {
            error = -26;
        }
    } else {
        wasBlocked = sMsgLibOpenState == 3;
        if (wasBlocked) {
            sMsgLibOpenState = 0;
        }
        if (!wasBlocked) {
            error = -9;
        }
    }
    OSRestoreInterrupts(level);
    return error;
}

/* 0x8051D98C (0xF0): raise the suspension depth and ask the device to suspend the scheduler. */
int NWC24SuspendScheduler(void)
{
    NWC24RequestWork* work = &sNwc24Work;
    s32 error;

    if ((sNwc24WorkInit & 1) == 0) {
        BOOL level = OSDisableInterrupts();

        if ((sNwc24WorkInit & 1) == 0) {
            OSInitMutex(&work->mutex_0x00);
            OSInitMutex(&work->mutex_0x18);
            memset(work->inBuffer, 0, 32);
            memset(work->outBuffer, 0, 32);
            sNwc24WorkInit |= 1;
        }
        OSRestoreInterrupts(level);
    }
    OSLockMutex(&work->mutex_0x18);
    error = NWC24iRequestCommand1();
    if (error >= 0) {
        sNwc24SuspendCount++;
        error = error - sNwc24ResumeLimit;
    }
    OSUnlockMutex(&work->mutex_0x18);
    return error;
}

/* 0x8051DA5C (0xE8): lower the suspension depth and resume the scheduler, unless there is nothing
 * suspended and the resume limit has not been reached. */
int NWC24ResumeScheduler(void)
{
    NWC24RequestWork* work = &sNwc24Work;
    s32 error;

    if ((sNwc24WorkInit & 1) == 0) {
        BOOL level = OSDisableInterrupts();

        if ((sNwc24WorkInit & 1) == 0) {
            OSInitMutex(&work->mutex_0x00);
            OSInitMutex(&work->mutex_0x18);
            memset(work->inBuffer, 0, 32);
            memset(work->outBuffer, 0, 32);
            sNwc24WorkInit |= 1;
        }
        OSRestoreInterrupts(level);
    }
    OSLockMutex(&work->mutex_0x18);
    if (sNwc24ResumeLimit > 0 && sNwc24SuspendCount == 0) {
        error = 0;
    } else {
        error = NWC24iRequestCommand3();
        if (sNwc24SuspendCount > 0) {
            sNwc24SuspendCount--;
            error = error - sNwc24ResumeLimit;
        }
    }
    OSUnlockMutex(&work->mutex_0x18);
    return error;
}

/* 0x8051DB4C (0x1A0): set the library's script mode through the request device.  The mode word and
 * the device's answer travel in the work block's two 32-byte buffers. */
int NWC24iSetScriptMode(u32 mode)
{
    s32 error = 0;
    s32 fd;

    if (OSGetCurrentThread() == 0) {
        error = -1;
    } else if (NWC24IsMsgLibOpened() || NWC24IsMsgLibOpenedByTool()) {
        error = -10;
    } else if (NWC24iIsMsgLibOpenBlocked()) {
        error = -26;
    }
    if (error < 0) {
        return error;
    }
    if ((sNwc24WorkInit & 1) == 0) {
        BOOL level = OSDisableInterrupts();

        if ((sNwc24WorkInit & 1) == 0) {
            OSInitMutex(&sNwc24Work.mutex_0x00);
            OSInitMutex(&sNwc24Work.mutex_0x18);
            memset(sNwc24Work.inBuffer, 0, 32);
            memset(sNwc24Work.outBuffer, 0, 32);
            sNwc24WorkInit |= 1;
        }
        OSRestoreInterrupts(level);
    }
    OSLockMutex(&sNwc24Work.mutex_0x00);
    memset(sNwc24Work.inBuffer, 0, 32);
    error = NWC24iOpenFd((u32)Nwc24SetScriptModeName, Nwc24RequestPath, &fd, 0);
    if (error >= 0) {
        sNwc24Work.inBuffer[0] = mode;
        error = NWC24iIoctl((u32)Nwc24SetScriptModeName, fd, 34, sNwc24Work.inBuffer, 32,
                            sNwc24Work.outBuffer, 32);
        if (error >= 0) {
            error = (s32)sNwc24Work.outBuffer[0];
        }
        {
            s32 closeError = NWC24iCloseFd((u32)Nwc24SetScriptModeName, fd);

            if (error >= 0) {
                error = closeError;
            }
        }
    }
    OSUnlockMutex(&sNwc24Work.mutex_0x00);
    return error;
}

/* 0x8051DCEC (0x1BC): ask the device to generate a user id; the id lands in the work block's
 * output buffer and is copied out with the ticket word that follows it. */
int NWC24iRequestGenerateUserId(u32* userId, u32* ticket)
{
    s32 error = 0;
    s32 fd;

    if (OSGetCurrentThread() == 0) {
        error = -1;
    } else if (NWC24IsMsgLibOpened() || NWC24IsMsgLibOpenedByTool()) {
        error = -10;
    }
    if (error < 0) {
        return error;
    }
    if ((sNwc24WorkInit & 1) == 0) {
        BOOL level = OSDisableInterrupts();

        if ((sNwc24WorkInit & 1) == 0) {
            OSInitMutex(&sNwc24Work.mutex_0x00);
            OSInitMutex(&sNwc24Work.mutex_0x18);
            memset(sNwc24Work.inBuffer, 0, 32);
            memset(sNwc24Work.outBuffer, 0, 32);
            sNwc24WorkInit |= 1;
        }
        OSRestoreInterrupts(level);
    }
    OSLockMutex(&sNwc24Work.mutex_0x00);
    error = NWC24iOpenFd((u32)Nwc24GenerateUserIdName, Nwc24RequestPath, &fd, 0);
    if (error >= 0) {
        error = NWC24iIoctl((u32)Nwc24GenerateUserIdName, fd, 15, 0, 0, sNwc24Work.outBuffer, 32);
        if (error >= 0) {
            error = (s32)sNwc24Work.outBuffer[0];
            if (error == 0 || error == -35 || error == -36) {
                if (userId != 0) {
                    userId[0] = sNwc24Work.outBuffer[1];
                    userId[1] = sNwc24Work.outBuffer[2];
                }
                if (ticket != 0) {
                    *ticket = sNwc24Work.outBuffer[3];
                }
            }
        }
        {
            s32 closeError =
                NWC24iCloseFd((u32)Nwc24GenerateUserIdName, fd);

            if (error >= 0) {
                error = closeError;
            }
        }
    }
    OSUnlockMutex(&sNwc24Work.mutex_0x00);
    return error;
}

int NWC24iRequestCommand6(u32* out)
{
    return NWC24iRequestIoctl(0, 6, out);
}

int NWC24iRequestCommand7(u32* out)
{
    return NWC24iRequestIoctl(0, 7, out);
}

int NWC24iLockSocket(void)
{
    return NWC24iRequestIoctl(0, 8, 0);
}

int NWC24iUnlockSocket(void)
{
    return NWC24iRequestIoctl(0, 9, 0);
}

int NWC24iRequestCommand1(void)
{
    return NWC24iRequestIoctl(0, 1, 0);
}

int NWC24iRequestCommand3(void)
{
    return NWC24iRequestIoctl(0, 3, 0);
}

/* 0x8051DF08 (0x160): the command engine every thunk tail-calls.  It opens the request device under
 * the work mutex, sends the caller's command with no input, and hands back the device's answer -
 * plus, for the two "retry" answers (-2 and -33), the word that follows it. */
int NWC24iRequestIoctl(u32 handle, u32 command, u32* argument)
{
    s32 error;
    s32 fd;

    if (OSGetCurrentThread() == 0) {
        return -1;
    }
    if ((sNwc24WorkInit & 1) == 0) {
        BOOL level = OSDisableInterrupts();

        if ((sNwc24WorkInit & 1) == 0) {
            OSInitMutex(&sNwc24Work.mutex_0x00);
            OSInitMutex(&sNwc24Work.mutex_0x18);
            memset(sNwc24Work.inBuffer, 0, 32);
            memset(sNwc24Work.outBuffer, 0, 32);
            sNwc24WorkInit |= 1;
        }
        OSRestoreInterrupts(level);
    }
    OSLockMutex(&sNwc24Work.mutex_0x00);
    error = NWC24iOpenFd(handle, Nwc24RequestPath, &fd, 0);
    if (error >= 0) {
        error = NWC24iIoctl(handle, fd, command, 0, 0, sNwc24Work.outBuffer, 32);
        if (error >= 0) {
            error = (s32)sNwc24Work.outBuffer[0];
            if ((error == -2 || error == -33) && argument != 0) {
                *argument = sNwc24Work.outBuffer[1];
            }
        }
        {
            s32 closeError = NWC24iCloseFd(handle, fd);

            if (closeError < 0) {
                error = closeError;
            }
        }
    }
    OSUnlockMutex(&sNwc24Work.mutex_0x00);
    return error;
}
