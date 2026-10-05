/*
 * include/Network/PatInterface.h - the declarations `src/Network/PatInterface.cpp` owns
 * (`.text` 0x803FCC34..0x803FE8E4, the `PatInterface` singleton's C-linkage surface).
 *
 * The unit has no bodies yet, so every parameter list is the one its callers' calls demonstrate; the
 * first parameter is the singleton (`this` in r3 - the target bodies read it), spelled the way each
 * caller holds it: `PatInterface` (the mediator band), `NetworkInstance` (what `getInstance_` returns,
 * the band header's typedef of this class since request net3-d-03c2#1) or
 * `NetworkStateMachine` (the state machine's spelling, a typedef of `PatInterface`).  Moved here from `include/unsplit/Network.h`,
 * `include/Network/network_state.h` and `src/Network/NetworkWiiMediator.cpp` (docs/plan.md 6.5 rule 2:
 * the owner declares).
 */
#ifndef MHTRI_NETWORK_PATINTERFACE_H
#define MHTRI_NETWORK_PATINTERFACE_H

#include "types.h"
#include "Network/sNetworkLibrary.h"   /* sNetworkLibraryError - the error triple `reportPatError` takes */

#include "Network/gamespy_interface_types.h"   /* NetworkErrorInfo - the kept error triple */

#include "unsplit/Network.h"           /* NetworkInstance (an alias of PatInterface), NetworkPostedError */

/* The 8-byte header of one received packet (the connection keeps it at +0x60AA): `recvCommand` matches the three
 * op-code bytes against the packet table, a status of -1/1 selects `recvAnsNg`/`recvAnsAlert`, and the handlers hand
 * the request id on to the session handlers. */
typedef struct PatPacketHeader {
    /* +0x00 */ u8 size_00[2];
    /* +0x02 */ u8 requestId_02[2];   /* copied out with `memcpy` - the packet is a byte stream */
    /* +0x04 */ u8 opcode_04[3];
    /* +0x07 */ s8 status_07;         /* -1 negative reply, 1 alert, else the handler's own */
} PatPacketHeader;   /* size: 0x08 */

/* One 0x5C-byte user row: the local row at +0x8B54 and each row of the array `+0x8BB8` points at.
 * `sendReqUserObject` compares one against the other field by field. */
/* The record a negative reply fills (`recvAnsNg`): the server's code, the packet table index and its message. */
typedef struct PatErrorRecord {
    /* +0x000 */ s32  code_000;
    /* +0x004 */ s32  index_004;
    /* +0x008 */ char message_008[0x200];
} PatErrorRecord;   /* size: 0x208 (`clearErrorRecord613c` clears 0x208 bytes) */

/* One session event handler: `dispatchSessionHandlers` passes the event code, the request id, the two values and
 * the payload, then the argument bound with the handler. */
typedef void (*PatEventCallback)(u32 code, u16 requestId, s32 value, u32 count, const u8* data, u32 arg);

/* The login ticket the session hands the server (`ticket_8BB0` points at the caller's buffer). */
typedef struct PatTicket {
    /* +0x000 */ u8  data_000[0x400];
    /* +0x400 */ u16 size_400;   /* bytes of `data_000` in use */
} PatTicket;   /* size: 0x402 (approximate: the last field the handlers touch) */

/* One server address: the host name and the port (`recvNtcRecconect` reads both, the constructor's default is
 * port 8200). */
typedef struct PatServerAddress {
    /* +0x000 */ char host_000[0x104];
    /* +0x104 */ u16  port_104;
} PatServerAddress;   /* size: 0x106 (the constructor clears 262 bytes) */

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
    /* +0x04 */ u8  pad_04[0x04];
    /* +0x08 */ u64 time_08;        /* copied out as two words by NetworkLayerPat's slot exporter 0x803EA61C (GUESS name) */
    /* +0x10 */ u32 done_10;
    /* +0x14 */ u32 total_14;
    /* +0x18 */ char name_18[0x20]; /* the exporter copies 31 bytes of it */
    /* +0x38 */ char text_38[0x04];
    /* +0x3C */ u32 value_3C;
} NetworkFmpSlot;   /* size: 0x40 */

/* The network singleton (`getInstance_` returns it, `mpInstance__12PatInterface` holds it).  The mediator band
 * (`Network/NetworkWiiMediator.cpp`) allocates it with `new` - 0xD640 bytes, the allocation
 * `initializeNetworkMediator` makes - and calls its five buffer setters by their mangled member names
 * (`setTermsBuffer__12PatInterfaceFPScUl`).  The constructor is out of line (`__ct__12PatInterfaceFv`,
 * 0x803FCC34): it first calls the base constructor 0x803FAE9C (`Network/PatConnection.cpp`'s range), then stores
 * the 161-slot table 0x80602198 (3 slots in this unit, 158 in `Network/network_layer_io.cpp`'s range).  The
 * virtual is declared and not defined here, so no table is emitted by a consumer.  The fields from +0x60E8 on are
 * the request state machine's (`Network/network_state.cpp`, which spells the type `NetworkStateMachine`, a
 * typedef of this class); every gap is padding that keeps the offsets exact.
 */
class PatInterface {
public:
    PatInterface();
    /* +0x00 - the vtable pointer; slot +0x08 is the deleting finalizer */
    virtual void finalize(s32 flags);
    /* +0x00C - restores the defaults (the base constructor calls it through the table) */
    virtual void resetDefaults();
    /* +0x010..+0x280 - the packet handlers: `recvCommand` finds the header's op-code in the packet table
     * (`PacketTable_BaseOffset_ID1`) and calls the entry's member pointer with the entry index */
    /* +0x010 */ virtual s32 recvReqLineCheck(s32 index, const PatPacketHeader* header);
    /* +0x014 */ virtual s32 recvAnsServerTime(s32 index, const PatPacketHeader* header);
    /* +0x018 */ virtual s32 recvAnsShut(s32 index, const PatPacketHeader* header);
    /* +0x01C */ virtual s32 recvNtcShut(s32 index, const PatPacketHeader* header);
    /* +0x020 */ virtual s32 recvNtcRecconect(s32 index, const PatPacketHeader* header);
    /* +0x024 */ virtual s32 recvReqConnection(s32 index, const PatPacketHeader* header);
    /* +0x028 */ virtual s32 recvNtcLogin(s32 index, const PatPacketHeader* header);
    /* +0x02C */ virtual s32 recvAnsTicket(s32 index, const PatPacketHeader* header);
    /* +0x030 */ virtual s32 recvReqTicket(s32 index, const PatPacketHeader* header);
    /* +0x034 */ virtual s32 recvReqWarning(s32 index, const PatPacketHeader* header);
    /* +0x038 */ virtual s32 recvAnsCommonKey(s32 index, const PatPacketHeader* header);
    /* +0x03C */ virtual s32 recvReqMemoryCheck(s32 index, const PatPacketHeader* header);
    /* +0x040 */ virtual s32 recvAnsLoginInfo(s32 index, const PatPacketHeader* header);
    /* +0x044 */ virtual s32 recvAnsChargeInfo(s32 index, const PatPacketHeader* header);
    /* +0x048 */ virtual s32 recvAnsUserListHead(s32 index, const PatPacketHeader* header);
    /* +0x04C */ virtual s32 recvAnsUserListData(s32 index, const PatPacketHeader* header);
    /* +0x050 */ virtual s32 recvAnsUserListFoot(s32 index, const PatPacketHeader* header);
    /* +0x054 */ virtual s32 recvAnsUserObject(s32 index, const PatPacketHeader* header);
    /* +0x058 */ virtual s32 recvAnsFmpListVersion(s32 index, const PatPacketHeader* header);
    /* +0x05C */ virtual s32 recvAnsFmpListHead(s32 index, const PatPacketHeader* header);
    /* +0x060 */ virtual s32 recvAnsFmpListData(s32 index, const PatPacketHeader* header);
    /* +0x064 */ virtual s32 recvAnsFmpListFoot(s32 index, const PatPacketHeader* header);
    /* +0x068 */ virtual s32 recvAnsFmpInfo(s32 index, const PatPacketHeader* header);
    /* +0x06C */ virtual s32 recvAnsRfpConnect(s32 index, const PatPacketHeader* header);
    /* +0x070 */ virtual s32 recvAnsLmpConnect(s32 index, const PatPacketHeader* header);
    /* +0x074 */ virtual s32 recvAnsTermsVersion(s32 index, const PatPacketHeader* header);
    /* +0x078 */ virtual s32 recvAnsTerms(s32 index, const PatPacketHeader* header);
    /* +0x07C */ virtual s32 recvAnsMaintenance(s32 index, const PatPacketHeader* header);
    /* +0x080 */ virtual s32 recvAnsAnnounce(s32 index, const PatPacketHeader* header);
    /* +0x084 */ virtual s32 recvAnsNoCharge(s32 index, const PatPacketHeader* header);
    /* +0x088 */ virtual s32 recvAnsMediaVersionInfo(s32 index, const PatPacketHeader* header);
    /* +0x08C */ virtual s32 recvAnsVulgarityInfoHigh(s32 index, const PatPacketHeader* header);
    /* +0x090 */ virtual s32 recvAnsVulgarityHigh(s32 index, const PatPacketHeader* header);
    /* +0x094 */ virtual s32 recvAnsVulgarityInfoLow(s32 index, const PatPacketHeader* header);
    /* +0x098 */ virtual s32 recvAnsVulgarityLow(s32 index, const PatPacketHeader* header);
    /* +0x09C */ virtual s32 recvAnsAuthenticationToken(s32 index, const PatPacketHeader* header);
    /* +0x0A0 */ virtual s32 recvAnsBinaryVersion(s32 index, const PatPacketHeader* header);
    /* +0x0A4 */ virtual s32 recvAnsBinaryHead(s32 index, const PatPacketHeader* header);
    /* +0x0A8 */ virtual s32 recvAnsBinaryData(s32 index, const PatPacketHeader* header);
    /* +0x0AC */ virtual s32 recvAnsBinarFoot(s32 index, const PatPacketHeader* header);
    /* +0x0B0 */ virtual s32 recvAnsLayerStart(s32 index, const PatPacketHeader* header);
    /* +0x0B4 */ virtual s32 recvAnsLayerEnd(s32 index, const PatPacketHeader* header);
    /* +0x0B8 */ virtual s32 recvNtcLayerUserNum(s32 index, const PatPacketHeader* header);
    /* +0x0BC */ virtual s32 recvAnsLayerJump(s32 index, const PatPacketHeader* header);
    /* +0x0C0 */ virtual s32 recvAnsLayerCreateHead(s32 index, const PatPacketHeader* header);
    /* +0x0C4 */ virtual s32 recvAnsLayerCreateSet(s32 index, const PatPacketHeader* header);
    /* +0x0C8 */ virtual s32 recvAnsLayerCreateFoot(s32 index, const PatPacketHeader* header);
    /* +0x0CC */ virtual s32 recvAnsLayerDown(s32 index, const PatPacketHeader* header);
    /* +0x0D0 */ virtual s32 recvNtcLayerIn(s32 index, const PatPacketHeader* header);
    /* +0x0D4 */ virtual s32 recvAnsLayerUp(s32 index, const PatPacketHeader* header);
    /* +0x0D8 */ virtual s32 recvNtcLayerOut(s32 index, const PatPacketHeader* header);
    /* +0x0DC */ virtual s32 recvNtcLayerJumpReady(s32 index, const PatPacketHeader* header);
    /* +0x0E0 */ virtual s32 recvNtcLayerJumpGo(s32 index, const PatPacketHeader* header);
    /* +0x0E4 */ virtual s32 recvAnsLayerInfoSe(s32 index, const PatPacketHeader* header);
    /* +0x0E8 */ virtual s32 recvNtcLayerInfoSet(s32 index, const PatPacketHeader* header);
    /* +0x0EC */ virtual s32 recvAnsLayerInfo(s32 index, const PatPacketHeader* header);
    /* +0x0F0 */ virtual s32 recvAnsLayerParentInfo(s32 index, const PatPacketHeader* header);
    /* +0x0F4 */ virtual s32 recvAnsLayerChildInfo(s32 index, const PatPacketHeader* header);
    /* +0x0F8 */ virtual s32 recvAnsLayerChildListHead(s32 index, const PatPacketHeader* header);
    /* +0x0FC */ virtual s32 recvAnsLayerChildListData(s32 index, const PatPacketHeader* header);
    /* +0x100 */ virtual s32 recvAnsLayerChildListFoot(s32 index, const PatPacketHeader* header);
    /* +0x104 */ virtual s32 recvAnsLayerSiblingListHead(s32 index, const PatPacketHeader* header);
    /* +0x108 */ virtual s32 recvAnsLayerSiblingListData(s32 index, const PatPacketHeader* header);
    /* +0x10C */ virtual s32 recvAnsLayerSiblingListFoot(s32 index, const PatPacketHeader* header);
    /* +0x110 */ virtual s32 recvAnsLayerHost(s32 index, const PatPacketHeader* header);
    /* +0x114 */ virtual s32 recvNtcLayerHost(s32 index, const PatPacketHeader* header);
    /* +0x118 */ virtual s32 recvAnsLayerUserInfoSet(s32 index, const PatPacketHeader* header);
    /* +0x11C */ virtual s32 recvNtcLayerUserInfoSet(s32 index, const PatPacketHeader* header);
    /* +0x120 */ virtual s32 recvAnsLayerUserList(s32 index, const PatPacketHeader* header);
    /* +0x124 */ virtual s32 recvAnsLayerUserListHead(s32 index, const PatPacketHeader* header);
    /* +0x128 */ virtual s32 recvAnsLayerUserListData(s32 index, const PatPacketHeader* header);
    /* +0x12C */ virtual s32 recvAnsLayerUserListFoot(s32 index, const PatPacketHeader* header);
    /* +0x130 */ virtual s32 recvAnsLayerUserSearchHead(s32 index, const PatPacketHeader* header);
    /* +0x134 */ virtual s32 recvAnsLayerUserSearchData(s32 index, const PatPacketHeader* header);
    /* +0x138 */ virtual s32 recvAnsLayerUserSearchFoot(s32 index, const PatPacketHeader* header);
    /* +0x13C */ virtual s32 recvNtcLayerBinary(s32 index, const PatPacketHeader* header);
    /* +0x140 */ virtual s32 recvNtcLayerUserPosition(s32 index, const PatPacketHeader* header);
    /* +0x144 */ virtual s32 recvNtcLayerChat(s32 index, const PatPacketHeader* header);
    /* +0x148 */ virtual s32 recvAnsLayerTell(s32 index, const PatPacketHeader* header);
    /* +0x14C */ virtual s32 recvNtcLayerTell(s32 index, const PatPacketHeader* header);
    /* +0x150 */ virtual s32 recvNtcLayerTellLow(s32 index, const PatPacketHeader* header);
    /* +0x154 */ virtual s32 recvAnsLayerMediationLock(s32 index, const PatPacketHeader* header);
    /* +0x158 */ virtual s32 recvNtcLayerMediationLock(s32 index, const PatPacketHeader* header);
    /* +0x15C */ virtual s32 recvAnsLayerMediationUnlock(s32 index, const PatPacketHeader* header);
    /* +0x160 */ virtual s32 recvNtcLayerMediationUnlock(s32 index, const PatPacketHeader* header);
    /* +0x164 */ virtual s32 recvAnsLayerMediationList(s32 index, const PatPacketHeader* header);
    /* +0x168 */ virtual s32 recvAnsLayerDetailSearchHead(s32 index, const PatPacketHeader* header);
    /* +0x16C */ virtual s32 recvAnsLayerDetailSearchData(s32 index, const PatPacketHeader* header);
    /* +0x170 */ virtual s32 recvAnsLayerDetailSearchFoot(s32 index, const PatPacketHeader* header);
    /* +0x174 */ virtual s32 recvAnsCircleCreate(s32 index, const PatPacketHeader* header);
    /* +0x178 */ virtual s32 recvAnsCircleInfo(s32 index, const PatPacketHeader* header);
    /* +0x17C */ virtual s32 recvAnsCircleJoin(s32 index, const PatPacketHeader* header);
    /* +0x180 */ virtual s32 recvNtcCircleJoin(s32 index, const PatPacketHeader* header);
    /* +0x184 */ virtual s32 recvAnsCircleLeave(s32 index, const PatPacketHeader* header);
    /* +0x188 */ virtual s32 recvNtcCircleLeave(s32 index, const PatPacketHeader* header);
    /* +0x18C */ virtual s32 recvAnsCircleBreak(s32 index, const PatPacketHeader* header);
    /* +0x190 */ virtual s32 recvNtcCircleBreak(s32 index, const PatPacketHeader* header);
    /* +0x194 */ virtual s32 recvAnsCircleMatchOptionSet(s32 index, const PatPacketHeader* header);
    /* +0x198 */ virtual s32 recvNtcCircleMatchOptionSet(s32 index, const PatPacketHeader* header);
    /* +0x19C */ virtual s32 recvAnsCircleMatchOptionGet(s32 index, const PatPacketHeader* header);
    /* +0x1A0 */ virtual s32 recvAnsCircleMatchStart(s32 index, const PatPacketHeader* header);
    /* +0x1A4 */ virtual s32 recvNtcCircleMatchStart(s32 index, const PatPacketHeader* header);
    /* +0x1A8 */ virtual s32 recvAnsCircleMatchEnd(s32 index, const PatPacketHeader* header);
    /* +0x1AC */ virtual s32 recvAnsCircleInfoSet(s32 index, const PatPacketHeader* header);
    /* +0x1B0 */ virtual s32 recvNtcCircleInfoSet(s32 index, const PatPacketHeader* header);
    /* +0x1B4 */ virtual s32 recvAnsCircleListLayer(s32 index, const PatPacketHeader* header);
    /* +0x1B8 */ virtual s32 recvAnsCircleSearchHead(s32 index, const PatPacketHeader* header);
    /* +0x1BC */ virtual s32 recvAnsCircleSearchData(s32 index, const PatPacketHeader* header);
    /* +0x1C0 */ virtual s32 recvAnsCircleSearchFoot(s32 index, const PatPacketHeader* header);
    /* +0x1C4 */ virtual s32 recvAnsCircleKick(s32 index, const PatPacketHeader* header);
    /* +0x1C8 */ virtual s32 recvNtcCircleKick(s32 index, const PatPacketHeader* header);
    /* +0x1CC */ virtual s32 recvAnsCircleDeleteKickList(s32 index, const PatPacketHeader* header);
    /* +0x1D0 */ virtual s32 recvAnsCircleHostHandover(s32 index, const PatPacketHeader* header);
    /* +0x1D4 */ virtual s32 recvNtcCircleHostHandover(s32 index, const PatPacketHeader* header);
    /* +0x1D8 */ virtual s32 recvAnsCircleHost(s32 index, const PatPacketHeader* header);
    /* +0x1DC */ virtual s32 recvNtcCircleHost(s32 index, const PatPacketHeader* header);
    /* +0x1E0 */ virtual s32 recvAnsCircleUserList(s32 index, const PatPacketHeader* header);
    /* +0x1E4 */ virtual s32 recvNtcCircleBinary(s32 index, const PatPacketHeader* header);
    /* +0x1E8 */ virtual s32 recvNtcChat(s32 index, const PatPacketHeader* header);
    /* +0x1EC */ virtual s32 recvAnsCircleTell(s32 index, const PatPacketHeader* header);
    /* +0x1F0 */ virtual s32 recvNtcCircleTell(s32 index, const PatPacketHeader* header);
    /* +0x1F4 */ virtual s32 recvAnsCircleInfoNoticeSet(s32 index, const PatPacketHeader* header);
    /* +0x1F8 */ virtual s32 recvNtcCircleListLayerCreate(s32 index, const PatPacketHeader* header);
    /* +0x1FC */ virtual s32 recvNtcCircleListLayerChange(s32 index, const PatPacketHeader* header);
    /* +0x200 */ virtual s32 recvNtcCircleListLayerDelete(s32 index, const PatPacketHeader* header);
    /* +0x204 */ virtual s32 recvAnsMcsCreate(s32 index, const PatPacketHeader* header);
    /* +0x208 */ virtual s32 recvNtcMcsCreate(s32 index, const PatPacketHeader* header);
    /* +0x20C */ virtual s32 recvNtcMcsStart(s32 index, const PatPacketHeader* header);
    /* +0x210 */ virtual s32 recvAnsTell(s32 index, const PatPacketHeader* header);
    /* +0x214 */ virtual s32 recvNtcTell(s32 index, const PatPacketHeader* header);
    /* +0x218 */ virtual s32 recvAnsBinaryUser(s32 index, const PatPacketHeader* header);
    /* +0x21C */ virtual s32 recvNtcBinaryUser(s32 index, const PatPacketHeader* header);
    /* +0x220 */ virtual s32 recvNtcBinaryServer(s32 index, const PatPacketHeader* header);
    /* +0x224 */ virtual s32 recvAnsUserSearchSet(s32 index, const PatPacketHeader* header);
    /* +0x228 */ virtual s32 recvAnsUserBinarySet(s32 index, const PatPacketHeader* header);
    /* +0x22C */ virtual s32 recvAnsUserBinaryNotice(s32 index, const PatPacketHeader* header);
    /* +0x230 */ virtual s32 recvNtcUserBinaryNotice(s32 index, const PatPacketHeader* header);
    /* +0x234 */ virtual s32 recvAnsUserSearchHead(s32 index, const PatPacketHeader* header);
    /* +0x238 */ virtual s32 recvAnsUserSearchData(s32 index, const PatPacketHeader* header);
    /* +0x23C */ virtual s32 recvAnsUserSearchFoot(s32 index, const PatPacketHeader* header);
    /* +0x240 */ virtual s32 recvAnsUserSearchInfo(s32 index, const PatPacketHeader* header);
    /* +0x244 */ virtual s32 recvAnsUserSearchInfoMine(s32 index, const PatPacketHeader* header);
    /* +0x248 */ virtual s32 recvAnsUserStatusSet(s32 index, const PatPacketHeader* header);
    /* +0x24C */ virtual s32 recvAnsUserStatus(s32 index, const PatPacketHeader* header);
    /* +0x250 */ virtual s32 recvAnsFriendAdd(s32 index, const PatPacketHeader* header);
    /* +0x254 */ virtual s32 recvNtcFriendAdd(s32 index, const PatPacketHeader* header);
    /* +0x258 */ virtual s32 recvAnsFriendAccept(s32 index, const PatPacketHeader* header);
    /* +0x25C */ virtual s32 recvNtcFriendAccept(s32 index, const PatPacketHeader* header);
    /* +0x260 */ virtual s32 recvAnsFriendDelete(s32 index, const PatPacketHeader* header);
    /* +0x264 */ virtual s32 recvAnsFriendList(s32 index, const PatPacketHeader* header);
    /* +0x268 */ virtual s32 recvAnsBlackAdd(s32 index, const PatPacketHeader* header);
    /* +0x26C */ virtual s32 recvAnsBlackDelete(s32 index, const PatPacketHeader* header);
    /* +0x270 */ virtual s32 recvAnsBlackList(s32 index, const PatPacketHeader* header);
    /* +0x274 */ virtual s32 recvAnsAgreementPageNum(s32 index, const PatPacketHeader* header);
    /* +0x278 */ virtual s32 recvAnsAgreementPageInfo(s32 index, const PatPacketHeader* header);
    /* +0x27C */ virtual s32 recvAnsAgreementPage(s32 index, const PatPacketHeader* header);
    /* +0x280 */ virtual s32 recvAnsAgreement(s32 index, const PatPacketHeader* header);
    /* +0x284 - dispatches one received packet (the connection's receive loop calls it) */
    virtual s32 recvCommand(const PatPacketHeader* header);
    /* +0x288 - keeps the first error and reports it to the session handlers */
    virtual void postError(NetworkErrorInfo* info);

    /* the by-value spelling: the caller's argument copy is what the virtual slot is handed */
    inline void postError(NetworkPostedError info) { postError((NetworkErrorInfo*)&info); }

    /* the three replies `recvCommand` handles itself: an op-code with no table entry, a negative and an alert status */
    s32 recvNotProvided(s32 index, const PatPacketHeader* header);
    s32 recvAnsNg(s32 index, const PatPacketHeader* header);
    s32 recvAnsAlert(s32 index, const PatPacketHeader* header);

    /* the connected server's tag every handler's log line starts with (defined in `Network/network_layer_io.cpp`) */
    inline const char* getServerName();

    /* +0x0004 */ u8 pad_0004[0x60A6];   /* the connection base's buffers (`Network/PatConnection.cpp`) */
    /* +0x60AA */ PatPacketHeader header_60AA;   /* the packet being dispatched */
    /* +0x60B2 */ u8 pad_60B2[0x02];
    /* +0x60B4 */ u8* readCursor_60B4;   /* the readers' position in the packet body */
    /* +0x60B8 */ u8 pad_60B8[0x18];
    /* +0x60D0 */ u8 cryptEnabled_60D0;   /* the receive loop decrypts the body while set */
    /* +0x60D1 */ u8 opening_state;     /* the opening sub-step the mediator's progress reads (0 and 90 ignored) */
    /* +0x60D2 */ u8 pad_60D2[0x02];
    /* +0x60D4 */ s32 refCount_60D4;   /* `increment60d4`/`decrement60d4` */
    /* +0x60D8 */ PatEventCallback eventCallbacks_60D8[8];   /* `dispatchSessionHandlers` calls each bound one */
    /* +0x60F8 */ u32 eventCallbackArgs_60F8[8];   /* the argument each callback is handed last */
    /* +0x6118 */ s8 searchMode_6118;   /* nonzero suppresses the user search reports */
    /* +0x6119 */ s8 searchKind_6119;   /* selects `recvAnsUserSearchInfo`'s event (1 or 2) */
    /* +0x611A */ s8 binaryMode_611A;   /* how binary replies are consumed: 5 reports them, 6 keeps the text (`recvAnsBinary*`) */
    /* +0x611B */ u8 busy_611B;   /* `testAndSet611b`/`set611b` */
    /* +0x611C */ f32 clock_611C;   /* the running clock `getGameTime` measures from */
    /* +0x6120 */ f32 clockAtSync_6120;   /* the clock when the server time last arrived */
    /* +0x6124 */ u8 pad_6124[0x04];
    /* +0x6128 */ u32 gameTimeBase_6128;   /* the server's game time at the last sync (`recvAnsServerTime`) */
    /* +0x612C */ u32 serverTime_612C;   /* the server's date-time at the last sync (`getServerTime`) */
    /* +0x6130 */ u8 sessionArmed_6130;   /* block-1 latch set by `resetNetworkState` */
    /* +0x6131 */ u8 termsArmed_6131;   /* block-3 latch set by `resetNetworkState3` */
    /* +0x6132 */ u8 sessionState_6132;   /* `handleNetworkState1`'s switch value */
    /* +0x6133 */ u8 subState_6133;   /* `advanceNetworkState5`'s gated state (== 5) */
    /* +0x6134 */ u8 binaryState_6134;   /* 10 selects the maintenance-reject path */
    /* +0x6135 */ u8 requestState_6135;   /* the block-2 sub-machine's switch value */
    /* +0x6136 */ u8 shutdownMode_6136;   /* the mode `sendReqShut` records */
    /* +0x6137 */ u8 fmpState_6137;   /* `handleNetworkState4`'s switch value */
    /* +0x6138 */ u8 flag_6138;   /* `setPatByte6138On` */
    /* +0x6139 */ u8 pad_6139[0x03];
    /* +0x613C */ PatErrorRecord errorRecord_613C;   /* the last negative reply (`recvAnsNg`) */
    /* +0x6344 */ PatErrorRecord shutdownRecord_6344;   /* the server's shutdown notice (`recvNtcShut`) */
    /* +0x654C */ NetworkErrorInfo pendingError_654C;   /* the first error kept (`postError`); code 0 while none */
    /* +0x6558 */ u8 flag_6558;
    /* +0x6559 */ u8 shutdownFlag_6559;
    /* +0x655A */ u8 pad_655A[0x02];
    /* +0x655C */ u32 loginDataSize_655C;   /* the login body's total */
    /* +0x6560 */ u32 loginDataCursor_6560;
    /* +0x6564 */ u32 loginDataPtr_6564;
    /* +0x6568 */ char reflectName3C_6568[0x20];   /* the mediator's reflect name +0x3C (`setPatReflectName3C`) */
    /* +0x6588 */ char mediaVersion_6588[0x20];   /* `getMediaVersion` (`recvAnsMediaVersionInfo`) */
    /* +0x65A8 */ char mediaVersionText_65A8[0x20];   /* `getStr1` (`readMediaVersionData`, item 2) */
    /* +0x65C8 */ char reflectName5C_65C8[0x20];   /* the mediator's reflect name +0x5C (`setPatReflectName5C`) */
    /* +0x65E8 */ u32 mySearchValue_65E8;   /* this client's own search row values (`recvAnsUserSearchInfoMine`) */
    /* +0x65EC */ u32 mySearchValue_65EC;
    /* +0x65F0 */ s32 serverType_65F0;   /* the connected server: 0 LMP, 1 FMP, 2 OPN, 3 RFP (`setConnectServerType`) */
    /* +0x65F4 */ u8 pad_65F4[0x04];
    /* +0x65F8 */ u32 fmpSelected_65F8;   /* the chosen FMP slot index */
    /* +0x65FC */ u8 pad_65FC[0x000C];
    /* +0x6608 */ u32 fmpSlotCount_6608;   /* number of live rows in `fmpSlots_6C40` */
    /* +0x660C */ u8 pad_660C[0x04];
    /* +0x6610 */ u32 rfpConnected_6610;   /* set once the RFP server's address arrived (`recvAnsRfpConnect`) */
    /* +0x6614 */ u8 pad_6614[0x0106];
    /* +0x671A */ u8 fmpReserve_671A[0x0106];
    /* +0x6820 */ u8 pad_6820[0x020C];
    /* +0x6A2C */ PatServerAddress serverAddress_6A2C;   /* the server to connect to (`chooseServerAddress`) */
    /* +0x6B32 */ PatServerAddress lmpServer_6B32;   /* the lobby server (`recvAnsLmpConnect`, or the mediator's first address) */
    /* +0x6C38 */ u32 fmpListActive_6C38;
    /* +0x6C3C */ u8 fmpListReady_6C3C;
    /* +0x6C3D */ u8 pad_6C3D[0x03];
    /* +0x6C40 */ NetworkFmpSlot fmpSlots_6C40[80];
    /* +0x8040 */ u32 fmpSelected_8040;     /* the FMP slot the query settled on (`getFmpSelected`) */
    /* +0x8044 */ u32 fmpQueryValue_8044;   /* the FMP list query argument / result */
    /* +0x8048 */ u8 fmpReply_8048[0x0106];
    /* +0x814E */ PatServerAddress rfpServer_814E;   /* the RFP server (`recvAnsRfpConnect`, or the mediator's second address) */
    /* +0x8254 */ u8 patState_8254;   /* == 3 means the PAT handshake is up */
    /* +0x8255 */ u8 pad_8255[0x03];
    /* +0x8258 */ u32 sendSlice_8258;   /* the slice descriptor handed to `sendReqVulgarity*` */
    /* +0x825C */ u32 dataTotal_825C;   /* body bytes to send, decremented on completion */
    /* +0x8260 */ u32 dataSent_8260;   /* body bytes already sent */
    /* +0x8264 */ u32 termsVersion_8264;   /* the terms version last read (`recvAnsTermsVersion`) */
    /* +0x8268 */ u8 termsChanged_8268;   /* set when that version differs from the previous one */
    /* +0x8269 */ u8 pad_8269[0x03];
    /* +0x826C */ u32 termsSize_826C;
    /* +0x8270 */ u32 maintenanceSize_8270;   /* the caller buffers' capacities (`setMaintenanceBuffer` ...) */
    /* +0x8274 */ u32 announceSize_8274;
    /* +0x8278 */ u32 noChargeSize_8278;
    /* +0x827C */ u32 vulgarityHighSize_827C;   /* `setPatBuffer` */
    /* +0x8280 */ u32 userListSize_8280;
    /* +0x8284 */ u32 vulgaritySize_8284;
    /* +0x8288 */ u32 patchMessageSize_8288;
    /* +0x828C */ u8* termsBufferPtr_828C;
    /* +0x8290 */ u8* maintenanceBuffer_8290;
    /* +0x8294 */ u8* announceBuffer_8294;
    /* +0x8298 */ u8* noChargeBuffer_8298;
    /* +0x829C */ u8* vulgarityHighBuffer_829C;
    /* +0x82A0 */ u8* userListPtr_82A0;
    /* +0x82A4 */ u8* vulgarityPtr_82A4;
    /* +0x82A8 */ u8* patchMessageBuffer_82A8;
    /* +0x82AC */ u8 loginFields_82AC[0x08];   /* the tag/value block `sendReqLoginInfo` builds */
    /* +0x82B4 */ u8 loginInfoSent_82B4;
    /* +0x82B5 */ u8 pad_82B5[0x0020];
    /* +0x82D5 */ char userIdText_82D5[0x2C];
    /* +0x8301 */ char userPasswordText_8301[0x2C];
    /* +0x832D */ u8 pad_832D[0x061B];
    /* +0x8948 */ u8* commonKeyBuffer_8948;   /* `recvAnsCommonKey` reads the 256-byte key into it */
    /* +0x894C */ u8 commonKeyReady_894C;
    /* +0x894D */ u8 fmpPhase_894D;   /* the FMP sub-machine's phase (== 1/== 2/== 3/== 5) */
    /* +0x894E */ u8 patPhase_894E;
    /* +0x894F */ u8 connectionPhase_894F;   /* the connection sub-machine's phase (2..6) */
    /* +0x8950 */ u8 sessionReady_8950;
    /* +0x8951 */ char termText_8951[0x200];   /* the login reply's message, truncated into `replyBuffer_8BC8` */
    /* +0x8B51 */ u8 pad_8B51[0x03];
    /* +0x8B54 */ NetworkUserRow userRow_8B54;   /* this client's own row */
    /* +0x8BB0 */ PatTicket* ticket_8BB0;   /* the login ticket (`recvAnsTicket`/`recvReqTicket`) */
    /* +0x8BB4 */ u32 userRowCount_8BB4;   /* rows in `userRows_8BB8`, 92 bytes each */
    /* +0x8BB8 */ u8* userRows_8BB8;
    /* +0x8BBC */ u8 warningKind_8BBC;   /* `recvReqWarning` */
    /* +0x8BBD */ u8 pad_8BBD[0x03];
    /* +0x8BC0 */ u32 warningValue_8BC0;   /* `getWarningUInt2` */
    /* +0x8BC4 */ u32 replySize_8BC4;   /* the reply buffer's capacity */
    /* +0x8BC8 */ u8* replyBuffer_8BC8;
    /* +0x8BCC */ u32 replySent_8BCC;
    /* +0x8BD0 */ u32 replyTotal_8BD0;
    /* +0x8BD4 */ u32 stackRemaining_8BD4;   /* the call stack's free bytes (`pushStack`/`growStackSize`) */
    /* +0x8BD8 */ u32 stackUsed_8BD8;
    /* +0x8BDC */ u8 pad_8BDC[0x4824];
    /* +0xD400 */ u8 binaryActive_D400;   /* the circle/binary sub-machine is armed */
    /* +0xD401 */ u8 pad_D401[0x03];
    /* +0xD404 */ u32 binarySize_D404;
    /* +0xD408 */ u8 binaryTextReady_D408;
    /* +0xD409 */ char binaryText_D409[0x0203];   /* the reply, split into tab-separated tokens */
    /* +0xD60C */ u32 binaryTokens_D60C[8];   /* token start offsets inside `binaryText_D409` */
    /* +0xD62C */ u8 layerMovePending_D62C;   /* cleared by the layer jump/down/up answers */
    /* +0xD62D */ u8 pad_D62D[0x03];
    /* +0xD630 */ s32 matchStartValue_D630;   /* the trailing value of the match start notice */
    /* +0xD634 */ u8 pad_D634[0x0C];

    void setTermsBuffer(s8* buffer, u32 size);
    void setMaintenanceBuffer(s8* buffer, u32 size);
    void setAnnounceBuffer(s8* buffer, u32 size);
    void setNoChargeBuffer(s8* buffer, u32 size);
    void setPatchMessageBuffer(s8* buffer, u32 size);
};   /* size: 0xD640 (the allocation `initializeNetworkMediator` makes) */

/* The request state machine's spelling of the singleton (`Network/network_state.cpp`, `Network/PatConnection.h`). */
typedef PatInterface NetworkStateMachine;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803FD2D8 / 0x803FD644 - resets the singleton's callbacks unless it is still busy; whether it is
 * idle (both bodies read the singleton in r3). */
void  PatInterface_clear(PatInterface* self);
s32   PatInterface_isReady(PatInterface* self);

/* 0x803FD324 - the singleton's per-frame step the Pat holder's drive calls (GUESS from the caller). */
void stepPatInterface(NetworkInstance* self);

/* the singleton's callback slots */
s32 isCallback(NetworkInstance* self, s32 index);
void resetCallback(NetworkInstance* self, s32 index);

void decrement60d4(NetworkInstance* self);
/* 0x803FD258 - takes a reference on the singleton like `increment60d4`, and the first one also clears the
 * receive buffer's head before opening it (GUESS name). */
void openPatInterface(NetworkInstance* self);
/* 0x803FD90C - keeps the first error triple (+0x654C) and dispatches it to the session handlers with
 * code `kind` (GUESS name). */
void reportPatError(NetworkInstance* self, u32 kind, sNetworkLibraryError error);
/* 0x803FD944 - dispatches event `code` with a payload to the session handlers (GUESS name). */
void notifyPatEvent(NetworkInstance* self, u32 code, u32 count, const u8* data);
/* 0x803FD298 - takes a reference on the singleton (+0x60D4); the first one opens it (its +0x0C slot). */
void increment60d4(NetworkInstance* self);
/* 0x803FD9CC - installs callback `index` (0..7) with its argument; 1, or -1 for a bad index. */
/* untyped: caller-owned payload - handed back to the callback */
s32 setCallback(NetworkInstance* self, void (*callback)(), void* arg, u32 index);
/* 0x803FDCB0 - selects the connect server type (masked to 0..3). */
void setConnectServerType(NetworkInstance* self, s32 type);
s32 hasMultipleRefs60d4(NetworkInstance* self);
/* untyped: caller-owned payload - the 0x208-byte error record copied in when non-null */
s32 errorRecordCode613c(NetworkInstance* self, const void* record);
/* 0x803FD794 - copies the kept error triple (+0x654C) out when `info` is non-null and returns its first word (the error code, 0 when none is kept). */
s32 getErrorInfo654c(NetworkInstance* self, u32* info);

/* 0x803768F8-band clock: the network singleton's game time (GUESS: the caller passes the singleton). */
u32 getGameTime(NetworkInstance* self);

/* 0x803FE854 / 0x803FE860 / 0x803FE404 / 0x803FE73C - the network singleton's Pat setters.  0x860 is a
 * name: the body `strlen`s its argument and copies at most 31 bytes of it. */
void setPatField854(PatInterface* self, u32 value);
void setPatField860(PatInterface* self, const char* name);
void setPatByteD400(NetworkInstance* self, u8 value);
void setPatByte6138On(NetworkInstance* self);

/* the term/opening queries and buffers the mediator band forwards */
void  setTermVersion(PatInterface* self, u32 value);
s32   getTermsVersion(PatInterface* self);
u32   getWarningUInt2(PatInterface* self);
s32   isOpeningAnnounce(PatInterface* self);
/* 0x803FE1E0 / 0x803FE1C8 - the status byte at +0x8254 reads 1 or 2 / reads 1 */
s32   isOpeningMaintenanceServer(PatInterface* self);
s32   isOpeningMaintenanceTerms(PatInterface* self);
void  updatePatInterface(PatInterface* self, u32 a, u32 b, u32 c);
void  setPatBuffer(PatInterface* self, u32 index, char* buffer, u32 size);
void  setPatRange(PatInterface* self, u32 index, u32 address, u32 size);
u32   getPatServerTime(PatInterface* self);
char* getPatAccountName(PatInterface* self);
char* getMediaVersion(PatInterface* self);
char* getStr1(PatInterface* self);
u32   getServerTime(PatInterface* self);

/* the connection sub-machine's state predicates (named for the field and value each tests) */
s32   isSubState_8254_3(PatInterface* self);     /* +0x8254 == 3 */
s32   isSubState_894F_2(PatInterface* self);     /* +0x894F == 2 */
s32   isSubState_894F_3(PatInterface* self);     /* +0x894F == 3 */
s32   isSubState_894F_4or6(PatInterface* self);  /* +0x894F == 4 || == 6 */
s32   isSubState_894F_5(PatInterface* self);     /* +0x894F == 5 */
s32   isSubState_894F_6(PatInterface* self);     /* +0x894F == 6 */

/* the reflect page fields the mediator mirrors */
void  setPatReflectPageRange(PatInterface* self, u32 address, u32 size);
void  setPatReflectField30(PatInterface* self, u32 value);
void  setPatReflectField34(PatInterface* self, u32 value);
void  setPatReflectField38(PatInterface* self, u32 value);
void  setPatReflectName3C(PatInterface* self, char* name);
void  setPatReflectName5C(PatInterface* self, char* name);

/* the login data getters, the FMP slot lookup and the call-stack helpers the state machine uses */
u32 getSomething3(NetworkStateMachine* self);       /* +0x655C */
u32 getSomething6(NetworkStateMachine* self);       /* +0x6560 */
u32 getSomething9(NetworkStateMachine* self);       /* +0x6564 */
u32 getFmpSlotIndex(NetworkStateMachine* self, u32 value);
u8*  createStack(NetworkStateMachine* self, u32 size, u32* outSize);
/* 0x803FDA78 - takes `size` bytes off the call stack (the granted size in `outSize`; `align` is the extra the
 * caller asked for). */
u8*  pushStack(NetworkStateMachine* self, u32 size, u32* outSize, u32* align);
/* 0x803FDF10 / 0x803FDF1C - the FMP slot the query settled on and the number of live slots. */
u32 getFmpSelected(NetworkStateMachine* self);
s32 getFmpSize(NetworkStateMachine* self);
/* 0x803FE4C8 / 0x803FE4D0 - this client's own search row values (+0x65E8 / +0x65EC). */
u32 getSomething8(NetworkStateMachine* self);
u32 getSomething7(NetworkStateMachine* self);
void growStackSize(NetworkStateMachine* self, u32 size);
void chooseServerAddress(NetworkStateMachine* self, u32 a, u32 b);

/* 0x803FD674 */
void clearErrorRecord613c(NetworkInstance* self);
/* 0x803FD6D8 */
void buildErrorInfo613c(NetworkInstance* self, u32 code, NetworkErrorInfo* out);
/* 0x803FD7BC */
void getErrorInfoOrCode654c(NetworkInstance* self, u32 code, NetworkErrorInfo* out);
/* 0x803FDC80 - 1 when the +0x611B flag is already set, else sets it and returns 0 (GUESS name: a test-and-set
 * in the scheme of `set611b`; the circle list query retries until it returns 0). */
s32 testAndSet611b(NetworkInstance* self);
/* 0x803FDCA4 */
void set611b(NetworkInstance* self);
/* 0x803FDE10 - the selection word +0x65F0 (0..3) and its table entry at +0x65F4; both -1 when out of range
 * (GUESS name). */
void getFmpSelection(NetworkInstance* self, s32* kind, s32* index);
/* 0x803FDECC */
s32 getServerDateTime(NetworkInstance* self);
/* 0x803FDF24 - copies FMP slot `index` (0x40 bytes at +0x6C40) to `out`: -1 for a null `out`, -2 out of
 * range, else `index` (GUESS name). */
s32 copyFmpSlot(NetworkInstance* self, NetworkFmpSlot* out, s32 index);
/* 0x803FE470 */
void getSelectedID(NetworkInstance* self, u8* out);
/* 0x803FE488 */
void getSelectedHunterName(NetworkInstance* self, char* out);
/* 0x803FE4A0 */
void setSomething(NetworkInstance* self, s32 value);
/* 0x803FE4A8 */
s8 getSomething4(NetworkInstance* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATINTERFACE_H */
