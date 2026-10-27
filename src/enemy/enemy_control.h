/* The records `enemy/enemy_control.cpp` keeps in its enemy-control work blob `emc_work` (0x806A4590, 0xF50) and the
 * entry points it publishes.  Its 0x18-byte per-kind slot is `EmcSlot` here and `EmcWork` in `enemy/em_kind.cpp`
 * (one record, two definitions).
 */
#ifndef MHTRI_ENEMY_ENEMY_CONTROL_H
#define MHTRI_ENEMY_ENEMY_CONTROL_H

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::VEC3 - the sparkle record's position (rule 11) */
#include "ef/cp_vector.h"   /* _CP_VECTOR - a spawn request's rotation */

struct _ENEMY_WORK;
struct _ENEMY_MINI_WORK;

/* one 0x18-byte per-kind slot of `emc_work` (the array `em_kind_slot_state_find` scans and `em_kind_slot_clear`
 * clears; `enemy/em_kind.cpp` calls the same record `EmcWork`).
 * size: 0x18 */
struct EmcSlot {
    /* +0x00 */ u8 id;         /* the slot index `em_kind_slot_clear` stores, returned by `em_kind_slot_claim` */
    /* +0x01 */ u8 state;      /* 0 idle, 1 claimed, 2 ready */
    /* +0x02 */ u8 kind;       /* the monster kind `em_kind_slot_claim` claims the slot for */
    /* +0x03 */ u8 flags;
    /* +0x04 */ u8 field_0x04; /* the sub-buffer owner id `em_sub_buffer_claim` latches (0: none) */
    /* +0x05 */ u8 unused_0x05[3];
    /* +0x08 */ s32 handle_0x08;  /* the kind's resource file handle */
    /* +0x0C */ s32 handle_0x0C;
    /* +0x10 */ s32 handle_0x10;  /* the second file's handle */
    /* +0x14 */ s32 handle_0x14;
};

/* one 0x14-byte sparkle ("senko") record: the array at `emc_work + 0x90` (`senko_free_get` returns its
 * first free element, `senko_set` fills it, `senko_free` frees an element).
 * size: 0x14 */
struct SenkoRec {
    SenkoRec();
    /* +0x00 */ nw4r::math::VEC3 pos_0x00;
    /* +0x0C */ f32 value_0x0C;  /* `senko_set`'s float */
    /* +0x10 */ s16 countdown;   /* `senko_set`'s third value / the tick `senko_move` steps */
    /* +0x12 */ u8 value_0x12;
    /* +0x13 */ u8 state;        /* 2 = free (set by `senko_free`), else in use */
};

/* one 0x14-byte smoke ("kemuri") record: the array at `emc_work + 0x130` (`kemuri_free_get` returns the first one whose
 * countdown ran out, `kemuri_set` fills it).  The layout is the sparkle record's; the type is its own (MWCC builds the
 * two arrays with two constructors, 0x80147C34 and 0x80147C64).
 * size: 0x14 */
struct KemuriRec {
    KemuriRec();
    /* +0x00 */ nw4r::math::VEC3 pos_0x00;
    /* +0x0C */ f32 value_0x0C;  /* `kemuri_set`'s float */
    /* +0x10 */ s16 countdown;   /* 570 frames from `kemuri_set`, stepped by `kemuri_move` */
    /* +0x12 */ u8 value_0x12;
    /* +0x13 */ u8 state;
};

/* one 0x24-byte spawn request of `emc_work + 0x1F8` (`em_spawn_request` fills a free one, `emc_spawn_req_move` replays it
 * on the area record it was matched to; `hud/pl_frame_sync.cpp` packs one into a net message).
 * size: 0x24 */
struct Marker2Rec {
    Marker2Rec();
    /* +0x00 */ u16 field_0x00;      /* the requesting record's spawn order */
    /* +0x02 */ u16 enemy_id_0x02;   /* the `em_get_unique_work` key of the area record it was matched to */
    /* +0x04 */ u8 field_0x04;       /* the requested monster kind */
    /* +0x05 */ u8 field_0x05;       /* the requested team */
    /* +0x06 */ u8 field_0x06;       /* the entry the spawned record takes */
    /* +0x07 */ u8 field_0x07;       /* the area (0xFF: any) */
    /* +0x08 */ u8 field_0x08;       /* bit 0 match the area exactly, bit 1 `pos_0x0C` is set, bit 2 the rotation is
                                      * set, bit 3 match the team, bit 4 no spawn notice */
    /* +0x09 */ u8 field_0x09;       /* 0 free, 1 matched and waiting for `emc_spawn_req_move` */
    /* +0x0A */ u8 field_0x0A;       /* the spawned record's +0xA05 request byte (0xFF: none) */
    /* +0x0B */ u8 pad_0x0B;
    /* +0x0C */ nw4r::math::VEC3 pos_0x0C;
    /* +0x18 */ u32 field_0x18;      /* the rotation (`_CP_VECTOR` x/y/z) */
    /* +0x1C */ u32 field_0x1C;
    /* +0x20 */ u32 field_0x20;
};

/* one 0x34-byte cached joint matrix of an `EmcJointCache`.
 * size: 0x34 */
struct EmcJointMtx {
    EmcJointMtx();
    /* +0x00 */ u32 joint;           /* the joint id `get_joint_wmat_em` was asked for */
    /* +0x04 */ nw4r::math::MTX34 mtx;
};

/* the joint matrices `emc_joint_cache_fill` caches for one rideable enemy per frame (two caches, `emc_work + 0x36C`).
 * size: 0x518 */
struct EmcJointCache {
    /* +0x000 */ u8 enemy_index;     /* the enemy work index the cache belongs to, 0xFF = none */
    /* +0x001 */ u8 count;           /* the matrices cached so far (at most 25) */
    /* +0x002 */ u8 pad_0x002[2];
    /* +0x004 */ EmcJointMtx mtx[25];
};

/* the last enemy-control event the net sync replays: the enemy's id, phase and step and the event kind and
 * its three values.  size: 0x8 */
struct EmcStatus {
    void assign(const EmcStatus* src);   /* copies the record field by field */
    /* +0x0 */ u16 enemy_id;
    /* +0x2 */ u8 phase;
    /* +0x3 */ u8 step;
    /* +0x4 */ u8 kind;
    /* +0x5 */ u8 value_0x05;
    union {
        /* +0x6 */ u8 value_0x06;
        /* +0x6 */ s8 area_slot_0x06; /* the same byte, the reporting record's area slot (`emc_status_report`) */
    };
    /* +0x7 */ u8 value_0x07;
};

/* one 0x14-byte resource buffer of `emc_work` (six main buffers at +0xDB0 carved by `em_level_set`, six sub-buffers
 * at +0xE28).
 * size: 0x14 */
struct EmResBuffer {
    /* +0x00 */ u8 used;             /* nonzero while a monster kind owns the buffer */
    /* +0x01 */ u8 size_class;       /* 0..3 (`em_res_size_tbl` / the sub-buffer sizes), 0xFF = no buffer */
    /* +0x02 */ u8 owner;            /* the monster kind (main) or the owner id (sub) */
    /* +0x03 */ u8 refs;             /* the sub-buffer's reference count */
    /* +0x04 */ u8* base;
    /* +0x08 */ u32 size;
    /* +0x0C */ u32 used_size;
    /* +0x10 */ u32 free_size;
};

/* the effect resources one sub-buffer registered (`em_sub_buffer_release` releases them with the buffer).
 * size: 0x17 */
struct EmcEftSet {
    /* +0x00 */ u8 count_b;          /* the ids in `ids_b` */
    /* +0x01 */ u8 count_c;          /* the ids in `ids_c` */
    /* +0x02 */ u8 id_a;
    /* +0x03 */ u8 ids_b[10];
    /* +0x0D */ u8 ids_c[10];
};

/* one 6-byte `{weight, min, max}` row of a size-scale roll table (`em_size_tbl`'s lists, ended by weight 0xFFFF).
 * size: 0x6 */
struct EmSizeRoll {
    /* +0x0 */ u16 weight;
    /* +0x2 */ u16 min;
    /* +0x4 */ u16 max;
};

/* the position and yaw an event target is moved to.  size: 0x10 */
struct EmcEventPos {
    /* +0x0 */ nw4r::math::VEC3 pos;
    /* +0xC */ u32 yaw;
};

/* one 0xC-byte target of an enemy-control event: the monster it applies to and the action it is given (a list
 * ended by kind 0).
 * size: 0xC */
struct EmcEventTarget {
    /* +0x0 */ u8 kind;              /* the monster kind */
    /* +0x1 */ u8 area_match;        /* nonzero: the monster must stand in `area` */
    /* +0x2 */ u8 area;
    /* +0x3 */ u8 act;               /* the action `emc_event_act_start` arms */
    /* +0x4 */ u8 end_act;           /* the action `emc_event_act_end` arms when the event ends */
    /* +0x5 */ u8 end_sub;
    /* +0x6 */ u8 pad_0x6[2];
    /* +0x8 */ EmcEventPos* pos;     /* where `emc_event_hold_set` moves the monster, NULL = stay */
};

/* one 6-byte spawn request an enemy-control event issues (a list ended by kind 0).
 * size: 0x6 */
struct EmcEventSpawn {
    /* +0x0 */ u8 kind;              /* the monster kind whose records request */
    /* +0x1 */ u8 req_kind;          /* the requested monster kind */
    /* +0x2 */ u8 req_team;
    /* +0x3 */ u8 keep_entry;        /* 1: the spawned record keeps the requester's entry */
    /* +0x4 */ u8 count;
    /* +0x5 */ u8 flags;
};

/* the monster an enemy-control event highlights.  size: 0x3 */
struct EmcEventMark {
    /* +0x0 */ u8 kind;
    /* +0x1 */ u8 area_match;
    /* +0x2 */ u8 area;
};

/* one enemy-control event (`emc_event_tbl_get`'s entries).
 * size: 0x10 */
struct EmcEvent {
    /* +0x0 */ u8 active;
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ EmcEventTarget* targets;
    /* +0x8 */ EmcEventSpawn* spawns;
    /* +0xC */ EmcEventMark* mark;
};

/* the enemy-control work blob `emc_work`.
 * size: 0xF50 */
struct EmcWork {
    /* +0x000 */ EmcSlot slot_0x000[6];
    /* +0x090 */ SenkoRec senko_0x090[8];
    /* +0x130 */ KemuriRec marker_0x130[10];
    /* +0x1F8 */ struct Marker2Rec marker2_0x1F8[10];
    /* +0x360 */ u32 field_0x360;
    /* +0x364 */ u32 field_0x364;
    /* +0x368 */ s16 field_0x368;    /* the delay frames `emc_delay_frames_get` returns */
    /* +0x36A */ u8 field_0x36A;     /* the last `isReadyCountOne` answer */
    /* +0x36B */ u8 field_0x36B;     /* set for the one frame the ready count turned to one (`emc_host_ck`) */
    /* +0x36C */ EmcJointCache joint_cache[2];
    /* +0xD9C */ EmcStatus status_0xD9C;   /* the last event `hud/pl_frame_sync.cpp` replays to its peers */
    /* +0xDA4 */ s32 res_mem_handle; /* the `pull_res_mem` handle, -1 = none */
    /* +0xDA8 */ u8* res_mem_base;
    /* +0xDAC */ u32 res_mem_size;
    /* +0xDB0 */ EmResBuffer res_buf[6];
    /* +0xE28 */ EmResBuffer sub_buf[6];
    /* +0xEA0 */ EmcEftSet eft_set[6];
    /* +0xF2A */ u8 pad_0xF2A[6];
    /* +0xF30 */ u8 peer_count[4][4];  /* per size class and player: the buffers that player still has to free */
    /* +0xF40 */ u8** stat_tbl;      /* per monster kind: the 0x1C-byte stat rows (`em_stat_tbl_get`) */
    /* +0xF44 */ EmSizeRoll** size_tbl;  /* per size variant: the size-scale roll list */
    /* +0xF48 */ EmcEvent** event_tbl;   /* per event: the event record (`emc_event_tbl_get`) */
    /* +0xF4C */ u32 field_0xF4C;
};

#ifdef __cplusplus
/* The enemy-control work blob (0x806A4590), defined by `enemy/enemy_control.cpp`; `enemy/em_common.cpp`'s sight test
 * reads its sparkle and smoke arrays. */
extern EmcWork emc_work;
#endif

struct EmGroundRec;
struct Q_ElementBlock;
struct EmSetEntry;

#ifdef __cplusplus
extern "C" {
#endif

/* The area-record event handlers the net sync (`hud/pl_frame_sync.cpp`) drives (GUESS: the roles). */
void emc_mini_event_b(struct _ENEMY_MINI_WORK* mini, u8 remote);
void emc_mini_event_a(struct _ENEMY_MINI_WORK* mini, u8 end_kind, u8 remote);
void emc_mini_step(struct _ENEMY_MINI_WORK* mini);
/* 0x80141F6C - replays a spawn request on an area record: its team, entry, order, area, request byte and flags, the
 * position and rotation (the last two arguments travel on the stack). */
void emc_marker_replay(struct _ENEMY_MINI_WORK* mini, u8 team, u8 entry, u16 order, u8 area, u8 request, u8 flags,
                       const nw4r::math::VEC3* pos, s32* rot, u8 remote);

/* 0x80143174 - copies a `VEC3` (`src` -> `dst`) and returns `dst`; the callers pass a third word it ignores. */
void* em_vec3_copy(void* a, void* b, s32 c);
/* The demo-camera helpers the enemy programs call. */
void em_demo_pos_set(struct _ENEMY_WORK* self, f32 x, f32 y, f32 z);
void em_demo_rot_set(struct _ENEMY_WORK* self, f32 x, f32 y, f32 z);

/* 0x801421E4 - fills `out` (an `EmGroundRec`) from the spawn record with order `id` (0xFFFF: none); returns 1 when
 * found. */
u32 em_ground_rec_find(u32 id, void* out);
/* 0x80146008 - 1 once the demo frame has passed half of `frames`. */
u32 em_demo_time_ck(u32 frames);
void em_demo_enable(struct _ENEMY_WORK* self);
/* 0x80145FE4 - the demo work's current frame. */
s16 em_demo_frame_get(void);
/* 0x8014616C - disables the record's demo control and places it at `pos` (NULL: the origin). */
void em_demo_reset(struct _ENEMY_WORK* self, const nw4r::math::VEC3* pos);
/* 0x801461A8 - applies the two key curves (position, rotation; `f32` key tables) at frame `frame * 2`. */
void em_demo_key3_apply(struct _ENEMY_WORK* self, s16 slot, void* a, void* b);
/* 0x801462A4 - applies one or two axes of the position and rotation key curves (`f32` key tables), `c`/`d` the axis
 * masks. */
void em_demo_key_apply(struct _ENEMY_WORK* self, s16 slot, void* a, void* b, u8 c, u8 d);

/* 0x80141B88 - the enemy spawn request (GUESS name): `count` records of `kind`/`team` to place near the area records
 * of spawn order `order`, at most `max`; `pos`/`rot` are optional. */
u32 em_spawn_request(u16 order, u8 kind, u8 team, u8 area, u8 entry, u8 max, u8 count, u8 flags, u8 request,
                     nw4r::math::VEC3* pos, s32 rot);

/* 0x80147894 - the address of entry `index` of the area block `area` for the entry kind `kind` (GUESS name). */
u32 em_area_entry_make(u8* area, u8 kind, u8 index);

/* 0x80143BF8 - 1 for the frame the ready count turned to one. */
u32 emc_host_ck(void);

void em_ground_rec_set(struct EmGroundRec* rec, const struct Q_ElementBlock* element);
struct _ENEMY_WORK* em_small_spawn(u8 monster, struct EmGroundRec* rec, u16 order);
struct _ENEMY_MINI_WORK* em_large_spawn(u8 monster, struct EmGroundRec* rec, u16 order);
void em_intruder_spawn(u8 monster, struct EmGroundRec* rec, u16 row);
s32 em_kind_release_ck(u8 monster);
void em_area_spawn_clear(void);
/* 0x80143A54 - turns every waiting area record of the current area back into a live enemy (GUESS name). */
void em_area_entries_release(void);
/* 0x801477C0 - the first free main resource buffer's base, NULL when all are taken (GUESS name). */
u8* em_res_buffer_get(void);
/* 0x801414D4 - the slot holding monster kind `kind` (0..5), 0xFF when none does (GUESS name). */
u8 em_kind_slot_find(u8 kind);
/* 0x801415A8 - releases slot `index`: its sound bank, its resources and its kind (GUESS name). */
void em_kind_slot_release(u8 index);
/* 0x80143A40 - frees one area record (GUESS name). */
void em_area_entry_release(struct _ENEMY_MINI_WORK* entry);
/* 0x801465A0 - carves the level's resource buffers (GUESS name). */
void em_level_set(s8 level);
/* 0x801422FC - resets every enemy work and the effect slot pool (GUESS name). */
void enemy_work_reset(void);
/* 0x80142E1C / 0x80142EEC - spawn a free-hunt placement entry's monster with spawn order `order` (GUESS names). */
struct _ENEMY_WORK* em_set_entry_spawn(struct EmSetEntry* entry, u16 order);
struct _ENEMY_WORK* em_set_boss_spawn(struct EmSetEntry* entry, u16 order);

/* 0x80147474 - `size` bytes (rounded up to 32) of the sub-buffer of monster kind `kind`; NULL when it is full. */
u8* em_kind_mem_alloc(u8 kind, u32 size);

/* The rest of the published entry points (GUESS names, see the unit header). */
void em_res_file_setup(u32* file);
void enemy_control_init(void);
u8 em_kind_slot_claim(u8 kind);
void em_kind_slot_ready_set(struct EmcSlot* slot);
void em_kind_slot_release_all(void);
u8* em_stat_tbl_get(u8 kind);
u8 em_stat_level_roll(u16 order, u16 seed);
f32 em_size_scale_roll(u16 order, u16 seed);
struct _ENEMY_WORK* em_small_spawn_stat(u8 monster, struct EmGroundRec* rec, u16 order, u8 level, f32 scale);
struct _ENEMY_MINI_WORK* em_area_entry_store(struct _ENEMY_WORK* self);
struct _ENEMY_MINI_WORK* em_large_entry_new(u8 monster, struct EmGroundRec* rec, u16 order, struct _ENEMY_WORK* work);
void enemy_work_store_all(void);
void enemy_control_move(void);
void enemy_control_post_move(void);
void emc_entry_report(struct _ENEMY_MINI_WORK* mini, u8 kind);
s16 emc_delay_frames_get(void);
void emc_event_size_reset_ck(struct _ENEMY_WORK* self);
void emc_event_set(u8 event, u8 area);
void emc_event_hold_release(u8 event);
void emc_event_act_request(u8 event);
void emc_event_act_end_request(void);
void em_res_buffers_free(void);
void em_res_peer_count_dec(s8 size_class);
void em_res_peer_count_inc(s8 size_class, u8 player);
struct EmResBuffer* em_res_buffer_claim(u8 kind);
u8* em_res_buffer_alloc(u8 kind, u32 size);
struct EmResBuffer* em_sub_buffer_claim(u8 kind);
u8 em_sub_buffer_refs_get(u8 kind);
s32 em_sub_buffer_index_get(u8 kind, u8 raw);

#ifdef __cplusplus
}

/* 0x801445E0 - the enemy record lookup by spawn order: stores a live work in `*out` (returns 1) or an area record in
 * `*mini` (returns 2); a C++ free function, so the map row is the mangling (rule 9). */
u8 em_get_unique_work(u16 id, struct _ENEMY_WORK** out, struct _ENEMY_MINI_WORK** mini);
/* 0x80144DA8 - the rideable enemy in area `area` whose joint points `pos` hits: the work, the joint and the hit
 * height (runtime-dump name). */
u32 em_ride_hit_check(nw4r::math::VEC3* pos, u8 area, struct _ENEMY_WORK** out, u32* joint, f32* height);
#endif

#endif
