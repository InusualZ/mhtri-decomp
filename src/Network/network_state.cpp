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
 * same source measures 69.42 % at `-O4,p` and 84.16 % at `-O3`, with five more functions at 100 %.
 * `#pragma exceptions on` is required for the object's `extab` 0x88 / `extabindex` 0xCC - the splits
 * block claims both ranges, the target has them and the lib sets exceptions off; neither `.text` nor
 * any per-symbol score moves with it.  `#pragma dont_inline on` around `handleNetworkState1` keeps
 * retail's `bl resetNetworkState3` in case 255 (`-inline auto` folds the 56-byte callee in).
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
 * `Network/fn_8041A87C.h`'s were updated in the same change) and the six sub-state predicates
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
 * RESIDUALS (measured; the unit scores 84.16 %, 7 of 21 functions at 100 % - the remaining 15.8 %):
 *  - `handleNetworkState2Binary` 61.05 %: its case-60 tokenizer is the one place the original leaves a
 *    loop from *inside* two nested loops (retail branches straight from the `== 0` test of the inner
 *    skip loop to the outer loop's exit).  The conformant shape (`&& != 0` in the skip loop plus a
 *    `break`) is written instead because `goto` is forbidden by section 6.5 rule 8, which leaves the
 *    function 112 B short of the target and most of the case body mismatched.
 *  - Register colouring: the switch value lands in `r4` where retail has `r6`
 *    (`handleNetworkState1`), `r4` vs `r6` (`handleNetworkState2`), `r31`/`r30` swapped for
 *    `self`/`session`; every affected instruction is an operand mismatch, not a different opcode.
 *  - `resetNetworkState` 99.94 % (one operand), `sendReqShut` 99.11 %, `sendServerTimeout` 99.19 %,
 *    `sendReqChargeInfo` 99.0 %, `sendReqUserListData` 98.71 %, `advanceNetworkState5` 96.0 %
 *    (`cmplwi` where retail has `cmpwi` - retail's local is signed, `s8` adds an `extsb`).
 *  - `sendReqUserObject` 82.24 %: its out-of-range error path builds two 12-byte records (retail
 *    stores `{0x80000000, 0, 0}` at `r1+20` *and* at `r1+8`) and reaches `postError_288` through a
 *    scratch register where retail's canonical vcall uses `r12` twice; ours builds one record.
 *  - `datagap`: `target-extra .text 7620 B (ours 7384 B)` - the size residual above, not a data gap;
 *    `ours-extra .rela.text 1680 B (target 1668 B)` - one extra relocation.  `extab`/`extabindex` now
 *    match the target's sizes (0x88 / 0xCC).
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

/* The target object carries `extab` 0x88 / `extabindex` 0xCC (the splits block claims both ranges), so
 * the original TU was built with C++ exceptions on; the lib sets them off.  The pragma adds exactly
 * those two sections and leaves `.text` (and every per-symbol score) unchanged - measured. */
#pragma exceptions on

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

/* the band's request emitters this range does not own */
void sendReqAuthenticationToken(NetworkInstance* self, u32 token);
void sendReqMaintenance(NetworkInstance* self);
void sendReqTermsVersion(NetworkInstance* self);
void sendReqTerms(NetworkInstance* self, u32 a, u32 b, u32 len);
void sendReqAnnounce(NetworkInstance* self);
void sendReqNoCharge(NetworkInstance* self);
void sendReqVulgarityInfoLow(NetworkInstance* self, s32 mode);
void sendReqVulgarityLow(NetworkInstance* self, s32 mode, u32 slice, u32 len);
void sendReqLmpConnect(NetworkInstance* self);
void sendReqMediaVersionInfo(NetworkInstance* self);
void sendReqRfpConnect(NetworkInstance* self);
void sendReqFmpListVersion(NetworkInstance* self);
void sendReqFmpListHead(NetworkInstance* self, s32 a, s32 b);
void sendReqFmpListData(NetworkInstance* self, u32 start, u32 count);
void sendReqFmpListFoot(NetworkInstance* self);
void sendReqFmpInfo(NetworkInstance* self, u32 value, s32 flag);
void sendReqBinaryHead(NetworkInstance* self, s32 a, s32 b);
void sendReqBinaryData(NetworkInstance* self, s32 a, u32 size, s32 flag);
void sendReqBinaryFoot(NetworkInstance* self, s32 a);
void sendReqCircleInfoNoticeSet(NetworkInstance* self);
void reqUserSearchInfoMine(NetworkInstance* self, s32 mode);


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
        if (isSubState_8254_3(st) != 0) {
            st->sessionState_6132 = 200;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqAuthenticationToken(self, getNASToken((NetworkInstance*)getInstance()));
        break;
    case 20:
        if (isOpeningMaintenanceServer() == 0) {
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
        if (isOpeningMaintenanceTerms() != 0) {
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
        if (st->termsReady_8268 != 0 && st->dataTotal_825C != 0 && st->termsSize_826C != 0) {
            if (st->dataTotal_825C >= st->termsSize_826C) {
                st->dataTotal_825C = st->termsSize_826C - 1;
            }
            memset(st->termsBufferPtr_828C, 0, st->termsSize_826C);
            st->dataSent_8260 = 0;
            st->sessionState_6132 += 5;
            break;
        }
        if (st->termsSize_826C != 0) {
            *st->termsBufferPtr_828C = 0;
        }
        st->sessionState_6132 += 20;
        break;
    case 45:
    {
        u32 len = st->dataTotal_825C - st->dataSent_8260;

        st->sessionState_6132 += 5;
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
        if (isOpeningMaintenanceServer() != 0) {
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
        if (st->dataTotal_825C != 0 && st->vulgaritySize_8284 != 0) {
            if (st->dataTotal_825C >= st->vulgaritySize_8284) {
                st->dataTotal_825C = st->vulgaritySize_8284 - 1;
            }
            memset(st->vulgarityPtr_82A4, 0, st->vulgaritySize_8284);
            st->dataSent_8260 = 0;
            st->sessionState_6132 += 5;
            break;
        }
        if (st->vulgaritySize_8284 != 0) {
            *st->vulgarityPtr_82A4 = 0;
        }
        st->sessionState_6132 += 20;
        break;
    case 95:
    {
        u32 len = st->dataTotal_825C - st->dataSent_8260;

        st->sessionState_6132 += 5;
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
        if (st->dataTotal_825C != 0 && st->userListSize_8280 != 0) {
            if (st->dataTotal_825C >= st->userListSize_8280) {
                st->dataTotal_825C = st->userListSize_8280 - 1;
            }
            memset(st->userListPtr_82A0, 0, st->userListSize_8280);
            st->dataSent_8260 = 0;
            st->sessionState_6132 += 5;
            break;
        }
        if (st->userListSize_8280 != 0) {
            *st->userListPtr_82A0 = 0;
        }
        st->sessionState_6132 += 20;
        break;
    case 125:
    {
        u32 len = st->dataTotal_825C - st->dataSent_8260;

        st->sessionState_6132 += 5;
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
        if (isOpeningMaintenanceServer() != 0 || isSubState_8254_3(st) != 0) {
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
        updatePatInterface(st, 0, 0, 0);
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
    u8 state = st->subState_6133;

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
    u8 state = st->requestState_6135;

    switch (state) {
    case 5:
        if (st->fmpPhase_894D != 1) {
            return -(s32)(state + 1);
        }
        if (st->loginInfoSent_82B4 == 0) {
            st->requestState_6135 = state + 5;
            break;
        }
        st->requestState_6135 = 120;
        break;
    case 10:
        st->requestState_6135 = state + 5;
        sendReqLoginInfo(self);
        break;
    case 20:
        if (st->connectionPhase_894F == 1) {
            st->requestState_6135 = state + 10;
            break;
        }
        if (isSubState_894F_5(st) != 0 || isSubState_894F_3(st) != 0) {
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
        if (isSubState_894F_2(st) != 0 || isSubState_894F_4or6(st) != 0) {
            st->requestState_6135 = 225;
            break;
        }
        return -state;
    case 30:
        st->requestState_6135 = state + 5;
        sendReqTicket(self);
        break;
    case 40:
        if (isSubState_894F_6(st) != 0) {
            st->requestState_6135 = 250;
            break;
        }
        st->requestState_6135 += 5;
        sendReqOpcode1B(self, 1, 6);
        break;
    case 50:
        if (st->userRowCount_8BB4 > 0) {
            u32 total;
            u32 rows;

            st->userRows_8BB8 = createStack(st, st->userRowCount_8BB4 * 92, &total);
            rows = total / 92;
            if (st->userRowCount_8BB4 > rows) {
                st->userRowCount_8BB4 = rows;
                growStackSize(st, total - rows * 92);
            }
            st->requestState_6135 += 5;
            sendReqUserListData(self, 1, st->userRowCount_8BB4);
            break;
        }
        st->requestState_6135 = state + 10;
        break;
    case 60:
        st->requestState_6135 = state + 5;
        sendReqOpcode1F(self);
        break;
    case 70:
    {
        NetworkUserRow* rows;
        u32 count;
        u32 i;

        count = st->userRowCount_8BB4;
        if ((s32)count <= 0) {
            return -(s32)(state + 1);
        }
        rows = (NetworkUserRow*)st->userRows_8BB8;
        for (i = 0; i < count; i++) {
            if (rows[i].shortName_04[0] == 0) {
                return -(s32)state;
            }
        }
        st->requestState_6135 += 10;
        break;
    }
    case 80:
        st->requestState_6135 = state + 5;
        sendReqServerTime(self);
        break;
    case 90:
        dispatchSessionHandlers(st, 3, 0, 0, st->userRowCount_8BB4, st->userRows_8BB8);
        return 1;
    case 100:
        st->loginInfoSent_82B4 = 1;
        st->requestState_6135 = state + 5;
        sendReqChargeInfo(self, 1);
        break;
    case 110:
        setConnectionPaths((NetworkInstance*)getInstance(), st->userIdText_82D5,
                           st->userPasswordText_8301);
        setMediatorState68A((NetworkInstance*)getInstance(), 0);
        st->requestState_6135 = 10;
        break;
    case 120:
        st->loginInfoSent_82B4 = 3;
        setMediatorState68A((NetworkInstance*)getInstance(), 0);
        st->requestState_6135 = 10;
        break;
    case 230:
        if (isSubState_894F_2(st) != 0) {
            st->requestState_6135 = 30;
            break;
        }
        if (isSubState_894F_6(st) != 0) {
            st->requestState_6135 += 5;
            sendReqRfpConnect(self);
            break;
        }
        st->requestState_6135 += 20;
        break;
    case 240:
        if (isSubState_894F_6(st) != 0) {
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
    NetworkFmpSlot* slots = st->fmpSlots_6C40;
    u8 state = st->requestState_6135;

    switch (state) {
    case 5:
        if (st->fmpPhase_894D != 2) {
            return -(s32)(state + 1);
        }
        if (st->loginInfoSent_82B4 != 0) {
            u8 mediatorState;

            getMediatorState68A((NetworkInstance*)getInstance(), &mediatorState);
            if (mediatorState == 1) {
                st->requestState_6135 = 100;
                break;
            }
            st->requestState_6135 += 5;
        } else {
            st->requestState_6135 = state + 5;
        }
        /* falls through */
    case 10:
    {
        NetworkUserRow* rows;
        u32 count;
        u32 i;

        count = st->userRowCount_8BB4;
        rows = (NetworkUserRow*)st->userRows_8BB8;
        for (i = 0; i < count; i++) {
            if (rows[i].id_00 == st->userRow_8B54.id_00) {
                break;
            }
        }
        if (i >= count) {
            return -(s32)state;
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
        if (st->sessionReady_8950 != 1) {
            return -(s32)state;
        }
        st->requestState_6135 = state + 5;
        sendReqTicket(self);
        break;
    case 30:
        st->requestState_6135 = state + 5;
        resetNetworkState4(self);
        break;
    case 35:
    {
        s32 result = handleNetworkState4(self, 80);

        if (result > 0) {
            if (st->fmpSlotCount_6608 <= 0) {
                return -(s32)state;
            }
            st->requestState_6135 += 5;
            break;
        }
        if (result < 0) {
            return -(s32)(state + result);
        }
        break;
    }
    case 40:
    {
        u32 bestIndex = (u32)-1;
        s32 bestValue = -1;
        s32 secondValue = -1;
        u32 i;

        st->fmpQueryValue_8040 = (u32)-1;
        for (i = 0; i < st->fmpSlotCount_6608; i++) {
            u32 remaining;

            if ((s32)slots[i].total_14 <= 0) {
                continue;
            }
            remaining = slots[i].total_14 - slots[i].done_10;
            if (bestValue > (s32)remaining) {
                if (secondValue <= (s32)remaining) {
                    secondValue = (s32)remaining;
                    st->fmpQueryValue_8040 = i;
                }
            } else {
                secondValue = bestValue;
                st->fmpQueryValue_8040 = bestIndex;
                bestValue = (s32)remaining;
                bestIndex = i;
            }
        }
        if (st->fmpQueryValue_8040 == (u32)-1) {
            st->requestState_6135 += 20;
            break;
        }
        st->fmpSelected_65F8 = st->fmpQueryValue_8040;
        st->requestState_6135 += 5;
        sendReqFmpInfo(self, slots[st->fmpSelected_65F8].payload_00, 1);
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

        for (i = 0; i < st->fmpSlotCount_6608; i++) {
            u32 remaining;

            if ((s32)slots[i].total_14 <= 0) {
                continue;
            }
            remaining = slots[i].total_14 - slots[i].done_10;
            if (bestValue <= (s32)remaining) {
                bestValue = (s32)remaining;
                bestIndex = i;
            }
        }
        if (bestIndex < 0) {
            return -(s32)state;
        }
        st->fmpSelected_65F8 = (u32)bestIndex;
        st->requestState_6135 += 5;
        sendReqFmpInfo(self, slots[bestIndex].payload_00, 1);
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
        st->requestState_6135 = state + 5;
        sendReqChargeInfo(self, 2);
        break;
    case 110:
        setConnectionPaths((NetworkInstance*)getInstance(), st->userIdText_82D5,
                           st->userPasswordText_8301);
        st->requestState_6135 = 200;
        break;
    case 200:
        st->requestState_6135 = state + 5;
        sendReqLoginInfo(self);
        break;
    case 210:
        if (st->connectionPhase_894F == 1) {
            st->requestState_6135 = 10;
            break;
        }
        return -(s32)state;
    default:
        break;
    }
    return 0;
}

/* Drives the block-2 binary/circle sub-machine. */
s32 handleNetworkState2Binary(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    u8 state = st->requestState_6135;

    switch (state) {
    case 5:
        if (st->fmpPhase_894D == 5) {
            st->binaryState_6134 = 90;
            st->requestState_6135 = 0;
            st->flag_6558 = 0;
            st->patPhase_894E = st->fmpPhase_894D;
            break;
        }
        st->fmpQueryValue_8040 = (u32)-1;
        if (st->fmpPhase_894D == 3) {
            st->shutdownFlag_6559 = 1;
            st->requestState_6135 += 5;
            break;
        }
        return -state;
    case 10:
        st->requestState_6135 = state + 5;
        sendReqServerTime(self);
        break;
    case 20:
        if (isCallback(self, 3) != 0) {
            st->requestState_6135 += 5;
            sendReqCircleInfoNoticeSet(self);
            break;
        }
        st->requestState_6135 += 10;
        break;
    case 30:
        if (st->binaryActive_D400 != 0) {
            st->requestState_6135 += 5;
            sendReqBinaryHead(self, st->binaryActive_D400, 6);
            break;
        }
        st->requestState_6135 = 70;
        break;
    case 40:
        if (st->binaryTextReady_D408 != 0 && st->dataTotal_825C != 0) {
            st->requestState_6135 += 5;
            sendReqBinaryData(self, st->binaryActive_D400, st->binarySize_D404, 0);
            break;
        }
        st->requestState_6135 += 10;
        break;
    case 50:
        st->requestState_6135 += 5;
        sendReqBinaryFoot(self, st->binaryActive_D400);
        break;
    case 60:
        if (st->binaryTextReady_D408 != 0) {
            u32 i = 0;
            u32 index = 0;
            u32 j;
            u32 k;

            memset(st->binaryTokens_D60C, 0, sizeof(st->binaryTokens_D60C));
            for (j = 0; j < 8; j++) {
                while (st->binaryText_D409[i] != '\t' && st->binaryText_D409[i] != 0) {
                    i++;
                }
                if (st->binaryText_D409[i] == 0) {
                    break;
                }
                i++;
                while (st->binaryText_D409[i] == '\t') {
                    i++;
                }
                if (st->binaryText_D409[i] == '\r' || st->binaryText_D409[i] == '\n') {
                    st->binaryText_D409[i] = 0;
                }
                if (st->binaryText_D409[i] == 0) {
                    break;
                }
                st->binaryTokens_D60C[index] = i;
                index++;
                i++;
                while (st->binaryText_D409[i] != '\r' && st->binaryText_D409[i] != '\n' &&
                       st->binaryText_D409[i] != 0) {
                    i++;
                }
                while (st->binaryText_D409[i] == '\r' || st->binaryText_D409[i] == '\n') {
                    st->binaryText_D409[i] = 0;
                    i++;
                }
                if (st->binaryText_D409[i] == 0) {
                    break;
                }
            }
            for (k = index; k < 8; k++) {
                st->binaryTokens_D60C[k] = i;
            }
        }
        st->requestState_6135 += 10;
        break;
    case 70:
        st->requestState_6135 += 5;
        reqUserSearchInfoMine(self, 0);
        break;
    case 80:
        st->requestState_6135 = 0;
        return 1;
    default:
        break;
    }
    return 0;
}

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
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    st->shutdownFlag_6559 = 0;
    st->shutdownMode_6136 = mode;
    ret = flushBuffer(st, 4, 0);
    writeUInt8(st, mode);
    encryptBuffer(st);
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
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    SessionTimeoutPayload payload;
    s32 ret;

    payload.code_00 = sessionTimeoutParam;
    payload.reason_02 = sessionTimeoutParam2;
    ret = flushBuffer(st, 17, 0);
    putItemTaggedLongs(st, values, 3, (const u8*)&payload);
    encryptBuffer(st);
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
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    u8 block[8];
    u32 count = 0;
    s32 ret;

    ret = flushBuffer(st, 20, 0);
    writeUInt8Array(st, data, size);
    if (tags != 0) {
        if (tags[0] != 0) {
            count = 1;
            block[0] = 1;
        }
        if (tags[1] != 0) {
            block[count] = 2;
            count++;
        }
    }
    putItemTaggedBytes(st, tags, count, block);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqLoginInfo(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    u8 block[4];
    u32 count = 0;
    s32 ret;

    ret = flushBuffer(st, 23, 0);
    if (st->loginInfoSent_82B4 == 0) {
        block[0] = 7;
        block[1] = 8;
        count = 3;
        block[2] = 9;
    } else if (st->loginInfoSent_82B4 - 1 <= 1) {
        block[0] = 6;
        count = 2;
        block[1] = 9;
    } else if (st->loginInfoSent_82B4 == 3) {
        block[0] = 10;
        count = 2;
        block[1] = 9;
    }
    putItemAny(st, st->loginFields_82AC, count, block);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqChargeInfo(NetworkInstance* self, s32 mode)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 25, 0);
    writeUInt8(st, mode);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqOpcode1B(NetworkInstance* self, u32 a, u32 b)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    u32 header[2];
    s32 ret;

    header[0] = requestHeaderWord0;
    header[1] = requestHeaderWord1;
    ret = flushBuffer(st, 27, 0);
    writeUInt32Shared(st, a);
    writeUInt32Shared(st, b);
    writeUInt8Array2(st, 8, (const u8*)header);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqUserListData(NetworkInstance* self, s32 mode, u32 count)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 29, 0);
    writeUInt32Shared(st, mode);
    writeUInt32Shared(st, count);
    encryptBuffer(st);
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
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    NetworkUserRow* found;
    u8 tags[16];
    u32 count = 0;
    s32 flag;
    s32 ret;

    if (index < 0 || index >= (s32)st->userRowCount_8BB4) {
        NetworkPostedError info;

        info.code_00 = -2147483648;
        info.param1_04 = 0;
        info.param2_08 = 0;
        info.reported_0C = 0;
        ((NetworkInstance*)self)->vtable->postError_288(self, (NetworkErrorInfo*)&info);
        return -1;
    }
    found = (NetworkUserRow*)(st->userRows_8BB8 + index * 92);
    if (strcmp(row->accountName_0C, found->accountName_0C) != 0) {
        count = 1;
        tags[0] = 3;
    }
    if (row->field_30 != found->field_30) {
        tags[count] = 5;
        count++;
    }
    if (row->field_2C != found->field_2C) {
        tags[count] = 4;
        count++;
    }
    if (row->field_34 != found->field_34) {
        tags[count] = 6;
        count++;
    }
    if (row->field_38 != found->field_38) {
        tags[count] = 7;
        count++;
    }
    if (strcmp(row->playerName_3C, found->playerName_3C) != 0) {
        tags[count] = 8;
        count++;
    }
    if (strcmp(found->shortName_04, maskedUserName) == 0) {
        flag = 1;
    } else {
        flag = count == 0 ? 2 : 3;
    }
    ret = flushBuffer(st, 33, 0);
    writeBool(st, flag);
    writeUInt32Shared(st, (u32)(index + 1));
    putUserSlotObjects(st, (const u8*)row, count, tags);
    encryptBuffer(st);
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
    u8 state = st->fmpState_6137;

    switch (state) {
    case 0:
        if (st->sessionMode_65F0 != 0 && st->sessionMode_65F0 != 1) {
            return -state;
        }
        st->fmpState_6137 += 5;
        sendReqFmpListVersion(self);
        break;
    case 10:
        if (st->fmpListActive_6C38 == 0) {
            return -state;
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
        if (st->replyTotal_8BD0 > 0) {
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
        sendReqFmpListData(self, finished + 1, count < 30 ? count : 30);
        break;
    }
    case 35:
        if (st->replySent_8BCC < 80 && st->replyTotal_8BD0 - st->replySent_8BCC > 0) {
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
        st->fmpSelected_65F8 = getFmpSlotIndex(st, st->fmpQueryValue_8040);
        return 1;
    default:
        break;
    }
    return 0;
}
