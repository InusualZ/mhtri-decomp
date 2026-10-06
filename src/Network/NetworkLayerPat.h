/*
 * Network/NetworkLayerPat.h - the declarations of `Network/NetworkLayerPat.cpp`: `NetworkLayerPat` (the `NetworkPat`
 *   holder's +0x0C element, derived from `NetworkLayer`; its table 0x805FC1E0 is emitted by the unit), the records its
 *   slots take and the unit's free functions.
 */
#ifndef MHTRI_NETWORK_NETWORKLAYERPAT_H
#define MHTRI_NETWORK_NETWORKLAYERPAT_H

#include "types.h"
#include "Network/NetworkConnection.h"      /* NetworkMutex - the member mutex */
#include "Network/NetworkSessionManager.h"   /* NetworkRequest - the record the state machine advances */
#include "Network/NetworkLayer.h"            /* NetworkLayer, NetworkLayerRequest, NetworkLayerId - the base this class builds on */
#include "Network/NetworkUniqueId.h"         /* NetworkUniqueId - the server id member */
#include "Network/gamespy_interface_types.h" /* NetworkErrorInfo - the kept session error (owner Network/GameSpyInterfaceThread.cpp) */

/* The exported form of a network id: the bytes `NetworkUniqueId::exportTo` writes (`formatNetId` renders ten of them,
 * the peer records keep that text) and `importNetId`/`isSameNetId` read back (six raw bytes, kind 3).  The id object
 * itself is `NetworkUniqueId` (Network/NetworkUniqueId.h).  size: 0xA */
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

/* The request record `NetworkLayerPat::submitRequest_9C`/`submitSelect_A4` take (`initNetLayerRequest` clears it,
 * `copyNetLayerRequest` copies it field by field): a count and up to four items, the unique id and the text a select
 * searches for, and a flag byte.  The id makes it a class the compiler builds (implicit constructor 0x803E1230,
 * destructor 0x803E0FBC). */
struct NetLayerRequest {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetLayerRequestItem items_0x04[4];
    /* +0x34 */ NetworkUniqueId id_34;      /* `NetCtrlWk::submitIdSelect` imports the id it selects */
    /* +0x54 */ char text_54[0x14];        /* `NetCtrlWk::submitTextSelect` copies the text it selects */
    /* +0x68 */ u8 flag_68;
    /* +0x69 */ u8 pad_69[0x3];

    NetLayerRequest();    /* 0x803E1230 - called out of line by the layer's and the work record's constructors */
    ~NetLayerRequest();   /* 0x803E0FBC */
};   /* size: 0x6C (the layer keeps two at a 0x6C stride, +0xF0C4 and +0xF130) */

/* The settings record `NetworkLayerPat::submitSettings_98` takes: a count of four and four
 * (enabled, value) pairs.  size: 0x24 */
typedef struct NetLayerSettingPair {
    /* +0x0 */ s32 enabled_0x0;
    /* +0x4 */ s32 value_0x4;
} NetLayerSettingPair; /* size: 0x8 */

typedef struct NetLayerSettings {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetLayerSettingPair pairs_0x04[4];
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
    /* +0x04 */ char name_0x04[0x40];
    /* +0x44 */ s32 count_0x44;          /* `PatLayerData::counts_058[1]` */
    /* +0x48 */ s32 population_0x48;
    /* +0x4C */ s32 capacity_0x4C;
    /* +0x50 */ s32 order_0x50;
    /* +0x54 */ u32 kind_0x54;         /* 4 = the record carries the four settings below */
    /* +0x58 */ NetCityValue values_0x58[4];
    /* +0x78 */ bool closed_0x78;
    /* +0x79 */ bool hidden_0x79;
    /* +0x7A */ u8 pad_0x7A[0x2];
} NetCityRec; /* size: 0x7C */

/* One server the server list reports (`exportServerRec` fills it from an FMP slot; GUESS on the names of the three
 * words, the slot's own). */
struct NetServerRec {
    /* +0x00 */ u32 id_00;
    /* +0x04 */ char name_04[0x20];   /* 31 characters of the slot's name and a terminator */
    /* +0x24 */ char text_24[0x1];    /* the exporter copies `sizeof - 1` = 0 bytes and terminates it */
    /* +0x25 */ u8 pad_25[0x3];
    /* +0x28 */ u32 done_28;
    /* +0x2C */ u32 total_2C;
    /* +0x30 */ u32 value_30;
    /* +0x34 */ u8 pad_34[0x4];
    /* +0x38 */ u64 time_38;
};   /* size: 0x40 (the stride `handleServerList` fills) */

/* The server list `handleServerList` reports: a count and up to 80 servers. */
struct NetServerList {
    /* +0x000 */ u32 count_000;
    /* +0x004 */ u8 pad_004[0x4];
    /* +0x008 */ NetServerRec entries_008[80];
};   /* size: 0x1408 */

/* The city list: a count and the 40 entries (`NetworkLayerPat::cities_540`).  size: 0x1364 */
typedef struct NetCityList {
    /* +0x000 */ u32 count_0x000;
    /* +0x004 */ NetCityRec entries_0x004[40];
} NetCityList; /* size: 0x1364 */

/* One 0xB8-byte entry of the layer's room list (GUESS: id, name and a population pair). */
typedef struct NetRoomRec {
    /* +0x00 */ u8 header_0x00[0x40];   /* the 64 bytes `readRoomHeader_C8` copies out */
    /* +0x40 */ s32 id_0x40;
    /* +0x44 */ char name_0x44[0x40];
    /* +0x84 */ s32 count_0x84;          /* `PatLayerData::counts_058[1]` */
    /* +0x88 */ s32 population_0x88;
    /* +0x8C */ s32 capacity_0x8C;
    /* +0x90 */ s32 order_0x90;
    /* +0x94 */ NetLayerSettings settings_0x94;
} NetRoomRec; /* size: 0xB8 */

/* The room list: a count and the 40 entries (`NetworkLayerPat::rooms_18A4`).  size: 0x1CC4 */
typedef struct NetRoomList {
    /* +0x000 */ u32 count_0x000;
    /* +0x004 */ NetRoomRec entries_0x004[40];
} NetRoomList; /* size: 0x1CC4 */

/* One 0x38-byte entry of the layer's friend id table (`NetCtrlWk::hasFriendDetail` searches it by id): the friend's
 * unique id, its name and two flags.  The id is a real `NetworkUniqueId`, so the compiler builds and destroys each
 * entry (constructor 0x803E1180, destructor 0x803E0E94). */
struct NetFriendRec {
    /* +0x00 */ NetworkUniqueId id_0x00;
    /* +0x20 */ char name_0x20[0x14];   /* cleared and copied with the id (`clearFriendRec`, `copyNetFriendRec`) */
    /* +0x34 */ u8 flag_0x34;
    /* +0x35 */ u8 valid_0x35;
    /* +0x36 */ u8 pad_0x36[0x2];

    NetFriendRec();
    ~NetFriendRec();   /* out of line: `NetFriendEntry`'s destructor calls it (an implicit one is inlined) */
};   /* size: 0x38 */

/* The friend id table: a count and the 100 entries (`NetworkLayerPat::friends_3568`).  Its constructor 0x803E1264 and
 * destructor 0x803E101C are called out of line (`handleUserList`, `NetCtrlWk`'s constructor), so they are declared and
 * defined in `src/Network/NetworkLayerPat.cpp` rather than left implicit (MWCC inlines an implicit one).  size: 0x15E4 */
struct NetFriendTable {
    /* +0x000 */ u32 count_0x000;   /* unsigned: `handleUserList` compares it with `cmplw` */
    /* +0x004 */ NetFriendRec entries_0x004[100];

    NetFriendTable();
    ~NetFriendTable();
};   /* size: 0x15E4 */

/* One 0x24-byte friend session record, paired with the friend of the same index: whether the friend is online and
 * the two packed words the friend list view shows (`NetCtrlWk::copyFriendList`). */
typedef struct NetFriendSession {
    /* +0x00 */ s32 state_00;    /* non-zero = online (`clearFriendSlot` clears it) */
    /* +0x04 */ u8 pad_04[0x4];
    /* +0x08 */ u32 status_08;   /* the top byte is the status the view shows */
    /* +0x0C */ u8 pad_0C[0x4];
    /* +0x10 */ u32 area_10;     /* the top half is the area the view shows */
    /* +0x14 */ u8 pad_14[0x4];
    /* +0x18 */ u32 settings_18; /* four packed setting bytes (a friend card's +0x128..+0x12B) */
    /* +0x1C */ u8 pad_1C[0x4];
    /* +0x20 */ u32 members_20;  /* the top half is the member count (a friend card's +0x12C) */
} NetFriendSession;   /* size: 0x24 (the 36 bytes `handleUserList` copies per entry) */

/* A friend roster: the friend table and the session record of each friend (GUESS on the name) - the payload
 * `handleUserList` builds on its stack and reports (the event's record is the roster's address, the sessions follow the
 * table at +0x15E4).  The layer (+0x3568) and every community record (+0x11C) hold the same two runs as separate members. */
struct NetFriendRoster {
    /* +0x0000 */ NetFriendTable table_0000;
    /* +0x15E4 */ NetFriendSession sessions_15E4[100];
};   /* size: 0x23F4 */

/* One 0x104-byte detail record paired with the friend of the same index (GUESS on the names: only the state word
 * `clearFriendSlot` clears and the flag byte `NetCtrlWk::hasFriendDetail` reads are named). */
typedef struct NetFriendDetail {
    /* +0x000 */ u32 size_0x000;       /* the binary's length, at most 0x100 (`storeFriendDetail`; `clearFriendSlot` clears it) */
    /* +0x004 */ u8 data_0x004[0xF9];  /* the user's binary (`PatLayerUser::binary_03C`), up to +0x104 */
    /* +0x0FD */ u8 flag_0x0FD;        /* a byte of that binary `NetCtrlWk::hasFriendDetail` tests */
    /* +0x0FE */ u8 data_0x0FE[0x6];
} NetFriendDetail; /* size: 0x104 */

/* The current layer's info record (`layerInfo_4CC`, reported with the layer-info event): the layer's name, four of
 * its words and its settings (GUESS on the word names: `PatLayerData`'s). */
typedef struct NetLayerInfoRec {
    /* +0x00 */ char name_00[0x40];
    /* +0x40 */ s32 count_40;          /* `PatLayerData::counts_058[1]` */
    /* +0x44 */ s32 count_44;          /* `PatLayerData::counts_058[0]` */
    /* +0x48 */ s32 memberLimit_48;    /* `PatLayerData::memberLimitA_064` */
    /* +0x4C */ s32 value_4C;          /* `PatLayerData::value_06C` */
    /* +0x50 */ NetLayerSettings settings_50;
} NetLayerInfoRec; /* size: 0x74 */

/* The friend event `pollFriendSlot` reports (event 6): the friend record and its detail record (GUESS on the name). */
struct NetFriendNotice {
    /* +0x000 */ NetFriendRec rec_000;
    /* +0x038 */ NetFriendDetail detail_038;

    NetFriendNotice();    /* 0x803EB9A4 */
    ~NetFriendNotice();   /* 0x803EB8BC */
};   /* size: 0x13C */

/* The friend status event `pollFriendSlot` reports (event 13): the friend's unique id and its status word (GUESS on
 * the name). */
struct NetFriendStatusNotice {
    /* +0x00 */ NetworkUniqueId id_00;
    /* +0x20 */ u32 status_20;

    NetFriendStatusNotice();    /* 0x803EB974 */
    ~NetFriendStatusNotice();   /* 0x803EB918 */
};   /* size: 0x24 */

/* One 0x2510-byte entry of the layer's community list (the lobby's group rows); only the fields the
 * list view copies out are named. */
typedef struct NetCommunityRec {
    /* +0x0000 */ u8 header_0x0000[0x40];   /* the 64 bytes `readCommunityHeader_C4` copies out */
    /* +0x0040 */ char name_0x0040[0x20];
    /* +0x0060 */ char leader_0x0060[0x40];
    /* +0x00A0 */ char comment_0x00A0[0x40];
    /* +0x00E0 */ s32 value_0x00E0;
    /* +0x00E4 */ s32 value_0x00E4;
    /* +0x00E8 */ s32 value_0x00E8;
    /* +0x00EC */ s32 value_0x00EC;
    /* +0x00F0 */ u32 matchKey_0x00F0;   /* the community's match key (`handleLayerJump` keeps it in `matchKey_3D4`) */
    /* +0x00F4 */ s32 settingsCount_0x00F4;   /* a `NetLayerSettings` record from here to +0x118 (`readCommunitySettings_C0`) */
    /* +0x00F8 */ s32 enabled_0x00F8;
    /* +0x00FC */ s32 slot_0x00FC;
    /* +0x0100 */ s32 enabled_0x0100;
    /* +0x0104 */ s32 slot_0x0104;
    /* +0x0108 */ s32 enabled_0x0108;
    /* +0x010C */ s32 slot_0x010C;
    /* +0x0110 */ s32 enabled_0x0110;
    /* +0x0114 */ s32 slot_0x0114;
    /* +0x0118 */ bool state_0x0118;   /* `getCommunityState_AC` */
    /* +0x0119 */ u8 pad_0x0119[0x3];
    /* +0x011C */ NetFriendTable members_0x011C;                /* the record's implicit constructor 0x803E11FC builds it */
    /* +0x1700 */ NetFriendSession memberSessions_0x1700[100];
} NetCommunityRec; /* size: 0x2510 */

/* The community list: a count and the 40 records (`NetworkLayerPat::communities_F1AC`; implicit constructor 0x803E11B0,
 * destructor 0x803E0EF0). */
struct NetCommunityList {
    /* +0x00000 */ s32 count_0x00000;
    /* +0x00004 */ NetCommunityRec entries_0x00004[40];

    NetCommunityList();
    ~NetCommunityList();
};   /* size: 0x5CA84 */

/* One 0x5C-byte friend-list entry: the friend record and its session record side by side (`handleUserList` splits a
 * list into the two tables; `NetCtrlWk::copyFriendList` copies id, name and the two packed words out).  Implicit
 * constructor 0x803E1150, destructor 0x803E0E38. */
struct NetFriendEntry {
    /* +0x00 */ NetFriendRec rec_00;
    /* +0x38 */ NetFriendSession session_38;
};   /* size: 0x5C */

/* The friend list the two friend-list sources share: a count followed by the entries.  size: 0x23F4 (the
 * network work record builds one in place at 0x90017F60; its constructor is 0x803E1104) */
typedef struct NetFriendList {
    /* +0x00 */ u32 count_0x00;   /* unsigned: the list readers clamp it with `cmplwi` */
    /* +0x04 */ NetFriendEntry entries_0x04[100];   /* `count_0x00` entries */

    NetFriendList();
    ~NetFriendList();   /* 0x803E0DCC */
} NetFriendList; /* size: 0x23F4 */

/* One field of the user-field block `NetworkLayerPat::sendUserFields_5C` sends: its kind (1..7 a value held inline
 * at +0x08, 8 a pointer at +0x08, 9 a pointer and a 16-bit size, anything else an empty field). */
typedef struct NetUserFieldBlob {
    /* +0x0 */ const u8* data_0;
    /* +0x4 */ u16 size_4;
    /* +0x6 */ u8 pad_6[0x2];
} NetUserFieldBlob;   /* size: 0x8 */

/* A user field's value: kinds 1..7 inline (u8, u16, u32, s64, f32, f64, u32), 8 and 9 a pointer (9 with a size). */
typedef union NetUserFieldValue {
    /* +0x0 */ u8 byte;
    /* +0x0 */ u16 half;
    /* +0x0 */ u32 word;
    /* +0x0 */ s64 wide;
    /* +0x0 */ f32 single;
    /* +0x0 */ f64 real;
    /* +0x0 */ NetUserFieldBlob blob;
} NetUserFieldValue;   /* size: 0x8 */

typedef struct NetUserField {
    /* +0x00 */ u8 kind_00;
    /* +0x01 */ u8 pad_01[0x07];
    /* +0x08 */ NetUserFieldValue value_08;
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

/* The user-field event `reflect` reports (event 17): the user and the fields the server sent (GUESS on the name). */
struct NetUserFieldsNotice {
    /* +0x000 */ NetworkUniqueId id_000;
    /* +0x020 */ NetUserFields fields_020;

    NetUserFieldsNotice();    /* 0x803EDF94 - inline, emitted after `reflect` */
    ~NetUserFieldsNotice();   /* 0x803EDF38 */
};   /* size: 0x428 */

/* The user-position event `reflect` reports (event 14): the user and the position it published (GUESS on the name). */
struct NetUserPositionNotice {
    /* +0x000 */ NetworkUniqueId id_00;
    /* +0x020 */ NetUserPosition position_20;

    NetUserPositionNotice();    /* 0x803EDF08 - inline, emitted after `reflect` */
    ~NetUserPositionNotice();   /* 0x803EDEAC */
};   /* size: 0x38 */

/* The login record the server sends this console when it enters a layer (only the fields `applyLoginRecord` takes
 * are named; GUESS on their meaning from where they go). */
typedef struct NetLayerLoginRecord {
    /* +0x00 */ u8 pad_00[0x14];
    /* +0x14 */ char name_14[0x40];
    /* +0x54 */ u8 pad_54[0x4];
    /* +0x58 */ u32 memberPeak_58;
    /* +0x5C */ s32 memberUsed_5C;
    /* +0x60 */ u8 pad_60[0x4];
    /* +0x64 */ u32 status_64;
    /* +0x68 */ u8 pad_68[0x4];
    /* +0x6C */ u32 status_6C;
    /* +0x70 */ u8 pad_70[0x8];
    /* +0x78 */ u32 positionInterval_78;   /* milliseconds */
    /* +0x7C */ s8 role_7C;               /* 1 = this console hosts */
    /* +0x7D */ u8 pad_7D[0x3];
} NetLayerLoginRecord;   /* size: 0x80 (approximation: the last field read is +0x7C) */

/* A position in the layer tree: two id words and the path of up to four layer indices (GUESS on the names: the
 * layer-info request clears the path entries below the depth it asks for). */
typedef struct NetLayerAddress {
    /* +0x00 */ u32 id_00;
    /* +0x04 */ u32 id_04;
    /* +0x08 */ u16 path_08[4];
} NetLayerAddress;   /* size: 0x10 */

/* The layer record a layer-info-by-id request fills: the layer's id and up to three tab-separated texts of its
 * comment. */
typedef struct NetLayerRecord {
    /* +0x000 */ u8 header_000[0x40];     /* a `NetworkLayerId` (`NetworkLayerIdImportFrom` fills it) */
    /* +0x040 */ s32 textCount_40;
    /* +0x044 */ char texts_044[3][0x40];
} NetLayerRecord;   /* size: 0x104 */

/* The user record a user-info request sends (only the key it fills is named). */
typedef struct NetLayerUserRecord {
    /* +0x000 */ u8 pad_000[0x38];
    /* +0x038 */ u32 key_38;
    /* +0x03C */ u8 pad_03C[0x104];
} NetLayerUserRecord;   /* size: 0x140 (the request clears 320 bytes) */

/* Up to 256 bytes of binary info and their length (the layer-info-set done event's payload). */
typedef struct NetLayerBinary {
    /* +0x000 */ u32 size_00;
    /* +0x004 */ u8 data_04[0x100];
} NetLayerBinary;   /* size: 0x104 */

/* One mediation entry: the unique id of the member and the two lock arguments (GUESS on the names: the mediation
 * lock/unlock handlers report one, the server id and their arguments, with their done event). */
struct NetLayerMediationEntry {
    /* +0x00 */ NetworkUniqueId id_00;
    /* +0x20 */ u8 lock_20;
    /* +0x21 */ u8 key_21;
    /* +0x22 */ u8 pad_22[0x2];

    NetLayerMediationEntry();   /* 0x803E10D4 - builds the id */
    ~NetLayerMediationEntry();  /* 0x803E0D70 - out of line: the mediation handlers call it (an implicit one is inlined) */
};   /* size: 0x24 (the element size the list's `__construct_array` passes) */

/* The mediation list the mediation-list request fills: a count and 32 entries. */
struct NetLayerMediationList {
    /* +0x000 */ s32 count_000;
    /* +0x004 */ NetLayerMediationEntry entries_004[32];

    NetLayerMediationList();    /* 0x803E1088 */
    ~NetLayerMediationList();   /* 0x803E0D04 */
};   /* size: 0x484 */

/* One search filter a detail search sends (`packLayerFilters` builds them from a `NetLayerRequest`'s items): the
 * mapped operator, the field number and the value when the item is enabled. */
typedef struct NetLayerFilter {
    /* +0x00 */ u8 op_00;
    /* +0x01 */ u8 pad_01[0x3];
    /* +0x04 */ u8 field_04;
    /* +0x05 */ u8 enabled_05;
    /* +0x06 */ u8 pad_06[0x2];
    /* +0x08 */ s32 value_08;
} NetLayerFilter;   /* size: 0xC */

/* The layer requests' field list and layer record are `PatInterface.cpp`'s (`PatTagList` of `PatTagValue`s,
 * `PatLayerData`; Network/PatInterface.h): the layer fills and sends them. */
typedef struct PatTagValue PatTagValue;
typedef struct NetworkFmpSlot NetworkFmpSlot;   /* Network/PatInterface.h - the FMP slot `exportServerRec` reads */
typedef struct PatTagList PatTagList;
typedef struct PatLayerData PatLayerData;   /* the layer record a layer answer carries */
typedef struct PatLayerUser PatLayerUser;   /* the user record a user answer carries */

/* The mixed voice of the transfer peers (the move warning's "mVoiceMixed.mSize"): the samples and their size in
 * bytes (528 is one whole frame). */
struct NetVoiceBuffer {
    /* +0x000 */ s16 mData[0x17A];
    /* +0x2F4 */ s32 mSize;
};   /* size: 0x2F8 (the class's last field: it ends at the allocation size 0x6EE8C) */

/* The layer the Pat network game drives (the pool strings name it: "NetworkLayerPat::move ...").  It derives from
 * `NetworkLayer`: the constructor 0x803E0C18 calls `NetworkLayer::NetworkLayer` first, then stores the table
 * 0x805FC1E0 (two RTTI words and 79 slots, +0x08..+0x140), which this unit emits - the key function is
 * `stepRequest`, the first virtual the class declares, defined in `src/Network/NetworkLayerPat.cpp`.  Every slot
 * the table fills with an address of this unit is declared here as an override; the base's pure slots carry the
 * same names.
 * Only the fields the units read are named; the rest is padding at its real offset. */
class NetworkLayerPat;
struct NetworkLayerPatRequest;

/* The handler one of the layer's own requests runs: a non-virtual member function of the layer (the descriptor is
 * `{0, -1, fn}`). */
typedef s32 (NetworkLayerPat::*NetworkLayerPatHandler)(NetworkLayerPatRequest* request);

/* The layer's own request record (`pool_1C8`): the `NetworkLayerRequest` layout with the owner and the handler typed
 * for this class.  Its members are inline - retail emits each one after the first function that calls it. */
struct NetworkLayerPatRequest {
    /* +0x00 */ s32 state_00;
    /* +0x04 */ u32 unused_04;
    /* +0x08 */ u32 unused_08;
    /* +0x0C */ u32 unused_0C;
    /* +0x10 */ u32 unused_10;
    /* +0x14 */ u32 unused_14;
    /* +0x18 */ u32 unused_18;
    /* +0x1C */ u32 unused_1C;
    /* +0x20 */ u32 unused_20;
    /* +0x24 */ u32 unused_24;
    /* +0x28 */ u32 count_28;              /* how many of `args_2C` the starter filled */
    /* +0x2C */ u32 args_2C[8];
    /* +0x4C */ f32 interval_4C;
    /* +0x50 */ f32 timeout_50;            /* the start time `begin` stamps */
    /* +0x54 */ u32 record_54;
    /* +0x58 */ u32 record_58;
    /* +0x5C */ u32 record_5C;
    /* +0x60 */ u32 unused_60;
    /* +0x64 */ u32 unused_64;
    /* +0x68 */ u32 unused_68;
    /* +0x6C */ u32 unused_6C;
    /* +0x70 */ u32 requestId_70;
    /* +0x74 */ u8 cancelled_74;
    /* +0x75 */ u8 pad_75[0x03];
    /* +0x78 */ NetworkMutex mutex_78;
    /* +0x94 */ NetworkLayerPat* owner_94;     /* set while the request runs */
    /* +0x98 */ NetworkLayerPatHandler handler_98;

    NetworkLayerPatRequest();
    ~NetworkLayerPatRequest();
    void clear();
    void reset();
    s32 isOwned();
    void run();
    void begin(NetworkLayerPat* owner, NetworkLayerPatHandler handler, u32 count, ...);
    s32 getRecord(NetworkRequestError* out);
    s32 getArgument(u32 index);
    void setRecord(u32 code, u32 arg_a, u32 arg_b);
};   /* size: 0xA4 (the pool's element size) */

class NetworkLayerPat : public NetworkLayer {   /* size: 0x6EE8C (the allocation `initNetworkPatControl` makes for it) */
public:
    /* 0x803E0C18 - builds the layer. */
    NetworkLayerPat();

    /* The key function (declared first so the table is emitted with it). */
    virtual s32 stepRequest(NetworkLayerRequest* request);                    /* +0x104 */
    virtual ~NetworkLayerPat();                                               /* +0x008 */
    virtual void setReflectCallback(u32 callback, u32 user);                  /* +0x00C */
    virtual void clear();                                                     /* +0x010 */
    virtual void release();                                                   /* +0x014 */
    virtual void move();                                                      /* +0x018 */
    virtual void readServerId_2C(NetworkUniqueId* id);                        /* +0x02C */
    virtual void readServerName_30(char* out, s32 size);                      /* +0x030 */
    virtual void readServerText_34(char* out, s32 size);                      /* +0x034 */
    virtual void sendUserFields_5C(NetUserFields* fields);                    /* +0x05C (GUESS: the user fields, clamped to 64) */
    virtual void sendUserPosition_60(const NetUserPosition* position);        /* +0x060 (GUESS) */
    virtual void readSelectedServer_88(NetServerRec* out);                    /* +0x088 */
    virtual void exportLayerId_8C(NetworkLayerId* out);                       /* +0x08C - this console's id (kind 3, 16 bytes) */
    virtual void readUserName_90(char* out, s32 size);                        /* +0x090 */
    virtual void setComment_94(const char* text);                             /* +0x094 */
    virtual void submitSettings_98(NetLayerSettings* settings);               /* +0x098 */
    virtual void submitRequest_9C(NetLayerRequest* request);                  /* +0x09C */
    virtual void setPresence_A0(const NetLayerSettings* presence);            /* +0x0A0 (GUESS: the four presence pairs) */
    virtual void submitSelect_A4(NetLayerRequest* request);                   /* +0x0A4 */
    virtual s32 getCommunityCount_A8();                                       /* +0x0A8 */
    virtual u8 getCommunityState_AC(s32 index);                               /* +0x0AC */
    virtual void readCommunityComment_B0(s32 index, char* out, s32 size);     /* +0x0B0 */
    virtual u32 getCommunityValueE0_B4(s32 index);                            /* +0x0B4 */
    virtual u32 getCommunityValueE4_B8(s32 index);                            /* +0x0B8 */
    virtual u32 getCommunityValueE8_BC(s32 index);                            /* +0x0BC */
    virtual void readCommunitySettings_C0(s32 index, NetLayerSettings* out);  /* +0x0C0 */
    virtual void readCommunityHeader_C4(s32 index, u8* out);                  /* +0x0C4 */
    virtual void readRoomHeader_C8(s32 index, u8* out);                       /* +0x0C8 */
    virtual void setFlag75(u32 value);                                        /* +0x0CC */
    virtual void setMediatorValue_D8(u32 value);                              /* +0x0D8 (GUESS: forwards to the mediator's terms flag) */
    virtual u8 getMediatorValue_DC();                                         /* +0x0DC */
    virtual void setFriendTransferMode_E0(s8 slot, u32 mode);                 /* +0x0E0 */
    virtual BOOL isFriendTransferActive_E4(s8 slot);                          /* +0x0E4 */
    virtual BOOL isFriendTransferReady_E8(s8 slot);                           /* +0x0E8 */
    virtual u8 getFriendFlagC084_EC(s8 slot);                                 /* +0x0EC */
    virtual BOOL getFriendTransferFlag_F0(s8 slot);                           /* +0x0F0 */
    virtual s32 handleConnect(NetworkLayerRequest* request);                  /* +0x0F4 */
    virtual s32 handleDisconnect(NetworkLayerRequest* request);               /* +0x0F8 */
    virtual s32 handleServerList(NetworkLayerRequest* request);               /* +0x0FC */
    virtual s32 handleServerSelect(NetworkLayerRequest* request);             /* +0x100 */
    virtual s32 handleLayerCreate(NetworkLayerRequest* request);              /* +0x108 */
    virtual s32 handleLayerInfo(NetworkLayerRequest* request);                /* +0x10C */
    virtual s32 handleChildList(NetworkLayerRequest* request);                /* +0x110 */
    virtual s32 handleSiblingList(NetworkLayerRequest* request);              /* +0x114 */
    virtual s32 handleUserList(NetworkLayerRequest* request);                 /* +0x118 */
    virtual s32 handleUserInfo(NetworkLayerRequest* request);                 /* +0x11C */
    virtual s32 handleChat(NetworkLayerRequest* request);                     /* +0x120 */
    virtual s32 handleDetailSearch(NetworkLayerRequest* request);             /* +0x124 */
    virtual s32 handleUserSearch(NetworkLayerRequest* request);               /* +0x128 */
    virtual s32 handleLayerJump(NetworkLayerRequest* request);                /* +0x12C */
    virtual s32 handleLayerInfoById(NetworkLayerRequest* request);            /* +0x130 */
    virtual s32 handleLayerInfoSet(NetworkLayerRequest* request);             /* +0x134 */
    virtual s32 handleMediationLock(NetworkLayerRequest* request);            /* +0x138 */
    virtual s32 handleMediationUnlock(NetworkLayerRequest* request);          /* +0x13C */
    virtual s32 handleMediationList(NetworkLayerRequest* request);            /* +0x140 */

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

    /* 0x803EF568 - records `code` (+ two arguments) as the request's error and reports it to the server. */
    void setCollectionLog(NetworkLayerRequest* request, u32 code, u32 arg_a, u32 arg_b);
    /* 0x803EF4C8 / 0x803EF3C0 (GUESS on both names) - the siblings of `setCollectionLog` for the two fixed codes the
     * handlers' flag tests report (0x80060012 cancelled, 0x80060033 the session dropped). */
    void setCollectionLogAborted(NetworkLayerRequest* request);
    void setCollectionLogSessionLost(NetworkLayerRequest* request);
    /* 0x803EF410 / 0x803EF518 / 0x803EF5E8 - the same three for the layer's own requests. */
    void setCollectionLogSessionLost(NetworkLayerPatRequest* request);
    void setCollectionLogAborted(NetworkLayerPatRequest* request);
    void setCollectionLog(NetworkLayerPatRequest* request, u32 code, u32 arg_a, u32 arg_b);
    /* 0x803EBAF0 (GUESS) - hands one layer event (kind 3 = error, kind 4 = done, ...) to the callback the layer was
     * initialised with, replacing a negative code by the singleton's own error record when it has one. */
    void notifyLayerEvent(u32 kind, s32 code, u32 has_info, NetworkRequestError* info, u32 context);
    /* 0x803EB9D4 (GUESS) - walks the 100 friend slots and polls each one that is not the server itself. */
    void pollLayerSlots();
    /* 0x803EBA5C (GUESS) - reports the layer's member counts as one kind-0x14 event. */
    void notifyLayerSlotSummary();
    /* 0x803EB46C / 0x803EB500 / 0x803EB630 (GUESS) - pack a settings record into numbered info fields, a request
     * record into search filters (both at most four, the count they wrote), and fill a field list from settings. */
    s32 packLayerSettings(PatTagValue* out, s32 max, NetLayerSettings* settings);
    s32 packLayerFilters(NetLayerFilter* out, s32 max, NetLayerRequest* request);
    void buildLayerInfoFields(PatTagList* out, NetLayerSettings* settings);
    /* 0x803EAB88 / 0x803EAC00 / 0x803EA6B4 (GUESS) - clear one friend record, clear everything kept for friend slot
     * `index` (and close its mediator transfer slot), forget the whole layer (address, name, members, friends). */
    s32 clearFriendRec(NetFriendRec* rec);
    s32 clearFriendSlot(u32 index);
    /* 0x803EA82C / 0x803EA8C0 (GUESS) - fill the current layer's info record and a community record from a layer
     * answer and its tag list. */
    void importLayerInfo(NetLayerInfoRec* out, const PatLayerData* layer, PatTagList* tags);
    void importCommunityRec(NetCommunityRec* rec, const PatLayerData* layer, PatTagList* tags);
    /* 0x803EACE4 / 0x803EAD90 / 0x803EAE40 (GUESS) - fill a friend record from a binary user id and a name, keep
     * friend slot `index`'s binary and key from a user answer, and find the friend slot of a binary user id (-1: no
     * id, -2: none). */
    s32 importFriendRec(NetFriendRec* rec, const u8* userId, const char* name);
    s32 storeFriendDetail(u32 index, const PatLayerUser* user);
    s32 findFriendByUserId(const u8* userId);
    /* 0x803EAF14 / 0x803EB1D4 (GUESS) - add a user to the first free friend slot (this console's own slot becomes the
     * transfer slot) and remove one; each returns the slot or a negative error. */
    s32 addFriendSlot(const u8* userId, const PatLayerUser* user);
    s32 removeFriendSlot(const u8* userId, NetFriendRec* out);
    /* 0x803EB678 (GUESS) - the inverse of `packLayerSettings`: the numbered fields 1..4 of `tags` as settings pairs. */
    void unpackLayerSettings(NetLayerSettings* out, PatTagList* tags);
    void resetLayerState();
    /* 0x803E2484 / 0x803E250C - the layer's own pool: the first idle request, and the release of one (its own
     * "NetworkLayerPat::deleteRequest" warning); 0x803E257C (GUESS) - runs the 22 request slots ("NetworkLayerPat::move"). */
    NetworkLayerPatRequest* allocRequest();
    void deleteRequest(NetworkLayerPatRequest** slot);
    void moveRequests();
    /* 0x803E2890 (GUESS) - while no request is parked at +0x1C0, takes one from the pool and starts it on the
     * handler 0x803E97F0 (the one that sends `sendReqLayerCreateHead`/`Foot`) with kind 2 and the two arguments;
     * the message pool passes its queued argument (or 0) and a 0/1 flag. */
    void requestLayerCreate(u32 arg, s32 mode);
    /* 0x803E97F0 (GUESS) - the handler `requestLayerCreate` starts: with `reserve` set, the create head for layer
     * `layerId` (kept as the pending id), else its create foot; reports event 36 with the reserve flag. */
    s32 handleLayerReserve(NetworkLayerPatRequest* request);
    /* 0x803E9C78 / 0x803EA11C / 0x803EA340 (GUESS) - clear the list-pending flags. */
    void clearListPending0();
    void clearListPending1();
    void clearListPending2();
    /* 0x803EE9EC (GUESS) - sets a friend slot's two flags from a bit mask. */
    void setFriendFlags(s8 slot, u32 flags);
    /* 0x803EF138 (GUESS) - the friend slot carrying unique id `id`, -1 when none does. */
    s32 findFriendById(const NetworkUniqueId* id);
    /* 0x803EF300 (GUESS) - sets the transfer mode of the friend carrying `id` and refreshes its transfer. */
    void setFriendTransferModeById(const NetworkUniqueId* id, u8 mode);
    /* 0x803EEAF8 (GUESS) - re-sends friend slot `slot`'s transfer state to the server; 0x803EEA44 (GUESS) - that state. */
    void refreshFriendTransfer(s8 slot, u32 flag);
    u32 getFriendTransferState(s8 slot);
    /* 0x803EBB9C (GUESS) - the server message callback's body: each message sets the reply bits the handlers poll
     * or updates the layer records (written later). */
    void reflect(s32 code, s32 requestId, s32 flag, s32 count, const u8* data);
    /* 0x803EA77C (GUESS) - takes this console's layer login record. */
    void applyLoginRecord(const NetLayerLoginRecord* record);
    /* 0x803E9C84 / 0x803E9CBC (GUESS on both names) - read the user list of the layer at `address` into the friend list
     * in batches of 10 rows (head, data, foot); with `filterCount >= 0` the rows are a user search's.  A step returns 0
     * while it waits, 1 when the foot arrived clean, -1 when the session dropped and -2 when it was cancelled. */
    s32 readUserList(NetLayerAddress* address, BOOL wide, u32 first, s32 count, NetworkLayerRequest* request);
    s32 readUserRows(NetLayerAddress* address, BOOL wide, u32 first, s32 count, const char* userId, const char* name,
                     NetLayerFilter* filters, s32 filterCount, NetworkLayerRequest* request);
    /* 0x803E1914 / 0x803E1A14 (GUESS on both names) - halve a run of voice samples into `out` (the pairs averaged), and
     * mix a peer's samples into `out` at twice the rate, scaled by the mediator's transfer level; both return the
     * output size in bytes, 0 when `out` is too small. */
    s32 downsampleVoice(s16* out, s32 outSize, const s16* in, s32 inSize);
    s32 mixVoice(s16* out, s32 outSize, const s16* in, s32 inSize);
    /* 0x803EEDCC (GUESS) - the valid friend slot whose GameSpy peer id is `peerId`, -1 when none (or `peerId` 0). */
    s8 findFriendByPeerId(u32 peerId);
    /* 0x803EECA0 (GUESS) - tells the layer the NAT negotiation state of the peer pair `from`/`to` (not for one peer). */
    void sendPairState(u32 from, u32 to, s8 state);
    /* 0x803EA61C (GUESS) - copies an FMP slot into a server record. */
    void exportServerRec(NetServerRec* out, const NetworkFmpSlot* slot);
    /* 0x803EB75C (GUESS) - polls friend slot `index` (written later; declared for `pollLayerSlots`). */
    void pollFriendSlot(u32 index);
    /* 0x803EEBA4 (GUESS) - sends this console's binary record to friend slot `slot` (to every member when
     * `broadcast` is set); nothing while this console has no transfer slot. */
    void sendPeerRecord(s8 slot, s32 broadcast);
    /* 0x803EECFC (GUESS) - the members' session state: 1 every flagged friend connected, 0 one still pending or
     * connecting, -1 one failed. */
    s32 checkMemberSessions();
    /* 0x803EA128 / 0x803EA34C (GUESS) - the two list runs a layer change finishes with, each its own step byte
     * (`listPending_3C0[1]`/`[2]`): leave to the parent layer and re-read its child info (and user list when hosting),
     * and re-read the child list (city list) in batches of 10; each returns 0 while it waits, 1 when done, -1 when
     * the session dropped and -2 when it was cancelled. */
    s32 stepLayerLeave(NetworkLayerRequest* request);
    s32 stepChildListRead(NetworkLayerRequest* request);

    /* +0x1C0 */ NetworkLayerPatRequest* ownRequests_1C0[1];   /* the layer's own request slot (index 21 of `moveRequests`) */
    /* +0x1C4 */ u8 ownRequestState_1C4[1];               /* set while it is moving */
    /* +0x1C5 */ u8 pad_1C5[0x3];
    /* +0x1C8 */ NetworkLayerPatRequest pool_1C8[2];   /* the layer's own two requests (`__construct_array`, 0xA4 each) */
    /* +0x0310 */ u32 requestFlags_310[22];   /* per request slot (`requests_0C`'s index): bit0 = session lost, bit1 =
                                                cancelled, higher bits = the replies the reflect callback saw */
    /* +0x0368 */ u32 requestIds_368[22];     /* per request slot (21 = the layer's own): the id the last `sendReq*` returned */
    /* +0x03C0 */ u8 listPending_3C0[3];   /* GUESS: cleared by 0x803E9C78 / 0x803EA11C / 0x803EA340 before a list run */
    /* +0x03C3 */ u8 pad_03C3;
    /* +0x03C4 */ f32 timers_3C4[3];   /* `clear` sets each to -3600 (GUESS: last-sent times, long ago) */
    /* +0x03D0 */ u8 connected_3D0;   /* zero = no session: the request cannot start */
    /* +0x03D1 */ u8 busy_3D1;   /* set while a request is in flight; the finishing states clear it */
    /* +0x03D2 */ u8 flag_3D2;
    /* +0x03D3 */ u8 flag_3D3;
    /* +0x03D4 */ u32 matchKey_3D4;   /* the layer's match key (a created layer's `PatLayerData::matchKey_070`) */
    /* +0x03D8 */ u32 jumpTicket_3D8;   /* GUESS: the word the jump-ready reply carries; jump-go sends it back */
    /* +0x03DC */ NetworkUniqueId serverId_3DC;   /* the server's unique id (`readServerId_2C` copies it out) */
    /* +0x03FC */ char serverName_3FC[0x14];   /* `readServerName_30` copies at most 19 characters */
    /* +0x0410 */ u8 serverFlag_410;
    /* +0x0411 */ u8 pad_0411[0x3];
    /* +0x0414 */ NetUserPosition lastPosition_414;   /* the last position `sendUserPosition_60` published */
    /* +0x042C */ u32 serverValue_42C;
    /* +0x0430 */ u32 serverValue_430;
    /* +0x0434 */ u16 serverPort_434;
    /* +0x0436 */ u8 pad_0436[0x2];
    /* +0x0438 */ u32 negotiateFrom_438;   /* the peer ids of the pair `move` is negotiating (0: none) */
    /* +0x043C */ u32 negotiateTo_43C;
    /* +0x0440 */ u8 negotiated_440;       /* set once the pair's result was recorded */
    /* +0x0441 */ u8 pad_0441[0x3];
    /* +0x0444 */ NetworkErrorInfo sessionError_444;   /* the error a failed member session left while a request ran */
    /* +0x0450 */ NetLayerSettings presence_450;       /* `setPresence_A0`'s pairs; `move` sends them while `flag_3D2` is set */
    /* +0x0474 */ NetLayerAddress address_474;   /* this console's layer address (`exportLayerId_8C` exports it as kind 3) */
    /* +0x0484 */ char userName_484[0x40];   /* `readUserName_90` copies at most 63 characters */
    /* +0x04C4 */ u8 hostMode_4C4;   /* selects the user-list step after the child-info reply */
    /* +0x04C5 */ u8 pad_04C5[0x3];
    /* +0x04C8 */ s32 memberCount_4C8;   /* must be positive for a request to start */
    /* +0x04CC */ NetLayerInfoRec layerInfo_4CC;   /* the record the layer-info handlers report with their done event */
    /* +0x0540 */ NetCityList cities_540;
    /* +0x18A4 */ NetRoomList rooms_18A4;
    /* +0x3568 */ NetFriendTable friends_3568;                  /* the friend slots (with the sessions below, the shape of a
                                                                   `NetFriendRoster`; separate members: the destructor calls
                                                                   the table's destructor with no null check) */
    /* +0x4B4C */ NetFriendSession friendSessions_4B4C[100];   /* per friend slot */
    /* +0x595C */ NetFriendDetail details_595C[100];
    /* +0xBEEC */ u32 friendStatus_BEEC[100];   /* per friend slot (GUESS: a status word `clearFriendSlot` resets) */
    /* +0xC07C */ s8 transferSlot_C07C;   /* the friend slot whose transfer state the mode setters refresh (-1: none) */
    /* +0xC07D */ u8 pad_C07D[0x3];
    /* +0xC080 */ s32 sessionState_C080;   /* `resetLayerState` sets -2 */
    /* +0xC084 */ u8 friendFlagC084_C084[100];
    /* +0xC0E8 */ u8 friendTransfer_C0E8[100];   /* per friend slot: the transfer is active */
    /* +0xC14C */ u8 friendTransferMode_C14C[100];
    /* +0xC1B0 */ GameSpyPeerId friendPeers_C1B0[100];   /* per friend slot: its GameSpy peer id (`move` negotiates pairs) */
    /* +0xC7F0 */ s8 pairState_C7F0[100][100];           /* per pair of friend slots: the NAT negotiation state
                                                            (GUESS: 0 none, 1 negotiated, 2 running, 3 failed, 4 retry) */
    /* +0xEF00 */ s8 friendSession_EF00[100];        /* per friend slot: the session slot linked to it, -1 when none */
    /* +0xEF64 */ u8 memberStatus_EF64[100];   /* per friend slot: 0 pending, 1 failed, 2 connecting, 3 connected */
    /* +0xEFC8 */ u8 friendFlagEFC8_EFC8[100];
    /* +0xF02C */ s32 memberUsed_F02C;   /* the member count `notifyLayerSlotSummary` reports when not hosting */
    /* +0xF030 */ u32 status_F030;
    /* +0xF034 */ u32 status_F034;
    /* +0xF038 */ u32 status_F038;
    /* +0xF03C */ NetLayerSettings layerSettings_F03C;   /* the current layer's settings (its info answers) */
    /* +0xF060 */ char comment_F060[0x40];   /* `setComment_94` copies at most 63 characters */
    /* +0xF0A0 */ NetLayerSettings settings_F0A0;
    /* +0xF0C4 */ NetLayerRequest request_F0C4;   /* `submitRequest_9C`'s record */
    /* +0xF130 */ NetLayerRequest select_F130;    /* `submitSelect_A4`'s record */
    /* +0xF19C */ s32 pendingRequestId_F19C;   /* GUESS: a request id is already pending when it is >= 0 */
    /* +0xF1A0 */ s32 listCursor_F1A0;   /* the next row a list read asks for */
    /* +0xF1A4 */ s32 listTotal_F1A4;    /* the rows the list head announced */
    /* +0xF1A8 */ u8 parentInfo_F1A8;   /* set while the layer-info request asks for the parent layer */
    /* +0xF1A9 */ u8 pad_F1A9[0x3];
    /* +0xF1AC */ NetCommunityList communities_F1AC;
    /* +0x6BC30 */ NetFriendList friendList_6BC30;
    /* +0x6E024 */ NetLayerRecord layerRecord_6E024;   /* the record `handleLayerInfoById` reports with its done event */
    /* +0x6E128 */ NetLayerMediationList mediationList_6E128;
    /* +0x6E5AC */ s16 voiceInput_6E5AC[0x17A];   /* the microphone samples and a peer's popped transfer record */
    /* +0x6E8A0 */ s16 voiceSend_6E8A0[0x17A];    /* the downsampled samples `move` posts to the session members */
    /* +0x6EB94 */ NetVoiceBuffer voiceMixed_6EB94;   /* "mVoiceMixed" (the move warning names it) */
};

/* The free functions of `src/Network/NetworkLayerPat.cpp`'s range (0x803E0BE8..0x803EF668; rule 2: the owner
 * declares). */
struct PatTerms;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803E247C - the terms object; 0x80416A18 - whether it reached its update-finished state. */
struct PatTerms* getPatTerms(void);

/* 0x803E0BE8 - the server message callback `NetworkLayerPat::handleConnect` installs (`setCallback(..., 4)`): it
 * re-orders the six arguments and runs `NetworkLayerPat::reflect` on the sixth. */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
void networkLayerReflectCallback(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5);

#ifdef __cplusplus
}
#endif

#endif
