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
 * NAMING.  `sAsyncIoctlSlot` (the two-word `.sbss` in-flight slot inside the unit's own `.sbss` claim)
 * and every other name this unit owns is derived from context and marked a GUESS where the image
 * carries no spelling; every generated map row was renamed through
 * `python tools/symbols/symedit.py rename` in the same change as the body that uses it.  The rows:
 *   0x8051E384 -> NWC24iSetRtcCounter        passes its own name string "NWC24iSetRtcCounter"
 *            (`.data` 0x806311D4) to its error path.
 *   0x8051E4CC -> NWC24iSynchronizeRtcCounter its only caller is `__OSInitNet` (0x804D67C4), which
 *            passes 0; the value travels on to `NWC24iSetRtcCounter`'s second argument, so the parameter
 *            is named for what it does there (a GUESS).
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
 *   0x8051E6D4 -> NWC24iPrepareShutdown      its only caller is `__OSInitNet` (0x804D6778).
 *   0x8051E794 -> NWC24iRequestShutdown      passes its own name string "NWC24iRequestShutdown"
 *            (`.data` 0x80631214) to its error path; it is the OS shutdown handler
 *            `NWC24iPrepareShutdown` registers, so its two parameters are the OS's `final`/`event`.
 *   0x804D5BB0 -> SCSetIdleMode (GUESS)      its only caller is `NWC24iPrepareShutdown`, which
 *            passes `SCIdleModeInfo.subIdle`; the body hands one byte to the SC device (ioctl command
 *            0x6002, 32-byte in/out blocks) and answers -6 when the SC was never initialised.
 *   0x804DCC60 -> SCGetCounterBias (GUESS)   its only caller is `NWC24iSynchronizeRtcCounter`:
 *            it reads the SC's U32 item 0 through `SCFindU32Item` and answers 0x0B49D800 when the
 *            configuration has none - the value the RTC counter is rebased against.
 *   The data rows inside this unit's own claims (`.sdata` 0x80794440, `.sbss` 0x807958C8-0x807958D0,
 *   `.bss` 0x80766BE0-0x80766C40, `.data` 0x806311E8-0x8063122A) are all derived from what the two
 *   shutdown functions do with them and are GUESSes: `sShutdownFd` (the request device's descriptor,
 *   initialised to -1 so the prepare step opens it), `sShutdownRetryCount` (armed at 5 by the prepare
 *   step, counted down by the handler), `sShutdownRequestPending` (set when the async request is issued,
 *   cleared when it completes) and `sShutdownIoctlResult` (the async callback's result word).
 *
 * SEAM.  `tools/splits/tudiscover.py at 0x8051D710` reports the seam between the two halves as its only
 * weak-cut cluster (0x8051E068 codegen, 0x8051E100, 0x8051E384), and the `.sdata` must-link anchor
 * `0x80794440` (span 400 B) binds NWC24iPrepareShutdown to NWC24iRequestShutdown at the band's top;
 * the split position and its window are documented in the sibling's header.
 *
 * BODY.  All twelve functions are reconstructed and seven are byte-identical: `NWC24iOpenFd`,
 * `NWC24iIsAsyncIoctlBusy`, `NWC24iCloseFd`, `NWC24iIoctl`, `NWC24iCheckUserIdCRC`,
 * `NWC24iSynchronizeRtcCounter` and `NWC24iRequestShutdown`.  The unit measures 80.69472 % fuzzy over
 * its 2044 B.  The definitions run in ADDRESS order (flip requirement), so a body written later slots
 * into its own address, not at the end.
 *
 * DATA CLAIMED (rule 12; each range is referenced ONLY by this unit's own rows - checked with
 * `tools/units/callers.py` - so it is this TU's to own): `.sdata` 0x80794440-0x80794448
 * (`sShutdownFd`), `.sbss` 0x807958B8-0x807958D8 (the RTC work flag, the async slot and the three
 * shutdown words), `.bss` 0x80766B00-0x80766C40 (the RTC work block, the OS shutdown record and the
 * async request/reply blocks) and `.data` 0x806311E8-0x8063122C (this half's three literals).  Still
 * unowned and therefore declared `extern` in `unsplit/NWC24.h`: the sibling half's device-path
 * literals (`.data` 0x80631178-0x806311E8) and the library's message work block
 * (`.bss` 0x80766980-0x80766B00) - both are read by `nwc24_msg.c`'s rows as well, so claiming them
 * The `.rodata` 0x80574E00-0x80574E10 nibble table (`sNwc24UserIdSbox`, renamed from lbl_80574E00) is claimed too: its only
 * referencer is `getUnScrambleId`, and the body now emits it.
 *
 * LOAD-BEARING SHAPES (each measured):
 *   - `NWC24iCheckUserIdCRC`: `for (i = 0; i < 43; i++) if ((id >> (53 - (i + 1))) & 1) id ^=
 *     0x635ULL << (42 - i);` over `getUnScrambleId()`'s u64 (the return type had to
 *     become `u64` - the object returns r3:r4 and the shifts lower to `__shr2u`/`__shl2i`), answering
 *     -37 when anything is left.  The `53 - (i + 1)` spelling is load-bearing: the folded `52 - i`
 *     loses the `addi r0,r29,0x1` the target carries and measures 97.08 % instead of 100 %.
 *   - `NWC24iCloseFd` / `NWC24iIoctl`: retail keeps the branch (`cmpwi`/`bge` + two `li r3`), so the
 *     bodies assign a `result` local under `if (error < 0)` and return it; the two-early-return shape
 *     the file used before was if-converted to `srwi`/`subi`/`andc` and measured 74.92 % / 81.89 %.
 *   - `NWC24iSynchronizeRtcCounter`: three shapes are each load-bearing.  (1) `SCCheckStatus` returns
 *     `u32` - retail compares `cmplwi r3,2`, and a signed declaration costs the instruction.  (2) The
 *     64-bit divide must be SIGNED to reach `__div2i` (the target's helper): the elapsed time is taken
 *     through an `s64` expression and divided by the `u32` bus-clock quarter, which promotes to `s64`.
 *     (3) The subtraction must be written as a 64-BIT one whose high half dies - `(u32)(((s64)OSGetTime()
 *     / (OS_BUS_CLOCK >> 2)) - bias)` - which is what keeps retail's `subfc r31,r31,r4` (a 32-bit
 *     spelling emits `subf`, 98.38 %) and its operand order (bias - elapsed emits `subfc` with the
 *     operands the other way round, 99.73 %).
 *   - `NWC24iRequestShutdown`: the countdown arm must NOT return inline - retail's `li r3,0` is shared
 *     by the countdown arm and the function tail, so the body is `if (count > 0) { pending = 0;
 *     count--; } else { return 1; }` inside the pending arm's if/else and the single `return 0;` sits
 *     after it.  An inline `return 0;` in the countdown arm costs the shared constant and one
 *     instruction (97.98 %).
 *   - `NWC24iPrepareShutdown`: the `if (sShutdownFd.fd < 0)` open path is a fall-through with `result`
 *     pre-loaded to 0, and the SC park loop is a `for (;;)` whose `continue` re-runs `SCCheckStatus`.
 *   - file-wide `#pragma dont_inline on`: retail's `NWC24iSetRtcCounter` keeps three `bl`s
 *     (`NWC24iOpenFd` at target `.text` +0x3DC, `NWC24iIoctl` +0x410, `NWC24iCloseFd` +0x430 - the
 *     target object's own relocations) where `-inline auto` folds all three wrappers into it; the
 *     pragma reclaims them and lifts the row 40.50 -> 92.24.  A scoped pair around
 *     `NWC24iSetRtcCounter` measures exactly the same (92.24390 / unit 39.65558), and the file-wide
 *     form was kept: the pragma is a TU-wide setting (playbook 33) and its sibling `nwc24_msg.c`
 *     carries the same line for the same reason.
 *
 * getUnScrambleId (45.35 %, semantics PROVEN): the function takes the 64-bit user id in
 *   r3:r4 (the CRC wrapper and its caller pass it straight through - both were declared `(void)` before, which
 *   is why a `bl` alone scored 100 %) and is, bit for bit: `v = ((id & 2^53-1) ^ 0x5E5E5E5E5E5E) & 2^53-1`; rotate
 *   `v` one bit right inside 53 bits; move bytes 1,5,3,4,2,0 round the 6-cycle (byte0<-1, 1<-5, 5<-3, 3<-4,
 *   4<-2, 2<-0); substitute bytes 0..5 through the 16-entry nibble table (`hi<<4 | lo`, `.rodata` 0x80574E00);
 *   rotate 10 bits left inside 53 bits; xor 0xB3B3B3B3B3B3.  Derived by emulating the target's 161 instructions
 *   over random inputs and matching a model (0 mismatches over 2000 inputs; the source transcribed likewise).
 *   Load-bearing shapes: the cycle is moves through ONE temporary and its START is load-bearing (start at byte
 *   2 = 45.35 %, the other five starts 36.7-43.8 %); per-byte replace is `(v & ~(0xFFULL << s)) | (u8)sub << s`
 *   with the nibbles indexed from a `u32` byte (`(u32)(v >> s) & 0xFF`), which is what makes MWCC insert with
 *   `rlwimi` and index with `rlwinm` as retail does; a `u8` sub value widened through `u64` costs a `srawi`.
 *   Residual: 0x290 B against 0x284; the mask/rotate chains of the byte cycle and the final 53-bit rotate are
 *   scheduled/combined differently (retail folds the final rotate into four `rlwinm`/`rlwimi` with no trailing
 *   mask; ours keeps `slwi`/`srwi`/`or`), and the register order of the early constants differs.  The original
 *   spelling of the cycle and the rotate is not decidable from the DOL; six cycle starts, two rotate spellings
 *   and two final-mask spellings were measured (grid, 24 builds).
 *
 * Other residuals:
 *   - `NWC24iSetRtcCounter`'s own residual (92.24 %, frame 0x30 both sides, all three `bl`s kept):
 *     retail's thread guard is an if-assignment into `error` that is re-tested (`li r3,-1` / `li r3,0` /
 *     `cmpwi r3,0` / `bge <body>` / `b <epilogue>`), four instructions ours does not carry because ours
 *     returns -1 straight to the epilogue; the faithful spelling of that shape -
 *     `error = OSGetCurrentThread() == 0 ? -1 : 0;` plus `if (error >= 0) { ...body... } return error;` -
 *     measured 87.55 % and was rejected (the body restructure costs more than the seam is worth).  What is
 *     left besides it is register colouring (retail keeps the disabled-interrupts level in r31 and the
 *     open/fd error in r28 where ours uses r29: `mr r31,r3` vs `mr r29,r3`, `stw r28,0x4(r6)` vs
 *     `stw r28,0xa4(r30)`) and one scheduling swap (retail materialises the in-buffer base `addi r6,r30,0xa0`
 *     before the `stw r27,0xa0(r30)`; ours after).
 *   - `NWC24iPrepareShutdown` 99.375 % (192 B both sides): the instruction multiset is identical; only
 *     the four-instruction window that builds `sShutdownInfo` is SCHEDULED differently (retail emits
 *     `lis r5,func@ha` / `lis r4,info@ha` / `addi r5,..` / `li r0,110` / `addi r3,..`, ours emits the
 *     two `lis` and the two `addi` in the other order).  Both source orders (`func` first, `priority`
 *     first) and a by-pointer spelling were measured and give the same schedule, so the residual is the
 *     list scheduler's, not the source's.
 *   - `NWC24iIoctlAsync` 91.30 % and `NWC24iAsyncIoctlCallback` 71.43 %: both are instruction-complete
 *     and differ only in the placement of ONE `li r3,0` - retail materialises the return value before
 *     the final `stw`, ours after.  Three spellings each (`if (error < 0) return -0x2A;` then the store
 *     then `return 0`, an if/else over a `result` local, and `result = 0` before/after the store) all
 *     measure identically, so the residual is the scheduler's here too.
 *
 * DATA residual (measured with `tools/units/datagap.py --unit NWC24/nwc24_io --mode both`): NO
 * `ours-extra` row.  `target-extra .data 68 B (ours 66 B)` is the two bytes of alignment padding
 * between this half's last literal (ends 0x8063122A) and the next `.data` object (0x80631230) - bytes
 * the source cannot emit as a definition, the same class as the sibling's own 6-byte pad.  The
 * `.rodata` and `.text` have no gap now.
 */

#include "types.h"
#include "NWC24/nwc24_io.h"   /* this unit's own API (rule 2) */
#include "NWC24/nwc24_msg.h"  /* NWC24iSetScriptMode / NWC24iRegisterVersion (rule 2) */
#include "IPC/ipcclt.h"
#include "unsplit/SC.h"       /* SCCheckStatus / SCGetIdleMode / SCGetCounterBias */
#include "OS/OSStateTM.h"    /* SCSetIdleMode (rule 2) */
#include "unsplit/NWC24.h"    /* the band's unowned work block and literals (rule 2) */
#include "unsplit/OS.h"       /* OSDisable(Interrupts) / OSRestoreInterrupts / OSInitMutex /
                                 OSGetTime / OSGetAppType / OSRegisterShutdownFunction */
#include "Runtime.PPCEABI.H/memset.h"

#pragma dont_inline on

/* The RTC work block's one-time-initialisation flag (`.sbss` 0x807958B8).  The band's `.sbss`
   reserves two words here (the map sizes the symbol at 8 B) and nothing in the unit touches +0x04. */
typedef struct NWC24RtcWorkFlag {
    /* +0x00 */ u32 init;
    /* +0x04 */ u32 unused_04;
} NWC24RtcWorkFlag; /* size: 0x8 */

static NWC24RtcWorkFlag sNwc24RtcWorkInit; /* 0x807958B8 */

/* The in-flight async command slot (`.sbss` 0x807958C0, inside the unit's own `.sbss` range).  The
   map sizes the object at 8 B with no label at +0x4, so it is a two-word slot; the band's code only
   ever touches word 0.  NAME (a GUESS, see the header): the async wrapper sets it, the completion
   callback clears it, and `NWC24iIsAsyncIoctlBusy` reads it. */
static u32 sAsyncIoctlSlot[2];

/* The shutdown half's own state (`.sbss` 0x807958C8, right after `sAsyncIoctlSlot`).  All four names
   are GUESSes - the image carries no spelling for them - derived from the two functions that use
   them: the prepare step arms the countdown at 5, the handler counts it down and clears the
   in-flight flag, and the async callback stores the device's answer into the result word. */
static s32 sShutdownRetryCount;     /* +0x00 */
static s32 sShutdownRequestPending; /* +0x04 */
static s32 sShutdownIoctlResult;    /* +0x08 */

/* The request device's descriptor slot (`.sdata` 0x80794440, 8 B): the descriptor starts at -1 so
   `NWC24iPrepareShutdown` opens the device, and `NWC24iRequestShutdown` hands the value to the async
   ioctl.  `.sdata` rather than `.bss` because of the initialiser - the DOL's own bytes there are
   0xFFFFFFFF followed by zero.  The trailing word is part of the band's own `.sdata` reservation (the
   map sizes the symbol at 8 B) and nothing in this unit reads it. */
typedef struct NWC24ShutdownFdSlot {
    /* +0x00 */ s32 fd;
    /* +0x04 */ u32 unused_04;
} NWC24ShutdownFdSlot; /* size: 0x8 */

static NWC24ShutdownFdSlot sShutdownFd = { -1, 0 };

/* The device layer's own work block (`.bss` 0x80766B00, 0xE0 B): the mutex its requests take and
   the two 32-byte buffers an ioctl travels in.  It is referenced only by `NWC24iSetRtcCounter`
   (checked with `tools/units/callers.py`), so it is this unit's to own (rule 12) - the type used to
   live in `unsplit/NWC24.h` as a foreign declaration. size: 0xE0 */
typedef struct NWC24RtcWork {
    /* +0x000 */ u8 pad_0x000[0x80];
    /* +0x080 */ OSMutex mutex;
    /* +0x098 */ u8 pad_0x098[0x08];
    /* +0x0A0 */ u32 inBuffer[8];
    /* +0x0C0 */ u32 outBuffer[8];
} NWC24RtcWork; /* size: 0xE0 */

static NWC24RtcWork sNwc24RtcWork; /* 0x80766B00 */

/* The record `NWC24iPrepareShutdown` registers with the OS (`.bss` 0x80766BE0).  The band's `.bss`
   reserves 0x20 bytes for it (the map's own gap to the next symbol at 0x80766C00), so the tail is
   modelled as padding; nothing in this unit touches +0x10..+0x1F. */
typedef struct NWC24ShutdownInfo {
    /* +0x00 */ OSShutdownFunctionInfo info;
    /* +0x10 */ u8 pad_0x10[0x10];
} NWC24ShutdownInfo; /* size: 0x20 */

/* The async request's input block (`.bss` 0x80766C00, 32 B, the ioctl's `inLen`).  Word 0 is the
   shutdown event the OS hands the handler; the rest is the device's. */
typedef struct NWC24ShutdownRequest {
    /* +0x00 */ u32 event;
    /* +0x04 */ u32 pad_0x04[7];
} NWC24ShutdownRequest; /* size: 0x20 */

static NWC24ShutdownInfo sShutdownInfo;       /* 0x80766BE0 */
static NWC24ShutdownRequest sShutdownRequest; /* 0x80766C00 */
static u32 sShutdownReply[8];                 /* 0x80766C20, the ioctl's `outLen` block */

/* The three literals this half of the library owns (`.data` 0x806311E8-0x8063122A).  `Nwc24RequestPath`
   (0x80631178) is the sibling unit's copy of the same device path - the two copies are what makes
   NWC24 two translation units (`-str reuse` merges identical literals inside one TU). */
static char Nwc24PrepareShutdownName[] = "NWC24iPrepareShutdown";
static char Nwc24RequestPath2[] = "/dev/net/kd/request";
static char Nwc24RequestShutdownName[] = "NWC24iRequestShutdown";

/* 0x8051E068 (0x98): validate the 64-bit user id - unscramble it and fold bit `52 - i` of the
 * running value back into it 42 + i positions up, 43 times; anything left over is the -37 answer. */
int NWC24iCheckUserIdCRC(u64 userId)
{
    u64 id = getUnScrambleId(userId);
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

/* The 4-bit substitution table of the user-id scramble (`.rodata` 0x80574E00). */
static const u8 sNwc24UserIdSbox[16] = {
    0x0D, 0x05, 0x09, 0x07, 0x00, 0x0F, 0x0A, 0x02, 0x0C, 0x03, 0x0E, 0x01, 0x08, 0x06, 0x0B, 0x04
};

#define NWC24_ID_MASK 0x1FFFFFFFFFFFFFULL

#define NWC24_BYTE(v, n) ((u32)((v) >> (8 * (n))) & 0xFF)
#define NWC24_PUT_BYTE(v, n, b) ((v) = ((v) & ~(0xFFULL << (8 * (n)))) | ((u64)(u8)(b) << (8 * (n))))

/* Replaces byte `n` of `v` with its substitution: each nibble goes through the table. */
#define NWC24_SUB_BYTE(v, n)                                                              \
    do {                                                                                  \
        u32 in_ = NWC24_BYTE(v, n);                                                       \
        NWC24_PUT_BYTE(v, n, (sNwc24UserIdSbox[in_ >> 4] << 4) | sNwc24UserIdSbox[in_ & 0xF]); \
    } while (0)

/* 0x8051E100 (0x284): undo the friend-code scramble of a 53-bit user id - mask, xor, rotate one bit
 * right, move six bytes round a cycle, substitute them, then rotate ten bits back and xor again. */
u64 getUnScrambleId(u64 id)
{
    u64 v = ((id & NWC24_ID_MASK) ^ 0x5E5E5E5E5E5EULL) & NWC24_ID_MASK;
    u32 first;

    v = (v >> 1) | ((v & 1) << 52);
    first = NWC24_BYTE(v, 2);
    NWC24_PUT_BYTE(v, 2, NWC24_BYTE(v, 0));
    NWC24_PUT_BYTE(v, 0, NWC24_BYTE(v, 1));
    NWC24_PUT_BYTE(v, 1, NWC24_BYTE(v, 5));
    NWC24_PUT_BYTE(v, 5, NWC24_BYTE(v, 3));
    NWC24_PUT_BYTE(v, 3, NWC24_BYTE(v, 4));
    NWC24_PUT_BYTE(v, 4, first);
    NWC24_SUB_BYTE(v, 0);
    NWC24_SUB_BYTE(v, 1);
    NWC24_SUB_BYTE(v, 2);
    NWC24_SUB_BYTE(v, 3);
    NWC24_SUB_BYTE(v, 4);
    NWC24_SUB_BYTE(v, 5);
    v = ((v << 10) | (v >> 43)) & NWC24_ID_MASK;
    return (v ^ 0xB3B3B3B3B3B3ULL) & NWC24_ID_MASK;
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
    if (sNwc24RtcWorkInit.init == 0) {
        BOOL level = OSDisableInterrupts();

        if (sNwc24RtcWorkInit.init == 0) {
            OSInitMutex(&work->mutex);
            memset(work->inBuffer, 0, 32);
            memset(work->outBuffer, 0, 32);
            sNwc24RtcWorkInit.init = 1;
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

/* 0x8051E4CC (0x94): wait for the SC to finish its configuration load, then rebase the device's RTC
 * counter from the console's own elapsed ticks - the SC's bias word minus the time since reset, in
 * bus-clock quarters - and hand the result to the RTC setter. */
int NWC24iSynchronizeRtcCounter(u32 flag)
{
    s32 error;
    u32 counter;
    u32 bias;

    for (;;) {
        u32 status = SCCheckStatus();

        if (status == 2) {
            error = -1;
            break;
        }
        if (status != 0) {
            continue;
        }
        bias = SCGetCounterBias();
        counter = (u32)(((s64)OSGetTime() / (OS_BUS_CLOCK >> 2)) - bias);
        error = 0;
        break;
    }
    if (error != 0) {
        return error;
    }
    return NWC24iSetRtcCounter(counter, flag != 0);
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

/* 0x8051E6D4 (0xC0): the one-shot bring-up of the shutdown half - register the version and the
 * shutdown handler with the OS, open the request device if it is not open yet, arm the handler's
 * retry countdown, park the SC until its configuration load settles, and put a disc-channel title
 * into script mode. */
int NWC24iPrepareShutdown(void)
{
    s32 result = 0;
    SCIdleModeInfo idleMode;

    NWC24iRegisterVersion();
    sShutdownInfo.info.func = NWC24iRequestShutdown;
    sShutdownInfo.info.priority = 110;
    OSRegisterShutdownFunction(&sShutdownInfo.info);
    if (sShutdownFd.fd < 0) {
        result = NWC24iOpenFd((u32)Nwc24PrepareShutdownName, Nwc24RequestPath2, &sShutdownFd.fd, 1);
    }
    sShutdownRetryCount = 5;
    for (;;) {
        u32 status = SCCheckStatus();

        if (status == 2) {
            break;
        }
        if (status == 1) {
            continue;
        }
        SCGetIdleMode(&idleMode);
        SCSetIdleMode(idleMode.subIdle);
        break;
    }
    if (OSGetAppType() == 0x40) {
        NWC24iSetScriptMode(1);
    }
    return result;
}

/* 0x8051E794 (0xD0): the OS shutdown handler - answers 1 once the device has been told to shut down
 * and otherwise polls the async request it issued, re-issuing it while the retry countdown lasts. */
BOOL NWC24iRequestShutdown(BOOL final, u32 event)
{
    if (final != 0) {
        return 1;
    }
    if (sShutdownRequestPending != 0) {
        if (NWC24iIsAsyncIoctlBusy() != 0) {
            return 0;
        }
        if (sShutdownIoctlResult >= 0) {
            return 1;
        }
        if (sShutdownRetryCount > 0) {
            sShutdownRequestPending = 0;
            sShutdownRetryCount--;
        } else {
            return 1;
        }
    } else {
        sShutdownRequest.event = event;
        if (NWC24iIoctlAsync((u32)Nwc24RequestShutdownName, sShutdownFd.fd, 40,
                             (u32*)&sShutdownRequest, 32, sShutdownReply, 32,
                             (u32*)&sShutdownIoctlResult) >= 0) {
            sShutdownRequestPending = 1;
        }
    }
    return 0;
}
