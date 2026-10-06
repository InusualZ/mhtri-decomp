/*
 * Network/NetworkCommunity.h - the declarations of `Network/NetworkCommunity.cpp`: `NetworkCommunity` and its request
 *   record `NetworkCommunityRequest`.
 */
#ifndef MHTRI_NETWORK_NETWORKCOMMUNITY_H
#define MHTRI_NETWORK_NETWORKCOMMUNITY_H

#include "types.h"
#include "Network/NetworkConnection.h"      /* NetworkMutex - the member mutex */

class NetworkCommunity;
struct NetworkCommunityRequest;
class NetworkUniqueId;                                      /* Network/NetworkUniqueId.h */
struct NetworkErrorInfo;                                   /* the 12-byte error record - Network/gamespy_interface_types.h */

/* The handler a community request runs: a member function of the layer that reports completion (non-zero). */
typedef s32 (NetworkCommunity::*NetworkCommunityHandler)(NetworkCommunityRequest* request);

/* The reflect callback the pat control installs (`communityReflectCallback`): the command, its result code, the count
 * of records at `data` and the user word `setReflectCallback` took. */
/* untyped: caller-owned payload - the record each command delivers */
typedef s32 (*NetworkCommunityReflectCallback)(u32 command, s32 result, s32 count, void* data, u32 user);

/* The community layer's request record: the layout of `NetworkLayerRequest` (Network/NetworkLayer.h) with the
 * owner and the handler typed for this class.  `run` is MWCC's member-function-pointer call (`__ptmf_scall`), `reset`
 * assigns the null member pointer (`__ptmf_null`); the constructor/destructor pair is the one the pool array is built
 * and destroyed with (`__construct_array`/`__destroy_arr`, element size 0xA4).  Fields keep `NetworkLayerRequest`'s
 * names. */
struct NetworkCommunityRequest {
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
    /* +0x94 */ NetworkCommunity* owner_94;     /* set while the request runs */
    /* +0x98 */ NetworkCommunityHandler handler_98;

    NetworkCommunityRequest();    /* builds the record's mutex, then resets it */
    ~NetworkCommunityRequest();   /* resets the record and releases its mutex */
    void reset();
    void clear();
    s32 isOwned();
    void run();
    void begin(NetworkCommunity* owner, NetworkCommunityHandler handler, u32 count, ...);
    s32 getRecord(NetworkErrorInfo* out);       /* inline - defined where it is used (Network/NetworkCommunityPat.cpp) */
    s32 getArgument(u32 index);                    /* inline - the same */
    void setRecord(u32 code, u32 arg0, u32 arg1);  /* inline - the same */
};   /* size: 0xA4 (the pool's element size) */

/* The community layer base class `NetworkCommunityPat` builds on: twelve request slots and a pool of two requests,
 * the `NetworkLayer` shape without the running flags.  Its table is 0x805FC440 (0x90 B: two RTTI words and 34
 * slots), emitted by `Network/NetworkCommunity.cpp`, which defines the key function (the destructor).  The slots the
 * original leaves 0 are pure virtual; the ten handler slots +0x68..+0x8C are the targets of the request descriptors.
 * The class name is the "NetworkCommunity::deleteRequest" string's; every other name is derived from the slot offset,
 * the field it touches or what the pat control passes (GUESS). */
class NetworkCommunity {
public:
    NetworkCommunity();
    virtual ~NetworkCommunity();                                      /* +0x08 */
    virtual void setReflectCallback(u32 callback, u32 user);          /* +0x0C - the reflect callback and its user word */
    virtual void clear();                                             /* +0x10 */
    virtual void release();                                           /* +0x14 */
    virtual void move();                                              /* +0x18 */
    virtual void openCommunity_1C();                                  /* +0x1C */
    virtual void shutdown_20();                                       /* +0x20 */
    virtual void getSelfId(NetworkUniqueId* out) = 0;                 /* +0x24 - copies this player's id out */
    virtual void getName(char* out, s32 size) = 0;                    /* +0x28 - this player's name, terminated */
    virtual void getTag(char* out, s32 size) = 0;                     /* +0x2C - the one-byte tag after the name */
    virtual void request_30();                                        /* +0x30 */
    virtual void request_34();                                        /* +0x34 */
    virtual void requestNews_38();                                    /* +0x38 */
    virtual void request_3C();                                        /* +0x3C */
    virtual void rejectRequest_40() = 0;                              /* +0x40 - reports the unsupported-request code */
    virtual s32 isFriend(const NetworkUniqueId* id) = 0;              /* +0x44 - whether `id` is on the friend list */
    virtual void request_48(u32 a, u32 b, u32 c);                     /* +0x48 */
    virtual void writeProfile_4C(const u8* data, u32 size) = 0;       /* +0x4C - the whole profile block */
    virtual void writeProfileRange_50(const u8* data, u32 size, u32 offset) = 0;   /* +0x50 - offset + size <= 0x100 */
    virtual void request_54(u32 a, u32 b);                            /* +0x54 - `request_58` with a zero third word */
    virtual void request_58(u32 a, u32 b, u32 c);                     /* +0x58 */
    virtual void request_5C(const u8* data, s32 id, u32 size);        /* +0x5C - `request_60` with zero flags */
    virtual void request_60(const u8* data, s32 id, u32 size, u32 flags);   /* +0x60 */
    virtual void request_64(u32 a, u32 b, u32 c);                     /* +0x64 */
    virtual s32 handle_68(NetworkCommunityRequest* request) = 0;      /* +0x68 - `openCommunity_1C` */
    virtual s32 handle_6C(NetworkCommunityRequest* request) = 0;      /* +0x6C - `shutdown_20` */
    virtual s32 handle_70(NetworkCommunityRequest* request) = 0;      /* +0x70 - `request_30` */
    virtual s32 handle_74(NetworkCommunityRequest* request) = 0;      /* +0x74 - `request_34` */
    virtual s32 handle_78(NetworkCommunityRequest* request) = 0;      /* +0x78 - `requestNews_38` */
    virtual s32 handle_7C(NetworkCommunityRequest* request) = 0;      /* +0x7C - `request_3C` */
    virtual s32 handle_80(NetworkCommunityRequest* request) = 0;      /* +0x80 - `request_48` */
    virtual s32 handle_84(NetworkCommunityRequest* request) = 0;      /* +0x84 - `request_58` */
    virtual s32 handle_88(NetworkCommunityRequest* request) = 0;      /* +0x88 - `request_60` */
    virtual s32 handle_8C(NetworkCommunityRequest* request) = 0;      /* +0x8C - `request_64` */

    NetworkCommunityRequest* allocRequest();
    void deleteRequest(NetworkCommunityRequest** slot);

    /* +0x004 */ NetworkCommunityReflectCallback reflectCallback_04;   /* `setReflectCallback`'s first word */
    /* +0x008 */ u32 reflectUser_08;                      /* `setReflectCallback`'s second word */
    /* +0x00C */ NetworkCommunityRequest* requests_0C[12];  /* the running request of each starter, 0 when idle */
    /* +0x03C */ NetworkCommunityRequest pool_3C[2];       /* the two requests `allocRequest` hands out */
};   /* size: 0x184 */

#endif /* MHTRI_NETWORK_NETWORKCOMMUNITY_H */
