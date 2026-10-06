/* The free-hunt monster set `enemy/em_pop.cpp` owns: the `.bss` work `em_set_work` (0x806CC420, 0x6648 B, built
 * by its static constructor and reached through the `.sdata` pointer `em_set_work_ptr`), its thirteen placement
 * entries, the default placement and kill-reward tables in `.data`, and the save block's free-hunt record the
 * work restores from.  Sizes from the object: `memset(&em_set_work, 0, 0x6648)` in `em_set_work_init`, the
 * constructor's `0x5C..0x368` loop of 0x3C-byte entries, `memcpy(.., 0x52)` for each count run and
 * `memcpy(.., 0x78)` for the reward box.  GUESS names throughout, from the bodies that read them.
 */
#ifndef MHTRI_ENEMY_EM_SET_WORK_H
#define MHTRI_ENEMY_EM_SET_WORK_H

#include "types.h"
#include "enemy/ENEMY_WORK.h"          /* EmGroundRec, the placement an entry spawns at */
#include "enemy/em_ground_rec_clear.h" /* em_ground_rec_clear (rule 2) */
#include "quest/quest_types.h"         /* Q_UserData, Q_CountSet, Q_ElementBlock */
#include "mh3_pad/lb_param_work.h"     /* EmSetPointRow */

/* The ground record an entry carries, prepared by `em_ground_rec_clear` (0x80125F54) when the entry is built.
 * size: 0x20 */
struct EmSetGround {
    /* +0x00 */ EmGroundRec rec;

    EmSetGround() { em_ground_rec_clear(&rec); }
};

/* One placement entry: the monster kind and count the default table seeds, its ground record, the enemy it
 * spawned and the save record's spawn parameters (`em_set_entry_rec_apply`).  size: 0x3C */
struct EmSetEntry {
    /* +0x00 */ u16 kind_0x00;          /* the kind `em_kind_release_ck` tests before the rotation spawns it */
    /* +0x02 */ s16 count_0x02;         /* the count the spawn record reports */
    /* +0x04 */ u8 unused_0x04[4];
    /* +0x08 */ EmSetGround ground_0x08;
    /* +0x28 */ struct _ENEMY_WORK* enemy_0x28;  /* the enemy `em_set_entry_spawn` returned */
    /* +0x2C */ s32 live_0x2C;          /* 1 once spawned */
    /* +0x30 */ u8 flags_0x30;          /* the spawn index with bit 7 set while the enemy is held */
    /* +0x31 */ u8 monster_0x31;        /* the monster `em_kind_release` loads */
    /* +0x32 */ u8 param_0x32[3];
    /* +0x35 */ u8 act_set_0x35;        /* the action set `em_set_entry_spawn` picks from */
    /* +0x36 */ u16 act_sub_0x36;
    /* +0x38 */ u16 stat_a_0x38;        /* the two stat rows the spawn rolls */
    /* +0x3A */ u16 stat_b_0x3A;

    EmSetEntry();
};

/* One row of the default placement table `em_set_default_tbl` (`kind` 0 ends it).  size: 0x30 */
struct EmSetDefault {
    /* +0x00 */ s32 kind_0x00;
    /* +0x04 */ s32 count_0x04;
    /* +0x08 */ Q_ElementBlock place_0x08;  /* the placement `em_ground_rec_set` turns into the ground record */
    /* +0x28 */ u8 tail_0x28[8];
};

/* One half of a monster's kill-reward row: four percentages and the item each one gives (the second half is
 * the large-variant row).  size: 0xC */
struct EmSetReward {
    /* +0x0 */ u8 rate_0x0[4];
    /* +0x4 */ u16 item_0x4[4];
};

/* One reward-box slot: an item and how many the hunt gave (capped at 99).  size: 0x4 */
struct EmSetRewardItem {
    /* +0x0 */ u16 item_0x0;
    /* +0x2 */ s16 count_0x2;
};

/* The work.  size: 0x6648 */
struct EmSetWork {
    /* +0x0000 */ u8 unused_0x0000[4];
    /* +0x0004 */ s8 step_0x0004;              /* `em_set_frame_step`'s state */
    /* +0x0005 */ u8 unused_0x0005[3];
    /* +0x0008 */ u8 map_0x0008;               /* the map the field setup enters (1, or 12 by the rank byte) */
    /* +0x0009 */ u8 area_0x0009;
    /* +0x000A */ u8 warp_stage_0x000A;        /* the stage `quest_warp_hub` warps to */
    /* +0x000B */ s8 season_0x000B;            /* the row of `em_set_season_area_tbl` */
    /* +0x000C */ s8 active_0x000C[3];         /* the live entries, -1 for none */
    /* +0x000F */ s8 live_count_0x000F;        /* entries whose monster is loaded */
    /* +0x0010 */ u32 frames_0x0010;           /* hunt frames, counted up to 25 minutes */
    /* +0x0014 */ u32 frames_long_0x0014;      /* the same, up to 99 minutes */
    /* +0x0018 */ s32 area_count_0x0018;
    /* +0x001C */ s16 area_sets_0x001C[16];    /* per area, the set file to load (-1 none) */
    /* +0x003C */ u8 unused_0x003C[0x20];
    /* +0x005C */ EmSetEntry entries_0x005C[13];
    /* +0x0368 */ union {
        u16 hunted_0x0368[0x29];             /* kills per monster, capped at 99 */
        Q_CountSet hunted_set_0x0368;
    };
    /* +0x03BA */ union {
        u16 captured_0x03BA[0x29];           /* captures per monster */
        Q_CountSet captured_set_0x03BA;
    };
    /* +0x040C */ u16 counts_c_0x040C[0x29];
    /* +0x045E */ u16 small_count_0x045E;      /* small-monster kills, capped at 99 */
    /* +0x0460 */ EmSetRewardItem rewards_0x0460[30];
    /* +0x04D8 */ u32 points_0x04D8;           /* the hunt's points, capped at 999999 */
    /* +0x04DC */ u8 unused_0x04DC[8];
    /* +0x04E4 */ Q_UserData userdata_0x04E4;  /* the save block, copied in at the work's init */
    /* +0x64E4 */ u8 unused_0x64E4[0x60];
    /* +0x6544 */ u8* area_buffers_0x6544[32]; /* the 0x800-byte set file buffers */
    /* +0x65C4 */ union {
        u8 rotation_0x65C4[4][2];            /* the lobby's rotation: monster kind and delay in minutes */
        u8 rotation_bytes_0x65C4[8];         /* the same run as the init fills it */
    };
    /* +0x65CC */ u8 rotation_state_0x65CC;
    /* +0x65CD */ u8 rotation_index_0x65CD;
    /* +0x65CE */ u8 unused_0x65CE[2];
    /* +0x65D0 */ u8 (*rotation_ptr_0x65D0)[2];
    /* +0x65D4 */ s32 rotation_timer_0x65D4;
    /* +0x65D8 */ EmSetPointRow points_0x65D8[18]; /* the lobby's point rows, ended by a zero row */
    /* +0x6644 */ u8 userdata_flag_0x6644;
    /* +0x6645 */ u8 unused_0x6645[3];

    EmSetWork();
};

/* The `.sdata` slot the module reads the work through; the second word is the slot's filler.  size: 0x8 */
struct EmSetWorkSlot {
    /* +0x0 */ EmSetWork* work;
    /* +0x4 */ u32 unused_0x04;
};

/* The free-hunt result the save band keeps (`get_FqResult_work`): the count runs, the reward box, the points,
 * the live monsters, the hunt time, the player's pouch and the result ranks.  size: 0x27C */
struct FqResultWork {
    /* +0x000 */ u16 hunted_0x000[0x29];
    /* +0x052 */ u16 captured_0x052[0x29];
    /* +0x0A4 */ u16 counts_c_0x0A4[0x29];
    /* +0x0F6 */ u16 small_count_0x0F6;
    /* +0x0F8 */ EmSetRewardItem rewards_0x0F8[30];
    /* +0x170 */ u32 points_0x170;
    /* +0x174 */ u8 active_0x174[3];          /* each live entry's index plus one, 0 for none */
    /* +0x177 */ u8 unused_0x177;
    /* +0x178 */ struct { /* size: 0x10 */
        /* +0x0 */ u8 area_0x0;               /* the area the entry's enemy was in */
        /* +0x1 */ u8 unused_0x1[0xF];
    } monsters_0x178[3];
    /* +0x1A8 */ u32 frames_0x1A8;
    /* +0x1AC */ u32 seconds_0x1AC;
    /* +0x1B0 */ u32 frames_long_0x1B0;
    /* +0x1B4 */ u8 pouch_0x1B4[0x60];        /* the player's item slots, copied whole */
    /* +0x214 */ u8 spare_0x214[0x20];
    /* +0x234 */ u8 unused_0x234[0xC];
    /* +0x240 */ s8 end_kind_0x240;
    /* +0x241 */ u8 unused_0x241;
    /* +0x242 */ s16 hunt_rank_0x242;
    /* +0x244 */ s16 time_rank_0x244;
    /* +0x246 */ u8 rank_rows_0x246[0x30];
    /* +0x276 */ u8 unused_0x276[2];
    /* +0x278 */ s32 rank_points_0x278;
};

#endif /* MHTRI_ENEMY_EM_SET_WORK_H */
