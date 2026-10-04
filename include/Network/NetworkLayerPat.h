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

/* The friend list the two friend-list sources share: a count followed by the entries. */
typedef struct NetFriendList {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ NetFriendEntry entries_0x04[1];   /* `count_0x00` entries */
} NetFriendList;

class NetworkLayer;
struct NetworkLayerRequest;
typedef struct NetworkRequestError NetworkRequestError;   /* include/unsplit/Network.h */

/* The handler a layer request runs: a member function of the layer that reports completion (non-zero). */
typedef s32 (NetworkLayer::*NetworkLayerHandler)(NetworkLayerRequest* request);

/* The layer's request record.  The same layout as the session manager's `NetworkRequest`, with the owner and
 * the handler typed for the layer: `run` is MWCC's member-function-pointer call (`mr r4,request;
 * addi r12,request,0x98; bl __ptmf_scall` with the owner in r3) and `reset` assigns the null member pointer
 * (a 12-byte copy of `__ptmf_null`).  The constructor and destructor are the pair the layer's pool array
 * is built and destroyed with (`__construct_array`/`__destroy_arr`, element size 0xA4).  Fields keep
 * `NetworkRequest`'s names. */
typedef struct NetworkLayerRequest {
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
    /* +0x50 */ f32 timeout_50;            /* the start time `NetworkLayerRequest_begin` stamps */
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
    /* +0x78 */ u8 mutex_78[0x1C];
    /* +0x94 */ NetworkLayer* owner_94;     /* set while the request runs */
    /* +0x98 */ NetworkLayerHandler handler_98;

    NetworkLayerRequest();    /* builds the record's mutex, then resets it */
    ~NetworkLayerRequest();   /* resets the record and releases its mutex */
    void reset();
    void clear();
    s32 isOwned();
    void run();
    void begin(NetworkLayer* owner, NetworkLayerHandler handler, u32 count, ...);
    s32 getRecord(NetworkRequestError* out);   /* 0x803E3598 - the error record, under the mutex; false while none is set */
} NetworkLayerRequest;   /* size: 0xA4 (the pool's element size) */

/* The layer base class: the request pool and the lazy request starters `NetworkLayerPat` builds on.  Its
 * table is 0x805FB5D0 (0x148 B: two RTTI words and 80 slots), emitted by `Network/NetworkSessionManagerPat.cpp`,
 * which defines the key function (the destructor).  The layout parallels `NetworkSessionManager` one word
 * earlier (no session word at +0x0C): 21 request slots, their 21 running flags, two flag bytes and a pool of
 * two requests.  The slots the original leaves 0 are pure virtual; the 21 handler slots +0xF4..+0x140 are the
 * targets of the request descriptors (`{0, slot, 0}` member-function pointers).  Every name except the
 * class's own (the "NetworkLayer::move"/"NetworkLayer::deleteRequest" strings) is derived from the slot
 * offset or the field it touches (GUESS). */
class NetworkLayer {
public:
    NetworkLayer();
    virtual ~NetworkLayer();                       /* +0x008 */
    virtual void init(u32 context0, u32 context1); /* +0x00C */
    virtual void clear();                          /* +0x010 */
    virtual void release();                        /* +0x014 */
    virtual void move();                           /* +0x018 */
    virtual void request_1C();                     /* +0x01C */
    virtual void request_20();                     /* +0x020 */
    virtual void request_24(u32 a);                /* +0x024 */
    virtual void request_28(u32 a);                /* +0x028 */
    virtual void slot_2C() = 0;                    /* +0x02C */
    virtual void slot_30() = 0;                    /* +0x030 */
    virtual void slot_34() = 0;                    /* +0x034 */
    virtual void request_38();                     /* +0x038 */
    virtual void request_3C(u32 a);                /* +0x03C */
    virtual void request_40(u32 a, u32 b, u32 c);  /* +0x040 */
    virtual void request_44(u32 a);                /* +0x044 */
    virtual void request_48(u32 a);                /* +0x048 */
    virtual void request_4C(u32 a);                /* +0x04C */
    virtual void request_50(u32 a);                /* +0x050 */
    virtual void request_54(u32 a, u32 b);         /* +0x054 */
    virtual void request_58(u32 a);                /* +0x058 */
    virtual void slot_5C() = 0;                    /* +0x05C */
    virtual void slot_60() = 0;                    /* +0x060 */
    virtual void request_64(u32 a, u32 b, u32 c, u32 d); /* +0x064 */
    virtual void request_68(u32 a);                /* +0x068 */
    virtual void request_6C(u32 a, u32 b);         /* +0x06C */
    virtual void request_70(u32 a, u32 b);         /* +0x070 */
    virtual void request_74(u32 a);                /* +0x074 */
    virtual void request_78(u32 a, u32 b);         /* +0x078 */
    virtual void request_7C(u32 a, u32 b);         /* +0x07C */
    virtual void request_80(u32 a);                /* +0x080 */
    virtual void request_84(u32 a);                /* +0x084 */
    virtual void slot_88() = 0;                    /* +0x088 */
    virtual void slot_8C() = 0;                    /* +0x08C */
    virtual void slot_90() = 0;                    /* +0x090 */
    virtual void slot_94() = 0;                    /* +0x094 */
    virtual void slot_98() = 0;                    /* +0x098 */
    virtual void slot_9C() = 0;                    /* +0x09C */
    virtual void slot_A0() = 0;                    /* +0x0A0 */
    virtual void slot_A4() = 0;                    /* +0x0A4 */
    virtual void slot_A8() = 0;                    /* +0x0A8 */
    virtual void slot_AC() = 0;                    /* +0x0AC */
    virtual void slot_B0() = 0;                    /* +0x0B0 */
    virtual void slot_B4() = 0;                    /* +0x0B4 */
    virtual void slot_B8() = 0;                    /* +0x0B8 */
    virtual void slot_BC() = 0;                    /* +0x0BC */
    virtual void slot_C0() = 0;                    /* +0x0C0 */
    virtual void slot_C4() = 0;                    /* +0x0C4 */
    virtual void slot_C8() = 0;                    /* +0x0C8 */
    virtual void setFlag75(u8 value);              /* +0x0CC */
    virtual u8 getFlag75();                        /* +0x0D0 */
    virtual void setFlag76(u8 value);              /* +0x0D4 */
    virtual void slot_D8() = 0;                    /* +0x0D8 */
    virtual void slot_DC() = 0;                    /* +0x0DC */
    virtual void slot_E0() = 0;                    /* +0x0E0 */
    virtual void slot_E4() = 0;                    /* +0x0E4 */
    virtual void slot_E8() = 0;                    /* +0x0E8 */
    virtual void slot_EC() = 0;                    /* +0x0EC */
    virtual void slot_F0() = 0;                    /* +0x0F0 */
    virtual s32 handle_F4(NetworkLayerRequest* request) = 0;   /* +0x0F4 - `request_1C` */
    virtual s32 handle_F8(NetworkLayerRequest* request) = 0;   /* +0x0F8 - `request_20` */
    virtual s32 handle_FC(NetworkLayerRequest* request) = 0;   /* +0x0FC - `request_24` */
    virtual s32 handle_100(NetworkLayerRequest* request) = 0;  /* +0x100 - `request_28` */
    virtual s32 handle_104(NetworkLayerRequest* request) = 0;  /* +0x104 - `request_38` */
    virtual s32 handle_108(NetworkLayerRequest* request) = 0;  /* +0x108 - `request_3C`, `request_40` */
    virtual s32 handle_10C(NetworkLayerRequest* request) = 0;  /* +0x10C - `request_44` */
    virtual s32 handle_110(NetworkLayerRequest* request) = 0;  /* +0x110 - `request_48` */
    virtual s32 handle_114(NetworkLayerRequest* request) = 0;  /* +0x114 - `request_4C` */
    virtual s32 handle_118(NetworkLayerRequest* request) = 0;  /* +0x118 - `request_50`, `request_54` */
    virtual s32 handle_11C(NetworkLayerRequest* request) = 0;  /* +0x11C - `request_58` */
    virtual s32 handle_120(NetworkLayerRequest* request) = 0;  /* +0x120 - `request_64` */
    virtual s32 handle_124(NetworkLayerRequest* request) = 0;  /* +0x124 - `request_68` */
    virtual s32 handle_128(NetworkLayerRequest* request) = 0;  /* +0x128 - `request_6C` */
    virtual s32 handle_12C(NetworkLayerRequest* request) = 0;  /* +0x12C - `request_70` */
    virtual s32 handle_130(NetworkLayerRequest* request) = 0;  /* +0x130 - `request_74` */
    virtual s32 handle_134(NetworkLayerRequest* request) = 0;  /* +0x134 - `request_78` */
    virtual s32 handle_138(NetworkLayerRequest* request) = 0;  /* +0x138 - `request_7C` */
    virtual s32 handle_13C(NetworkLayerRequest* request) = 0;  /* +0x13C - `request_80` */
    virtual s32 handle_140(NetworkLayerRequest* request) = 0;  /* +0x140 - `request_84` */
    virtual void slot_144() = 0;                   /* +0x144 */

    NetworkLayerRequest* allocRequest();
    void deleteRequest(NetworkLayerRequest** slot);

    /* +0x004 */ u32 context_04;                   /* `init`'s first word */
    /* +0x008 */ u32 context_08;                   /* `init`'s second word */
    /* +0x00C */ NetworkLayerRequest* requests_0C[21];  /* the running request of each starter, 0 when idle */
    /* +0x060 */ u8 requestState_60[21];           /* set while the request of the same index is moving */
    /* +0x075 */ u8 flag_75;                       /* `setFlag75`/`getFlag75`; the constructor sets it */
    /* +0x076 */ u8 flag_76;                       /* `setFlag76`; the constructor sets it */
    /* +0x077 */ u8 pad_077;
    /* +0x078 */ NetworkLayerRequest pool_78[2];        /* the two requests `allocRequest` hands out */
};   /* size: 0x1C0 (the derived constructor's first member array starts at +0x1C8) */

class NetworkLayerPat {   /* size: 0x6BC34+ (approximate) */
public:
    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void pad_0C();
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
    /* +0x4C */ virtual void pad_4C();
    /* +0x50 */ virtual void pad_50();
    /* +0x54 */ virtual void pad_54();
    /* +0x58 */ virtual void pad_58();
    /* +0x5C */ virtual void pad_5C();
    /* +0x60 */ virtual void pad_60();
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
    /* +0x8C */ virtual void pad_8C();
    /* +0x90 */ virtual void pad_90();
    /* +0x94 */ virtual void pad_94();
    /* +0x98 */ virtual void submitSettings_98(NetLayerSettings* settings);
    /* +0x9C */ virtual void submitRequest_9C(NetLayerRequest* request);
    /* +0xA0 */ virtual void pad_A0();
    /* +0xA4 */ virtual void submitSelect_A4(NetLayerRequest* request);

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
    /* +0x6BC34 */ NetFriendEntry friends_6BC34[1];   /* `friendCount_6BC30` entries */

    /* Advances the request state in `request` by one step; true once the request finished. */
    bool stepRequest(NetworkLayerRequest* request);
    /* 0x803EF1D4 (GUESS) - the member slot whose friend entry carries address `id`, -1 when none does. */
    s8 getMemberSlot(const NetworkSmallObject* id);
    /* 0x803EF220 (GUESS) - copies member slot `slot`'s address into `out` (cleared first). */
    void getMemberAddress(s8 slot, NetworkSmallObject* out);
};

#endif
