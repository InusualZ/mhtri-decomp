/*
 * include/Network/fn_803D3CE8.h - the externs and layouts the Network session band needs.
 *
 * The types are unit-local (nothing else includes it yet): `NetworkSessionStable`,
 * `NetworkSessionManager` and `NetworkRequest` are reconstructed here from the range's own
 * disassembly (every field offset is the one the target instructions address).  `NetworkSessionManager`
 * is a CLASS with inheritance (rule 10): the base declares the 112-slot vtable the DOL carries at
 * 0x805FA908, so MWCC emits the table and the vptr store instead of the unit writing them by hand; the
 * 61 slots the original leaves 0 are pure virtual (`NetworkSessionManagerPat`'s table fills them).  The
 * bit-stream writer and the neighbouring `fn_` helpers are declared `extern "C"` because their owners
 * are still unsplit; per brief section 6.5 rule 2 those sites are the named unsplit gap.
 */

#ifndef FN_803D3CE8_H
#define FN_803D3CE8_H

#include "types.h"
#include "Network/network_transport.h"

/* ---------------- the bit-stream writer's frame objects (the writer band's classes) ------------- */

/* Only the sizes are evidenced: `NetworkStreamWriter` is the 0x20-byte local every op-code sender in
   this range reserves (retail's `send8` frame is 0x30 with the writer at +0x10), and
   `NetworkStreamWriterDefault` the one `NetworkSessionStable_move` reserves.  Their members belong to
   the writer's own band. */
typedef struct NetworkStreamWriter {
    u8 bytes_00[0x20];   /* +0x00..+0x1F */
} NetworkStreamWriter;   /* size: 0x20 */

typedef struct NetworkStreamWriterDefault {
    u8 bytes_00[0x1C];   /* +0x00..+0x1B */
} NetworkStreamWriterDefault;   /* size: 0x1C */

/* ---------------- bit-stream writer (owned by the network-serialization band) -------------- */

/* The stream buffer every op-code writer puts its packet on.  A polymorphic class (rule 10): its
   table lives at 0x805F9150 in the serialization band, which this unit does not own, so the class
   only *declares* its virtuals - none is defined here, so MWCC emits no table of ours - and the
   slots are the offsets the target's calls address (a declared virtual at index i is at +8+4*i).
   The three never-called groups are the dispatch holes between the called slots: the entry points
   this unit reaches are named from what the call passes, the eight `slot_NN` slots from their offset
   alone (GUESS - the buffer class's own names are unknown). */
class NetworkBuffer {
public:
    /* +0x08 */ virtual u32 destroy(u32 flags);
    /* +0x0C */ virtual u32 signal(u32 a, const char* fmt, ...);
    /* +0x10 */ virtual u32 begin();
    /* +0x14 */ virtual u32 end();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual u32 available();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual void pad_28();
    /* +0x2C */ virtual void pad_2C();
    /* +0x30 (GUESS: offset-derived) */ virtual void slot_30();
    /* +0x34 (GUESS: offset-derived) */ virtual void slot_34();
    /* +0x38 */ virtual u16 put(u32 a, u32 b, u32 c, u32 d, const void* data, u8 e);  /* untyped: byte range (the packet's data bytes) */
    /* +0x3C */ virtual void pad_3C();
    /* +0x40 */ virtual void pad_40();
    /* +0x44 */ virtual void flush();
    /* +0x48 */ virtual void pad_48();
    /* +0x4C */ virtual void pad_4C();
    /* +0x50 (GUESS: offset-derived) */ virtual void slot_50();
    /* +0x54 (GUESS: offset-derived) */ virtual void slot_54();
    /* +0x58 */ virtual void pad_58();
    /* +0x5C (GUESS: offset-derived) */ virtual void slot_5C();
    /* +0x60 (GUESS: offset-derived) */ virtual void slot_60();
    /* +0x64 (GUESS: offset-derived) */ virtual void slot_64();
    /* +0x68 (GUESS: offset-derived) */ virtual void slot_68();
    /* +0x6C */ virtual void pad_6C();
    /* +0x70 */ virtual void pad_70();
    /* +0x74 */ virtual void pad_74();
    /* +0x78 */ virtual void pad_78();
    /* +0x7C */ virtual void pad_7C();
    /* +0x80 */ virtual void pad_80();
    /* +0x84 */ virtual void pad_84();
    /* +0x88 */ virtual void pad_88();
    /* +0x8C */ virtual f32 getFloat(s32 idx);
    /* +0x90 */ virtual s32 getInt(s32 idx);
};   /* size: 0x04 - only ever reached through a pointer in this unit */

/* ---------------- the manager's logger (its accessor is another band's) ------------------- */

typedef struct NetworkManagerLoggerVtable {
    u8 pad00[0x0C];
    void (*verbose_0C)(void* self, u32 level, const char* fmt, ...);   /* +0x0C */
    void (*warn_10)(void* self, const char* fmt, ...);                 /* +0x10 */
    void (*log_14)(void* self, const char* fmt, ...);                  /* +0x14 */
    u8 pad18[0x48];
    f32 (*getTime_60)(void* self);                                     /* +0x60 */
    u8 pad64[0x08];              /* the real vtable is longer; only the called slots are named */
} NetworkManagerLoggerVtable;   /* size: 0x6C (approximation) */

typedef struct NetworkSessionManagerLogger {
    NetworkManagerLoggerVtable* vtable;   /* +0x00 */
} NetworkSessionManagerLogger;   /* size: 0x04 */

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
/* The small object the writer band's `networkSmallObject_construct`/`_dtor` manage.  Its +0x00 word
   is a function-pointer table - `NetworkSessionManagerPat::clear` dispatches its +0x18 slot - so it is
   modelled as a struct with a vtable member, never as a polymorphic class: a class would make MWCC
   initialise the vptr of every element of the four record arrays, which the target does not do. */
typedef struct NetworkSmallObjectVtable {
    void* rtti_00;
    void* rtti_04;
    u8 pad08[0x10];
    void (*slot_18)(void* self);   /* +0x18 */
} NetworkSmallObjectVtable;   /* size: 0x1C (approximation - only +0x18 is called) */

typedef struct NetworkSmallObject {
    NetworkSmallObjectVtable* vtable;   /* +0x00 */
    u8 pad_04[0x0C];                    /* +0x04..+0x0F */
} NetworkSmallObject;   /* size: 0x10 (approximation - only the +0x18 dispatch is evidenced) */

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
    u32 unused_00;              /* +0x00 */
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
    virtual void pure_02C() = 0;                           /* +0x02C */
    virtual void pure_030() = 0;                           /* +0x030 */
    virtual void pure_034() = 0;                           /* +0x034 */
    virtual void pure_038() = 0;                           /* +0x038 */
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
    virtual void pure_068() = 0;                           /* +0x068 */
    virtual void pure_06C() = 0;                           /* +0x06C */
    virtual void pure_070() = 0;                           /* +0x070 */
    virtual void pure_074() = 0;                           /* +0x074 */
    virtual void pure_078() = 0;                           /* +0x078 */
    virtual void pure_07C() = 0;                           /* +0x07C */
    virtual void pure_080() = 0;                           /* +0x080 */
    virtual void pure_084() = 0;                           /* +0x084 */
    virtual void pure_088() = 0;                           /* +0x088 */
    virtual void pure_08C() = 0;                           /* +0x08C */
    virtual void pure_090() = 0;                           /* +0x090 */
    virtual void pure_094() = 0;                           /* +0x094 */
    virtual void pure_098() = 0;                           /* +0x098 */
    virtual void pure_09C() = 0;                           /* +0x09C */
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
    virtual void pure_0D4() = 0;                           /* +0x0D4 */
    virtual void pure_0D8() = 0;                           /* +0x0D8 */
    virtual void pure_0DC() = 0;                           /* +0x0DC */
    virtual void pure_0E0() = 0;                           /* +0x0E0 */
    virtual void pure_0E4() = 0;                           /* +0x0E4 */
    virtual void pure_0E8() = 0;                           /* +0x0E8 */
    virtual void pure_0EC() = 0;                           /* +0x0EC */
    virtual void pure_0F0() = 0;                           /* +0x0F0 */
    virtual void pure_0F4() = 0;                           /* +0x0F4 */
    virtual void pure_0F8() = 0;                           /* +0x0F8 */
    virtual void pure_0FC() = 0;                           /* +0x0FC */
    virtual void pure_100() = 0;                           /* +0x100 */
    virtual void pure_104() = 0;                           /* +0x104 */
    virtual void pure_108() = 0;                           /* +0x108 */
    virtual void pure_10C() = 0;                           /* +0x10C */
    virtual void pure_110() = 0;                           /* +0x110 */
    virtual void pure_114() = 0;                           /* +0x114 */
    virtual void pure_118() = 0;                           /* +0x118 */
    virtual void pure_11C() = 0;                           /* +0x11C */
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
    virtual void pure_16C() = 0;                           /* +0x16C */
    virtual void pure_170() = 0;                           /* +0x170 */
    virtual void pure_174() = 0;                           /* +0x174 */
    virtual void pure_178() = 0;                           /* +0x178 */
    virtual void pure_17C() = 0;                           /* +0x17C */
    virtual void pure_180() = 0;                           /* +0x180 */
    virtual void pure_184() = 0;                           /* +0x184 */
    virtual void pure_188() = 0;                           /* +0x188 */
    virtual void pure_18C() = 0;                           /* +0x18C */
    virtual void pure_190() = 0;                           /* +0x190 */
    virtual void pure_194() = 0;                           /* +0x194 */
    virtual void pure_198() = 0;                           /* +0x198 */
    virtual void pure_19C() = 0;                           /* +0x19C */
    virtual void pure_1A0() = 0;                           /* +0x1A0 */
    virtual void pure_1A4() = 0;                           /* +0x1A4 */
    virtual void pure_1A8() = 0;                           /* +0x1A8 */
    virtual void pure_1AC() = 0;                           /* +0x1AC */
    virtual void pure_1B0() = 0;                           /* +0x1B0 */
    virtual void pure_1B4() = 0;                           /* +0x1B4 */
    virtual void pure_1B8() = 0;                           /* +0x1B8 */
    virtual void pure_1BC() = 0;                           /* +0x1BC */
    virtual s8 mapId_1C0(s32 value) = 0;                   /* +0x1C0 */
    virtual void pure_1C4() = 0;                           /* +0x1C4 */

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

/* The two channel objects the Pat manager holds at +0x658 / +0x65C and pumps once per frame.  Only
 * their *pointers* are ever held here - their layouts belong to the band that builds them, which is
 * still unclaimed - so both are incomplete classes and the manager only forwards them.  Their names
 * are **GUESSED** from the two pumps (`receivePatInterfaces`, `flushPatRequests`), which is all the
 * binary gives: the manager's own fields were anonymous (`field_658`/`field_65C`) before this pass. */
class PatReceiver;      /* +0x658 - `receivePatInterfaces` reads its transport at +0x04 */
class PatRequestQueue;  /* +0x65C - `flushPatRequests` walks its request list */

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

/* The Pat band's helpers.  `receivePatInterfaces` (0x803CE064) and `flushPatRequests` (0x803CE5F0)
 * sit inside `Network/network_transport.cpp`'s claimed range, so rule 2 gives their declarations to
 * that unit's header (`Network/network_transport.h`, included at the top of this file).  The rest have
 * no registered owner, and this header - not `include/unsplit/Network.h` - is where this band's
 * unowned helpers already live (`networkPatAttachBuffer`, `networkPatResetCircleInfo`, `PatInterface_*`),
 * so they are declared beside the records they take.  The first three of the five were the map's
 * `fn_803CE064` / `fn_803CE5F0` / `fn_803DE524` until the Pat pass renamed them from what their bodies
 * do (**GUESSES**, each recorded where it is declared); the last two are the map's own names. */
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

/* The derived class the tail of the range defines.  It is declared here so MWCC emits the
   constructor's vptr store and the destructor itself (rule 10).  `move` is declared **first**: it is
   the class's key function and its body lives in the next band (0x803D70B8, now named after what it
   overrides - `move__24NetworkSessionManagerPatFv` in the map), so no Pat vtable is emitted into this
   object - which is what the target shows (its `.data` is the base table alone).

   THE VTABLE IS STILL NOBODY'S (2026-09-28).  MWCC emits a class's table in the TU that defines its
   key function, so `__vt__24NetworkSessionManagerPat` (0x805FB0F0, 0x1C8 B / 114 slots) belongs to the
   band that opens at 0x803D70B8 - unclaimed, and its `.data` run 0x805FAAD0..0x805FB2B8 (the two
   message strings, the three jump tables and the table itself) is unclaimed with it.  This declaration
   still overrides only the five slots above, while the target's table is filled by 112 functions: 49
   of them in 0x803D70B8..0x803DDB64, 49 inside this unit's range, 14 elsewhere.  No unit can emit a
   matching table until those ~100 overrides are declared - which is why 0x805FB0F0 is left unclaimed
   rather than owned-but-wrong.  The slot - address census is in
   `.pi/notes/network-pat-abstraction.md` section (a) and in the 2026-09-28 `network-pat-class` report. */
class NetworkSessionManagerPat : public NetworkSessionManager {
public:
    virtual void move();                     /* +0x018 - the key function, defined in the next band */
    NetworkSessionManagerPat();
    virtual ~NetworkSessionManagerPat();     /* +0x008 */
    virtual void init(u32 a, u32 b);         /* +0x00C */
    virtual void clear();                    /* +0x010 */
    virtual void release();                  /* +0x014 - the Pat flush ("finalNetwork") */

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
    PatReceiver* receiver_658;                 /* +0x658 - pumped by `receivePatInterfaces` */
    PatRequestQueue* requestQueue_65C;         /* +0x65C - pumped by `flushPatRequests` */
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
    void canClose();
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
extern "C" GameSpyInterfaceThread* GameSpyInterfaceThread_getInstance(void);
/* untyped: opaque handle passed through - the context's layout belongs to the Pat band */
extern "C" void networkLog_destroyContext(NetworkSessionManagerLogger* log, void* context);
extern "C" void networkPatResetCircleInfo(NetworkSessionManagerPat* self, s32 index);
extern "C" void networkPatAttachBuffer(NetworkBuffer* buffer);
extern "C" void networkPatReleaseBuffer(NetworkSessionManagerPat* self);

/* ---------------- NetworkSessionStable ---------------------------------------------------- */

typedef struct NetworkSessionStableVtable {
    void* rtti_00;
    void* rtti_04;
    u8 pad08[0x84];
    f32 (*getFloat_8C)(void* self, s32 idx);            /* +0x8C */
    u8 pad90[0x130];
    s8 (*mapId_1C0)(void* self, s32 value);             /* +0x1C0 */
} NetworkSessionStableVtable;

typedef struct NetworkSessionSlot {
    s8  ownerIndex_00;              /* +0x00 */
    u8  pad01[0x08];
    u8  linked_09;              /* +0x09 */
    u8  pad0A;
    u8  ready_0B;              /* +0x0B */
    u8  pad0C[0x10];
    NetworkBuffer* active_1C;   /* +0x1C - the stream buffer this slot transmits on */
    u32 state_20;              /* +0x20 */
    u8  pad24[0x1C];
    u32 playerId_40;              /* +0x40 */
    u32 bits_44;              /* +0x44 */
    u8  pad48[0x14];
    u32 bits_5C;              /* +0x5C */
    u8  pad60[0x58];
    f32 rate_B8;              /* +0xB8 */
    f32 rate2_BC;              /* +0xBC */
    s32 counter_C0;              /* +0xC0 */
    f32 last_C4;              /* +0xC4 */
    f32 base_C8;              /* +0xC8 */
    s32 limit_CC;              /* +0xCC */
    u8  flag_D0;              /* +0xD0 */
    u8  padD1[0x03];
    f32 accel_D4;              /* +0xD4 */
    f32 limit2_D8;              /* +0xD8 */
    u8  pad_DC[0x848];         /* +0xDC..+0x923 - the element tail; the target's multiply for
                                  a variable index is 0x924, so one array element is that size */
} NetworkSessionSlot;       /* size: 0x924 */

typedef struct NetworkSessionStable {
    NetworkSessionStableVtable* vtable;    /* +0x00 */
    u8 pad04[0x09];
    char sendBuffer_0D[0x400];             /* +0x0D */
    u8 pad40D[0x14419];
    u8 field_14826;                           /* +0x14826 */
    u8 pad14827[0x11];
    NetworkSessionSlot slots_14838[4];     /* +0x14838..+0x16CC7 */
    u8 pad16CC8[0x10];
    u32 tick_16CD8;                          /* +0x16CD8 */
    f32 time_16CDC;                          /* +0x16CDC */
    u8 pad16CE0[0x18];
    f32 limit_16CF8;                          /* +0x16CF8 */
    s32 count_16CFC;                          /* +0x16CFC */
} NetworkSessionStable;

/* ---------------- externs ----------------------------------------------------------------- */

extern "C" {

/* class vtables live in another TU's `.data` - reference, never rebuild (rule 10) */
extern NetworkSessionStableVtable NetworkSessionStable_VTable;

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
   reserves, `NetworkStreamWriterDefault` the one `NetworkSessionStable_move` reserves. */
void fn_803CB9B4(NetworkStreamWriter* self);
void dtor_803CB958(NetworkStreamWriter* self, s32 flags);
void networkStreamWriter_constructDefault(NetworkStreamWriterDefault* self);
void dtor_803CB8FC(NetworkStreamWriterDefault* self, s32 flags);
void fn_803F89D0(NetworkStreamWriter* self, const void* buffer, u32 size);
void fn_803F8A14(NetworkStreamWriter* self, s32 mode);
u16 writeByte(NetworkStreamWriter* self, u32 value);
u16 writeUInt(NetworkStreamWriter* self, u32 value);
u16 writeSize(NetworkStreamWriter* self, u16 size);
u16 writeBytes(NetworkStreamWriter* self, const void* data, u32 len);
u16 fn_803F8BDC(NetworkStreamWriter* self, const void* value);

/* the writer's remaining entry points the tail of the range drives (the second writer class) */
void networkStreamWriter_attach(NetworkBuffer* buffer, NetworkStreamWriterDefault* stream);
void networkStreamWriter_reserve(NetworkBuffer* buffer, u32 a, u32 b, u32 c);
void networkStreamWriter_putBytes(NetworkStreamWriterDefault* self, const void* data, u32 size);
void networkStreamWriter_flush(NetworkStreamWriterDefault* self);
void networkStreamWriter_setMode(NetworkStreamWriterDefault* self, u32 mode);
void networkStreamWriter_putU16(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_putU16b(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_putU32(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_putU32b(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable1(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable2(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable3(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_commit(NetworkStreamWriterDefault* self);
void networkStreamWriter_bytes(NetworkStreamWriterDefault* self);
u32 networkStreamWriter_size(const void* sub);

/* the send/flush tail (`NetworkSessionStable_sendStream`) is declared by its owner,
   `Network/network_transport.h`, which this header includes at the top. */

/* The manager logger accessor `getNetworkLogger` is *not* declared here: no registered unit owns it,
   so rule 2 puts it in the band header `include/unsplit/Network.h` (which types it as the class
   `NetworkLogger` that the logging band uses).  A consumer that wants the older
   `NetworkSessionManagerLogger` view of the same object casts. */

/* the band's float constants and singleton slots (unowned addresses - playbook 29: declared, never
   defined).  Each value is read off the DOL; the name is derived from the use the range makes of it. */
extern f32 networkMillisecondsPerSecond;   /* 0x8079C6EC = 1000.0f */
extern f32 networkRateScale;               /* 0x8079C6F0 = 2.0f */
extern f32 networkRateMax;                 /* 0x8079C6F8 = 1.0f */
extern f32 networkRateMin;                 /* 0x8079C708 = 0.1f */
extern f32 networkRateUpStep;              /* 0x8079C718 = 0.017f */
extern f32 networkRateUpLerp;              /* 0x8079C730 = 0.5f */
extern f32 networkRateDownStep;            /* 0x8079C734 = 0.008f */
extern f32 networkRateDecay;               /* 0x8079C738 = 0.002f */
extern f32 networkRateDownLerp;            /* 0x8079C73C = 0.25f */
extern f32 networkRateFloor;               /* 0x8079392C = 0.032f (.sdata) */
extern f32 networkRequestZero;             /* 0x8079C740 = 0.0f */
extern f32 networkRequestTimerIdle;        /* 0x8079C748 = 0.0f */
extern f32 networkRequestTimerReset;       /* 0x8079C750 = 0.0f */
extern f32 networkSessionPatTimeOrigin;    /* 0x8079C754 = -3600.0f */
extern void* sGameSpyInterfaceThread;      /* 0x80794CE4 (.sbss) */
extern const char NetworkSessionStable_downPerformancePackMessage[];
extern const char NetworkSessionStable_downPerformanceByteMessage[];
extern const char NetworkSessionStable_upPerformancePackMessage[];
extern const char NetworkSessionStable_upPerformanceByteMessage[];
extern const char NetworkSessionStable_moveOutOfBandMessage[];
extern const char NetworkSessionManager_moveStandByMessage[];
extern const char NetworkSessionManagerPat_finalMessage[];

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

/* untyped: opaque handle passed through - only the writer band owns the layout */
s32 __ptmf_scall(void* self);

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

s32 __ptmf_scall(void* self);

}

#endif
