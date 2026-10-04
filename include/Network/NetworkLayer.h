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
} NetworkLayerRequest;   /* size: 0xA4 (the pool's element size) */

/* The layer base class: the request pool and the lazy request starters `NetworkLayerPat` builds on.  Its
 * table is 0x805FB5D0 (0x148 B: two RTTI words and 80 slots), emitted by `Network/NetworkLayer.cpp`,
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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKLAYER_H */
