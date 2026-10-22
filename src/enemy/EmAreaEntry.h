#ifndef MHTRI_ENEMY_EMAREAENTRY_H
#define MHTRI_ENEMY_EMAREAENTRY_H

#include "types.h"

/* One entry of the area/group table `em_area_entry_tbl` (0x806A54E0..0x806A76E0, `enemy/enemy_control.cpp`'s `.bss`):
 * four of them per group, 32 groups.
 * Only the four bytes `fn_8012E968` tests are named.
 * size: 0x44 */
typedef struct EmAreaEntry {
    /* +0x00 */ u8 active;           /* nonzero while the entry is in use */
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 group;            /* matched against `_ENEMY_WORK::group` */
    /* +0x03 */ u8 team_0x03;  /* the enemy kind (`em_area_entry_near_ck` hands it to `enemy_data_by_team_get`) */
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8 kind_0x08;        /* `enemy/em012_prog.cpp`'s pack count counts kind 4; `enemy/em003_prog.cpp`'s
                                    * call scans count kind 1 */
    /* +0x09 */ u8 entry_0x09;       /* compared with `_ENEMY_WORK::field_0x46C` by the same count */
    /* +0x0A */ u8 area_no;          /* matched against `_ENEMY_WORK::field_0x00A` */
    /* +0x0B */ u8 unused_0x0B;
    /* +0x0C */ u8 field_0x0C;       /* must be 1 for the entry to count */
    /* +0x0D */ u8 unused_0x0D[0x16 - 0x0D];
    /* +0x16 */ u16 order_0x16;      /* the spawn order of the spawn record that placed it (32 and up: an area list's) */
    /* +0x18 */ u8 unused_0x18[0x30 - 0x18];
    /* +0x30 */ u32 spawn_0x30;      /* nonzero for an entry `em003_call_group_pick` may pick (a spawned record's handle) */
    /* +0x34 */ u8 unused_0x34[0x44 - 0x34];
} EmAreaEntry; /* size: 0x44 */

#endif
