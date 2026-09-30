/*
 * The arena-task band's record types (`include/unsplit/<band>.h`, the band's fallback home).
 *
 * This header held the band's *unowned* declarations while no registered unit owned them.  The data
 * symbols are owned now: `src/quest/arenatask.cpp` claimed its own `.data`/`.bss`/`.sdata`/`.sbss`/
 * `.sdata2` runs in `config/RMHE08/splits.txt` (rule 12), so their declarations moved to that unit's
 * own header, `include/quest/arenatask.h`, and a consumer includes *that* (rule 2).  What is left here
 * is what a symbol-free header can hold - the band's record types, which the band's other units will
 * include once they register.  A declaration of a symbol *another* unit owns is still wrong here: it
 * belongs in that owner's header (`my_player_no` -> `include/ef/fn_800CDB2C.h`, `get_arena_cfg` ->
 * `include/menu/get_pop_dat_ptr.h`, `setVec3` -> `include/mh3_pad.h`, `multi_arena_clr_time` ->
 * this file's sibling `include/unsplit/menu.h`; `quest_record_get` (owner `menu/arena_result.cpp`)
 * has no published header yet - that absence is what blocks `dl_acdata_to_ar_eqdata`).
 *
 * Evidence classes: the runtime dump's own local names (`arena_work`, `arena_draw_func`, `que_info`,
 * `arena_lsp_data_adrs`, `multi_arena_clr_time` and the `arena_multi_*` tables) come from
 * `D:/WiiExperiment/Dump_Loading85.raw.map`; every field name is derived from the body that reads it,
 * and the two `.bss` vector names are marked a GUESS in the unit header (`NAMES`).
 */
#ifndef MHTRI_UNSPLIT_ARENA_H
#define MHTRI_UNSPLIT_ARENA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The arena quest-info row list `arena_quest_info_build` fills (`.sbss` 0x80794D3C, the dump's `que_info`):
 * ten 0x18-byte rows, one per arena quest id 0x2328 + i. size: 0x18 */
typedef struct ArenaQuestInfo {
    /* +0x00 */ u8 kind_0x00;             /* the quest row's own +0x8A category - 1 */
    /* +0x01 */ u8 rank_0x01;             /* the quest row's +0x8B */
    /* +0x02 */ u8 flag_0x02;             /* the quest row's +0x314 */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ const u8* record_0x04;    /* the quest row itself */
    /* +0x08 */ const u8* sub_0x08;       /* the quest row + 0x2E */
    /* +0x0C */ u8 unused_0x0C[0x04];
    /* +0x10 */ s32 time_0x10;            /* the quest row's +0x13A (the target stores it with `stw`) */
    /* +0x14 */ u32 clear_limit_0x14[1];  /* the `multi_arena_clr_time` entry for this quest */
} ArenaQuestInfo; /* size: 0x18 */
typedef struct ArenaQuestInfoList {
    /* +0x00 */ ArenaQuestInfo row_0x00[10];
} ArenaQuestInfoList; /* size: 0xF0 */

/* One 0xC-byte per-player equipment record of the arena acdata (`arena_eqdata_head_set` copies the
 * id byte out of the source record's first u16 and the u16 that follows it). size: 0xC */
typedef struct ArenaEqDataHead {
    /* +0x00 */ u8 id_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u8 unused_0x04[0x08];
} ArenaEqDataHead; /* size: 0xC */

/* The 0xEC-byte arena equipment record `dl_acdata_to_ar_eqdata` fills: eight heads, a count byte and
 * two trailing blobs copied verbatim out of the acdata record. size: 0xEC */
typedef struct _arena_eq_data {
    /* +0x00 */ ArenaEqDataHead head_0x00[8];
    /* +0x60 */ u8 count_0x60;
    /* +0x61 */ u8 unused_0x61[0x0B];
    /* +0x6C */ u8 body_0x6C[0x60];
    /* +0xCC */ u8 tail_0xCC[0x20];
} _arena_eq_data; /* size: 0xEC */

/* One stage record of `arena_stage_config` (`.data` 0x80604D30, ten of them): four per-player equip
 * records; the rest of the 0x3B0 is exactly those four (4 * 0xEC). size: 0x3B0 */
typedef struct ArenaStageRecord {
    /* +0x000 */ _arena_eq_data eq_0x000[4];
} ArenaStageRecord; /* size: 0x3B0 */

/* One 0x18-byte per-player arena equip slot: `arena_eqdata_reset` clears it, `arena_eqdata_apply` fills
 * `kind_0x08`/`state_0x03` in it and `arena_result_next` binds the player's Vs user block to it.
 * size: 0x18 */
struct _vs_user_data;
typedef struct ArenaEqSlot {
    /* +0x00 */ u8 player_0x00;       /* the move-work player index `arena_eqdata_apply` scales by 0xB20 */
    /* +0x01 */ u8 mode_0x01;         /* 3 / 4 / 5 */
    /* +0x02 */ u8 clear_0x02;        /* zeroed by `arena_eqdata_apply` */
    /* +0x03 */ u8 state_0x03;        /* 0 / 5 / 4 / 3 */
    /* +0x04 */ u8 stage_col_0x04;    /* the stage's column on the `pair_a_0x06`-wide stage grid (stage % width) */
    /* +0x05 */ u8 stage_row_0x05;    /* its row (stage / width) */
    /* +0x06 */ u8 pair_a_0x06;       /* 3 / 4; also the stage grid's width in `arena_result_next`'s mode 1 */
    /* +0x07 */ u8 pair_b_0x07;       /* 2 */
    /* +0x08 */ u8 kind_0x08;         /* the `arena_eqdata_apply` kind */
    /* +0x09 */ u8 state_0x09;        /* 3 when the owner's +0x04 is 2, else 4 */
    /* +0x0A */ u8 cleared_0x0A;      /* only ever cleared by `arena_eqdata_reset` */
    /* +0x0B */ u8 flag_0x0B;
    /* +0x0C */ u8 pair_0x0C;
    /* +0x0D */ u8 mirror_0x0D;       /* a `get_arena_cfg` byte */
    /* +0x0E */ u16 value_0x0E;
    /* +0x10 */ u16 value_0x10;
    /* +0x12 */ u16 value_0x12;
    /* +0x14 */ struct _vs_user_data* user_0x14;   /* the player's Vs user block (`get_vsUser_work`) */
} ArenaEqSlot; /* size: 0x18 */

/* The arena task's state block (`.bss` 0x806E4010, the dump's `arena_work`): the setup mode, the Vs mode
 * and stage, the two equip slots the mode selects and the sub-mode the task compares against 3.  It is
 * also the parameter block `arena_eqdata_setup`/`_reset`/`_apply` and `arena_player_init` fill (their old
 * `ArenaEqParams` view was this same record's first 0x4C bytes).  Only the offsets the band's functions
 * touch are named; the rest is filler so the size and the offsets stay checkable. size: 0x58 */
typedef struct ArenaWork {
    /* +0x00 */ u8 mode_0x00;         /* the setup's own 0/1/2 mode */
    /* +0x01 */ u8 timer_0x01;        /* GUESS name: 0xC8 in the Vs setups, cleared by every setup */
    /* +0x02 */ u8 unused_0x02;       /* only ever cleared */
    /* +0x03 */ u8 round_0x03;        /* GUESS name: counts `arena_player_init` passes; its `<< 8` is the high byte of the players' motion word */
    /* +0x04 */ u8 mode_0x04;         /* 0 = one-player, 1 = two-player Vs, 2 = network Vs; 2 selects the acdata -> eqdata conversion */
    /* +0x05 */ u8 player_count_0x05; /* the network Vs player count */
    /* +0x06 */ u8 pad_0x06[0x02];
    /* +0x08 */ u16 ready_mask_0x08;  /* the players' ready bits, ORed from their user blocks */
    /* +0x0A */ u16 item_mask_0x0A;   /* one bit per non-empty item slot of the players' user blocks */
    /* +0x0C */ s32 stage_0x0C;       /* 0..9, the arena stage index (-1 once the task leaves the stage select); `arena_task` writes `0x2328 + stage` to `lb_param_w+0x00` and indexes the 0x3B0-byte config record with it */
    /* +0x10 */ const ArenaStageRecord* eq_data_0x10;   /* the stage's 0xEC-byte acdata equip records, indexed by a slot's `player_0x00` */
    /* +0x14 */ ArenaEqSlot slot_0x14[2];
    /* +0x44 */ u16 value_0x44;
    /* +0x46 */ u16 value_0x46;
    /* +0x48 */ u16 value_0x48;
    /* +0x4A */ u8 pad_0x4A[0x04];
    /* +0x4E */ s16 sub_mode_0x4E;    /* compared against 3 by `arena_task`, set from the flag `arena_sub_mode_set` takes */
    /* +0x50 */ u8 solo_0x50;         /* 1 when the play is not online (`system_w`'s +0x8AE is 0) */
    /* +0x51 */ u8 pad_0x51[0x07];
} ArenaWork; /* size: 0x58 */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_ARENA_H */
