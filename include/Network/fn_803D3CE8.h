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

/* ---------------- bit-stream writer (owned by the network-serialization band) -------------- */

typedef struct NetworkBufferVtable {
    void* rtti_00;
    void* rtti_04;
    u32 (*destroy_08)(void* self, u32 flags);                    /* +0x08 */
    u32 (*signal_0C)(void* self, u32 a, const char* fmt, ...);   /* +0x0C */
    u32 (*begin_10)(void* self);                                 /* +0x10 */
    u32 (*end_14)(void* self);                                   /* +0x14 */
    u8 pad18[0x08];
    u32 (*available_20)(void* self);                                   /* +0x20 */
    u8 pad24[0x14];
    u16 (*put_38)(void* self, u32 a, u32 b, u32 c, u32 d, const void* data, u32 e); /* +0x38 */
    u8 pad3C[0x08];
    void (*flush_44)(void* self);                                  /* +0x44 */
    u8 pad48[0x44];
    f32 (*getFloat_8C)(void* self, s32 idx);                     /* +0x8C */
    s32 (*getInt_90)(void* self, s32 idx);                       /* +0x90 */
} NetworkBufferVtable;

typedef struct NetworkBuffer {
    NetworkBufferVtable* vtable;   /* +0x00 */
    u8 pad04[0x08];
    u8 payload_0C[];               /* +0x0C */
} NetworkBuffer;

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
    virtual void slot_18();                                /* +0x018 */
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
    virtual void broadcastPlayerSlots();                   /* +0x128 */
    virtual void putTerminatorA(u32 a, u32 b, u8 c);       /* +0x12C */
    virtual void putTerminatorB(u32 a, u32 b);             /* +0x130 */
    virtual void putTerminatorC(u32 a, u32 b, u8 c);       /* +0x134 */
    virtual void sendBatch_138(u32 count, const s8* data); /* +0x138 */
    virtual void slot_13C();                               /* +0x13C */
    virtual void slot_140();                               /* +0x140 */
    virtual void slot_144();                               /* +0x144 */
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
    s32 active_1C;              /* +0x1C */
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

extern NetworkRequestDesc lbl_805FA7CC;
extern NetworkRequestDesc lbl_805FA7D8;
extern NetworkRequestDesc lbl_805FA7E4;
extern NetworkRequestDesc lbl_805FA7F0;
extern NetworkRequestDesc lbl_805FA7FC;
extern NetworkRequestDesc lbl_805FA808;
extern NetworkRequestDesc lbl_805FA814;
extern NetworkRequestDesc lbl_805FA820;
extern NetworkRequestDesc lbl_805FA82C;
extern NetworkRequestDesc lbl_805FA838;
extern NetworkRequestDesc lbl_805FA844;
extern NetworkRequestDesc lbl_805FA850;
extern NetworkRequestDesc lbl_805FA85C;
extern NetworkRequestDesc lbl_805FA868;
extern NetworkRequestDesc lbl_805FA874;
extern NetworkRequestDesc lbl_805FA880;
extern NetworkRequestDesc lbl_805FA88C;
extern NetworkRequestDesc lbl_805FA898;
extern NetworkRequestDesc lbl_805FA8A4;
extern NetworkRequestDesc lbl_805FA8B0;
extern NetworkRequestDesc lbl_805FA8BC;

/* bit-stream writer API (another unit) */
void fn_803CB9B4(void* self);
void fn_803CB9F0(void* self);
void dtor_803CB958(void* self, s32 flags);
void dtor_803CB8FC(void* self, s32 flags);
void fn_803F89D0(void* self, const void* buffer, u32 size);
void fn_803F8A14(void* self, s32 mode);
u16 writeByte(void* self, u32 value);
u16 writeUInt(void* self, u32 value);
u16 writeSize(void* self, u16 size);
u16 writeBytes(void* self, const void* data, u32 len);
u16 fn_803F8BDC(void* self, const void* value);

/* the send/flush tail */
void fn_803D39BC(NetworkSessionStable* self, void* stream, u32 a, u32 b, const void* term, u32 c);

/* neighbouring helpers */
void networkSessionReflectCallback(void);   /* 0x803D6870, in this unit's span; the mediator opener's callback */
void fn_803CA338(void* self);
void dtor_803CA338(void* self);
void fn_803CA37C(void* self);
void fn_803CF66C(s32 value);
void fn_803F87B8(void* self);
void __construct_array(void* ptr, void* ctor, void* dtor, u32 size, u32 count);
void __destroy_arr(void* ptr, void* dtor, u32 size, u32 count);
s32 __ptmf_scall(void* self);
void LockMutex(void* mutex);
void UnlockMutex(void* mutex);

}

#endif
