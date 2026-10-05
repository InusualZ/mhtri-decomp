/*
 * include/Network/NetworkLayer.h - the declarations of `src/Network/NetworkLayer.cpp`: the layer base class `NetworkLayer`,
 * its request record `NetworkLayerRequest`, the layer user id and the free functions of the unit's range
 * (0x803DF2EC..0x803E0BE8).  Moved here from `Network/NetworkLayerPat.h` and `Network/NetworkSessionManagerPat.h` when the
 * recut gave the class its own unit (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORKLAYER_H
#define MHTRI_NETWORK_NETWORKLAYER_H

#include "types.h"

/* The 0x44-byte layer user id `NetworkLayerIdImportFrom`/`NetworkLayerIdExportTo` fill and read (their own log
 * strings name them): an id kind (1..5) and up to 0x40 id bytes. */
typedef struct NetworkLayerId {
    /* +0x00 */ u8 kind_00;
    /* +0x01 */ u8 pad_01[0x03];
    /* +0x04 */ u8 data_04[0x40];
} NetworkLayerId;   /* size: 0x44 */

class NetworkLayer;
struct NetworkLayerRequest;
/* The records the pure slots take, defined beside the class that implements them (include/Network/NetworkLayerPat.h). */
struct NetId;
struct NetUserFields;
struct NetUserPosition;
struct NetCityRec;
struct NetLayerSettings;
struct NetLayerRequest;
struct NetworkErrorInfo;   /* the 12-byte error record - Network/gamespy_interface_types.h */
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
    void setRecord(u32 code, u32 arg_a, u32 arg_b);   /* 0x803E3610 - stores the error record under the mutex */
    s32 getArgument(u32 index);                /* 0x803E3C70 - argument `index`, 0 (with a warning) past `count_28` */
} NetworkLayerRequest;   /* size: 0xA4 (the pool's element size) */

/* The layer base class: the request pool and the lazy request starters `NetworkLayerPat` builds on.  Its
 * table is 0x805FB5D0 (two RTTI words and 79 slots, +0x08..+0x140 - the derived table 0x805FC1E0 is 0x144 B, so
 * the map's 0x148 extent carries 4 bytes of alignment), emitted by `Network/NetworkLayer.cpp`, which defines the
 * key function (the destructor).  The layout parallels `NetworkSessionManager` one word earlier (no session word
 * at +0x0C): 21 request slots, their 21 running flags, two flag bytes and a pool of two requests.  The slots the
 * original leaves 0 are pure virtual and carry the name and parameters of `NetworkLayerPat`'s override (its body
 * is the evidence); the 21 handler slots +0xF4..+0x140 are the targets of the request descriptors (`{0, slot, 0}`
 * member-function pointers).  Every name except the class's own (the "NetworkLayer::move"/
 * "NetworkLayer::deleteRequest" strings) is derived from the slot offset, the override's body or the field it
 * touches (GUESS). */
class NetworkLayer {
public:
    NetworkLayer();
    virtual ~NetworkLayer();                       /* +0x008 */
    virtual void setReflectCallback(u32 callback, u32 user); /* +0x00C */
    virtual void clear();                          /* +0x010 */
    virtual void release();                        /* +0x014 */
    virtual void move();                           /* +0x018 */
    virtual void closeSession_1C();   /* +0x01C */
    virtual void shutdown_20();   /* +0x020 */
    virtual void requestServers_24(s32 count);   /* +0x024 */
    virtual void selectServer_28(s32 id);   /* +0x028 */
    virtual void readServerId_2C(NetId* id) = 0;                 /* +0x02C */
    virtual void readServerName_30(char* out, s32 size) = 0;     /* +0x030 */
    virtual void readServerText_34(char* out, s32 size) = 0;     /* +0x034 */
    virtual void request_38();                     /* +0x038 */
    virtual void selectCity_3C(s32 id);   /* +0x03C */
    virtual void request_40(u32 a, u32 b, u32 c);  /* +0x040 */
    virtual void request_44(u32 a);                /* +0x044 */
    virtual void requestCities_48(s32 count);   /* +0x048 */
    virtual void request_4C(u32 a);                /* +0x04C */
    virtual void request_50(u32 a);                /* +0x050 */
    virtual void request_54(u32 a, u32 b);         /* +0x054 */
    virtual void request_58(u32 a);                /* +0x058 */
    virtual void sendUserFields_5C(NetUserFields* fields) = 0;              /* +0x05C */
    virtual void sendUserPosition_60(const NetUserPosition* position) = 0;  /* +0x060 */
    virtual void sendMessage_64(const char* text, s32 value, s32 flag, u32 flags);   /* +0x064 */
    virtual void setPageSize_68(s32 size);   /* +0x068 */
    virtual void requestRefresh_6C(s32 code, s32 mask);   /* +0x06C */
    virtual void request_70(u32 a, u32 b);         /* +0x070 */
    virtual void request_74(u32 a);                /* +0x074 */
    virtual void request_78(u32 a, u32 b);         /* +0x078 */
    virtual void request_7C(u32 a, u32 b);         /* +0x07C */
    virtual void request_80(u32 a);                /* +0x080 */
    virtual void requestAccount_84(s32 kind);   /* +0x084 */
    virtual void readSelectedServer_88(NetCityRec* out) = 0;                 /* +0x088 */
    virtual void exportLayerId_8C(NetworkLayerId* out) = 0;                  /* +0x08C */
    virtual void readUserName_90(char* out, s32 size) = 0;                   /* +0x090 */
    virtual void setComment_94(const char* text) = 0;                        /* +0x094 */
    virtual void submitSettings_98(NetLayerSettings* settings) = 0;          /* +0x098 */
    virtual void submitRequest_9C(NetLayerRequest* request) = 0;             /* +0x09C */
    virtual void setPresence_A0(const NetLayerSettings* presence) = 0;       /* +0x0A0 */
    virtual void submitSelect_A4(NetLayerRequest* request) = 0;              /* +0x0A4 */
    virtual s32 getCommunityCount_A8() = 0;                                  /* +0x0A8 */
    virtual u8 getCommunityState_AC(s32 index) = 0;                          /* +0x0AC */
    virtual void readCommunityComment_B0(s32 index, char* out, s32 size) = 0; /* +0x0B0 */
    virtual u32 getCommunityValueE0_B4(s32 index) = 0;                       /* +0x0B4 */
    virtual u32 getCommunityValueE4_B8(s32 index) = 0;                       /* +0x0B8 */
    virtual u32 getCommunityValueE8_BC(s32 index) = 0;                       /* +0x0BC */
    virtual void readCommunitySettings_C0(s32 index, NetLayerSettings* out) = 0; /* +0x0C0 */
    virtual void readCommunityHeader_C4(s32 index, u8* out) = 0;             /* +0x0C4 */
    virtual void readRoomHeader_C8(s32 index, u8* out) = 0;                  /* +0x0C8 */
    virtual void setFlag75(u32 value);             /* +0x0CC */
    virtual u8 getFlag75();                        /* +0x0D0 */
    virtual void setFlag76(u8 value);              /* +0x0D4 */
    virtual void setMediatorValue_D8(u32 value) = 0;                         /* +0x0D8 */
    virtual u8 getMediatorValue_DC() = 0;                                    /* +0x0DC */
    virtual void setFriendTransferMode_E0(s8 slot, u32 mode) = 0;            /* +0x0E0 */
    virtual BOOL isFriendTransferActive_E4(s8 slot) = 0;                     /* +0x0E4 */
    virtual BOOL isFriendTransferReady_E8(s8 slot) = 0;                      /* +0x0E8 */
    virtual u8 getFriendFlagC084_EC(s8 slot) = 0;                            /* +0x0EC */
    virtual u8 getFriendTransferFlag_F0(s8 slot) = 0;                        /* +0x0F0 */
    virtual s32 handleConnect(NetworkLayerRequest* request) = 0;          /* +0x0F4 - `request_1C` */
    virtual s32 handleDisconnect(NetworkLayerRequest* request) = 0;       /* +0x0F8 - `request_20` */
    virtual s32 handleServerList(NetworkLayerRequest* request) = 0;       /* +0x0FC - `request_24` */
    virtual s32 handleServerSelect(NetworkLayerRequest* request) = 0;     /* +0x100 - `request_28` */
    virtual s32 stepRequest(NetworkLayerRequest* request) = 0;            /* +0x104 - `request_38` */
    virtual s32 handleLayerCreate(NetworkLayerRequest* request) = 0;      /* +0x108 - `request_3C`, `request_40` */
    virtual s32 handleLayerInfo(NetworkLayerRequest* request) = 0;        /* +0x10C - `request_44` */
    virtual s32 handleChildList(NetworkLayerRequest* request) = 0;        /* +0x110 - `request_48` */
    virtual s32 handleSiblingList(NetworkLayerRequest* request) = 0;      /* +0x114 - `request_4C` */
    virtual s32 handleUserList(NetworkLayerRequest* request) = 0;         /* +0x118 - `request_50`, `request_54` */
    virtual s32 handleUserInfo(NetworkLayerRequest* request) = 0;         /* +0x11C - `request_58` */
    virtual s32 handleChat(NetworkLayerRequest* request) = 0;             /* +0x120 - `request_64` */
    virtual s32 handleDetailSearch(NetworkLayerRequest* request) = 0;     /* +0x124 - `request_68` */
    virtual s32 handleUserSearch(NetworkLayerRequest* request) = 0;       /* +0x128 - `request_6C` */
    virtual s32 handleLayerJump(NetworkLayerRequest* request) = 0;        /* +0x12C - `request_70` */
    virtual s32 handleLayerInfoById(NetworkLayerRequest* request) = 0;    /* +0x130 - `request_74` */
    virtual s32 handleLayerInfoSet(NetworkLayerRequest* request) = 0;     /* +0x134 - `request_78` */
    virtual s32 handleMediationLock(NetworkLayerRequest* request) = 0;    /* +0x138 - `request_7C` */
    virtual s32 handleMediationUnlock(NetworkLayerRequest* request) = 0;  /* +0x13C - `request_80` */
    virtual s32 handleMediationList(NetworkLayerRequest* request) = 0;    /* +0x140 - `request_84` */

    NetworkLayerRequest* allocRequest();
    void deleteRequest(NetworkLayerRequest** slot);

    /* +0x004 */ u32 context_04;                   /* `setReflectCallback`'s first word (the callback) */
    /* +0x008 */ u32 context_08;                   /* `setReflectCallback`'s second word (its user word) */
    /* +0x00C */ NetworkLayerRequest* requests_0C[21];  /* the running request of each starter, 0 when idle */
    /* +0x060 */ u8 requestState_60[21];           /* set while the request of the same index is moving */
    /* +0x075 */ u8 flag_75;                       /* `setFlag75`/`getFlag75`; the constructor sets it */
    /* +0x076 */ u8 flag_76;                       /* `setFlag76`; the constructor sets it */
    /* +0x077 */ u8 pad_077;
    /* +0x078 */ NetworkLayerRequest pool_78[2];        /* the two requests `allocRequest` hands out */
};   /* size: 0x1C0 (the derived constructor's first member array starts at +0x1C8) */

typedef struct NetLayerRequest NetLayerRequest;   /* include/Network/NetworkLayerPat.h */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803DF9A0 / 0x803DFACC / 0x803DFB7C - the layer id helpers, named by their own log strings
 * ("NetworkLayerIdImportFrom: ...", "NetworkLayerIdExportTo: ...", "NetworkUniqueIdEquals: ..."). */
void NetworkLayerIdImportFrom(NetworkLayerId* id, u8 kind, const u8* data, u32 size);
void NetworkLayerIdExportTo(const NetworkLayerId* id, u8* out, u32 size);
BOOL NetworkUniqueIdEquals(const NetworkLayerId* a, const NetworkLayerId* b);

/* 0x803DFC34 - initialises a layer request record. */
void initNetLayerRequest(NetLayerRequest* request);
/* 0x803DFCA8 (GUESS) - copies a layer request record (`NetworkLayerPat::submitRequest_9C`/`submitSelect_A4` store
 * theirs with it): the count, the items, the embedded unique id (through its +0x28 `copyFrom`) and the tail. */
void copyNetLayerRequest(NetLayerRequest* dst, const NetLayerRequest* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKLAYER_H */
