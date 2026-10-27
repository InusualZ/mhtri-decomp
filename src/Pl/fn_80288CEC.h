/*
 * The records and declarations shared by `Pl/pl_motion.cpp`, `Pl/pl_hit_sphere.cpp` and `Pl/pl_coll.cpp` (rule 1):
 * the box record the box builders (`pl_box_set`/`pl_segment_box_set`) fill and `Pl/pl_coll.cpp`'s point-vs-box tests read,
 * and the player-mode root work.
 */
#ifndef MHTRI_PL_FN_80288CEC_H
#define MHTRI_PL_FN_80288CEC_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* The 0x24-byte record the box builders fill: three 0xC-byte vectors.  `copyVec3` copies one
 * 0xC-byte record and `subVec3` writes `vec_0x0C - vec_0x00` into the third, so +0x00/+0x0C are
 * the two endpoints and +0x18 their difference.  size: 0x24 */
struct PlBox {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;
    /* +0x0C */ nw4r::math::VEC3 vec_0x0C;
    /* +0x18 */ nw4r::math::VEC3 vec_0x18;
};


/* ------------------------------------------------------------------------------------------------ *
 * The player-mode root work (`get_move_work_adrs(0)`).
 *
 * Only the fields these functions read are named (rule 5); untouched bytes keep their offset as an
 * `unused_0xNN` run (rules 4/5).  `recs_0x154` is the four 0x800-byte area records `fn_8028EF7C` /
 * `quest_spawn_rec_find_in` walk when the key is >= 0x20, each an array of 0x10-byte `{key, ...}` entries whose
 * count lives in `rec_counts_0x2154`; `sparse_0x2274` is the six-entry table they walk for a
 * key < 0x20.
 * size: 0x22E8 (lower bound - the highest field read is 0x22E7)
 * ------------------------------------------------------------------------------------------------ */
/* size: 0x10 */
struct PlRootEntry {
    /* +0x0 */ u32 key_0x00; /* matched against the caller's key */
    /* +0x4 */ u8 unused_0x04[0x10 - 0x4];
};

/* size: 0x800 */
struct PlRootArea {
    /* +0x000 */ PlRootEntry entries_0x00[0x800 / 0x10];
};

/* size: 0x22E8 (lower bound - the highest field read is 0x22E7) */
struct _PL_ROOT {
    /* +0x0000 */ u8 mode_0x00;         /* the scene mode the game tasks dispatch on: 0 boot, 1 load, 2 main, 3 end,
                                         * 4 leave (`root_mode2_ck` tests `== 2`) */
    /* +0x0001 */ u8 seq_0x01;          /* the mode's step */
    /* +0x0002 */ u8 kind_0x02;         /* the step's sub-step */
    /* +0x0003 */ u8 sub_step_0x03;     /* the sub-step's own counter (`scene_area_change_step` step 4) */
    /* +0x0004 */ union {   /* the two timers, and the per-player input holds the versus warp indexes */
        struct {   /* size: 0x4 */
            /* +0x0004 */ s16 timer_0x04; /* the mode's frame countdown (also player 0's input hold) */
            /* +0x0006 */ s16 timer_0x06; /* player 1's input hold, and the network ready-poll counter */
        };
        /* +0x0004 */ s16 hold_timers_0x04[2];
    };
    /* +0x0008 */ u8 unused_0x0008[0x0018 - 0x0008];
    /* +0x0018 */ u32 counters_0x18[0x20]; /* cleared by the scene work inits */
    /* +0x0098 */ u8 unused_0x0098[0x009C - 0x0098];
    /* +0x009C */ u32 counters_0x9C[0x10]; /* cleared by the scene work inits */
    /* +0x00DC */ u8* mode_work_0xDC;    /* `pl_net_move_handover` sets its +0x6A3E byte */
    /* +0x00E0 */ s32 area_slots_0xE0[2]; /* the stage area slot each half holds (-1: none) */
    /* +0x00E8 */ u8 area_slot_index_0xE8; /* which of `area_slots_0xE0` is live */
    /* +0x00E9 */ u8 area_no_0xE9;       /* `pl_map_kind_get` hands it to `stage_map_kind_get` */
    /* +0x00EA */ u8 area_no_0xEA;       /* the area the players are in */
    /* +0x00EB */ u8 area_no_0xEB;       /* the area `quest_marker_draw_at` places the master's marker in */
    /* +0x00EC */ u8 prev_area_no_0xEC;  /* the area an area change left */
    /* +0x00ED */ u8 area_no_0xED;       /* the two `fn_8028C570` scene selectors */
    /* +0x00EE */ u8 unused_0x00EE;
    /* +0x00EF */ u8 npc_present_0xEF;   /* the AI NPC's marker is drawn while this is set (`quest_npc_marker_draw`) */
    /* +0x00F0 */ u8 field_0x00F0;       /* copied from `system_w`'s +0x7CD by the quest scene init */
    /* +0x00F1 */ u8 unused_0x00F1[0x00FA - 0x00F1];
    /* +0x00FA */ u8 end_kind_0xFA;      /* why the quest ends: 2 running, 4/6/7 the result messages, 8 a lost link */
    /* +0x00FB */ u8 seq_0xFB;           /* `pl_quest_end_request` stores 4 or 6 */
    /* +0x00FC */ u8 seq_0xFC;           /* `pl_net_move_handover` stores 4 */
    /* +0x00FD */ u8 unused_0x00FD[0x0111 - 0x00FD];
    /* +0x0111 */ u8 main_step_0x111;    /* the quest main mode's end-check step */
    /* +0x0112 */ u8 frozen_0x112;       /* non-zero while an area change holds the actors */
    /* +0x0113 */ u8 scene_kind_0x113;   /* 1 for the field scene, 2 for the versus scene */
    /* +0x0114 */ s32 flag_0x114;       /* `quest_move_idle_ck` tests `<= 0`; `root_wait_frames_set` stores */
    /* +0x0118 */ u8 unused_0x0118[0x011C - 0x0118];
    /* +0x011C */ s32 field_0x11C;       /* set to -1 by the quest scene init */
    /* +0x0120 */ s32 stage_no_0x120;    /* the quest's stage, the index into `quest_stage_table` */
    /* +0x0124 */ s32 map_no_0x124;      /* the stage's map number */
    /* +0x0128 */ VEC3 pos_0x128;        /* the master's world position (`hud/cockpit_quest.cpp`'s marker) */
    /* +0x0134 */ u16 rot_y_0x134;       /* its facing, 0x18000 minus it picks the marker's icon rotation */
    /* +0x0136 */ u8 bytes_0x136[0x0138 - 0x0136]; /* `Pl_area_flag_get` indexes it by a `u8` (one per player) */
    /* +0x0138 */ u8* se_handles_0x138[3]; /* the three `se_entry_request` handles `scene_se_handles_release` clears */
    /* +0x0144 */ s32 stage_variant_0x144; /* the quest's `quest_stage_variant_table` byte */
    /* +0x0148 */ u32 unlock_seen_0x148[2]; /* copied from `system_w`'s +0x888 */
    /* +0x0150 */ u8 unused_0x0150[0x0154 - 0x0150];
    /* +0x0154 */ PlRootArea recs_0x154[4];
    /* +0x2154 */ u32 rec_counts_0x2154[4];
    /* +0x2164 */ u8 unused_0x2164[0x2254 - 0x2164];
    /* +0x2254 */ void* spawn_work_0x2254; /* the 0x1200-byte work the scene inits allocate */
    /* +0x2258 */ PlRootEntry* extra_0x2258;
    /* +0x225C */ s32 field_0x225C[6];   /* set to -1 by the scene inits */
    /* +0x2274 */ PlRootEntry sparse_0x2274[6];
    /* +0x22D4 */ u8 unused_0x22D4[0x22D7 - 0x22D4];
    /* +0x22D7 */ s8 field_0x22D7;
    /* +0x22D8 */ u8 field_0x22D8;       /* the day/night stage byte `get_gm_daynight` returns */
    /* +0x22D9 */ u8 field_0x22D9;
    /* +0x22DA */ u8 field_0x22DA;       /* 1 while the scene runs as a network session */
    /* +0x22DB */ u8 field_0x22DB;
    /* +0x22DC */ u8 unused_0x22DC;
    /* +0x22DD */ u8 server_slots_0x22DD[4]; /* per player: 1 present, 4 dropped from the session */
    /* +0x22E1 */ u8 lowest_server_0x22E1; /* the lowest live server index */
    /* +0x22E2 */ u8 server_count_0x22E2; /* the occupied server slots at the scene start */
    /* +0x22E3 */ u8 warp_req_0x22E3;    /* non-zero asks the quest main mode for an entry-point warp */
    /* +0x22E4 */ u8 retry_0x22E4;       /* 1 when the scene init was asked for kind 2 */
    /* +0x22E5 */ u8 loading_0x22E5;     /* 1 while a scene's load mode runs */
    /* +0x22E6 */ u8 bgm_volume_0x22E6;  /* the BGM volume the field end mode fades down */
    /* +0x22E7 */ u8 se_volume_0x22E7;   /* the SE volume the field end mode fades down */
};

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028EF30 - the per-player flag byte at +0x136 of the player move work (0 when the work is missing). */
u8 Pl_area_flag_get(u8 index);

/* 0x8028BD54 - is the root work in mode 2?  (`Pl_net_can_send` asks before it lets a client send.) */
u32 root_mode2_ck(void);

#ifdef __cplusplus
}
#endif


/* 0x8028D0EC - starts a player warp to `pos` facing `angle`: stores the destination in the move work,
 * arms the transfer mode and the fade; `mode` 0xFF warps the current player only.  C linkage (the map
 * row is a plain name).  GUESS name from those effects; `quest/quest_entry.cpp` calls it. */
#ifdef __cplusplus
extern "C" {
void pl_warp_start(u8 mode, nw4r::math::VEC3* pos, u16 angle);
}

/* 0x8028C5B4 - loads the players' motion resources (`pl_motion_set__Fv`, C++ scope). */
void pl_motion_set(void);
#endif

#endif /* MHTRI_PL_FN_80288CEC_H */
