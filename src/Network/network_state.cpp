/*
 * Network/network_state.cpp - the NetworkSessionManager state machine
 * (`.text` 0x803FE8E4..0x804006A8, 21 functions / 8900 B).
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
 * `handleNetworkStateN` dispatchers (1 is the 0x6C8-byte main machine), plus the request emitters
 * 0x803FFD74..0x804002C0 (`sendReqServerTime`/`sendReqShut`/`sendReqTicket`/`sendReqCommonKey`/
 * `sendReqLoginInfo`/`sendReqChargeInfo`/`sendReqUserListData`/`sendReqUserObject`).  C++ but every
 * function is `extern "C"` (the map's names are unmangled).
 *
 * NAMES.  The runtime dump's map ("D:/WiiExperiment/DumpSymbols.zip") names 14 of the 21 already in
 * `symbols.txt`.  The six helpers it leaves as `zz_` were named in this batch's naming pass from
 * their own code: `advanceNetworkState5` gates on the `+0x6133 == 5` state byte, and the three
 * block-2 sub-machine dispatchers are named after the request family each drives
 * (`handleNetworkState2` -> login/charge/ticket/user-list, `handleNetworkState2Fmp` -> `sendReqFmp*`,
 * `handleNetworkState2Binary` -> `sendReqBinary*`/circle).  `sendReqOpcode1B`/`sendReqOpcode1F` are
 * the two emitters the account machine calls, named after their `flushBuffer` request opcode.
 *
 * NAMING GUESSES.  The map gives all three block-2 dispatchers the same `handleNetworkState2`
 * spelling (three distinct local addresses), so that name is not usable for all three; the
 * `Fmp`/`Binary` suffixes are inferred from the request family each calls, not from a recovered API
 * name.  `sendReqOpcode1B`/`1F` record the wire opcode because the request name is unknown (the
 * 0x1B/0x1F ids sit in the gaps of the `sendReq*` opcode table).
 *
 * BODIES.  Three of 21 are reconstructed (`resetNetworkState`, `resetNetworkState3`,
 * `resetNetworkState4`); the rest are stubs - a registered, compiling, measured baseline.
 *
 * Every symbol this file defines is named; no `fn_` spelling survives in it (rule 7 clean, no
 * deferral needed).
 */
#include "types.h"
#include "unsplit/Network.h"
#include "Network/network_state.h"

extern "C" {

/* defined here */
s32 resetNetworkState(NetworkInstance* self);
s32  resetNetworkState3(NetworkInstance* self);
s32 resetNetworkState4(NetworkInstance* self);
void handleNetworkState1(NetworkInstance* self);
void handleNetworkState4(NetworkInstance* self);
void sendReqServerTime(NetworkInstance* self);
void sendReqTicket(NetworkInstance* self);
void sendReqCommonKey(NetworkInstance* self);
void sendReqLoginInfo(NetworkInstance* self);
void sendReqChargeInfo(NetworkInstance* self);
void sendReqUserListData(NetworkInstance* self);
void sendReqUserObject(NetworkInstance* self);
void sendReqShut(NetworkInstance* self, s32 mode);
void advanceNetworkState5(NetworkInstance* self);
void handleNetworkState2(NetworkInstance* self);
void handleNetworkState2Fmp(NetworkInstance* self);
void handleNetworkState2Binary(NetworkInstance* self);
void sendServerTimeout(NetworkInstance* self);
void sendReqUnknownCheck(NetworkInstance* self);
void sendReqOpcode1B(NetworkInstance* self);
void sendReqOpcode1F(NetworkInstance* self);

} /* extern "C" */

s32 resetNetworkState(NetworkInstance* self)
{
    u8* base = (u8*)self;

    if (base[0x6130] == 0) {
        base[0x6130] = 1;
        base[0x6132] = 0;
        base[0x6133] = 0;
        base[0x6135] = 0;
        base[0x6136] = 0;
        base[0x6558] = 0;
        base[0x6559] = 0;
        base[0x894E] = 0;
    }
    return 0;
}
s32 resetNetworkState3(NetworkInstance* self)
{
    u8* base = (u8*)self;

    if (base[0x6131] == 0) {
        base[0x6131] = 1;
        base[0x6132] = 0;
        base[0x6133] = 0;
        base[0x6136] = 0;
        base[0x6130] = 0;
        base[0x6558] = 0;
        base[0x6559] = 0;
    }
    return 0;
}
s32 resetNetworkState4(NetworkInstance* self)
{
    ((u8*)self)[0x6137] = 0;
    return 0;
}
void handleNetworkState1(NetworkInstance* self) { (void)self; }
void handleNetworkState4(NetworkInstance* self) { (void)self; }
void sendReqServerTime(NetworkInstance* self) { (void)self; }
void sendReqTicket(NetworkInstance* self) { (void)self; }
void sendReqCommonKey(NetworkInstance* self) { (void)self; }
void sendReqLoginInfo(NetworkInstance* self) { (void)self; }
void sendReqChargeInfo(NetworkInstance* self) { (void)self; }
void sendReqUserListData(NetworkInstance* self) { (void)self; }
void sendReqUserObject(NetworkInstance* self) { (void)self; }
void sendReqShut(NetworkInstance* self, s32 mode) { (void)self; (void)mode; }
void advanceNetworkState5(NetworkInstance* self) { (void)self; }
void handleNetworkState2(NetworkInstance* self) { (void)self; }
void handleNetworkState2Fmp(NetworkInstance* self) { (void)self; }
void handleNetworkState2Binary(NetworkInstance* self) { (void)self; }
void sendServerTimeout(NetworkInstance* self) { (void)self; }
void sendReqUnknownCheck(NetworkInstance* self) { (void)self; }
void sendReqOpcode1B(NetworkInstance* self) { (void)self; }
void sendReqOpcode1F(NetworkInstance* self) { (void)self; }
