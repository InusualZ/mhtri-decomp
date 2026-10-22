/*
 * ef/eft_slot.cpp - the `_EFT` family's 10-entry slot pool (`lbl_806BF0A0`, one 0x3C-byte `EftSlot` per entry), the
 *   enemy-record scan that drives it, the `EftDef` definition table and the slot predicates and state setters.
 * RANGE. .text 0x803432B4-0x80348A48 (76 functions); extab 0x80016DB4-0x80016F74, extabindex 0x800362F4-0x80036594,
 *   .ctors 0x8056F3A4-0x8056F3A8, .data 0x805E7B70-0x805E91E8, .bss 0x806BF0A0-0x806BF310, .sdata
 *   0x807930F0-0x807932F0, .sdata2 0x8079B320-0x8079B368.  The tail from 0x80348A48 is `menu/menu_effect_slot.cpp`.
 *   The left edge is false: `fn_803432B4` is the `eft050_set` record's move state (`ef/eft050.cpp`'s
 *   `fn_80343130` and dispatcher call it, it reads `Eft050SetWork`), `eft_state_advance`/`eft_instance_release`
 *   are that dispatcher's other arms, and `.sdata2` 0x8079B320-0x8079B350 (20, 1900, 2.125, 400, 150, 0.625, 255
 *   and the magic) is `fn_803432B4`'s own pool - this unit starts at 0x80343758 (`eft_slot_clear`).  The `.data`
 *   records 0x805E7FB0/0x805E87F0/0x805E8B48/0x805E9118/0x805E9140 are `{init, step, exit}` triples of this range's
 *   programs.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps the unfused `clrlwi` + `cmpwi` of
 *   `eft_slot_live_ck` and the `cntlzw`/`srwi` of `eft_slot_persist_ck`; playbook 39); `#pragma optimization_level 4`
 *   around the slot programs whose switches retail lowers to range tests (`eft_slot_track_step`, `eft_slot_mode_rearm`,
 *   `eft_slot_unbound_step`, `eft_slot_roam_step`, `eft_slot_arena_state_change`..`eft_slot_arena_step`).
 * NAMES. Every definition is a GUESS from its body (the dump has only placeholders here): the pool (`eft_slot_clear`,
 *   `eft_slot_pool_clear`, `eft_slot_find_free`, `eft_slot_find`, `eft_slot_spawn`, `eft_slot_spawn_targets`), the
 *   definition table (`eft_def_get`, `eft_def_flags`, `eft_def_handler`, `eft_def_model_block`), the predicates
 *   (`eft_slot_armed_ck`, `eft_slot_live_ck`, `eft_slot_persist_ck`, `eft_slot_area_ck`), the per-frame work
 *   (`eft_slot_work_bind`, `eft_slot_work_update`, `eft_slot_counters_step`, `eft_slot_kind_set`,
 *   `eft_slot_effect_key`, `eft_slot_state_set`, `eft_slot_state_request`), the match tests (`eft_work_match_ck`, `eft_target_match_ck`,
 *   `eft_work_wide_ck`, `eft_target_wide_ck`, `eft_slot_match_count`), the (mode, value) pair at +0x0F/+0x10
 *   (`eft_slot_mode_set`, `eft_slot_mode_set_imm`, `eft_slot_mode_set_defer`, `eft_slot_mode_set_map`,
 *   `eft_slot_mode_set_work`) and the two instance arms `eft_state_advance`/`eft_instance_release`.
 *   `enemy_data_find`/`enemy_data_grp` are named for the enemy band's callers, which read the record as
 *   `_ENEMY_DATA` (`enemy/ENEMY_DATA.h`); this file keeps its own `EftSlot` view.
 *   The slot programs are GUESS-described `fn_` rows (their map names are the placeholders): the definition walkers
 *   (`eft_slot_effect_kind_get`, `eft_slot_shape_block_get`, `eft_slot_entry3_get`, `eft_slot_entry4_get`, `eft_slot_area_entry_get`), the shape helpers (`eft_shape_block_center`,
 *   `eft_slot_shape_move`, `fn_80345A2C`, `eft_slot_shape_hit_ck`, `eft_slot_enemy_near_ck`), the enemy binding and rating (`eft_slot_enemy_rank`,
 *   `eft_slot_track_recount`, `eft_slot_pool_frame`, `eft_slot_enemy_bind`, `eft_slot_entry3_has`, `eft_slot_area_rate`, `eft_slot_flee_area_get`, `eft_slot_weight_roll`,
 *   `eft_slot_grade_set`, `eft_slot_enemy_counts_ck`) and the per-definition programs (`eft_slot_track_step`..`eft_slot_arena_step`).
 *   GUESS (from each body and its callers): `eft_slot_enemy_rank`, `eft_slot_track_recount`, `eft_slot_pool_frame`,
 *   `eft_shape_block_center`, `eft_slot_effect_kind_get`, `eft_slot_shape_block_get`, `eft_slot_shape_move`,
 *   `eft_slot_shape_hit_ck`, `eft_slot_enemy_near_ck`, `eft_slot_entry3_get`, `eft_slot_enemy_bind`,
 *   `eft_slot_entry3_has`, `eft_slot_area_rate`, `eft_slot_entry4_get`, `eft_slot_flee_area_get`,
 *   `eft_slot_area_entry_get`, `eft_slot_weight_roll`, `eft_slot_grade_set`, `eft_slot_enemy_counts_ck`,
 *   `eft_slot_track_step`, `eft_slot_mode_rearm`, `eft_slot_key_shape_move`, `eft_slot_escape_time_get`,
 *   `eft_slot_unbound_step`, `eft_slot_state_change`, `eft_slot_area_bind`, `eft_slot_shape_block3_set`,
 *   `eft_slot_link_marks_get`, `eft_slot_linked_step`, `eft_slot_linked_state_change`, `eft_slot_shape_block0_set`,
 *   `eft_slot_roam_step`, `eft_slot_arena_state_change`, `eft_slot_shape_block11_set`, `eft_slot_arena_step`,
 *   `eft_slot_roam_tables`.
 * RESIDUALS. 5 rows unwritten: `fn_803432B4` (0x490, the eft050 seam above), `eft_slot_marks_set_sync` (0x4C: retail calls it
 *   `(entry, 2, area)` from `enemy/em022_prog.cpp`, whose source and `ef/eft_slot.h` spell two arguments - a filed
 *   request), and the compiler-emitted `fn_803489F0`/`fn_803489F4`/`fn_80348A14` (the pool's `__sinit`, which needs
 *   `EftSlot` to be a class with a constructed `VEC3`).
 *   Partial rows (`measure.py ef/eft_slot` lists them), including:
 *  - `enemy_data_grp`: the code is the target's; ours names its switch table `@2246` where retail names
 *    `jumptable_805E918C`;
 *  - `eft_slot_work_update`: one stride reads 0xB20 in retail and 0xB18 in ours, and ours narrows the two ids
 *    (`extsb`, `clrlwi`) at the call where retail does not;
 *  - `eft_slot_clear`: ours indexes the +0x14 store with `stbx` where retail adds first;
 *  - `eft_slot_counters_step`: ours sign-extends the incremented resend timer (`extsh`) before its `sth`;
 *  - `eft_slot_track_recount`: ours keeps the work loop's base in a second register (620 vs 648 bytes).
 *   flipcheck: `.ctors`/`.sdata`/`.data` claimed, not emitted; `.sdata2` ours is the literal pool, not the target's.
 * SHAPES. The definition table `lbl_805E9168` (9 entries, index 0 NULL) and `.bss` `lbl_806BF0A0`/`lbl_806BF2F8` are
 *   defined here; `enemy_data_grp`'s switch table is emitted beside the table (one `.data` chunk, playbook 58).
 */

#include "enemy/em_area_entry_tbl.h" /* em_area_entry_tbl (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "unsplit/unknown.h"
#include "unsplit/ef.h"
#include "fn_8004CAD8.h" /* the vector/geometry helpers the range calls */
#include "Network/network_pat_control.h" /* isReadyCountOne */
#include "camera/camera.h" /* get_camera_pos / get_camera_direction / fn_802BE088 */
#include "ef/fn_800CDB2C.h" /* my_player_no */
#include "ef/EftSlot.h"
#include "ef/eft_slot.h" /* the unit's own declarations (eft_net_send via hud/eft_net_send.h) */
#include "enemy/ENEMY_WORK.h" /* the move-work records `eft_slot_spawn_targets`/`eft_slot_work_bind` walk */
#include "enemy/fn_8012BDF4.h" /* em_work_die_ck, fn_8012D1A8 */
#include "enemy/fn_801251D0.h" /* fn_80125FF0, fn_8012A9E8 */
#include "enemy/enemy_control.h" /* fn_80143BF8 */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 */
#include "g3d/g3d_calcworld.h" /* addVec3To */
#include "ef/get_move_work_adrs.h" /* get_move_work_adrs / get_move_work_max */
#include "enemy/fn_8012EC74.h" /* get_em_scale */
#include "Pl/pl_hit_sphere.h" /* PlCapsule / PlCapsuleBox / pl_capsule_box_set */
#include "Pl/fn_8028F66C.h" /* fn_802907BC */
#include "ef/effect.h" /* fn_800FA378 */
#include "enemy/em_common.h" /* fn_8013023C */
#include "enemy/em003_prog.h" /* fn_8015D6A8 */
#include "enemy/em_kind.h" /* fn_80140C00 */
#include "ai/ainpc.h" /* _AINPC_W */
#include "ai/ainpc_w.h" /* ainpc_w */
#include "stage/stg_w.h" /* stage_map_kind_get */
#include "lobby/lb_quest_screen.h" /* quest_em_stat_tbl_get */
#include "enemy/em_mot_finished_ck.h" /* em_mot_finished_ck */

/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)

struct EftEntry;
/* One record of the definition table `lbl_805E9168`, indexed by `EftSlot::key_0x00`.  `kind_0x00` is
 * the byte `eft_def_flags` returns (its two low bits select the spawn path), `field_0x14` points at the
 * three per-kind entry points `eft_def_handler` selects between. size: 0x18 (to the next named member) */
struct EftDef {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 team_0x01;             /* the enemy team `eft_slot_enemy_bind` binds the slot to */
    /* +0x02 */ u8 mask_0x02;             /* the enemy flag mask `eft_slot_area_rate` requires */
    /* +0x03 */ u8 unused_0x03;
    /* +0x04 */ EftEntry* entries_0x04;   /* the per-motion table `eft_def_model_block` walks */
    /* +0x08 */ EftEntry* entries_0x08;   /* the per-key value lists `eft_slot_entry3_get` walks */
    /* +0x0C */ EftEntry* entries_0x0C;   /* the per-area flee lists `eft_slot_entry4_get` walks */
    /* +0x10 */ EftEntry* entries_0x10;   /* the second table `eft_slot_effect_kind_get` walks */
    /* +0x14 */ void (**handlers_0x14)(_EFT*);
};

/* One 8-byte entry of a definition's sub-table: `key_0x00` is 255 at the end of the run, and
 * `sub_0x04` is the next level down (the model table, or the 2-byte pair table). size: 0x08 */
struct EftEntry {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 unused_0x01[0x04 - 0x01];
    /* +0x04 */ EftEntry* sub_0x04;
};

/* One 2-byte pair of the second table's lowest level: the key and the value; key 255 ends the run.  size: 0x2 */
struct EftPair {
    /* +0x00 */ u8 key_0x00;
    /* +0x01 */ u8 value_0x01;
};

/* One 0x20-byte shape block of the model table: a sphere (`a_0x04`, `radius_0x1C`) or a capsule from `a_0x04` to
 * `b_0x10`.  size: 0x20 */
struct EftShapeBlock {
    /* +0x00 */ u8 kind_0x00;       /* 0 sphere, 1 capsule */
    /* +0x01 */ u8 unused_0x01[0x3];
    /* +0x04 */ Vec a_0x04;
    /* +0x10 */ Vec b_0x10;
    /* +0x1C */ f32 radius_0x1C;
};

/* One 0x44-byte record of the 128-entry array `em_area_entry_tbl` the live-slot scan walks: the same
 * (team, area, action) key an enemy work record carries, plus the key bytes the slot pool matches
 * against.  size: 0x44 */
struct EftTargetRecord {
    /* +0x00 */ u8 live_0x00;
    /* +0x01 */ u8 unused_0x01;
    /* +0x02 */ u8 team_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 unused_0x04[0x08 - 0x04];
    /* +0x08 */ u8 state_0x08;  /* 1 and 2 are skipped by the live-slot scan */
    /* +0x09 */ u8 key_0x09;    /* the slot key the record matches, 255 = none */
    /* +0x0A */ u8 index_0x0A;  /* the slot's own index argument */
    /* +0x0B */ u8 unused_0x0B[0x0F - 0x0B];
    /* +0x0F */ u8 flag_0x0F;   /* 1 keeps an unbound slot from fleeing (`eft_slot_unbound_step`) */
    /* +0x10 */ u8 unused_0x10[0x44 - 0x10];
};

/* The 10-slot pool `eft_slot_pool_clear` clears, `eft_slot_find_free` scans for a free entry and `enemy_data_find`
 * looks a key up in. */

extern "C" {
EftSlot lbl_806BF0A0[10]; /* .bss 0x806BF0A0, 0x258 B */
}
/* The definition table the slot's `key_0x00` indexes (36 B, 9 entries; index 0 is the "no definition"
 * slot).  This unit owns it: its only referrers are this range's own functions (`eft_def_get`,
 * `eft_def_flags`, `eft_def_handler` - 6 relocs, 2 per caller, and no object outside the band names it), it
 * is contiguous with the switch table MWCC emits for `enemy_data_grp` below, and the `.data` claim covers
 * 0x805E9168..0x805E91E8 so the object is the target object.  The values and their order are the DOL's
 * words at 0x805E9168..0x805E918C; the eight non-null entries point at the 0x18-byte `EftDef` records
 * the band's `.data` band holds (playbook 58: a unit's sole-referencer data is claimed, not extern'd). */

extern "C" {
EftDef lbl_806BF2F8;   /* .bss  0x806BF2F8, 0x18 B */
}
extern "C" EftDef lbl_805E7FC0;   /* .data 0x805E7FC0 */
extern "C" EftDef lbl_805E7FD8;   /* .data 0x805E7FD8 */
extern "C" EftDef lbl_805E8800;   /* .data 0x805E8800 */
extern "C" EftDef lbl_805E8828;   /* .data 0x805E8828 */
extern "C" EftDef lbl_805E8B58;   /* .data 0x805E8B58 */
extern "C" EftDef lbl_805E9128;   /* .data 0x805E9128 */
extern "C" EftDef lbl_805E9150;   /* .data 0x805E9150 */
extern "C" EftDef* lbl_805E9168[9] = {
    NULL,
    &lbl_806BF2F8,
    &lbl_805E7FC0,
    &lbl_805E8800,
    &lbl_805E8828,
    &lbl_805E8B58,
    &lbl_805E9128,
    &lbl_805E9150,
    &lbl_805E7FD8,
};
/* The 128-entry record array `eft_slot_spawn_targets`/`eft_slot_match_count` walk. */
extern "C" f32 lbl_8079B350;
extern "C" void eft_state_advance(_EFT* self);  /* `_EFT::state_0x05++` - the dispatcher's advance arm */
extern "C" void eft_instance_release(_EFT* self);  /* hands the instance to `eft_res_slot_release`, the shared release path */
extern "C" void eft_slot_clear(u8 index);  /* zeroes one pool entry's key byte and stamps its +0x14 byte 255 */
extern "C" void eft_slot_pool_clear(void);  /* the 10-iteration loop over `eft_slot_clear` */
extern "C" u8 eft_slot_find_free(void);  /* first entry whose `key_0x00` is zero, 255 when the pool is full */
extern "C" EftDef* eft_def_get(EftSlot* slot);  /* `lbl_805E9168[slot->key_0x00]` */
extern "C" u8 eft_def_flags(EftSlot* slot);  /* the definition's first byte; callers test bits 0, 1, 2 and 3 */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*);  /* `def->handlers_0x14[kind]`, NULL when absent */
extern "C" u32 eft_slot_armed_ck(EftSlot* slot);  /* returns the entry's `armed_0x02` byte */
extern "C" u32 eft_slot_live_ck(EftSlot* slot);  /* a definition flag bit set AND `field_0x18` (the live work) non-zero */
extern "C" u32 eft_slot_persist_ck(EftSlot* slot);  /* the definition's bit 1 clear = the instance survives its work */
extern "C" EftSlot* eft_slot_spawn(u8 key1, u8 key0, u8 index);  /* find-or-allocate, then seed the whole 0x3C block */
extern "C" void eft_slot_spawn_targets(void);  /* one spawn per live enemy work record and per live target record */
extern "C" void eft_slot_work_bind(EftSlot* slot);  /* settles `work_0x03`/`armed_0x02` against `my_player_no()` */
extern "C" void eft_slot_work_update(EftSlot* slot);  /* the per-frame work pick; signals states 5, 7 and 8 */
extern "C" u32 eft_work_match_ck(EftSlot* slot, _ENEMY_WORK* work);  /* does the slot already track this work record? */
extern "C" u32 eft_target_match_ck(EftSlot* slot, EftTargetRecord* record);  /* the same test on the 128-entry array */
extern "C" void eft_slot_match_count(EftSlot* slot);  /* counts the matches into the entry's +0x0D byte */
extern "C" u32 eft_work_wide_ck(EftSlot* slot, _ENEMY_WORK* work);  /* wider test: the key, or a second slot sharing +0x17 */
extern "C" u32 eft_target_wide_ck(EftSlot* slot, EftTargetRecord* record);  /* the same wider test, 128-entry array */
extern "C" u32 eft_slot_area_ck(EftSlot* slot);  /* tracked work -> 0; a matching record of the current area -> 1 */
extern "C" void eft_slot_kind_set(EftSlot* slot, u8 key, u8 force);  /* re-points +0x14, the previous value kept in +0x15 */
extern "C" void eft_slot_counters_step(EftSlot* slot);  /* the three per-frame counters, each saturating at its ceiling */
extern "C" void eft_slot_state_set(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index);  /* writes +0x08/+0x09, calls handler 2 */
extern "C" void eft_slot_state_request(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index);  /* live -> switch now, else record it */
extern "C" u8 eft_slot_effect_key(EftSlot* slot);  /* +0x10 in mode 3, else +0x14 - the key the family's spawner stamps */
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 immediate);  /* (mode, value) into +0x0F/+0x10 */
extern "C" void eft_slot_mode_set_imm(void* slot, u8 mode, u8 value);  /* the wrapper that passes immediate = 1 */
extern "C" void eft_slot_mode_set_defer(void* slot, u8 mode, u8 value);  /* the wrapper that passes immediate = 0 */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 immediate);  /* mode/kind/value onto the pair */
extern "C" void fn_80345A2C(EftSlot* slot);  /* this range's still-unwritten entry `eft_slot_kind_set` calls */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 immediate);  /* the work record rewrites the triple first */
extern "C" EftShapeBlock* eft_def_model_block(EftSlot* slot, u8 index);  /* the per-motion walk down to the 0x20-byte block */
extern "C" void eft_shape_block_center(EftShapeBlock* block, VEC3* out);
extern "C" u8 eft_slot_effect_kind_get(EftSlot* slot);
extern "C" void eft_slot_shape_move(EftSlot* slot, u8 index);
extern "C" EftEntry* eft_slot_entry3_get(EftSlot* slot, u8 key);
extern "C" u32 eft_slot_entry3_has(EftSlot* slot, u8 key, u8 value);
extern "C" void eft_slot_enemy_rank(EftSlot* slot, _ENEMY_WORK** works, s32 n);
extern "C" void eft_slot_enemy_bind(EftSlot* slot);

#pragma peephole off

/* Advances the family's step counter. */
extern "C" void eft_state_advance(_EFT* self) {
    self->state_0x05++;
}

/* Hands the instance to the shared effect release path. */
extern "C" void eft_instance_release(_EFT* self) {
    eft_res_slot_release(self);
}

/* Resets one slot of the 0x3C-byte slot pool: cleared, with its +0x14 byte stamped 255. */
extern "C" void eft_slot_clear(u8 index) {
    u32 offset = (u32)index * 0x3C;

    /* the pool as bytes: neither store names a field (rule 6's byte-offset case) */
    ((u8*)lbl_806BF0A0)[offset] = 0;
    ((u8*)lbl_806BF0A0)[offset + 0x14] = 255;
}

/* Clears the whole 10-slot pool. */
extern "C" void eft_slot_pool_clear(void) {
    u8 i;

    for (i = 0; i < 10; i++) {
        eft_slot_clear(i);
    }
}

/* Returns the first free slot's index, or 255 when the pool is full. */
extern "C" u8 eft_slot_find_free(void) {
    u8 i;

    for (i = 0; i < 10; i++) {
        if (lbl_806BF0A0[i].key_0x00 == 0) {
            return i;
        }
    }
    return 255;
}

/* Looks the 10-entry table up by its group index and the enemy-data id. */
extern "C" void* enemy_data_find(u8 key0, u8 key1) {
    u8 i;

    for (i = 0; i < 10; i++) {
        EftSlot* slot = &lbl_806BF0A0[i];

        if (slot->key_0x00 == key0 && slot->key_0x01 == key1) {
            return slot;
        }
    }
    return NULL;
}

/* The group index the 10-entry table is keyed on, from an enemy's kind byte and its variant. */
extern "C" u8 enemy_data_grp(u8 kind, u8 flag) {
    switch (kind) {
    default:
        return 1;
    case 10:
    case 11:
    case 12:
        return 2;
    case 13:
    case 14:
        return 8;
    case 23:
        return 5;
    case 27:
    case 28:
        return 3;
    case 32:
        return flag == 2 ? 4 : 1;
    case 22:
        return flag == 0 ? 7 : 6;
    }
}

/* Returns the definition record a slot's key byte selects. */
extern "C" EftDef* eft_def_get(EftSlot* slot) {
    return lbl_805E9168[slot->key_0x00];
}

/* Returns the definition's kind byte. */
extern "C" u8 eft_def_flags(EftSlot* slot) {
    return *(u8*)lbl_805E9168[slot->key_0x00];
}

/* Returns the definition's per-kind entry point, or NULL when the definition has none. */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*) {
    EftDef* def = lbl_805E9168[slot->key_0x00];

    if (def->handlers_0x14 == NULL) {
        return NULL;
    }
    switch (kind) {
    case 0:
        return def->handlers_0x14[0];
    case 1:
        return def->handlers_0x14[1];
    case 2:
        return def->handlers_0x14[2];
    }
    return NULL;
}

/* Returns the slot's own kind byte. */
extern "C" u32 eft_slot_armed_ck(EftSlot* slot) {
    return slot->armed_0x02;
}

/* Tests whether the slot's definition asks for the camera-facing variant and the slot is live. */
extern "C" u32 eft_slot_live_ck(EftSlot* slot) {
    if ((eft_def_flags(slot) & 1) != 0 && slot->field_0x18 != 0) {
        return 1;
    }
    return 0;
}

/* Tests whether the slot's definition leaves the instance in the world after its work ends. */
extern "C" u32 eft_slot_persist_ck(EftSlot* slot) {
    return (eft_def_flags(slot) & 2) == 0;
}

/* Spawns (or finds) the slot for one (key, kind) pair and seeds its whole state block. */
extern "C" EftSlot* eft_slot_spawn(u8 key1, u8 key0, u8 index) {
    EftSlot* slot = (EftSlot*)enemy_data_find(key1, key0);
    u8 free_index;

    if (slot != NULL) {
        return slot;
    }
    free_index = eft_slot_find_free();
    if (free_index == 255) {
        return NULL;
    }
    slot = &lbl_806BF0A0[free_index];
    slot->key_0x01 = key0;
    slot->field_0x08 = 0;
    slot->field_0x0A = 0;
    slot->field_0x0B = 0;
    slot->field_0x13 = get_now_mapno();
    slot->field_0x14 = index;
    slot->field_0x11 = 255;
    slot->field_0x12 = 255;
    eft_slot_mode_set_defer(slot, 0, 0);
    slot->field_0x1C = 0;
    slot->field_0x1E = (u16)-1;
    slot->field_0x20 = 0;
    slot->field_0x22 = 0;
    slot->field_0x34 = 0;
    slot->field_0x0D = 0;
    slot->field_0x0E = 0;
    slot->field_0x38 = 0;
    slot->field_0x39 = 255;
    slot->field_0x17 = 255;
    slot->field_0x18 = 0;
    slot->field_0x24 = lbl_8079B350;
    slot->field_0x28 = lbl_8079B350;
    slot->field_0x2C = lbl_8079B350;
    slot->field_0x30 = lbl_8079B350;
    slot->field_0x36 = 255;
    slot->field_0x15 = 255;
    slot->field_0x3A = 0;
    slot->field_0x3B = 255;
    slot->field_0x16 = 0;
    slot->key_0x00 = key1;
    {
        void (*entry)(_EFT*) = eft_def_handler(slot, 0);

        if (entry != NULL) {
            entry((_EFT*)slot);
        }
    }
    slot->field_0x37 = slot->field_0x39 - 1;
    slot->field_0x06 = 0;
    eft_slot_work_bind(slot);
    slot->field_0x04 = 0;
    slot->field_0x05 = 0;
    return slot;
}

/* Spawns the live slots of every enemy work record, then of the 128-entry record array. */
extern "C" void eft_slot_spawn_targets(void) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
    u16 count = get_move_work_max(3);
    s32 i;

    for (i = 0; i < (s32)count; i++, work++) {
        if (em_work_die_ck(work) != 1 && work->field_0x46C != 255) {
            eft_slot_spawn(work->field_0x46C, enemy_data_grp(work->team, work->field_0x00A),
                        work->area_no);
        }
    }
    for (i = 0; i < 128; i++, record++) {
        if (record->live_0x00 != 0 && record->state_0x08 != 1 && record->state_0x08 != 2 &&
            record->key_0x09 != 255) {
            eft_slot_spawn(record->key_0x09, enemy_data_grp(record->team_0x02, record->field_0x03),
                        record->index_0x0A);
        }
    }
}

/* Settles the slot's move-work index and the two bytes that follow it. */
extern "C" void eft_slot_work_bind(EftSlot* slot) {
    s8 current = (s8)my_player_no();

    if ((eft_def_flags(slot) & 2) != 0) {
        slot->armed_0x02 = 1;
        slot->work_0x03 = current;
        return;
    }
    slot->work_0x03 = -1;
    {
        u8* work = (u8*)get_move_work_adrs(2);
        u16 count = get_move_work_max(2);
        s8 i;

        for (i = 0; i < (s8)count; i++, work += 0xB20) {
            if (work[0] != 0 && em_pl_slot_leaving_ck(work[8]) == 0) {
                slot->work_0x03 = i;
                break;
            }
        }
    }
    if (slot->work_0x03 == current) {
        slot->armed_0x02 = 0;
        slot->field_0x0C = slot->work_0x03;
    } else if (slot->work_0x03 == -1) {
        slot->armed_0x02 = 1;
    }
}

/* Picks the move work the slot follows: the free-slot scan, then the forward search for the record
 * whose area the slot was seeded with. */
extern "C" void eft_slot_work_update(EftSlot* slot) {
    s8 current = (s8)my_player_no();
    _ENEMY_WORK* work;
    u16 count;

    if ((eft_def_flags(slot) & 2) != 0) {
        return;
    }
    if (isServerSelectState() != 1) {
        return;
    }
    work = (_ENEMY_WORK*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    if (eft_slot_armed_ck(slot) == 0) {
        if (isReadyCountOne() == 1) {
            u32 found = 0;

            if (work[(s8)slot->work_0x03].active == 0 ||
                em_pl_slot_leaving_ck(slot->work_0x03) != 0) {
                u16 i;

                for (i = 0; i < count; i++) {
                    if (work[i].active != 0 && em_pl_slot_leaving_ck((u8)i) == 0) {
                        slot->work_0x03 = (s8)i;
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 1) {
                if ((s8)slot->work_0x03 == current) {
                    slot->armed_0x02 = 1;
                    eft_net_send(slot, 5, 0);
                    return;
                }
                eft_net_send(slot, 7, slot->work_0x03);
                return;
            }
            if (fn_80143BF8() == 1) {
                eft_net_send(slot, 7, slot->work_0x03);
            }
        } else {
            s16 timer = slot->field_0x06;

            if (timer < 900) {
                slot->field_0x06 = timer + 1;
                return;
            }
            eft_net_send(slot, 8, 0);
        }
    } else {
        if (fn_80143BF8() == 1) {
            slot->armed_0x02 = 1;
            eft_net_send(slot, 5, 0);
            return;
        }
        {
            u8 index = slot->work_0x03;

            if (slot->field_0x14 != *((u8*)work + (s8)index * 0xB20 + 0x16)) {
                u8 next = index + 1;
                u16 i;

                for (i = 0; i < (u16)(count - 1); i++, next++) {
                    _ENEMY_WORK* record;

                    if (next >= count) {
                        next = 0;
                    }
                    record = (_ENEMY_WORK*)((u8*)work + next * 0xB20);
                    if (record->active != 0 && em_pl_slot_leaving_ck(next) == 0 &&
                        slot->field_0x14 == record->field_0x016) {
                        slot->work_0x03 = (s8)next;
                        slot->armed_0x02 = 0;
                        eft_net_send(slot, 5, 0);
                        return;
                    }
                }
            }
        }
    }
}

/* Tests whether the slot is already tracking the enemy work record the scan handed over. */
extern "C" u32 eft_work_match_ck(EftSlot* slot, _ENEMY_WORK* work) {
    if (em_work_die_ck(work) == 0) {
        if (work->action != 0xB && work->action != 0xC &&
            slot->key_0x00 == enemy_data_grp(work->team, work->field_0x00A)) {
            u8 key = work->field_0x46C;

            if (key != 0xFF && key == slot->key_0x01) {
                return 1;
            }
        }
    }
    return 0;
}

/* The same test against one record of the 128-entry array. */
extern "C" u32 eft_target_match_ck(EftSlot* slot, EftTargetRecord* record) {
    if (record->live_0x00 != 0 && record->state_0x08 == 0 &&
        slot->key_0x00 == enemy_data_grp(record->team_0x02, record->field_0x03)) {
        u8 key = record->key_0x09;

        if (key != 0xFF && key == slot->key_0x01) {
            return 1;
        }
    }
    return 0;
}

/* Counts the work records and array entries the slot matches, into the slot's +0x0D byte. */
extern "C" void eft_slot_match_count(EftSlot* slot) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 count = get_move_work_max(3);
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
    u8 i;

    slot->field_0x0D = 0;
    for (i = 0; i < (u8)count; i++, work++) {
        if (eft_work_match_ck(slot, work) == 1) {
            slot->field_0x0D++;
        }
    }
    for (i = 0; i < 128; i++, record++) {
        if (eft_target_match_ck(slot, record) == 1) {
            slot->field_0x0D++;
        }
    }
}

/* The wider match test: the work record's own key, or another live slot that shares this slot's
 * +0x17 byte (or, for the definitions that ask for it, its +0x14 byte). */
extern "C" u32 eft_work_wide_ck(EftSlot* slot, _ENEMY_WORK* work) {
    if (em_work_die_ck(work) == 0 && work->action != 0xB && work->action != 0xC) {
        u8 key = slot->key_0x00;

        if (key == enemy_data_grp(work->team, work->field_0x00A)) {
            u8 other_key = work->field_0x46C;

            if (other_key == slot->key_0x01 && other_key != 0xFF) {
                return 1;
            }
        }
        {
            EftSlot* other = (EftSlot*)enemy_data_find(key, work->field_0x46C);

            if (other != NULL) {
                u8 a = other->field_0x17;
                u8 b = slot->field_0x17;

                if (b == a && b != 0xFF && a != 0xFF) {
                    return 1;
                }
                if ((eft_def_flags(slot) & 8) != 0 && slot->field_0x14 == other->field_0x14) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The same wider test against one record of the 128-entry array. */
extern "C" u32 eft_target_wide_ck(EftSlot* slot, EftTargetRecord* record) {
    if (record->live_0x00 != 0 && record->state_0x08 == 0) {
        u8 key = slot->key_0x00;

        if (key == enemy_data_grp(record->team_0x02, record->field_0x03)) {
            u8 other_key = record->key_0x09;

            if (other_key == slot->key_0x01 && other_key != 0xFF) {
                return 1;
            }
        }
        {
            EftSlot* other = (EftSlot*)enemy_data_find(key, record->key_0x09);

            if (other != NULL) {
                u8 a = other->field_0x17;
                u8 b = slot->field_0x17;

                if (b == a && b != 0xFF && a != 0xFF) {
                    return 1;
                }
                if ((eft_def_flags(slot) & 8) != 0 && slot->field_0x14 == other->field_0x14) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Whether a record of the current area still matches the slot: any enemy work record it already
 * tracks means "not spawned", any array entry whose area byte is the current area means "spawned". */
extern "C" u32 eft_slot_area_ck(EftSlot* slot) {
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 count = get_move_work_max(3);
    s32 i;

    if (eft_slot_armed_ck(slot) == 0) {
        return 0;
    }
    for (i = 0; i < (s32)count; i++, work++) {
        if (eft_work_wide_ck(slot, work) == 1) {
            return 0;
        }
    }
    {
        EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
        s32 j;

        for (j = 0; j < 128; j++, record++) {
            if (eft_target_wide_ck(slot, record) == 1 && record->index_0x0A == get_now_areano()) {
                return 1;
            }
        }
    }
    return 0;
}

/* Re-points the slot at another effect kind (the +0x14 byte the family's spawner stamps). */
extern "C" void eft_slot_kind_set(EftSlot* slot, u8 key, u8 force) {
    if (force != 0 || eft_slot_armed_ck(slot) != 0) {
        u8 previous = slot->field_0x14;

        if (previous != key) {
            slot->field_0x15 = previous;
            slot->field_0x14 = key;
            slot->field_0x34 = 0;
            fn_80345A2C(slot);
            if (force == 0 && (eft_def_flags(slot) & 4) == 0) {
                eft_net_send(slot, 3, 0);
            }
        }
    }
}

/* Steps the slot's three per-frame counters, each saturating at its own ceiling. */
extern "C" void eft_slot_counters_step(EftSlot* slot) {
    s16 timer;

    if (slot->field_0x34 < 0x7FFF) {
        slot->field_0x34 += 1;
    }
    if (slot->field_0x1C < 0x7FFF) {
        slot->field_0x1C += 1;
    }
    timer = slot->field_0x1E;
    if ((u16)timer <= 0x7FFE) {
        slot->field_0x1E = timer + 1;
    }
}

/* Numbers the `n` enemies of `works` for the slot (their +0x46E rank): the definition's own team ranks 0; in the
 * hunting state of keys 2 and 8 the rest rank by distance to the tracked target (a part frame, the AI companion or
 * another enemy), else in list order; then counts the slot's spare hunters into +0x37. */
extern "C" void eft_slot_enemy_rank(EftSlot* slot, _ENEMY_WORK** works, s32 n)
{
    EftDef* def = eft_def_get(slot);
    u8 taken[32];
    s32 order[32];
    f32 dist[32];
    s32 i;
    s32 count;
    _ENEMY_WORK* work;
    _ENEMY_WORK* other;
    f32 d;
    s32 j;
    s32 k;
    s32 spare;

    for (i = 0; i < n; i++) {
        taken[i] = 0;
    }
    slot->field_0x38 = 0;
    if (slot->key_0x00 == 2 || slot->key_0x00 == 8) {
        if (slot->field_0x08 == 1 && slot->field_0x0F <= 2) {
            for (i = 0; i < n; i++) {
                if (works[i]->team == def->team_0x01) {
                    works[i]->state_0x46E = 0;
                    taken[i] = 1;
                } else if (works[i]->state_0x46F != 0xFF) {
                    slot->field_0x38++;
                }
            }
            count = 0;
            for (i = 0; i < n; i++) {
                if (taken[i] != 0) {
                    continue;
                }
                work = works[i];
                switch (slot->field_0x0F) {
                case 0:
                    d = work->frames_0x454[slot->field_0x10];
                    if (d < 0.0f) {
                        work->state_0x46E = 0xFF;
                        continue;
                    }
                    break;
                case 1:
                    if (ainpc_w.active != 0 && work->area_no == ainpc_w.field_0x1A4) {
                        d = vec3_distance(&work->pos, &ainpc_w.vec_0x178);
                        break;
                    }
                    work->state_0x46E = 0xFF;
                    continue;
                case 2:
                    other = (_ENEMY_WORK*)get_move_work_adrs(3) + slot->field_0x10;
                    if (other->active != 0 && work->area_no == other->area_no) {
                        d = vec3_distance(&work->pos, &other->pos);
                        break;
                    }
                    work->state_0x46E = 0xFF;
                    continue;
                }
                if (count == 0) {
                    dist[0] = d;
                    order[0] = i;
                } else {
                    for (j = 0; j < count; j++) {
                        if (dist[j] > d) {
                            for (k = count; k > j; k--) {
                                dist[k] = dist[k - 1];
                                order[k] = order[k - 1];
                            }
                            break;
                        }
                    }
                    dist[j] = d;
                    order[j] = i;
                }
                count++;
            }
            for (i = 0; i < count; i++) {
                works[order[i]]->state_0x46E = i;
            }
        } else {
            k = 0;
            for (i = 0; i < n; i++) {
                if (works[i]->team == def->team_0x01) {
                    works[i]->state_0x46E = 0;
                    taken[i] = 1;
                } else {
                    works[i]->state_0x46E = k;
                    if (works[i]->state_0x46F != 0xFF) {
                        slot->field_0x38++;
                    }
                    k++;
                }
            }
        }
    } else {
        for (i = 0; i < n; i++) {
            works[i]->state_0x46E = i;
            if (works[i]->state_0x46F != 0xFF) {
                slot->field_0x38++;
            }
        }
    }
    if (slot->field_0x39 < n && (spare = slot->field_0x39 - slot->field_0x38) > 0) {
        slot->field_0x37 = spare - 1;
        for (i = 0; i <= slot->field_0x37; i++, works++) {
            if (taken[i] == 0 && (*works)->state_0x46F != 0xFF) {
                slot->field_0x37++;
            }
        }
    }
}

/* Recounts the enemies and records each live slot tracks (slots sharing a definition and key once), numbering the
 * enemies and handing them to the distance sort. */
extern "C" void eft_slot_track_recount(void)
{
    EftSlot* slot = lbl_806BF0A0;
    _ENEMY_WORK* base = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 max = get_move_work_max(3);
    u8 done[10] = {0};
    _ENEMY_WORK* list[32];
    s32 i;
    _ENEMY_WORK* work;
    s32 j;
    EftTargetRecord* record;
    u8 key;
    EftSlot* other;
    s32 k;

    for (i = 0; i < 10; i++, slot++) {
        if (done[i] == 0) {
            done[i] = 1;
            if (slot->key_0x00 == 0 || slot->field_0x16 == 1) {
                continue;
            }
            slot->field_0x0E = 0;
            for (j = 0, work = base; j < max; j++, work++) {
                if (eft_work_wide_ck(slot, work) == 1) {
                    work->state_0x46D = slot->field_0x0E;
                    list[slot->field_0x0E] = work;
                    slot->field_0x0E++;
                    key = work->field_0x46C;
                    if (key != slot->key_0x01) {
                        for (k = j + 1, other = slot + 1; k < 10; k++, other++) {
                            if (other->key_0x00 == slot->key_0x00 && key == other->key_0x01) {
                                done[k] = 1;
                                break;
                            }
                        }
                    }
                }
            }
            if (slot->field_0x0E != 0) {
                eft_slot_enemy_rank(slot->field_0x17 != 0xFF ? (EftSlot*)enemy_data_find(slot->key_0x00, slot->field_0x17) : slot,
                            list, slot->field_0x0E);
            }
            for (j = 0, record = (EftTargetRecord*)em_area_entry_tbl; j < 0x80; j++, record++) {
                if (eft_target_wide_ck(slot, record) == 1) {
                    slot->field_0x0E++;
                    key = record->key_0x09;
                    if (key != slot->key_0x01) {
                        for (k = j + 1, other = slot + 1; k < 10; k++, other++) {
                            if (other->key_0x00 != slot->key_0x00 && key == other->key_0x01) {
                                done[k] = 1;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            for (k = 0, other = lbl_806BF0A0; k < i; k++, other++) {
                if (slot->key_0x00 == other->key_0x00 && slot->field_0x17 != 0xFF &&
                    slot->field_0x17 == other->field_0x17) {
                    slot->field_0x0E = other->field_0x0E;
                }
            }
        }
    }
}

/* Runs the pool's frame: every live slot picks its work, binds its enemy and counts its matches, then steps its
 * counters and its definition's frame handler; the enemy numbering follows. */
extern "C" void eft_slot_pool_frame(void)
{
    EftSlot* slot = lbl_806BF0A0;
    EftSlot* entry = slot;
    s32 i;
    void (*step)(EftSlot*);

    for (i = 0; i < 10; i++, entry++) {
        if (entry->key_0x00 != 0 && entry->field_0x08 != 0xFF) {
            eft_slot_work_update(entry);
            eft_slot_enemy_bind(entry);
            eft_slot_match_count(entry);
        }
    }
    for (i = 0; i < 10; i++, slot++) {
        if (slot->key_0x00 != 0 && slot->field_0x08 != 0xFF && slot->field_0x16 != 1) {
            eft_slot_counters_step(slot);
            step = (void (*)(EftSlot*))eft_def_handler(slot, 1);
            if (step != NULL) {
                step(slot);
            }
        }
    }
    eft_slot_track_recount();
}

/* Switches the slot to another state byte and hands the previous one to the definition's state-2
 * entry point. */
extern "C" void eft_slot_state_set(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index) {
    u8 previous = slot->field_0x08;
    void (*entry)(EftSlot*, u8, _ENEMY_WORK*);

    slot->field_0x08 = state;
    slot->field_0x09 = state;
    slot->field_0x0A = 0;
    slot->field_0x0B = 0;
    slot->field_0x22 = 0;
    slot->field_0x20 = 0;
    slot->field_0x05 = index;
    entry = (void (*)(EftSlot*, u8, _ENEMY_WORK*))eft_def_handler(slot, 2);
    if (entry != NULL) {
        entry(slot, previous, work);
    }
    slot->field_0x3A = 0;
    slot->field_0x3B = 255;
}

/* The external state setter: the live slots switch straight away, the others only record it. */
extern "C" void eft_slot_state_request(EftSlot* slot, u8 state, _ENEMY_WORK* work, u8 index) {
    if (eft_slot_armed_ck(slot) == 1 || eft_slot_live_ck(slot) == 1 || (eft_def_flags(slot) & 4) != 0) {
        if ((u32)(state - 4) > 1) {
            eft_slot_mode_set_defer(slot, 0xFF, 0);
        }
        eft_slot_state_set(slot, state, work, index);
        if (eft_slot_live_ck(slot) == 0 || (eft_def_flags(slot) & 4) == 0) {
            eft_net_send(slot, 1, 0);
        }
    } else {
        slot->field_0x09 = state;
    }
}

/* The byte the family's spawner stamps as the slot's effect key: the +0x10 byte in mode 3, the +0x14
 * byte otherwise. */
extern "C" u8 eft_slot_effect_key(EftSlot* slot) {
    if (slot->field_0x0F == 3) {
        return slot->field_0x10;
    }
    return slot->field_0x14;
}

/* Records a new (mode, value) pair on the slot; the previous pair is kept in the two bytes above. */
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 immediate) {
    if (immediate != 0 || eft_slot_live_ck(slot) == 1) {
        slot->field_0x11 = slot->field_0x0F;
        slot->field_0x12 = slot->field_0x10;
        slot->field_0x0F = mode;
        slot->field_0x10 = value;
        slot->field_0x1C = 0;
        return;
    }
    if (eft_slot_armed_ck(slot) == 1) {
        u8 previous = slot->field_0x0F;

        if (previous != mode || slot->field_0x10 != value) {
            slot->field_0x11 = previous;
            slot->field_0x12 = slot->field_0x10;
            slot->field_0x0F = mode;
            slot->field_0x10 = value;
            slot->field_0x1C = 0;
            if ((eft_def_flags(slot) & 4) == 0) {
                eft_net_send(slot, 2, 0);
            }
        }
    }
}

/* The two thin wrappers the family's spawner uses to stamp a pair with (1 = immediate). */
extern "C" void eft_slot_mode_set_imm(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 1);
}

extern "C" void eft_slot_mode_set_defer(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 0);
}

/* Maps a (mode, kind, value) triple onto the pair the slot records, then stamps it. */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 immediate) {
    u8 pair_mode = 255;
    u8 pair_value = 0;

    switch (mode) {
    case 1:
        if (kind == 2) {
            pair_mode = 0;
            pair_value = value;
        }
        break;
    case 2:
        pair_mode = 1;
        pair_value = value;
        break;
    case 3:
        if (kind == 2) {
            pair_mode = 2;
            pair_value = value;
        }
        break;
    }
    if (immediate == 0) {
        eft_slot_mode_set_imm(slot, pair_mode, pair_value);
        return;
    }
    eft_slot_mode_set_defer(slot, pair_mode, pair_value);
}

/* Stamps a (kind, a, b) triple on the slot, after the enemy work's record rewrites it. */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 immediate) {
    u8 first = kind;
    u8 second = a;
    u8 third = b;

    if (kind <= 4) {
        fn_8012A9E8(work, &first, &second, &third);
        eft_slot_mode_set_map(slot, first, second, third, immediate);
        return;
    }
    if (immediate == 0) {
        eft_slot_mode_set_imm(slot, 0, 0xFF);
        return;
    }
    eft_slot_mode_set_defer(slot, 0, 0xFF);
}

/* Writes the centre of a shape block into `out`: a sphere's centre, or a capsule's midpoint. */
extern "C" void eft_shape_block_center(EftShapeBlock* block, VEC3* out)
{
    VEC3 mid;
    VEC3 a;
    VEC3 b;

    VEC3_ctor(&mid);
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    if (block->kind_0x00 == 0) {
        setVector3(out, block->a_0x04.x, block->a_0x04.y, block->a_0x04.z);
    } else {
        setVector3(&a, block->a_0x04.x, block->a_0x04.y, block->a_0x04.z);
        setVector3(&b, block->b_0x10.x, block->b_0x10.y, block->b_0x10.z);
        copyVec3(out, &VEC3_SCALE(&VEC3_ADD(&a, &b), 0.5f));
    }
}

/* Walks the definition's per-motion table down to the 0x20-byte block the slot's effect key selects. */
extern "C" EftShapeBlock* eft_def_model_block(EftSlot* slot, u8 index) {
    EftDef* def;
    EftEntry* entry;
    EftEntry* model;

    if (index == 0xFF) {
        return NULL;
    }
    def = eft_def_get(slot);
    if (def == NULL) {
        return NULL;
    }
    entry = def->entries_0x04;
    if (entry == NULL) {
        return NULL;
    }
    while (entry->key_0x00 != 0xFF) {
        if (fn_80125FF0(slot->field_0x13, entry->key_0x00) == 1) {
            break;
        }
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return NULL;
    }
    model = entry->sub_0x04;
    while (model->key_0x00 != 0xFF) {
        if (model->key_0x00 == slot->field_0x14) {
            break;
        }
        model++;
    }
    if (model->key_0x00 == 0xFF) {
        return NULL;
    }
    return (EftShapeBlock*)model->sub_0x04 + index;
}

/* Returns the effect kind the slot's definition gives its key pair in the current map kind (the second table's map
 * entry, its +0x14 key, then its +0x15 pair), 0 when either level has none. */
extern "C" u8 eft_slot_effect_kind_get(EftSlot* slot)
{
    EftDef* def;
    EftEntry* entry;
    EftEntry* sub;
    EftPair* pair;

    def = eft_def_get(slot);
    if (def == NULL) {
        return 0;
    }
    entry = def->entries_0x10;
    if (entry == NULL) {
        return 0;
    }
    while (entry->key_0x00 != 0xFF) {
        if (fn_80125FF0(slot->field_0x13, entry->key_0x00) == 1) {
            break;
        }
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return 0;
    }
    sub = entry->sub_0x04;
    while (sub->key_0x00 != 0xFF) {
        if (sub->key_0x00 == slot->field_0x14) {
            break;
        }
        sub++;
    }
    if (sub->key_0x00 == 0xFF) {
        return 0;
    }
    pair = (EftPair*)sub->sub_0x04;
    while (pair->key_0x00 != 0xFF) {
        if (pair->key_0x00 == slot->field_0x15) {
            break;
        }
        pair++;
    }
    return pair->value_0x01;
}

/* Returns shape block `index` of the slot's definition for key `key` in the current map kind, NULL when none. */
extern "C" EftShapeBlock* eft_slot_shape_block_get(EftSlot* slot, u8 key, u8 index)
{
    EftDef* def;
    EftEntry* entry;
    EftEntry* model;

    if (index == 0xFF) {
        return NULL;
    }
    def = eft_def_get(slot);
    if (def == NULL) {
        return NULL;
    }
    entry = def->entries_0x04;
    if (entry == NULL) {
        return NULL;
    }
    while (entry->key_0x00 != 0xFF) {
        if (fn_80125FF0(slot->field_0x13, entry->key_0x00) == 1) {
            break;
        }
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return NULL;
    }
    model = entry->sub_0x04;
    while (model->key_0x00 != 0xFF) {
        if (model->key_0x00 == key) {
            break;
        }
        model++;
    }
    if (model->key_0x00 == 0xFF) {
        return NULL;
    }
    return (EftShapeBlock*)model->sub_0x04 + index;
}

/* Moves the slot's hit shape to block `index` (its centre and radius), or to the origin with a 1500 radius when the
 * definition has none. */
extern "C" void eft_slot_shape_move(EftSlot* slot, u8 index)
{
    EftShapeBlock* block = eft_def_model_block(slot, index);
    VEC3 zero;

    if (block != NULL) {
        eft_shape_block_center(block, &slot->pos_0x24);
        slot->field_0x30 = block->radius_0x1C;
        slot->field_0x36 = index;
    } else {
        copyVec3(&slot->pos_0x24, setVec3(&zero, 0.0f, 0.0f, 0.0f));
        slot->field_0x30 = 1500.0f;
        slot->field_0x36 = 0xFF;
    }
}

/* Moves the slot's hit shape to the block its key pair names, when it names one. */
extern "C" void fn_80345A2C(EftSlot* slot)
{
    u8 index = eft_slot_effect_kind_get(slot);

    if (index != 0xFF) {
        eft_slot_shape_move(slot, index);
    }
}

/* Whether `pos` lies within `margin` of the slot's shape block `index` (a sphere or a capsule), or of the slot's own
 * sphere when the block is missing. */
extern "C" u32 eft_slot_shape_hit_ck(EftSlot* slot, u8 index, VEC3* pos, f32 margin)
{
    EftShapeBlock* block = NULL;
    s32 capsule = 0;
    PlCapsuleBox box;
    VEC3 a;
    VEC3 b;

    eft_shape_init((EftShape*)&box);
    VEC3_ctor(&a);
    VEC3_ctor(&b);
    if (index != 0xFF) {
        block = eft_def_model_block(slot, index);
        if (block != NULL && block->kind_0x00 == 1) {
            capsule = 1;
        }
    }
    if (block != NULL) {
        if (capsule == 0) {
            setVector3(&a, block->a_0x04.x, block->a_0x04.y, block->a_0x04.z);
            if (vec3_dist_sq(&a, pos) <= (block->radius_0x1C + margin) * (block->radius_0x1C + margin)) {
                return 1;
            }
        } else {
            setVector3(&a, block->a_0x04.x, block->a_0x04.y, block->a_0x04.z);
            setVector3(&b, block->b_0x10.x, block->b_0x10.y, block->b_0x10.z);
            capsule_box_from_points(&a, &b, &box, block->radius_0x1C + margin);
            pl_capsule_box_set((PlCapsule*)&box, &box);
            if (pl_capsule_point_ck(pos, &box) == 1) {
                return 1;
            }
        }
    } else {
        if (vec3_dist_sq(&slot->pos_0x24, pos) <= slot->field_0x30 * slot->field_0x30) {
            return 1;
        }
    }
    return 0;
}

/* Whether a live enemy of the slot's area (with its bit-0 flag set) stands within `margin` of the slot's shape. */
extern "C" u32 eft_slot_enemy_near_ck(EftSlot* slot, u8 index, f32 margin)
{
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 i;

    for (i = 0; i < max; i++, work++) {
        if (em_work_die_ck(work) != 1 && slot->field_0x14 == work->area_no && (work->field_0x1C8 & 1) != 0 &&
            eft_slot_shape_hit_ck(slot, index, &work->pos, margin) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Returns the slot's third-table entry for key `key` in the current map kind, NULL when none. */
extern "C" EftEntry* eft_slot_entry3_get(EftSlot* slot, u8 key)
{
    EftDef* def;
    EftEntry* entry;
    EftEntry* sub;

    def = eft_def_get(slot);
    if (def == NULL) {
        return NULL;
    }
    entry = def->entries_0x08;
    if (entry == NULL) {
        return NULL;
    }
    while (entry->key_0x00 != 0xFF) {
        if (fn_80125FF0(slot->field_0x13, entry->key_0x00) == 1) {
            break;
        }
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return NULL;
    }
    sub = entry->sub_0x04;
    while (sub->key_0x00 != 0xFF) {
        if (sub->key_0x00 == key) {
            break;
        }
        sub++;
    }
    if (sub->key_0x00 == 0xFF) {
        return NULL;
    }
    return sub;
}

/* Binds the slot to the live enemy of its definition's team: the one its +0x01 key names (unless it is in action
 * 11), else one of its area in the flagged state (for keys 2 and 8 the biggest). */
extern "C" void eft_slot_enemy_bind(EftSlot* slot)
{
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 max = get_move_work_max(3);
    EftDef* def;
    s32 i;
    f32 scale;

    slot->field_0x17 = 0xFF;
    slot->field_0x18 = 0;
    def = eft_def_get(slot);
    if (def == NULL) {
        return;
    }
    for (i = 0; i < (u16)max; i++, work++) {
        if (em_work_die_ck(work) == 1 || work->team != def->team_0x01) {
            continue;
        }
        if (slot->key_0x01 == work->field_0x46C) {
            if (work->action == 11) {
                return;
            }
            slot->field_0x17 = work->field_0x46C;
            slot->field_0x18 = (u32)work;
            return;
        }
        if (work->area_no != slot->field_0x14 || work->field_0x43D != 1) {
            continue;
        }
        if (slot->key_0x00 == 2 || slot->key_0x00 == 8) {
            if (slot->field_0x17 == 0xFF) {
                slot->field_0x17 = work->field_0x46C;
                slot->field_0x18 = (u32)work;
            } else {
                scale = get_em_scale((_ENEMY_WORK*)slot->field_0x18);
                if (get_em_scale(work) > scale) {
                    slot->field_0x17 = work->field_0x46C;
                    slot->field_0x18 = (u32)work;
                }
            }
        } else {
            slot->field_0x17 = work->field_0x46C;
            slot->field_0x18 = (u32)work;
            return;
        }
    }
}

/* Whether `value` is listed in the slot's third-table entry for key `key`. */
extern "C" u32 eft_slot_entry3_has(EftSlot* slot, u8 key, u8 value)
{
    EftEntry* entry = eft_slot_entry3_get(slot, key);
    u8* list;

    if (entry != NULL) {
        list = (u8*)entry->sub_0x04;
        while (*list != 0xFF) {
            if (value == *list) {
                return 1;
            }
            list++;
        }
    }
    return 0;
}

/* Rates area `area` for the slot: 0 when a live enemy (not of the definition's team, carrying its flag mask) stands
 * in it, 1 when one stands in an area the third table lists for it, else 2; `strict` skips team-3 enemies the em003
 * test excludes. */
extern "C" u8 eft_slot_area_rate(EftSlot* slot, u8 area, u8 strict)
{
    u8 result = 2;
    EftDef* def = eft_def_get(slot);
    _ENEMY_WORK* work;
    u16 max;
    s32 i;

    if (def != NULL && def->mask_0x02 != 0) {
        work = (_ENEMY_WORK*)get_move_work_adrs(3);
        max = get_move_work_max(3);
        for (i = 0; i < max; i++, work++) {
            if (em_work_die_ck(work) == 1 || work->action == 11) {
                continue;
            }
            if (strict == 1 && work->team == 3 && em003_skip_ck(work) == 1) {
                continue;
            }
            if ((def->mask_0x02 & work->field_0x1C8) != def->mask_0x02 || work->team == def->team_0x01) {
                continue;
            }
            if (area == work->area_no) {
                result = 0;
                break;
            }
            if (eft_slot_entry3_has(slot, area, work->area_no) == 1) {
                result = 1;
            }
        }
    }
    return result;
}

/* Returns the slot's fourth-table entry for key `key` in the current map kind, NULL when none. */
extern "C" EftEntry* eft_slot_entry4_get(EftSlot* slot, u8 map, u8 key)
{
    EftDef* def;
    EftEntry* entry;
    EftEntry* sub;

    def = eft_def_get(slot);
    if (def == NULL) {
        return NULL;
    }
    entry = def->entries_0x0C;
    if (entry == NULL) {
        return NULL;
    }
    while (entry->key_0x00 != 0xFF) {
        if (fn_80125FF0(slot->field_0x13, entry->key_0x00) == 1) {
            break;
        }
        entry++;
    }
    if (entry->key_0x00 == 0xFF) {
        return NULL;
    }
    sub = entry->sub_0x04;
    while (sub->key_0x00 != 0xFF) {
        if (sub->key_0x00 == key) {
            break;
        }
        sub++;
    }
    if (sub->key_0x00 == 0xFF) {
        return NULL;
    }
    return sub;
}

/* Returns the first area of the slot's flee list no enemy holds (`eft_slot_area_rate`), else the list's first area; 255
 * without a list. */
extern "C" u8 eft_slot_flee_area_get(EftSlot* slot)
{
    EftEntry* entry = eft_slot_entry4_get(slot, get_now_mapno(), slot->field_0x14);
    u8* list;
    s32 i;
    u8* area;

    if (entry == NULL) {
        return 0xFF;
    }
    list = (u8*)entry->sub_0x04;
    i = 0;
    area = list;
    while (*area != 0xFF) {
        if (eft_slot_area_rate(slot, *area, 0) != 0) {
            return list[i];
        }
        i++;
        area++;
    }
    return list[0];
}

/* Records the marks a received message carries (`a` at +0x3A, `b` at +0x3B) when they are new for the slot's area
 * or raise its mark; 1 when they changed. */
extern "C" u32 eft_slot_marks_set(EftSlot* slot, u8 a, u8 b)
{
    if (slot->field_0x14 == b) {
        if (slot->field_0x3B != b) {
            slot->field_0x3A = a;
            slot->field_0x3B = b;
            return 1;
        }
        if (slot->field_0x3A < a) {
            slot->field_0x3A = a;
            return 1;
        }
    } else if (slot->field_0x14 != slot->field_0x3B && slot->field_0x3A < a) {
        slot->field_0x3A = a;
        slot->field_0x3B = b;
        return 1;
    }
    return 0;
}

/* Returns the entry of `list` for the current map kind and the slot's area, NULL when none. */
extern "C" EftEntry* eft_slot_area_entry_get(EftSlot* slot, EftEntry* list)
{
    EftEntry* sub;

    if (list == NULL) {
        return NULL;
    }
    while (list->key_0x00 != 0xFF) {
        if (fn_80125FF0(slot->field_0x13, list->key_0x00) == 1) {
            break;
        }
        list++;
    }
    if (list->key_0x00 == 0xFF) {
        return NULL;
    }
    sub = list->sub_0x04;
    while (sub->key_0x00 != 0xFF) {
        if (sub->key_0x00 == slot->field_0x14) {
            break;
        }
        sub++;
    }
    if (sub->key_0x00 == 0xFF) {
        return NULL;
    }
    return sub;
}

/* Rolls one value of the slot's weighted table (row `row`, key `key` of `list`): a percent roll walks the weights;
 * 255 without a table or past the weights. */
extern "C" u8 eft_slot_weight_roll(EftSlot* slot, u8 row, u8 key, EftEntry* list)
{
    EftPair* rolls = NULL;
    u8 result = 0xFF;
    EftEntry* entry = eft_slot_area_entry_get(slot, list);
    EftEntry* sub;
    u16 roll;
    u16 sum;

    if (entry == NULL) {
        return 0xFF;
    }
    sub = ((EftEntry**)entry->sub_0x04)[row];
    if (sub == NULL) {
        return 0xFF;
    }
    while (sub->key_0x00 != 0xFF) {
        if (sub->key_0x00 == key) {
            rolls = (EftPair*)sub->sub_0x04;
            break;
        }
        sub++;
    }
    if (rolls == NULL) {
        return 0xFF;
    }
    roll = (u16)ran_suu(0) % 100;
    sum = 0;
    while (rolls->key_0x00 != 0xFF) {
        sum += rolls->value_0x01;
        if (roll < sum) {
            result = rolls->key_0x00;
            break;
        }
        rolls++;
    }
    return result;
}

/* Sets the slot's quest grade: 1 up to grade 3 of the quest's stat 0x20, else 2. */
extern "C" void eft_slot_grade_set(EftSlot* slot)
{
    if (quest_em_stat_tbl_get(0x20) <= 3) {
        slot->field_0x39 = 1;
    } else {
        slot->field_0x39 = 2;
    }
}

/* Whether the slot's bound enemy still counts: its own key always does; in map kind 1 area 2 and map kind 4 areas
 * 5/6 an enemy in the listed stage does not. */
extern "C" u32 eft_slot_enemy_counts_ck(EftSlot* slot)
{
    if (slot->key_0x01 == slot->field_0x17) {
        return 1;
    }
    switch (stage_map_kind_get(slot->field_0x13)) {
    case 1:
        if (slot->field_0x14 == 2 && em_phase_get((_ENEMY_WORK*)slot->field_0x18) == 1) {
            return 0;
        }
        break;
    case 4:
        switch (slot->field_0x14) {
        case 5:
            if (em_phase_get((_ENEMY_WORK*)slot->field_0x18) == 3) {
                return 0;
            }
            break;
        case 6:
            if (em_phase_get((_ENEMY_WORK*)slot->field_0x18) == 1) {
                return 0;
            }
            break;
        }
        break;
    }
    return 1;
}

#pragma push
#pragma optimization_level 4
/* One frame of the slot's tracking: follows its bound enemy (states 4/5/1/0 from its motion and flags) or rates its
 * area (2 when held, 1 when its own enemy or a matching record is found), requests the new state, then steps the
 * state's timers and the bound enemy's mode. */
extern "C" void eft_slot_track_step(EftSlot* slot)
{
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* bound = (_ENEMY_WORK*)slot->field_0x18;
    _ENEMY_WORK* target = NULL;
    u8 index = 0;
    u8 state;
    u8 shape;
    s32 i;

    if (bound != NULL) {
        eft_slot_kind_set(slot, bound->area_no, 0);
        target = bound;
        if (em_mot_finished_ck(bound) != 0 && bound->field_0x1F9 == 1 && bound->field_0x43D == 1) {
            state = 4;
        } else if (bound->field_0x1F9 == 1) {
            state = 5;
        } else if (bound->field_0x43D == 1) {
            state = 1;
            slot->field_0x20 = 0;
        } else {
            state = 0;
        }
        if (slot->key_0x01 != slot->field_0x17 && (state == 5 || eft_slot_enemy_counts_ck(slot) == 0)) {
            slot->field_0x17 = 0xFF;
            slot->field_0x18 = 0;
            bound = NULL;
            target = NULL;
        } else {
            copyVec3(&slot->pos_0x24, &bound->pos);
            slot->field_0x30 = 1200.0f;
            slot->field_0x36 = 0xFF;
        }
    }
    if (bound == NULL) {
        shape = 0;
        if (stage_map_kind_get(slot->field_0x13) == 2 && slot->field_0x14 == 4 && stg_area_flag_ck(4, 1) == 1) {
            shape = 1;
        }
        eft_slot_shape_move(slot, shape);
        state = 0;
        if (eft_slot_area_rate(slot, slot->field_0x14, 1) == 0) {
            state = 2;
        } else {
            for (i = 0; i < max; i++, work++) {
                if (em_work_die_ck(work) != 1 && work->action != 11 &&
                    slot->key_0x00 == enemy_data_grp(work->team, work->field_0x00A) && state != 1 &&
                    slot->key_0x01 == work->field_0x46C && work->field_0x43D == 1) {
                    state = 1;
                    target = work;
                }
            }
            if (slot->field_0x05 == 0) {
                if (slot->field_0x08 != 1 && state != 1 && eft_slot_area_ck(slot) == 1) {
                    index = 1;
                    state = 1;
                }
            } else if (state != 1 && eft_slot_area_ck(slot) == 1) {
                index = 1;
                state = 1;
            }
        }
    }
    if (slot->field_0x08 != state) {
        eft_slot_state_request(slot, state, target, index);
    }
    switch (slot->field_0x08) {
    case 4:
    case 5:
        slot->field_0x22++;
        break;
    case 1:
        if (bound == NULL) {
            if (slot->field_0x1C > 90 && target != NULL) {
                if (slot->field_0x05 == 0 || eft_slot_armed_ck(slot) == 1) {
                    eft_slot_mode_set_work(slot, target, 4, 0, 0, 0);
                } else {
                    eft_slot_mode_set_work(slot, target, 4, 0, 0, 1);
                }
            }
            slot->field_0x0B = 0;
            slot->field_0x22 = 0;
            break;
        }
        eft_slot_mode_set_map(slot, target->field_0x380, target->state_0x381, target->field_0x382, 0);
        switch (slot->field_0x0B) {
        case 0:
        case 2:
        case 3:
            if (slot->field_0x0F == 0xFF) {
                slot->field_0x0B = 3;
            } else if (em_action_mid_ck(bound) == 1) {
                slot->field_0x0B = 1;
                slot->field_0x22 = 900;
            } else if (em_alt_mode_ck(bound) == 1) {
                slot->field_0x0B = 2;
                slot->field_0x22 = 0;
            } else {
                slot->field_0x0B = 0;
                slot->field_0x22 = 0;
            }
            break;
        case 1:
            if (--slot->field_0x22 <= 0) {
                slot->field_0x0B = 0;
                slot->field_0x22 = 0;
            }
            break;
        }
        break;
    }
}
#pragma pop

#pragma push
#pragma optimization_level 4
/* Re-arms the slot's mode from its bound enemy: states 4/5 take the enemy's area byte, state 1 its work triple. */
extern "C" void eft_slot_mode_rearm(EftSlot* slot, u8 previous, _ENEMY_WORK* work)
{
    switch (slot->field_0x08) {
    case 4:
    case 5:
        if (work != NULL) {
            eft_slot_mode_set_defer(slot, 3, work->field_0x9F7);
        }
        return;
    case 1:
        if (work != NULL) {
            eft_slot_mode_set_work(slot, work, 4, 0, 0, 0);
        }
        return;
    }
}
#pragma pop

/* Moves the slot's hit shape to the block its key pair names (the definition's arm). */
extern "C" void eft_slot_key_shape_move(EftSlot* slot)
{
    fn_80345A2C(slot);
}

/* Returns the slot's escape timer: 6300 frames, 3600 in map kind 1's area 1. */
extern "C" s32 eft_slot_escape_time_get(EftSlot* slot)
{
    s32 frames = 6300;

    if (stage_map_kind_get(slot->field_0x13) == 1 && slot->field_0x14 == 1) {
        frames = 3600;
    }
    return frames;
}

#pragma push
#pragma optimization_level 4
/* One frame of an unbound slot: gathers the enemies and records it tracks, follows a mode-3 area move, rates its area
 * (held, threatened, idle or gone), requests the new state and, after 600 frames of state 3, picks the area to flee
 * to. */
extern "C" void eft_slot_unbound_step(EftSlot* slot)
{
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
    u8 here = 0;
    u8 flagged = 0;
    _ENEMY_WORK* works[32];
    EftTargetRecord* records[32];
    _ENEMY_WORK* work;
    u16 max;
    u8 nw;
    u8 nr;
    s32 i;
    u8 all_here;
    u8 state;
    u8 force;
    u8 found;
    u8 key;
    u8 pick;
    u8 area;
    s16 frames;

    for (i = 0; i < 32; i++) {
        works[i] = NULL;
    }
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    nw = 0;
    nr = 0;
    for (i = 0; i < max; i++, work++) {
        if (eft_work_match_ck(slot, work) == 1) {
            works[nw] = work;
            nw++;
            if (work->area_no == slot->field_0x14) {
                here++;
            }
            if (work->field_0x010 == 1) {
                flagged = 1;
            }
        }
    }
    for (i = 0; i < 0x80; i++, record++) {
        if (eft_target_match_ck(slot, record) == 1) {
            records[nr] = record;
            nr++;
            if (record->index_0x0A == slot->field_0x14) {
                here++;
            }
            if (record->flag_0x0F == 1) {
                flagged = 1;
            }
        }
    }
    all_here = nw + nr <= here;
    state = slot->field_0x08;
    force = 0;
    found = 0;
    if (slot->field_0x0F == 3) {
        key = slot->field_0x10;
        for (i = 0; i < nw; i++) {
            if (key == works[i]->area_no) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            for (i = 0; i < nr; i++) {
                if (key == records[i]->index_0x0A) {
                    found = 1;
                    break;
                }
            }
        }
        if (found == 1) {
            eft_slot_kind_set(slot, key, 0);
            state = 0;
            force = 1;
        }
    }
    switch (slot->field_0x08) {
    case 0:
    case 2:
        if (slot->field_0x3B == slot->field_0x14 && (u8)(slot->field_0x3A - 1) <= 1) {
            state = 4;
        }
        if (state != 4) {
            switch (eft_slot_area_rate(slot, slot->field_0x14, 0)) {
            case 0:
                state = 3;
                break;
            case 1:
                state = 2;
                break;
            case 2:
                state = 0;
                break;
            }
        }
        if ((state == 0 || state == 2) && all_here == 1 && slot->field_0x34 > (s16)eft_slot_escape_time_get(slot) && flagged != 1) {
            state = 5;
        }
        break;
    case 5:
        if (slot->field_0x3B == slot->field_0x14 && (u8)(slot->field_0x3A - 1) <= 1) {
            state = 4;
        }
        if (state != 4 && eft_slot_area_rate(slot, slot->field_0x14, 0) == 0) {
            state = 4;
        }
        break;
    case 3:
        if (--slot->field_0x20 <= 0) {
            state = 4;
        }
        break;
    }
    if (force != 0 || slot->field_0x08 != state) {
        eft_slot_state_request(slot, state, NULL, 0);
    }
    switch (slot->field_0x08) {
    case 0:
    case 2:
    case 5:
        if (slot->field_0x3B != slot->field_0x14 && (u8)(slot->field_0x3A - 1) <= 1) {
            slot->field_0x0B = 2;
        }
        break;
    case 4:
        slot->field_0x22++;
        break;
    case 3:
        frames = ++slot->field_0x22;
        if (slot->field_0x0A == 0 && frames >= 600 && eft_slot_armed_ck(slot) == 1 && slot->field_0x0F != 3) {
            slot->field_0x0A++;
            pick = 0;
            switch (stage_map_kind_get(slot->field_0x13)) {
            case 1:
                if (slot->field_0x14 == 2) {
                    pick = 1;
                }
                break;
            case 2:
                if (slot->field_0x14 == 2) {
                    pick = 1;
                }
                break;
            case 5:
                if (slot->field_0x14 == 3) {
                    pick = 1;
                }
                break;
            }
            if (pick == 1) {
                if (nw != 0) {
                    area = em_nearest_area_get(get_enemy_data(works[0]), &slot->pos_0x24, slot->field_0x14);
                } else if (nr != 0) {
                    area = em_nearest_area_get(enemy_data_by_team_get(records[0]->team_0x02, records[0]->field_0x03), &slot->pos_0x24,
                                       slot->field_0x14);
                } else {
                    area = eft_slot_flee_area_get(slot);
                }
            } else {
                area = eft_slot_flee_area_get(slot);
            }
            eft_slot_mode_set_imm(slot, 3, area);
        }
        break;
    }
}
#pragma pop

/* The definitions' state-change handler: state 3 arms its 900-frame countdown, states 4 and 5 pick the area to flee
 * to (5 keeps the alert mode when the previous state was not 0). */
extern "C" void eft_slot_state_change(EftSlot* slot, u8 previous, _ENEMY_WORK* work)
{
    switch (slot->field_0x08) {
    case 3:
        slot->field_0x20 = 900;
        break;
    case 4:
        if (slot->field_0x3A == 2) {
            slot->field_0x0B = 1;
        } else {
            slot->field_0x0B = 0;
        }
        if (slot->field_0x0F != 3) {
            eft_slot_mode_set_defer(slot, 3, eft_slot_flee_area_get(slot));
        }
        break;
    case 5:
        if (previous == 0) {
            slot->field_0x0B = 0;
        } else {
            slot->field_0x0B = 1;
        }
        eft_slot_mode_set_defer(slot, 3, eft_slot_flee_area_get(slot));
        break;
    }
}

/* Binds the slot to the first enemy of its area its wider test accepts (when it is armed or its net timer ran out),
 * and resends it once the timer runs out. */
extern "C" void eft_slot_area_bind(EftSlot* slot)
{
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 max = get_move_work_max(3);
    s32 i;

    if (slot->field_0x1E >= 60) {
        slot->field_0x1E = -1;
    }
    slot->field_0x18 = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (slot->field_0x14 == work->area_no && eft_work_wide_ck(slot, work) == 1) {
            if (eft_slot_armed_ck(slot) == 1 || slot->field_0x1E < 0) {
                slot->field_0x18 = (u32)work;
                copyVec3(&slot->pos_0x24, &work->pos);
            }
            break;
        }
    }
    if (eft_slot_armed_ck(slot) == 1 && slot->field_0x1E < 0) {
        slot->field_0x1E = 0;
        eft_net_send(slot, 4, 0);
    }
}

/* Moves the slot's hit shape to block 3 and requests the matching mode. */
extern "C" void eft_slot_shape_block3_set(EftSlot* slot)
{
    slot->field_0x36 = 3;
    slot->field_0x22 = 0;
    eft_slot_shape_move(slot, 3);
    eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
}

/* Writes the two link marks of the slot's area: map kind 1 areas 7 and 12, map kind 3 areas 2 and 8; 0 elsewhere. */
extern "C" void eft_slot_link_marks_get(EftSlot* slot, u8* a, u8* b)
{
    switch (stage_map_kind_get(slot->field_0x13)) {
    case 1:
        switch (slot->field_0x14) {
        case 7:
            *a = 4;
            *b = 3;
            break;
        case 12:
            *a = 3;
            *b = 4;
            break;
        default:
            *a = 0;
            *b = 0;
            break;
        }
        break;
    case 3:
        switch (slot->field_0x14) {
        case 2:
            *a = 2;
            *b = 2;
            break;
        case 8:
            *a = 2;
            *b = 2;
            break;
        default:
            *a = 0;
            *b = 0;
            break;
        }
        break;
    default:
        *a = 0;
        *b = 0;
        break;
    }
}

/* One frame of a linked slot (the areas `eft_slot_link_marks_get` marks): binds the enemy of its area, rates it (0 idle, 2 held,
 * 3 crowded), requests the state, then walks the enemy along the linked shape blocks as it reaches each. */
extern "C" void eft_slot_linked_step(EftSlot* slot)
{
    _ENEMY_WORK* work;
    u16 max;
    u8 i;
    u8 count;
    u8 base;
    u8 state;
    u8 index;
    u8 n;
    EftShapeBlock* block;

    slot->field_0x18 = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (em_work_die_ck(work) != 1 && slot->field_0x14 == work->area_no && eft_work_wide_ck(slot, work) == 1) {
            slot->field_0x18 = (u32)work;
            break;
        }
    }
    if (slot->field_0x18 == 0) {
        return;
    }
    eft_slot_link_marks_get(slot, &count, &base);
    if (count == 0) {
        return;
    }
    if (base == 0) {
        return;
    }
    state = slot->field_0x08;
    switch (state) {
    case 0:
        if (eft_slot_area_rate(slot, slot->field_0x14, 0) == 0) {
            state = 2;
        } else {
            state = 0;
        }
        break;
    case 2:
        if (eft_slot_area_rate(slot, slot->field_0x14, 0) == 0) {
            if (eft_slot_enemy_near_ck(slot, slot->field_0x36, 600.0f) != 0) {
                state = 3;
            }
        } else {
            state = 0;
        }
        break;
    case 3:
        if (slot->field_0x22 >= 300) {
            index = slot->field_0x36;
            for (n = 0; n < count; n++) {
                if (++index >= count) {
                    index = 0;
                }
                if (eft_slot_enemy_near_ck(slot, index, 600.0f) == 0) {
                    break;
                }
            }
            if (n < count) {
                state = 2;
            }
        }
        break;
    }
    if (slot->field_0x08 != state) {
        eft_slot_state_request(slot, state, (_ENEMY_WORK*)slot->field_0x18, 0);
    }
    switch (slot->field_0x08) {
    case 0:
        if (slot->field_0x3A == 1 && slot->field_0x0B == 0) {
            slot->field_0x0B = 1;
            slot->field_0x22 = 1800;
        }
        if (slot->field_0x0A == 0) {
            if (slot->field_0x1C > 1800 ||
                eft_slot_shape_hit_ck(slot, slot->field_0x36, &((_ENEMY_WORK*)slot->field_0x18)->pos, 0.0f) != 0) {
                slot->field_0x0A++;
                slot->field_0x22 = 0;
                slot->field_0x3A = 0;
                slot->field_0x0B = 0;
            }
        } else if (slot->field_0x22 >= 1800) {
            if (eft_slot_armed_ck(slot) == 1) {
                slot->field_0x0A = 0;
                slot->field_0x22 = 0;
                if (++slot->field_0x36 >= base + count) {
                    slot->field_0x36 = count;
                }
                eft_slot_shape_move(slot, slot->field_0x36);
                eft_slot_mode_set_imm(slot, 4, slot->field_0x36);
            } else {
                slot->field_0x0A = 2;
                if (slot->field_0x10 != slot->field_0x36) {
                    slot->field_0x0A = 0;
                    slot->field_0x22 = 0;
                }
            }
        } else {
            slot->field_0x22++;
        }
        if (slot->field_0x0F == 4) {
            eft_slot_shape_move(slot, slot->field_0x10);
        }
        break;
    case 2:
        if (slot->field_0x3A == 1 && slot->field_0x0B == 0) {
            if (eft_slot_armed_ck(slot) == 1) {
                slot->field_0x0B = 1;
                index = slot->field_0x36;
                for (n = 0; n < count; n++) {
                    if (++index >= count) {
                        index = 0;
                    }
                    if (eft_slot_enemy_near_ck(slot, index, 600.0f) == 0) {
                        block = eft_def_model_block(slot, index);
                        eft_shape_block_center(block, &slot->pos_0x24);
                        slot->field_0x30 = block->radius_0x1C;
                        break;
                    }
                }
                slot->field_0x36 = index;
                eft_slot_shape_move(slot, index);
                eft_slot_mode_set_imm(slot, 4, index);
            } else {
                slot->field_0x0A = 2;
                if (slot->field_0x10 != slot->field_0x36) {
                    slot->field_0x0B = 1;
                    slot->field_0x0A = 0;
                }
            }
        } else if (slot->field_0x0B == 1 &&
                   (slot->field_0x1C > 1800 ||
                    eft_slot_shape_hit_ck(slot, slot->field_0x36, &((_ENEMY_WORK*)slot->field_0x18)->pos, 0.0f) != 0)) {
            slot->field_0x22 = 0;
            slot->field_0x3A = 0;
            slot->field_0x0B = 0;
        }
        if (slot->field_0x0F == 4) {
            eft_slot_shape_move(slot, slot->field_0x10);
        }
        break;
    case 3:
        if (slot->field_0x22 < 300) {
            slot->field_0x22++;
        }
        break;
    }
}

/* The linked slots' state-change handler: state 0 moves the hit shape to the linked block nearest the enemy, state 2
 * to the nearest free one (or, after state 0, to the next free one round the ring). */
extern "C" void eft_slot_linked_state_change(EftSlot* slot, u8 previous, _ENEMY_WORK* work)
{
    VEC3 center;
    VEC3 best_pos;
    u8 count;
    u8 base;
    u8 i;
    u8 index;
    u8 take;
    u8 free;
    u8 best_free;
    f32 best;
    f32 dist;
    EftShapeBlock* block;

    VEC3_ctor(&center);
    VEC3_ctor(&best_pos);
    slot->field_0x0B = 0;
    eft_slot_link_marks_get(slot, &count, &base);
    i = count;
    if (i == 0) {
        return;
    }
    if (base == 0) {
        return;
    }
    switch (slot->field_0x08) {
    case 0:
        if (work != NULL) {
            index = 0xFF;
            for (; i < count + base; i++) {
                eft_shape_block_center(eft_def_model_block(slot, i), &center);
                dist = vec3_distance(&work->pos, &center);
                if (i == count) {
                    best = dist;
                    index = i;
                    copyVec3(&best_pos, &center);
                } else if (best > dist) {
                    best = dist;
                    index = i;
                    copyVec3(&best_pos, &center);
                }
            }
            eft_slot_shape_move(slot, index);
            slot->field_0x36 = index;
        } else {
            index = slot->field_0x36;
        }
        eft_slot_mode_set_defer(slot, 4, index);
        break;
    case 2:
        if (previous == 0) {
            if (work != NULL) {
                best_free = 0;
                index = 0xFF;
                for (i = 0; i < count; i++) {
                    take = 0;
                    free = eft_slot_enemy_near_ck(slot, i, 600.0f) == 1;
                    eft_shape_block_center(eft_def_model_block(slot, i), &center);
                    dist = vec3_distance(&work->pos, &center);
                    if (i == 0) {
                        take = 1;
                    } else if (free == best_free) {
                        if (best > dist) {
                            take = 1;
                        }
                    } else if (free == 0) {
                        take = 1;
                    }
                    if (take == 1) {
                        best = dist;
                        index = i;
                        copyVec3(&best_pos, &center);
                        best_free = free;
                    }
                }
            } else {
                index = slot->field_0x36;
            }
        } else {
            index = slot->field_0x36;
            for (i = 0; i < count; i++) {
                if (++index >= count) {
                    index = 0;
                }
                if (eft_slot_enemy_near_ck(slot, index, 600.0f) == 0) {
                    block = eft_def_model_block(slot, index);
                    eft_shape_block_center(block, &slot->pos_0x24);
                    slot->field_0x30 = block->radius_0x1C;
                    break;
                }
            }
        }
        slot->field_0x36 = index;
        eft_slot_shape_move(slot, index);
        eft_slot_mode_set_defer(slot, 4, index);
        break;
    }
}

/* Moves the slot's hit shape to block 0, requests the matching mode and sets its grade 2. */
extern "C" void eft_slot_shape_block0_set(EftSlot* slot)
{
    slot->field_0x36 = 0;
    eft_slot_shape_move(slot, 0);
    eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
    slot->field_0x39 = 2;
}

#pragma push
#pragma optimization_level 4
/* The weighted area tables `eft_slot_roam_step`'s roaming slot rolls its next block from, in the unit's `.data` claim;
 * declared, not defined. */
extern EftEntry eft_slot_roam_tables[2];

/* One frame of a roaming slot: binds the enemy it tracks, rates its area (1 when its own enemy or a matching record
 * is in it, 5 when all of them are and the slot is old, 0 when they left), requests the state, then rolls the next
 * block when an enemy reaches the current one. */
extern "C" void eft_slot_roam_step(EftSlot* slot)
{
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
    u8 here = 0;
    _ENEMY_WORK* target = NULL;
    u8 index = 0;
    _ENEMY_WORK* works[32];
    EftTargetRecord* records[32];
    _ENEMY_WORK* work;
    u16 max;
    s32 i;
    u8 nw;
    u8 nr;
    s32 all_here;
    u8 state;
    u8 gone;
    u8 found;
    u8 key;
    u8 roll;

    slot->field_0x18 = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++, work++) {
        if (em_work_die_ck(work) != 1 && eft_work_wide_ck(slot, work) == 1) {
            target = work;
            slot->field_0x18 = (u32)work;
            break;
        }
    }
    for (i = 0; i < 32; i++) {
        works[i] = NULL;
    }
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    nw = 0;
    for (i = 0; i < max; i++, work++) {
        if (eft_work_match_ck(slot, work) == 1) {
            works[nw++] = work;
            if (work->area_no == slot->field_0x14) {
                here++;
            }
        }
    }
    nr = 0;
    for (i = 0; i < 0x80; i++, record++) {
        if (eft_target_match_ck(slot, record) == 1) {
            records[nr++] = record;
            if (record->index_0x0A == slot->field_0x14) {
                here++;
            }
        }
    }
    all_here = nw + nr <= here;
    state = slot->field_0x08;
    switch (state) {
    case 0:
        for (i = 0; i < nw; i++) {
            if (slot->field_0x14 == works[i]->area_no) {
                if (slot->field_0x0B == 0) {
                    if (works[i]->field_0x43D == 1) {
                        target = works[i];
                        state = 1;
                        break;
                    }
                } else if (slot->field_0x0B == 1 && works[i]->field_0x833 == 1 && slot->field_0x22 > 300) {
                    target = works[i];
                    state = 1;
                    break;
                }
            }
        }
        if (all_here != 0 && slot->field_0x34 > 3600 && slot->field_0x0F != 3) {
            state = 5;
        }
        if (slot->field_0x0B != 1 && state != 1 && state != 5 && eft_slot_area_ck(slot) == 1) {
            index = 1;
            state = 1;
        }
        break;
    case 1:
        if (slot->field_0x3A == 2) {
            if ((ran_suu(0) & 3) == 0) {
                state = 0;
                break;
            }
            slot->field_0x3A = 0;
        }
        gone = 1;
        for (i = 0; i < nw; i++) {
            if (works[i]->field_0x43D != 0 && slot->field_0x14 == works[i]->area_no) {
                gone = 0;
                break;
            }
        }
        if (slot->field_0x05 == 1 && eft_slot_area_ck(slot) == 1) {
            gone = 0;
        }
        if (gone == 1) {
            state = 0;
        }
        break;
    case 5:
        if (slot->field_0x0F == 3 && slot->field_0x14 == slot->field_0x10) {
            state = 0;
            target = NULL;
        }
        break;
    }
    if (slot->field_0x08 != state) {
        eft_slot_state_request(slot, state, target, index);
    }
    switch (slot->field_0x08) {
    case 0:
        if (slot->field_0x0B == 1) {
            if (slot->field_0x22 > 900) {
                slot->field_0x0B = 0;
            } else {
                slot->field_0x22++;
            }
        }
        if (slot->field_0x0F == 4) {
            eft_slot_shape_move(slot, slot->field_0x10);
        }
        if (eft_slot_armed_ck(slot) == 1 && slot->field_0x1C > 90) {
            for (i = 0; i < nw; i++) {
                if (slot->field_0x14 == works[i]->area_no &&
                    eft_slot_shape_hit_ck(slot, slot->field_0x36, &works[i]->pos, 0.0f) != 0) {
                    roll = eft_slot_weight_roll(slot, slot->field_0x10, slot->field_0x12, eft_slot_roam_tables);
                    if (roll == 0xFF) {
                        if ((slot->field_0x36 = slot->field_0x10 + 1) >= 6) {
                            slot->field_0x36 = 0;
                        }
                    } else {
                        slot->field_0x36 = roll;
                    }
                    eft_slot_shape_move(slot, slot->field_0x36);
                    eft_slot_mode_set_imm(slot, 4, slot->field_0x36);
                    break;
                }
            }
        }
        break;
    case 1:
        if (slot->field_0x1C > 90) {
            for (i = 0; i < nw; i++) {
                if (em_work_die_ck(works[i]) != 1 && works[i]->action != 11 &&
                    slot->key_0x00 == enemy_data_grp(works[i]->team, works[i]->field_0x00A) &&
                    slot->field_0x14 == works[i]->area_no && works[i]->field_0x43D == 1) {
                    if (slot->field_0x05 == 0 || eft_slot_armed_ck(slot) == 1) {
                        eft_slot_mode_set_work(slot, works[i], 0, 0, 0, 0);
                    } else {
                        eft_slot_mode_set_work(slot, works[i], 0, 0, 0, 1);
                    }
                    if (slot->field_0x10 != 0xFF) {
                        break;
                    }
                }
            }
        }
        if (eft_slot_area_rate(slot, slot->field_0x14, 0) == 0) {
            switch (slot->field_0x0A) {
            case 0:
                slot->field_0x0A++;
                slot->field_0x0B = 1;
                slot->field_0x22 = 0;
                break;
            case 1:
                if (++slot->field_0x22 > 900) {
                    slot->field_0x0A++;
                    slot->field_0x0B = 2;
                }
                break;
            }
        } else {
            slot->field_0x0A = 0;
            slot->field_0x0B = 0;
        }
        break;
    case 5:
        key = slot->field_0x10;
        if (slot->field_0x14 != key) {
            found = 0;
            if (slot->field_0x0F == 3) {
                for (i = 0; i < nw; i++) {
                    if (key == works[i]->area_no) {
                        found = 1;
                        break;
                    }
                }
                if (found == 0) {
                    for (i = 0; i < nr; i++) {
                        if (key == records[i]->index_0x0A) {
                            found = 1;
                            break;
                        }
                    }
                }
                if (found == 1) {
                    eft_slot_kind_set(slot, key, 0);
                }
            }
        }
        break;
    }
}
#pragma pop

#pragma push
#pragma optimization_level 4
/* The arena slots' state-change handler: state 0 moves the hit shape to the block nearest the enemy (keeping the
 * alert mode a mark-2 message raised), state 1 follows the enemy, state 5 flees. */
extern "C" void eft_slot_arena_state_change(EftSlot* slot, u8 previous, _ENEMY_WORK* work)
{
    VEC3 center;
    u8 mark;
    u8 index;
    u32 i;
    f32 best;
    f32 dist;

    VEC3_ctor(&center);
    mark = slot->field_0x3A;
    slot->field_0x3A = 0;
    slot->field_0x0A = 0;
    slot->field_0x22 = 0;
    slot->field_0x0B = 0;
    switch (slot->field_0x08) {
    case 0:
        if (previous == 1 && mark == 2) {
            slot->field_0x0B = 1;
        }
        if (work != NULL) {
            index = 0xFF;
            for (i = 0; i < 6; i++) {
                eft_shape_block_center(eft_def_model_block(slot, i), &center);
                dist = vec3_distance(&work->pos, &center);
                if (index == 0xFF) {
                    best = dist;
                    index = i;
                } else if (best > dist) {
                    best = dist;
                    index = i;
                }
            }
            eft_slot_shape_move(slot, index);
            slot->field_0x36 = index;
            eft_slot_mode_set_defer(slot, 4, index);
        } else {
            eft_slot_shape_move(slot, slot->field_0x36);
            eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
        }
        break;
    case 1:
        if (work != NULL) {
            eft_slot_mode_set_work(slot, work, 0, 0, 0, 0);
        } else if (slot->field_0x05 == 1) {
            eft_slot_mode_set_defer(slot, 0, 0xFF);
        }
        break;
    case 5:
        slot->field_0x0B = 0;
        eft_slot_mode_set_defer(slot, 3, eft_slot_flee_area_get(slot));
        break;
    }
}

/* Puts the slot's hit shape on block 11 and requests the matching mode. */
extern "C" void eft_slot_shape_block11_set(EftSlot* slot)
{
    slot->field_0x36 = 11;
    slot->field_0x22 = 0;
    eft_slot_shape_move(slot, 11);
    eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
    slot->field_0x0B = 0;
}

/* One frame of an arena slot: binds the enemies of its area, rolls its pattern (60 % wander, 20 % chase, 20 % rest)
 * and runs it: a random side block, the block nearest the slot until the enemy charges, a random corner until every
 * enemy rests, or a random end block. */
extern "C" void eft_slot_arena_step(EftSlot* slot)
{
    _ENEMY_WORK* bound = NULL;
    VEC3 center;
    _ENEMY_WORK* work;
    u16 max;
    u8 n;
    s32 i;
    _ENEMY_WORK* works[32];
    u8 mode;
    u16 roll;
    u8 index;
    u8 found;
    f32 best;
    f32 dist;

    VEC3_ctor(&center);
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    n = 0;
    for (i = 0; i < max; i++, work++) {
        if (em_work_die_ck(work) != 1 && eft_work_wide_ck(slot, work) != 0 && work->area_no == slot->field_0x14) {
            if (bound == NULL) {
                bound = work;
                slot->field_0x18 = (u32)work;
            }
            works[n++] = work;
        }
    }
    if (n == 0) {
        return;
    }
    if (bound == NULL) {
        return;
    }
    if (slot->field_0x08 != 0) {
        eft_slot_state_request(slot, 0, bound, 0);
    }
    if (slot->field_0x08 != 0) {
        return;
    }
    mode = slot->field_0x0B;
    if ((mode == 0 && slot->field_0x1C > 240) || (mode == 2 && slot->field_0x0A == 2) ||
        (mode == 3 && slot->field_0x1C > 600)) {
        slot->field_0x0A = 0;
        roll = (u16)ran_suu(0) % 100 + 1;
        if (roll >= 1 && roll <= 60) {
            slot->field_0x0B = 0;
        } else if (roll >= 61 && roll <= 80) {
            slot->field_0x0B = 1;
        } else if (roll >= 81 && roll <= 100) {
            slot->field_0x0B = 3;
            if (mode == 3) {
                slot->field_0x0B = 0;
            }
        }
    }
    switch (slot->field_0x0B) {
    case 0:
        if (slot->field_0x0A == 0) {
            slot->field_0x0A++;
            index = (u16)ran_suu(1) % 3 + 4;
            if (slot->field_0x24 < 0.0f) {
                index += 3;
            }
            slot->field_0x36 = index;
            eft_slot_shape_move(slot, index);
            eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
        }
        break;
    case 1:
        switch (slot->field_0x0A) {
        case 0:
            slot->field_0x0A++;
            index = 0xFF;
            for (i = 0; i < 4; i++) {
                eft_shape_block_center(eft_def_model_block(slot, i), &center);
                dist = vec3_dist_sq(&slot->pos_0x24, &center);
                if (index == 0xFF) {
                    best = dist;
                    index = i;
                } else if (best > dist) {
                    best = dist;
                    index = i;
                }
            }
            slot->field_0x36 = index;
            eft_slot_shape_move(slot, index);
            eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
            break;
        case 1:
            found = 0;
            for (i = 0; i < n; i++) {
                if (works[i]->field_0x1E2 == 0 && works[i]->field_0x43C == 1) {
                    found = 1;
                    break;
                }
            }
            if (found == 1) {
                slot->field_0x0A++;
                slot->field_0x22 = 0;
            }
            break;
        case 2:
            found = 1;
            for (i = 0; i < n; i++) {
                if (works[i]->field_0x1E2 != 0 || works[i]->field_0x43C != 1) {
                    found = 0;
                    break;
                }
            }
            if (++slot->field_0x22 >= 600) {
                found = 1;
            }
            if (found == 1) {
                slot->field_0x0B = 2;
                slot->field_0x0A = 0;
            }
            break;
        }
        break;
    case 2:
        switch (slot->field_0x0A) {
        case 0:
            slot->field_0x0A++;
            index = ran_suu(1) & 3;
            slot->field_0x36 = index;
            eft_slot_shape_move(slot, index);
            eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
            break;
        case 1:
            found = 1;
            for (i = 0; i < n; i++) {
                if (works[i]->field_0x1E2 != 4) {
                    found = 0;
                    break;
                }
            }
            if (found == 1) {
                slot->field_0x0A++;
            }
            break;
        }
        break;
    case 3:
        if (slot->field_0x0A == 0) {
            slot->field_0x0A++;
            index = (ran_suu(1) & 1) + 10;
            slot->field_0x36 = index;
            eft_slot_shape_move(slot, index);
            eft_slot_mode_set_defer(slot, 4, slot->field_0x36);
        }
        break;
    }
}
#pragma pop

