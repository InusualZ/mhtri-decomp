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

/* The record classes the Pat layer's arrays and stack frames hold.  Each size is evidenced (the
   `__construct_array` / `__destroy_arr` element sizes, and the field offsets the range addresses).  Each embeds a
   `NetworkUniqueId`, so each has a constructor and a destructor of its own (this unit defines them out of line, in
   the target's order); the containers' array construction and the stack objects' cleanup are MWCC's. */
/* The chat message record the Pat layer reports (event 18 for this console's own message, 19 for a received
   one): the sender's address, name and text, the caller's tag word and the server time it was stamped with.
   The name says "slot info" for historical reasons (the map's `NetworkSessionSlotInfo_*` rows); the layout is
   the one `slot_19C` and `receiveSessionChat` fill on their stack. */
class NetworkSessionSlotInfo {
public:
    NetworkSessionSlotInfo();
    ~NetworkSessionSlotInfo();

    NetworkUniqueId smallObject_00;      /* +0x00 - the sender's address object */
    char name_20[0x13];                  /* +0x20 - the sender's name (19 characters) */
    u8 nameEnd_33;                       /* +0x33 - its terminator */
    u8 flag_34;                          /* +0x34 - cleared by both fillers */
    char text_35[0x200];                 /* +0x35 - the message (at most 0x1FF characters) */
    u8 textEnd_235;                      /* +0x235 */
    u8 pad_236[0x02];                    /* +0x236..+0x237 */
    u32 tag_238;                         /* +0x238 - the sender's tag word */
    s32 time_23C;                        /* +0x23C - `getServerDateTime` when the message was reported */
};   /* size: 0x240 */

/* One circle (lobby room) entry.  The name, the record block and the four counters are the fields the
   Pat getters read (`getCircleItemName` copies at most 0x100 name bytes, `getCircleItemRecord` 0x48 record
   bytes); the counters pair up as limit/used (`getCircleItemSize_170_178` returns +0x170 - +0x178) - the
   limit/used reading is a GUESS from that subtraction. */
/* One option slot of a circle: `setCircleInfo` enables slot `n - 1` for an option list entry numbered `n`. */
typedef struct NetworkCircleOptionSlot {
    u32 enabled_00;                       /* +0x00 */
    s32 value_04;                         /* +0x04 */
} NetworkCircleOptionSlot;   /* size: 0x08 */

/* The 0x48-byte option block `getCircleItemRecord` copies out: the highest enabled slot + 1 and eight slots. */
typedef struct NetworkCircleOptions {
    u32 unused_00;                        /* +0x00 */
    u32 count_04;                         /* +0x04 */
    NetworkCircleOptionSlot slots_08[8];  /* +0x08..+0x47 */
} NetworkCircleOptions;   /* size: 0x48 */

class NetworkSessionCircleInfo {
public:
    NetworkSessionCircleInfo();
    ~NetworkSessionCircleInfo();

    s32 id_000;                           /* +0x000 - the circle id (0 = free entry) */
    s32 ownerId_004;                      /* +0x004 */
    char name_008[0x100];                 /* +0x008 - `setCircleInfo` stores at most 63 characters */
    NetworkUniqueId smallObject_108;      /* +0x108 - the circle's address object `exportCircleItem` copies out */
    NetworkCircleOptions options_128;     /* +0x128 */
    s32 limitA_170;                       /* +0x170 */
    s32 limitB_174;                       /* +0x174 */
    s32 usedA_178;                        /* +0x178 */
    s32 usedB_17C;                        /* +0x17C */
    u8 flag_180;                          /* +0x180 */
    u8 pad_181[0x03];                     /* +0x181..+0x183 */
    u32 recordCount_184;                  /* +0x184 - at most 0x100 */
    u8 records_188[0x100];                /* +0x188 */
    char comment_288[0x91];               /* +0x288 - 144 characters + terminator */
    u8 active_319;                        /* +0x319 - the info's state was neither 0 nor -1 (GUESS on the name) */
    u8 pad_31A[0x02];                     /* +0x31A..+0x31B */
};   /* size: 0x31C */

class NetworkSessionCircleList {
public:
    NetworkSessionCircleList();
    ~NetworkSessionCircleList();

    s32 count_00;                            /* +0x00 - `getCircleInfoCount` */
    NetworkSessionCircleInfo items_04[32];   /* +0x04..+0x6383 */
};   /* size: 0x6384 */

class NetworkSessionPlayerRecord {
public:
    NetworkSessionPlayerRecord();
    ~NetworkSessionPlayerRecord();

    u8 active_00;                        /* +0x00 - tested before every read of the record */
    u8 announced_01;                     /* +0x01 - the join event was posted (the leave event clears it) */
    u8 flag_02;                          /* +0x02 */
    u8 state_03;                         /* +0x03 - `updatePlayerRecord` posts event 27 when it changes */
    u8 linked_04;                        /* +0x04 - cleared on removal while a session exists (GUESS on the name) */
    u8 pad_05[0x03];                     /* +0x05..+0x07 */
    NetworkUniqueId smallObject_08;      /* +0x08 - the player's address object `getPlayerRecord` copies out */
    char name_28[0x14];                  /* +0x28 - `getPlayerRecordName` copies at most 0x14 bytes */
    s32 value_3C;                        /* +0x3C - the payload of event 21 (`announcePlayers`) */
    NetworkPeerAddress address_40;       /* +0x40 - the Udp peer `removePlayerRecord` drops */
    u8 pad_46[0x02];                     /* +0x46..+0x47 */
};   /* size: 0x48 */

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
class NetworkSessionManager;
struct NetworkRequest;
struct NetworkErrorInfo;   /* the 12-byte error record - Network/gamespy_interface_types.h */

/* A request's handler: a member function of the manager that reports completion (non-zero).  The starters
   pass it by value (the `networkRequestDescNNN` constants are `{0, handler slot, 0}`), the request keeps
   it at +0x98, `NetworkRequest::run` is MWCC's member-function-pointer call and the reset assigns the
   null member pointer (a 12-byte copy of `__ptmf_null`). */
typedef s32 (NetworkSessionManager::*NetworkRequestDesc)(NetworkRequest* request);   /* size: 0xC */

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
    NetworkSessionManager* owner_94;   /* +0x94 - set while the request runs */
    NetworkRequestDesc handler_98;     /* +0x98..+0xA3 - the member function the request runs */

    void run();   /* runs the handler; a handler that reports completion clears the record */
    s32 getRecord(NetworkErrorInfo* out);    /* the error record, under the mutex; false while none is set */
    void setRecord(u32 a, u32 b, u32 c);     /* stores the error record, under the mutex */
    s32 getArgument(u32 idx);                /* the starter's word argument `idx` (0 past the count) */
    s32 isTimedOut();                        /* waiting longer than `interval_4C` (never while idle) */
    void restartTimer(f32 interval);         /* the clock becomes the baseline, the interval is replaced */
} NetworkRequest;           /* size: 0xA4 */

/* The Pat layer's own request record: the same 0xA4-byte record with a constructor and a destructor of its own
   (the pair the Pat constructor's `__construct_array` passes, distinct from the base pool's), so the Pat manager's
   member construction runs it in member order (GUESS on the name: derived from its owner). */
class NetworkRequestPat {
public:
    NetworkRequestPat();
    ~NetworkRequestPat();

    /* +0x00 */ NetworkRequest request_00;
};   /* size: 0xA4 */

/* One entry of the name list: `copyNameList` copies an entry only when its first word is set. */
typedef struct NetworkNameEntry {
    /* +0x00 */ u32 enabled_00;   /* 1 = the entry carries a value (`packCircleOptions` tests == 1) */
    /* +0x04 */ u32 value_04;
} NetworkNameEntry;   /* size: 0x08 */

/* The name/entry list `buildCircleInfoName` packs: a count at +0x04 and at most eight entries after it.
 * `copyNameList` caps the count at 8 and copies the whole list as 0x48 bytes when its own is empty. */
typedef struct NetworkNameList {
    /* +0x000 */ u32 head_00;
    /* +0x004 */ u32 count_04;
    /* +0x008 */ NetworkNameEntry entries_08[8];
} NetworkNameList;   /* size: 0x48 */

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
    virtual void copyNameList(const NetworkNameList* src) = 0; /* +0x02C */
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
    virtual s32 getCircleInfoCount() = 0;                 /* +0x070 */
    virtual void getCircleItemName(char* dst, s32 size, s32 idx) = 0; /* +0x074 */
    virtual void exportCircleItem(NetworkUniqueId* dst, s32 idx) = 0; /* +0x078 */
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
    virtual s32 getWord_524() = 0;                        /* +0x0DC */
    virtual u32 getWord_530() = 0;                        /* +0x0E0 */
    virtual u32 getWord_52C() = 0;                        /* +0x0E4 */
    virtual u32 getSize_528_530() = 0;                    /* +0x0E8 */
    virtual u32 getSize_524_52C() = 0;                    /* +0x0EC */
    virtual void getPlayerRecordName(s8 idx, char* dst, s32 size) = 0; /* +0x0F0 */
    virtual void clearStringWithId(u32 id, char* dst, s32 size) = 0; /* +0x0F4 */
    virtual s32 getPlayerRecord(s8 idx, NetworkUniqueId* dst) = 0; /* +0x0F8 */
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
    virtual s32 updateSession(NetworkRequest* request) = 0; /* +0x16C */
    virtual s32 shutdown(NetworkRequest* request) = 0;   /* +0x170 */
    virtual s32 handleCircleCreate(NetworkRequest* request) = 0; /* +0x174 */
    virtual s32 slot_178(NetworkRequest* request) = 0;   /* +0x178 */
    virtual s32 handleCircleListLayer(NetworkRequest* request) = 0; /* +0x17C */
    virtual s32 handleCircleJoin(NetworkRequest* request) = 0; /* +0x180 */
    virtual s32 slot_184(NetworkRequest* request) = 0;   /* +0x184 */
    virtual s32 slot_188(NetworkRequest* request) = 0;   /* +0x188 */
    virtual s32 handleServerTimeout(NetworkRequest* request) = 0; /* +0x18C */
    virtual s32 slot_190(NetworkRequest* request) = 0;   /* +0x190 */
    virtual s32 handleCircleInfoSet(NetworkRequest* request) = 0; /* +0x194 */
    virtual s32 handleCircleLeave(NetworkRequest* request) = 0; /* +0x198 */
    virtual s32 slot_19C(NetworkRequest* request) = 0;   /* +0x19C */
    virtual s32 slot_1A0(NetworkRequest* request) = 0;   /* +0x1A0 */
    virtual s32 slot_1A4(NetworkRequest* request) = 0;   /* +0x1A4 */
    virtual s32 slot_1A8(NetworkRequest* request) = 0;   /* +0x1A8 */
    virtual s32 handleCircleMatchOptionSet(NetworkRequest* request) = 0; /* +0x1AC */
    virtual s32 handleCircleMatchStart(NetworkRequest* request) = 0; /* +0x1B0 */
    virtual s32 moveStartSession(NetworkRequest* request) = 0; /* +0x1B4 */
    virtual s32 slot_1B8(NetworkRequest* request) = 0;   /* +0x1B8 */
    virtual s32 handleCircleMatchEnd(NetworkRequest* request) = 0; /* +0x1BC */
    virtual s8 mapId_1C0(s32 value) = 0;                   /* +0x1C0 */
    virtual s32 slot_1C4(s8 value) = 0;                   /* +0x1C4 */

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

/* The circle-info block: `move` fills and `sendReqCircleInfoSet` sends one, the circle-list handlers
 * receive one per circle (`setCircleInfo` copies it into the manager's circle entry).  `move` zeroes 892
 * bytes of it, which is the size below. */
typedef struct PatCircleInfo {
    /* +0x000 */ s32 id_000;
    /* +0x004 */ char name_004[0x40];
    /* +0x044 */ s8 flag_044;          /* 1 = the block carries a session name */
    /* +0x045 */ char sessionName_045[0x0F];
    /* +0x054 */ u8 sessionNameEnd_054;
    /* +0x055 */ u8 pad_055;
    /* +0x056 */ u8 records_56[0x100];
    /* +0x156 */ u16 recordCount_156;
    /* +0x158 */ char comment_158[0x90];
    /* +0x1E8 */ u8 commentEnd_1E8;
    /* +0x1E9 */ u8 pad_1E9[0x16F];
    /* +0x358 */ s32 limitA_358;
    /* +0x35C */ s32 usedA_35C;
    /* +0x360 */ s32 limitB_360;
    /* +0x364 */ s32 usedB_364;
    /* +0x368 */ s32 ownerId_368;
    /* +0x36C */ s32 slotNumber_36C;   /* the circle's list index + 1 (0 = remove it) */
    /* +0x370 */ u8 address_370[0x08];
    /* +0x378 */ s8 mode_378;
    /* +0x379 */ s8 state_379;
    /* +0x37A */ s8 state_37A;   /* `handleCircleInfoSet` writes the same open (1) / closed (-1) value as +0x379 */
    /* +0x37B */ u8 pad_37B;
} PatCircleInfo;   /* size: 0x37C */

/* The 0x30-byte match option block `sendReqCircleMatchOptionSet` sends (the manager keeps its own at +0x3EC;
 * `handleCircleMatchOptionSet` sends a zeroed one carrying only the mode byte). */
typedef struct PatMatchOptions {
    /* +0x00 */ u8 pad_00[0x04];
    /* +0x04 */ u16 sessionKey_04;     /* 10000..19999, drawn at login (`updateSession`) */
    /* +0x06 */ s8 mode_06;            /* 2 = the match is reported to the server when it ends */
    /* +0x07 */ u8 pad_07;
    /* +0x08 */ u8 userId_08[0x08];    /* this console's selected id (`getSelectedID`) */
    /* +0x10 */ char name_10[0x13];    /* this console's name (`slot_19C` copies 19 characters into its chat record) */
    /* +0x23 */ u8 pad_23[0x0D];
} PatMatchOptions;   /* size: 0x30 */

/* One search condition of the list at +0x7E8: an option slot, its comparison kind (1..6), whether it is set
   and the value (`packCircleConditions` maps the kind to the server's operator). */
typedef struct PatCondition {
    /* +0x00 */ u8 slot_00;
    /* +0x01 */ u8 kind_01;
    /* +0x02 */ u8 pad_02[0x02];
    /* +0x04 */ u32 enabled_04;     /* 1 = the condition is set */
    /* +0x08 */ s32 value_08;
} PatCondition;   /* size: 0xC */

/* The condition list: a count at +0x04 (clamped to 8) and the eight conditions. */
typedef struct PatConditionList {
    /* +0x00 */ u32 head_00;
    /* +0x04 */ u32 count_04;
    /* +0x08 */ PatCondition entries_08[8];
} PatConditionList;   /* size: 0x68 */

/* One packed search filter `packCircleConditions` builds from a condition: the server's operator (5, 6, 4,
   3, 2, 1 for the condition kinds 1..6), the option number (slot + 1), the enable flag and the value. */
typedef struct PatCircleFilter {
    /* +0x00 */ u8 op_00;
    /* +0x01 */ u8 pad_01[0x03];
    /* +0x04 */ u8 slot_04;
    /* +0x05 */ u8 enabled_05;
    /* +0x06 */ u8 pad_06[0x02];
    /* +0x08 */ s32 value_08;
} PatCircleFilter;   /* size: 0xC */

/* The 10-byte header every session chat packet opens with (`sendSessionChat` writes it, `readChatHeader`
   reads it back): the packet kind (2 = chat), the target slot (-1 = everyone), the sender's slot and the
   caller's tag word. */
typedef struct PatChatHeader {
    /* +0x00 */ u8 kind_00;
    /* +0x01 */ s8 target_01;
    /* +0x02 */ u8 pad_02[0x02];
    /* +0x04 */ u32 sender_04;
    /* +0x08 */ u32 tag_08;
} PatChatHeader;   /* size: 0xC */

/* One entry of a packed option list: an option number, its enable flag and its value. */
typedef struct PatCircleOption {
    /* +0x00 */ u8 slot_00;
    /* +0x01 */ bool enabled_01;
    /* +0x02 */ u8 pad_02[0x2];
    /* +0x04 */ s32 value_04;
} PatCircleOption;   /* size: 0x08 */

/* The packed option list `buildCircleInfoName` fills (at most 32 entries) and `setCircleInfo` reads. */
typedef struct PatCircleOptionList {
    /* +0x00 */ u8 count_00;
    /* +0x01 */ u8 pad_01[0x3];
    /* +0x04 */ PatCircleOption entries_04[32];
} PatCircleOptionList;   /* size: 0x104 */

/* The Pat band's helpers: the members the manager pumps (`NetworkSingleTcp::move`, `NetworkMultipleUdp::move`)
 * are declared with their classes in `Network/network_transport_types.h`.  The free helpers that take
 * the records above are owned by registered units and declared in their owners' headers (rule 2):
 * `buildCircleInfoName`, `circleAvailable` and the `networkPat*` buffer helpers in
 * `Network/NetworkSessionManagerPat.h`, `sendReqCircleInfoSet` in `Network/network_layer_io.h`. */
class NetworkSessionManagerPat;

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
   (`NetworkRequest*` for the 21 handlers, which all call `NetworkRequest::getArgument`/
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
    virtual void copyNameList(const NetworkNameList* src); /* +0x02C */
    virtual void copyNameListTail(const u8* src);         /* +0x030 */
    virtual void setSessionName(const char* name);        /* +0x034 */
    virtual void setCircleRecords(const u8* src, u32 count); /* +0x038 */
    virtual void setFlag79(s8 value);                     /* +0x03C */
    virtual void slot_068();                              /* +0x068 */
    virtual void slot_06C();                              /* +0x06C */
    virtual s32 getCircleInfoCount();                     /* +0x070 */
    virtual void getCircleItemName(char* dst, s32 size, s32 idx); /* +0x074 */
    virtual void exportCircleItem(NetworkUniqueId* dst, s32 idx); /* +0x078 */
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
    virtual s32 getWord_524();                            /* +0x0DC */
    virtual u32 getWord_530();                            /* +0x0E0 */
    virtual u32 getWord_52C();                            /* +0x0E4 */
    virtual u32 getSize_528_530();                        /* +0x0E8 */
    virtual u32 getSize_524_52C();                        /* +0x0EC */
    virtual void getPlayerRecordName(s8 idx, char* dst, s32 size); /* +0x0F0 */
    virtual void clearStringWithId(u32 id, char* dst, s32 size); /* +0x0F4 */
    virtual s32 getPlayerRecord(s8 idx, NetworkUniqueId* dst); /* +0x0F8 */
    virtual u32 slot_0FC();                               /* +0x0FC */
    virtual u8 getByte_534();                             /* +0x100 */
    virtual f32 getTimeSincePublish();                    /* +0x104 */
    virtual void slot_108();                              /* +0x108 */
    virtual u32 slot_10C();                               /* +0x10C */
    virtual u32 slot_110();                               /* +0x110 */
    virtual u32 slot_114();                               /* +0x114 */
    virtual u32 slot_118();                               /* +0x118 */
    virtual u32 slot_11C();                               /* +0x11C */
    virtual s32 updateSession(NetworkRequest* request);  /* +0x16C */
    virtual s32 shutdown(NetworkRequest* request);       /* +0x170 */
    virtual s32 handleCircleCreate(NetworkRequest* request); /* +0x174 */
    virtual s32 slot_178(NetworkRequest* request);       /* +0x178 */
    virtual s32 handleCircleListLayer(NetworkRequest* request); /* +0x17C */
    virtual s32 handleCircleJoin(NetworkRequest* request); /* +0x180 */
    virtual s32 slot_184(NetworkRequest* request);       /* +0x184 */
    virtual s32 slot_188(NetworkRequest* request);       /* +0x188 */
    virtual s32 handleServerTimeout(NetworkRequest* request); /* +0x18C */
    virtual s32 slot_190(NetworkRequest* request);       /* +0x190 */
    virtual s32 handleCircleInfoSet(NetworkRequest* request); /* +0x194 */
    virtual s32 handleCircleLeave(NetworkRequest* request); /* +0x198 - leaves the circle (event 16) */
    virtual s32 slot_19C(NetworkRequest* request);       /* +0x19C */
    virtual s32 slot_1A0(NetworkRequest* request);       /* +0x1A0 */
    virtual s32 slot_1A4(NetworkRequest* request);       /* +0x1A4 */
    virtual s32 slot_1A8(NetworkRequest* request);       /* +0x1A8 */
    virtual s32 handleCircleMatchOptionSet(NetworkRequest* request); /* +0x1AC */
    virtual s32 handleCircleMatchStart(NetworkRequest* request); /* +0x1B0 */
    virtual s32 moveStartSession(NetworkRequest* request); /* +0x1B4 - its own log string "moveStartSession::mMatchPhase(0) NG" */
    virtual s32 slot_1B8(NetworkRequest* request);       /* +0x1B8 */
    virtual s32 handleCircleMatchEnd(NetworkRequest* request); /* +0x1BC */
    virtual s8 mapId_1C0(s32 value);                      /* +0x1C0 */
    virtual s32 slot_1C4(s8 value);                       /* +0x1C4 */

    /* non-virtual members (called by the session-close band and the reflection handlers) */
    /* untyped: caller-owned payload - an error record, an index or a state byte, by event */
    void postEvent(s32 code, s8 slot, s32 value, s32 kind, const void* payload, u32 context); /* 0x803DEE6C (GUESS: forwards an event to the +0x04 callback) */
    void addCircleInfo(s32 index, const PatCircleInfo* info, PatCircleOptionList* options);  /* 0x803DDB10 (GUESS) */
    void removeCircleInfo(s32 index);                     /* 0x803DDB64 (GUESS: resets the entry, shrinks the count) */
    void setCircleInfo(s32 index, const PatCircleInfo* info, PatCircleOptionList* options);  /* 0x803DDBF0 (GUESS) */
    void resetPlayerRecord(s8 index);                     /* 0x803DDFDC (GUESS) */
    void addPlayerRecord(s8 index, const u8* address, const char* name, u32 state, s32 counted, s32 notify); /* 0x803DE070 (GUESS) */
    void removePlayerRecord(s8 index, s32 counted);       /* 0x803DE150 (GUESS) */
    void updatePlayerRecord(s8 index, const u8* address, const char* name, u32 state); /* 0x803DE238 (GUESS) */
    s32 packCircleOptions(PatCircleOption* dst, s32 max, NetworkNameList* src);      /* 0x803DE360 (GUESS) */
    s32 packCircleConditions(PatCircleFilter* dst, s32 max, PatConditionList* src); /* 0x803DE3F4 (GUESS: the search filters of the circle list query) */
    s32 uniqueIdToMember(const NetworkUniqueId* id);   /* 0x803DE6D4 - its own log string names it */
    void resetCircleState();                              /* 0x803DE7C8 (GUESS: clears the pending circle publish) */
    void announcePlayers();                               /* 0x803DE82C (GUESS: re-posts every remote player's events) */
    s32 joinSession();                                    /* 0x803DEC34 (GUESS: marks the session joined, sets its timeouts) */
    void resetSessionSlot(s8 index);                      /* 0x803DECCC (GUESS: the session's +0x1C `resetSlot`) */
    void setHostConnectionIndex(s8 index);                /* 0x803DED58 - its own log string names it */
    void setCircleComment(const char* comment);           /* 0x803DF0C8 (GUESS: the 144-character text `move` publishes beside the mode) */
    void setCircleMode(u8 mode);                          /* 0x803DF144 (GUESS: arms the pending mode byte `move` consumes) */
    void post(const u8* data, s32 size, s8 channel, s8 index); /* 0x803DF180 - forwards to the session's `post` */
    void beginCircleLeave();                                    /* 0x803DA294 (GUESS: drops the pending publish, rewinds the leave steps) */
    s32 stepCircleLeave(NetworkErrorInfo* error);               /* 0x803DA2C8 (GUESS: one step of leaving the circle) */
    void readChatHeader(PatChatHeader* out, const u8* data);    /* 0x803DC0E4 (GUESS: parses the 10-byte chat header) */
    void sendSessionChat(const char* text, u32 tag, s8 target); /* 0x803DC184 (GUESS: the chat packet over the session) */
    void receiveSessionChat(const u8* data, u32 size);          /* 0x803DC2E4 (GUESS: reports a received chat packet, event 19) */
    s8 connectPeer(const NetworkUniqueId* address, u32 value); /* 0x803DEB38 (GUESS: opens a session slot for a layer member) */
    void reportSessionError(const NetworkErrorInfo* info);      /* 0x803DEDE0 (GUESS: sends the record and the thread's error to the server) */
    void setSessionLog(NetworkRequest* request, u32 code, u32 arg_a, u32 arg_b); /* 0x803DF1CC (GUESS: the layer's `setCollectionLog`) */
    void setSessionLogAborted(NetworkRequest* request);     /* 0x803DF24C (GUESS: requestFlags bit 1, code 0x80050012) */
    void setSessionLogSessionLost(NetworkRequest* request); /* 0x803DF29C (GUESS: requestFlags bit 0, code 0x80050031) */

    NetworkRequestPat pool2_1C4[2];            /* +0x1C4..+0x30B */
    u32 requestFlags_30C[21];                  /* +0x30C..+0x35F - per request: bit 0 = session lost, bit 1 = cancelled, higher bits = the replies */
    s32 requestIds_360[21];                    /* +0x360..+0x3B3 - per request: the id the last `sendReq*` returned (-1 = none) */
    u8 leaveState_3B4;                         /* +0x3B4 - `stepCircleLeave`'s step (0, 5, 10) */
    u8 pad_3B5[0x03];                          /* +0x3B5..+0x3B7 */
    f32 field_3B8;                             /* +0x3B8 */
    f32 field_3BC;                             /* +0x3BC */
    u8 connected_3C0;                          /* +0x3C0 - zero: every request fails with 0x80050001; `shutdown` clears it */
    u8 matchRunning_3C1;                       /* +0x3C1 - a match is running (`handleCircleMatchEnd` fails with 0x80050045 without one) */
    u8 field_3C2;                              /* +0x3C2 */
    u8 field_3C3;                              /* +0x3C3 */
    s32 field_3C4;                             /* +0x3C4 */
    u8 field_3C8;                              /* +0x3C8 */
    u8 pad_3C9[0x03];                          /* +0x3C9..+0x3CB */
    NetworkUniqueId field_3CC;                 /* +0x3CC */
    PatMatchOptions matchOptions_3EC;          /* +0x3EC..+0x41B - the options `sendReqCircleMatchOptionSet` sends */
    s32 circleInfoRequestId_41C;               /* +0x41C - the id `sendReqCircleInfoSet` sends under (`canSend_28`: > 0 once assigned) */
    s32 circleOwnerId_420;                     /* +0x420 - the joined circle's owner id (from the received block's +0x368) */
    char circleName_424[0x100];                /* +0x424..+0x523 - the joined circle's name (63 characters + terminator) */
    s32 limitB_524;                            /* +0x524 - the session's own counters, read by `getWord_5xx` */
    s32 limitA_528;                            /* +0x528 - players added (`addPlayerRecord` counts every one) */
    s32 usedB_52C;                             /* +0x52C */
    s32 usedA_530;                             /* +0x530 - players added with the `counted` flag */
    u8 flag_534;                               /* +0x534 - `getByte_534` */
    u8 circleLost_535;                         /* +0x535 - the circle went away: leaving it needs no server request (GUESS on the name) */
    s8 selfIndex_536;                          /* +0x536 - this console's slot (GUESS: `circleAvailable` compares it with +0x537) */
    s8 hostIndex_537;                          /* +0x537 - the host's slot (GUESS) */
    NetworkSessionPlayerRecord players_538[4]; /* +0x538..+0x657 */
    NetworkSingleTcp* tcp_658;                 /* +0x658 - the Tcp connection, pumped by `NetworkSingleTcp::move` */
    NetworkMultipleUdp* udp_65C;               /* +0x65C - the Udp socket, pumped by `NetworkMultipleUdp::move` */
    NetworkResolverBase* resolver_660;         /* +0x660 - the resolver the library lent the session (released on shutdown) */
    s32 matchMemberCount_664;                  /* +0x664 - the payload count of the match-start event (24) */
    u8 matchMembers_668[4];                    /* +0x668 - the match members' slots (the event's payload) */
    u8 matchPhase_66C;                         /* +0x66C - `mMatchPhase` in `moveStartSession`'s log string: 0 = none, 3 = the session started */
    u8 pad_66D;                                /* +0x66D */
    u8 matchData_66E[0x126];                   /* +0x66E..+0x793 - cleared with the match (GUESS: the rest of the match record) */
    s32 errorCode_794;                         /* +0x794 - the error record the GameSpy handler reports (3 words) */
    s32 errorParam1_798;                       /* +0x798 */
    s32 errorParam2_79C;                       /* +0x79C */
    NetworkNameList nameList_7A0;              /* +0x7A0..+0x7E7 - `buildCircleInfoName` reads it */
    PatConditionList conditionList_7E8;        /* +0x7E8..+0x84F - `copyNameListTail` copies the 0x68 bytes in */
    char sessionName_850[0x100];               /* +0x850..+0x94F - `setSessionName` (255 characters + terminator) */
    u32 circleRecordCount_950;                 /* +0x950 - records waiting to be sent (max 256) */
    u8 circleRecords_954[0x100];               /* +0x954..+0xA53 - the records `move` copies out */
    char circleComment_A54[0x91];              /* +0xA54..+0xAE4 - `setCircleComment` (144 characters + terminator) */
    u8 field_AE5;                              /* +0xAE5 - `setCircleMode`'s value: `move` sends mode 1 when set, 2 otherwise */
    u8 field_AE6;                              /* +0xAE6 - pending-mode flag `move` consumes */
    u8 pad_AE7;                                /* +0xAE7 */
    s32 listCursor_AE8;                        /* +0xAE8 - circles fetched so far by the list query (`slot_178`) */
    s32 listTotal_AEC;                         /* +0xAEC - circles the list query found */
    NetworkSessionCircleList circleList_AF0;   /* +0xAF0..+0x6E73 */
    u8 field_6E74;                             /* +0x6E74 */
    u8 field_6E75;                             /* +0x6E75 */
};   /* size: 0x6E76 */

/* The mediator records (`PatInterface`, `GameSpyInterfaceThread`, `NetworkErrorInfo`) are their owners'
 * (`Network/PatInterface.h`, `Network/gamespy_interface_types.h`), included only by the units that use them. */

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

/* the writer's remaining entry points the tail of the range drives (the second writer class); the
   writer methods `Network/NetworkCommunityPat.cpp` owns (`writeByte`..`writeBytes`,
   `networkStreamWriter_putBytes`..`_size`) are declared in `Network/NetworkCommunityPat.h` */
void networkStreamWriter_attach(NetworkConnectionStable* connection, NetworkStreamWriterDefault* stream);
void networkStreamWriter_reserve(NetworkConnectionStable* connection, const u8* bytes, u32 length, s8 kind);

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
   record whose layout this range never reads.  The reflection adapters `networkSessionReflect0`/`1`
   are declared in `Network/NetworkSessionManagerPat.h` and `NetworkUniqueId` in
   `Network/NetworkCommunityPat.h` (their owners' headers). */

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
void NetworkRequest_copyRecord(NetworkSessionRecordBlock* dst, const NetworkSessionRecordBlock* src);
void NetworkRequestPat_reset(NetworkRequest* self);
void NetworkRequestPat_clear(NetworkRequest* self);
/* untyped: caller-owned payload - the array constructors take raw element pointers */
void __construct_array(void* ptr, void* ctor, void* dtor, u32 size, u32 count);
/* untyped: caller-owned payload - the array destructors take raw element pointers */
void __destroy_arr(void* ptr, void* dtor, u32 size, u32 count);

}

#endif
