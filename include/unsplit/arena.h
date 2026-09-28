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

/* The arena task's own state block (`.bss` 0x806E4010, 0x58 B, the dump's `arena_work`).  Only the
 * offsets `src/quest/arenatask.cpp`'s functions touch are named; the rest is filler so the size and
 * the offsets stay checkable against the disassembly (rule 3/4/5). size: 0x58 */
typedef struct ArenaWork {
    /* +0x00 */ u8 unused_0x00[0x04];
    /* +0x04 */ u8 mode_0x04;             /* 1 vs 2 selects the Vs (2-player) variant, read by `arena_task` */
    /* +0x05 */ u8 unused_0x05[0x07];
    /* +0x0C */ s32 stage_0x0C;           /* 0..9, the arena stage index; `arena_task` writes `0x2328 + stage` to `lb_param_w+0x00` and indexes the 0x3B0-byte config record with it */
    /* +0x10 */ const u8* eq_data_0x10;   /* the arena's 0xEC-byte acdata equip records `arena_eqdata_from_userdata` indexes with the player's chunk slot (`arena_work + 0x10` is loaded as a full word) */
    /* +0x14 */ u8 eq_slot_0x14;          /* 0..3, the 0xEC-byte eq record inside the stage's config */
    /* +0x15 */ u8 unused_0x15[0x17];
    /* +0x2C */ u8 other_eq_0x2C;         /* the equip index a *remote* player chose */
    /* +0x2D */ u8 unused_0x2D[0x0A];
    /* +0x37 */ u8 other_eq_dirty_0x37;   /* set with `other_eq_0x2C` */
    /* +0x38 */ u8 unused_0x38[0x16];
    /* +0x4E */ u16 sub_mode_0x4E;        /* compared against 3 by `arena_task`, set from the flag `arena_sub_mode_set` takes */
    /* +0x50 */ u8 unused_0x50[0x08];
} ArenaWork; /* size: 0x58 */

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
    /* +0x10 */ s16 time_0x10;            /* the quest row's +0x13A */
    /* +0x12 */ u8 unused_0x12[0x02];
    /* +0x14 */ u16 clear_limit_0x14[2];  /* the `multi_arena_clr_time` pair for this quest (a 4-byte row there) */
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

/* One 0x18-byte per-player arena equip slot: `arena_eqdata_reset` clears it and `arena_eqdata_apply`
 * fills `kind_0x08`/`state_0x03` in it.  Only the offsets those two functions touch are named. size: 0x18 */
typedef struct ArenaEqSlot {
    /* +0x00 */ u8 player_0x00;       /* the move-work player index `arena_eqdata_apply` scales by 0xB20 */
    /* +0x01 */ u8 mode_0x01;         /* 3 / 4 / 5 */
    /* +0x02 */ u8 clear_0x02;        /* zeroed by `arena_eqdata_apply` */
    /* +0x03 */ u8 state_0x03;        /* 0 / 5 / 4 / 3 */
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 unused_0x05;
    /* +0x06 */ u8 pair_a_0x06;       /* 3 / 4 */
    /* +0x07 */ u8 pair_b_0x07;       /* 2 */
    /* +0x08 */ u8 kind_0x08;         /* the `arena_eqdata_apply` kind */
    /* +0x09 */ u8 state_0x09;        /* 3 when the owner's +0x04 is 2, else 4 */
    /* +0x0A */ u8 unused_0x0A;
    /* +0x0B */ u8 flag_0x0B;
    /* +0x0C */ u8 pair_0x0C;
    /* +0x0D */ u8 mirror_0x0D;       /* a `get_arena_cfg` byte */
    /* +0x0E */ u16 value_0x0E;
    /* +0x10 */ u16 value_0x10;
    /* +0x12 */ u16 value_0x12;
    /* +0x14 */ u8 unused_0x14[0x04];
} ArenaEqSlot; /* size: 0x18 */

/* The arena's parameter block `arena_eqdata_setup` switches on and `arena_eqdata_reset`/
 * `arena_eqdata_apply` fill: a header, two equip slots and three trailing u16s. size: 0x4C */
typedef struct ArenaEqParams {
    /* +0x00 */ u8 mode_0x00;         /* the setup's own 0/1/2 mode */
    /* +0x01 */ u8 field_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 vs_mode_0x04;      /* == 2 selects the acdata -> eqdata conversion */
    /* +0x05 */ u8 unused_0x05[0x0B];
    /* +0x10 */ const u8* eq_records_0x10;   /* 0xEC-byte records, indexed by a slot's +0x00 */
    /* +0x14 */ ArenaEqSlot slot_0x14;
    /* +0x2C */ ArenaEqSlot slot_0x2C;
    /* +0x44 */ u16 value_0x44;
    /* +0x46 */ u16 value_0x46;
    /* +0x48 */ u16 value_0x48;
    /* +0x4A */ u8 unused_0x4A[0x02];
} ArenaEqParams; /* size: 0x4C */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_ARENA_H */
