/*
 * include/Network/NetworkSessionManager.h - the externs and layouts the Network session band needs.
 *
 * The types are unit-local (nothing else includes it yet): `NetworkSessionManager` and `NetworkRequest`
 * (`NetworkSessionStable` lives in `Network/NetworkSessionStable.h`) are reconstructed here from the range's own
 * disassembly (every field offset is the one the target instructions address).  `NetworkSessionManager`
 * is a CLASS with inheritance (rule 10): the base declares the 112-slot vtable the DOL carries at
 * 0x805FA908, so MWCC emits the table and the vptr store instead of the unit writing them by hand; the
 * 61 slots the original leaves 0 are pure virtual, and `NetworkSessionManagerPat` declares the override
 * that fills each one (see that class's own comment).  The
 * bit-stream writer and the neighbouring `fn_` helpers are declared `extern "C"` because their owners
 * are still unsplit; per brief section 6.5 rule 2 those sites are the named unsplit gap.
 */

#ifndef NETWORK_NetworkSessionManager_H
#define NETWORK_NetworkSessionManager_H

#include "types.h"
#include "Network/network_transport.h"
#include "Network/network_shared_data.h"
#include "Network/network_writer_types.h"
#include "Network/sGameSpyInterfaceThread.h"   /* sGameSpyInterfaceThread - owner Network/fn_8041A87C.cpp */
#include "Runtime.PPCEABI.H/ptmf.h"

/* The 0x60-byte record block `NetworkRequest_copyRecord` moves.  Only its size (24 words) is
   evidenced - the two leading scalars are how MWCC splits the copy, not a field the range reads. */
typedef struct NetworkSessionRecordPair {
    u32 lo_00;         /* +0x00 */
    u32 hi_04;         /* +0x04 */
} NetworkSessionRecordPair;   /* size: 0x08 */

typedef struct NetworkSessionRecordBlock {
    u32 first_00;      /* +0x00 */
    u32 second_04;     /* +0x04 */
    NetworkSessionRecordPair rest_08[11];   /* +0x08..+0x5F */
} NetworkSessionRecordBlock;   /* size: 0x60 */

/* The record classes the Pat layer's arrays hold.  Each size is evidenced (the `__construct_array` /
   `__destroy_arr` element sizes, and the field offsets the range addresses); only the one member each
   constructor initialises is named, everything else is filler. */
typedef struct NetworkSessionSlotInfo {
    NetworkSmallObject smallObject_00;   /* +0x00 */
    u8 pad_10[0x64];                     /* +0x10..+0x73 (size not evidenced; only +0x00 is built) */
} NetworkSessionSlotInfo;   /* size: 0x74 (approximation) */

typedef struct NetworkSessionCircleInfo {
    u8 pad_000[0x108];
    NetworkSmallObject smallObject_108;   /* +0x108 */
    u8 pad_118[0x204];                    /* +0x118..+0x31B */
} NetworkSessionCircleInfo;   /* size: 0x31C */

typedef struct NetworkSessionCircleList {
    u8 pad_00[0x04];
    NetworkSessionCircleInfo items_04[32];   /* +0x04..+0x6383 */
} NetworkSessionCircleList;   /* size: 0x6384 */

typedef struct NetworkSessionPlayerRecord {
    u8 pad_00[0x08];
    NetworkSmallObject smallObject_08;   /* +0x08 */
    u8 pad_18[0x30];                     /* +0x18..+0x47 */
} NetworkSessionPlayerRecord;   /* size: 0x48 */

/* The variadic helpers: the pinned toolchain ships no `<stdarg.h>`, so the CodeWarrior `va_list`
   layout and the two compiler intrinsics the header expands to are declared here. */
typedef struct NetworkVaState {
    u8  gpr;                 /* +0x00 */
    u8  fpr;                 /* +0x01 */
    u16 reserved;            /* +0x02 */
    u8* input_arg_area;      /* +0x04 */
    u8* reg_save_area;       /* +0x08 */
} NetworkVaState;            /* size: 0x0C */
extern "C" void __va_start(NetworkVaState* ap);
extern "C" u32* __va_arg(NetworkVaState* ap, s32 type);

/* ---------------- NetworkRequest ---------------------------------------------------------- */
typedef struct NetworkRequestDesc {
    u32 id_0;
    u32 value_4;
    u32 type_8;
} NetworkRequestDesc;

typedef struct NetworkRequest {
    s32 state_00;               /* +0x00 - the request's state-machine step */
    u32 unused_04;              /* +0x04 */
    u32 unused_08;              /* +0x08 */
    NetworkBuffer* buffer;  /* +0x0C */
    u32 unused_10;              /* +0x10 */
    u32 unused_14;              /* +0x14 */
    u32 unused_18;              /* +0x18 */
    u32 unused_1C;              /* +0x1C */
    u32 unused_20;              /* +0x20 */
    u32 unused_24;              /* +0x24 */
    u32 count_28;           /* +0x28 */
    u32 args_2C[8];         /* +0x2C..+0x4B */
    f32 interval_4C;              /* +0x4C */
    f32 timeout_50;         /* +0x50 */
    u32 record_54;              /* +0x54 */
    u32 record_58;              /* +0x58 */
    u32 record_5C;              /* +0x5C */
    u32 unused_60;              /* +0x60 */
    u32 unused_64;              /* +0x64 */
    u32 unused_68;              /* +0x68 */
    u32 unused_6C;              /* +0x6C */
    u32 requestId_70;              /* +0x70 */
    u8 cancelled_74;               /* +0x74 */
    u8 pad75[0x03];
    u8 mutex_78[0x1C];      /* +0x78..+0x93 */
    void* owner_94;         /* +0x94 */
    u32 desc_98;            /* +0x98 */
    u32 desc_9C;            /* +0x9C */
    u32 desc_A0;            /* +0xA0 */
} NetworkRequest;           /* size: 0xA4 */

/* ---------------- NetworkSessionManager --------------------------------------------------- */

typedef struct NetworkSessionManagerVtable {
    void* rtti_00;
    void* rtti_04;
    u32 (*destroy_08)(void* self, u32 flags);                    /* +0x08 */
    u32 (*signal_0C)(void* self, u32 a, const char* fmt, ...);   /* +0x0C */
    u32 (*reset_10)(void* self);                                 /* +0x10 */
    u32 (*tick_14)(void* self);                                   /* +0x14 */
    u8 pad18[0x10];
    u32 (*canSend_28)(void* self);                                   /* +0x28 */
    u8 pad2C[0x0C];
    u16 (*put_38)(void* self, u32 a, u32 b, u32 c, u32 d, const void* data, u32 e); /* +0x38 */
    u8 pad3C[0x08];
    void (*flush_44)(void* self);                                  /* +0x44 */
    u8 pad48[0x44];
    f32 (*getFloat_8C)(void* self, s32 idx);                     /* +0x8C */
    s32 (*getInt_90)(void* self, s32 idx);                       /* +0x90 */
    u8 pad94[0xA4];
    void (*sendBatch_138)(void* self, u32 count, const s8* data);      /* +0x138 */
    u8 pad13C[0x84];
    s8 (*mapId_1C0)(void* self, s32 value);                      /* +0x1C0 */
} NetworkSessionManagerVtable;

class NetworkSessionManager {
public:
    NetworkSessionManager();
    virtual ~NetworkSessionManager();                      /* +0x008 */
    virtual void init(u32 a, u32 b);                       /* +0x00C */
    virtual void clear();                                  /* +0x010 */
    virtual void release();                                /* +0x014 */
    virtual void move();                                   /* +0x018 - "NetworkSessionManager::move: ..." */
    virtual void request364();                             /* +0x01C */
    virtual void request368();                             /* +0x020 */
    virtual s32 hasBuffer();                               /* +0x024 */
    virtual u32 canSend_28() = 0;                          /* +0x028 */
    virtual void copyNameList(const u8* src) = 0;         /* +0x02C */
    virtual void copyNameListTail(const u8* src) = 0;     /* +0x030 */
    virtual void setSessionName(const char* name) = 0;    /* +0x034 */
    virtual void setCircleRecords(const u8* src, u32 count) = 0; /* +0x038 */
    virtual void setFlag79(s8 value);                      /* +0x03C */
    virtual void notify(s32 value);                        /* +0x040 */
    virtual void setFlag7A(s8 value);                      /* +0x044 */
    virtual void request372(u32 a, u32 b);                 /* +0x048 */
    virtual void request376(u32 a);                        /* +0x04C */
    virtual void request380(u32 a);                        /* +0x050 */
    virtual void request384(u32 a);                        /* +0x054 */
    virtual void request388(u32 a);                        /* +0x058 */
    virtual void request392();                             /* +0x05C */
    virtual void request396(u32 a, u32 b);                 /* +0x060 */
    virtual void request400(u32 a, u32 b);                 /* +0x064 */
    virtual void slot_068() = 0;                          /* +0x068 */
    virtual void slot_06C() = 0;                          /* +0x06C */
    virtual u32 getCircleInfoCount() = 0;                 /* +0x070 */
    virtual void getCircleItemName(char* dst, u32 size, s32 idx) = 0; /* +0x074 */
    virtual void exportCircleItem(u8* dst, s32 idx) = 0;  /* +0x078 */
    virtual void getCircleItemRecord(char* dst, s32 idx) = 0; /* +0x07C */
    virtual u32 getCircleItemWord_170(s32 idx) = 0;       /* +0x080 */
    virtual u32 getCircleItemWord_174(s32 idx) = 0;       /* +0x084 */
    virtual u32 getCircleItemWord_178(s32 idx) = 0;       /* +0x088 */
    virtual u32 getCircleItemWord_17C(s32 idx) = 0;       /* +0x08C */
    virtual u32 getCircleItemSize_170_178(s32 idx) = 0;   /* +0x090 */
    virtual u32 getCircleItemSize_174_17C(s32 idx) = 0;   /* +0x094 */
    virtual u32 getCircleItemByte_180(s32 idx) = 0;       /* +0x098 */
    virtual u32 slot_09C() = 0;                           /* +0x09C */
    virtual void abortRequest4();                          /* +0x0A0 */
    virtual void abortRequest14();                         /* +0x0A4 */
    virtual void request404(u32 a);                        /* +0x0A8 */
    virtual void request408();                             /* +0x0AC */
    virtual void request412(u32 a, u32 b, s8 c);           /* +0x0B0 */
    virtual void request416(u32 a);                        /* +0x0B4 */
    virtual void request420(s8 a);                         /* +0x0B8 */
    virtual void request424();                             /* +0x0BC */
    virtual void request428(u32 a);                        /* +0x0C0 */
    virtual void request432();                             /* +0x0C4 */
    virtual void request436();                             /* +0x0C8 */
    virtual void request440();                             /* +0x0CC */
    virtual void request444();                             /* +0x0D0 */
    virtual void clearString(char* dst, s32 size) = 0;    /* +0x0D4 */
    virtual u32 getWord_528() = 0;                        /* +0x0D8 */
    virtual u32 getWord_524() = 0;                        /* +0x0DC */
    virtual u32 getWord_530() = 0;                        /* +0x0E0 */
    virtual u32 getWord_52C() = 0;                        /* +0x0E4 */
    virtual u32 getSize_528_530() = 0;                    /* +0x0E8 */
    virtual u32 getSize_524_52C() = 0;                    /* +0x0EC */
    virtual void getPlayerRecordName(u8 idx, char* dst, s32 size) = 0; /* +0x0F0 */
    virtual void clearStringWithId(u32 id, char* dst, s32 size) = 0; /* +0x0F4 */
    virtual s32 getPlayerRecord(u8 idx, u8* dst) = 0;     /* +0x0F8 */
    virtual u32 slot_0FC() = 0;                           /* +0x0FC */
    virtual u8 getByte_534() = 0;                         /* +0x100 */
    virtual f32 getTimeSincePublish() = 0;                /* +0x104 */
    virtual void slot_108() = 0;                          /* +0x108 */
    virtual u32 slot_10C() = 0;                           /* +0x10C */
    virtual u32 slot_110() = 0;                           /* +0x110 */
    virtual u32 slot_114() = 0;                           /* +0x114 */
    virtual u32 slot_118() = 0;                           /* +0x118 */
    virtual u32 slot_11C() = 0;                           /* +0x11C */
    virtual s32 getInt(s8 value);                          /* +0x120 */
    virtual f32 getFloat(s8 value);                        /* +0x124 */
    virtual void broadcastPlayerSlots(u32 a, u32 b);       /* +0x128 */
    virtual void putTerminatorA(u32 a, u32 b, u8 c);       /* +0x12C */
    virtual void putTerminatorB(u32 a, u32 b);             /* +0x130 */
    virtual void putTerminatorC(u32 a, u32 b, u8 c);       /* +0x134 */
    virtual void sendBatch_138(u32 a, u32 b, s32 count, const s8* data);       /* +0x138 */
    virtual void slot_13C(u32 a, u32 b, s32 count, const s8* data, u8 flags);  /* +0x13C */
    virtual void slot_140(u32 a, u32 b, s8 idx);           /* +0x140 */
    virtual void slot_144(u32 a, u32 b, s8 idx, u8 flags); /* +0x144 */
    virtual void flush();                                  /* +0x148 */
    virtual void slot_14C();                               /* +0x14C */
    virtual void slot_150();                               /* +0x150 */
    virtual void slot_154();                               /* +0x154 */
    virtual void slot_158();                               /* +0x158 */
    virtual void slot_15C();                               /* +0x15C */
    virtual void slot_160();                               /* +0x160 */
    virtual void slot_164();                               /* +0x164 */
    virtual void slot_168();                               /* +0x168 */
    virtual void updateSession(NetworkRequest* request) = 0; /* +0x16C */
    virtual void shutdown(NetworkRequest* request) = 0;   /* +0x170 */
    virtual void handleCircleCreate(NetworkRequest* request) = 0; /* +0x174 */
    virtual void slot_178(NetworkRequest* request) = 0;   /* +0x178 */
    virtual void handleCircleListLayer(NetworkRequest* request) = 0; /* +0x17C */
    virtual void handleCircleJoin(NetworkRequest* request) = 0; /* +0x180 */
    virtual void slot_184(NetworkRequest* request) = 0;   /* +0x184 */
    virtual void slot_188(NetworkRequest* request) = 0;   /* +0x188 */
    virtual void handleServerTimeout(NetworkRequest* request) = 0; /* +0x18C */
    virtual void slot_190(NetworkRequest* request) = 0;   /* +0x190 */
    virtual void handleCircleInfoSet(NetworkRequest* request) = 0; /* +0x194 */
    virtual void handleCircleMatchEndInfo(NetworkRequest* request) = 0; /* +0x198 */
    virtual void slot_19C(NetworkRequest* request) = 0;   /* +0x19C */
    virtual void slot_1A0(NetworkRequest* request) = 0;   /* +0x1A0 */
    virtual void slot_1A4(NetworkRequest* request) = 0;   /* +0x1A4 */
    virtual void slot_1A8(NetworkRequest* request) = 0;   /* +0x1A8 */
    virtual void handleCircleMatchOptionSet(NetworkRequest* request) = 0; /* +0x1AC */
    virtual void handleCircleMatchStart(NetworkRequest* request) = 0; /* +0x1B0 */
    virtual void handleGameSpyError(NetworkRequest* request) = 0; /* +0x1B4 */
    virtual void slot_1B8(NetworkRequest* request) = 0;   /* +0x1B8 */
    virtual void handleCircleMatchEnd(NetworkRequest* request) = 0; /* +0x1BC */
    virtual s8 mapId_1C0(s32 value) = 0;                   /* +0x1C0 */
    virtual void slot_1C4(s8 value) = 0;                  /* +0x1C4 */

    u32 unused_04;                             /* +0x04 */
    u32 unused_08;                             /* +0x08 */
    NetworkBuffer* buffer;                     /* +0x0C */
    NetworkRequest* requests_10[21];           /* +0x10..+0x63 */
    u8 request_state_64[21];                   /* +0x64..+0x78 */
    u8 unused_79;                              /* +0x79 */
    u8 unused_7A;                              /* +0x7A */
    u8 pad7B;                                  /* +0x7B */
    NetworkRequest pool_7C[2];                 /* +0x7C..+0x1C3 */
};   /* size: 0x1C4 */

/* ---------------- the Pat band's channel records -------------------------------------------- */

/* The name/entry list `buildCircleInfoName` packs: a count at +0x04 and the entries after it.  The
 * sizes are bounded by the manager's own layout (the list runs from +0x7A0 up to the circle-record
 * count at +0x950), not read from the list itself.  size: 0x1B0 (approximate). */
typedef struct NetworkNameList {
    /* +0x000 */ u32 head_00;
    /* +0x004 */ u32 count_04;
    /* +0x008 */ u8 entries_08[0x1A8];
} NetworkNameList;   /* size: 0x1B0 */

/* The circle-info request block `move` fills and `sendReqCircleInfoSet` sends: the packed records at
 * +0x56 (at most 256 - the copy is capped there), their count as a u16 at +0x156, and the mode byte
 * at +0x378.  `move` zeroes 892 bytes of it, which is the size below. */
typedef struct PatCircleInfo {
    /* +0x000 */ u8 pad_00[0x56];
    /* +0x056 */ u8 records_56[0x100];
    /* +0x156 */ u16 recordCount_156;
    /* +0x158 */ u8 pad_158[0x220];
    /* +0x378 */ u8 mode_378;
    /* +0x379 */ u8 pad_379[0x3];
} PatCircleInfo;   /* size: 0x37C */

/* The Pat band's helpers: the members the manager pumps (`NetworkSingleTcp::move`, `NetworkMultipleUdp::move`)
 * are declared with their classes in `Network/network_transport_types.h`.  The rest have
 * no registered owner, and this header - not `include/unsplit/Network.h` - is where this band's
 * unowned helpers already live (`networkPatAttachBuffer`, `networkPatResetCircleInfo`, `PatInterface_*`),
 * so they are declared beside the records they take.  The first of the three was the map's
 * `fn_803DE524` until the Pat pass renamed it from what its body
 * does (**GUESS**, recorded where it is declared); the other two are the map's own names. */
/* the manager itself is declared below - the band's helpers take it, so name it first, and
 * `NetworkInstance` is the session singleton's class (`include/unsplit/Network.h` defines it; a
 * forward declaration is enough here because only a pointer crosses the call) */
class NetworkSessionManagerPat;
class NetworkInstance;
extern "C" void buildCircleInfoName(NetworkSessionManagerPat* self, char* dst, NetworkNameList* src);
extern "C" s32 circleAvailable(NetworkSessionManagerPat* self);
extern "C" void sendReqCircleInfoSet(NetworkInstance* instance, u32 request_id, PatCircleInfo* info,
                                      const char* name);

/* -------------------------------- NetworkSessionManagerPat ---------------------------------- */

/* The derived class the tail of the range defines.  Declared here so MWCC emits the vptr store and
   the destructor itself (rule 10).  `move` is declared **first** - it is the class's key function, so
   the table is emitted in the TU that defines it (this band) and nowhere else.

   62 OF THE TABLE'S 112 SLOTS ARE THIS CLASS'S OVERRIDES (2026-09-28, slot work).  The base leaves
   those slots pure, so without a declaration here each one is a **zero word** in our emitted table:
   the target holds a code address, ours held 0x00000000 at 61 slots and the base's `setFlag79` at the
   62nd.  The overrides below therefore are not decoration - they are what makes the table carry a
   relocation at every one of the 112 slots the target relocates.

   THE NAMES AND PARAMETER LISTS ARE RECONSTRUCTIONS, and the list is the evidence.  Each body's own
   callees and field offsets name it where they identify it (`copyNameList`, `setSessionName`,
   `setCircleRecords`, `handleCircleJoin`, ...); where the disassembly identifies nothing, the name is
   the vtable offset it fills (`slot_068`), which is this class's own existing scheme
   (`slot_13C`/`slot_140`/`slot_148` in the base).  The parameter lists come from the argument
   registers each body reads before writing them, with the pointer types the bodies demonstrate
   (`NetworkRequest*` for the 21 handlers, which all call `NetworkRequest_getArgument`/
   `getRecord`).  A later pass that writes one of these bodies owns refining both.  The slot census
   (index, address, target, owner) is `python tools/units/vtableaudit.py --at 0x805FB0F8 --json`. */
class NetworkSessionManagerPat : public NetworkSessionManager {
public:
    virtual void move();                     /* +0x018 - the key function, defined in the next band */
    NetworkSessionManagerPat();
    virtual ~NetworkSessionManagerPat();     /* +0x008 */
    virtual void init(u32 a, u32 b);         /* +0x00C */
    virtual void clear();                    /* +0x010 */
    virtual void release();                  /* +0x014 - the Pat flush ("finalNetwork") */

    /* every slot of the class's table that the target fills from outside the base's own band: 62
       overrides, in slot order (each body lives at the address the slot points to) */
    virtual u32 canSend_28();                             /* +0x028 */
    virtual void copyNameList(const u8* src);             /* +0x02C */
    virtual void copyNameListTail(const u8* src);         /* +0x030 */
    virtual void setSessionName(const char* name);        /* +0x034 */
    virtual void setCircleRecords(const u8* src, u32 count); /* +0x038 */
    virtual void setFlag79(s8 value);                     /* +0x03C */
    virtual void slot_068();                              /* +0x068 */
    virtual void slot_06C();                              /* +0x06C */
    virtual u32 getCircleInfoCount();                     /* +0x070 */
    virtual void getCircleItemName(char* dst, u32 size, s32 idx); /* +0x074 */
    virtual void exportCircleItem(u8* dst, s32 idx);      /* +0x078 */
    virtual void getCircleItemRecord(char* dst, s32 idx); /* +0x07C */
    virtual u32 getCircleItemWord_170(s32 idx);           /* +0x080 */
    virtual u32 getCircleItemWord_174(s32 idx);           /* +0x084 */
    virtual u32 getCircleItemWord_178(s32 idx);           /* +0x088 */
    virtual u32 getCircleItemWord_17C(s32 idx);           /* +0x08C */
    virtual u32 getCircleItemSize_170_178(s32 idx);       /* +0x090 */
    virtual u32 getCircleItemSize_174_17C(s32 idx);       /* +0x094 */
    virtual u32 getCircleItemByte_180(s32 idx);           /* +0x098 */
    virtual u32 slot_09C();                               /* +0x09C */
    virtual void clearString(char* dst, s32 size);        /* +0x0D4 */
    virtual u32 getWord_528();                            /* +0x0D8 */
    virtual u32 getWord_524();                            /* +0x0DC */
    virtual u32 getWord_530();                            /* +0x0E0 */
    virtual u32 getWord_52C();                            /* +0x0E4 */
    virtual u32 getSize_528_530();                        /* +0x0E8 */
    virtual u32 getSize_524_52C();                        /* +0x0EC */
    virtual void getPlayerRecordName(u8 idx, char* dst, s32 size); /* +0x0F0 */
    virtual void clearStringWithId(u32 id, char* dst, s32 size); /* +0x0F4 */
    virtual s32 getPlayerRecord(u8 idx, u8* dst);         /* +0x0F8 */
    virtual u32 slot_0FC();                               /* +0x0FC */
    virtual u8 getByte_534();                             /* +0x100 */
    virtual f32 getTimeSincePublish();                    /* +0x104 */
    virtual void slot_108();                              /* +0x108 */
    virtual u32 slot_10C();                               /* +0x10C */
    virtual u32 slot_110();                               /* +0x110 */
    virtual u32 slot_114();                               /* +0x114 */
    virtual u32 slot_118();                               /* +0x118 */
    virtual u32 slot_11C();                               /* +0x11C */
    virtual void updateSession(NetworkRequest* request);  /* +0x16C */
    virtual void shutdown(NetworkRequest* request);       /* +0x170 */
    virtual void handleCircleCreate(NetworkRequest* request); /* +0x174 */
    virtual void slot_178(NetworkRequest* request);       /* +0x178 */
    virtual void handleCircleListLayer(NetworkRequest* request); /* +0x17C */
    virtual void handleCircleJoin(NetworkRequest* request); /* +0x180 */
    virtual void slot_184(NetworkRequest* request);       /* +0x184 */
    virtual void slot_188(NetworkRequest* request);       /* +0x188 */
    virtual void handleServerTimeout(NetworkRequest* request); /* +0x18C */
    virtual void slot_190(NetworkRequest* request);       /* +0x190 */
    virtual void handleCircleInfoSet(NetworkRequest* request); /* +0x194 */
    virtual void handleCircleMatchEndInfo(NetworkRequest* request); /* +0x198 */
    virtual void slot_19C(NetworkRequest* request);       /* +0x19C */
    virtual void slot_1A0(NetworkRequest* request);       /* +0x1A0 */
    virtual void slot_1A4(NetworkRequest* request);       /* +0x1A4 */
    virtual void slot_1A8(NetworkRequest* request);       /* +0x1A8 */
    virtual void handleCircleMatchOptionSet(NetworkRequest* request); /* +0x1AC */
    virtual void handleCircleMatchStart(NetworkRequest* request); /* +0x1B0 */
    virtual void handleGameSpyError(NetworkRequest* request); /* +0x1B4 */
    virtual void slot_1B8(NetworkRequest* request);       /* +0x1B8 */
    virtual void handleCircleMatchEnd(NetworkRequest* request); /* +0x1BC */
    virtual s8 mapId_1C0(s32 value);                      /* +0x1C0 */
    virtual void slot_1C4(s8 value);                      /* +0x1C4 */

    NetworkRequest pool2_1C4[2];               /* +0x1C4..+0x30B */
    u8 pad_30C[0x54];                          /* +0x30C..+0x35F */
    s32 field_360[21];                         /* +0x360..+0x3B3 */
    u8 pad_3B4[0x04];                          /* +0x3B4..+0x3B7 */
    f32 field_3B8;                             /* +0x3B8 */
    f32 field_3BC;                             /* +0x3BC */
    u8 field_3C0;                              /* +0x3C0 */
    u8 field_3C1;                              /* +0x3C1 */
    u8 field_3C2;                              /* +0x3C2 */
    u8 field_3C3;                              /* +0x3C3 */
    s32 field_3C4;                             /* +0x3C4 */
    u8 field_3C8;                              /* +0x3C8 */
    u8 pad_3C9[0x03];                          /* +0x3C9..+0x3CB */
    NetworkSmallObject field_3CC;              /* +0x3CC */
    u8 pad_3DC[0x10];                          /* +0x3DC..+0x3EB */
    u8 field_3EC[0x30];                        /* +0x3EC..+0x41B */
    u32 circleInfoRequestId_41C;               /* +0x41C - the id `sendReqCircleInfoSet` sends under */
    u8 pad_420[0x118];                         /* +0x420..+0x537 */
    NetworkSessionPlayerRecord players_538[4]; /* +0x538..+0x657 */
    NetworkSingleTcp* tcp_658;                 /* +0x658 - the Tcp connection, pumped by `NetworkSingleTcp::move` */
    NetworkMultipleUdp* udp_65C;               /* +0x65C - the Udp socket, pumped by `NetworkMultipleUdp::move` */
    s32 field_660;                             /* +0x660 */
    u8 pad_664[0x13C];                         /* +0x664..+0x79F */
    NetworkNameList nameList_7A0;              /* +0x7A0..+0x94F - `buildCircleInfoName` reads it */
    u32 circleRecordCount_950;                 /* +0x950 - records waiting to be sent (max 256) */
    u8 circleRecords_954[0x100];               /* +0x954..+0xA53 - the records `move` copies out */
    u8 pad_A54[0x91];                          /* +0xA54..+0xAE4 */
    u8 field_AE5;                              /* +0xAE5 */
    u8 field_AE6;                              /* +0xAE6 - pending-mode flag `move` consumes */
    u8 pad_AE7[0x9];                           /* +0xAE7..+0xAF0 */
    NetworkSessionCircleList circleList_AF0;   /* +0xAF0..+0x6E73 */
    u8 field_6E74;                             /* +0x6E74 */
    u8 field_6E75;                             /* +0x6E75 */
};   /* size: 0x6E76 */

/* the Pat interface singletons the Pat methods build on demand (another band) */
class PatInterface {
public:
    virtual void destroy(u32 flags);   /* +0x08 - the key function, defined in the Pat band */
    PatInterface();
    static PatInterface* getInstance();
};
/* The error record `GameSpyInterfaceThread::getErrorStruct` fills and `NetworkInstance::postError`
 * (declared in `include/unsplit/Network.h`, which only forward-declares this type) takes back.  It is
 * declared here beside that handshake, from the Pat band's `move`: it reads +0x04 as the error
 * **code** (it forwards the record only when it is 0x4B) and copies the three words
 * +0x00/+0x04/+0x08 into its own copy, so only those three are named; the tail is untouched anywhere
 * and is padding.  size: 0x10 (approximate - only +0x00..+0x0B is evidenced). */
struct NetworkErrorInfo {
    /* +0x00 */ u32 value_00;
    /* +0x04 */ u32 code_04;
    /* +0x08 */ u32 extra_08;
    /* +0x0C */ u32 pad_0C;
};

class GameSpyInterfaceThread {
public:
    virtual void destroy(u32 flags);   /* +0x08 - the key function, defined in the Pat band */
    GameSpyInterfaceThread();
    /* the live worker thread (`.sbss` 0x80794CE4); defined in `Network/NetworkSessionManager.cpp` */
    static GameSpyInterfaceThread* getInstance();
    void canClose();
    s32 initialize();
    void armCancel();
    bool requestClose();
    /* The error handshake `move` runs (all three are plain members - the target calls them by their
     * own mangling, not through the table): the result the thread finished with (negative = error),
     * the error record it filled in, and the acknowledgement that clears it. */
    s32 getResult();
    void getErrorStruct(NetworkErrorInfo* info);
    void clearError();
};

/* the Pat accessors keep their plain (unmangled) map names.

   The singleton accessor at 0x803768F0 and `memset` are deliberately **not** declared here: both are
   owned by another registered unit (`getInstance_` sits inside `enemy/em020_ai.cpp`'s range,
   `memset` is `Runtime.PPCEABI.H/memset.c`), and rule 2 puts the declaration in the owner's header -
   which the consumer includes (`enemy/em020_ai.h`, `Runtime.PPCEABI.H/memset.h`). */
extern "C" void PatInterface_clear(void);
extern "C" int PatInterface_isReady(void);
/* untyped: opaque handle passed through - the context's layout belongs to the Pat band */
extern "C" void networkLog_destroyContext(NetworkSessionManagerLogger* log, void* context);
extern "C" void networkPatResetCircleInfo(NetworkSessionManagerPat* self, s32 index);
extern "C" void networkPatAttachBuffer(NetworkBuffer* buffer);
extern "C" void networkPatReleaseBuffer(NetworkSessionManagerPat* self);

/* ---------------- externs ----------------------------------------------------------------- */

extern "C" {

extern NetworkRequestDesc networkRequestDesc364;
extern NetworkRequestDesc networkRequestDesc368;
extern NetworkRequestDesc networkRequestDesc372;
extern NetworkRequestDesc networkRequestDesc376;
extern NetworkRequestDesc networkRequestDesc380;
extern NetworkRequestDesc networkRequestDesc384;
extern NetworkRequestDesc networkRequestDesc388;
extern NetworkRequestDesc networkRequestDesc432;
extern NetworkRequestDesc networkRequestDesc416;
extern NetworkRequestDesc networkRequestDesc420;
extern NetworkRequestDesc networkRequestDesc424;
extern NetworkRequestDesc networkRequestDesc404;
extern NetworkRequestDesc networkRequestDesc436;
extern NetworkRequestDesc networkRequestDesc440;
extern NetworkRequestDesc networkRequestDesc444;
extern NetworkRequestDesc networkRequestDesc408;
extern NetworkRequestDesc networkRequestDesc412;
extern NetworkRequestDesc networkRequestDesc392;
extern NetworkRequestDesc networkRequestDesc396;
extern NetworkRequestDesc networkRequestDesc400;
extern NetworkRequestDesc networkRequestDesc428;

/* bit-stream writer API (another band).  The two writer classes are reconstructions from the frame
   each constructor is given: `NetworkStreamWriter` is the 0x20-byte local every op-code sender
   reserves, `NetworkStreamWriterDefault` the one `NetworkSessionStable::move` reserves. */
void networkStreamWriter_dtor(NetworkStreamWriter* self, s32 flags);
void networkStreamWriter_constructDefault(NetworkStreamWriterDefault* self);
void networkStreamWriterDefault_dtor(NetworkStreamWriterDefault* self, s32 flags);
u32 writeByte(NetworkStreamWriter* self, u32 value);
u32 writeUInt(NetworkStreamWriter* self, u32 value);
u16 writeSize(NetworkStreamWriter* self, u16 size);
s32 writeBytes(NetworkStreamWriter* self, const void* data, u32 len);

/* the writer's remaining entry points the tail of the range drives (the second writer class) */
void networkStreamWriter_attach(NetworkConnectionStable* connection, NetworkStreamWriterDefault* stream);
void networkStreamWriter_reserve(NetworkConnectionStable* connection, const u8* bytes, u32 length, s8 kind);
void networkStreamWriter_putBytes(NetworkStreamWriterDefault* self, const void* data, u32 size);
void networkStreamWriter_flush(NetworkStreamWriterDefault* self);
void networkStreamWriter_setMode(NetworkStreamWriterDefault* self, u32 mode);
void networkStreamWriter_putU16(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU16b(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU32(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_putU32b(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable1(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable2(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable3(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_commit(NetworkStreamWriterDefault* self);
void networkStreamWriter_bytes(NetworkStreamWriterDefault* self);
u32 networkStreamWriter_size(const void* sub);

/* The manager logger accessor `getNetworkLogger` is *not* declared here: no registered unit owns it,
   so rule 2 puts it in the band header `include/unsplit/Network.h` (which types it as the class
   `NetworkLogger` that the logging band uses).  A consumer that wants the older
   `NetworkSessionManagerLogger` view of the same object casts. */

/* the band's float constants and singleton slots live in the shared pool owned by the data-only unit
   `Network/network_shared_data.cpp`; its header (included at the top of this file) declares them, so
   they are not re-declared here (rule 2). */

/* 0x80794CA0 (.sbss) - the request-id source: `requestId_70 = counter; counter = requestId_70 + 1`. */
extern u32 NetworkRequest_idCounter;

/* neighbouring helpers.  Each `untyped:` marker below is the honest case for that declaration: a
   callback adapter whose arguments are forwarded unchanged, or a record whose layout this range
   never reads. */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
void networkSessionReflect0(void* a0, void* a1, s8 a2, void* a3, void* a4, void* a5);
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
void networkSessionReflect1(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5);
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkSmallObject_construct(void* self);

/* untyped: opaque handle passed through - only the writer band owns the layout */
void fn_803CA338(void* self);
/* untyped: opaque handle passed through - only the writer band owns the layout */
void dtor_803CA338(void* self, s32 flags);
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_initMutex(void* self);

/* the two reflection adapters (this unit defines them; `initNetworkSessionStable` takes the first
   one's address as the session vtable's +0x0C callback) */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
void networkSessionReflectCallback(void* a0, void* a1, s8 a2, void* a3, void* a4, void* a5);
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
void networkSessionReflectCallbackEx(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5);

/* this unit's own record/stream helpers, defined in the tail of the range */
s32 NetworkRequest_isTimedOut(NetworkRequest* self);
void NetworkRequest_restartTimer(NetworkRequest* self, f32 interval);
void NetworkRequest_copyRecord(NetworkSessionRecordBlock* dst, const NetworkSessionRecordBlock* src);
NetworkSessionSlotInfo* NetworkSessionSlotInfo_construct(NetworkSessionSlotInfo* self);
NetworkSessionSlotInfo* NetworkSessionSlotInfo_dtor(NetworkSessionSlotInfo* self, s16 flags);
NetworkSessionCircleList* NetworkSessionCircleList_construct(NetworkSessionCircleList* self);
NetworkSessionCircleList* NetworkSessionCircleList_dtor(NetworkSessionCircleList* self, s16 flags);
NetworkSessionCircleInfo* NetworkSessionCircleInfo_construct(NetworkSessionCircleInfo* self);
NetworkSessionCircleInfo* NetworkSessionCircleInfo_dtor(NetworkSessionCircleInfo* self, s16 flags);
NetworkSessionPlayerRecord* NetworkSessionPlayerRecord_construct(NetworkSessionPlayerRecord* self);
NetworkSessionPlayerRecord* NetworkSessionPlayerRecord_dtor(NetworkSessionPlayerRecord* self, s16 flags);
NetworkRequest* NetworkRequestPat_construct(NetworkRequest* self);
NetworkRequest* NetworkRequestPat_dtor(NetworkRequest* self, s16 flags);
void NetworkRequestPat_reset(NetworkRequest* self);
void NetworkRequestPat_clear(NetworkRequest* self);
/* untyped: caller-owned payload - the array constructors take raw element pointers */
void __construct_array(void* ptr, void* ctor, void* dtor, u32 size, u32 count);
/* untyped: caller-owned payload - the array destructors take raw element pointers */
void __destroy_arr(void* ptr, void* dtor, u32 size, u32 count);

}

#endif
