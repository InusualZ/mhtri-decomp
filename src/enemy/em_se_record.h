/* enemy/em_se_record.h - the per-motion sound-effect program records the enemy programs define in their `.data`
 * and `em_se_tbl_play`/`em_se_tbl_play_alt` (`enemy/em_common.cpp`) walk.  The walker that reads the kinds is
 * `enemy/fn_8011D448.cpp`'s table interpreter.  Field names are a GUESS from the values. */
#ifndef MHTRI_ENEMY_EM_SE_RECORD_H
#define MHTRI_ENEMY_EM_SE_RECORD_H

#include "types.h"

/* The two-float `.sdata` offset a sound record can point at. */
struct EmSeOffset {
    /* +0x00 */ f32 x_0x00;
    /* +0x04 */ f32 y_0x04;
}; /* size: 0x08 */

/* The sound-position record a kind 0x0A entry points at. */
struct EmSePos {
    /* +0x00 */ f32 x_0x00;
    /* +0x04 */ f32 y_0x04;
    /* +0x08 */ s16 channel_0x08;
    /* +0x0A */ s16 angle_0x0A;
    /* +0x0C */ s32 pad_0x0C;
}; /* size: 0x10 */

/* One sound-effect record of a per-motion table: the id runs 0x64..0x72, the delay is a small frame count. */
struct EmSeRecord {
    /* +0x00 */ u16 se_id_0x00;
    /* +0x02 */ u16 pad_0x02;
    /* +0x04 */ s32 delay_0x04;
    /* +0x08 */ s32 length_0x08;
    /* +0x0C */ s32 pad_0x0C;
    /* +0x10 */ u8 flags_0x10[4];
    /* +0x14 */ const EmSeOffset* shift_0x14;
}; /* size: 0x18 */

/* One entry of a per-motion sound table: a kind byte and a word that is a record, a count, a branch or a jump
 * target, by kind (0/1 = record, 5 = count, 8 = jump into a table, 9 = branch, 0xFF/0xFE/0xFD end the table). */
struct EmSeEntry {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 pad_0x01[3];
    /* +0x04 */ const EmSeRecord* arg_0x04;
}; /* size: 0x08 */

/* The record a kind 9 entry points at: a selector word, the id it is compared with and the sub-table it runs. */
struct EmSeBranch {
    /* +0x00 */ u32 mode_0x00;
    /* +0x04 */ u32 id_0x04;
    /* +0x08 */ const EmSeEntry* table_0x08;
}; /* size: 0x0C */

#endif
