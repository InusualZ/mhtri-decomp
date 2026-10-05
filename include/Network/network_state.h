/*
 * include/Network/network_state.h - the two NetworkSessionManager entry points `Network/network_state.cpp`
 * owns, declared here so consumers include the owner's header (docs/plan.md 6.5 rule 2).
 *
 * `sendReqShut` (0x803FFDCC, opcode 0x04) and `resetNetworkState3` (0x803FE924) are the two the
 * GameSpy band (`Network/GameSpyInterfaceThread.cpp`) calls; every other symbol the range owns is declared and
 * defined in `src/Network/network_state.cpp` itself.  The `NetworkInstance` layout lives in
 * `include/unsplit/Network.h` (a shared type, not an owned symbol).
 *
 * The unsplit callees this unit needs are declared here rather than in the band header - the same convention
 * `include/Network/NetworkSessionManager.h` follows for its own range.
 */
#ifndef NETWORK_STATE_H
#define NETWORK_STATE_H

#include "types.h"
#include "unsplit/Network.h"

/* ---- the state machine's view of the network session object ------------------------------------ */

/* `NetworkStateMachine` is a typedef of `PatInterface` (the singleton's class, `Network/PatInterface.h`, which
 * carries the state machine's fields), and `NetworkUserRow`/`NetworkFmpSlot` live beside it. */
#include "Network/PatInterface.h"

/* The 3-byte payload `sendServerTimeout` builds from the two unowned constants. */
typedef struct SessionTimeoutPayload {
    /* +0x00 */ u16 code_00;
    /* +0x02 */ u8  reason_02;
    /* +0x03 */ u8  pad_03;
} SessionTimeoutPayload;   /* size: 0x04 */

/* `NetworkPostedError` (0x0C B) is defined in `unsplit/Network.h`; `PatInterface::postError` takes it by value. */

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
