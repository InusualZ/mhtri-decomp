/*
 * include/Network/fn_8041A87C.h - the types and externs the 0x8041A87C Network band needs.
 *
 * Everything here is reconstructed from the range's own disassembly (every offset is one the target
 * instructions address) plus the string pool at `.data` 0x80603000..0x806036xx, which names the two
 * classes: `NetworkGameSpyInterface::` and `NetworkPeerGameSpy::`.  The declarations of the
 * neighbouring `fn_`/SDK helpers the band calls have no registered owner and live in
 * `include/unsplit/Network.h`; `fn_803D6A98` is declared there as `void*` because its result is this
 * unit's own thread object, so the casts stay at the call sites.
 */

#ifndef FN_8041A87C_H
#define FN_8041A87C_H

#include "types.h"
#include "unsplit/Network.h"

/* --------------------------------------------------------------------------------------------- */
/* GameSpy interface state machine - `NetworkGameSpyInterface`                                    */
/* --------------------------------------------------------------------------------------------- */

typedef struct GameSpyChannel {
    /* +0x00 */ u8  ownerId_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 peerId_04;
    /* +0x08 */ u8  address_08[0x1F];
    /* +0x27 */ u8  tail_27;
} GameSpyChannel;   /* size: 0x28 */

typedef struct NetworkGameSpyInterface {
    /* +0x0000 */ u8  pad_00[0x0C];
    /* +0x000C */ u32 flags_0C;
    /* +0x0010 */ s32 task_10;
    /* +0x0014 */ u8  searchStep_14;
    /* +0x0015 */ u8  connectStep_15;
    /* +0x0016 */ u8  pad_16;
    /* +0x0017 */ u8  callbackStep_17;
    /* +0x0018 */ u8  channel_18;
    /* +0x0019 */ u8  pad_19[0x03];
    /* +0x001C */ u32 peerId_1C;
    /* +0x0020 */ u8  recvArea_20[0x8000];
    /* +0x8020 */ s32 channelCount_8020;
    /* +0x8024 */ GameSpyChannel channels_8024[8];
    /* +0x8164 */ u32 limit_8164;
    /* +0x8168 */ u32 writePos_8168;
} NetworkGameSpyInterface;   /* size: 0x816C (approximation: the range addresses up to +0x8168) */

/* --------------------------------------------------------------------------------------------- */
/* `NetworkGameSpyInterface`'s worker thread object                                               */
/* --------------------------------------------------------------------------------------------- */

typedef struct NetworkErrorInfo {
    /* +0x00 */ s32 code_00;
    /* +0x04 */ s32 param1_04;
    /* +0x08 */ s32 param2_08;
    /* +0x0C */ s32 reported_0C;
} NetworkErrorInfo;   /* size: 0x10 */

typedef struct GameSpyInterfaceThread {
    /* +0x0000 */ void* vtable_00;
    /* +0x0004 */ s32 errorCode_04;
    /* +0x0008 */ s32 errorParam1_08;
    /* +0x000C */ s32 errorParam2_0C;
    /* +0x0010 */ u8  errorReported_10;
    /* +0x0011 */ u8  pad_11[0x03];
    /* +0x0014 */ u32 receivers_14[4];
    /* +0x0024 */ u8  receiverState_24[4];
    /* +0x0028 */ u8  receiverCount_28;
    /* +0x0029 */ u8  pad_29[0x03];
    /* +0x002C */ s32 state_2C;
    /* +0x0030 */ u32 value_30;
    /* +0x0034 */ u32 receiverIds_34[4];
    /* +0x0044 */ u32 slotIds_44[4];
    /* +0x0054 */ u8  slotState_54[4];
    /* +0x0058 */ u32 slotHandles_58[4];
    /* +0x0068 */ s32 field_68;
    /* +0x006C */ s32 running_6C;
    /* +0x0070 */ s32 frame_70;
    /* +0x0074 */ s32 phase_74;
    /* +0x0078 */ u8  flag_78;
    /* +0x0079 */ u8  pad_79[0x03];
    /* +0x007C */ s32 paramA_7C;
    /* +0x0080 */ u32 paramB_80;
    /* +0x0084 */ s32 paramC_84;
    /* +0x0088 */ s32 paramD_88;
    /* +0x008C */ s32 paramE_8C;
    /* +0x0090 */ s32 result_90;
    /* +0x0094 */ s32 handle_94;
    /* +0x0098 */ s32 stage_98;
    /* +0x009C */ char name_9C[0x80];
    /* +0x011C */ s8  idByte_11C;
    /* +0x011D */ u8  idByte_11D;
    /* +0x011E */ u8  idByte_11E;
    /* +0x011F */ u8  idByte_11F;
    /* +0x0120 */ u8  started_120;
    /* +0x0121 */ u8  stopRequested_121;
    /* +0x0122 */ u8  cancelPending_122;
    /* +0x0123 */ u8  closePending_123;
    /* +0x0124 */ u8  initRequested_124;
    /* +0x0125 */ u8  openRequested_125;
    /* +0x0126 */ u8  stepRequested_126;
    /* +0x0127 */ u8  pad_127;
    /* +0x0128 */ u32 busy_128;
    /* +0x012C */ u8  pad_12C[0x04];
    /* +0x0130 */ u8  thread_130[0x1E50];
    /* +0x4448 */ u32 threadParam_4448;
    /* +0x444C */ u8  mutexReady_444C;
    /* +0x444D */ u8  pad_444D[0x03];
    /* +0x4450 */ u8  mutex_4450[0x18];
    /* +0x4468 */ s32 field_4468;
    /* +0x446C */ u8  negotiation_446C;
    /* +0x446D */ u8  negotiationResult_446D;
    /* +0x446E */ s8  negotiationDone_446E;
    /* +0x446F */ u8  pad_446F;
    /* +0x4470 */ u32 peerId_4470;
    /* +0x4474 */ u32 selfPeerId_4474;
    /* +0x4478 */ s32 peerMatch_4478;
    /* +0x447C */ u32 session_447C;
    /* +0x4480 */ s8  negotiationStep_4480;
    /* +0x4481 */ u8  pad_4481;
    /* +0x4482 */ s16 bufferSize_4482;
    /* +0x4484 */ u8  sessionOpen_4484;
    /* +0x4485 */ u8  profile_4485[0x14];
} GameSpyInterfaceThread;   /* size: 0x8450 (approximation: the 0x4000-byte stack at +0x4448 is the upper bound) */

/* --------------------------------------------------------------------------------------------- */
/* the small timed handler whose ctor sits at the end of the range                                */
/* --------------------------------------------------------------------------------------------- */

typedef struct NetworkTimedHandler {
    /* +0x00 */ void* vtable_00;
    /* +0x04 */ s32 state_04;
    /* +0x08 */ s32 interval_08;
    /* +0x0C */ s32 limit_0C;
    /* +0x10 */ u8  ready_10;
    /* +0x11 */ u8  pad_11[0x03];
    /* +0x14 */ s32 timeout_14;
    /* +0x18 */ u8  expired_18;
    /* +0x19 */ u8  pad_19[0x03];
} NetworkTimedHandler;   /* size: 0x1C */

/* --------------------------------------------------------------------------------------------- */
/* `NetworkPeerGameSpy` - the 0x600-byte send / 0x6000-byte receive buffer pair                   */
/* --------------------------------------------------------------------------------------------- */

typedef struct NetworkPeerGameSpy {
    /* +0x0000 */ void* vtable_00;
    /* +0x0004 */ u8  pad_04[0x0C];
    /* +0x0010 */ u32 received_10;
    /* +0x0014 */ u8  sendBuffer_14[0x600];
    /* +0x0614 */ u8  recvBuffer_614[0x6000];
    /* +0x6614 */ u8  mutex_6614[0x1C];
    /* +0x6630 */ s32 field_6630;
    /* +0x6634 */ GameSpyInterfaceThread* interface_6634;
    /* +0x6638 */ u32 peer_6638;
} NetworkPeerGameSpy;   /* size: 0x663C (approximation: the range addresses up to +0x6638) */

/* the DWC slot callback the peer installs (its vtable slot +0x18 takes the tail-call's five args) */
typedef struct GameSpyReceiverVtable {
    /* +0x00 */ u8 pad_00[0x18];
    /* +0x18 */ void (*handle_18)(void* self, s32 a, s32 b, s32 c, s32 d, s32 e);
} GameSpyReceiverVtable;   /* size: 0x1C */

typedef struct GameSpyReceiver {
    /* +0x00 */ GameSpyReceiverVtable* vtable;
} GameSpyReceiver;   /* size: 0x04 */

/* the vtable slot `fn_8041DD28` ticks, reached through the caller's pointer-to-object */
typedef struct NetworkPeerCallbackVtable {
    /* +0x00 */ u8 pad_00[0x28];
    /* +0x28 */ void (*tick_28)(void* self);
} NetworkPeerCallbackVtable;   /* size: 0x2C */

/* the 8-byte GameSpy header `fn_8041B270` builds on the stack */
typedef struct GameSpyHeader {
    /* +0x00 */ u8  type_0;
    /* +0x01 */ u8  code_1;
    /* +0x02 */ u16 peer_2;
    /* +0x04 */ u32 value_4;
} GameSpyHeader;   /* size: 0x08 */

/* an 8-byte GameSpy peer address */
typedef struct GameSpyAddress {
    /* +0x00 */ u8  first_00;
    /* +0x01 */ u8  second_01;
    /* +0x02 */ u16 port_02;
    /* +0x04 */ u32 value_04;
} GameSpyAddress;   /* size: 0x08 */

/* the result record `fn_8041B334` fills in for the DWC callback */
typedef struct GameSpyResultInfo {
    /* +0x00 */ u32 connected_00;
    /* +0x04 */ u32 result_04;
    /* +0x08 */ GameSpyAddress address_08;
} GameSpyResultInfo;   /* size: 0x10 */

/* the id pair `fn_8041D530` starts a NAT negotiation from */
typedef struct GameSpyPeerId {
    /* +0x00 */ u32 peerId_00;
    /* +0x04 */ u32 mode_04;
    /* +0x08 */ u32 session_08;
    /* +0x0C */ u16 port_0C;
    /* +0x0E */ u16 pad_0E;
} GameSpyPeerId;   /* size: 0x10 */

/* the peer thread's NAT-negotiation record (`lbl_806D3660`) */
typedef struct GameSpyNegotiation {
    /* +0x00 */ u32 active_00;
    /* +0x04 */ u32 result_04;
    /* +0x08 */ u16 value_08;
    /* +0x0A */ u16 encoded_0A;
    /* +0x0C */ u32 session_0C;
} GameSpyNegotiation;   /* size: 0x10 */

extern "C" GameSpyNegotiation lbl_806D3660;   /* .bss 0x806D3660 */

/* the two overlapping views of a DWC event message */
typedef struct GameSpyChannelMsg {
    /* +0x00 */ u8  channel_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 peerId_04;
    /* +0x08 */ u32 limit_08;
    /* +0x0C */ u8  count_0C;
    /* +0x0D */ u8  pad_0D[0x03];
    /* +0x10 */ const GameSpyChannel* channels_10;
} GameSpyChannelMsg;   /* size: 0x14 (approximation: only the leading words are addressed) */

typedef struct GameSpyDataMsg {
    /* +0x00 */ u8  channel_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 writePos_04;
    /* +0x08 */ u32 size_08;
    /* +0x0C */ void* data_0C;
} GameSpyDataMsg;   /* size: 0x10 (approximation: only the leading words are addressed) */

/* DWC hands the same payload to both event codes, so the record is read through either view */
typedef union GameSpyEventMsg {
    /* +0x00 */ GameSpyChannelMsg channelView;
    /* +0x00 */ GameSpyDataMsg dataView;
} GameSpyEventMsg;   /* size: 0x14 */

/* --------------------------------------------------------------------------------------------- */
/* Declarations of symbols a *registered* unit owns.  They sit here, not in that unit's header,
 * because the owners' headers do not carry them yet - the shared-file edit that would move them
 * (`include/Network/fn_803D3CE8.h`, a new `include/DWCi/fn_805113B0.h`, `include/sound/fn_800E46E8.h`)
 * is recorded in this lane's outbox.  The spellings are the owners' own definitions.
 * --------------------------------------------------------------------------------------------- */

extern "C" {

/* owner: src/Network/fn_803D3CE8.cpp - returns this unit's thread object, so it stays `void*` here */
void* fn_803D6A98(void);

/* owner: src/DWCi/fn_805113B0.c */
u16 fn_80512200(u16 port);
char* fn_80512210(u32 addr, u16 port, char* buf);

/* owner: src/sound/fn_800E46E8.cpp */
void* fn_800E89D8(void);

}

#endif
