/*
 * Network/network_state.cpp - the NetworkSessionManager state machine
 * (`.text` 0x803FE8E4..0x804006A8, 21 functions / 7620 B).
 *
 * BOUNDARY.  Left seam 0x803FE8E4 = the closure edge of `resetNetworkState` (weak; the only left
 * candidates `tudiscover at resetNetworkState` offers are the far-away 0x803FE4C0/4C8/4D0 at share
 * 0.038).  Right seam 0x804006A8 = the top right cut of `handleNetworkState1` (weak, share 0.072,
 * "sendReqFmpListData/Head called only from this range") - it is the start of `sendReqFmpListVersion`,
 * and **the seam is unproven**: the `sendReqFmp*` family beyond it is the same session's request
 * layer.  Sections: `.text` 0x803FE8E4..0x804006A8, `extab` 0x8001BD9C..0x8001BE24,
 * `extabindex` 0x8003C06C..0x8003C138 (17 unwind records).
 *
 * WHAT IT IS.  The `NetworkSessionManager` request-state machine: the block-1..4 resets and the
 * `handleNetworkStateN` dispatchers (1 is the main machine), plus the request emitters
 * 0x803FFD74..0x804002C0 (`sendReqServerTime`/`sendReqShut`/`sendReqTicket`/`sendReqCommonKey`/
 * `sendReqLoginInfo`/`sendReqChargeInfo`/`sendReqUserListData`/`sendReqUserObject`).  C++ but every
 * function is `extern "C"` (the map's names are unmangled).
 *
 * FLAGS.  Per-unit `-O3` in `configure.py` (the lib default `-O4,p` hoists every emitters's constant
 * setup into the prologue's `mflr`->`stw` latency slot and lays the switch tails out unsorted): the
 * same source measures 69.42 % at `-O4,p` and 84.16 % at `-O3`.
 * `-Cpp_exceptions on` (`cflags_network`, flags-audit 2026-09-28) is required for the object's `extab`
 * 0x88 / `extabindex` 0xCC - the splits block claims both ranges and the target has them; neither
 * `.text` nor any per-symbol score moves with it.  `#pragma dont_inline on` around `handleNetworkState1` keeps
 * retail's `bl resetNetworkState3` in case 255 (`-inline auto` folds the 56-byte callee in);
 * `#pragma peephole off`/`on` around `handleNetworkState2Binary` keeps retail's unfused
 * `extsb r0,r0` + `cmpwi r0,0` where the pass fuses them into `extsb.` (playbook 39).
 *
 * SOURCE SHAPES (each measured on this unit; they are levers, not preferences):
 *  - A local that only *aliases* something is its own web and takes a register retail gives to the
 *    parameter or field: `NetworkStateMachine* st = (NetworkStateMachine*)self`, `u8 state =
 *    <field>`, `NetworkFmpSlot* slots = st->fmpSlots_6C40`, `NetworkUserRow* rows = ...`.  Writing
 *    the cast/field expression at the use site instead is worth 0.3-3 points per function here.
 *  - An `if (bad) return x;` written first compiles to `beq <body>` with the return in line; retail
 *    branches *over* the body and leaves the return last, which is the positive test with the return
 *    as the case's last statement (`handleNetworkState2` case 5, `handleNetworkState2Fmp` 5/20).
 *    Same for `if`/`else`: retail's layout wants the branch-taken block written as the `else`.
 *  - `a && b && c` guarding a body whose else is a short tail: retail falls through into the tail,
 *    i.e. the source is the negated `a == 0 || b == 0 || c == 0` (`handleNetworkState1` 40/90/120).
 *  - Where retail has `cmpwi`/`ble`/`cmpw`, the source compared *signed*: `(s32)field` casts
 *    (`handleNetworkState4` 0/20/35, `handleNetworkState2` 50, `handleNetworkState2Fmp` 10/70).
 *  - A state byte taken into a local is an `s32` local, not `u8` (retail's `cmpwi`, not `cmplwi`).
 *  - A `u32` count/limit compared against a small constant is `cmpwi` in retail, i.e. the source
 *    cast: `(s32)count < 30` not `count < 30`, `(s32)i < (s32)field` not `i < field`,
 *    `(s32)field == -1` not `field == (u32)-1`.
 *  - A *u8-typed* comparison survives only on a memory-loaded operand: `(u8)(field + 255) <= 1`
 *    reproduces retail's `addi r0,rX,255; clrlwi r0,r0,24; cmplwi r0,1` where `field - 1 <= 1` gives
 *    `subi`/`cmpwi` and `(u8)(field - 1)` gives `addi r0,rX,-1` + the mask.  On a local whose range
 *    the optimizer proves (the tag index) the same cast measures byte-identical - the mask is folded
 *    away, which is why `sendReqUserObject`'s masked index cannot be reached from the source side.
 *  - A local byte buffer's *declared* size is the frame: `u8 block[4]` gives a 0x20 frame where the
 *    target's 0x30 needs `u8 block[16]` (the bytes actually used are the same three), and every
 *    prologue and save offset follows from it.
 *  - An extern whose target reference is `R_PPC_EMB_SDA21` needs the symbol's real *size* in the
 *    declaration (`extern const char maskedUserName[7]`, the map's own size); with `[]` MWCC emits
 *    `lis`/`addi` plus two `R_PPC_ADDR16_*` relocations instead of one sda21 reference.
 *  - An index store written `count = count + 1; tags[count - 1] = v` instead of
 *    `tags[count] = v; count++` scores 3 points better on both rows that have it and emits
 *    `addi r3,r1,base-1` + the unmasked index - an address-base fold (alignment), not retail's
 *    `addi r3,r1,base` + masked copy.
 *
 * NAMES.  The runtime dump's map ("D:/WiiExperiment/DumpSymbols.zip") names 14 of the 21 already in
 * `symbols.txt`.  The six helpers it leaves as `zz_` were named in the registration batch's naming
 * pass from their own code (`advanceNetworkState5` gates on the `+0x6133 == 5` state byte; the three
 * block-2 dispatchers are named after the request family each drives; `sendReqOpcode1B`/`sendReqOpcode1F`
 * after their `flushBuffer` request opcode).  The unsplit callees the bodies call were renamed in the
 * same batch (rule 7, map + references): `writeUInt32`/`writeUInt32Shared` (0x803FC2D0/0x803FC414 -
 * the dump names both `writeUInt32`, so the 4-byte tail-branch twin's suffix is a GUESS),
 * `putItemTaggedLongs`/`putItemTaggedBytes` (0x8040EBBC/0x8040EC88), `chooseServerAddress`
 * (0x803FDD48, dump name), `getSomething6`/`getSomething9` (0x803FE4B8/0x803FE4C0, numbered into the
 * map's existing `getSomethingN` scheme), `dispatchSessionHandlers` (0x8041233C, walks the session's
 * eight handler slots), `setConnectionPaths` (0x80416320), `getInstance` (0x800E89D8, dump name - the
 * mediator singleton getter, **owned by `src/sound/fn_800E46E8.cpp`**, whose declaration and
 * `Network/GameSpyInterfaceThread.h`'s were updated in the same change) and the six sub-state predicates
 * `isSubState_8254_3`/`isSubState_894F_2..6` (named for the field and value each tests).  The five
 * `lbl_` rows were named from their content and use: `sessionTimeoutParam`/`sessionTimeoutParam2`
 * (0x8079C7D8/DDA = the bytes {1,2,3}), `requestHeaderWord0`/`requestHeaderWord1` (0x8079C7E0/E4 =
 * the 8-byte block {1,2,3,5,4,6,7,8}) and `maskedUserName` (0x80793968 = "******").
 *
 * NAMING GUESSES.  The map gives all three block-2 dispatchers the same `handleNetworkState2`
 * spelling (three distinct local addresses), so the `Fmp`/`Binary` suffixes are inferred from the
 * request family each calls, not from a recovered API name.  `sendReqOpcode1B`/`1F` record the wire
 * opcode because the request name is unknown (the 0x1B/0x1F ids sit in the gaps of the `sendReq*`
 * opcode table).  `NetworkStateMachine`'s field names are derived from how each field is used in this
 * unit, not from a recovered header - the object is 0x16D08 bytes and the dump's struct views do not
 * cover it.
 *
 * RESIDUALS (measured 2026-09-27, second pass: 93.85 %, 12 of 21 functions at 100 %, `.text` 7620 B
 * target / 7472 B ours - the whole size gap is `handleNetworkState2Binary`'s tokenizer below; every
 * other function is within 4-8 B of the target):
 *  - `handleNetworkState2Binary` 71.25 % (992 B / 916 B): its case-60 tokenizer is the one place the
 *    original leaves a loop from *inside* two nested loops (retail branches straight from the inner
 *    skip loop's `== 0` test to the outer loop's exit), and the conformant shape (`&& != 0` in the
 *    skip loop plus a `break`) is 76 B short.  Ours keeps a *rolling* pointer `self + i` where retail
 *    recomputes the offset from `i` and uses `lbzx` off `self`; MWCC emits the
 *    `for (k = index; k < 8; k++) tokens[k] = i` fill loop as a plain `subfic`/`mtctr` counted loop
 *    where retail has the unroll-by-8 block plus an `andi.` remainder; and our prologue saves 5
 *    callee-saved registers (`bl _savegpr_27`) where retail saves 2 in line - the only remaining
 *    relocation difference in the object (`target 1668 B` vs `ours 1692 B` of `.rela.text`).
 *    Rejected for this row: a local `u8*`/`char* text` (60.8 %), a `u32* tokens` alias (60.1 %),
 *    `-O2` (71.66 % on the row, 89.58 % unit), `-opt nostrength` (62.23 %, unit 90.47 %), and every
 *    statement/declaration/loop-form permutation tried (byte-identical or worse).  The one landed
 *    change is the `u32* tokens = self->binaryTokens_D60C;` alias (+1.03 % on the row, object 24 B
 *    *smaller*) - an alignment effect of the fuzzy metric, not retail's shape.
 *  - `sendReqUserObject` 89.87 % (520 B): the out-of-range error path is retail's now - two 12-byte
 *    `{0x80000000, 0, 0}` records written high-then-low with the *low* one's address passed - which
 *    is what a by-value `postError_288(self, info)` call produces (`NetworkPostedError` is 0x0C; a
 *    by-value parameter makes MWCC build the argument copy and re-materialise the constants), and it
 *    restores retail's 0x50 frame and every `r1+0x20` tag offset.  Left: (a) the masked tag index -
 *    retail `mr r0,rX; addi rX,rX,1; clrlwi r0,r0,24; stbx` on all five variable indices, ours
 *    index-only and unmasked; every shape tried measures byte-identical (`u8 count`, `s32 count`,
 *    block- and function-scope `u8`/`u32` temps, `(u8)count` in all positions, `count++` as the index,
 *    `tags[count++]`, `*(block + count)` and an inlined `appendTag` helper).  The reason is visible
 *    now: `count` is a *local* whose range MWCC proves clean, so it folds the u8 conversion - the
 *    mask survives only where the operand comes from memory (see `sendReqLoginInfo`) or is a
 *    `u8`-typed conversion the optimizer cannot see through; (b) the vcall's vptr load - retail's
 *    `lwz r12,0(r3)` + `lwz r12,0x288(r12)` is the genuine-virtual shape while our table view stages
 *    it through a scratch register (`lwz r5,0(r27)`), and the sibling `Network/GameSpyInterfaceThread.cpp` gets
 *    the r12 form from the same table view, so it is allocator choice, not the declaration.  Two
 *    measured shapes are landed: `count` declared *before* `found`, and the index/increment pair
 *    written `count = count + 1; tags[count - 1] = N;` rather than `tags[count] = N; count++;`.
 *  - `sendReqUnknownCheck` 92.92 % (196 B / 184 B): the same masked index (`block[count] = 2`) and one
 *    extra `li` retail has because its two `1` constants do not CSE.  The same index/increment
 *    spelling as above (89.29 -> 92.92 %).
 *  - `handleNetworkState2Fmp` 95.12 % (1192 B / 1184 B): one extra callee-saved register in the
 *    prologue (`stw r29`) - ours caches `fmpSlotCount_6608` in `r30` and keeps the case-40 loop
 *    counter in `r29`, where retail reloads the count from the object every iteration and keeps `i`
 *    in the volatile `r10`; a `while` rewrite of that loop (to force the reload) measures 92.74 %, so
 *    the caching is the optimizer's, not the shape's.  Elsewhere residual colouring (a loop counter
 *    in `r6`/`r10` where retail has `r30`/`r7`) and the `memcpy` argument order in case 50.  Landed
 *    here: case 35's `if (result > 0) { if (count > 0) {...; break;} return ...; }` layout (+1.16 %),
 *    case 40's `if (bestValue <= remaining) {...} else if (secondValue <= remaining) {...}` with the
 *    `<=` arm first (+3.59 %), the signed `(s32)fmpQueryValue_8044 == -1` test and the signed loop
 *    bounds (+0.20 %).
 *  - `handleNetworkState2` 98.19 % (1164 B / 1160 B): the loop's char test is retail's unfused
 *    `lbz; extsb; cmpwi r0,0` where ours folds to `extsb.` - and this one is **not** a peephole row
 *    (measured: a scoped peephole off/on pair around this function changes neither its bytes nor its
 *    score, unlike `handleNetworkState2Binary`'s).  Nothing tried unfuses it: an `s8`/`char` local,
 *    `(s32)`/`(s8)` casts on the element, `-O2`, and `-opt nocse` / `nopropagation` / `nodeadcode` /
 *    `nolifetimes` (each byte-identical).  Also one `mr r5,r30` and the two `setConnectionPaths`
 *    argument setups one slot out of place.  Landed here: case 70 as `if (count > 0) { loop } else
 *    { return ...; } st->requestState_6135 += 10;` (95.53 -> 98.19 %) and the signed
 *    `(s32)st->userRowCount_8BB4 > 0`.
 *  - `handleNetworkState4` 98.01 % (464 B / 460 B): the same unfused `subf` + `cmpwi r0,0` (vs our
 *    `subf.`) as `handleNetworkState2` in case 35, plus the `memset` argument order in case 10
 *    (retail computes `addi r3,r3,0x6c40` before the two `li`s).  Hoisting the subtraction into a
 *    `s32 remaining` local matches the target's *size* but drops the row to 95.47 %, so it is not
 *    landed.  Landed: case 25's `(s32)count < 30` signed min-clamp (+0.52 %).
 *  - `handleNetworkState1` 99.45 % (1736 B, byte-identical size): the switch value is in `r4` where
 *    retail has `r6`, a cascade of 6 operand rows in the two `len < 8192 ? len : 8192` argument
 *    setups (the transient `dataSent` lands in `r4`/`r6` where retail uses `r6`/`r7`) and one
 *    `li r4,2` one slot earlier in case 245.  Pure allocator colouring; no shape found moves it.
 *  - `sendReqLoginInfo` 95.30 % (228 B, byte-identical size): the u8-narrowed test is
 *    `(u8)(loginInfoSent_82B4 + 255) <= 1` - retail's `addi r0,r3,255; clrlwi r0,r0,24;
 *    cmplwi r0,1`, i.e. mod-256 arithmetic on a *memory-loaded* u8 field, which MWCC can neither fold
 *    into `subi`/`cmpwi` nor drop.  `(u8)(field - 1)` measures 95.19 % (`addi r0,r3,-1`) and the
 *    un-cast `field - 1 <= 1` 92.39 %.  `u8 block[4]` was the other half of this row: the target's
 *    frame is 0x30 and ours 0x20 until the array is declared `u8 block[16]`, which makes the whole
 *    prologue match (92.21 -> 92.39 % before the comparison fix).
 *  - `sendReqOpcode1B` 94.74 % (156 B, byte-identical size): retail loads *both* request-header words
 *    before storing either (`lwz r4,0(0); lwz r0,0(0); stw r4,8(r1); stw r0,12(r1)`); ours interleaves
 *    load/store/load/store.  A brace initialiser, hoisted `u32` temporaries, a struct-typed local,
 *    the reversed statement order and a `word0`/`word1` re-association all measure byte-identical.
 *  - `datagap`: `target-extra .text 7620 B (ours 7472 B)` - the size residual above, not a data gap;
 *    `ours-extra .rela.text 1692 B (target 1668 B)` - exactly the two `_savegpr_27`/`_restgpr_27`
 *    relocations of `handleNetworkState2Binary`'s five-register prologue (the `maskedUserName`
 *    reference is the target's `R_PPC_EMB_SDA21` again since the extern carries the map's size).
 *    `extab`/`extabindex` match the target's sizes (0x88 / 0xCC).  The unit is not a
 *    `datagap --flip-blockers` row.
 *  - FLAG AXIS (each measured with `measure.py --main .`; `recompile.py`/`measure.py` take the command
 *    line from MAIN's `build.ninja`, so a per-unit flag that has not landed cannot be measured with
 *    the default form): `-O3` is best.  `-O2` 89.58 % unit (2Binary 71.66 %); `-opt nostrength`
 *    90.47 % unit (2Binary 62.23 %); `-opt nocse`, `-opt nopropagation`, `-opt nodeadcode` and
 *    `-opt nolifetimes` all byte-identical.
 *
 * DATA.  The unit owns no data section of its own: the constants it loads are unowned `.sdata2`/
 * `.sdata` rows that dtk keeps in the band's auto objects, declared `extern` in the unit header and
 * never defined (playbook 29).
 */

#include "types.h"
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"     /* strlen */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_state.h"
#include "Network/PatInterface.h"          /* the state predicates, the call-stack helpers, isCallback */
#include "Network/network_layer_io.h"      /* the request emitters, the item writers, the hand-off dispatch */
#include "Network/NetworkCommunityPat.h"   /* the request writers */
#include "Network/NetworkWiiMediator.h"    /* setMediatorState68A / getMediatorState68A */

/* The target object carries `extab` 0x88 / `extabindex` 0xCC (the splits block claims both ranges), so
 * the original TU was built with C++ exceptions on; the lib sets them off.  The pragma adds exactly
 * those two sections and leaves `.text` (and every per-symbol score) unchanged - measured. */

extern "C" {

/* defined here */
s32 resetNetworkState(NetworkInstance* self);
s32 resetNetworkState3(NetworkInstance* self);
s32 resetNetworkState4(NetworkInstance* self);
s32 handleNetworkState1(NetworkInstance* self);
s32 handleNetworkState4(NetworkInstance* self, s32 arg);
s32 advanceNetworkState5(NetworkInstance* self);
s32 handleNetworkState2(NetworkInstance* self);
s32 handleNetworkState2Fmp(NetworkInstance* self);
s32 handleNetworkState2Binary(NetworkInstance* self);
s32 sendReqServerTime(NetworkInstance* self);
s32 sendReqShut(NetworkInstance* self, s32 mode);
s32 sendReqTicket(NetworkInstance* self);
s32 sendReqCommonKey(NetworkInstance* self);
s32 sendReqLoginInfo(NetworkInstance* self);
s32 sendReqChargeInfo(NetworkInstance* self, s32 mode);
s32 sendReqUserListData(NetworkInstance* self, s32 mode, u32 count);
s32 sendReqUserObject(NetworkInstance* self, s32 index, NetworkUserRow* row);
s32 sendReqOpcode1B(NetworkInstance* self, u32 a, u32 b);
s32 sendReqOpcode1F(NetworkInstance* self);
s32 sendServerTimeout(NetworkInstance* self, const u32* values);
s32 sendReqUnknownCheck(NetworkInstance* self, const u8* tags, const u8* data, u32 size);

/* the band's request emitters this range does not own are declared in `Network/network_layer_io.h` */

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */

s32 resetNetworkState(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    if (st->sessionArmed_6130 == 0) {
        st->sessionArmed_6130 = 1;
        st->sessionState_6132 = 0;
        st->subState_6133 = 0;
        st->requestState_6135 = 0;
        st->shutdownMode_6136 = 0;
        st->flag_6558 = 0;
        st->shutdownFlag_6559 = 0;
        st->patPhase_894E = 0;
    }
    return 0;
}

s32 resetNetworkState3(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    if (st->termsArmed_6131 == 0) {
        st->termsArmed_6131 = 1;
        st->sessionState_6132 = 0;
        st->subState_6133 = 0;
        st->shutdownMode_6136 = 0;
        st->sessionArmed_6130 = 0;
        st->flag_6558 = 0;
        st->shutdownFlag_6559 = 0;
    }
    return 0;
}

/* Drives the request state machine one step for the current `+0x6132` state.  The `dont_inline` region
 * is load-bearing: with `-inline auto` MWCC folds the 56-byte `resetNetworkState3` into case 255's
 * `bl`, which retail keeps (the proc is also called from another TU, so the original build did not
 * inline it here). */
#pragma dont_inline on
s32 handleNetworkState1(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 flag;
    s32 mode;

    switch (st->sessionState_6132) {
    case 1:
        if (st->binaryState_6134 == 10) {
            st->binaryState_6134 = 90;
        }
        break;
    case 5:
        st->sessionState_6132 += 5;
        break;
    case 10:
        if (isSubState_8254_3((PatInterface*)st) != 0) {
            st->sessionState_6132 = 200;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqAuthenticationToken(self, getNASToken(getInstance()));
        break;
    case 20:
        if (isOpeningMaintenanceServer((PatInterface*)st) == 0) {
            if (st->announcePending_8270 != 0) {
                *st->announceBufferPtr_8290 = 0;
            }
            st->sessionState_6132 += 10;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqMaintenance(self);
        break;
    case 30:
        if (isOpeningMaintenanceTerms((PatInterface*)st) != 0) {
            if (st->termsSize_826C != 0) {
                *st->termsBufferPtr_828C = 0;
            }
            st->sessionState_6132 = 245;
            break;
        }
        st->dataTotal_825C = 0;
        st->sessionState_6132 += 5;
        sendReqTermsVersion(self);
        break;
    case 40:
        if (st->termsReady_8268 == 0 || st->dataTotal_825C == 0 || st->termsSize_826C == 0) {
            if (st->termsSize_826C != 0) {
                *st->termsBufferPtr_828C = 0;
            }
            st->sessionState_6132 += 20;
            break;
        }
        if (st->dataTotal_825C >= st->termsSize_826C) {
            st->dataTotal_825C = st->termsSize_826C - 1;
        }
        memset(st->termsBufferPtr_828C, 0, st->termsSize_826C);
        st->dataSent_8260 = 0;
        st->sessionState_6132 += 5;
        break;
    case 45:
    {
        u32 len;

        st->sessionState_6132 += 5;
        len = st->dataTotal_825C - st->dataSent_8260;
        sendReqTerms(self, st->termsBuffer_8264, st->dataSent_8260,
                     len < 8192 ? len : 8192);
        break;
    }
    case 55:
        if (st->dataTotal_825C > st->dataSent_8260) {
            st->sessionState_6132 -= 10;
        } else {
            st->sessionState_6132 += 5;
        }
        break;
    case 60:
        if (isOpeningMaintenanceServer((PatInterface*)st) != 0) {
            if (st->serverInfoPending_8274 != 0) {
                *st->serverInfoPtr_8294 = 0;
            }
            st->sessionState_6132 = 245;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqAnnounce(self);
        break;
    case 70:
        if (st->chargePending_8278 == 0) {
            st->sessionState_6132 += 10;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqNoCharge(self);
        break;
    case 80:
        st->dataTotal_825C = 0;
        st->sessionState_6132 += 5;
        sendReqVulgarityInfoLow(self, 2);
        break;
    case 90:
        if (st->dataTotal_825C == 0 || st->vulgaritySize_8284 == 0) {
            if (st->vulgaritySize_8284 != 0) {
                *st->vulgarityPtr_82A4 = 0;
            }
            st->sessionState_6132 += 20;
            break;
        }
        if (st->dataTotal_825C >= st->vulgaritySize_8284) {
            st->dataTotal_825C = st->vulgaritySize_8284 - 1;
        }
        memset(st->vulgarityPtr_82A4, 0, st->vulgaritySize_8284);
        st->dataSent_8260 = 0;
        st->sessionState_6132 += 5;
        break;
    case 95:
    {
        u32 len;

        st->sessionState_6132 += 5;
        len = st->dataTotal_825C - st->dataSent_8260;
        sendReqVulgarityLow(self, 2, st->sendSlice_8258, len < 8192 ? len : 8192);
        break;
    }
    case 105:
        if (st->dataTotal_825C > st->dataSent_8260) {
            st->sessionState_6132 -= 10;
        } else {
            st->sessionState_6132 += 5;
        }
        break;
    case 110:
        st->dataTotal_825C = 0;
        st->sessionState_6132 += 5;
        sendReqVulgarityInfoLow(self, 1);
        break;
    case 120:
        if (st->dataTotal_825C == 0 || st->userListSize_8280 == 0) {
            if (st->userListSize_8280 != 0) {
                *st->userListPtr_82A0 = 0;
            }
            st->sessionState_6132 += 20;
            break;
        }
        if (st->dataTotal_825C >= st->userListSize_8280) {
            st->dataTotal_825C = st->userListSize_8280 - 1;
        }
        memset(st->userListPtr_82A0, 0, st->userListSize_8280);
        st->dataSent_8260 = 0;
        st->sessionState_6132 += 5;
        break;
    case 125:
    {
        u32 len;

        st->sessionState_6132 += 5;
        len = st->dataTotal_825C - st->dataSent_8260;
        sendReqVulgarityLow(self, 1, st->sendSlice_8258, len < 8192 ? len : 8192);
        break;
    }
    case 135:
        if (st->dataTotal_825C > st->dataSent_8260) {
            st->sessionState_6132 -= 10;
        } else {
            st->sessionState_6132 += 5;
        }
        break;
    case 140:
        st->sessionState_6132 += 5;
        sendReqCommonKey(self);
        break;
    case 150:
        st->sessionState_6132 += 5;
        sendReqLmpConnect(self);
        break;
    case 160:
        st->sessionState_6132 = 245;
        break;
    case 200:
        st->sessionState_6132 += 5;
        sendReqMediaVersionInfo(self);
        break;
    case 210:
        st->sessionState_6132 = 245;
        break;
    case 245:
        if (st->binaryState_6134 != 10) {
            st->sessionState_6132 += 1;
            break;
        }
        st->sessionState_6132 += 5;
        flag = 0;
        if (isOpeningMaintenanceServer((PatInterface*)st) != 0 || isSubState_8254_3((PatInterface*)st) != 0) {
            flag = 1;
        }
        mode = 2;
        if (flag != 0) {
            mode = 1;
        }
        sendReqShut(self, mode);
        break;
    case 255:
        resetNetworkState3(self);
        chooseServerAddress(st, 0, 0);
        updatePatInterface((PatInterface*)st, 0, 0, 0);
        return 1;
    default:
        break;
    }
    return 0;
}

#pragma dont_inline off

/* Clears the `+0x6133` sub-state once the `+0x894D` phase has advanced past 1. */
s32 advanceNetworkState5(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 state = st->subState_6133;

    if (state == 5) {
        if (st->fmpPhase_894D != 1) {
            return -state;
        }
        st->subState_6133 = 0;
        return 1;
    }
    return 0;
}

/* Drives the block-2 account sub-machine: login, charge, ticket and the user list. */
s32 handleNetworkState2(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    switch (st->requestState_6135) {
    case 5:
        if (st->fmpPhase_894D == 1) {
            if (st->loginInfoSent_82B4 == 0) {
                st->requestState_6135 = st->requestState_6135 + 5;
                break;
            }
            st->requestState_6135 = 120;
            break;
        }
        return -(s32)(st->requestState_6135 + 1);
    case 10:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqLoginInfo(self);
        break;
    case 20:
        if (st->connectionPhase_894F == 1) {
            st->requestState_6135 = st->requestState_6135 + 10;
            break;
        }
        if (isSubState_894F_5((PatInterface*)st) != 0 || isSubState_894F_3((PatInterface*)st) != 0) {
            u32 size = st->replySize_8BC4;

            if (size != 0) {
                u32 len = size - 1;

                if (strlen(st->termText_8951) < len) {
                    len = strlen(st->termText_8951);
                }
                memcpy(st->replyBuffer_8BC8, st->termText_8951, len);
                st->replyBuffer_8BC8[len] = 0;
            }
            st->requestState_6135 = 250;
            break;
        }
        if (isSubState_894F_2((PatInterface*)st) != 0 || isSubState_894F_4or6((PatInterface*)st) != 0) {
            st->requestState_6135 = 225;
            break;
        }
        return -st->requestState_6135;
    case 30:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqTicket(self);
        break;
    case 40:
        if (isSubState_894F_6((PatInterface*)st) != 0) {
            st->requestState_6135 = 250;
            break;
        }
        st->requestState_6135 += 5;
        sendReqOpcode1B(self, 1, 6);
        break;
    case 50:
        if ((s32)st->userRowCount_8BB4 > 0) {
            u32 total;
            u32 rows;

            st->userRows_8BB8 = createStack(st, st->userRowCount_8BB4 * 92, &total);
            rows = total / 92;
            if ((s32)st->userRowCount_8BB4 > (s32)rows) {
                st->userRowCount_8BB4 = rows;
                growStackSize(st, total - rows * 92);
            }
            st->requestState_6135 += 5;
            sendReqUserListData(self, 1, st->userRowCount_8BB4);
            break;
        }
        st->requestState_6135 = st->requestState_6135 + 10;
        break;
    case 60:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqOpcode1F(self);
        break;
    case 70:
    {
        s32 count;
        s32 i;

        count = (s32)st->userRowCount_8BB4;
        if (count > 0) {
            for (i = 0; i < count; i++) {
                if (((NetworkUserRow*)st->userRows_8BB8)[i].shortName_04[0] == 0) {
                    return -(s32)st->requestState_6135;
                }
            }
        } else {
            return -(s32)(st->requestState_6135 + 1);
        }
        st->requestState_6135 += 10;
        break;
    }
    case 80:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqServerTime(self);
        break;
    case 90:
        dispatchSessionHandlers(st, 3, 0, 0, st->userRowCount_8BB4, st->userRows_8BB8);
        return 1;
    case 100:
        st->loginInfoSent_82B4 = 1;
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqChargeInfo(self, 1);
        break;
    case 110:
        setConnectionPaths((NetworkInstance*)getInstance(), st->userIdText_82D5,
                           st->userPasswordText_8301);
        setMediatorState68A((NetworkWiiMediatorFields*)getInstance(), 0);
        st->requestState_6135 = 10;
        break;
    case 120:
        st->loginInfoSent_82B4 = 3;
        setMediatorState68A((NetworkWiiMediatorFields*)getInstance(), 0);
        st->requestState_6135 = 10;
        break;
    case 230:
        if (isSubState_894F_2((PatInterface*)st) != 0) {
            st->requestState_6135 = 30;
            break;
        }
        if (isSubState_894F_6((PatInterface*)st) != 0) {
            st->requestState_6135 += 5;
            sendReqRfpConnect(self);
            break;
        }
        st->requestState_6135 += 20;
        break;
    case 240:
        if (isSubState_894F_6((PatInterface*)st) != 0) {
            st->requestState_6135 = 30;
            break;
        }
        st->requestState_6135 += 10;
        break;
    case 250:
        return 1;
    default:
        break;
    }
    return 0;
}

/* Drives the block-2 FMP sub-machine: the friend-list versions, rows and info requests. */
s32 handleNetworkState2Fmp(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    switch (st->requestState_6135) {
    case 5:
        if (st->fmpPhase_894D == 2) {
            if (st->loginInfoSent_82B4 == 0) {
                st->requestState_6135 = st->requestState_6135 + 5;
            } else {
                u8 mediatorState;

                getMediatorState68A((NetworkWiiMediatorFields*)getInstance(), &mediatorState);
                if (mediatorState == 1) {
                    st->requestState_6135 = 100;
                    break;
                }
                st->requestState_6135 += 5;
            }
        } else {
            return -(s32)(st->requestState_6135 + 1);
        }
        /* falls through */
    case 10:
    {
        NetworkUserRow* rows;
        s32 count;
        s32 i;

        count = (s32)st->userRowCount_8BB4;
        rows = (NetworkUserRow*)st->userRows_8BB8;
        for (i = 0; i < count; i++) {
            if (rows[i].id_00 == st->userRow_8B54.id_00) {
                break;
            }
        }
        if (i >= count) {
            return -(s32)st->requestState_6135;
        }
        st->userRow_8B54.field_30 = 2;
        st->userRow_8B54.field_2C = getSomething3(st);
        st->userRow_8B54.field_34 = getSomething6(st);
        st->userRow_8B54.field_38 = getSomething9(st);
        st->requestState_6135 += 5;
        sendReqUserObject(self, i, &st->userRow_8B54);
        growStackSize(st, count * 92);
        st->userRows_8BB8 = 0;
        break;
    }
    case 20:
        if (st->sessionReady_8950 == 1) {
            st->requestState_6135 = st->requestState_6135 + 5;
            sendReqTicket(self);
            break;
        }
        return -(s32)st->requestState_6135;
    case 30:
        st->requestState_6135 = st->requestState_6135 + 5;
        resetNetworkState4(self);
        break;
    case 35:
    {
        s32 result = handleNetworkState4(self, 80);

        if (result > 0) {
            if ((s32)st->fmpSlotCount_6608 > 0) {
                st->requestState_6135 += 5;
                break;
            }
            return -(s32)st->requestState_6135;
        }
        if (result < 0) {
            return -(s32)(st->requestState_6135 + result);
        }
        break;
    }
    case 40:
    {
        u32 bestIndex = (u32)-1;
        s32 bestValue = -1;
        s32 secondValue = -1;
        u32 i;

        st->fmpQueryValue_8044 = (u32)-1;
        for (i = 0; (s32)i < (s32)st->fmpSlotCount_6608; i++) {
            u32 remaining;

            if ((s32)st->fmpSlots_6C40[i].total_14 <= 0) {
                continue;
            }
            remaining = st->fmpSlots_6C40[i].total_14 - st->fmpSlots_6C40[i].done_10;
            if (bestValue <= (s32)remaining) {
                secondValue = bestValue;
                st->fmpQueryValue_8044 = bestIndex;
                bestValue = (s32)remaining;
                bestIndex = i;
            } else if (secondValue <= (s32)remaining) {
                secondValue = (s32)remaining;
                st->fmpQueryValue_8044 = i;
            }
        }
        if ((s32)st->fmpQueryValue_8044 == -1) {
            st->requestState_6135 += 20;
            break;
        }
        st->fmpSelected_65F8 = st->fmpQueryValue_8044;
        st->requestState_6135 += 5;
        sendReqFmpInfo(self, st->fmpSlots_6C40[st->fmpSelected_65F8].payload_00, 1);
        break;
    }
    case 50:
        memcpy(st->fmpReply_8048, st->fmpReserve_671A, 262);
        st->requestState_6135 += 10;
        break;
    case 60:
    {
        s32 bestIndex = -1;
        s32 bestValue = -1;
        u32 i;

        for (i = 0; (s32)i < (s32)st->fmpSlotCount_6608; i++) {
            u32 remaining;

            if ((s32)st->fmpSlots_6C40[i].total_14 <= 0) {
                continue;
            }
            remaining = st->fmpSlots_6C40[i].total_14 - st->fmpSlots_6C40[i].done_10;
            if (bestValue <= (s32)remaining) {
                bestValue = (s32)remaining;
                bestIndex = i;
            }
        }
        if (bestIndex < 0) {
            return -(s32)st->requestState_6135;
        }
        st->fmpSelected_65F8 = (u32)bestIndex;
        st->requestState_6135 += 5;
        sendReqFmpInfo(self, st->fmpSlots_6C40[bestIndex].payload_00, 1);
        break;
    }
    case 70:
        if (st->handlersArmed_60E8 == 0) {
            dispatchSessionHandlers(st, 0x8001, 0, 0, st->fmpSlotCount_6608,
                                    (const u8*)st->fmpSlots_6C40);
        }
        return 1;
    case 100:
        st->loginInfoSent_82B4 = 2;
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqChargeInfo(self, 2);
        break;
    case 110:
        setConnectionPaths((NetworkInstance*)getInstance(), st->userIdText_82D5,
                           st->userPasswordText_8301);
        st->requestState_6135 = 200;
        break;
    case 200:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqLoginInfo(self);
        break;
    case 210:
        if (st->connectionPhase_894F == 1) {
            st->requestState_6135 = 10;
            break;
        }
        return -(s32)st->requestState_6135;
    default:
        break;
    }
    return 0;
}

/* Drives the block-2 binary/circle sub-machine. */
#pragma peephole off
s32 handleNetworkState2Binary(NetworkInstance* self)
{
    u8 state = ((NetworkStateMachine*)self)->requestState_6135;

    switch (state) {
    case 5:
        if (((NetworkStateMachine*)self)->fmpPhase_894D == 5) {
            ((NetworkStateMachine*)self)->binaryState_6134 = 90;
            ((NetworkStateMachine*)self)->requestState_6135 = 0;
            ((NetworkStateMachine*)self)->flag_6558 = 0;
            ((NetworkStateMachine*)self)->patPhase_894E = ((NetworkStateMachine*)self)->fmpPhase_894D;
            break;
        }
        ((NetworkStateMachine*)self)->fmpQueryValue_8044 = (u32)-1;
        if (((NetworkStateMachine*)self)->fmpPhase_894D == 3) {
            ((NetworkStateMachine*)self)->shutdownFlag_6559 = 1;
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            break;
        }
        return -state;
    case 10:
        ((NetworkStateMachine*)self)->requestState_6135 = state + 5;
        sendReqServerTime(self);
        break;
    case 20:
        if (isCallback(self, 3) != 0) {
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            sendReqCircleInfoNoticeSet(self);
            break;
        }
        ((NetworkStateMachine*)self)->requestState_6135 += 10;
        break;
    case 30:
        if (((NetworkStateMachine*)self)->binaryActive_D400 != 0) {
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            sendReqBinaryHead(self, ((NetworkStateMachine*)self)->binaryActive_D400, 6);
            break;
        }
        ((NetworkStateMachine*)self)->requestState_6135 = 70;
        break;
    case 40:
        if (((NetworkStateMachine*)self)->binaryTextReady_D408 != 0 && ((NetworkStateMachine*)self)->dataTotal_825C != 0) {
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            sendReqBinaryData(self, ((NetworkStateMachine*)self)->binaryActive_D400, ((NetworkStateMachine*)self)->binarySize_D404, 0);
            break;
        }
        ((NetworkStateMachine*)self)->requestState_6135 += 10;
        break;
    case 50:
        ((NetworkStateMachine*)self)->requestState_6135 += 5;
        sendReqBinaryFoot(self, ((NetworkStateMachine*)self)->binaryActive_D400);
        break;
    case 60:
        if (((NetworkStateMachine*)self)->binaryTextReady_D408 != 0) {
            u32 i = 0;
            u32 index = 0;
            u32 j;
            u32 k;

            u32* tokens = ((NetworkStateMachine*)self)->binaryTokens_D60C;
            memset(((NetworkStateMachine*)self)->binaryTokens_D60C, 0, sizeof(((NetworkStateMachine*)self)->binaryTokens_D60C));
            for (j = 0; j < 8; j++) {
                while (((NetworkStateMachine*)self)->binaryText_D409[i] != '\t' && ((NetworkStateMachine*)self)->binaryText_D409[i] != 0) {
                    i++;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == 0) {
                    break;
                }
                i++;
                while (((NetworkStateMachine*)self)->binaryText_D409[i] == '\t') {
                    i++;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == '\r' || ((NetworkStateMachine*)self)->binaryText_D409[i] == '\n') {
                    ((NetworkStateMachine*)self)->binaryText_D409[i] = 0;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == 0) {
                    break;
                }
                tokens[index] = i;
                index++;
                i++;
                while (((NetworkStateMachine*)self)->binaryText_D409[i] != '\r' && ((NetworkStateMachine*)self)->binaryText_D409[i] != '\n' &&
                       ((NetworkStateMachine*)self)->binaryText_D409[i] != 0) {
                    i++;
                }
                while (((NetworkStateMachine*)self)->binaryText_D409[i] == '\r' || ((NetworkStateMachine*)self)->binaryText_D409[i] == '\n') {
                    ((NetworkStateMachine*)self)->binaryText_D409[i] = 0;
                    i++;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == 0) {
                    break;
                }
            }
            for (k = index; k < 8; k++) {
                tokens[k] = i;
            }
        }
        ((NetworkStateMachine*)self)->requestState_6135 += 10;
        break;
    case 70:
        ((NetworkStateMachine*)self)->requestState_6135 += 5;
        reqUserSearchInfoMine(self, 0);
        break;
    case 80:
        ((NetworkStateMachine*)self)->requestState_6135 = 0;
        return 1;
    default:
        break;
    }
    return 0;
}
#pragma peephole on

/* ------------------------------------------------------------------------------------------------ */
/* the request emitters                                                                              */
/* ------------------------------------------------------------------------------------------------ */

s32 sendReqServerTime(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 2, 0);
    writeUInt32(st, st->loginDataCursor_6560);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqShut(NetworkInstance* self, s32 mode)
{
    s32 ret;

    ((NetworkStateMachine*)self)->shutdownFlag_6559 = 0;
    ((NetworkStateMachine*)self)->shutdownMode_6136 = mode;
    ret = flushBuffer(((NetworkStateMachine*)self), 4, 0);
    writeUInt8(((NetworkStateMachine*)self), mode);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqTicket(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 11, 0);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendServerTimeout(NetworkInstance* self, const u32* values)
{
    SessionTimeoutPayload payload;
    s32 ret;

    payload.code_00 = sessionTimeoutParam;
    payload.reason_02 = sessionTimeoutParam2;
    ret = flushBuffer(((NetworkStateMachine*)self), 17, 0);
    putItemTaggedLongs(((NetworkStateMachine*)self), values, 3, (const u8*)&payload);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqCommonKey(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 18, 0);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqUnknownCheck(NetworkInstance* self, const u8* tags, const u8* data, u32 size)
{
    u8 block[8];
    u32 count = 0;
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 20, 0);
    writeUInt8Array(((NetworkStateMachine*)self), data, size);
    if (tags != 0) {
        if (tags[0] != 0) {
            count = 1;
            block[0] = 1;
        }
        if (tags[1] != 0) {
            count = count + 1;
            block[count - 1] = 2;
        }
    }
    putItemTaggedBytes(((NetworkStateMachine*)self), tags, count, block);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqLoginInfo(NetworkInstance* self)
{
    u8 block[16];
    u32 count = 0;
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 23, 0);
    if (((NetworkStateMachine*)self)->loginInfoSent_82B4 == 0) {
        block[0] = 7;
        block[1] = 8;
        count = 3;
        block[2] = 9;
    } else if ((u8)(((NetworkStateMachine*)self)->loginInfoSent_82B4 + 255) <= 1) {
        block[0] = 6;
        count = 2;
        block[1] = 9;
    } else if (((NetworkStateMachine*)self)->loginInfoSent_82B4 == 3) {
        block[0] = 10;
        count = 2;
        block[1] = 9;
    }
    putItemAny(((NetworkStateMachine*)self), ((NetworkStateMachine*)self)->loginFields_82AC, count, block);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqChargeInfo(NetworkInstance* self, s32 mode)
{
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 25, 0);
    writeUInt8(((NetworkStateMachine*)self), mode);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqOpcode1B(NetworkInstance* self, u32 a, u32 b)
{
    u32 header[2];
    s32 ret;

    header[0] = requestHeaderWord0;
    header[1] = requestHeaderWord1;
    ret = flushBuffer(((NetworkStateMachine*)self), 27, 0);
    writeUInt32Shared(((NetworkStateMachine*)self), a);
    writeUInt32Shared(((NetworkStateMachine*)self), b);
    writeUInt8Array2(((NetworkStateMachine*)self), 8, (const u8*)header);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqUserListData(NetworkInstance* self, s32 mode, u32 count)
{
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 29, 0);
    writeUInt32Shared(((NetworkStateMachine*)self), mode);
    writeUInt32Shared(((NetworkStateMachine*)self), count);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqOpcode1F(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 31, 0);
    encryptBuffer(st);
    return (u16)ret;
}

/* Sends one user's row, tagging every field that differs from the local row. */
s32 sendReqUserObject(NetworkInstance* self, s32 index, NetworkUserRow* row)
{
    u32 count = 0;
    NetworkUserRow* found;
    u8 tags[16];
    s32 flag;
    s32 ret;

    if (index < 0 || index >= (s32)((NetworkStateMachine*)self)->userRowCount_8BB4) {
        NetworkPostedError info;

        info.code_00 = -2147483648;
        info.param1_04 = 0;
        info.param2_08 = 0;
        ((void (*)(void*, NetworkPostedError))((NetworkInstance*)self)->vtable->postError_288)(self, info);
        return -1;
    }
    found = (NetworkUserRow*)(((NetworkStateMachine*)self)->userRows_8BB8 + index * 92);
    if (strcmp(row->accountName_0C, found->accountName_0C) != 0) {
        count = 1;
        tags[0] = 3;
    }
    if (row->field_30 != found->field_30) {
        count = count + 1;
        tags[count - 1] = 5;
    }
    if (row->field_2C != found->field_2C) {
        count = count + 1;
        tags[count - 1] = 4;
    }
    if (row->field_34 != found->field_34) {
        count = count + 1;
        tags[count - 1] = 6;
    }
    if (row->field_38 != found->field_38) {
        count = count + 1;
        tags[count - 1] = 7;
    }
    if (strcmp(row->playerName_3C, found->playerName_3C) != 0) {
        count = count + 1;
        tags[count - 1] = 8;
    }
    if (strcmp(found->shortName_04, maskedUserName) == 0) {
        flag = 1;
    } else {
        flag = count == 0 ? 2 : 3;
    }
    ret = flushBuffer(((NetworkStateMachine*)self), 33, 0);
    writeBool(((NetworkStateMachine*)self), flag);
    writeUInt32Shared(((NetworkStateMachine*)self), (u32)(index + 1));
    putUserSlotObjects(((NetworkStateMachine*)self), (const u8*)row, count, tags);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 resetNetworkState4(NetworkInstance* self)
{
    ((NetworkStateMachine*)self)->fmpState_6137 = 0;
    return 0;
}

/* Drives the block-4 FMP list sub-machine (version, head, data, foot). */
s32 handleNetworkState4(NetworkInstance* self, s32 arg)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    switch (st->fmpState_6137) {
    case 0:
        if ((s32)st->sessionMode_65F0 != 0 && (s32)st->sessionMode_65F0 != 1) {
            return -st->fmpState_6137;
        }
        st->fmpState_6137 += 5;
        sendReqFmpListVersion(self);
        break;
    case 10:
        if (st->fmpListActive_6C38 == 0) {
            return -st->fmpState_6137;
        }
        if (st->fmpListReady_6C3C != 0) {
            st->fmpSlotCount_6608 = 0;
            memset(st->fmpSlots_6C40, 0, 5120);
        }
        st->replySent_8BCC = 0;
        st->fmpState_6137 += 5;
        sendReqFmpListHead(self, 1, arg);
        break;
    case 20:
        if ((s32)st->replyTotal_8BD0 > 0) {
            st->fmpState_6137 += 5;
        } else {
            st->fmpState_6137 += 20;
        }
        break;
    case 25:
    {
        u32 finished;
        u32 count;

        st->fmpState_6137 += 5;
        finished = st->replySent_8BCC;
        count = st->replyTotal_8BD0 - finished;
        sendReqFmpListData(self, finished + 1, (s32)count < 30 ? (s32)count : 30);
        break;
    }
    case 35:
        if ((s32)st->replySent_8BCC < 80 && (s32)(st->replyTotal_8BD0 - st->replySent_8BCC) > 0) {
            st->fmpState_6137 -= 10;
        } else {
            st->fmpState_6137 += 5;
        }
        break;
    case 40:
        st->fmpState_6137 += 5;
        sendReqFmpListFoot(self);
        break;
    case 50:
        st->fmpSelected_65F8 = getFmpSlotIndex(st, st->fmpSelected_8040);
        return 1;
    default:
        break;
    }
    return 0;
}
