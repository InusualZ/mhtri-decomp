/*
 * include/Network/network_layer_io.h - the declarations `src/Network/network_layer_io.cpp` owns
 * (`.text` 0x804006A8..0x80413450, the network layer's `sendReq*` request builders and their handlers).
 *
 * The unit has no bodies yet, so every parameter list is the one its callers' calls demonstrate.  Moved
 * here from the band header `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORK_LAYER_IO_H
#define MHTRI_NETWORK_NETWORK_LAYER_IO_H

#include "types.h"

typedef struct NetLayerFilter NetLayerFilter;           /* include/Network/NetworkLayerPat.h */
typedef struct NetUserPosition NetUserPosition;         /* include/Network/NetworkLayerPat.h */
typedef struct NetLayerUserRecord NetLayerUserRecord;   /* include/Network/NetworkLayerPat.h */
struct PatCircleOptionList;
struct PatMatchOptions;
typedef struct PatCircleFilter PatCircleFilter;   /* include/Network/NetworkSessionManager.h */

class PatInterface;                               /* include/Network/PatInterface.h */
typedef PatInterface NetworkInstance;             /* include/unsplit/Network.h: the band's alias */
typedef PatInterface NetworkStateMachine;          /* the state machine's spelling of the singleton */
typedef struct NetworkUserRow NetworkUserRow;     /* include/Network/PatInterface.h */
typedef struct NetworkFmpSlot NetworkFmpSlot;     /* include/Network/PatInterface.h */
typedef struct PatCircleInfo PatCircleInfo;       /* include/Network/NetworkSessionManager.h */
typedef struct NetworkWiiMediatorFields NetworkWiiMediatorFields;   /* include/Network/NetworkWiiMediator.h */
class NetworkWiiMediator;                         /* include/Network/NetworkWiiMediator.h */
class NetworkReflectService;                      /* include/Network/NetworkReflectService.h */

/* The network library's linear-congruential random generator (`sNetworkLibrary::mpRandom`): the constructor
 * 0x80413384 stores the table 0x806024A0 and the classic `rand` constants (seed 1, multiplier 0x41C64E6D,
 * increment 12345, result shift 16, mask 0x7FFF); 0x80413424 steps it.  Class name GUESSED.  size: 0x18 */
class NetworkRandom {
public:
    NetworkRandom();
    /* +0x08 */ virtual ~NetworkRandom();

    /* +0x04 */ u32 seed;
    /* +0x08 */ u32 multiplier;
    /* +0x0C */ u32 increment;
    /* +0x10 */ u32 shift;
    /* +0x14 */ u32 mask;
};

/* The 0x3A20-byte network singleton `getNetworkPool` returns (.sbss 0x80794CB8): the constructor 0x80412528
 * stores the table 0x80602490 and publishes itself, 0x8041275C clears its state (the +0x6B progress byte,
 * the +0x3A10 timestamp among it), and the mediator's forwarding wrappers create it with `new` (0x3A20).
 * Class name GUESSED from the runtime dump's `GetPool` on the accessor.  The virtual is declared and not
 * defined here, so no table is emitted by a consumer (rule 10).  size: 0x3A20 */
class NetworkPool {
public:
    /* +0x08 */ virtual ~NetworkPool();

    /* 0x804128C4 - resets the state and starts the EC (shop) sequence (GUESS name). */
    void start();
    /* 0x8041793C (`Network/NetworkWiiMediator.cpp`) - whether the EC sequence is running (+0x44). */
    BOOL isECStarted();

    /* +0x0004 */ u8  pad_0004[0x40];
    /* +0x0044 */ u8  ec_started;
    /* +0x0045 */ u8  pad_0045[0x26];
    /* +0x006B */ u8  progress;        /* the opening progress the mediator adds to 90 while step 6 runs */
    /* +0x006C */ u8  pad_006C[0x39A4];
    /* +0x3A10 */ u64 timestamp;       /* the server timestamp the mediator stamps the account with */
    /* +0x3A18 */ u8  pad_3A18[0x08];
};

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
    /* +0x000 */ u8  pad_000[0x04];
    /* +0x004 */ u8   path_004[0x10];   /* `readUnkShortArrayStruct` fills it before the layer's own items */
    /* +0x014 */ char name_014[0x40];   /* item 3 of the layer requests (`writeLayerDownData`) */
    /* +0x054 */ s16 layerId_054;      /* the id `recvAnsLayerChildInfo` read first */
    /* +0x056 */ u8  pad_056[0x27];
    /* +0x07D */ s8  isCurrent_07D;    /* selects `recvAnsLayerInfo`'s event (GUESS) */
    /* +0x07E */ u8  pad_07E[0xC0];
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
    /* +0x028 */ u8   path_028[0x50];    /* the host answers' layer path (`readUnkShortArrayStruct`) */
    /* +0x078 */ u8   pad_078[0xC8];
} PatLayerUser;   /* size: 0x140 (the handlers clear 320 bytes) */

/* One row of the layer user lists: the user and its tag list. */
typedef struct PatLayerUserInfo {
    /* +0x000 */ PatLayerUser user_000;
    /* +0x140 */ PatTagList   tags_140;
} PatLayerUserInfo;   /* size: 0x244 (the list answers allocate 580 bytes a row) */

/* The sender of a chat or tell (`readChatData`). */
typedef struct PatChatInfo {
    /* +0x00 */ u32  value_00;
    /* +0x04 */ u8   pad_04[0x04];
    /* +0x08 */ char userId_08[0x08];   /* the tells read it before the sender's items */
    /* +0x10 */ u8   pad_10[0x20];
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
    /* +0x9 */ u8   pad_9;
} PatMediation;   /* size: 0xA (the notices clear 10 bytes, the list answer 32 of them) */

/* One circle (a hunting party) as the circle answers read it: its id, the info items and the tag list. */
typedef struct PatCircleEntry {
    /* +0x000 */ s32        circleId_000;
    /* +0x004 */ u8         pad_004[0x378];
    /* +0x37C */ PatTagList tags_37C;
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
    /* +0x00 */ u8   flag_00;
    /* +0x01 */ u8   pad_01[0x03];
    /* +0x04 */ u16  value_04;
    /* +0x06 */ u8   pad_06;
    /* +0x07 */ s8   slot_07;    /* sent 1-based (`readCircleSlot`) */
    /* +0x08 */ char userId_08[0x08];
    /* +0x10 */ u8   pad_10[0x20];
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
    /* +0x00 */ u32  value_00;
    /* +0x04 */ char userId_04[0x08];   /* the user notice reads it first */
    /* +0x0C */ u8   pad_0C[0x20];
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
typedef struct PatUserSearch {
    /* +0x000 */ u8         pad_000[0x228];
    /* +0x228 */ u32        value_228;    /* `recvAnsUserSearchInfoMine` keeps both values */
    /* +0x22C */ u32        value_22C;
    /* +0x230 */ PatTagList tags_230;
} PatUserSearch;   /* size: 0x334 (the list answer allocates 820 bytes a row) */

/* One friend (`readFriendData`). */
typedef struct PatFriend {
    /* +0x00 */ u32  value_00;
    /* +0x04 */ char userId_04[0x08];
    /* +0x0C */ u8   pad_0C[0x20];
    /* +0x2C */ s8   state_2C;   /* the friend notice reads it last */
    /* +0x2D */ u8   pad_2D[0x03];
} PatFriend;   /* size: 0x30 */

/* One black list entry (`readBlackListData`). */
typedef struct PatBlackListEntry {
    /* +0x00 */ char userId_00[0x08];
    /* +0x08 */ u8   pad_08[0x24];
} PatBlackListEntry;   /* size: 0x2C */

/* The agreement's page info as the session handlers receive it (the page records on the call stack). */
typedef struct PatAgreementInfo {
    /* +0x00 */ u8  version_00;
    /* +0x01 */ u8  pad_01[0x0B];
    /* +0x0C */ u8  pageCount_0C;
    /* +0x0D */ u8  pad_0D[0x03];
    /* +0x10 */ u8* pages_10;   /* `pageCount_0C` records of 40 bytes */
} PatAgreementInfo;   /* size: 0x14 */

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

#ifdef __cplusplus
extern "C" {
#endif

/* the channel requests the GameSpy band sends */
void sendReqChannelInfo(NetworkInstance* self, u32 handle);
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
void sendReqCircleInfoNoticeSet(NetworkInstance* self);
void reqUserSearchInfoMine(NetworkInstance* self, s32 mode);

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
void readFmpCompoundData(NetworkStateMachine* self, NetworkFmpSlot* slots, u8* reserve, s32 count);
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
void readChatData(NetworkStateMachine* self, PatChatInfo* senders, s32 count);
void readMediationData(NetworkStateMachine* self, PatMediation* entries, s32 count);
void readCircleInfoDataArray(NetworkStateMachine* self, PatCircleEntry* circles, s32 count);
void readCircleMatchData(NetworkStateMachine* self, PatCircleMatch* members, s32 count);
void readNtcCompoundData(NetworkStateMachine* self, PatNtcCompound* senders, s32 count);
void readUserSearchData(NetworkStateMachine* self, PatUserSearch* rows, s32 count);
/* 0x80411BD8 - reads `count` 7-byte user status records (GUESS name). */
void readUserStatusData(NetworkStateMachine* self, u8* status, s32 count);
void readFriendData(NetworkStateMachine* self, PatFriend* friends, s32 count);
void readBlackListData(NetworkStateMachine* self, PatBlackListEntry* entries, s32 count);
void readAgreementPageData(NetworkStateMachine* self, u8* pages, s32 count);
/* 0x80411FC4 - reads `count` agreement page info records' items (GUESS name). */
void readAgreementInfoData(NetworkStateMachine* self, PatAgreementInfo* infos, s32 count);
/* Writes `count` tagged items of the login block (`tags` selects each one). */
void putSomethingList(NetworkStateMachine* self, const u8* loginFields, u8 count, const u8* tags);

/* The pool singleton the mediator band forwards to. */
NetworkPool* getNetworkPool(void);

/* The circle (lobby room) requests the Pat session manager sends: each writes its op-code and returns the
 * request id (stored whole by the caller - playbook 66); the `sendNtc*` notices return nothing.  Names
 * marked GUESS come from the manager's call sites (`.pi/outbox/net2-l2-101a-requests.json`): the List
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
void sendNtcCircleChat(NetworkInstance* self, PatMatchOptions* options, const char* text);  /* 0x80402E5C (GUESS) */
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
void sendNtcLayerUserPosition(NetworkInstance* self, const NetUserPosition* position);
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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_LAYER_IO_H */
