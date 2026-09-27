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
 * `python tools/symbols/symedit.py rename` (18 rows, `.pi/notes/net-nwc24.md`); no `rule 7 deferred`
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
 * Bodies: the small state predicates, the version registration, `NWC24BlockOpenMsgLib` and the six
 * command thunks are reconstructed and measured below.  The remainder - `NWC24iGetUserId` (the library
 * entry that reads the RTC shadow at 0x800031C0, checks the user-id CRC and generates a user id),
 * `NWC24iSetScriptMode`, `NWC24iRequestGenerateUserId` and `NWC24iRequestIoctl` (the engine every thunk
 * tail-calls) - still have forward declarations only; their pools (`.data` 0x80631178-0x806311BC,
 * `.bss` 0x80766980) are NOT claimed until their bodies emit them (playbook 23).
 */

#include "types.h"
#include "unsplit/OS.h"   /* OSRegisterVersion / OSDisable(Interrupts) / OSRestoreInterrupts (rule 2 band) */

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

/* owned by this unit; declared here until its body lands */
int NWC24iRequestIoctl(u32 handle, u32 command, u32 argument);

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

void NWC24iRegisterVersion(void)
{
    if (sVersionRegistered == 0) {
        OSRegisterVersion(sVersionTag.version_00);
        sVersionRegistered = 1;
    }
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

int NWC24iRequestCommand6(u32 argument)
{
    return NWC24iRequestIoctl(0, 6, argument);
}

int NWC24iRequestCommand7(u32 argument)
{
    return NWC24iRequestIoctl(0, 7, argument);
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
