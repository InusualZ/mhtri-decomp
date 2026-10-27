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
 * BODY.  All 17 functions are reconstructed; the unit emits its whole data claim: `.data`
 * 0x80631128-0x806311C0 (the version string, the request path and the two request names), `.sdata`
 * 0x80794438 (`sVersionTag`), `.bss` 0x80766980-0x80766B00 and `.sbss` 0x80795898-0x807958B8.
 *
 * LOAD-BEARING SHAPES (each measured on its own):
 *   - the file carries `#pragma auto_inline off`: retail KEEPS the `bl NWC24IsMsgLibOpened` /
 *     `...ByTool` / `...IsMsgLibOpenBlocked` calls in the request paths, and `-inline auto` folds
 *     their `sMsgLibOpenState` reads into the callers (playbook 61); the explicit `static inline`
 *     helpers below are still inlined, as retail's are.
 *   - the user id is a `u64`: `NWC24iGetUserId` copies it with one 64-bit load/store pair (low word
 *     stored first), the generated seed is 9999999999999999 (0x002386F26FC0FFFF), and the generate
 *     step (`NWC24iGenerateUserIdBlocked`) is an inline helper whose result reaches the caller in r3.
 *     The prototype keeps `u32*` because `DWCi/dwc_auth_interface.c` passes `(u32*)&DWCi_consoleFriendCode`;
 *     a `u64*` parameter compiles to the same row.  `error = (error != 0) ? error : resumeError;` is
 *     retail's tail (an `if (error == 0)` form loses the two copies).
 *   - the request work is FOUR file statics in `.bss` (`sNwc24RequestMutex` +0x00, `sNwc24SchedulerMutex`
 *     +0x18, the 32-byte-aligned `sNwc24InBuffer` +0x40 and `sNwc24OutBuffer` +0x60), not one struct: MWCC
 *     addresses them from the section anchor, which is what gives retail's `addi r3,r31,0x0` for the
 *     offset-0 mutex and the re-materialised `addi r3,r29,0x60` before each answer read (a struct
 *     behind a `work` pointer folds both).  The one-time initialiser is the inline
 *     `NWC24iInitRequestWork` with `level` declared at its top (a block-local `level` stops it inlining).
 *   - `NULL` is the SDK's `((void*)0)` (playbook 125): the out-pointer tests are `li r0,0; cmplw`.
 *     `NWC24iRequestGenerateUserId` tests the answer with `error == 0 || error == -35 || error == -36`.
 *   - the request entry points' thread/open checks are inline helpers returning -1/-10(/-26)/0 in r3
 *     (`NWC24iCheckRequestThread`, `NWC24iCheckScriptModeThread`); the answer copy is the inline
 *     `NWC24iCopyGeneratedUserId`.
 *   - `sMsgLibOpenState` is `s32`: retail compares it SIGNED, so a `u32` gives `cmplwi` where the
 *     target has `cmpwi` in `NWC24BlockOpenMsgLib`.
 *   - data layout: `.sbss` is laid out in reverse declaration order, so the words are declared from the
 *     top of the range down; the work-pointer slot is 8 bytes (an unreferenced separate word is
 *     dead-stripped by mwld and shifts the rest of `.sbss`).  `.data` holds the four strings as arrays
 *     (as literals at the call sites they follow the arrays and land in first-use order).
 *
 * RESIDUALS.
 *   - `.data`: `Nwc24RequestPath` is declared `__attribute__((aligned(8)))`, which reproduces retail's six
 *     zero bytes after the version string; what the original declared there is not known.
 *   - `.bss` 0x80766A00-0x80766B00 (0x100 B) is addressed by nothing in the image; it is emitted as the
 *     unreferenced `sNwc24ReservedWork` (GUESS), which mwld keeps.  The map carries one row per `.bss`
 *     static (the two mutexes, the two buffers and the reserve) and `sNwc24Version` for the version string.
 */

#include "types.h"
#include "NWC24/nwc24_msg.h"
#include "NWC24/nwc24_io.h"   /* NWC24iOpenFd / NWC24iIoctl / NWC24iCloseFd / NWC24iCheckUserIdCRC */
#include "unsplit/OS.h"       /* OSRegisterVersion / OSDisableInterrupts / OSGetCurrentThread */
#include "unsplit/NWC24.h"    /* the band's unowned data (rule 2) / memset */
#include "Runtime.PPCEABI.H/memset.h"

#pragma auto_inline off

#undef NULL
#define NULL ((void*)0)

/* The head of the library's work area as the user-id read sees it: the cached 64-bit user id at +0x08. */
typedef struct NWC24UserWork {
    /* +0x00 */ u8 pad_0x00[0x08];
    /* +0x08 */ u64 userId;
} NWC24UserWork; /* size: 0x10 (approximation: only the user-id prefix is modelled) */

/* The `.sbss` slot holding the work pointer: 8 bytes in retail, of which only the pointer is addressed. */
typedef struct NWC24UserWorkSlot {
    /* +0x00 */ NWC24UserWork* work;
    /* +0x04 */ u32 unused_0x04;
} NWC24UserWorkSlot; /* size: 0x8 */

/* `.data` 0x80631128-0x806311BC: the version string, the request device's path and the two request
   names the device layer reports errors under. */
static char sNwc24Version[] = "<< RVL_SDK - NWC24 \trelease build: Jun  9 2009 11:59:51 (0x4199_60831) >>";
static char Nwc24RequestPath[] __attribute__((aligned(8))) = "/dev/net/kd/request";
static char Nwc24SetScriptModeName[] = "NWC24iSetScriptMode";
static char Nwc24GenerateUserIdName[] = "NWC24iRequestGenerateUserId";

/* `.bss` 0x80766980-0x80766B00: the request and scheduler mutexes, the 32-byte command input and
   output buffers, and 0x100 bytes nothing in the image addresses. */
static OSMutex sNwc24RequestMutex;
static OSMutex sNwc24SchedulerMutex;
static u32 sNwc24InBuffer[8] __attribute__((aligned(32)));
static u32 sNwc24OutBuffer[8] __attribute__((aligned(32)));
static u8 sNwc24ReservedWork[0x100];

/* `.sbss` 0x80795898-0x807958B4 (MWCC lays the words out in reverse declaration order), from the bottom:
   the library's work-pointer slot, the open state (0 closed, 1 opened by the game, 2 opened by a tool, 3 open blocked), the 0/1 flag
   NWC24iRegisterVersion sets around its OSRegisterVersion call, the request work's one-time
   initialisation flag (bit 0), and the scheduler pair's suspend depth and the limit both compare
   against. */
static s32 sNwc24ResumeLimit;
static s32 sNwc24SuspendCount;
static u32 sNwc24WorkInit;
static u32 sVersionRegistered;
static s32 sMsgLibOpenState;
static NWC24UserWorkSlot sNwc24UserWork;

/* Initialises the request mutexes and buffers once. */
static inline void NWC24iInitRequestWork(void)
{
    BOOL level;

    if ((sNwc24WorkInit & 1) == 0) {
        level = OSDisableInterrupts();

        if ((sNwc24WorkInit & 1) == 0) {
            OSInitMutex(&sNwc24RequestMutex);
            OSInitMutex(&sNwc24SchedulerMutex);
            memset(sNwc24InBuffer, 0, 32);
            memset(sNwc24OutBuffer, 0, 32);
            sNwc24WorkInit |= 1;
        }
        OSRestoreInterrupts(level);
    }
}

/* The library's version tag (`.sdata` 0x80794438, 8 B: the version-string pointer plus a zero word -
   the DOL's own bytes there are 0x80631128 then 0x00000000).  The literal itself is pooled into `.data`. */
typedef struct NWC24VersionTag {
    const char* version_00; /* +0x00 */
    u32 unused_04;          /* +0x04 */
} NWC24VersionTag; /* size: 0x8 */

/* NAME (a GUESS, see the header): the tag NWC24iRegisterVersion hands to OSRegisterVersion. */
static NWC24VersionTag sVersionTag = {
    sNwc24Version,
    0,
};

/* The RTC shadow's user-id slot: the game keeps the id at the top of main memory and the NWC24
   library both reads and refreshes it there (a 32-byte DCStoreRange covers the two words). */
#define NWC24_RTC_USER_ID_SHADOW 0x800031C0
#define NWC24_RTC_USER_ID64 (*(u64*)NWC24_RTC_USER_ID_SHADOW)

/* Generates a fresh user id with the message library's open blocked, and refreshes the RTC shadow's
 * copy of it.  Retail inlines it into its one caller. */
static inline s32 NWC24iGenerateUserIdBlocked(u64* userId)
{
    s32 error = NWC24BlockOpenMsgLib(1);
    s32 openError;
    u32 ticket;

    if (error < 0) {
        return error;
    }
    ticket = 0;
    if (userId == 0) {
        error = -3;
    } else {
        *userId = 9999999999999999ULL;
        error = NWC24iRequestGenerateUserId(userId, &ticket);
        NWC24_RTC_USER_ID64 = *userId;
        DCStoreRange((void*)NWC24_RTC_USER_ID_SHADOW, 32);
    }
    openError = NWC24BlockOpenMsgLib(0);
    if (error >= 0) {
        error = openError;
    }
    return error;
}

/* 0x8051D710 (0x168): the user-id read.  With the message library open it answers the cached id the
 * work structure holds; otherwise it takes the RTC shadow's copy, validates it with the CRC check
 * and, when that fails, suspends the scheduler, blocks the open, generates a fresh id, writes it
 * back through the cache flush and resumes the scheduler. */
int NWC24iGetUserId(u32* userId) {
    s32 error = 0;

    if (NWC24IsMsgLibOpened() || NWC24IsMsgLibOpenedByTool()) {
        *(u64*)userId = sNwc24UserWork.work->userId;
    } else {
        s32 check;
        s32 resumeError;

        *(u64*)userId = NWC24_RTC_USER_ID64;
        if (*(u64*)userId == 0) {
            check = -5;
        } else {
            check = NWC24iCheckUserIdCRC(*(u64*)userId);
        }
        if (check == 0) {
            return check;
        }
        check = NWC24SuspendScheduler();
        if (check < 0) {
            return check;
        }
        error = NWC24iGenerateUserIdBlocked((u64*)userId);
        if ((u32)(error + 36) <= 1) {
            error = 0;
        }
        resumeError = NWC24ResumeScheduler();
        if (resumeError < 0) {
            error = (error != 0) ? error : resumeError;
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
    s32 error;

    NWC24iInitRequestWork();
    OSLockMutex(&sNwc24SchedulerMutex);
    error = NWC24iRequestCommand1();
    if (error >= 0) {
        sNwc24SuspendCount++;
        error = error - sNwc24ResumeLimit;
    }
    OSUnlockMutex(&sNwc24SchedulerMutex);
    return error;
}

/* 0x8051DA5C (0xE8): lower the suspension depth and resume the scheduler, unless there is nothing
 * suspended and the resume limit has not been reached. */
int NWC24ResumeScheduler(void)
{
    s32 error;

    NWC24iInitRequestWork();
    OSLockMutex(&sNwc24SchedulerMutex);
    if (sNwc24ResumeLimit > 0 && sNwc24SuspendCount == 0) {
        error = 0;
    } else {
        error = NWC24iRequestCommand3();
        if (sNwc24SuspendCount > 0) {
            sNwc24SuspendCount--;
            error = error - sNwc24ResumeLimit;
        }
    }
    OSUnlockMutex(&sNwc24SchedulerMutex);
    return error;
}

/* Answers -1 off a thread, -10 while the message library is open and -26 while its open is
 * blocked, else 0. */
static inline s32 NWC24iCheckScriptModeThread(void)
{
    if (OSGetCurrentThread() == 0) {
        return -1;
    }
    if (NWC24IsMsgLibOpened() || NWC24IsMsgLibOpenedByTool()) {
        return -10;
    }
    if (NWC24iIsMsgLibOpenBlocked()) {
        return -26;
    }
    return 0;
}

/* 0x8051DB4C (0x1A0): set the library's script mode through the request device.  The mode word and
 * the device's answer travel in the work block's two 32-byte buffers. */
int NWC24iSetScriptMode(u32 mode)
{
    s32 error = 0;
    s32 fd;

    error = NWC24iCheckScriptModeThread();
    if (error < 0) {
        return error;
    }
    NWC24iInitRequestWork();
    OSLockMutex(&sNwc24RequestMutex);
    memset(sNwc24InBuffer, 0, 32);
    error = NWC24iOpenFd((u32)Nwc24SetScriptModeName, Nwc24RequestPath, &fd, 0);
    if (error >= 0) {
        sNwc24InBuffer[0] = mode;
        error = NWC24iIoctl((u32)Nwc24SetScriptModeName, fd, 34, sNwc24InBuffer, 32,
                            sNwc24OutBuffer, 32);
        if (error >= 0) {
            error = (s32)sNwc24OutBuffer[0];
        }
        {
            s32 closeError = NWC24iCloseFd((u32)Nwc24SetScriptModeName, fd);

            if (error >= 0) {
                error = closeError;
            }
        }
    }
    OSUnlockMutex(&sNwc24RequestMutex);
    return error;
}

/* Answers -1 off a thread and -10 while the message library is open, else 0. */
static inline s32 NWC24iCheckRequestThread(void)
{
    if (OSGetCurrentThread() == 0) {
        return -1;
    }
    if (NWC24IsMsgLibOpened() || NWC24IsMsgLibOpenedByTool()) {
        return -10;
    }
    return 0;
}

/* Copies the generated id (words 1-2 of the device's answer) and its ticket (word 3) out to the
 * caller's optional slots. */
static inline void NWC24iCopyGeneratedUserId(u32* answer, u64* userId, u32* ticket)
{
    if (userId != NULL) {
        *userId = *(u64*)&answer[1];
    }
    if (ticket != NULL) {
        *ticket = answer[3];
    }
}

/* 0x8051DCEC (0x1BC): ask the device to generate a user id; the id lands in the work block's
 * output buffer and is copied out with the ticket word that follows it. */
int NWC24iRequestGenerateUserId(u64* userId, u32* ticket)
{
    s32 error = 0;
    s32 fd;

    error = NWC24iCheckRequestThread();
    if (error < 0) {
        return error;
    }
    NWC24iInitRequestWork();
    OSLockMutex(&sNwc24RequestMutex);
    error = NWC24iOpenFd((u32)Nwc24GenerateUserIdName, Nwc24RequestPath, &fd, 0);
    if (error >= 0) {
        error = NWC24iIoctl((u32)Nwc24GenerateUserIdName, fd, 15, 0, 0, sNwc24OutBuffer, 32);
        if (error >= 0) {
            error = (s32)sNwc24OutBuffer[0];
            if (error == 0 || error == -35 || error == -36) {
                NWC24iCopyGeneratedUserId(sNwc24OutBuffer, userId, ticket);
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
    OSUnlockMutex(&sNwc24RequestMutex);
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
    NWC24iInitRequestWork();
    OSLockMutex(&sNwc24RequestMutex);
    error = NWC24iOpenFd(handle, Nwc24RequestPath, &fd, 0);
    if (error >= 0) {
        error = NWC24iIoctl(handle, fd, command, 0, 0, sNwc24OutBuffer, 32);
        if (error >= 0) {
            error = (s32)sNwc24OutBuffer[0];
            if ((error == -2 || error == -33) && argument != NULL) {
                *argument = sNwc24OutBuffer[1];
            }
        }
        {
            s32 closeError = NWC24iCloseFd(handle, fd);

            if (closeError < 0) {
                error = closeError;
            }
        }
    }
    OSUnlockMutex(&sNwc24RequestMutex);
    return error;
}
