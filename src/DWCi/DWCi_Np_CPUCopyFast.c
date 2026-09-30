/*
 * DWCi_Np_CPUCopyFast.c - the Nintendo Wi-Fi Connection (DWCi) SDK band, `.text` 0x80507C40..0x80509DB0.
 *
 * REGISTRATION (recon lane, 2026-09-27).  One `DWCi` unit for the named function `DWCi_Np_CPUCopyFast`
 * (0x80507C40, 0x9E4 B) and its 14 following neighbours (15 functions, 8560 B).  Right edge is a
 * STRONG cut: `tudiscover.py at 0x80507C40` reports `cut 18505 0x80509DB0 strong x2`
 * (.sdata run jump 0x807942D8 -> 0x807942FC and 0x80794328 -> 0x80794330; the second admits
 * only cuts in [18505,18508], and the first pins 18505).  The left edge at 0x80507C40 is a weak cut
 * (only the .data jumptable run jump jumptable_8062FD00 -> jumptable_8062FF90, a 14-function interval)
 * - 0x80507C40 is taken because it is the named symbol's own (16-aligned) start.  The function
 * `0x805078F0` (`DWCi_report`) immediately below it ends at 0x80507C38.
 *
 * SECTIONS.  `.text` plus two of the band's data objects, claimed 2026-09-28 so that the range the
 * unit's own code stores into is the unit's (rule 12; the declarations lived in
 * `include/unsplit/DWCi.h`, which is rule 12's finding):
 *   .sbss 0x807957D0..0x807957F8 - the state machine's words: the free-list head, the runtime/result
 *     block and the state-ladder word.  Every symbol in the run is stored by *this* object
 *     (`DWCi_FreeList`, `DWCi_SetResult`, `DWCi_AdvanceStatus`) and by no other registered unit, so
 *     the definer test - a store, not a load (`DWCi_runtime` 57 loads, 2 stores; `DWCi_state` 28
 *     stores) - picks this unit.  0x807957F8 (not this unit's) is the first symbol past the run.
 *   .sdata 0x80794200..0x80794210 - the word `DWCi_authDataTask` loads and hands to its
 *     `NANDPrivateOpenAsync`/`NANDPrivateDeleteAsync` calls as the session's path
 *     (`DWCi_authDataPathPtr`, 0x80794200), then the two all-zero name strings the opener is handed
 *     (0x80794204/0x80794208) and the 3-byte "//" at 0x8079420C (`DWCi_urlSchemeSeparator`: the
 *     `strstr` needle `fn_80509270` searches a URL with, copying from the match + 2).  The four
 *     object bytes at 0x80794200 read zero only because the word is a *relocation*: the target
 *     object's single `.rela.sdata` entry is an `R_PPC_ADDR32` to `DWCi_authDataPath`, the
 *     "/shared2/DWC_AUTHDATA" path string.  All four words are read only here (still declarations -
 *     see the residual list).
 * Both boundaries are symbol-aligned and 8-byte aligned, which `dtk dol split` requires of a claim
 * (an 8-misaligned boundary makes it die with `Invalid alignment for split`).  One run per section:
 * a second run whose gap no registered unit owns makes the split die with a link-order cycle
 * (playbook 53), so nothing else here is claimed.  `.data` joined the set on 2026-09-28:
 * 0x8062FF90..0x806302C8, one contiguous run, claimed and emitted by this file (see "DATA" below).
 * Still unowned and declared in the band header: `DWCi_protocolMagic`, `DWCi_addressFormat*`, the
 * digit class table, `DWCi_addressRingIndex`, `DWCi_stateBlock` and `DWCi_workBuffer` - all read
 * here but stored elsewhere or nowhere.  `DWCi_addressRing` (0x807625C0) is *not* in that list: the word falls
 * inside the NATNEG unit's `.bss` run, so rule 2 declares it in `include/DWCi/DWCi_NatNeg.h`,
 * which this file reaches through the band header.
 *
 * FLAGS (open question, reported).  The whole 0x80507C40..0x80512490 band has every function start
 * 16-byte aligned (152/152) with 4/8/12-byte zero `gap_*` runs between neighbours, which is the
 * `-func_align 16` layout; the sibling `DWCi_sendControlFrame` unit is registered under `cflags_dwc`'s
 * `-func_align 4`.  This unit copies `cflags_dwc` (the task's instruction: a copied flag is safe, a
 * changed flag needs its own evidence); the 16-alignment is recorded for the flip pass, where it is
 * the difference between an object that links at the right address and one that does not.
 *
 * NAMING (rule 7, no exemption).  The unit owns 15 functions and twelve have bodies here.  Ten are
 * named from their own code, callers and the fields they touch (each is a GUESS - the runtime dump
 * answers `zz_0512xxx_` for the whole band, so there is no map name to recover; the unit header
 * records that):
 *   0x805091F0 -> DWCi_GetStatus       reads the first word of the DWCi state block DWCi_stateBlock
 *   0x805091D0 -> DWCi_IsStatusReady   that word == 1
 *   0x80509200 -> DWCi_GetWorkBuffer   address of the 0x190-B DWCi work buffer DWCi_workBuffer
 *   0x80509180 -> DWCi_AdvanceStatus   the 0x19 -> 0x1A state ladder over DWCi_state
 *   0x80509250 -> DWCi_SetResult       publishes flag + value into DWCi_runtime
 *   0x80508750 -> DWCi_FreeList        drains the DWCi_freeListHead list through DWCi_freeNode
 *   0x805087A0 -> DWCi_npStart         the reset a session begins with: clears the state block and
 *                                      its two trailing regions, DWCi_state / DWCi_runtime, and
 *                                      remembers the argument in DWCi_initArgument
 *   0x805089F0 -> DWCi_npSetup         opens the session: calls the host callback below with the two
 *                                      argument words, zeroes the work buffer, copies the 5-byte tag
 *   0x80509210 -> DWCi_npSetValue      issues the runtime block's command 13 with its argument
 *   0x80509230 -> DWCi_npSetValueEx    the three-argument form of that same command
 * One row is named from a call site instead of a body: 0x80508830 -> `DWCi_initRuntime`, the
 * six-argument host callback `DWCi_npSetup` invokes with the band's two empty-name defaults.
 * Criterion, stated once for both DWCi units: no auto-generated name is spelled in source, and a row
 * that no written body reaches keeps its stem - here `fn_80509270`;
 * the unit's own row `DWCi_Np_CPUCopyFast` carries the map's name and has no body yet either.
 * Rows renamed by the data pass (2026-09-28), because the source now spells them:
 * `jumptable_8062FF90` -> `DWCi_npCopyFastTailTable`, `jumptable_8063025C` ->
 * `DWCi_authDataStateTable` (the two switch tables), the run's eleven data labels -> the names of the
 * objects they start (`DWCi_authDataPath`, `DWCi_acUrlDev`, `DWCi_prUrlDev`, ...), and
 * `fn_80508A70` -> `DWCi_authDataTask`, the state machine the second table dispatches into (a GUESS
 * from its shape: a no-argument task the middle band's `DWC_NASLoginProcess`/`DWC_SVLProcess` step functions
 * call, driving the NAND auth-data I/O and the HTTP layer through a 27-way state test).
 * Rows renamed by the body pass (2026-09-28): `fn_80508630` -> `DWCi_GetConsoleFriendCode` (its body
 * below), the two `.sbss` words it stores (`lbl_807957D0` -> `DWCi_friendCodeReady`, `lbl_807957D8` ->
 * `DWCi_consoleFriendCode`), and the helpers outside this unit its two new bodies call: `fn_80507690`
 * -> `DWCi_allocNode` and `fn_805078F0` -> `DWCi_report` (both this band, declared in
 * `include/unsplit/DWCi.h`), `fn_80521CA0/D70/DE0` -> `VFipf2Init`/`VFipf2Shutdown`/
 * `VFipf2IsInitialized` (the file-system band; its new `include/unsplit/VF.h` is their home - rule 2
 * cannot place that address, the nearest registered units below and above are `NWC24` and the
 * game-UI band - and the three names are a GUESS from the scheme their own callees use, not names
 * recovered from the SDK, so a later pass may confirm them), `fn_8051C554` -> `NCDGetCurrentIfConfig`
 * (the NCD band, unregistered: declared in
 * the existing `include/unsplit/NCD.h`; the name is read off this file's own
 * "...IfConfig failed.[%d]" line, so a later pass can confirm it), and `fn_8051A4E8` ->
 * `NHTTPi_RegisterCallbacks` - a GUESS from this unit's call site, which hands the two command
 * callbacks and command 17 to the HTTP layer's bring-up.  That row lives inside `NHTTP/d_nhttp.c`'s
 * registered range, so its declaration went into that unit's header (`include/NHTTP/d_nhttp.h`) and
 * the NHTTP lane, which owns the body, may refine the name.
 * Rows renamed by the review pass (2026-09-28): the run's two remaining `.sdata` stems, both read
 * off the code that touches them - `lbl_80794200` -> `DWCi_authDataPathPtr` (the word is an
 * `R_PPC_ADDR32` to `DWCi_authDataPath`) and `lbl_8079420C` -> `DWCi_urlSchemeSeparator` (the
 * `strstr` needle) - and the two coarse `.data` rows, re-sized to the objects the source has.
 *
 * The one helper it calls that no registered unit owns, `DWCi_freeNode` (0x805076F0, the middle band
 * 0x80509DB0..0x805113B0), is a GUESS derived from this file's own call site - `DWCi_FreeList`
 * drains its list by calling it once per node with the 0xC kind tag - and is declared in the band
 * header `include/unsplit/DWCi.h`; it is a marker for a later reconstruction to confirm.
 *
 * BODIES (body pass).  Twelve of the 15 functions are written; the unit is 14.70 % over its 8464 B
 * of `.text` (was 9.51 % after the first body).  Byte-identical (100 %): `DWCi_FreeList`,
 * `DWCi_AdvanceStatus`, `DWCi_IsStatusReady`, `DWCi_GetStatus`, `DWCi_GetWorkBuffer`,
 * `DWCi_SetResult`, `DWCi_npSetValue`, `DWCi_npSetValueEx`, `DWCi_GetConsoleFriendCode`.
 *   DWCi_initRuntime 99.7818  440 B
 *   DWCi_npSetup     99.6774  124 B
 *   DWCi_npStart     95.1471  136 B
 * Two load-bearing shapes came out of this pass:
 *   - `DWCi_GetConsoleFriendCode`'s early exit must be
 *     `if (ready == 0) { ...body... } return code;` - the target has ONE return path (a single `bne`
 *     to an epilogue that loads the value).  The early-return spelling measured 91.8857 with 292 B
 *     against 280 B, because it duplicates the two loads and the branch.
 *   - `DWCi_initRuntime`'s two stored-configuration words share one `lwz r4, DWCi_runtime` in the
 *     target; two direct `DWCi_runtime->field = ...` statements do not give that (MWCC reloads the
 *     global for the second store), while hoisting the block into a `runtime` local for those two
 *     statements does: 98.1455 -> 99.7818.
 * Residuals:
 *   - `DWCi_initRuntime` 99.7818: four `addi r4, r30, +0x118/+0x130/+0x144/+0x168` where ours are
 *     `+0x1A8/+0x1C0/+0x1D4/+0x1F8` off the *section* symbol - retail's pool base is the
 *     `DWCi_authDataPath` literal, while MWCC names the section for the arrays this file defines, so
 *     the displacements differ although the addresses are the same.  The same choice colours the
 *     pair of stored-configuration words the other way round (retail r28/r27, ours r27/r28).
 *     Recorded, not chased: the hypothesis is that making those strings literals inside this body
 *     is what would move the base (hypothesis, not measured: with this unit's own command line a
 *     string literal becomes an *anonymous* `.data` object, so it is not obviously the lever that
 *     materialises the base from the *named* `DWCi_authDataPath`), and the `.data` run's emission
 *     order forbids it while its other users are unwritten.
 *   - `DWCi_npSetup` 99.6774 is one instruction off - the host callback takes six arguments, and
 *     only that arity reproduces retail's `mr r7,r4`/`mr r8,r5` argument shuffles; `DWCi_npStart`
 *     95.1471's frame is 0x10 and settles one instruction short (`memset(state, ...)` gives
 *     `mr r3,r31` where retail's `addi r3,r31,0` is the same address written as a computation).
 * Unwritten, with sizes: `DWCi_authDataTask` (0x708), `DWCi_Np_CPUCopyFast` (0x9E4) and
 * `fn_80509270` (0xB40) - the DWCi save/HTTP state machine and the two `DWCi_*_Fast` copy loops,
 * multi-wave bodies by their own size.
 *
 * DATA (data pass, 2026-09-28).  `.data` 0x8062FF90..0x806302C8 (824 B) is claimed and emitted by
 * this file; the claim's evidence is the target object's only `.rela.sdata` entry, the
 * `R_PPC_ADDR32` from the unit's own `.sdata` word 0x80794200 to the "/shared2/DWC_AUTHDATA" string
 * at 0x80630020 - pointer and string are one TU's data (the full argument, including why the
 * previously proposed end 0x806301A0 was wrong, is in the range request recorded in the campaign's
 * outbox).  The run is one contiguous range by rule (playbook 53) and both edges are 8-aligned.
 * `datagap.py --unit DWCi/DWCi_Np_CPUCopyFast` now reports the `.data` row byte-identical; `.sdata`
 * 16 B and `.sbss` 40 B stay `target-extra` and are declared here rather than defined, for two
 * different reasons:
 *   - `.sdata`: its two name strings are all-zero, and the compiler puts a zero-initialised object
 *     in `.sbss`/`.bss`, never in `.sdata` (measured on this host with the unit's own command line),
 *     so no body is an emitter for it at all;
 *   - `.sbss`: the target's run is eight rows (40 B) while the source covers 32 B of it - seven
 *     objects, in which `DWCi_friendCodeReady` (0x807957D0) and `DWCi_freeListHead` (0x807957E0) are
 *     4-byte where the target's rows are 8 bytes each - leaving the two words 0x807957D4 and
 *     0x807957E4 unaccounted for, and a partial `.sbss` is what `flipcheck.py` refuses.  The
 *     writers are not the blocker: the target's own `.rela.text` touches `DWCi_friendCodeReady`
 *     only from `DWCi_GetConsoleFriendCode` (+0x1c read, +0x60 store) and `DWCi_freeListHead` only
 *     from `DWCi_FreeList` (+0x10 read, +0x28 store), both measured bodies.  Modelling the two
 *     words is the unblock, and it is a later pass' scope.
 */

#include "types.h"
#include "Runtime.PPCEABI.H/memset.h"    /* memset: the runtime unit's header (rule 2) */
#include "NHTTP/d_nhttp.h"               /* NHTTPi_RegisterCallbacks: its owner's header (rule 2) */
#include "NWC24/nwc24_msg.h"             /* NWC24iGetUserId: the NWC24 band's own header (rule 2) */
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers, and - through it - */
                                         /* the two owners' headers for the data they now own       */
#include "unsplit/NCD.h"                 /* NCDGetCurrentIfConfig: the NCD band's header (rule 2) */
#include "unsplit/Runtime.PPCEABI.H.h"   /* strncpy / wcsncpy */
#include "unsplit/VF.h"                  /* VFipf2*: the file-system band's own header (rule 2) */

/* The two host callbacks `DWCi_initRuntime` stores in the runtime block: the allocator it is handed
 * the block by, and the command/free callback it publishes results through. */
typedef struct DWCiRuntime* (*DWCiAllocCallback)(u32 command, u32 size);
typedef void (*DWCiCommandCallback)(u32 command, u32 arg1, u32 arg2);

/* The DWCi runtime block `DWCi_runtime` (this unit's own object, declared in
 * `include/DWCi/DWCi_Np_CPUCopyFast.h`) points at.  Only the fields this unit's bodies read or
 * write are named; every other offset is an unmodelled run of the original's own layout.
 * size: 0x5B30 - the size DWCi_initRuntime asks the host allocator for, which is also where its
 * last named field ends. */
typedef struct DWCiRuntime {
    /* +0x0000 */ u8 pad_0x0000[0x4000];
    /* +0x4000 */ u8 ifConfig_0x4000[0x15E];  /* the interface configuration `NCDGetCurrentIfConfig` fills */
    /* +0x415E */ u16 playerName_0x415E[26];  /* 52 B: the wide player name `DWCi_initRuntime` copies */
    /* +0x4192 */ char friendCode_0x4192[12];
    /* +0x419E */ char tag_0x419E[5];   /* the 5-byte tag DWCi_npSetup copies; no terminator is written */
    /* +0x41A3 */ u8 pad_0x41A3[0x1815];
    /* +0x59B8 */ u32 resultFlag;       /* DWCi_SetResult publishes a pending result here */
    /* +0x59BC */ u32 resultValue;
    /* +0x59C0 */ u8 pad_0x59C0[0x08];
    /* +0x59C8 */ u32 mode_0x59C8;      /* 1 from DWCi_initRuntime, 2 while DWCi_npSetup's session is open */
    /* +0x59CC */ u8 pad_0x59CC[0x148];
    /* +0x5B14 */ DWCiAllocCallback allocCallback;
    /* +0x5B18 */ DWCiCommandCallback commandCallback; /* its second argument is the runtime block on
                                                     * DWCi_initRuntime's failure path and a command
                                                     * argument in DWCi_npSetValueEx */
    /* +0x5B1C */ u8 pad_0x5B1C[0x0C];
    /* +0x5B28 */ u32 storedHost;       /* DWCi_initRuntime's third argument (a GUESS: the pair is the */
    /* +0x5B2C */ u32 storedPort;       /*    stored configuration DWCi_useStoredConfig's reader uses) */
} DWCiRuntime; /* size: 0x5B30 */

/* A node of the DWCi list `DWCi_FreeList` drains; the link sits at +0x18. size: 0x1C */
typedef struct DWCiListNode {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ struct DWCiListNode* next;
} DWCiListNode;

/* The DWCi state block `DWCi_stateBlock` (declared `u32[]` in `include/unsplit/DWCi.h` because its
 * unsized form is what makes MWCC address it absolutely), as `DWCi_npStart` clears it: three
 * regions zeroed in place and one count word cleared.  The regions' own contents belong to the
 * band's helpers, so they stay unnamed runs. size: >= 0x380 */
/* This unit's own entry points, in address order. */
u32 DWCi_npStart(u32 arg);
u32 DWCi_npSetup(char* tag, DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback);
void DWCi_npSetValue(u32 value);
void DWCi_npSetValueEx(u32 value);
u32 DWCi_initRuntime(char* playerName, char* friendCode, u32 storedHost, u32 storedPort,
                     DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback);

/* A code address: an arm of one of this unit's own switch functions, which is what the two tables
 * below hold (the target's relocations are `R_PPC_ADDR32` with the arm's offset as the addend).  The
 * addresses are code, so they carry this type rather than a bare `u8*`. */
typedef u8* DWCiCodeAddress;

/* ------------------------------------------------------------------------------------------------
 * The unit's `.data` run, 0x8062FF90..0x806302C8 (824 B), in address order.  The offsets are
 * load-bearing: the target addresses its string pool through ONE base register plus displacements
 * (DWCi_initRuntime reaches its four report lines as +0x118/+0x130/+0x144/+0x168 off the auth-data
 * path object), so the distance from one object to the next is part of the match.  Two measured
 * layout facts are what reproduce it on this host with the unit's own command line:
 *   - an object whose size is a multiple of 8 is 8-aligned, every other object is 4-aligned
 *     (this is what puts the URL tables at 0x806300AC/0x8063012C and the 40-byte URL strings at 8);
 *   - `DWCi_authDataPath` is the one exception - 22 bytes, but the target's object sits at
 *     0x80630020 where a 4-byte rule would put it at 0x8063001C - so the array states `aligned(8)`.
 * The strings are objects rather than literals because the bodies that would emit them are not
 * reconstructed yet; every object below sits at the offset, and has the size, the target's own
 * symbol has, and `datagap.py` compares the section byte for byte.  Two of the map's rows were
 * coarse runs that swallowed later objects; they were split at the object boundaries in the same
 * change (`DWCi_acUrlDev` 0x34 -> 0x27 and `DWCi_prUrlDev` 0x158 -> 0x27, with a row added per
 * object they covered), so the statement above is true of every row now.  The split also lets
 * `flipcheck.py` name what the unwritten bodies still have to keep alive: the two tables and the
 * eleven report strings are `force-active` in the target's own `.comment` while no relocation in
 * our link references them yet, which is the flip pass' problem, not this pass'.
 * ------------------------------------------------------------------------------------------------ */

/* 0x8062FF90 (0x40, `scope:local`): the 16-entry switch table of `DWCi_Np_CPUCopyFast`'s tail -
 * entry N is the arm that copies N halfwords, the arms falling through (the target's own entries:
 * 0x384..0x474 by 0x10, descending, the address the table's last arm starts at first).  MWCC
 * emits this table from the switch itself; the body is not written yet, so it is spelled here. */
static DWCiCodeAddress DWCi_npCopyFastTailTable[16] = {
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x474, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x464,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x454, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x444,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x434, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x424,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x414, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x404,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x3F4, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x3E4,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x3D4, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x3C4,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x3B4, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x3A4,
    (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x394, (DWCiCodeAddress)DWCi_Np_CPUCopyFast + 0x384,
};

/* 0x8062FFD0 (0x24) / 0x8062FFF4 (0x25): the two lines the friend-code reporter prints, the value
 * as `%016lld` (the 64-bit pair `lbl_807957D8`/`0xDC` prints as two varargs registers). */
char DWCi_reportFriendCodeFormat[] = " get console friend code = %016lld\n";
char DWCi_reportFriendCodeFailedFormat[] = " failed to get console friend code.\n";

/* 0x80630020 (0x16): the save file's path.  `DWCi_authDataTask` hands the `.sdata` word that
 * points here to NANDPrivateOpenAsync / NANDPrivateDeleteAsync, and DWCi_initRuntime's report
 * lines are addressed as displacements off it.  `aligned(8)` is the target's layout, not
 * decoration (see the block comment above). */
__attribute__((aligned(8))) char DWCi_authDataPath[] = "/shared2/DWC_AUTHDATA";

/* 0x80630038..0x806300AC: the account-creation URL per server environment, then the three-entry
 * table the environment index selects from (test / production / development, in that order). */
char DWCi_acUrlTest[] = "https://naswii.test.nintendowifi.net/ac";
char DWCi_acUrlProd[] = "https://naswii.nintendowifi.net/ac";
char DWCi_acUrlDev[] = "https://naswii.dev.nintendowifi.net/ac";
char* DWCi_acUrlTable[3] = { DWCi_acUrlTest, DWCi_acUrlProd, DWCi_acUrlDev };

/* 0x806300B8..0x8063012C: the profile ("pr") URLs and their table, the same shape. */
char DWCi_prUrlTest[] = "https://naswii.test.nintendowifi.net/pr";
char DWCi_prUrlProd[] = "https://naswii.nintendowifi.net/pr";
char DWCi_prUrlDev[] = "https://naswii.dev.nintendowifi.net/pr";
char* DWCi_prUrlTable[3] = { DWCi_prUrlTest, DWCi_prUrlProd, DWCi_prUrlDev };

/* 0x80630138..0x8063025C: the state machine's report lines, in the order its arms reach them
 * (the four DWCi_initRuntime prints first, then the auth-data task's own). */
char DWCi_reportAuthProcessing[] = " auth is processing\n";
char DWCi_reportMemoryShortage[] = " memory shortage\n";
char DWCi_reportIfConfigQueryFailed[] = " NCDGetCurrentIfConfig failed.[%d]\n";
char DWCi_reportHttpStartFailed[] = " failed to start NHTTP\n";
char DWCi_reportHttpDestroy[] = "NHTTPDestroyResponse()\n";
char DWCi_reportUserIdRead[] = " read userid = %llu\n";
char DWCi_reportUserIdReadSize[] = " illegal size userid read = %d\n";
char DWCi_reportUserIdDelete[] = " delete illegal userid.\n";
char DWCi_reportAccountCreateTimeout[] = " acctcreate timeout.\n";
char DWCi_reportUserIdWriteSize[] = " illegal size userid write = %d\n";
char DWCi_reportLoginTimeout[] = " login timeout.\n";

/* 0x8063025C (0x6C, `scope:local`): the 27-entry dispatch table of `DWCi_authDataTask`'s state
 * test -- entry N is the address that state's arm starts at (the target's own entries; the last
 * three all fall into the state machine's terminator).  Spelled here for the same reason as the
 * table at the head of the run. */
static DWCiCodeAddress DWCi_authDataStateTable[27] = {
    (DWCiCodeAddress)DWCi_authDataTask + 0x6EC, (DWCiCodeAddress)DWCi_authDataTask + 0x044,
    (DWCiCodeAddress)DWCi_authDataTask + 0x0AC, (DWCiCodeAddress)DWCi_authDataTask + 0x0D4,
    (DWCiCodeAddress)DWCi_authDataTask + 0x104, (DWCiCodeAddress)DWCi_authDataTask + 0x18C,
    (DWCiCodeAddress)DWCi_authDataTask + 0x1B0, (DWCiCodeAddress)DWCi_authDataTask + 0x1D8,
    (DWCiCodeAddress)DWCi_authDataTask + 0x1FC, (DWCiCodeAddress)DWCi_authDataTask + 0x240,
    (DWCiCodeAddress)DWCi_authDataTask + 0x274, (DWCiCodeAddress)DWCi_authDataTask + 0x378,
    (DWCiCodeAddress)DWCi_authDataTask + 0x3A4, (DWCiCodeAddress)DWCi_authDataTask + 0x3CC,
    (DWCiCodeAddress)DWCi_authDataTask + 0x3F8, (DWCiCodeAddress)DWCi_authDataTask + 0x420,
    (DWCiCodeAddress)DWCi_authDataTask + 0x450, (DWCiCodeAddress)DWCi_authDataTask + 0x4AC,
    (DWCiCodeAddress)DWCi_authDataTask + 0x4D0, (DWCiCodeAddress)DWCi_authDataTask + 0x4F8,
    (DWCiCodeAddress)DWCi_authDataTask + 0x564, (DWCiCodeAddress)DWCi_authDataTask + 0x668,
    (DWCiCodeAddress)DWCi_authDataTask + 0x680, (DWCiCodeAddress)DWCi_authDataTask + 0x6A4,
    (DWCiCodeAddress)DWCi_authDataTask + 0x6EC, (DWCiCodeAddress)DWCi_authDataTask + 0x6EC,
    (DWCiCodeAddress)DWCi_authDataTask + 0x6EC,
};

typedef struct DWCiStateBlock {
    /* +0x000 */ u8 pad_0x000[0x1B8];
    /* +0x1B8 */ u32 cleared_0x1B8;    /* the count word DWCi_npStart clears on its own */
    /* +0x1BC */ u8 pad_0x1BC[0x14];
    /* +0x1D0 */ u8 region_0x1D0[0x174];
    /* +0x344 */ u8 pad_0x344[0x1C];
    /* +0x360 */ u8 region_0x360[0x20];
} DWCiStateBlock; /* size: 0x380 */


/* 0x80508630 (0x118): the console's friend code, fetched once.  The first call brings the file
 * system up, takes the two blocks `DWCi_allocNode` hands it, reads the id into the unit's own
 * `.sbss` value and reports it; every later call answers out of that value. */
u64 DWCi_GetConsoleFriendCode(void) {
    s32 vfInitialized;
    u8* first;
    u8* second;

    if (DWCi_friendCodeReady == 0) {
        vfInitialized = VFipf2IsInitialized();
        first = DWCi_allocNode(3, 0x4000, 0x20);
        second = DWCi_allocNode(3, 0x8000, 0x20);
        DWCi_friendCodeReady = 1;
        if (vfInitialized != 1) {
            VFipf2Init(second, 0x8000);
        }
        if (NWC24iGetUserId((u32*)&DWCi_consoleFriendCode) == 0) {
            DWCi_report(0x8000000, DWCi_reportFriendCodeFormat, DWCi_consoleFriendCode);
        } else {
            DWCi_report(0x8000000, DWCi_reportFriendCodeFailedFormat);
            DWCi_consoleFriendCode = 0;
        }
        if (vfInitialized != 1) {
            VFipf2Shutdown();
        }
        DWCi_freeNode(3, first, 0);
        DWCi_freeNode(3, second, 0);
    }
    return DWCi_consoleFriendCode;
}


/* 0x805091F0 (0xC): the DWCi state word. */
u32 DWCi_GetStatus(void) {
    return DWCi_stateBlock[0];
}

/* 0x805091D0 (0x18): `state == 1`. */
u32 DWCi_IsStatusReady(void) {
    return DWCi_stateBlock[0] == 1;
}

/* 0x80509200 (0xC): address of the DWCi work buffer. */
u8* DWCi_GetWorkBuffer(void) {
    return DWCi_workBuffer;
}

/* 0x80509180 (0x44): the state ladder 0x19 -> 0x1A; 1 while the state is 0, 0x1A or just advanced. */
u32 DWCi_AdvanceStatus(void) {
    if (DWCi_state == 0x19) {
        DWCi_state = 0x1A;
        return 1;
    }
    if (DWCi_state == 0 || DWCi_state == 0x1A) {
        return 1;
    }
    return 0;
}

/* 0x80509250 (0x18): publish a flag + the argument into the runtime block. */
void DWCi_SetResult(u32 arg) {
    DWCi_runtime->resultFlag = 1;
    DWCi_runtime->resultValue = arg;
}

/* 0x80508750 (0x50): drain the request list, calling DWCi_freeNode(0xC, node, 0) per node. */
u32 DWCi_FreeList(void) {
    DWCiListNode* node = DWCi_freeListHead;
    while (node != 0) {
        DWCiListNode* next = node->next;
        DWCi_freeNode(0xC, node, 0);
        DWCi_freeListHead = next;
        node = next;
    }
    return 0;
}

/* 0x805087A0 (0x88): clear the whole DWCi state block and its trailing regions, drop the runtime
 * block, remember the argument and return 1.  The reset every session begins with. */
u32 DWCi_npStart(u32 arg) {
    DWCiStateBlock* state = (DWCiStateBlock*)DWCi_stateBlock;

    memset(state, 0, 0x1D0);
    memset(state->region_0x1D0, 0, 0x174);
    memset(state->region_0x360, 0, 0x20);
    DWCi_runtime = 0;
    DWCi_state = 0;
    DWCi_initArgument = arg;
    state->cleared_0x1B8 = 0;
    DWCi_useStoredConfig = 0;
    return 1;
}

/* 0x80508830 (0x1B8): bring a DWC session's runtime block up.  The host's allocator hands over the
 * 0x5B30-byte block, the interface configuration is read into its +0x4000 region, the HTTP layer is
 * started with the caller's two command callbacks and the player name / friend code are copied in -
 * or the block goes back through the callback and 0 is answered. */
u32 DWCi_initRuntime(char* playerName, char* friendCode, u32 storedHost, u32 storedPort,
                     DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback) {
    DWCiRuntime* runtime;
    s32 error;

    if (DWCi_state != 0 && DWCi_state != 0x1A) {
        DWCi_report(0x1000000, DWCi_reportAuthProcessing);
        return 0;
    }
    DWCi_runtime = allocCallback(0, 0x5B30);
    if (DWCi_runtime == 0) {
        DWCi_report(0x1000000, DWCi_reportMemoryShortage);
        return 0;
    }
    memset(DWCi_runtime, 0, 0x5B30);
    DWCi_runtime->allocCallback = allocCallback;
    DWCi_runtime->commandCallback = commandCallback;
    error = NCDGetCurrentIfConfig(DWCi_runtime->ifConfig_0x4000);
    if (error != 0) {
        DWCi_report(0x1000000, DWCi_reportIfConfigQueryFailed, error);
    } else if (NHTTPi_RegisterCallbacks(DWCi_npSetValue, DWCi_npSetValueEx, 17) < 0) {
        DWCi_report(0x1000000, DWCi_reportHttpStartFailed);
    } else {
        wcsncpy(DWCi_runtime->playerName_0x415E, (const u16*)playerName, 26);
        strncpy(DWCi_runtime->friendCode_0x4192, friendCode, 12);
        DWCi_runtime->mode_0x59C8 = 1;
        memset(DWCi_stateBlock, 0, 0x1D0);
        ((DWCiStateBlock*)DWCi_stateBlock)->cleared_0x1B8 = 0;
        runtime = DWCi_runtime;
        runtime->storedPort = storedPort;
        runtime->storedHost = storedHost;
        DWCi_state = 1;
        return 1;
    }
    DWCi_runtime->commandCallback(0, (u32)DWCi_runtime, 0);
    return 0;
}

/* 0x805089F0 (0x7C): open a session: bring the runtime block up under the band's two empty-name
 * defaults and the caller's two host callbacks, clear the work buffer, copy the 5-byte tag into the
 * runtime block and set its mode. */
u32 DWCi_npSetup(char* tag, DWCiAllocCallback allocCallback, DWCiCommandCallback commandCallback) {
    DWCi_initRuntime(DWCi_npEmptyPlayerName, DWCi_npEmptyFriendCode, 0, 0, allocCallback,
                     commandCallback);
    memset(DWCi_workBuffer, 0, 0x174);
    strncpy(DWCi_runtime->tag_0x419E, tag, 5);
    DWCi_runtime->mode_0x59C8 = 2;
    return 1;
}

/* 0x80509210 (0x18): issue the host callback's command 13 with `value`. */
void DWCi_npSetValue(u32 value) {
    DWCi_runtime->allocCallback(13, value);
}

/* 0x80509230 (0x1C): the three-argument form of that same command. */
void DWCi_npSetValueEx(u32 value) {
    DWCi_runtime->commandCallback(13, value, 0);
}
