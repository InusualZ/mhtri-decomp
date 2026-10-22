/* The small per-enemy area record (`_ENEMY_MINI_WORK` in `em_get_unique_work`'s mangling): one element of
 * `enemy/enemy_control.cpp`'s table `em_area_entry_tbl` (128 of them, 0x806A54E0..0x806A76E0).  An enemy that is
 * not in the player's area lives here instead of in a 0xB18-byte `_ENEMY_WORK`; `em_area_entry_store` turns a work
 * into a record and `em_area_entry_restore` turns it back.  `EmAreaEntry` is the same record (`enemy/EmAreaEntry.h`), so the
 * three bytes both spellings name carry both names.
 * size: 0x44 (the table stride, `mulli 0x44` in `em_area_entry_index_clear`) */
#ifndef MHTRI_ENEMY_ENEMY_MINI_WORK_H
#define MHTRI_ENEMY_ENEMY_MINI_WORK_H

#include "types.h"

struct _ENEMY_WORK;

struct _ENEMY_MINI_WORK {
    /* +0x00 */ u8 active;               /* nonzero while the record is in use */
    /* +0x01 */ u8 index_0x01;           /* its own index in `em_area_entry_tbl` (`em_area_entry_index_clear`) */
    union {
        /* +0x02 */ u8 monster_0x02;     /* the kind `em_set_kill_record` tallies */
        /* +0x02 */ u8 group;            /* the same byte as `EmAreaEntry::group` */
    };
    /* +0x03 */ u8 team_0x03;
    /* +0x04 */ u8 step_0x04;            /* the per-frame step `em_area_entry_move` runs: 0 enter, 1 run, 2 hold, 3 drop */
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    union {
        /* +0x08 */ u8 state_0x08;       /* the mode the net events compare against (1, 3 or 4) */
        /* +0x08 */ u8 kind_0x08;        /* the same byte as `EmAreaEntry::kind_0x08` */
    };
    /* +0x09 */ u8 entry_0x09;           /* the enemy data entry `enemy/em022_prog.cpp`'s mini hooks look up */
    union {
        /* +0x0A */ u8 area_0x0A;        /* the area the mini work stands in (`enemy/em022_prog.cpp`) */
        /* +0x0A */ u8 area_no;          /* the same byte as `EmAreaEntry::area_no` */
    };
    /* +0x0B */ u8 area_prev_0x0B;       /* the area it stood in before the last move */
    union {
        /* +0x0C */ u8 mode_0x0C;        /* the placement mode (3: a spawn-record placement, 0xFE: a fresh record);
                                          * `enemy/em010_prog.cpp`'s mini step roams only in mode 3 */
        /* +0x0C */ u8 field_0x0C;       /* the same byte as `EmAreaEntry::field_0x0C` */
    };
    /* +0x0D */ u8 element_0x0D;         /* the quest element it belongs to (above 2: none) */
    /* +0x0E */ u8 spawn_kind_0x0E;      /* how the work it turns back into is entered (1, 2, 3 or 4) */
    /* +0x0F */ u8 field_0x0F;           /* the work's +0x10 byte */
    /* +0x10 */ u8 field_0x10;           /* set by `em_area_entry_restore` once the work was rebuilt from it */
    /* +0x11 */ u8 field_0x11;           /* nonzero, `em_area_entry_store` steps the record at once */
    /* +0x12 */ u8 step_0x12;            /* the step counter the net events raise */
    /* +0x13 */ u8 field_0x13;           /* the work's frame-table index (`_ENEMY_WORK::frames_index_0x015`) */
    /* +0x14 */ u8 end_kind_0x14;        /* 2 when the enemy left without a kill (`em_mini_kill_kind_get`) */
    /* +0x15 */ u8 busy_0x15;            /* `enemy/em010_prog.cpp`'s state reset runs only while it is zero */
    /* +0x16 */ u16 order_0x16;          /* its spawn order (`quest_spawn_rec_find`) */
    /* +0x18 */ u16 link_order_0x18;     /* the work's `link_order_0x01C` */
    /* +0x1A */ s16 timer_0x1A;          /* the frames left in a timed state (3 and 4) */
    /* +0x1C */ u8 phase_0x1C;           /* the phase counter the net events compare against */
    /* +0x1D */ u8 area_slot_0x1D;       /* the area list its spawn record was filed under */
    /* +0x1E */ u8 field_0x1E;
    /* +0x1F */ u8 stat_level_0x1F;      /* the stat level `em_stat_level_roll` picked */
    /* +0x20 */ u8 roam_state_0x20;      /* `enemy/em010_prog.cpp`'s mini step: 0 idle, 1 move timer armed */
    /* +0x21 */ u8 unused_0x21;
    /* +0x22 */ s16 move_timer_0x22;     /* the frames until the next area move (`enemy/em022_prog.cpp`), -1 = unarmed */
    /* +0x24 */ f32 size_scale_0x24;     /* the scale `em_size_scale_roll` picked */
    /* +0x28 */ s32 field_0x28;          /* the work's `field_0x7A0` */
    /* +0x2C */ s32 field_0x2C;          /* the work's `field_0x7AC` */
    union {
        /* +0x30 */ struct _ENEMY_WORK* work_0x30;   /* the live work a large record is linked to */
        /* +0x30 */ struct _ENEMY_WORK* enemy_0x30;  /* the same word as `QuestBossSpawn::enemy_0x30` */
        /* +0x30 */ u32 spawn_0x30;                  /* the same word as `enemy/em003_prog.cpp`'s pick reads it */
    };
    union {
        /* +0x34 */ u8 data_0x34[0x10];      /* the work's `data_0xB04` */
        struct {
            /* +0x34 */ u8 resume_mode_0x34;  /* the work's `resume_mode_0xB04` (`enemy/em027_prog.cpp`'s mini step) */
            /* +0x35 */ u8 seated_keep_0x35;  /* the work's `seated_keep_0xB05` */
        }; /* size: 0x02 */
    };
};

#endif /* MHTRI_ENEMY_ENEMY_MINI_WORK_H */
