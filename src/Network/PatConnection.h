/*
 * Network/PatConnection.h - the declarations of `Network/PatConnection.cpp`: the `PatConnection` class (the
 *   `PatInterface` base), the packet table and the connection's free entry points (the body readers and the request
 *   writers).
 */
#ifndef MHTRI_NETWORK_PATCONNECTION_H
#define MHTRI_NETWORK_PATCONNECTION_H

#include "types.h"
#include "Network/gamespy_interface_types.h"   /* NetworkErrorInfo - the error `postError` takes */
#include "Network/network_socket_streams.h"   /* NetworkPeerAddress, NetworkResolverBase */

class PatInterface;                       /* Network/PatInterface.h */
class NetworkSocketBase;                  /* Network/NetworkFileFetcher.h */

/* The 8-byte header of one received packet (the connection keeps it at +0x60AA): `recvCommand` matches the three
 * op-code bytes against the packet table, a status of -1/1 selects `recvAnsNg`/`recvAnsAlert`, and the handlers hand
 * the request id on to the session handlers. */
typedef struct PatPacketHeader {
    /* +0x00 */ u8 size_00[2];
    /* +0x02 */ u8 requestId_02[2];   /* copied out with `memcpy` - the packet is a byte stream */
    /* +0x04 */ u8 opcode_04[3];
    /* +0x07 */ s8 status_07;         /* -1 negative reply, 1 alert, else the handler's own */
} PatPacketHeader;   /* size: 0x08 */

/* The server a connection dials: the host name and the address it resolves to (`setServerAddress` fills both; a
 * non-zero address skips the name lookup). */
typedef struct PatServerInfo {
    /* +0x00 */ char host_00[0x80];
    /* +0x80 */ NetworkPeerAddress address_80;
} PatServerInfo;   /* size: 0x86 */

/* The Pat server connection, `PatInterface`'s base: the constructor 0x803FAE9C stores `__vt__13PatConnection`
 * (0x806006E8), whose slots are the destructor 0x803FAF34, `resetDefaults` 0x803FAF78 and 159 pure virtuals (null
 * words +0x10..+0x288: the packet handlers, `recvCommand` and `postError`, which `PatInterface` overrides).  The
 * destructor is the key function, so `Network/PatConnection.cpp` emits the table (rule 10). */
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
    /* +0x288 */ virtual s32 postError(NetworkErrorInfo* info) = 0;

    /* the by-value spelling: the caller's argument copy is what the virtual slot is handed */
    inline void postError(NetworkErrorInfo info) { postError(&info); }

    /* 0x803FB018 - one step of the connect state machine (+0x60D1: resolve, socket, the secure open when
     * `setSecureServer` armed it, connect); the failures fill `error` and log "PatConnection::connectServer ...
     * fail" (.data 0x80600500..), which names it. */
    s32 connectServer(NetworkErrorInfo* error);
    /* 0x803FB490 - one step of the connection's close: arms it (state 0 -> 10), then releases the resolver and
     * the socket and returns 1 once closed, else 0 (GUESS name from the body). */
    s32 disconnect();
    /* 0x803FB634 - receives what the socket holds into the receive buffer; "PatConnection::receiveCommand fail"
     * (.data 0x806005B8) names it. */
    s32 receiveCommand(NetworkErrorInfo* error);
    /* 0x803FB77C (GUESS name) - takes the next whole packet out of the receive buffer, decrypts it while
     * `cryptEnabled_60D0` is set ("PatCryptDecrypt fail") and hands its header to `recvCommand` (+0x284). */
    s32 dispatchCommand(NetworkErrorInfo* error);
    /* 0x803FC548 - fails when the handler read past the packet's declared size ("PatConnection::checkBufError"
     * names it). */
    s32 checkBufError();

    /* +0x0004 */ PatServerInfo server_0004;            /* the server `setServerAddress` names */
    /* +0x008A */ u8 pad_008A[0x02];
    /* +0x008C */ PatServerInfo* serverInfo_008C;       /* the server the connect dials (NULL: none set) */
    /* +0x0090 */ NetworkResolverBase* resolver_0090;   /* the name lookup while the connect resolves */
    /* +0x0094 */ NetworkSocketBase* socket_0094;       /* the connection's socket */
    /* +0x0098 */ u8 connected_0098;                    /* set once the connect completed */
    /* +0x0099 */ u8 pad_0099[0x03];
    /* +0x009C */ s32 sendSize_009C;                    /* bytes of `sendBuffer_00A0` waiting to be sent */
    /* +0x00A0 */ u8 sendBuffer_00A0[0x2000];
    /* +0x20A0 */ s32 recvSize_20A0;                    /* bytes of `recvBuffer_20A8` received */
    /* +0x20A4 */ s32 blockSize_20A4;                   /* the size `beginReadBlock` read */
    /* +0x20A8 */ u8 recvBuffer_20A8[0x4000];
    /* +0x60A8 */ u16 sequence_60A8;                    /* the last request id sent (seeded with `rand`) */
    /* +0x60AA */ PatPacketHeader header_60AA;          /* the packet being dispatched */
    /* +0x60B2 */ u8 pad_60B2[0x02];
    /* +0x60B4 */ u8* readCursor_60B4;                  /* the readers' position in the packet body */
    /* +0x60B8 */ u8* blockStart_60B8;                  /* the read position `beginReadBlock` saved */
    /* +0x60BC */ u8* packetStart_60BC;                 /* the request being written (its 8-byte header) */
    /* +0x60C0 */ u8* writeCursor_60C0;                 /* the writers' position in the request body */
    /* +0x60C4 */ const char* secureHost_60C4;          /* the secure server's host (empty: plain socket) */
    /* +0x60C8 */ const u8* rootCA_60C8;                /* its root certificate */
    /* +0x60CC */ s32 rootCASize_60CC;
    /* +0x60D0 */ u8 cryptEnabled_60D0;   /* the receive loop decrypts the body while set */
    /* +0x60D1 */ u8 opening_state;     /* the opening sub-step the mediator's progress reads (0 and 90 ignored) */
    /* +0x60D2 */ u8 connectionState_60D2;   /* `disconnect`'s state: 0 idle, 10 closing */
    /* +0x60D3 */ u8 pad_60D3;
};   /* size: 0x60D4 (the derived `PatInterface`'s first own field, the reference count, is at +0x60D4) */
typedef PatInterface NetworkStateMachine;  /* the state machine's spelling of the singleton */

/* A packet handler: a `PatConnection` virtual the table names (`recvCommand` calls it through `__ptmf_scall`). */
typedef s32 (PatConnection::*PatPacketHandler)(s32 index, const PatPacketHeader* header);

/* One row of the packet table: the three op-code bytes (the third is 1 for a request, 2 for its answer and 0x10
 * for a notice), the handler of a received packet (null for a packet this side only sends), and two descriptions,
 * the Japanese one and an empty one. */
typedef struct PatPacketEntry {
    /* +0x00 */ u8 opcode_00[3];
    /* +0x04 */ PatPacketHandler handler_04;
    /* +0x10 */ const char* description_10;
    /* +0x14 */ const char* label_14;
} PatPacketEntry;   /* size: 0x18 */

/* 0x805FE910 - the 297 packets of the Pat protocol and a zero terminator (GUESS name; `flushBuffer` and
 * `recvCommand` index it with the packet number). */
extern PatPacketEntry patPacketTable[298];

#ifdef __cplusplus
extern "C" {
#endif

/* The connection's free entry points: the map names them without a mangling.  `self` is the connection (the
 * request builders pass the `PatInterface` singleton). */

/* 0x803FB550 - names the server (`host` printed into the connection's own server record, the 4-byte address and
 * the port copied) and makes it the one `connectServer` dials. */
void setServerAddress(PatConnection* self, const char* host, const u8* address, const u16* port);
/* 0x803FB5CC / 0x803FB5E0 / 0x803FB5F0 (GUESS names from the bodies) - copy the server's 4-byte address to `out`;
 * store the host, root certificate and its size that `connectServer` hands the socket's `openSecure`; set the
 * PatCamellia key and raise `cryptEnabled_60D0`. */
void getServerAddress(PatConnection* self, u8* out);
void setSecureServer(PatConnection* self, const char* host, const u8* rootCA, s32 rootCASize);
void enableCrypt(PatConnection* self, const u8* key);
/* 0x803FB628 (GUESS name: `enableCrypt`'s inverse) - clears `cryptEnabled_60D0`. */
void disableCrypt(PatConnection* self);
/* 0x803FB9B0 - sends the queued requests ("PatConnection::sendCommand fail" names it): 0 when sent or nothing
 * was queued, negative with `error` filled on failure. */
s32 sendCommand(PatConnection* self, NetworkErrorInfo* error);

/* 0x803FBB04..0x803FBED0 - the packet body readers: each takes its value at the read cursor (multi-byte values
 * converted from network order) and advances the cursor. */
void readUInt8(PatConnection* self, u8* out);
void readUInt16(PatConnection* self, u16* out);
void readUInt32_(PatConnection* self, u32* out);
void readUInt64(PatConnection* self, u64* out);
void readUInt8_(PatConnection* self, s8* out);
/* the signed halfword reader (`readUInt16` into a temporary, stored as `s16`), named in its siblings' scheme
 * (`readUInt8_`/`readInt32`, `writeInt16`) */
void readInt16(PatConnection* self, s16* out);
void readInt32(PatConnection* self, s32* out);
/* a 16-bit length and that many bytes: at most `size - 1` (the string) or `size` (the array) are copied into the
 * cleared buffer and `*length` says how many; the cursor skips the whole field */
void readString(PatConnection* self, u32* length, char* buffer, u16 size);
void readUInt8Array(PatConnection* self, u32* length, u8* buffer, u16 size);
/* a block: its 16-bit size, then `endReadBlock` skips whatever of it the reader left */
void beginReadBlock(PatConnection* self, u16* size);
void endReadBlock(PatConnection* self);

/* 0x803FBEF0 - starts a request of packet `index` (its 8-byte header: a fresh request id, or the answered
 * request's for an answer, `flags` and the op-code); sends the queue first when less than 0x800 bytes are free.
 * Returns the request id, 0 when not connected or the send failed. */
u32  flushBuffer(PatConnection* self, s32 index, u8 flags);
/* 0x803FC084 - closes the request: encrypts its body while crypt is on, stores its size and queues it. */
void encryptBuffer(PatConnection* self);
/* 0x803FC210..0x803FC4B4 - the request body writers: each stores its value at the write cursor (multi-byte values
 * in network order) and returns where it went, NULL when not connected. */
u8*  writeUInt8(PatConnection* self, u8 value);
u8*  writeUInt16(PatConnection* self, u16 value);
u8*  writeUInt32(PatConnection* self, u32 value);
u8*  writeUInt64(PatConnection* self, u64 value);
u8*  writeBool(PatConnection* self, s8 value);
u8*  writeInt16(PatConnection* self, s16 value);
/* the 4-byte tail-branch twin of `writeUInt32` */
u8*  writeUInt32Shared(PatConnection* self, u32 value);
u8*  writeString(PatConnection* self, const char* text);
u8*  writeUInt8Array(PatConnection* self, const u8* data, u16 count);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATCONNECTION_H */
