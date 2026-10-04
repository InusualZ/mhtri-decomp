/*
 * include/Network/network_state.h - the two NetworkSessionManager entry points `Network/network_state.cpp`
 * owns, declared here so consumers include the owner's header (docs/plan.md 6.5 rule 2).
 *
 * `sendReqShut` (0x803FFDCC, opcode 0x04) and `resetNetworkState3` (0x803FE924) are the two the
 * GameSpy band (`Network/GameSpyInterfaceThread.cpp`) calls; every other symbol the range owns is declared and
 * defined in `src/Network/network_state.cpp` itself.  The `NetworkInstance` layout lives in
 * `include/unsplit/Network.h` (a shared type, not an owned symbol).
 *
 * The `NetworkStateMachine`/`NetworkUserRow` views below and the unsplit callees this unit needs are
 * declared here rather than in the band header - the same convention `include/Network/NetworkSessionManager.h`
 * follows for its own range, because `include/unsplit/Network.h` belongs to the sibling mediator lane.
 */
#ifndef NETWORK_STATE_H
#define NETWORK_STATE_H

#include "types.h"
#include "unsplit/Network.h"

/* ---- the state machine's views of the network session object ----------------------------------- */

/* One 0x5C-byte user row: the local row at +0x8B54 and each row of the array `+0x8BB8` points at.
 * `sendReqUserObject` compares one against the other field by field. */
typedef struct NetworkUserRow {
    /* +0x00 */ u32  id_00;
    /* +0x04 */ char shortName_04[0x08];
    /* +0x0C */ char accountName_0C[0x20];
    /* +0x2C */ u32  field_2C;
    /* +0x30 */ u32  field_30;
    /* +0x34 */ u32  field_34;
    /* +0x38 */ u32  field_38;
    /* +0x3C */ char playerName_3C[0x20];
} NetworkUserRow;   /* size: 0x5C */

/* One 0x40-byte row of the FMP slot table at +0x6C40 (80 rows, cleared by `handleNetworkState4`). */
typedef struct NetworkFmpSlot {
    /* +0x00 */ u32 payload_00;
    /* +0x04 */ u8  pad_04[0x0C];
    /* +0x10 */ u32 done_10;
    /* +0x14 */ u32 total_14;
    /* +0x18 */ u8  pad_18[0x28];
} NetworkFmpSlot;   /* size: 0x40 */

/* The network session object as the request state machine addresses it.  Only the fields this unit's
 * functions touch are named; every gap is padding that keeps the offsets exact.  The object itself is
 * 0x16D08 bytes (`initNetworkSessionStable` allocates it); this view stops at the last field the unit
 * writes. */
typedef struct NetworkStateMachine {
    /* +0x0000 */ u8 pad_0000[0x60E8];
    /* +0x60E8 */ u32 handlersArmed_60E8;   /* gate for the FMP hand-off dispatch */
    /* +0x60EC */ u8 pad_60EC[0x0044];
    /* +0x6130 */ u8 sessionArmed_6130;   /* block-1 latch set by `resetNetworkState` */
    /* +0x6131 */ u8 termsArmed_6131;   /* block-3 latch set by `resetNetworkState3` */
    /* +0x6132 */ u8 sessionState_6132;   /* `handleNetworkState1`'s switch value */
    /* +0x6133 */ u8 subState_6133;   /* `advanceNetworkState5`'s gated state (== 5) */
    /* +0x6134 */ u8 binaryState_6134;   /* 10 selects the maintenance-reject path */
    /* +0x6135 */ u8 requestState_6135;   /* the block-2 sub-machine's switch value */
    /* +0x6136 */ u8 shutdownMode_6136;   /* the mode `sendReqShut` records */
    /* +0x6137 */ u8 fmpState_6137;   /* `handleNetworkState4`'s switch value */
    /* +0x6138 */ u8 pad_6138[0x0420];
    /* +0x6558 */ u8 flag_6558;
    /* +0x6559 */ u8 shutdownFlag_6559;
    /* +0x655A */ u8 pad_655A[0x02];
    /* +0x655C */ u32 loginDataSize_655C;   /* the login body's total */
    /* +0x6560 */ u32 loginDataCursor_6560;
    /* +0x6564 */ u32 loginDataPtr_6564;
    /* +0x6568 */ u8 pad_6568[0x0088];
    /* +0x65F0 */ u32 sessionMode_65F0;   /* 0/1 gate for the FMP list version request */
    /* +0x65F4 */ u8 pad_65F4[0x04];
    /* +0x65F8 */ u32 fmpSelected_65F8;   /* the chosen FMP slot index */
    /* +0x65FC */ u8 pad_65FC[0x000C];
    /* +0x6608 */ u32 fmpSlotCount_6608;   /* number of live rows in `fmpSlots_6C40` */
    /* +0x660C */ u8 pad_660C[0x010E];
    /* +0x671A */ u8 fmpReserve_671A[0x0106];
    /* +0x6820 */ u8 pad_6820[0x0418];
    /* +0x6C38 */ u32 fmpListActive_6C38;
    /* +0x6C3C */ u8 fmpListReady_6C3C;
    /* +0x6C3D */ u8 pad_6C3D[0x03];
    /* +0x6C40 */ NetworkFmpSlot fmpSlots_6C40[80];
    /* +0x8040 */ u32 fmpSelected_8040;     /* the FMP slot the query settled on (`getFmpSelected`) */
    /* +0x8044 */ u32 fmpQueryValue_8044;   /* the FMP list query argument / result */
    /* +0x8048 */ u8 fmpReply_8048[0x0106];
    /* +0x814E */ u8 pad_814E[0x0106];
    /* +0x8254 */ u8 patState_8254;   /* == 3 means the PAT handshake is up */
    /* +0x8255 */ u8 pad_8255[0x03];
    /* +0x8258 */ u32 sendSlice_8258;   /* the slice descriptor handed to `sendReqVulgarity*` */
    /* +0x825C */ u32 dataTotal_825C;   /* body bytes to send, decremented on completion */
    /* +0x8260 */ u32 dataSent_8260;   /* body bytes already sent */
    /* +0x8264 */ u32 termsBuffer_8264;
    /* +0x8268 */ u8 termsReady_8268;
    /* +0x8269 */ u8 pad_8269[0x03];
    /* +0x826C */ u32 termsSize_826C;
    /* +0x8270 */ u32 announcePending_8270;
    /* +0x8274 */ u32 serverInfoPending_8274;
    /* +0x8278 */ u32 chargePending_8278;
    /* +0x827C */ u8 pad_827C[0x04];
    /* +0x8280 */ u32 userListSize_8280;
    /* +0x8284 */ u32 vulgaritySize_8284;
    /* +0x8288 */ u8 pad_8288[0x04];
    /* +0x828C */ u8* termsBufferPtr_828C;
    /* +0x8290 */ u8* announceBufferPtr_8290;
    /* +0x8294 */ u8* serverInfoPtr_8294;
    /* +0x8298 */ u8 pad_8298[0x08];
    /* +0x82A0 */ u8* userListPtr_82A0;
    /* +0x82A4 */ u8* vulgarityPtr_82A4;
    /* +0x82A8 */ u8 pad_82A8[0x04];
    /* +0x82AC */ u8 loginFields_82AC[0x08];   /* the tag/value block `sendReqLoginInfo` builds */
    /* +0x82B4 */ u8 loginInfoSent_82B4;
    /* +0x82B5 */ u8 pad_82B5[0x0020];
    /* +0x82D5 */ char userIdText_82D5[0x2C];
    /* +0x8301 */ char userPasswordText_8301[0x2C];
    /* +0x832D */ u8 pad_832D[0x0620];
    /* +0x894D */ u8 fmpPhase_894D;   /* the FMP sub-machine's phase (== 1/== 2/== 3/== 5) */
    /* +0x894E */ u8 patPhase_894E;
    /* +0x894F */ u8 connectionPhase_894F;   /* the connection sub-machine's phase (2..6) */
    /* +0x8950 */ u8 sessionReady_8950;
    /* +0x8951 */ char termText_8951[0x2C];   /* the terms reply, truncated into `replyBuffer_8BC8` */
    /* +0x897D */ u8 pad_897D[0x01D7];
    /* +0x8B54 */ NetworkUserRow userRow_8B54;   /* this client's own row */
    /* +0x8BB0 */ u8 pad_8BB0[0x04];
    /* +0x8BB4 */ u32 userRowCount_8BB4;   /* rows in `userRows_8BB8`, 92 bytes each */
    /* +0x8BB8 */ u8* userRows_8BB8;
    /* +0x8BBC */ u8 pad_8BBC[0x08];
    /* +0x8BC4 */ u32 replySize_8BC4;   /* the reply buffer's capacity */
    /* +0x8BC8 */ u8* replyBuffer_8BC8;
    /* +0x8BCC */ u32 replySent_8BCC;
    /* +0x8BD0 */ u32 replyTotal_8BD0;
    /* +0x8BD4 */ u8 pad_8BD4[0x482C];
    /* +0xD400 */ u8 binaryActive_D400;   /* the circle/binary sub-machine is armed */
    /* +0xD401 */ u8 pad_D401[0x03];
    /* +0xD404 */ u32 binarySize_D404;
    /* +0xD408 */ u8 binaryTextReady_D408;
    /* +0xD409 */ char binaryText_D409[0x0203];   /* the reply, split into tab-separated tokens */
    /* +0xD60C */ u32 binaryTokens_D60C[8];   /* token start offsets inside `binaryText_D409` */
} NetworkStateMachine;   /* size: 0xD62C */

/* The 3-byte payload `sendServerTimeout` builds from the two unowned constants. */
typedef struct SessionTimeoutPayload {
    /* +0x00 */ u16 code_00;
    /* +0x02 */ u8  reason_02;
    /* +0x03 */ u8  pad_03;
} SessionTimeoutPayload;   /* size: 0x04 */

/* The DWC error record `NetworkInstanceVtable::postError_288` takes (0x10 B; `Network/GameSpyInterfaceThread.h`
 * owns the named definition, this unit builds an equivalent record and casts). */
/* `NetworkPostedError` (0x0C B) is defined in `unsplit/Network.h`, beside the class that dispatches it. */

extern "C" {

s32  sendReqShut(NetworkInstance* self, s32 mode);
s32  resetNetworkState3(NetworkInstance* self);
/* 0x803FE8E4 / 0x803FF024 - the state machine's reset and its gated step out of state 5. */
s32  resetNetworkState(NetworkInstance* self);
s32  advanceNetworkState5(NetworkInstance* self);
/* 0x803FE95C - one step of the login sub-machine; nonzero once it has finished. */
s32  handleNetworkState1(NetworkInstance* self);

/* The callees this unit drives are declared in their owners' headers, which `network_state.cpp`
 * includes (rule 2): the request writers in `Network/NetworkCommunityPat.h`, the item writers, the
 * hand-off dispatch and the request emitters in `Network/network_layer_io.h`, the state predicates, the
 * maintenance queries and the call-stack helpers in `Network/PatInterface.h`, the NAS token and the
 * connection paths in `Network/NetworkWiiMediator.h`, and the mediator state accessors in
 * `Network/NetworkWiiMediator.h`. */

/* `getInstance` (0x800E89D8) is owned by `src/sound/fn_800E46E8.cpp`; its header declares it. */
#include "sound/fn_800E46E8.h"

/* MSL primitive with no registered owner (rule 2's unsplit gap, the same home as `strlen` above). */
int strcmp(const char* a, const char* b);

/* The band's request-header constants (no registered owner; declared, never defined - playbook 29).
 * `sessionTimeoutParam`/`Param2` are the bytes {1,2,3}, `requestHeaderWord0`/`Word1` the 8-byte block
 * {1,2,3,5,4,6,7,8} `sendReqOpcode1B` copies, `maskedUserName` the "******" sentinel
 * `sendReqUserObject` compares a row's short name against. */
extern const u16 sessionTimeoutParam;      /* 0x8079C7D8 */
extern const u8  sessionTimeoutParam2;     /* 0x8079C7DA */
extern const u32 requestHeaderWord0;       /* 0x8079C7E0 */
extern const u32 requestHeaderWord1;       /* 0x8079C7E4 */
extern const char maskedUserName[7];       /* 0x80793968 - the map's own size, so MWCC uses sda21 */

/* 0x803FFE88 - sends the server-timeout request built from the three words at `values`. */
s32 sendServerTimeout(NetworkInstance* self, const u32* values);

/* 0x803FF060 */
s32 handleNetworkState2(NetworkInstance* self);
/* 0x803FF4EC */
s32 handleNetworkState2Fmp(NetworkInstance* self);
/* 0x803FF994 */
s32 handleNetworkState2Binary(NetworkInstance* self);

/* 0x803FFF50 - sends the check request: two tag bytes and a packed record of `size` bytes. */
s32 sendReqUnknownCheck(NetworkInstance* self, const u8* tags, const u8* data, u32 size);
}

#endif /* NETWORK_STATE_H */
