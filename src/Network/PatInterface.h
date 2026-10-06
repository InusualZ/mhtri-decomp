/*
 * Network/PatInterface.h - the declarations of `Network/PatInterface.cpp`: the `PatInterface` singleton, its request
 *   state machine and its packet layer (`NetworkPool`/`NetworkRandom` are `Network/NetworkPool.h`'s).
 * SHAPES. An unwritten body's parameter list is the one its callers demonstrate; the first parameter is the singleton
 *   (`this` in r3), spelled as each caller holds it: `PatInterface`, or the typedefs `NetworkInstance` (what
 *   `getInstance_` returns) and `NetworkStateMachine`.
 */
#ifndef MHTRI_NETWORK_PATINTERFACE_H
#define MHTRI_NETWORK_PATINTERFACE_H

#include "types.h"
#include "Network/sNetworkLibrary.h"   /* sNetworkLibraryError - the error triple `reportPatError` takes */

#include "Network/gamespy_interface_types.h"   /* NetworkErrorInfo - the kept error triple */

#include "unsplit/Network.h"           /* NetworkInstance (an alias of PatInterface), NetworkPostedError */
#include "Network/PatConnection.h"     /* PatConnection - the base class, PatPacketHeader */
#include "Network/NetworkSessionManager.h"   /* PatCircleInfo - the circle entries embed it */
#include "sound/fn_800E46E8.h"         /* getInstance - the state machine's mediator lookup */

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

/* One server block of the server tables: the host name, the address it resolves to and the port (the FMP reply's
 * items 2 and 3 fill the name and the port; the connect step copies the address in). */
typedef struct PatServerBlock {
    /* +0x000 */ char host_000[0x100];
    /* +0x100 */ u8   address_100[4];
    /* +0x104 */ u16  port_104;
} PatServerBlock;   /* size: 0x106 */

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
 * the 161-slot table 0x80602198 (every slot in this unit's range).  The
 * key function (the destructor) is defined in `src/Network/PatInterface.cpp`, so only that object emits the table.  The fields from +0x60E8 on are
 * the request state machine's (which spells the type `NetworkStateMachine`, a typedef of this class); every gap is padding that keeps the offsets exact.
 */
class PatInterface : public PatConnection {
public:
    /* 0x803FCC34 */
    PatInterface();
    /* +0x008 - 0x803FCF8C */
    virtual ~PatInterface();
    /* +0x00C - restores the defaults (the base constructor calls it through the table) */
    virtual void resetDefaults();
    /* +0x010..+0x280 - the packet handlers: `recvCommand` finds the header's op-code in the packet table
     * (`patPacketTable`, `Network/PatConnection.h`) and calls the entry's member pointer with the entry index */
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
    virtual s32 postError(NetworkErrorInfo* info);

    /* the by-value spelling: the caller's argument copy is what the virtual slot is handed */
    inline void postError(NetworkPostedError info) { postError((NetworkErrorInfo*)&info); }
    inline void postError(NetworkErrorInfo info) { postError(&info); }

    /* the three replies `recvCommand` handles itself: an op-code with no table entry, a negative and an alert status */
    s32 recvNotProvided(s32 index, const PatPacketHeader* header);
    s32 recvAnsNg(s32 index, const PatPacketHeader* header);
    s32 recvAnsAlert(s32 index, const PatPacketHeader* header);

    /* the connected server's tag every handler's log line starts with (defined in `Network/PatInterface.cpp`) */
    inline const char* getServerName();

    /* +0x0000..+0x60D4 - `PatConnection` (the table pointer, the buffers, the packet header) */
    /* +0x60D4 */ s32 refCount_60D4;   /* `increment60d4`/`decrement60d4` */
    /* +0x60D8 */ PatEventCallback eventCallbacks_60D8[8];   /* `dispatchSessionHandlers` calls each bound one */
    /* +0x60F8 */ u32 eventCallbackArgs_60F8[8];   /* the argument each callback is handed last */
    /* +0x6118 */ s8 searchMode_6118;   /* nonzero suppresses the user search reports */
    /* +0x6119 */ s8 searchKind_6119;   /* selects `recvAnsUserSearchInfo`'s event (1 or 2) */
    /* +0x611A */ s8 binaryMode_611A;   /* how binary replies are consumed: 5 reports them, 6 keeps the text (`recvAnsBinary*`) */
    /* +0x611B */ u8 busy_611B;   /* `testAndSet611b`/`set611b` */
    /* +0x611C */ f32 clock_611C;   /* the running clock `getGameTime` measures from */
    /* +0x6120 */ f32 clockAtSync_6120;   /* the clock when the server time last arrived */
    /* +0x6124 */ f32 connectTime_6124;   /* the clock when the connection opened (`stepPatConnect`) */
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
    /* +0x65F4 */ u32 serverIndex_65F4[4];   /* per server type (`serverType_65F0`): the chosen address, -1 for none
                                             * (the constructor's `memset`); [1] is the chosen FMP slot */
    /* +0x6604 */ u32 serverCount_6604[4];   /* per server type: the known addresses - the constructor sets LMP and OPN to 1;
                                             * [1] is the number of live rows in `fmpSlots_6C40`, [3] is set once the
                                             * RFP server's address arrived (`recvAnsRfpConnect`) */
    /* +0x6614 */ PatServerBlock serverReserve_6614[4];   /* per server type, a server block (the constructor clears all four);
                                                     * [1] is the FMP reply's (`recvAnsFmpInfo`) */
    /* +0x6A2C */ PatServerAddress serverAddress_6A2C;   /* the server to connect to (`chooseServerAddress`) */
    /* +0x6B32 */ PatServerAddress lmpServer_6B32;   /* the lobby server (`recvAnsLmpConnect`, or the mediator's first address) */
    /* +0x6C38 */ u32 fmpListActive_6C38;
    /* +0x6C3C */ u8 fmpListReady_6C3C;
    /* +0x6C3D */ u8 pad_6C3D[0x03];
    /* +0x6C40 */ NetworkFmpSlot fmpSlots_6C40[80];
    /* +0x8040 */ u32 fmpSelected_8040;     /* the FMP slot the query settled on (`getFmpSelected`) */
    /* +0x8044 */ u32 fmpQueryValue_8044;   /* the FMP list query argument / result */
    /* +0x8048 */ PatServerBlock fmpReply_8048;
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
    /* +0x82AC */ u32 loginFields_82AC[2];   /* the tag/value block `sendReqLoginInfo` builds (`resetDefaults` clears both words) */
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
    /* +0x8BD4 */ u32 stackTop_8BD4;    /* the call stack's bytes in use (`pushStack`/`growStackSize`) */
    /* +0x8BD8 */ u32 stackFree_8BD8;   /* its free bytes ("mStackSize" in `pushStack`'s logs) */
    /* +0x8BDC */ u8 pad_8BDC[0x24];
    /* +0x8C00 */ u8 stack_8C00[0x4800];   /* the call stack the handlers take their records from */
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

    /* .sbss 0x80794CB0 - the live singleton (the constructor publishes it, the destructor clears it, `getInstance_` reads it) */
    static PatInterface* mpInstance;
};   /* size: 0xD640 (the allocation `initializeNetworkMediator` makes) */

/* The request state machine's spelling of the singleton (this unit's state machine, `Network/PatConnection.h`). */
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
/* 0x803FDE48 (GUESS name) - copies server block `type` (0x106 bytes of `serverReserve_6614`, the index taken modulo
 * 4) to `out`; nothing for a null `out`. */
void copyServerBlock(NetworkInstance* self, s32 type, u8* out);
/* 0x803FE410 (GUESS name) - token `index` (0..7) of the split binary reply: `binaryText_D409` plus
 * `binaryTokens_D60C[index]`, NULL past 7.  The network control's default error texts read tokens 0/1 with it. */
char* getBinaryToken(NetworkInstance* self, u32 index);
/* 0x803FE470 */
void getSelectedID(NetworkInstance* self, u8* out);
/* 0x803FE488 */
void getSelectedHunterName(NetworkInstance* self, char* out);
/* 0x803FE4A0 */
void setSomething(NetworkInstance* self, s32 value);
/* 0x803FE4A8 */
u8 getSomething4(NetworkInstance* self);

#ifdef __cplusplus
}
#endif

/* ==== the packet layer (the former `Network/network_layer_io.cpp` head) ==================================== */

typedef struct NetLayerFilter NetLayerFilter;           /* Network/NetworkLayerPat.h */
typedef struct NetUserPosition NetUserPosition;         /* Network/NetworkLayerPat.h */
typedef struct NetLayerUserRecord NetLayerUserRecord;   /* Network/NetworkLayerPat.h */
struct PatCircleOptionList;
struct PatMatchOptions;
typedef struct PatCircleFilter PatCircleFilter;   /* Network/NetworkSessionManager.h */

typedef struct PatCircleInfo PatCircleInfo;       /* Network/NetworkSessionManager.h */
class NetworkWiiMediator;                         /* Network/NetworkWiiMediator.h */
class NetworkReflectService;                      /* Network/NetworkReflectService.h */

/* ---- the records the packet handlers read and hand to the session handlers ---------------------------------- */

/* One tagged value of a `PatTagList`: `type` 1 carries the 32-bit `value`. */
typedef struct PatTagValue {
    /* +0x0 */ u8  tag_0;
    /* +0x1 */ u8  type_1;
    /* +0x2 */ u8  pad_2[2];
    /* +0x4 */ u32 value_4;
} PatTagValue;   /* size: 0x8 */

/* Up to 32 tagged values (`readUnkByteIntStruct` reads `count` of them; records of 260 bytes). */
typedef struct PatTagList {
    /* +0x000 */ u8          count_000;
    /* +0x001 */ u8          pad_001[3];
    /* +0x004 */ PatTagValue values_004[32];
} PatTagList;   /* size: 0x104 */

/* One layer (a lobby level) as `readLayerData` fills it: only the fields the handlers touch are named. */
typedef struct PatLayerData {
    /* +0x000 */ u32  id_000;           /* item 1 of the layer requests (`writeLayerDownData`) */
    /* +0x004 */ u8   path_004[0x10];   /* `readUnkShortArrayStruct` fills it before the layer's own items */
    /* +0x014 */ char name_014[0x40];   /* item 3 of the layer requests (`writeLayerDownData`) */
    /* +0x054 */ s16 layerId_054;      /* the id `recvAnsLayerChildInfo` read first (item 5, sent 1-based) */
    /* +0x056 */ u8  pad_056[0x02];
    /* +0x058 */ s32 counts_058[3];    /* items 6..8 (the counting form's items 2..4) */
    /* +0x064 */ s32 memberLimitA_064;  /* item 9: the create request's first member limit */
    /* +0x068 */ s32 memberLimitB_068;  /* item 10: its second member limit */
    /* +0x06C */ s32 value_06C;        /* item 11 */
    /* +0x070 */ u32 matchKey_070;      /* item 12: the layer's match key (NetworkLayerPat keeps it at +0x3D4) */
    /* +0x074 */ u16 value_074;        /* item 13 */
    /* +0x076 */ s8  value_076;        /* item 16 */
    /* +0x077 */ u8  pad_077;
    /* +0x078 */ u32 value_078;        /* item 17 */
    /* +0x07C */ s8  value_07C;        /* item 18 */
    /* +0x07D */ s8  isCurrent_07D;    /* selects `recvAnsLayerInfo`'s event (item 21, sent 1-based; GUESS) */
    /* +0x07E */ char comment_07E[0xC0];   /* item 22 */
    /* +0x13E */ u8  binary_13E[0x100]; /* the binary info `NetworkLayerPat::handleLayerInfoSet` copies in */
    /* +0x23E */ u16 value_23E;        /* the layer setting request sends item 23 while it is nonzero (the binary size) */
} PatLayerData;   /* size: 0x240 (the handlers clear 576 bytes) */

/* One layer record of the layer info and list answers: the layer and its tag list. */
typedef struct PatLayerInfo {
    /* +0x000 */ u8           kind_000;   /* `recvNtcLayerUserNum` reads it first */
    /* +0x001 */ u8           pad_001[0x03];
    /* +0x004 */ PatLayerData layer_004;
    /* +0x244 */ PatTagList   tags_244;
} PatLayerInfo;   /* size: 0x348 (the list answers allocate 840 bytes a row) */

/* One user of a layer (`readLayerUserData`): the handlers read its short id first. */
typedef struct PatLayerUser {
    /* +0x000 */ char userId_000[0x08];
    /* +0x008 */ char name_008[0x20];    /* the host answers read it after the id */
    /* +0x028 */ u8   path_028[0x10];    /* the user's layer path (`readUnkShortArrayStruct`) */
    /* +0x038 */ u32  key_038;           /* item 6 */
    /* +0x03C */ u8   binary_03C[0x100]; /* item 7 */
    /* +0x13C */ u16  binarySize_13C;
    /* +0x13E */ u8   pad_13E[0x02];
} PatLayerUser;   /* size: 0x140 (the handlers clear 320 bytes) */

/* One row of the layer user lists: the user and its tag list. */
typedef struct PatLayerUserInfo {
    /* +0x000 */ PatLayerUser user_000;
    /* +0x140 */ PatTagList   tags_140;
} PatLayerUserInfo;   /* size: 0x244 (the list answers allocate 580 bytes a row) */

/* The inline value of one received item: the scalars `readItemList` reads by type (the strings and byte arrays keep a
 * pointer into the call stack here instead). */
typedef union PatItemValue {
    /* +0x0 */ u8          byte;    /* type 1 */
    /* +0x0 */ u16         half;    /* type 2 */
    /* +0x0 */ u32         word;    /* types 3 and 7 */
    /* +0x0 */ u64         dword;   /* type 4 */
    /* +0x0 */ f32         real;    /* type 5 */
    /* +0x0 */ f64         dreal;   /* type 6 */
    /* +0x0 */ const char* text;    /* type 8 */
    struct {
        /* +0x0 */ const u8* data;
        /* +0x4 */ u16       size;
    } bytes;                        /* type 9 */
} PatItemValue;   /* size: 0x8 */

/* One typed item of a received item list (`readItemList`): its tag, its type (0..9) and its value. */
typedef struct PatItem {
    /* +0x00 */ u8           tag_00;
    /* +0x01 */ u8           type_01;
    /* +0x02 */ u8           pad_02[0x06];
    /* +0x08 */ PatItemValue value_08;
} PatItem;   /* size: 0x10 (`createItemListStack` sizes the array in 16-byte rows) */

/* A received item list: `initItemList` binds `capacity_03` rows at `items_04`, `readItemList` fills `count_02` of them
 * (more than the capacity is an overflow the handlers do not report).  Names GUESS from the two bodies. */
typedef struct PatItemList {
    /* +0x0 */ u16      marker_00;     /* `initItemList` stores 1, `readItemList` reads it first (0: no items) */
    /* +0x2 */ u8       count_02;
    /* +0x3 */ u8       capacity_03;
    /* +0x4 */ PatItem* items_04;
} PatItemList;   /* size: 0x8 */

/* A binary item notice from a layer or circle member: the sender's id and its item list. */
typedef struct PatItemNotice {
    /* +0x00 */ char        userId_00[0x08];
    /* +0x08 */ PatItemList items_08;
} PatItemNotice;   /* size: 0x10 (the handlers clear 16 bytes) */

/* A layer member's position notice: the sender's id and the position its six items carry (tags 1..3 the floats,
 * 4..6 the words - the values `NetworkLayerPat::sendUserPosition_60` sends as a `NetUserPosition`). */
typedef struct PatUserPositionNotice {
    /* +0x00 */ char userId_00[0x08];
    /* +0x08 */ f32  position_08[3];
    /* +0x14 */ s32  value_14[3];
} PatUserPositionNotice;   /* size: 0x20 (the handler clears 32 bytes) */

/* One detail search answer as the session handlers receive it (on the call stack): the layer, its tag list and its
 * users (a second call-stack block).  Name GUESS from `recvAnsLayerDetailSearchData`. */
typedef struct PatDetailSearchResult {
    /* +0x000 */ s32               userCount_000;   /* clamped to the rows the call stack could hold */
    /* +0x004 */ PatLayerUserInfo* users_004;
    /* +0x008 */ u8                pad_008[0x04];
    /* +0x00C */ PatLayerData      layer_00C;
    /* +0x24C */ PatTagList        tags_24C;
} PatDetailSearchResult;   /* size: 0x350 (the handler takes 848 bytes off the call stack) */

/* The sender of a chat or tell (`readChatData`). */
typedef struct PatChatInfo {
    /* +0x00 */ u32  value_00;
    /* +0x04 */ u32  time_04;           /* sent relative to the server time (`readTimeOffset`) */
    /* +0x08 */ char userId_08[0x08];   /* the tells read it before the sender's items */
    /* +0x10 */ char name_10[0x20];
} PatChatInfo;   /* size: 0x30 */

/* One chat or tell as the session handlers receive it: the text and its sender. */
typedef struct PatChatMessage {
    /* +0x000 */ char        text_000[0x100];
    /* +0x100 */ PatChatInfo sender_100;
} PatChatMessage;   /* size: 0x130 (the handlers clear 304 bytes) */

/* One mediation lock entry (`readMediationData`): the user and the lock value. */
typedef struct PatMediation {
    /* +0x0 */ char userId_0[0x08];
    /* +0x8 */ u8   value_8;
    /* +0x9 */ u8   key_9;     /* the second lock argument (`writeMediationItems` item 3) */
} PatMediation;   /* size: 0xA (the notices clear 10 bytes, the list answer 32 of them) */

/* One circle (a hunting party) as the circle answers read it: its id, the info items and the tag list. */
typedef struct PatCircleEntry {
    /* +0x000 */ PatCircleInfo info_000;   /* `readCircleInfoDataArray` fills it */
    /* +0x37C */ PatTagList    tags_37C;
} PatCircleEntry;   /* size: 0x480 (the handlers clear 1152 bytes) */

/* One member event of a circle: the circle, the member's slot and state, its id and name. */
typedef struct PatCircleUser {
    /* +0x00 */ s32  circleId_00;
    /* +0x04 */ s8   slot_04;    /* sent 1-based (`readCircleSlot`) */
    /* +0x05 */ u8   state_05;
    /* +0x06 */ char userId_06[0x08];
    /* +0x0E */ char name_0E[0x20];
    /* +0x2E */ u8   pad_2E[0x02];
} PatCircleUser;   /* size: 0x30 (the handlers clear 48 bytes) */

/* One circle member's match options (`readCircleMatchData`; the match start notice fills the head itself). */
typedef struct PatCircleMatch {
    /* +0x00 */ u8   tag_00[4];  /* item 1 (a 4-byte string: the member's tag word) */
    /* +0x04 */ u16  value_04;
    /* +0x06 */ s8   mode_06;
    /* +0x07 */ s8   slot_07;    /* sent 1-based (`readCircleSlot`) */
    /* +0x08 */ char userId_08[0x08];
    /* +0x10 */ char name_10[0x20];
} PatCircleMatch;   /* size: 0x30 (the handlers clear 48 bytes, the user list 4 of them) */

/* The match start notice as the session handlers receive it: the members (on the handler's stack) and the tail. */
typedef struct PatMatchStart {
    /* +0x00 */ s32             count_00;     /* 1..4 members */
    /* +0x04 */ PatCircleMatch* members_04;
    /* +0x08 */ u8              flag_08;      /* the optional tail, read while the block has bytes left */
    /* +0x09 */ u8              pad_09[0x03];
    /* +0x0C */ u32             values_0C[4];
} PatMatchStart;   /* size: 0x1C (the handler clears 28 bytes) */

/* The kick notice: a kind byte and a byte string with its length. */
typedef struct PatKickList {
    /* +0x000 */ u8  kind_000;
    /* +0x001 */ u8  pad_001;
    /* +0x002 */ u8  data_002[0x100];
    /* +0x102 */ u16 size_102;
} PatKickList;   /* size: 0x104 (the handler clears 260 bytes) */

/* The MCS server a match starts on: its address and name. */
typedef struct PatMcsServer {
    /* +0x000 */ char host_000[0x104];   /* the `PatServerAddress` layout */
    /* +0x104 */ u16  port_104;
    /* +0x106 */ char name_106[0x20];
} PatMcsServer;   /* size: 0x126 (the handler clears 294 bytes) */

/* The sender block of the binary notices (`readNtcCompoundData`). */
typedef struct PatNtcCompound {
    /* +0x00 */ u32  time_00;           /* sent relative to the server time (`readTimeOffset`) */
    /* +0x04 */ char userId_04[0x08];   /* the user notice reads it first */
    /* +0x0C */ char name_0C[0x20];
} PatNtcCompound;   /* size: 0x2C (the handlers clear 44 bytes) */

/* A binary notice from a user or the server: the bytes, their length and the sender. */
typedef struct PatBinaryMessage {
    /* +0x000 */ u8             data_000[0x100];
    /* +0x100 */ u16            size_100;
    /* +0x102 */ u8             pad_102[0x02];
    /* +0x104 */ PatNtcCompound sender_104;
} PatBinaryMessage;   /* size: 0x130 */

/* A user's binary notice: the user, a kind, the bytes, their length and a value. */
typedef struct PatUserBinaryNotice {
    /* +0x000 */ char userId_000[0x08];
    /* +0x008 */ u8   kind_008;
    /* +0x009 */ u8   data_009[0x100];
    /* +0x109 */ u8   pad_109[0x03];
    /* +0x10C */ u32  size_10C;
    /* +0x110 */ u32  value_110;
} PatUserBinaryNotice;   /* size: 0x114 */

/* One user search row (`readUserSearchData`) and its tag list. */
typedef struct PatUserSearchRow {
    /* +0x000 */ char userId_000[0x08];
    /* +0x008 */ char name_008[0x20];
    /* +0x028 */ u8   binary_028[0x100];
    /* +0x128 */ u16  binarySize_128;
    /* +0x12A */ u8   pad_12A[0x02];
    /* +0x12C */ u8   path_12C[0x10];
    /* +0x13C */ s8   value_13C;        /* item 7, sent 1-based */
    /* +0x13D */ char comment_13D[0xC0];
    /* +0x1FD */ s8   value_1FD;
    /* +0x1FE */ char text_1FE[0x20];
    /* +0x21E */ u8   pad_21E[0x02];
    /* +0x220 */ s32  value_220;
    /* +0x224 */ s32  value_224;
    /* +0x228 */ u32  value_228;        /* `recvAnsUserSearchInfoMine` keeps both values */
    /* +0x22C */ u32  value_22C;
} PatUserSearchRow;   /* size: 0x230 */

typedef struct PatUserSearch {
    /* +0x000 */ PatUserSearchRow row_000;
    /* +0x230 */ PatTagList       tags_230;
} PatUserSearch;   /* size: 0x334 (the list answer allocates 820 bytes a row) */

/* One friend (`readFriendData`). */
typedef struct PatFriend {
    /* +0x00 */ u32  value_00;
    /* +0x04 */ char userId_04[0x08];
    /* +0x0C */ char name_0C[0x20];
    /* +0x2C */ s8   state_2C;   /* the friend notice reads it last */
    /* +0x2D */ u8   pad_2D[0x03];
} PatFriend;   /* size: 0x30 */

/* One black list entry (`readBlackListData`). */
typedef struct PatBlackListEntry {
    /* +0x00 */ u32  value_00;
    /* +0x04 */ char userId_04[0x08];
    /* +0x0C */ char name_0C[0x20];
} PatBlackListEntry;   /* size: 0x2C */

/* The agreement's page info as the session handlers receive it (the page records on the call stack). */
typedef struct PatAgreementHead {
    /* +0x00 */ u8  version_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 value_04;   /* item 4 */
    /* +0x08 */ u32 value_08;   /* item 2 */
    /* +0x0C */ u8  pageCount_0C;
    /* +0x0D */ u8  pad_0D[0x03];
} PatAgreementHead;   /* size: 0x10 (`readAgreementInfoData` fills one) */

typedef struct PatAgreementInfo {
    /* +0x00 */ PatAgreementHead head_00;
    /* +0x10 */ u8* pages_10;   /* `head_00.pageCount_0C` `PatAgreementPageInfo` records */
} PatAgreementInfo;   /* size: 0x14 */

/* One agreement page record of the page info answer. */
typedef struct PatAgreementPageInfo {
    /* +0x00 */ u8   page_00;
    /* +0x01 */ u8   pad_01[0x03];
    /* +0x04 */ u32  value_04;
    /* +0x08 */ char title_08[0x20];
} PatAgreementPageInfo;   /* size: 0x28 */

/* One agreement page as the session handlers receive it (the text on the call stack). */
typedef struct PatAgreementPage {
    /* +0x0 */ u32 page_0;
    /* +0x4 */ u32 value_4;
    /* +0x8 */ u32 size_8;
    /* +0xC */ u8* data_C;
} PatAgreementPage;   /* size: 0x10 */

/* One binary transfer chunk as the session handlers receive it. */
typedef struct PatBinaryChunk {
    /* +0x0 */ u32 fileHandle_0;
    /* +0x4 */ u32 offset_4;
    /* +0x8 */ u32 size_8;
    /* +0xC */ u8* data_C;   /* the call stack's copy of the chunk */
} PatBinaryChunk;   /* size: 0x10 */

/* The type byte of a request item (`putItem*`): a byte, a halfword, a word, a string and a byte array (both with a
 * 16-bit length). */
enum PatRequestItemType {
    PAT_ITEM_BYTE = 1,
    PAT_ITEM_HALF = 2,
    PAT_ITEM_WORD = 3,
    PAT_ITEM_STRING = 5,
    PAT_ITEM_BINARY = 6
};

/* The login block at +0x82AC as the login items read it (`putSomethingList`, `putItemAny`): the two id words, the
 * byte that selects which name is sent, the name, the user id and the 0x644-byte credentials blob. */
typedef struct PatLoginBlock {
    /* +0x000 */ u32  fields_00[2];
    /* +0x008 */ u8   nameSet_08;        /* 0 sends `name_09`, else the mediator's reflect name */
    /* +0x009 */ char name_09[0x20];
    /* +0x029 */ char userId_29[0x2C];
    /* +0x055 */ u8   blob_55[0x644];
} PatLoginBlock;   /* size: 0x69C (0x699 and the word alignment's tail) */


#ifdef __cplusplus
extern "C" {
#endif

/* the channel requests the GameSpy band sends */
u32 sendReqChannelInfo(NetworkInstance* self, u32 handle);
u32 sendReqChannelData(NetworkInstance* self, u32 handle, u32 offset, u32 size);
u32 sendReqConnect(NetworkInstance* self);

/* The layer requests: each writes its op-code and returns the request id (the callee narrows it to 16
 * bits, but its caller stores the full register, so the declared type is the wide one - playbook 66). */
u32 sendReqLayerUp(NetworkInstance* self);
u32 sendReqLayerChildInfo(NetworkInstance* self, s16 layer_id, u32 unused_arg);
u32 sendReqLayerUserList(NetworkInstance* self);

/* The request emitters the session state machine (`Network/network_state.cpp`) drives. */
u32 sendReqAuthenticationToken(NetworkInstance* self, const char* token);
u32 sendReqMaintenance(NetworkInstance* self);
u32 sendReqTermsVersion(NetworkInstance* self);
u32 sendReqTerms(NetworkInstance* self, u32 a, u32 b, u32 len);
u32 sendReqAnnounce(NetworkInstance* self);
u32 sendReqNoCharge(NetworkInstance* self);
u32 sendReqVulgarityInfoLow(NetworkInstance* self, s32 mode);
u32 sendReqVulgarityLow(NetworkInstance* self, s32 mode, u32 slice, u32 offset, u32 length);
u32 sendReqLmpConnect(NetworkInstance* self);
u32 sendReqMediaVersionInfo(NetworkInstance* self);
u32 sendReqRfpConnect(NetworkInstance* self);
u32 sendReqFmpListVersion(NetworkInstance* self);
u32 sendReqFmpListHead(NetworkInstance* self, s32 first, s32 count);
u32 sendReqFmpListData(NetworkInstance* self, u32 start, u32 count);
u32 sendReqFmpListFoot(NetworkInstance* self);
u32 sendReqFmpInfo(NetworkInstance* self, u32 value, s32 brief);
/* The layer session's requests (op-codes 89..115). */
u32 sendReqLayerStart(NetworkInstance* self);
u32 sendReqLayerEnd(NetworkInstance* self);
u32 sendReqLayerJump(NetworkInstance* self, const u8* path, u32 value);
u32 sendReqLayerCreateHead(NetworkInstance* self, s16 layerId);
u32 sendReqLayerCreateSet(NetworkInstance* self, s16 layerId, PatLayerData* layer, PatTagList* tags);
u32 sendReqLayerCreateFoot(NetworkInstance* self, s16 layerId, u8 move);
u32 sendReqLayerDown(NetworkInstance* self, s16 layerId, PatLayerData* layer);
u32 sendReqLayerJumpReady(NetworkInstance* self, const u8* path, u32 value);
u32 sendReqLayerJumpGo(NetworkInstance* self, const u8* path, u32 value);
u32 sendReqLayerInfoSet(NetworkInstance* self, PatLayerData* layer, PatTagList* tags);
u32 sendReqLayerInfo(NetworkInstance* self, const u8* path, const PatTagList* tags, s8 mode);
u32 sendReqLayerParentInfo(NetworkInstance* self, const PatTagList* tags);
u32 sendReqLayerChildListHead(NetworkInstance* self, s32 first, s32 count);
u32 sendReqLayerChildListData(NetworkInstance* self, s32 first, s32 count);
u32 sendReqLayerChildListFoot(NetworkInstance* self);
u32 sendReqLayerSiblingListHead(NetworkInstance* self, s32 first, s32 count);
u32 sendReqLayerSiblingListData(NetworkInstance* self, s32 first, s32 count);
u32 sendReqLayerSiblingListFoot(NetworkInstance* self);
u32 sendReqLayerHost(NetworkInstance* self, const u8* path);
u32 sendReqLayerUserListData(NetworkInstance* self, s32 first, s32 count);
u32 sendReqLayerUserListFoot(NetworkInstance* self);
u32 sendReqLayerUserSearchData(NetworkInstance* self, s32 first, s32 count);
u32 sendReqLayerUserSearchFoot(NetworkInstance* self);
/* The binary transfer requests return the request id the reply is matched against. */
s32 sendReqBinaryHead(NetworkInstance* self, u8 fileId, s8 mode);
s32 sendReqBinaryData(NetworkInstance* self, u8 fileId, u32 handle, u32 offset, u32 size);
s32 sendReqBinaryFoot(NetworkInstance* self, u8 fileId);
u32 sendReqCircleInfoNoticeSet(NetworkInstance* self);
u32 reqUserSearchInfoMine(NetworkInstance* self, s32 mode);

/* The item writers and the hand-off dispatch the state machine calls with its own view of the session. */
void writeUInt8Array2(NetworkStateMachine* self, u8 count, const u8* data);
/* The request item writers: a layer path, a layer's tagged items, a tag list, a 2-byte array, the off-by-one
 * counters the server reads 1-based (0x8040F068/0x8040F05C, GUESS names) and the two fixed item lists the layer
 * requests ask for (0x8040F1B0 the layer items, 0x8040F208 the user items; GUESS names). */
void writeUnkShortArray(NetworkStateMachine* self, const u8* path);
void writeLayerDownData(NetworkStateMachine* self, PatLayerData* layer, u8 count, const u8* tags);
void writeUnkByteIntStruct(NetworkStateMachine* self, PatTagList* tags);
void writeUnk2ByteArray(NetworkStateMachine* self, const PatTagList* tags);
void writeShortPlusOne(NetworkStateMachine* self, s16 value);
void writeBytePlusOne(NetworkStateMachine* self, s8 value);
void writeLayerItemRequest(NetworkStateMachine* self);
void writeUserItemRequest(NetworkStateMachine* self);
void putItemAny(NetworkStateMachine* self, const u8* values, u32 count, const u8* tags);
void putItemTaggedLongs(NetworkStateMachine* self, const u32* values, u8 count, const u8* tags);
void putItemTaggedBytes(NetworkStateMachine* self, const u8* values, u8 count, const u8* tags);
void putUserSlotObjects(NetworkStateMachine* self, const u8* row, u8 count, const u8* tags);
void dispatchSessionHandlers(NetworkStateMachine* self, u32 code, const u8* requestId, s32 value, u32 count, const u8* data);

/* The packet body readers the handlers call: the login block's tagged items at +0x82AC, `count` user rows. */
void readChargeInfo(NetworkStateMachine* self, u8* loginFields);
void readUserObjects(NetworkStateMachine* self, NetworkUserRow* rows, s32 count);
/* Reads `count` FMP slots into `slots` (the server-name block of each into `reserve`). */
void readFmpCompoundData(NetworkStateMachine* self, NetworkFmpSlot* slots, PatServerBlock* reserve, s32 count);
/* Reads `count` media version records (item 2 is the text `getStr1` returns; `unused` is not read). */
void readMediaVersionData(NetworkStateMachine* self, u8* unused, s32 count);
/* 0x80410C44 - reads one slice of a body transfer (its offset and length) into `buffer`: -1 (and a kept failure)
 * when the offset is not the next one or the buffer is full, else 0 (GUESS name). */
s32 readBodySlice(NetworkStateMachine* self, u8* buffer, u32 capacity);
/* The record readers: layers, the counting layer form (0x80411074, GUESS name: the user-number notice reads it),
 * the layer path, tag lists, layer users, and the off-by-one counters the server sends 1-based. */
void readLayerData(NetworkStateMachine* self, PatLayerData* layers, s32 count);
void readLayerCountData(NetworkStateMachine* self, PatLayerData* layers, s32 count);
void readUnkShortArrayStruct(NetworkStateMachine* self, u8* path);
void readUnkByteIntStruct(NetworkStateMachine* self, PatTagList* lists, s32 count);
void readLayerUserData(NetworkStateMachine* self, PatLayerUser* users, s32 count);
void readShortMinusOne(NetworkStateMachine* self, s16* out);
void readByteMinusOne(NetworkStateMachine* self, s8* out);
/* 0x80411478 - reads a 1-based circle slot (a twin of `readByteMinusOne`; GUESS name). */
void readCircleSlot(NetworkStateMachine* self, s8* out);
/* The item readers (`PatInterface:getItem*()` in their log strings): each checks the item's type byte (logging a bad
 * one) and reads the value; `getItemAny` skips an item of a tag the reader does not know, and the `Char`/`Long_`
 * forms store the signed value.  0x804107F0 reads the memory check request's two-byte records and 0x804117D0 a time
 * the server sends relative to its own clock (GUESS names). */
void getItemAny(NetworkStateMachine* self);
void getItemByte(NetworkStateMachine* self, u8* out);
void getItemWord(NetworkStateMachine* self, u16* out);
void getItemLong(NetworkStateMachine* self, u32* out);
void getItemLongLong(NetworkStateMachine* self, u64* out);
void getItemChar(NetworkStateMachine* self, s8* out);
void getItemLong_(NetworkStateMachine* self, s32* out);
void getItemString(NetworkStateMachine* self, u32* length, char* buffer, u16 size);
void getItemBinary(NetworkStateMachine* self, u32* length, u8* buffer, u16 size);
void readMemoryCheckData(NetworkStateMachine* self, u8* records, s32 count);
void readTimeOffset(NetworkStateMachine* self, u32* out);
/* 0x80412188 / 0x804122DC (GUESS names) - one step of the connection's open (on success: crypt on or off, the chosen
 * FMP slot, the server's address copied into its block; on failure the next FMP server or the error event) and of its
 * close (event 0x8005 once closed); `stepPatInterface` drives both. */
void stepPatConnect(NetworkStateMachine* self);
void stepPatDisconnect(NetworkStateMachine* self);
void readChatData(NetworkStateMachine* self, PatChatInfo* senders, s32 count);
void readMediationData(NetworkStateMachine* self, PatMediation* entries, s32 count);
void readCircleInfoDataArray(NetworkStateMachine* self, PatCircleInfo* circles, s32 count);
void readCircleMatchData(NetworkStateMachine* self, PatCircleMatch* members, s32 count);
void readNtcCompoundData(NetworkStateMachine* self, PatNtcCompound* senders, s32 count);
void readUserSearchData(NetworkStateMachine* self, PatUserSearchRow* rows, s32 count);
/* 0x80411BD8 - reads `count` 7-byte user status records (GUESS name). */
void readUserStatusData(NetworkStateMachine* self, u8* status, s32 count);
void readFriendData(NetworkStateMachine* self, PatFriend* friends, s32 count);
void readBlackListData(NetworkStateMachine* self, PatBlackListEntry* entries, s32 count);
void readAgreementPageData(NetworkStateMachine* self, PatAgreementPageInfo* pages, s32 count);
/* 0x80411FC4 - reads `count` agreement page info records' items (GUESS name). */
void readAgreementInfoData(NetworkStateMachine* self, PatAgreementHead* infos, s32 count);
/* The received item lists (GUESS names from the bodies): 0x8040E7F4 binds `capacity` rows at `items` and marks the
 * list; 0x8041043C reads the list's marker, count and typed items (strings and byte arrays into `buffer`, at most
 * `size` bytes); 0x803FE4D8 takes the rows off the call stack (at most 64, 8-byte aligned: `reserved` carries the
 * alignment in and the padding out) and binds them; 0x803FE590 gives the rows and the padding back. */
void initItemList(NetworkStateMachine* self, PatItemList* list, PatItem* items, u8 capacity);
void getFo(NetworkStateMachine* self, PatItemList* list, u8* buffer, u16 size);
void createItemListStack(NetworkStateMachine* self, PatItemList* list, u32* reserved);
void releaseItemListStack(NetworkStateMachine* self, PatItemList* list, u32 reserved);
/* Writes `count` tagged items of the login block (`tags` selects each one). */
void putSomethingList(NetworkStateMachine* self, const u8* loginFields, u8 count, const u8* tags);
/* The item writers the request builders call (GUESS names from the records each one reads): an item list by value,
 * a search's filters, the memory check reply's bytes, a layer user record, a mediation entry, a circle info block
 * (with the tags it builds from the set fields), match options, a chat's sender block, a user search query, the user
 * status record, a black list entry, and the fixed item lists the requests ask for. */
void putItemByte(NetworkStateMachine* self, u8 value);
void putUInt16(NetworkStateMachine* self, u16 value);
void putItemLong(NetworkStateMachine* self, u32 value);
void putItemByte_(NetworkStateMachine* self, s32 value);
void putItemLong2(NetworkStateMachine* self, u32 value);
void putItemString(NetworkStateMachine* self, const char* text);
void putItemBinary(NetworkStateMachine* self, const u8* data, u16 size);
void writeAny(NetworkStateMachine* self, const PatItemList* list);
void putItemList(NetworkStateMachine* self, PatItemList list);
void writeSearchFilters(NetworkStateMachine* self, const PatCircleFilter* filters, s32 count, u32 mode);
void putMemoryCheckBytes(NetworkStateMachine* self, const u8* values, u8 count, const u8* tags);
void writeLayerUserItems(NetworkStateMachine* self, const PatLayerUser* record, u8 count, const u8* tags);
void writeLayerUserItemRequest(NetworkStateMachine* self);
void writeMediationItems(NetworkStateMachine* self, const PatMediation* entry, u8 count, const u8* tags);
void writeMediationItemRequest(NetworkStateMachine* self);
void writeCircleInfoItems(NetworkStateMachine* self, const PatCircleInfo* info);
void putCircleInfoItems(NetworkStateMachine* self, const PatCircleInfo* info, u8 count, const u8* tags);
void writeCircleItemRequest(NetworkStateMachine* self);
void writeCircleUserItemRequest(NetworkStateMachine* self);
void writeMatchOptionItems(NetworkStateMachine* self, const PatMatchOptions* options, u8 count, const u8* tags);
void writeChatOptionItems(NetworkStateMachine* self, const PatMatchOptions* options, u8 count, const u8* tags);
void writeCompoundLayerBinary(NetworkStateMachine* self, const u8* sender, u8 count, const u8* tags);
void writeUserSearchItems(NetworkStateMachine* self, const u8* query, u8 count, const u8* tags);
void writeUserStatusItems(NetworkStateMachine* self, const u8* status, u8 count, const u8* tags);
void writeFriendItemRequest(NetworkStateMachine* self);
void writeBlackListItems(NetworkStateMachine* self, const u32* options, u8 count, const u8* tags);
void writeBlackListItemRequest(NetworkStateMachine* self);
void writeChannelItemRequest(NetworkStateMachine* self);
void writeChannelDataItemRequest(NetworkStateMachine* self);

/* The circle (lobby room) requests the Pat session manager sends: each writes its op-code and returns the
 * request id (stored whole by the caller - playbook 66); the `sendNtc*` notices return nothing.  Names
 * marked GUESS come from the manager's call sites: the List
 * Head/Data/Foot triple is the op-code run 0xD0/0xD2/0xD4, Kick 0xD6 sends a user id, Chat 0xE8 has no
 * reply slot (0xE9 follows it), the three value notices wrap `sendNtcCircleBinary` (kinds 1/2 and 3)
 * and its addressed twin. */
u32 sendReqCircleCreate(NetworkInstance* self, PatCircleInfo* info, PatCircleOptionList* options);   /* 0x804025B4 */
u32 sendReqCircleInfo(NetworkInstance* self, s32 circleId, s32 mode);                                /* 0x80402630 */
u32 sendReqCircleJoin(NetworkInstance* self, PatCircleInfo* info);                                   /* 0x804026B4 */
u32 sendReqCircleLeave(NetworkInstance* self, s32 circleId);                                         /* 0x8040275C */
u32 sendReqCircleMatchOptionSet(NetworkInstance* self, PatMatchOptions* options);                    /* 0x804027C0 */
u32 sendReqCircleMatchStart(NetworkInstance* self);                                                  /* 0x80402880 */
u32 sendReqCircleMatchEnd(NetworkInstance* self, s32 mode);                                          /* 0x804028CC */
u32 sendReqCircleInfoSet(NetworkInstance* instance, u32 request_id, PatCircleInfo* info, const char* name); /* 0x80402930 */
u32 sendReqCircleListLayer(NetworkInstance* self);                                                   /* 0x804029AC */
u32 sendReqCircleListHead(NetworkInstance* self, s32 mode, s32 count, PatCircleFilter* filters, s32 filterCount,
                          u32 flag);                                                       /* 0x80402A64 (GUESS) */
u32 sendReqCircleListData(NetworkInstance* self, s32 first, s32 count);                   /* 0x80402B04 (GUESS) */
u32 sendReqCircleListFoot(NetworkInstance* self);                                         /* 0x80402B80 (GUESS) */
u32 sendReqCircleKick(NetworkInstance* self, const char* userId);                         /* 0x80402BCC (GUESS) */
u32 sendReqCircleHost(NetworkInstance* self, s32 circleId);                                          /* 0x80402C64 */
u32 sendReqCircleUserList(NetworkInstance* self);                                                    /* 0x80402CC8 */
u32 sendNtcCircleChat(NetworkInstance* self, PatMatchOptions* options, const char* text);  /* 0x80402E5C (GUESS) */
u32 sendReqCircleTell(NetworkInstance* self, const char* userId, PatMatchOptions* options,
                      const char* text);                                                  /* 0x80402EE8 (GUESS) */
void sendNtcCircleUserValue(NetworkInstance* self, s32 circleId, s32 value, u8 notify);     /* 0x80402FD0 (GUESS) */
void sendNtcCircleUserValueReply(NetworkInstance* self, s32 circleId, s32 value,
                                 const u8* address);                                      /* 0x804030A8 (GUESS) */
void sendNtcCircleMatchState(NetworkInstance* self, s32 circleId, s32 state);               /* 0x80403174 (GUESS) */

/* The user search, user binary and friend requests (op-codes 252..285; the names of 276/279/285 are GUESSES from the
 * op-code order). */
u32 sendReqUserSearchSet(NetworkInstance* self, PatTagList* tags);
u32 sendReqUserBinarySet(NetworkInstance* self, u32 value, const u8* data, u32 size);
u32 sendReqUserBinaryNotice(NetworkInstance* self, u8 kind, const char* text, u32 a, u32 b);
u32 sendReqUserSearchData(NetworkInstance* self, s32 first, s32 count);
u32 sendReqUserSearchFoot(NetworkInstance* self);
u32 sendReqFriendAccept(NetworkInstance* self, const char* userId, u8 accept);
u32 sendReqFriendDelete(NetworkInstance* self, const char* userId);
u32 sendReqBlackDelete(NetworkInstance* self, const char* userId);

/* 0x80400F28 (GUESS) - request op 0x47: the checksum of file `fileId`; returns the request id. */
s32 sendReqBinaryChecksum(NetworkInstance* self, u8 fileId);

/* 0x80401AF4 - request op 136, between sendReqLayerHost and sendReqLayerUserList (recvAnsLayerUserInfoSet's slot) */
u32 sendReqLayerUserInfoSet(NetworkInstance* self, const NetLayerUserRecord* record);
/* 0x80401F50 */
u32 sendNtcLayerUserPosition(NetworkInstance* self, const NetUserPosition* position);
/* 0x80402090 */
u32 sendNtcLayerChat(NetworkInstance* self, u8 channel, const PatMatchOptions* options, const char* text);
/* 0x8040211C - request op 159 after sendNtcLayerChat, as sendReqCircleTell follows sendNtcCircleChat (recvAnsLayerTell) */
u32 sendReqLayerTell(NetworkInstance* self, const u8* userId, const PatMatchOptions* options, const char* text);
/* 0x80402248 */
u32 sendReqLayerMediationLock(NetworkInstance* self, u8 lock, u8 key);
/* 0x804022E4 */
u32 sendReqLayerMediationUnlock(NetworkInstance* self, u8 lock, u8 key);
/* 0x80402380 */
u32 sendReqLayerMediationList(NetworkInstance* self, u8 mode, u8 count);
/* 0x80402404 */
u32 sendReqLayerDetailSearchHead(NetworkInstance* self, u32 kind, u32 mode, s32 count, const NetLayerFilter* filters, s32 filterCount);
/* 0x804024EC */
u32 sendReqLayerDetailSearchData(NetworkInstance* self, s32 first, s32 count);
/* 0x80402568 */
u32 sendReqLayerDetailSearchFoot(NetworkInstance* self);
/* 0x80403230 */
void sendNtcLayerUserTransfer(NetworkInstance* self, u32 state, const u8* userId, u32 active);

/* 0x8040354C */
s32 sendReqTell(NetworkInstance* self, const u8* id, const u32* options, const char* text);
/* 0x804035D8 */
s32 sendReqBinaryUser(NetworkInstance* self, const u8* id, const u8* data, u16 size);
/* 0x80403978 */
s32 sendReqUserSearchInfo(NetworkInstance* self, const u8* query, s8 mode);
/* 0x80403A88 */
s32 sendReqUserStatusSet(NetworkInstance* self, const u8* settings);
/* 0x80403BF4 */
s32 sendReqFriendAdd(NetworkInstance* self, const u8* id, const u32* options, const char* text);
/* 0x80403D60 */
s32 sendReqFriendList(NetworkInstance* self, s32 mode, s32 max);
/* 0x80403DE4 */
s32 sendReqBlackAdd(NetworkInstance* self, const u8* id, const u32* options);
/* 0x80403EDC */
s32 sendReqBlackList(NetworkInstance* self, s32 mode, s32 max);

#ifdef __cplusplus
}
#endif

/* ==== the request state machine (the former `Network/network_state.cpp`) ===================================== */

/* The 3-byte payload `sendServerTimeout` builds from the two unowned constants. */
typedef struct SessionTimeoutPayload {
    /* +0x00 */ u16 code_00;
    /* +0x02 */ u8  reason_02;
    /* +0x03 */ u8  pad_03;
} SessionTimeoutPayload;   /* size: 0x04 */

/* `NetworkPostedError` (0x0C B) is defined in `unsplit/Network.h`; `PatInterface::postError` takes it by value. */

#ifdef __cplusplus
extern "C" {
#endif

s32  sendReqShut(NetworkInstance* self, s32 mode);
s32  resetNetworkState3(NetworkInstance* self);
/* 0x803FE8E4 / 0x803FF024 - the state machine's reset and its gated step out of state 5. */
s32  resetNetworkState(NetworkInstance* self);
s32  advanceNetworkState5(NetworkInstance* self);
/* 0x803FE95C - one step of the login sub-machine; nonzero once it has finished. */
s32  handleNetworkState1(NetworkInstance* self);
/* 0x804004C8 / 0x804004D8 - the FMP list query's reset and one step of it (`NetworkLayerPat::handleServerList`). */
s32  resetNetworkState4(NetworkInstance* self);
s32  handleNetworkState4(NetworkInstance* self, s32 arg);

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

/* 0x803FE03C - keeps the selected FMP slot and its reserve record while the server type is 1, else -1
 * (GUESS name, NetworkLayerPat's handleServerSelect calls it before another server is chosen). */
s32 saveFmpSelection(NetworkInstance* self);
/* 0x803FE0AC - while the server type is 1 and a selection is kept, restores it (the next server is chosen, the kept
 * slot and reserve record put back and the server type re-applied) and returns the slot, else -1 (GUESS name:
 * `saveFmpSelection`'s inverse). */
s32 restoreFmpSelection(NetworkInstance* self);
/* 0x803FE154 - while the server type is 1, whether the PAT phase reads 5, else -1 (GUESS name: the caller reports
 * the layer's server-rejected error when it is non-zero). */
s32 isFmpServerRejected(NetworkInstance* self);
/* 0x803FE5A0 - appends one tagged item (`type` 0..9; a value pointer, plus `size` for the sized kind) to an item list
 * (GUESS name, the `createItemListStack`/`releaseItemListStack` siblings' scheme). */
void appendItemList(NetworkStateMachine* self, PatItemList* list, u8 tag, u8 type, const u8* value, u16 size);
/* 0x80401BD4 */
u32 sendReqLayerUserListHead(NetworkInstance* self, u8 kind, const u8* path, u32 first, u32 count);
/* 0x80401D30 */
u32 sendReqLayerUserSearchHead(NetworkInstance* self, u8 kind, const u8* path, u32 first, u32 count, const char* userId, const char* name, const NetLayerFilter* filters, s32 filterCount);
/* 0x80401EC8 - sends op 154: the sender's compound record and the item list, the shape `recvNtcLayerBinary` reads
 * (name from that receiver). */
u32 sendNtcLayerBinary(NetworkInstance* self, PatItemList list);
/* 0x80403460 - packs (from, to, state) as four items and sends them with `sendNtcLayerBinary` (GUESS name: the
 * caller is NetworkLayerPat's NAT pair-state update). */
void sendNtcLayerBinaryNatState(NetworkInstance* self, u32 from, u32 to, s8 state);
/* 0x804021A8 / 0x80402D1C / 0x80402DBC - send an item list to one layer member (op 165), to a circle (op 228) and
 * to one circle member (op 230), each after an empty sender block (GUESS names from the op-code order and the
 * value notices that wrap them). */
u32 sendNtcLayerBinaryTo(NetworkInstance* self, PatItemList list, const char* userId);
u32 sendNtcCircleBinary(NetworkInstance* self, s32 circleId, PatItemList list);
u32 sendNtcCircleBinaryTo(NetworkInstance* self, s32 circleId, PatItemList list, const char* userId);
/* 0x80403304 - packs a kind byte and the 14-byte record (three words, a halfword) as five items and sends them: to
 * the whole layer with `sendNtcLayerBinary` (kind 6) when `broadcast` is set, else to the member whose exported
 * id is `userId` through 0x804021A8 (kind 7).  NAME (a GUESS): its caller is NetworkLayerPat's 0x803EEBA4 with the
 * layer's per-slot record at +0xC1B0. */
void sendLayerBinaryRecord(NetworkInstance* self, u32 a, u32 b, u32 c, u16 d, const u8* userId, s32 broadcast);
/* 0x804037EC */
u32 sendReqUserSearchHead(NetworkInstance* self, u32 kind, u32 count, const char* userId, const char* name, const NetLayerFilter* filters, s32 filterCount, s8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATINTERFACE_H */
