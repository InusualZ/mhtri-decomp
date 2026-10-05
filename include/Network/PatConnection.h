/*
 * include/Network/PatConnection.h - the declarations of `src/Network/PatConnection.cpp` (`.text` 0x803FAE9C..0x803FCC34): the
 * `PatConnection` class (the `PatInterface` base) and the session band's request writers.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_PATCONNECTION_H
#define MHTRI_NETWORK_PATCONNECTION_H

#include "types.h"
#include "Network/gamespy_interface_types.h"   /* NetworkErrorInfo - the error `postError` takes */

class PatInterface;                       /* include/Network/PatInterface.h */

/* The 8-byte header of one received packet (the connection keeps it at +0x60AA): `recvCommand` matches the three
 * op-code bytes against the packet table, a status of -1/1 selects `recvAnsNg`/`recvAnsAlert`, and the handlers hand
 * the request id on to the session handlers. */
typedef struct PatPacketHeader {
    /* +0x00 */ u8 size_00[2];
    /* +0x02 */ u8 requestId_02[2];   /* copied out with `memcpy` - the packet is a byte stream */
    /* +0x04 */ u8 opcode_04[3];
    /* +0x07 */ s8 status_07;         /* -1 negative reply, 1 alert, else the handler's own */
} PatPacketHeader;   /* size: 0x08 */

/* The Pat server connection, `PatInterface`'s base: the constructor 0x803FAE9C stores the table 0x806006E8, whose
 * slots are the destructor 0x803FAF34, `resetDefaults` 0x803FAF78 and 159 pure virtuals (null words +0x10..+0x288:
 * the packet handlers, `recvCommand` and `postError`, which `PatInterface` overrides).  The fields the bodies read
 * are named; the rest of the buffers is padding that keeps the offsets exact.  The virtuals are declared and the
 * key function (the destructor) is not defined here, so no table is emitted by a consumer (rule 10). */
class PatConnection {
public:
    /* 0x803FAE9C */
    PatConnection();
    /* +0x008 - 0x803FAF34 */
    virtual ~PatConnection();
    /* +0x00C - 0x803FAF78 - restores the connection's defaults */
    virtual void resetDefaults();
    /* +0x010 */ virtual s32 recvReqLineCheck(s32 index, const PatPacketHeader* header) = 0;
    /* +0x014 */ virtual s32 recvAnsServerTime(s32 index, const PatPacketHeader* header) = 0;
    /* +0x018 */ virtual s32 recvAnsShut(s32 index, const PatPacketHeader* header) = 0;
    /* +0x01C */ virtual s32 recvNtcShut(s32 index, const PatPacketHeader* header) = 0;
    /* +0x020 */ virtual s32 recvNtcRecconect(s32 index, const PatPacketHeader* header) = 0;
    /* +0x024 */ virtual s32 recvReqConnection(s32 index, const PatPacketHeader* header) = 0;
    /* +0x028 */ virtual s32 recvNtcLogin(s32 index, const PatPacketHeader* header) = 0;
    /* +0x02C */ virtual s32 recvAnsTicket(s32 index, const PatPacketHeader* header) = 0;
    /* +0x030 */ virtual s32 recvReqTicket(s32 index, const PatPacketHeader* header) = 0;
    /* +0x034 */ virtual s32 recvReqWarning(s32 index, const PatPacketHeader* header) = 0;
    /* +0x038 */ virtual s32 recvAnsCommonKey(s32 index, const PatPacketHeader* header) = 0;
    /* +0x03C */ virtual s32 recvReqMemoryCheck(s32 index, const PatPacketHeader* header) = 0;
    /* +0x040 */ virtual s32 recvAnsLoginInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x044 */ virtual s32 recvAnsChargeInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x048 */ virtual s32 recvAnsUserListHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x04C */ virtual s32 recvAnsUserListData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x050 */ virtual s32 recvAnsUserListFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x054 */ virtual s32 recvAnsUserObject(s32 index, const PatPacketHeader* header) = 0;
    /* +0x058 */ virtual s32 recvAnsFmpListVersion(s32 index, const PatPacketHeader* header) = 0;
    /* +0x05C */ virtual s32 recvAnsFmpListHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x060 */ virtual s32 recvAnsFmpListData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x064 */ virtual s32 recvAnsFmpListFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x068 */ virtual s32 recvAnsFmpInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x06C */ virtual s32 recvAnsRfpConnect(s32 index, const PatPacketHeader* header) = 0;
    /* +0x070 */ virtual s32 recvAnsLmpConnect(s32 index, const PatPacketHeader* header) = 0;
    /* +0x074 */ virtual s32 recvAnsTermsVersion(s32 index, const PatPacketHeader* header) = 0;
    /* +0x078 */ virtual s32 recvAnsTerms(s32 index, const PatPacketHeader* header) = 0;
    /* +0x07C */ virtual s32 recvAnsMaintenance(s32 index, const PatPacketHeader* header) = 0;
    /* +0x080 */ virtual s32 recvAnsAnnounce(s32 index, const PatPacketHeader* header) = 0;
    /* +0x084 */ virtual s32 recvAnsNoCharge(s32 index, const PatPacketHeader* header) = 0;
    /* +0x088 */ virtual s32 recvAnsMediaVersionInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x08C */ virtual s32 recvAnsVulgarityInfoHigh(s32 index, const PatPacketHeader* header) = 0;
    /* +0x090 */ virtual s32 recvAnsVulgarityHigh(s32 index, const PatPacketHeader* header) = 0;
    /* +0x094 */ virtual s32 recvAnsVulgarityInfoLow(s32 index, const PatPacketHeader* header) = 0;
    /* +0x098 */ virtual s32 recvAnsVulgarityLow(s32 index, const PatPacketHeader* header) = 0;
    /* +0x09C */ virtual s32 recvAnsAuthenticationToken(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0A0 */ virtual s32 recvAnsBinaryVersion(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0A4 */ virtual s32 recvAnsBinaryHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0A8 */ virtual s32 recvAnsBinaryData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0AC */ virtual s32 recvAnsBinarFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0B0 */ virtual s32 recvAnsLayerStart(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0B4 */ virtual s32 recvAnsLayerEnd(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0B8 */ virtual s32 recvNtcLayerUserNum(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0BC */ virtual s32 recvAnsLayerJump(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0C0 */ virtual s32 recvAnsLayerCreateHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0C4 */ virtual s32 recvAnsLayerCreateSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0C8 */ virtual s32 recvAnsLayerCreateFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0CC */ virtual s32 recvAnsLayerDown(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0D0 */ virtual s32 recvNtcLayerIn(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0D4 */ virtual s32 recvAnsLayerUp(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0D8 */ virtual s32 recvNtcLayerOut(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0DC */ virtual s32 recvNtcLayerJumpReady(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0E0 */ virtual s32 recvNtcLayerJumpGo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0E4 */ virtual s32 recvAnsLayerInfoSe(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0E8 */ virtual s32 recvNtcLayerInfoSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0EC */ virtual s32 recvAnsLayerInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0F0 */ virtual s32 recvAnsLayerParentInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0F4 */ virtual s32 recvAnsLayerChildInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0F8 */ virtual s32 recvAnsLayerChildListHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x0FC */ virtual s32 recvAnsLayerChildListData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x100 */ virtual s32 recvAnsLayerChildListFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x104 */ virtual s32 recvAnsLayerSiblingListHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x108 */ virtual s32 recvAnsLayerSiblingListData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x10C */ virtual s32 recvAnsLayerSiblingListFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x110 */ virtual s32 recvAnsLayerHost(s32 index, const PatPacketHeader* header) = 0;
    /* +0x114 */ virtual s32 recvNtcLayerHost(s32 index, const PatPacketHeader* header) = 0;
    /* +0x118 */ virtual s32 recvAnsLayerUserInfoSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x11C */ virtual s32 recvNtcLayerUserInfoSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x120 */ virtual s32 recvAnsLayerUserList(s32 index, const PatPacketHeader* header) = 0;
    /* +0x124 */ virtual s32 recvAnsLayerUserListHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x128 */ virtual s32 recvAnsLayerUserListData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x12C */ virtual s32 recvAnsLayerUserListFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x130 */ virtual s32 recvAnsLayerUserSearchHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x134 */ virtual s32 recvAnsLayerUserSearchData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x138 */ virtual s32 recvAnsLayerUserSearchFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x13C */ virtual s32 recvNtcLayerBinary(s32 index, const PatPacketHeader* header) = 0;
    /* +0x140 */ virtual s32 recvNtcLayerUserPosition(s32 index, const PatPacketHeader* header) = 0;
    /* +0x144 */ virtual s32 recvNtcLayerChat(s32 index, const PatPacketHeader* header) = 0;
    /* +0x148 */ virtual s32 recvAnsLayerTell(s32 index, const PatPacketHeader* header) = 0;
    /* +0x14C */ virtual s32 recvNtcLayerTell(s32 index, const PatPacketHeader* header) = 0;
    /* +0x150 */ virtual s32 recvNtcLayerTellLow(s32 index, const PatPacketHeader* header) = 0;
    /* +0x154 */ virtual s32 recvAnsLayerMediationLock(s32 index, const PatPacketHeader* header) = 0;
    /* +0x158 */ virtual s32 recvNtcLayerMediationLock(s32 index, const PatPacketHeader* header) = 0;
    /* +0x15C */ virtual s32 recvAnsLayerMediationUnlock(s32 index, const PatPacketHeader* header) = 0;
    /* +0x160 */ virtual s32 recvNtcLayerMediationUnlock(s32 index, const PatPacketHeader* header) = 0;
    /* +0x164 */ virtual s32 recvAnsLayerMediationList(s32 index, const PatPacketHeader* header) = 0;
    /* +0x168 */ virtual s32 recvAnsLayerDetailSearchHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x16C */ virtual s32 recvAnsLayerDetailSearchData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x170 */ virtual s32 recvAnsLayerDetailSearchFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x174 */ virtual s32 recvAnsCircleCreate(s32 index, const PatPacketHeader* header) = 0;
    /* +0x178 */ virtual s32 recvAnsCircleInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x17C */ virtual s32 recvAnsCircleJoin(s32 index, const PatPacketHeader* header) = 0;
    /* +0x180 */ virtual s32 recvNtcCircleJoin(s32 index, const PatPacketHeader* header) = 0;
    /* +0x184 */ virtual s32 recvAnsCircleLeave(s32 index, const PatPacketHeader* header) = 0;
    /* +0x188 */ virtual s32 recvNtcCircleLeave(s32 index, const PatPacketHeader* header) = 0;
    /* +0x18C */ virtual s32 recvAnsCircleBreak(s32 index, const PatPacketHeader* header) = 0;
    /* +0x190 */ virtual s32 recvNtcCircleBreak(s32 index, const PatPacketHeader* header) = 0;
    /* +0x194 */ virtual s32 recvAnsCircleMatchOptionSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x198 */ virtual s32 recvNtcCircleMatchOptionSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x19C */ virtual s32 recvAnsCircleMatchOptionGet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1A0 */ virtual s32 recvAnsCircleMatchStart(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1A4 */ virtual s32 recvNtcCircleMatchStart(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1A8 */ virtual s32 recvAnsCircleMatchEnd(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1AC */ virtual s32 recvAnsCircleInfoSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1B0 */ virtual s32 recvNtcCircleInfoSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1B4 */ virtual s32 recvAnsCircleListLayer(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1B8 */ virtual s32 recvAnsCircleSearchHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1BC */ virtual s32 recvAnsCircleSearchData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1C0 */ virtual s32 recvAnsCircleSearchFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1C4 */ virtual s32 recvAnsCircleKick(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1C8 */ virtual s32 recvNtcCircleKick(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1CC */ virtual s32 recvAnsCircleDeleteKickList(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1D0 */ virtual s32 recvAnsCircleHostHandover(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1D4 */ virtual s32 recvNtcCircleHostHandover(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1D8 */ virtual s32 recvAnsCircleHost(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1DC */ virtual s32 recvNtcCircleHost(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1E0 */ virtual s32 recvAnsCircleUserList(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1E4 */ virtual s32 recvNtcCircleBinary(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1E8 */ virtual s32 recvNtcChat(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1EC */ virtual s32 recvAnsCircleTell(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1F0 */ virtual s32 recvNtcCircleTell(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1F4 */ virtual s32 recvAnsCircleInfoNoticeSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1F8 */ virtual s32 recvNtcCircleListLayerCreate(s32 index, const PatPacketHeader* header) = 0;
    /* +0x1FC */ virtual s32 recvNtcCircleListLayerChange(s32 index, const PatPacketHeader* header) = 0;
    /* +0x200 */ virtual s32 recvNtcCircleListLayerDelete(s32 index, const PatPacketHeader* header) = 0;
    /* +0x204 */ virtual s32 recvAnsMcsCreate(s32 index, const PatPacketHeader* header) = 0;
    /* +0x208 */ virtual s32 recvNtcMcsCreate(s32 index, const PatPacketHeader* header) = 0;
    /* +0x20C */ virtual s32 recvNtcMcsStart(s32 index, const PatPacketHeader* header) = 0;
    /* +0x210 */ virtual s32 recvAnsTell(s32 index, const PatPacketHeader* header) = 0;
    /* +0x214 */ virtual s32 recvNtcTell(s32 index, const PatPacketHeader* header) = 0;
    /* +0x218 */ virtual s32 recvAnsBinaryUser(s32 index, const PatPacketHeader* header) = 0;
    /* +0x21C */ virtual s32 recvNtcBinaryUser(s32 index, const PatPacketHeader* header) = 0;
    /* +0x220 */ virtual s32 recvNtcBinaryServer(s32 index, const PatPacketHeader* header) = 0;
    /* +0x224 */ virtual s32 recvAnsUserSearchSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x228 */ virtual s32 recvAnsUserBinarySet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x22C */ virtual s32 recvAnsUserBinaryNotice(s32 index, const PatPacketHeader* header) = 0;
    /* +0x230 */ virtual s32 recvNtcUserBinaryNotice(s32 index, const PatPacketHeader* header) = 0;
    /* +0x234 */ virtual s32 recvAnsUserSearchHead(s32 index, const PatPacketHeader* header) = 0;
    /* +0x238 */ virtual s32 recvAnsUserSearchData(s32 index, const PatPacketHeader* header) = 0;
    /* +0x23C */ virtual s32 recvAnsUserSearchFoot(s32 index, const PatPacketHeader* header) = 0;
    /* +0x240 */ virtual s32 recvAnsUserSearchInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x244 */ virtual s32 recvAnsUserSearchInfoMine(s32 index, const PatPacketHeader* header) = 0;
    /* +0x248 */ virtual s32 recvAnsUserStatusSet(s32 index, const PatPacketHeader* header) = 0;
    /* +0x24C */ virtual s32 recvAnsUserStatus(s32 index, const PatPacketHeader* header) = 0;
    /* +0x250 */ virtual s32 recvAnsFriendAdd(s32 index, const PatPacketHeader* header) = 0;
    /* +0x254 */ virtual s32 recvNtcFriendAdd(s32 index, const PatPacketHeader* header) = 0;
    /* +0x258 */ virtual s32 recvAnsFriendAccept(s32 index, const PatPacketHeader* header) = 0;
    /* +0x25C */ virtual s32 recvNtcFriendAccept(s32 index, const PatPacketHeader* header) = 0;
    /* +0x260 */ virtual s32 recvAnsFriendDelete(s32 index, const PatPacketHeader* header) = 0;
    /* +0x264 */ virtual s32 recvAnsFriendList(s32 index, const PatPacketHeader* header) = 0;
    /* +0x268 */ virtual s32 recvAnsBlackAdd(s32 index, const PatPacketHeader* header) = 0;
    /* +0x26C */ virtual s32 recvAnsBlackDelete(s32 index, const PatPacketHeader* header) = 0;
    /* +0x270 */ virtual s32 recvAnsBlackList(s32 index, const PatPacketHeader* header) = 0;
    /* +0x274 */ virtual s32 recvAnsAgreementPageNum(s32 index, const PatPacketHeader* header) = 0;
    /* +0x278 */ virtual s32 recvAnsAgreementPageInfo(s32 index, const PatPacketHeader* header) = 0;
    /* +0x27C */ virtual s32 recvAnsAgreementPage(s32 index, const PatPacketHeader* header) = 0;
    /* +0x280 */ virtual s32 recvAnsAgreement(s32 index, const PatPacketHeader* header) = 0;
    /* +0x284 */ virtual s32 recvCommand(const PatPacketHeader* header) = 0;
    /* +0x288 */ virtual void postError(NetworkErrorInfo* info) = 0;

    /* 0x803FB490 - one step of the connection's close: arms it (state 0 -> 10), then releases the log context and
     * the socket and returns 1 once closed, else 0 (GUESS name from the body). */
    s32 disconnect();

    /* +0x0004 */ u8 pad_0004[0x60A6];   /* the connection's buffers and socket state */
    /* +0x60AA */ PatPacketHeader header_60AA;   /* the packet being dispatched */
    /* +0x60B2 */ u8 pad_60B2[0x02];
    /* +0x60B4 */ u8* readCursor_60B4;   /* the readers' position in the packet body */
    /* +0x60B8 */ u8 pad_60B8[0x18];
    /* +0x60D0 */ u8 cryptEnabled_60D0;   /* the receive loop decrypts the body while set */
    /* +0x60D1 */ u8 opening_state;     /* the opening sub-step the mediator's progress reads (0 and 90 ignored) */
    /* +0x60D2 */ u8 connectionState_60D2;   /* `disconnect`'s state: 0 idle, 10 closing */
    /* +0x60D3 */ u8 pad_60D3;
};   /* size: 0x60D4 (the derived `PatInterface`'s first own field, the reference count, is at +0x60D4) */
typedef PatInterface NetworkStateMachine;  /* the state machine's spelling of the singleton */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the session band's request writers (moved here from `Network/network_state.h`; the state machine
   passes its own view of the session object) */
u32  flushBuffer(NetworkStateMachine* self, u32 opcode, u32 flags);
void encryptBuffer(NetworkStateMachine* self);
u32  writeUInt8(NetworkStateMachine* self, u8 value);
void writeUInt16(NetworkStateMachine* self, u16 value);
void writeUInt32(NetworkStateMachine* self, u32 value);
void writeUInt32Shared(NetworkStateMachine* self, u32 value);
void writeUInt8Array(NetworkStateMachine* self, const u8* data, u16 count);
void writeBool(NetworkStateMachine* self, s8 value);

/* 0x803FBB04 */
void readUInt8(NetworkStateMachine* self, u8* out);
/* 0x803FBB20 */
void readUInt16(NetworkStateMachine* self, u16* out);
/* 0x803FBB8C */
void readUInt32_(NetworkStateMachine* self, u32* out);
/* 0x803FBC6C */
void readUInt8_(NetworkStateMachine* self, s8* out);
/* 0x803FBCE0 */
void readInt32(NetworkStateMachine* self, s32* out);
/* 0x803FBD18 */
void readString(NetworkStateMachine* self, u32* length, char* buffer, u16 size);
/* 0x803FBDDC */
void readUInt8Array(NetworkStateMachine* self, u32* length, u8* buffer, u16 size);
/* 0x803FBE88 */
void beginReadBlock(NetworkStateMachine* self, u16* size);
/* 0x803FBED0 */
void endReadBlock(NetworkStateMachine* self);
/* 0x803FC40C */
void writeInt16(NetworkStateMachine* self, s16 value);
/* 0x803FC418 */
u32 writeString(NetworkStateMachine* self, const char* text);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATCONNECTION_H */
