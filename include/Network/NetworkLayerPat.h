/*
 * include/Network/NetworkLayerPat.h - the `NetworkLayerPat` view the network units need.
 *
 * `NetworkLayerPat` is the +0x0C element of the `NetworkPat` holder (include/Network/NetworkPat.h):
 * the class `the layer constructor (0x803E0C18)` builds, whose vtable is `the layer vtable (0x805FC1E0)`
 * (0x805FC1E0, 0x144 B) and whose own method names ("NetworkLayerPat::move ...") come from the pool
 * strings in the band's `.data`.  Only the fields the units read are named; the rest is padding at its
 * real offset.  The table is dispatched through, never built here (rule 10): the class **declares** its
 * virtuals - a declared virtual at index i is at +8+4*i - and defines none, so MWCC emits no table of
 * ours; the slots between the called ones are `pad_NN`.  The size is approximate: the constructor
 * builds members out to +0x7E128.
 */
#ifndef MHTRI_NETWORK_NETWORKLAYERPAT_H
#define MHTRI_NETWORK_NETWORKLAYERPAT_H

#include "types.h"
#include "Network/NetworkSessionManager.h"   /* NetworkRequest - the record the state machine advances */
#include "Network/NetworkLayer.h"            /* NetworkLayer, NetworkLayerRequest, NetworkLayerId - the base this class builds on */

/* The 10-byte network user id the friend/peer code copies and compares (`importNetId` imports one,
 * `isSameNetId` compares two, `formatNetId` renders one as text).  size: 0xA */
typedef struct NetId {
    /* +0x00 */ char text_0x00[0xA];
} NetId; /* size: 0xA */

/* One entry of a `NetLayerRequest`: a slot number, an enable flag and the value the layer acts on. */
typedef struct NetLayerRequestItem {
    /* +0x00 */ u8 slot_0x00;
    /* +0x01 */ u8 enabled_0x01;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ s32 flag_0x04;
    /* +0x08 */ s32 value_0x08;
} NetLayerRequestItem; /* size: 0xC */

/* The request record `NetworkLayerPat::submitRequest_9C`/`submitSelect_A4` take (initialised by
 * `initNetLayerRequest`): a count and up to four items.  size: 0x34 */
typedef struct NetLayerRequest {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetLayerRequestItem items_0x04[4];
} NetLayerRequest; /* size: 0x34 */

/* The settings record `NetworkLayerPat::submitSettings_98` takes: a count of four and four
 * (enabled, value) pairs.  size: 0x24 */
typedef struct NetLayerSettings {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ struct {
        /* +0x0 */ s32 enabled_0x0;
        /* +0x4 */ s32 value_0x4;
    } pairs_0x04[4];
} NetLayerSettings; /* size: 0x24 */

/* One of the four packed settings a city record carries (`NetCtrlWk::settings_0x6210` takes the values). */
typedef struct NetCityValue {
    /* +0x00 */ u32 tag_0x00;
    /* +0x04 */ u32 value_0x04;
} NetCityValue; /* size: 0x8 */

/* One 0x7C-byte entry of the layer's city list (GUESS: the server-select "city" rows - id, name,
 * population against capacity, and the closed/hidden flags the list view maps to a state). */
typedef struct NetCityRec {
    /* +0x00 */ s32 id_0x00;
    /* +0x04 */ char name_0x04[0x44];
    /* +0x48 */ s32 population_0x48;
    /* +0x4C */ s32 capacity_0x4C;
    /* +0x50 */ s32 order_0x50;
    /* +0x54 */ u32 kind_0x54;         /* 4 = the record carries the four settings below */
    /* +0x58 */ NetCityValue values_0x58[4];
    /* +0x78 */ u8 closed_0x78;
    /* +0x79 */ u8 hidden_0x79;
    /* +0x7A */ u8 pad_0x7A[0x2];
} NetCityRec; /* size: 0x7C */

/* The city list: a count and the 40 entries (`NetworkLayerPat::cities_540`).  size: 0x1364 */
typedef struct NetCityList {
    /* +0x000 */ u32 count_0x000;
    /* +0x004 */ NetCityRec entries_0x004[40];
} NetCityList; /* size: 0x1364 */

/* One 0xB8-byte entry of the layer's room list (GUESS: id, name and a population pair). */
typedef struct NetRoomRec {
    /* +0x00 */ u8 pad_0x00[0x40];
    /* +0x40 */ s32 id_0x40;
    /* +0x44 */ char name_0x44[0x44];
    /* +0x88 */ s32 population_0x88;
    /* +0x8C */ s32 capacity_0x8C;
    /* +0x90 */ u8 pad_0x90[0x28];
} NetRoomRec; /* size: 0xB8 */

/* The room list: a count and the 40 entries (`NetworkLayerPat::rooms_18A4`).  size: 0x1CC4 */
typedef struct NetRoomList {
    /* +0x000 */ u32 count_0x000;
    /* +0x004 */ NetRoomRec entries_0x004[40];
} NetRoomList; /* size: 0x1CC4 */

/* One 0x38-byte entry of the layer's friend id table (`NetCtrlWk::hasFriendDetail` searches it by id). */
typedef struct NetFriendRec {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ u8 pad_0x0A[0x2B];
    /* +0x35 */ u8 valid_0x35;
    /* +0x36 */ u8 pad_0x36[0x2];
} NetFriendRec; /* size: 0x38 */

/* The friend id table: a count and the 100 entries (`NetworkLayerPat::friends_3568`).  size: 0x15E4 */
typedef struct NetFriendTable {
    /* +0x000 */ s32 count_0x000;
    /* +0x004 */ NetFriendRec entries_0x004[100];
} NetFriendTable; /* size: 0x15E4 */

/* One 0x104-byte detail record paired with the friend of the same index (GUESS on the base: only the
 * flag byte at +0x101 is read). */
typedef struct NetFriendDetail {
    /* +0x000 */ u8 pad_0x000[0x101];
    /* +0x101 */ u8 flag_0x101;
    /* +0x102 */ u8 pad_0x102[0x2];
} NetFriendDetail; /* size: 0x104 */

/* One 0x2510-byte entry of the layer's community list (the lobby's group rows); only the fields the
 * list view copies out are named. */
typedef struct NetCommunityRec {
    /* +0x0000 */ u8 pad_0x0000[0x40];
    /* +0x0040 */ char name_0x0040[0x20];
    /* +0x0060 */ char leader_0x0060[0x40];
    /* +0x00A0 */ char comment_0x00A0[0x40];
    /* +0x00E0 */ s32 value_0x00E0;
    /* +0x00E4 */ u8 pad_0x00E4[0x4];
    /* +0x00E8 */ s32 value_0x00E8;
    /* +0x00EC */ s32 value_0x00EC;
    /* +0x00F0 */ u8 pad_0x00F0[0xC];
    /* +0x00FC */ s32 slot_0x00FC;
    /* +0x0100 */ u8 pad_0x0100[0x4];
    /* +0x0104 */ s32 slot_0x0104;
    /* +0x0108 */ u8 pad_0x0108[0x4];
    /* +0x010C */ s32 slot_0x010C;
    /* +0x0110 */ u8 pad_0x0110[0x4];
    /* +0x0114 */ s32 slot_0x0114;
    /* +0x0118 */ u8 pad_0x0118[0x23F8];
} NetCommunityRec; /* size: 0x2510 */

/* One 0x5C-byte friend-list entry (`NetCtrlWk::copyFriendList` copies id, name and two packed words out). */
typedef struct NetFriendEntry {
    /* +0x00 */ NetId id_0x00;
    /* +0x0A */ u8 pad_0x0A[0x16];
    /* +0x20 */ char name_0x20[0x18];
    /* +0x38 */ s32 online_0x38;
    /* +0x3C */ u8 pad_0x3C[0x4];
    /* +0x40 */ u32 status_0x40;
    /* +0x44 */ u8 pad_0x44[0x4];
    /* +0x48 */ u32 area_0x48;
    /* +0x4C */ u8 pad_0x4C[0x10];
} NetFriendEntry; /* size: 0x5C */

/* The friend list the two friend-list sources share: a count followed by the entries.  size: 0x23F4 (the
 * network work record builds one in place at 0x90017F60; its constructor is 0x803E1104) */
typedef struct NetFriendList {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetFriendEntry entries_0x04[100];   /* `count_0x00` entries */

    NetFriendList();
} NetFriendList; /* size: 0x23F4 */

/* One field of the user-field block `NetworkLayerPat::sendUserFields_5C` sends: its kind (1..7 a value held inline
 * at +0x08, 8 a pointer at +0x08, 9 a pointer and a 16-bit size, anything else an empty field). */
typedef struct NetUserField {
    /* +0x00 */ u8 kind_00;
    /* +0x01 */ u8 pad_01[0x07];
    /* +0x08 */ const u8* data_08;   /* kinds 1..7: the value itself, inline */
    /* +0x0C */ u16 size_0C;         /* kind 9 */
    /* +0x0E */ u8 pad_0E[0x02];
} NetUserField;   /* size: 0x10 */

/* The user-field block: how many fields are set (at most 64) and the fields. */
typedef struct NetUserFields {
    /* +0x000 */ u32 count_000;
    /* +0x004 */ u32 pad_004;
    /* +0x008 */ NetUserField fields_008[64];
} NetUserFields;   /* size: 0x408 */

/* The position record `NetworkLayerPat::sendUserPosition_60` publishes (GUESS on the names: three floats and three
 * words, sent only when it changed and the throttle interval passed). */
typedef struct NetUserPosition {
    /* +0x00 */ f32 position_00[3];
    /* +0x0C */ s32 value_0C[3];
} NetUserPosition;   /* size: 0x18 */


class NetworkLayerPat {   /* size: 0x6EE8C (the allocation `initNetworkPatControl` makes for it) */
public:
    /* 0x803E0C18 - builds the layer (defined by the owner's range; declared so `new` can name the class). */
    NetworkLayerPat();

    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void setReflectCallback(u32 callback, u32 user);   /* the reflect callback and its user word */
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void pad_14();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void closeSession_1C();
    /* +0x20 */ virtual void shutdown_20();
    /* +0x24 */ virtual void requestServers_24(s32 count);
    /* +0x28 */ virtual void selectServer_28(s32 id);
    /* +0x2C */ virtual void readServerId_2C(NetId* id);
    /* +0x30 */ virtual void pad_30();
    /* +0x34 */ virtual void pad_34();
    /* +0x38 */ virtual void pad_38();
    /* +0x3C */ virtual void selectCity_3C(s32 id);
    /* +0x40 */ virtual void pad_40();
    /* +0x44 */ virtual void pad_44();
    /* +0x48 */ virtual void requestCities_48(s32 count);
    /* +0x4C */ virtual void request_4C(u32 a);   /* `NetworkLayer::request_4C` */
    /* +0x50 */ virtual void request_50(u32 a);   /* `NetworkLayer::request_50` */
    /* +0x54 */ virtual void pad_54();
    /* +0x58 */ virtual void pad_58();
    /* +0x5C */ virtual void sendUserFields_5C(NetUserFields* fields);   /* 0x803E2B10 (GUESS: the user fields, clamped to 64) */
    /* +0x60 */ virtual void sendUserPosition_60(const NetUserPosition* position);   /* 0x803E2D7C (GUESS) */
    /* +0x64 */ virtual void sendMessage_64(const char* text, s32 value, s32 flag, u8 flags);
    /* +0x68 */ virtual void setPageSize_68(s32 size);
    /* +0x6C */ virtual void requestRefresh_6C(s32 code, s32 mask);
    /* +0x70 */ virtual void pad_70();
    /* +0x74 */ virtual void pad_74();
    /* +0x78 */ virtual void pad_78();
    /* +0x7C */ virtual void pad_7C();
    /* +0x80 */ virtual void pad_80();
    /* +0x84 */ virtual void requestAccount_84(s32 kind);
    /* +0x88 */ virtual void pad_88();
    /* +0x8C */ virtual void exportLayerId_8C(NetworkLayerId* out);   /* 0x803EE054: this console's id (kind 3, 16 bytes) */
    /* +0x90 */ virtual void pad_90();
    /* +0x94 */ virtual void pad_94();
    /* +0x98 */ virtual void submitSettings_98(NetLayerSettings* settings);
    /* +0x9C */ virtual void submitRequest_9C(NetLayerRequest* request);
    /* +0xA0 */ virtual void setPresence_A0(const NetLayerSettings* presence);   /* 0x803EE1A8 (GUESS: the four presence pairs) */
    /* +0xA4 */ virtual void submitSelect_A4(NetLayerRequest* request);
    /* +0xA8 */ virtual void pad_A8();
    /* +0xAC */ virtual void pad_AC();
    /* +0xB0 */ virtual void pad_B0();
    /* +0xB4 */ virtual void pad_B4();
    /* +0xB8 */ virtual void pad_B8();
    /* +0xBC */ virtual void pad_BC();
    /* +0xC0 */ virtual void pad_C0();
    /* +0xC4 */ virtual void pad_C4();
    /* +0xC8 */ virtual void pad_C8();
    /* +0xCC */ virtual void pad_CC();
    /* +0xD0 */ virtual void pad_D0();
    /* +0xD4 */ virtual void pad_D4();
    /* +0xD8 */ virtual void setMediatorValue_D8(u8 value);   /* 0x803EE748 (GUESS: forwards to the mediator's 0x80416B90) */

    /* +0x0004 */ u8 pad_0004[0x4];
    /* +0x0008 */ u32 context_08;   /* the context word every layer event is reported with */
    /* +0x000C */ u8 pad_000C[0x318];
    /* +0x0324 */ u32 requestFlags_324;   /* bit0 = session lost, bit1 = cancelled, higher bits = replies */
    /* +0x0328 */ u8 pad_0328[0x54];
    /* +0x037C */ u32 requestResult_37C;   /* the id the last `sendReqLayer*` returned */
    /* +0x0380 */ u8 pad_0380[0x50];
    /* +0x03D0 */ u8 connected_3D0;   /* zero = no session: the request cannot start */
    /* +0x03D1 */ u8 busy_3D1;   /* set while a request is in flight; the finishing states clear it */
    /* +0x03D2 */ u8 pad_03D2[0xF2];
    /* +0x04C4 */ u8 hostMode_4C4;   /* selects the user-list step after the child-info reply */
    /* +0x04C5 */ u8 pad_04C5[0x3];
    /* +0x04C8 */ s32 memberCount_4C8;   /* must be positive for a request to start */
    /* +0x04CC */ u8 pad_04CC[0x74];
    /* +0x0540 */ NetCityList cities_540;
    /* +0x18A4 */ NetRoomList rooms_18A4;
    /* +0x3568 */ NetFriendTable friends_3568;
    /* +0x4B4C */ u8 pad_4B4C[0xE0C];
    /* +0x5958 */ NetFriendDetail details_5958[100];
    /* +0xBEE8 */ u8 pad_BEE8[0x3148];
    /* +0xF030 */ u32 status_F030;
    /* +0xF034 */ u32 status_F034;
    /* +0xF038 */ u8 pad_F038[0x164];
    /* +0xF19C */ s32 pendingRequestId_F19C;   /* GUESS: a request id is already pending when it is >= 0 */
    /* +0xF1A0 */ u8 pad_F1A0[0xC];
    /* +0xF1AC */ s32 communityCount_F1AC;
    /* +0xF1B0 */ NetCommunityRec communities_F1B0[40];
    /* +0x6BC30 */ s32 friendCount_6BC30;
    /* +0x6BC34 */ NetFriendEntry friends_6BC34[100];   /* `friendCount_6BC30` entries (with the count, a
                                                          `NetFriendList`'s 0x23F4 bytes) */
    /* +0x6E024 */ u8 pad_6E024[0xE68];

    /* Advances the request state in `request` by one step; true once the request finished. */
    bool stepRequest(NetworkLayerRequest* request);
    /* 0x803EF1D4 (GUESS) - the member slot whose friend entry carries address `id`, -1 when none does. */
    s8 getMemberSlot(const NetworkUniqueId* id);
    /* 0x803EF220 (GUESS) - copies member slot `slot`'s address into `out` (cleared first). */
    void getMemberAddress(s8 slot, NetworkUniqueId* out);
    /* 0x803EEED0 (GUESS) - marks the friend linked to session slot `slot` connected (state 3). */
    void onSessionConnected(s8 slot);
    /* 0x803EEF1C (GUESS) - marks the friend linked to session slot `slot` failed and keeps its error. */
    void onSessionFailed(s8 slot, const NetworkErrorInfo* error);
    /* 0x803EF010 (GUESS) - the friend whose session slot is `slot`, -1 when none is. */
    s8 findSessionFriend(s8 slot);
    /* 0x803EF2B4 (GUESS) - the negotiation state of the member at `address`: 0 pending, 1 failed, 2 still
     * connecting (`moveStartSession` waits while it reads 0 or 2). */
    u8 getMemberStatus(const NetworkUniqueId* address);
};

/* The free functions of `src/Network/NetworkLayerPat.cpp`'s range (0x803E0BE8..0x803EF668), moved here from
 * `Network/NetworkCommunityPat.h` and `Network/NetworkSessionManagerPat.h` by the round 3 recut (rule 2: the owner declares). */
struct PatTerms;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803E247C - the terms object; 0x80416A18 - whether it reached its update-finished state. */
struct PatTerms* getPatTerms(void);

/* GUESS: 0x803EBAF0 hands one layer event (kind 3 = error, kind 4 = done) to the callback object the
 * layer holds, after posting `info` to the network singleton. */
void notifyLayerEvent(NetworkLayerPat* self, u32 kind, s32 code, u32 has_info, NetworkRequestError* info,
                      u32 context);

/* GUESS on both names: 0x803EF3C0 and 0x803EF4C8 are the siblings of `setCollectionLog` for the two
 * fixed codes the state machine's flag tests report (0x80060033 and 0x80060012); the names follow the
 * flag bits that select them (bit 0 = the session dropped, bit 1 = the request was cancelled). */
void setCollectionLogSessionLost(NetworkLayerPat* self, NetworkLayerRequest* request);

void setCollectionLogAborted(NetworkLayerPat* self, NetworkLayerRequest* request);

/* 0x803EF568 - records `code` (+ two arguments) as the request's error and reports it to the server. */
void setCollectionLog(NetworkLayerPat* self, NetworkLayerRequest* request, u32 code, u32 arg_a, u32 arg_b);

/* GUESS: 0x803EB9D4 walks the layer's 100 child slots and releases each finished one. */
void pollLayerSlots(NetworkLayerPat* self);

/* GUESS: 0x803EBA5C reports the layer's slot counts as one kind-0x14 event. */
void notifyLayerSlotSummary(NetworkLayerPat* self);

#ifdef __cplusplus
}
#endif

#endif
