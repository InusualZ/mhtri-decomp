/*
 * ef/eft_slot.cpp - the `_EFT` family's 10-entry slot pool (`eft_slot_pool`, one 0x3C-byte `EftSlot` per entry), the
 *   enemy-record scan that drives it, the `EftDef` definition table and the slot predicates and state setters.
 * RANGE. .text 0x803432B4-0x80348A48 (76 functions, 75 written); extab 0x80016DB4-0x80016F74, extabindex
 *   0x800362F4-0x80036594, .ctors 0x8056F3A4-0x8056F3A8, .data 0x805E7B70-0x805E91E8, .bss 0x806BF0A0-0x806BF310,
 *   .sdata 0x807930F0-0x807932F0, .sdata2 0x8079B320-0x8079B368.  The tail from 0x80348A48 is
 *   `menu/menu_effect_slot.cpp`.  The left edge is false (a seam request): `eft050_set_move` (0x803432B4) is the
 *   `eft050_set` record's move state (`ef/eft050.cpp`'s create and dispatcher call it, it reads `Eft050SetWork` and
 *   `lbl_805E7B50` in that unit's `.data`), `eft_state_advance`/`eft_instance_release` are that dispatcher's other
 *   arms, and `.sdata2` 0x8079B320-0x8079B350 is `eft050_set_move`'s own pool: it holds 0.0f at 0x8079B320 while
 *   this unit's pool holds its own 0.0f at 0x8079B350 (one value twice = two TUs), so this unit starts at
 *   0x80343758 (`eft_slot_clear`).  The `.data` program tables are `{setup, step, state change, NULL}` quadruples.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps the unfused `clrlwi` + `cmpwi` of
 *   `eft_slot_live_ck` and the `cntlzw`/`srwi` of `eft_slot_persist_ck`; playbook 39); `#pragma optimization_level 4`
 *   around the slot programs whose switches retail lowers to range tests (`eft_slot_track_step`, `eft_slot_mode_rearm`,
 *   `eft_slot_unbound_step`, `eft_slot_roam_step`, `eft_slot_arena_state_change`..`eft_slot_arena_step`).
 * NAMES. Every definition is a GUESS from its body (the dump has only placeholders here): the pool (`eft_slot_pool`,
 *   `eft_slot_clear`, `eft_slot_pool_clear`, `eft_slot_find_free`, `eft_slot_spawn`, `eft_slot_spawn_targets`), the
 *   definition table (`eft_def_table`, `eft_def_get`, `eft_def_flags`, `eft_def_handler`, `eft_def_model_block`), the
 *   predicates (`eft_slot_armed_ck`, `eft_slot_live_ck`, `eft_slot_persist_ck`, `eft_slot_area_ck`), the per-frame
 *   work (`eft_slot_work_bind`, `eft_slot_work_update`, `eft_slot_counters_step`, `eft_slot_kind_set`,
 *   `eft_slot_effect_key`, `eft_slot_state_set`, `eft_slot_state_request`), the match tests (`eft_work_match_ck`,
 *   `eft_target_match_ck`, `eft_work_wide_ck`, `eft_target_wide_ck`, `eft_slot_match_count`), the (mode, value) pair
 *   at +0x0F/+0x10 (`eft_slot_mode_set`, `eft_slot_mode_set_sync`, `eft_slot_mode_set_local`, `eft_slot_mode_set_map`,
 *   `eft_slot_mode_set_work`), the marks (`eft_slot_marks_set`, `eft_slot_marks_set_sync`), the two instance arms
 *   `eft_state_advance`/`eft_instance_release`, the definition walkers (`eft_slot_effect_kind_get`,
 *   `eft_slot_shape_block_get`, `eft_slot_entry3_get`, `eft_slot_entry4_get`, `eft_slot_area_entry_get`), the shape
 *   helpers (`eft_shape_block_center`, `eft_slot_shape_move`, `eft_slot_kind_shape_move`, `eft_slot_shape_hit_ck`,
 *   `eft_slot_enemy_near_ck`), the enemy binding and rating (`eft_slot_enemy_rank`, `eft_slot_track_recount`,
 *   `eft_slot_pool_frame`, `eft_slot_enemy_bind`, `eft_slot_entry3_has`, `eft_slot_area_rate`,
 *   `eft_slot_flee_area_get`, `eft_slot_weight_roll`, `eft_slot_grade_set`, `eft_slot_enemy_counts_ck`,
 *   `eft_slot_escape_time_get`, `eft_slot_link_marks_get`) and the per-definition programs (`eft_slot_track_step`..
 *   `eft_slot_arena_step`, `eft_slot_arena_state_noop`).  The data is named by its path from `eft_def_table`:
 *   `eft_def<N>` is entry N, `_shapes`/`_links`/`_flee`/`_kinds` its four sub-tables (+0x04..+0x10), `_g<G>` a map
 *   group, `_a<A>`/`_k<K>` an area or key, `_programs` its program table; `eft_slot_roam_tables_g<G>_m<M>_r<R>_k<K>`
 *   likewise (`_m` mode, `_r` row).  `enemy_data_find`/`enemy_data_grp` are named for the enemy band's callers.
 *   `__sinit_\eft_slot_cpp` and `__ct__7EftSlotFv` are the compiler's.
 * RESIDUALS. 1 row unwritten: `eft050_set_move` (0x490, the seam above; it cannot be written here without copying
 *   `Eft050SetWork` out of `ef/eft050.cpp`).  Partial rows (register assignment unless noted):
 *  - `eft_slot_counters_step`: ours sign-extends the incremented resend timer (`extsh`) before its `sth`; an `s16`/`s32`
 *    local, `+= 1`, `++timer` and a cast all keep it;
 *  - `enemy_data_grp`: ours narrows the `flag == 0 ? 6 : 7` result (`clrlwi`) for the `u8` return; retail does not;
 *  - `eft_slot_shape_hit_ck`: retail schedules `addi r5` (the box) after the radius `fadds`, ours before;
 *  - `eft_slot_arena_state_change`: retail stores the u32 `index` raw to +0x36 and narrows for the call, ours narrows
 *    once for both;
 *  - `eft_slot_roam_step`, `eft_slot_linked_step`: retail narrows `state` (`clrlwi`) at the state compares and the
 *    request call, ours keeps it narrow; `eft_slot_roam_step` also narrows the +0x36 sum before its store;
 *  - `eft_slot_unbound_step`, `eft_slot_enemy_rank`, `eft_slot_linked_state_change`, `eft_slot_track_step`,
 *    `eft_slot_track_recount`, `eft_slot_work_update`, `eft_slot_spawn`: register assignment only.
 *   Data: `.data`/`.sdata` byte-identical; `.sdata2` is short by `eft050_set_move`'s pool (0x8079B320-0x8079B350), and
 *   `extab`/`extabindex` by its unwind entry - both close with the seam.
 * SHAPES. A `get_move_work_max` count is held in a `u32` and narrowed at the loop compare (`(u16)max`), which keeps the
 *   retail late `clrlwi` (playbook 168); `(u32)` on `em_work_die_ck`/`em_action_mid_ck` results gives retail's
 *   `cmplwi`; the two-value tests `x == 1 || x == 2` are `(u32)(x - 1) <= 1`.
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
/* One record of the definition table `eft_def_table`, indexed by `EftSlot::key_0x00`.  `kind_0x00` is
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
EftSlot eft_slot_pool[10]; /* .bss 0x806BF0A0, 0x258 B */
}
/* The definition table the slot's `key_0x00` indexes (36 B, 9 entries; index 0 is the "no definition"
 * slot).  This unit owns it: its only referrers are this range's own functions (`eft_def_get`,
 * `eft_def_flags`, `eft_def_handler` - 6 relocs, 2 per caller, and no object outside the band names it), it
 * is contiguous with the switch table MWCC emits for `enemy_data_grp` below, and the `.data` claim covers
 * 0x805E9168..0x805E91E8 so the object is the target object.  The values and their order are the DOL's
 * words at 0x805E9168..0x805E918C; the eight non-null entries point at the 0x18-byte `EftDef` records
 * the band's `.data` band holds (playbook 58: a unit's sole-referencer data is claimed, not extern'd). */

extern "C" {
EftDef eft_def1;   /* .bss  0x806BF2F8, 0x18 B */
}
/* One entry point of a definition record's program table. */
typedef void (*EftProgram)(_EFT*);

extern "C" {
/* The programs the definition records point at (`EftDef::handlers_0x14`): setup, per-frame step, state change. */
void eft_slot_grade_set(EftSlot* slot);
void eft_slot_track_step(EftSlot* slot);
void eft_slot_mode_rearm(EftSlot* slot, u8 previous, _ENEMY_WORK* work);
void eft_slot_key_shape_move(EftSlot* slot);
void eft_slot_unbound_step(EftSlot* slot);
void eft_slot_state_change(EftSlot* slot, u8 previous, _ENEMY_WORK* work);
void eft_slot_area_bind(EftSlot* slot);
void eft_slot_shape_block3_set(EftSlot* slot);
void eft_slot_linked_step(EftSlot* slot);
void eft_slot_linked_state_change(EftSlot* slot, u8 previous, _ENEMY_WORK* work);
void eft_slot_shape_block0_set(EftSlot* slot);
void eft_slot_roam_step(EftSlot* slot);
void eft_slot_arena_state_change(EftSlot* slot, u8 previous, _ENEMY_WORK* work);
void eft_slot_shape_block11_set(EftSlot* slot);
void eft_slot_arena_step(EftSlot* slot);
void eft_slot_arena_state_noop(EftSlot* slot, u8 previous, _ENEMY_WORK* work);

/* The definition records and the tables under them, in the DOL's `.data`/`.sdata` order.  Each `EftEntry` list is
 * keyed first by map group (`fn_80125FF0`), then by area or key, and ends with key 255; the leaves are shape-block
 * arrays (`_shapes`) or byte lists (`_links`, `_flee`, `_kinds`, the roam weights). */
EftShapeBlock eft_def2_shapes_g1_a1[1] = {
    {0, {0}, {828.9f, 582.7f, -2991.6f}, {0.0f, 0.0f, 0.0f}, 900.0f},
};
EftShapeBlock eft_def2_shapes_g1_a2[1] = {
    {0, {0}, {-6627.7f, 12.7f, 411.6f}, {0.0f, 0.0f, 0.0f}, 600.0f},
};
EftShapeBlock eft_def2_shapes_g1_a3[1] = {
    {0, {0}, {-1598.4f, 23.8f, -617.2f}, {0.0f, 0.0f, 0.0f}, 1140.0f},
};
EftShapeBlock eft_def2_shapes_g1_a4[1] = {
    {0, {0}, {-351.6f, 22.0f, 3644.0f}, {0.0f, 0.0f, 0.0f}, 500.0f},
};
EftShapeBlock eft_def2_shapes_g1_a5[1] = {
    {0, {0}, {-1817.0f, 36.0f, -720.0f}, {0.0f, 0.0f, 0.0f}, 1000.0f},
};
EftShapeBlock eft_def2_shapes_g1_a9[1] = {
    {0, {0}, {446.2f, -6.3f, 682.0f}, {0.0f, 0.0f, 0.0f}, 740.0f},
};
EftShapeBlock eft_def2_shapes_g1_a10[1] = {
    {0, {0}, {-3604.4f, -200.7f, 429.3f}, {-2442.9f, -107.9f, -1278.1f}, 500.0f},
};
EftEntry eft_def2_shapes_g1[8] = {
    {1, {0}, (EftEntry*)eft_def2_shapes_g1_a1},
    {2, {0}, (EftEntry*)eft_def2_shapes_g1_a2},
    {3, {0}, (EftEntry*)eft_def2_shapes_g1_a3},
    {4, {0}, (EftEntry*)eft_def2_shapes_g1_a4},
    {5, {0}, (EftEntry*)eft_def2_shapes_g1_a5},
    {9, {0}, (EftEntry*)eft_def2_shapes_g1_a9},
    {10, {0}, (EftEntry*)eft_def2_shapes_g1_a10},
    {255, {0}, NULL},
};
EftShapeBlock eft_def2_shapes_g2_a1[1] = {
    {0, {0}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1500.0f},
};
EftShapeBlock eft_def2_shapes_g2_a2[1] = {
    {0, {0}, {-492.6f, 20.8f, -1288.5f}, {0.0f, 0.0f, 0.0f}, 680.0f},
};
EftShapeBlock eft_def2_shapes_g2_a3[1] = {
    {1, {0}, {2212.4f, 40.0f, -842.4f}, {2606.4f, 40.0f, -39.2f}, 500.0f},
};
EftShapeBlock eft_def2_shapes_g2_a4[2] = {
    {0, {0}, {700.2f, -40.6f, 2681.5f}, {0.0f, 0.0f, 0.0f}, 1500.0f},
    {0, {0}, {-2536.5f, -40.0f, -715.9f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftShapeBlock eft_def2_shapes_g2_a5[1] = {
    {0, {0}, {176.9f, 24.0f, -207.7f}, {0.0f, 0.0f, 0.0f}, 1340.0f},
};
EftShapeBlock eft_def2_shapes_g2_a6[1] = {
    {0, {0}, {-819.4f, 5.7f, -1130.4f}, {0.0f, 0.0f, 0.0f}, 820.0f},
};
EftShapeBlock eft_def2_shapes_g2_a7[1] = {
    {1, {0}, {3533.5f, -56.7f, -3151.0f}, {3457.3f, -11.8f, -652.0f}, 800.0f},
};
EftShapeBlock eft_def2_shapes_g2_a9[1] = {
    {0, {0}, {3139.9f, -24.3f, 771.4f}, {0.0f, 0.0f, 0.0f}, 820.0f},
};
EftShapeBlock eft_def2_shapes_g2_a10[1] = {
    {0, {0}, {1194.0f, -99.4f, 1052.5f}, {0.0f, 0.0f, 0.0f}, 900.0f},
};
EftEntry eft_def2_shapes_g2[10] = {
    {1, {0}, (EftEntry*)eft_def2_shapes_g2_a1},
    {2, {0}, (EftEntry*)eft_def2_shapes_g2_a2},
    {3, {0}, (EftEntry*)eft_def2_shapes_g2_a3},
    {4, {0}, (EftEntry*)eft_def2_shapes_g2_a4},
    {5, {0}, (EftEntry*)eft_def2_shapes_g2_a5},
    {6, {0}, (EftEntry*)eft_def2_shapes_g2_a6},
    {7, {0}, (EftEntry*)eft_def2_shapes_g2_a7},
    {9, {0}, (EftEntry*)eft_def2_shapes_g2_a9},
    {10, {0}, (EftEntry*)eft_def2_shapes_g2_a10},
    {255, {0}, NULL},
};
EftShapeBlock eft_def2_shapes_g4_a1[1] = {
    {0, {0}, {-2273.5f, 9.7f, -1336.7f}, {0.0f, 0.0f, 0.0f}, 820.0f},
};
EftShapeBlock eft_def2_shapes_g4_a2[1] = {
    {0, {0}, {-2699.8f, 20.0f, -1217.5f}, {0.0f, 0.0f, 0.0f}, 940.0f},
};
EftShapeBlock eft_def2_shapes_g4_a3[1] = {
    {0, {0}, {3409.8f, -10.7f, -237.3f}, {0.0f, 0.0f, 0.0f}, 1300.0f},
};
EftShapeBlock eft_def2_shapes_g4_a5[1] = {
    {0, {0}, {313.0f, 24.0f, -1588.1f}, {0.0f, 0.0f, 0.0f}, 600.0f},
};
EftShapeBlock eft_def2_shapes_g4_a6[1] = {
    {0, {0}, {1208.4f, 1.0f, -1276.0f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftShapeBlock eft_def2_shapes_g4_a7[1] = {
    {1, {0}, {-2014.3f, 25.1f, 412.4f}, {139.5f, 3.2f, -2497.2f}, 1140.0f},
};
EftShapeBlock eft_def2_shapes_g4_a9[1] = {
    {0, {0}, {-151.7f, -7.3f, -1862.6f}, {0.0f, 0.0f, 0.0f}, 700.0f},
};
EftEntry eft_def2_shapes_g4[8] = {
    {1, {0}, (EftEntry*)eft_def2_shapes_g4_a1},
    {2, {0}, (EftEntry*)eft_def2_shapes_g4_a2},
    {3, {0}, (EftEntry*)eft_def2_shapes_g4_a3},
    {5, {0}, (EftEntry*)eft_def2_shapes_g4_a5},
    {6, {0}, (EftEntry*)eft_def2_shapes_g4_a6},
    {7, {0}, (EftEntry*)eft_def2_shapes_g4_a7},
    {9, {0}, (EftEntry*)eft_def2_shapes_g4_a9},
    {255, {0}, NULL},
};
EftShapeBlock eft_def2_shapes_g8_a1[1] = {
    {0, {0}, {1750.7f, -86.0f, -1697.6f}, {0.0f, 0.0f, 0.0f}, 1300.0f},
};
EftEntry eft_def2_shapes_g8[2] = {
    {1, {0}, (EftEntry*)eft_def2_shapes_g8_a1},
    {255, {0}, NULL},
};
EftEntry eft_def2_shapes_g9[2] = {
    {0, {0}, (EftEntry*)eft_def2_shapes_g8_a1},
    {255, {0}, NULL},
};
EftEntry eft_def2_shapes[6] = {
    {1, {0}, eft_def2_shapes_g1},
    {2, {0}, eft_def2_shapes_g2},
    {4, {0}, eft_def2_shapes_g4},
    {8, {0}, eft_def2_shapes_g8},
    {9, {0}, eft_def2_shapes_g9},
    {255, {0}, NULL},
};
EftProgram eft_def2_programs[4] = {
    (EftProgram)eft_slot_grade_set,
    (EftProgram)eft_slot_track_step,
    (EftProgram)eft_slot_mode_rearm,
    NULL,
};
EftDef eft_def2 = {0x09, 0x0C, 0x01, 0x00, eft_def2_shapes, NULL, NULL, NULL, eft_def2_programs};
EftDef eft_def8 = {0x09, 0x0E, 0x01, 0x00, eft_def2_shapes, NULL, NULL, NULL, eft_def2_programs};
EftShapeBlock eft_def3_shapes_g1_a1[1] = {
    {0, {0}, {-2058.4f, 282.9f, -5976.4f}, {0.0f, 0.0f, 0.0f}, 1180.0f},
};
EftShapeBlock eft_def3_shapes_g1_a2[2] = {
    {0, {0}, {-5005.94f, -2.79f, 257.02f}, {0.0f, 0.0f, 0.0f}, 1220.0f},
    {0, {0}, {4044.0f, 34.0f, -1296.0f}, {0.0f, 0.0f, 0.0f}, 1080.0f},
};
EftShapeBlock eft_def3_shapes_g1_a3[2] = {
    {0, {0}, {807.15f, -10.88f, 1584.68f}, {0.0f, 0.0f, 0.0f}, 1550.6f},
    {0, {0}, {-1481.33f, 0.54f, 146.7f}, {0.0f, 0.0f, 0.0f}, 1903.1f},
};
EftEntry eft_def3_shapes_g1[4] = {
    {1, {0}, (EftEntry*)eft_def3_shapes_g1_a1},
    {2, {0}, (EftEntry*)eft_def3_shapes_g1_a2},
    {3, {0}, (EftEntry*)eft_def3_shapes_g1_a3},
    {255, {0}, NULL},
};
EftShapeBlock eft_def3_shapes_g2_a1[2] = {
    {0, {0}, {-112.1f, -73.1f, 1158.4f}, {0.0f, 0.0f, 0.0f}, 1180.0f},
    {0, {0}, {-1614.9f, -73.1f, 1323.4f}, {0.0f, 0.0f, 0.0f}, 1200.0f},
};
EftShapeBlock eft_def3_shapes_g2_a2[2] = {
    {0, {0}, {-41.3f, 26.4f, -2755.0f}, {0.0f, 0.0f, 0.0f}, 760.0f},
    {0, {0}, {-1245.7f, 26.4f, 869.4f}, {0.0f, 0.0f, 0.0f}, 900.0f},
};
EftShapeBlock eft_def3_shapes_g2_a3[2] = {
    {0, {0}, {1832.0f, 50.0f, 974.4f}, {0.0f, 0.0f, 0.0f}, 1300.0f},
    {0, {0}, {403.7f, 50.0f, -1771.7f}, {0.0f, 0.0f, 0.0f}, 1300.0f},
};
EftShapeBlock eft_def3_shapes_g2_a4[2] = {
    {0, {0}, {726.6f, 8.3f, -209.3f}, {0.0f, 0.0f, 0.0f}, 1200.0f},
    {0, {0}, {-100.6f, 8.3f, 4649.6f}, {0.0f, 0.0f, 0.0f}, 1000.0f},
};
EftEntry eft_def3_shapes_g2[5] = {
    {1, {0}, (EftEntry*)eft_def3_shapes_g2_a1},
    {2, {0}, (EftEntry*)eft_def3_shapes_g2_a2},
    {3, {0}, (EftEntry*)eft_def3_shapes_g2_a3},
    {4, {0}, (EftEntry*)eft_def3_shapes_g2_a4},
    {255, {0}, NULL},
};
EftShapeBlock eft_def3_shapes_g3_a1[2] = {
    {0, {0}, {988.8f, -60.0f, 294.4f}, {0.0f, 0.0f, 0.0f}, 700.0f},
    {0, {0}, {3064.6f, -60.0f, -151.0f}, {0.0f, 0.0f, 0.0f}, 700.0f},
};
EftShapeBlock eft_def3_shapes_g3_a3[2] = {
    {0, {0}, {-306.3f, 21.8f, 865.1f}, {0.0f, 0.0f, 0.0f}, 800.0f},
    {0, {0}, {72.7f, 51.8f, -1391.9f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftShapeBlock eft_def3_shapes_g3_a5[2] = {
    {0, {0}, {-1545.7f, 25.0f, -1880.0f}, {0.0f, 0.0f, 0.0f}, 1000.0f},
    {0, {0}, {1016.3f, 25.0f, -3461.8f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftShapeBlock eft_def3_shapes_g3_a7[2] = {
    {0, {0}, {-635.4f, -15.4f, 1309.1f}, {0.0f, 0.0f, 0.0f}, 800.0f},
    {0, {0}, {1326.4f, -15.4f, 1420.6f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftEntry eft_def3_shapes_g3[5] = {
    {1, {0}, (EftEntry*)eft_def3_shapes_g3_a1},
    {3, {0}, (EftEntry*)eft_def3_shapes_g3_a3},
    {5, {0}, (EftEntry*)eft_def3_shapes_g3_a5},
    {7, {0}, (EftEntry*)eft_def3_shapes_g3_a7},
    {255, {0}, NULL},
};
EftShapeBlock eft_def3_shapes_g4_a1[2] = {
    {0, {0}, {-1893.2f, 25.0f, -792.3f}, {0.0f, 0.0f, 0.0f}, 1100.0f},
    {0, {0}, {439.1f, 25.0f, -1061.3f}, {0.0f, 0.0f, 0.0f}, 1200.0f},
};
EftShapeBlock eft_def3_shapes_g4_a2[2] = {
    {0, {0}, {1069.5f, 69.4f, 1999.6f}, {0.0f, 0.0f, 0.0f}, 1200.0f},
    {0, {0}, {985.3f, 69.4f, 400.9f}, {0.0f, 0.0f, 0.0f}, 1100.0f},
};
EftShapeBlock eft_def3_shapes_g4_a3[2] = {
    {0, {0}, {1360.5f, 26.4f, 1457.3f}, {0.0f, 0.0f, 0.0f}, 1100.0f},
    {0, {0}, {-683.8f, 36.1f, -745.0f}, {0.0f, 0.0f, 0.0f}, 1300.0f},
};
EftEntry eft_def3_shapes_g4[4] = {
    {1, {0}, (EftEntry*)eft_def3_shapes_g4_a1},
    {2, {0}, (EftEntry*)eft_def3_shapes_g4_a2},
    {3, {0}, (EftEntry*)eft_def3_shapes_g4_a3},
    {255, {0}, NULL},
};
EftShapeBlock eft_def3_shapes_g5_a1[2] = {
    {0, {0}, {1020.0f, 90.5f, -1151.4f}, {0.0f, 0.0f, 0.0f}, 800.0f},
    {0, {0}, {-1027.1f, 30.5f, -768.7f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftShapeBlock eft_def3_shapes_g5_a2[2] = {
    {0, {0}, {-1634.3f, 48.9f, 899.3f}, {0.0f, 0.0f, 0.0f}, 900.0f},
    {0, {0}, {1713.8f, -161.1f, -670.6f}, {0.0f, 0.0f, 0.0f}, 1000.0f},
};
EftShapeBlock eft_def3_shapes_g5_a3[2] = {
    {0, {0}, {7279.7f, 101.6f, 6307.6f}, {0.0f, 0.0f, 0.0f}, 800.0f},
    {0, {0}, {6147.3f, 11.6f, 4117.3f}, {0.0f, 0.0f, 0.0f}, 800.0f},
};
EftShapeBlock eft_def3_shapes_g5_a6[2] = {
    {0, {0}, {1284.4f, -5.4f, 680.2f}, {0.0f, 0.0f, 0.0f}, 1000.0f},
    {0, {0}, {1062.9f, -5.4f, -715.8f}, {0.0f, 0.0f, 0.0f}, 900.0f},
};
EftEntry eft_def3_shapes_g5[5] = {
    {1, {0}, (EftEntry*)eft_def3_shapes_g5_a1},
    {2, {0}, (EftEntry*)eft_def3_shapes_g5_a2},
    {3, {0}, (EftEntry*)eft_def3_shapes_g5_a3},
    {6, {0}, (EftEntry*)eft_def3_shapes_g5_a6},
    {255, {0}, NULL},
};
EftEntry eft_def3_shapes[6] = {
    {1, {0}, eft_def3_shapes_g1},
    {2, {0}, eft_def3_shapes_g2},
    {3, {0}, eft_def3_shapes_g3},
    {4, {0}, eft_def3_shapes_g4},
    {5, {0}, eft_def3_shapes_g5},
    {255, {0}, NULL},
};
u8 eft_def3_links_g1_k1[3] = {0x02, 0x03, 0xFF};
u8 eft_def3_links_g1_k2[5] = {0x01, 0x03, 0x04, 0x0B, 0xFF};
u8 eft_def3_links_g1_k3[5] = {0x00, 0x01, 0x02, 0x05, 0xFF};
EftEntry eft_def3_links_g1[4] = {
    {1, {0}, (EftEntry*)eft_def3_links_g1_k1},
    {2, {0}, (EftEntry*)eft_def3_links_g1_k2},
    {3, {0}, (EftEntry*)eft_def3_links_g1_k3},
    {255, {0}, NULL},
};
u8 eft_def3_links_g2_k1[3] = {0x02, 0x04, 0xFF};
u8 eft_def3_links_g2_k2[4] = {0x01, 0x03, 0x09, 0xFF};
u8 eft_def3_links_g2_k3[3] = {0x02, 0x04, 0xFF};
u8 eft_def3_links_g2_k4[6] = {0x01, 0x03, 0x05, 0x06, 0x0B, 0xFF};
EftEntry eft_def3_links_g2[5] = {
    {1, {0}, (EftEntry*)eft_def3_links_g2_k1},
    {2, {0}, (EftEntry*)eft_def3_links_g2_k2},
    {3, {0}, (EftEntry*)eft_def3_links_g2_k3},
    {4, {0}, (EftEntry*)eft_def3_links_g2_k4},
    {255, {0}, NULL},
};
u8 eft_def3_links_g3_k1[5] = {0x02, 0x05, 0x06, 0x07, 0xFF};
u8 eft_def3_links_g3_k3[6] = {0x02, 0x04, 0x05, 0x07, 0x08, 0xFF};
u8 eft_def3_links_g3_k5[3] = {0x01, 0x03, 0xFF};
u8 eft_def3_links_g3_k7[5] = {0x01, 0x03, 0x08, 0x09, 0xFF};
EftEntry eft_def3_links_g3[5] = {
    {1, {0}, (EftEntry*)eft_def3_links_g3_k1},
    {3, {0}, (EftEntry*)eft_def3_links_g3_k3},
    {5, {0}, (EftEntry*)eft_def3_links_g3_k5},
    {7, {0}, (EftEntry*)eft_def3_links_g3_k7},
    {255, {0}, NULL},
};
u8 eft_def3_links_g4_k1[5] = {0x00, 0x02, 0x03, 0x08, 0xFF};
u8 eft_def3_links_g4_k2[5] = {0x01, 0x03, 0x04, 0x06, 0xFF};
u8 eft_def3_links_g4_k3[5] = {0x01, 0x02, 0x05, 0x06, 0xFF};
EftEntry eft_def3_links_g4[4] = {
    {1, {0}, (EftEntry*)eft_def3_links_g4_k1},
    {2, {0}, (EftEntry*)eft_def3_links_g4_k2},
    {3, {0}, (EftEntry*)eft_def3_links_g4_k3},
    {255, {0}, NULL},
};
u8 eft_def3_links_g5_k1[3] = {0x02, 0x06, 0xFF};
u8 eft_def3_links_g5_k2[3] = {0x01, 0x03, 0xFF};
u8 eft_def3_links_g5_k3[4] = {0x02, 0x04, 0x06, 0xFF};
u8 eft_def3_links_g5_k6[4] = {0x01, 0x03, 0x07, 0xFF};
EftEntry eft_def3_links_g5[5] = {
    {1, {0}, (EftEntry*)eft_def3_links_g5_k1},
    {2, {0}, (EftEntry*)eft_def3_links_g5_k2},
    {3, {0}, (EftEntry*)eft_def3_links_g5_k3},
    {6, {0}, (EftEntry*)eft_def3_links_g5_k6},
    {255, {0}, NULL},
};
EftEntry eft_def3_links[6] = {
    {1, {0}, eft_def3_links_g1},
    {2, {0}, eft_def3_links_g2},
    {3, {0}, eft_def3_links_g3},
    {4, {0}, eft_def3_links_g4},
    {5, {0}, eft_def3_links_g5},
    {255, {0}, NULL},
};
u8 eft_def3_flee_g1_a1[3] = {0x03, 0x02, 0xFF};
u8 eft_def3_flee_g1_a2[3] = {0x01, 0x03, 0xFF};
u8 eft_def3_flee_g1_a3[3] = {0x02, 0x01, 0xFF};
EftEntry eft_def3_flee_g1[4] = {
    {1, {0}, (EftEntry*)eft_def3_flee_g1_a1},
    {2, {0}, (EftEntry*)eft_def3_flee_g1_a2},
    {3, {0}, (EftEntry*)eft_def3_flee_g1_a3},
    {255, {0}, NULL},
};
u8 eft_def3_flee_g2_a1[3] = {0x02, 0x04, 0xFF};
u8 eft_def3_flee_g2_a2[3] = {0x03, 0x01, 0xFF};
u8 eft_def3_flee_g2_a3[3] = {0x04, 0x02, 0xFF};
u8 eft_def3_flee_g2_a4[3] = {0x01, 0x03, 0xFF};
EftEntry eft_def3_flee_g2[5] = {
    {1, {0}, (EftEntry*)eft_def3_flee_g2_a1},
    {2, {0}, (EftEntry*)eft_def3_flee_g2_a2},
    {3, {0}, (EftEntry*)eft_def3_flee_g2_a3},
    {4, {0}, (EftEntry*)eft_def3_flee_g2_a4},
    {255, {0}, NULL},
};
u8 eft_def3_flee_g3_a1[3] = {0x05, 0x07, 0xFF};
u8 eft_def3_flee_g3_a3[3] = {0x07, 0x05, 0xFF};
u8 eft_def3_flee_g3_a5[3] = {0x03, 0x01, 0xFF};
u8 eft_def3_flee_g3_a7[3] = {0x01, 0x03, 0xFF};
EftEntry eft_def3_flee_g3[5] = {
    {1, {0}, (EftEntry*)eft_def3_flee_g3_a1},
    {3, {0}, (EftEntry*)eft_def3_flee_g3_a3},
    {5, {0}, (EftEntry*)eft_def3_flee_g3_a5},
    {7, {0}, (EftEntry*)eft_def3_flee_g3_a7},
    {255, {0}, NULL},
};
u8 eft_def3_flee_g4_a1[3] = {0x03, 0x02, 0xFF};
u8 eft_def3_flee_g4_a2[3] = {0x01, 0x03, 0xFF};
u8 eft_def3_flee_g4_a3[3] = {0x02, 0x01, 0xFF};
EftEntry eft_def3_flee_g4[4] = {
    {1, {0}, (EftEntry*)eft_def3_flee_g4_a1},
    {2, {0}, (EftEntry*)eft_def3_flee_g4_a2},
    {3, {0}, (EftEntry*)eft_def3_flee_g4_a3},
    {255, {0}, NULL},
};
u8 eft_def3_flee_g5_a1[3] = {0x02, 0x06, 0xFF};
u8 eft_def3_flee_g5_a2[3] = {0x03, 0x01, 0xFF};
u8 eft_def3_flee_g5_a3[3] = {0x06, 0x02, 0xFF};
u8 eft_def3_flee_g5_a6[3] = {0x01, 0x03, 0xFF};
EftEntry eft_def3_flee_g5[5] = {
    {1, {0}, (EftEntry*)eft_def3_flee_g5_a1},
    {2, {0}, (EftEntry*)eft_def3_flee_g5_a2},
    {3, {0}, (EftEntry*)eft_def3_flee_g5_a3},
    {6, {0}, (EftEntry*)eft_def3_flee_g5_a6},
    {255, {0}, NULL},
};
EftEntry eft_def3_flee[6] = {
    {1, {0}, eft_def3_flee_g1},
    {2, {0}, eft_def3_flee_g2},
    {3, {0}, eft_def3_flee_g3},
    {4, {0}, eft_def3_flee_g4},
    {5, {0}, eft_def3_flee_g5},
    {255, {0}, NULL},
};
u8 eft_def3_kinds_g1_a1[1] = {0xFF};
u8 eft_def3_kinds_g1_a2[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
u8 eft_def3_kinds_g1_a3[5] = {0x01, 0x00, 0x02, 0x01, 0xFF};
EftEntry eft_def3_kinds_g1[4] = {
    {1, {0}, (EftEntry*)eft_def3_kinds_g1_a1},
    {2, {0}, (EftEntry*)eft_def3_kinds_g1_a2},
    {3, {0}, (EftEntry*)eft_def3_kinds_g1_a3},
    {255, {0}, NULL},
};
u8 eft_def3_kinds_g2_a1[5] = {0x02, 0x00, 0x04, 0x01, 0xFF};
u8 eft_def3_kinds_g2_a2[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
u8 eft_def3_kinds_g2_a3[5] = {0x02, 0x00, 0x04, 0x01, 0xFF};
u8 eft_def3_kinds_g2_a4[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
EftEntry eft_def3_kinds_g2[5] = {
    {1, {0}, (EftEntry*)eft_def3_kinds_g2_a1},
    {2, {0}, (EftEntry*)eft_def3_kinds_g2_a2},
    {3, {0}, (EftEntry*)eft_def3_kinds_g2_a3},
    {4, {0}, (EftEntry*)eft_def3_kinds_g2_a4},
    {255, {0}, NULL},
};
u8 eft_def3_kinds_g3_a1[5] = {0x05, 0x00, 0x07, 0x01, 0xFF};
u8 eft_def3_kinds_g3_a3[5] = {0x05, 0x00, 0x07, 0x01, 0xFF};
u8 eft_def3_kinds_g3_a5[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
u8 eft_def3_kinds_g3_a7[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
EftEntry eft_def3_kinds_g3[5] = {
    {1, {0}, (EftEntry*)eft_def3_kinds_g3_a1},
    {3, {0}, (EftEntry*)eft_def3_kinds_g3_a3},
    {5, {0}, (EftEntry*)eft_def3_kinds_g3_a5},
    {7, {0}, (EftEntry*)eft_def3_kinds_g3_a7},
    {255, {0}, NULL},
};
u8 eft_def3_kinds_g4_a1[6] = {0x02, 0x00, 0x03, 0x01, 0xFF, 0x01};
u8 eft_def3_kinds_g4_a2[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
u8 eft_def3_kinds_g4_a3[5] = {0x01, 0x00, 0x02, 0x01, 0xFF};
EftEntry eft_def3_kinds_g4[4] = {
    {1, {0}, (EftEntry*)eft_def3_kinds_g4_a1},
    {2, {0}, (EftEntry*)eft_def3_kinds_g4_a2},
    {3, {0}, (EftEntry*)eft_def3_kinds_g4_a3},
    {255, {0}, NULL},
};
u8 eft_def3_kinds_g5_a1[5] = {0x02, 0x00, 0x06, 0x01, 0xFF};
u8 eft_def3_kinds_g5_a2[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
u8 eft_def3_kinds_g5_a3[5] = {0x02, 0x00, 0x06, 0x01, 0xFF};
u8 eft_def3_kinds_g5_a6[5] = {0x01, 0x00, 0x03, 0x01, 0xFF};
EftEntry eft_def3_kinds_g5[5] = {
    {1, {0}, (EftEntry*)eft_def3_kinds_g5_a1},
    {2, {0}, (EftEntry*)eft_def3_kinds_g5_a2},
    {3, {0}, (EftEntry*)eft_def3_kinds_g5_a3},
    {6, {0}, (EftEntry*)eft_def3_kinds_g5_a6},
    {255, {0}, NULL},
};
EftEntry eft_def3_kinds[6] = {
    {1, {0}, eft_def3_kinds_g1},
    {2, {0}, eft_def3_kinds_g2},
    {3, {0}, eft_def3_kinds_g3},
    {4, {0}, eft_def3_kinds_g4},
    {5, {0}, eft_def3_kinds_g5},
    {255, {0}, NULL},
};
EftProgram eft_def3_programs[4] = {
    (EftProgram)eft_slot_key_shape_move,
    (EftProgram)eft_slot_unbound_step,
    (EftProgram)eft_slot_state_change,
    NULL,
};
EftDef eft_def3 = {0x00, 0x00, 0x01, 0x00, eft_def3_shapes, eft_def3_links, eft_def3_flee, eft_def3_kinds, eft_def3_programs};
EftProgram eft_def4_programs[4] = {
    NULL,
    (EftProgram)eft_slot_area_bind,
    NULL,
    NULL,
};
EftDef eft_def4 = {0x04, 0x00, 0x01, 0x00, NULL, NULL, NULL, NULL, eft_def4_programs};
EftShapeBlock eft_def5_shapes_g1_a7[7] = {
    {1, {0}, {3229.4f, -3000.0f, 87.2f}, {3715.5f, -3000.0f, -2399.7f}, 500.0f},
    {1, {0}, {2833.0f, -3098.3f, -5274.7f}, {1138.7f, -3040.0f, -4024.4f}, 450.0f},
    {1, {0}, {-1833.7f, -2846.7f, -1930.0f}, {-946.1f, -2813.3f, -3325.0f}, 450.0f},
    {1, {0}, {2833.0f, -3098.3f, -5274.7f}, {1138.7f, -3040.0f, -4024.4f}, 450.0f},
    {0, {0}, {1982.2f, -1237.2f, 1554.2f}, {0.0f, 0.0f, 0.0f}, 1260.0f},
    {0, {0}, {2233.4f, -2265.7f, -3691.7f}, {0.0f, 0.0f, 0.0f}, 1380.0f},
    {0, {0}, {9.7f, -1215.7f, -1038.0f}, {0.0f, 0.0f, 0.0f}, 1000.0f},
};
EftShapeBlock eft_def5_shapes_g1_a12[7] = {
    {1, {0}, {-6946.1f, -917.1f, -2332.6f}, {-5816.5f, -913.0f, -748.3f}, 400.0f},
    {1, {0}, {-9.1f, -1180.0f, -6874.4f}, {1558.1f, -1129.0f, -6419.4f}, 350.0f},
    {1, {0}, {-6540.7f, -1517.1f, -8316.6f}, {-5527.5f, -1333.0f, -9494.0f}, 400.0f},
    {1, {0}, {-2151.7f, -897.1f, -1985.6f}, {804.5f, -913.0f, -1992.4f}, 710.0f},
    {1, {0}, {27.7f, -940.6f, -5490.0f}, {-2359.7f, -930.6f, -5430.3f}, 710.0f},
    {1, {0}, {-5555.7f, -940.6f, -5304.7f}, {-8009.7f, -930.0f, -5047.0f}, 790.0f},
    {1, {0}, {27.7f, -940.6f, -5490.0f}, {-2359.7f, -930.6f, -5430.3f}, 710.0f},
};
EftEntry eft_def5_shapes_g1[3] = {
    {7, {0}, (EftEntry*)eft_def5_shapes_g1_a7},
    {12, {0}, (EftEntry*)eft_def5_shapes_g1_a12},
    {255, {0}, NULL},
};
EftShapeBlock eft_def5_shapes_g3_a2[4] = {
    {1, {0}, {-1319.6f, -620.0f, -1129.5f}, {-591.9f, -620.0f, -504.8f}, 330.0f},
    {1, {0}, {-3660.3f, -620.0f, 267.3f}, {-2695.0f, -620.0f, 1301.5f}, 330.0f},
    {1, {0}, {-3433.4f, -500.0f, 623.9f}, {-2525.0f, -500.0f, -740.4f}, 420.0f},
    {1, {0}, {1471.2f, -540.0f, 1838.3f}, {-170.6f, -540.0f, 1097.1f}, 420.0f},
};
EftShapeBlock eft_def5_shapes_g3_a8[4] = {
    {1, {0}, {1375.1f, -1100.0f, 964.2f}, {1981.8f, -1125.4f, -135.4f}, 330.0f},
    {1, {0}, {-1113.3f, -1100.0f, -3711.2f}, {345.3f, -1125.4f, -3711.9f}, 330.0f},
    {1, {0}, {1158.6f, -660.0f, -2536.1f}, {-1659.0f, -660.0f, -2560.5f}, 600.0f},
    {1, {0}, {-18.3f, -660.0f, 894.5f}, {1363.9f, -685.4f, 351.6f}, 600.0f},
};
EftEntry eft_def5_shapes_g3[3] = {
    {2, {0}, (EftEntry*)eft_def5_shapes_g3_a2},
    {8, {0}, (EftEntry*)eft_def5_shapes_g3_a8},
    {255, {0}, NULL},
};
EftEntry eft_def5_shapes[3] = {
    {1, {0}, eft_def5_shapes_g1},
    {3, {0}, eft_def5_shapes_g3},
    {255, {0}, NULL},
};
EftProgram eft_def5_programs[4] = {
    (EftProgram)eft_slot_shape_block3_set,
    (EftProgram)eft_slot_linked_step,
    (EftProgram)eft_slot_linked_state_change,
    NULL,
};
EftDef eft_def5 = {0x00, 0x00, 0x01, 0x00, eft_def5_shapes, NULL, NULL, NULL, eft_def5_programs};
EftShapeBlock eft_def6_shapes_g2_a7[7] = {
    {0, {0}, {-5343.0f, -217.0f, 2946.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-5876.0f, 100.0f, -344.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-3035.0f, -278.0f, -3932.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {156.0f, -278.0f, -5335.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-318.0f, -227.0f, 455.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-1811.0f, -231.0f, 5154.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-2974.0f, -92.0f, 114.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
};
EftShapeBlock eft_def6_shapes_g2_a8[7] = {
    {0, {0}, {-2563.0f, -183.0f, 3089.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {3091.0f, -62.0f, 3092.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {3722.0f, -142.0f, -2039.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-361.0f, -144.0f, -4430.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-5879.0f, -15.0f, -4308.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-6813.0f, -84.0f, 24.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-1673.0f, -212.0f, -303.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
};
EftEntry eft_def6_shapes_g2[3] = {
    {7, {0}, (EftEntry*)eft_def6_shapes_g2_a7},
    {8, {0}, (EftEntry*)eft_def6_shapes_g2_a8},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r0_k5[3] = {0x01, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r0_k1[3] = {0x05, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r0[3] = {
    {5, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r0_k5},
    {1, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r0_k1},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r1_k0[5] = {0x02, 0x32, 0x06, 0x32, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r1_k2[5] = {0x00, 0x32, 0x06, 0x32, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r1_k6[5] = {0x00, 0x32, 0x02, 0x32, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r1[4] = {
    {0, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r1_k0},
    {2, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r1_k2},
    {6, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r1_k6},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r2_k1[3] = {0x03, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r2_k3[3] = {0x01, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r2[3] = {
    {1, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r2_k1},
    {3, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r2_k3},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r3_k2[3] = {0x04, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r3_k4[3] = {0x02, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r3[3] = {
    {2, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r3_k2},
    {4, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r3_k4},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r4_k3[5] = {0x05, 0x32, 0x06, 0x32, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r4_k5[5] = {0x03, 0x32, 0x06, 0x32, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r4_k6[5] = {0x03, 0x32, 0x05, 0x32, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r4[4] = {
    {3, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r4_k3},
    {5, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r4_k5},
    {6, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r4_k6},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r5_k4[3] = {0x00, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r5_k0[3] = {0x04, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r5[3] = {
    {4, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r5_k4},
    {0, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r5_k0},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m7_r6_k1[3] = {0x04, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m7_r6_k4[3] = {0x01, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m7_r6[3] = {
    {1, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r6_k1},
    {4, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7_r6_k4},
    {255, {0}, NULL},
};
EftEntry* eft_slot_roam_tables_g2_m7[8] = {
    eft_slot_roam_tables_g2_m7_r0,
    eft_slot_roam_tables_g2_m7_r1,
    eft_slot_roam_tables_g2_m7_r2,
    eft_slot_roam_tables_g2_m7_r3,
    eft_slot_roam_tables_g2_m7_r4,
    eft_slot_roam_tables_g2_m7_r5,
    eft_slot_roam_tables_g2_m7_r6,
    NULL,
};
u8 eft_slot_roam_tables_g2_m8_r0_k5[3] = {0x01, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r0_k1[3] = {0x05, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r0[3] = {
    {5, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r0_k5},
    {1, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r0_k1},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m8_r1_k0[5] = {0x02, 0x50, 0x06, 0x14, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r1_k2[5] = {0x00, 0x50, 0x06, 0x14, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r1_k6[5] = {0x00, 0x32, 0x02, 0x32, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r1[4] = {
    {0, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r1_k0},
    {2, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r1_k2},
    {6, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r1_k6},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m8_r2_k1[3] = {0x03, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r2_k3[3] = {0x01, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r2[3] = {
    {1, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r2_k1},
    {3, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r2_k3},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m8_r3_k2[3] = {0x04, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r3_k4[3] = {0x02, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r3[3] = {
    {2, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r3_k2},
    {4, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r3_k4},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m8_r4_k3[5] = {0x05, 0x50, 0x06, 0x14, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r4_k5[5] = {0x03, 0x50, 0x06, 0x14, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r4_k6[5] = {0x03, 0x32, 0x05, 0x32, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r4[4] = {
    {3, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r4_k3},
    {5, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r4_k5},
    {6, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r4_k6},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m8_r5_k4[3] = {0x00, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r5_k0[3] = {0x04, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r5[3] = {
    {4, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r5_k4},
    {0, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r5_k0},
    {255, {0}, NULL},
};
u8 eft_slot_roam_tables_g2_m8_r6_k1[3] = {0x04, 0x64, 0xFF};
u8 eft_slot_roam_tables_g2_m8_r6_k4[3] = {0x01, 0x64, 0xFF};
EftEntry eft_slot_roam_tables_g2_m8_r6[3] = {
    {1, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r6_k1},
    {4, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8_r6_k4},
    {255, {0}, NULL},
};
EftEntry* eft_slot_roam_tables_g2_m8[8] = {
    eft_slot_roam_tables_g2_m8_r0,
    eft_slot_roam_tables_g2_m8_r1,
    eft_slot_roam_tables_g2_m8_r2,
    eft_slot_roam_tables_g2_m8_r3,
    eft_slot_roam_tables_g2_m8_r4,
    eft_slot_roam_tables_g2_m8_r5,
    eft_slot_roam_tables_g2_m8_r6,
    NULL,
};
EftEntry eft_slot_roam_tables_g2[3] = {
    {7, {0}, (EftEntry*)eft_slot_roam_tables_g2_m7},
    {8, {0}, (EftEntry*)eft_slot_roam_tables_g2_m8},
    {255, {0}, NULL},
};
EftEntry eft_slot_roam_tables[2] = {
    {2, {0}, eft_slot_roam_tables_g2},
    {255, {0}, NULL},
};
EftShapeBlock eft_def6_shapes_g6_a1[12] = {
    {0, {0}, {736.0f, 242.7f, -82.5f}, {0.0f, 0.0f, 0.0f}, 250.0f},
    {0, {0}, {712.6f, 242.7f, 452.6f}, {0.0f, 0.0f, 0.0f}, 250.0f},
    {0, {0}, {-684.3f, 242.7f, -56.9f}, {0.0f, 0.0f, 0.0f}, 250.0f},
    {0, {0}, {-700.7f, 242.7f, 425.4f}, {0.0f, 0.0f, 0.0f}, 250.0f},
    {1, {0}, {1914.6f, 32.7f, -1847.4f}, {1877.8f, 32.7f, -2986.8f}, 550.0f},
    {1, {0}, {2002.6f, 32.7f, 854.5f}, {1942.3f, 32.7f, -443.1f}, 550.0f},
    {1, {0}, {1800.4f, 32.7f, 2810.8f}, {1813.6f, 32.7f, 1673.9f}, 550.0f},
    {1, {0}, {-1771.0f, 32.7f, -2027.1f}, {-1757.9f, 32.7f, -3164.0f}, 550.0f},
    {1, {0}, {-1909.3f, 32.7f, 809.5f}, {-1896.1f, 32.7f, -327.4f}, 550.0f},
    {1, {0}, {-1865.1f, 32.7f, 2992.0f}, {-1851.9f, 32.7f, 1855.0f}, 550.0f},
    {0, {0}, {1781.7f, 32.7f, 3764.0f}, {0.0f, 0.0f, 0.0f}, 600.0f},
    {0, {0}, {-1650.8f, 32.7f, 3492.2f}, {0.0f, 0.0f, 0.0f}, 600.0f},
};
EftEntry eft_def6_shapes_g6[2] = {
    {1, {0}, (EftEntry*)eft_def6_shapes_g6_a1},
    {255, {0}, NULL},
};
EftEntry eft_def6_shapes[3] = {
    {2, {0}, eft_def6_shapes_g2},
    {6, {0}, eft_def6_shapes_g6},
    {255, {0}, NULL},
};
u8 eft_def6_links_g2_k7[2] = {0x08, 0xFF};
u8 eft_def6_links_g2_k8[2] = {0x07, 0xFF};
EftEntry eft_def6_links_g2[3] = {
    {7, {0}, (EftEntry*)eft_def6_links_g2_k7},
    {8, {0}, (EftEntry*)eft_def6_links_g2_k8},
    {255, {0}, NULL},
};
EftEntry eft_def6_links[2] = {
    {2, {0}, eft_def6_links_g2},
    {255, {0}, NULL},
};
u8 eft_def6_flee_g2_a7[2] = {0x08, 0xFF};
u8 eft_def6_flee_g2_a8[2] = {0x07, 0xFF};
EftEntry eft_def6_flee_g2[3] = {
    {7, {0}, (EftEntry*)eft_def6_flee_g2_a7},
    {8, {0}, (EftEntry*)eft_def6_flee_g2_a8},
    {255, {0}, NULL},
};
EftEntry eft_def6_flee[2] = {
    {2, {0}, eft_def6_flee_g2},
    {255, {0}, NULL},
};
EftProgram eft_def6_programs[4] = {
    (EftProgram)eft_slot_shape_block0_set,
    (EftProgram)eft_slot_roam_step,
    (EftProgram)eft_slot_arena_state_change,
    NULL,
};
EftDef eft_def6 = {0x00, 0x00, 0x01, 0x00, eft_def6_shapes, eft_def6_links, eft_def6_flee, NULL, eft_def6_programs};
EftProgram eft_def7_programs[4] = {
    (EftProgram)eft_slot_shape_block11_set,
    (EftProgram)eft_slot_arena_step,
    (EftProgram)eft_slot_arena_state_noop,
    NULL,
};
EftDef eft_def7 = {0x00, 0x00, 0x01, 0x00, eft_def6_shapes, NULL, NULL, NULL, eft_def7_programs};

/* The definition table the slot's `key_0x00` indexes; index 0 is "no definition" and index 1 the `.bss` record. */
EftDef* eft_def_table[9] = {
    NULL,
    &eft_def1,
    &eft_def2,
    &eft_def3,
    &eft_def4,
    &eft_def5,
    &eft_def6,
    &eft_def7,
    &eft_def8,
};
}
extern "C" void eft_state_advance(_EFT* self);  /* `_EFT::state_0x05++` - the dispatcher's advance arm */
extern "C" void eft_instance_release(_EFT* self);  /* hands the instance to `eft_res_slot_release`, the shared release path */
extern "C" void eft_slot_clear(u8 index);  /* zeroes one pool entry's key byte and stamps its +0x14 byte 255 */
extern "C" void eft_slot_pool_clear(void);  /* the 10-iteration loop over `eft_slot_clear` */
extern "C" u8 eft_slot_find_free(void);  /* first entry whose `key_0x00` is zero, 255 when the pool is full */
extern "C" EftDef* eft_def_get(EftSlot* slot);  /* `eft_def_table[slot->key_0x00]` */
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
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 synced);  /* (mode, value) into +0x0F/+0x10 */
extern "C" void eft_slot_mode_set_sync(void* slot, u8 mode, u8 value);  /* the wrapper that passes synced = 1 */
extern "C" void eft_slot_mode_set_local(void* slot, u8 mode, u8 value);  /* the wrapper that passes synced = 0 */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 local);  /* mode/kind/value onto the pair */
extern "C" void eft_slot_kind_shape_move(EftSlot* slot);  /* this range's still-unwritten entry `eft_slot_kind_set` calls */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 local);  /* the work record rewrites the triple first */
extern "C" EftShapeBlock* eft_def_model_block(EftSlot* slot, u8 index);  /* the per-motion walk down to the 0x20-byte block */
extern "C" void eft_shape_block_center(EftShapeBlock* block, VEC3* out);
extern "C" u8 eft_slot_effect_kind_get(EftSlot* slot);
extern "C" void eft_slot_shape_move(EftSlot* slot, u8 index);
extern "C" EftEntry* eft_slot_entry3_get(EftSlot* slot, u8 key);
extern "C" u8 eft_slot_entry3_has(EftSlot* slot, u8 key, u8 value);
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
    eft_slot_pool[index].key_0x00 = 0;
    eft_slot_pool[index].field_0x14 = 255;
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
        if (eft_slot_pool[i].key_0x00 == 0) {
            return i;
        }
    }
    return 255;
}

/* Looks the 10-entry table up by its group index and the enemy-data id. */
extern "C" void* enemy_data_find(u8 key0, u8 key1) {
    u8 i;

    for (i = 0; i < 10; i++) {
        EftSlot* slot = &eft_slot_pool[i];

        if (key0 == slot->key_0x00 && key1 == slot->key_0x01) {
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
        return flag == 0 ? 6 : 7;
    }
}

/* Returns the definition record a slot's key byte selects. */
extern "C" EftDef* eft_def_get(EftSlot* slot) {
    return eft_def_table[slot->key_0x00];
}

/* Returns the definition's kind byte. */
extern "C" u8 eft_def_flags(EftSlot* slot) {
    return eft_def_table[slot->key_0x00]->kind_0x00;
}

/* Returns the definition's per-kind entry point, or NULL when the definition has none. */
extern "C" void (*eft_def_handler(EftSlot* slot, u8 kind))(_EFT*) {
    EftDef* def = eft_def_table[slot->key_0x00];

    if (def->handlers_0x14 != NULL) {
        switch (kind) {
        case 0:
            return def->handlers_0x14[0];
        case 1:
            return def->handlers_0x14[1];
        case 2:
            return def->handlers_0x14[2];
        }
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
    EftSlot* slot = (EftSlot*)enemy_data_find(key0, key1);
    u8 free_index;

    if (slot == NULL) {
        free_index = eft_slot_find_free();
        if (free_index != 255) {
            slot = &eft_slot_pool[free_index];
            slot->key_0x01 = key0;
            slot->field_0x08 = 0;
            slot->field_0x0A = 0;
            slot->field_0x0B = 0;
            slot->field_0x13 = get_now_mapno();
            slot->field_0x14 = index;
            slot->field_0x11 = 255;
            slot->field_0x12 = 255;
            eft_slot_mode_set_local(slot, 0, 0);
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
            slot->field_0x24 = 0.0f;
            slot->field_0x28 = 0.0f;
            slot->field_0x2C = 0.0f;
            slot->field_0x30 = 0.0f;
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
        } else {
            slot = NULL;
        }
    }
    return slot;
}

/* Spawns the live slots of every enemy work record, then of the 128-entry record array. */
extern "C" void eft_slot_spawn_targets(void) {
    s32 i;
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u32 count = get_move_work_max(3);
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;

    for (i = 0; i < (u16)count; i++, work++) {
        if ((u32)em_work_die_ck(work) != 1 && work->field_0x46C != 255) {
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
    u32 count;
    s8 current;
    EmAreaWork* work;
    s8 i;

    current = ((s8 (*)(void))my_player_no)();
    if ((eft_def_flags(slot) & 2) != 0) {
        slot->armed_0x02 = 1;
        slot->work_0x03 = current;
        return;
    }
    slot->work_0x03 = -1;
    work = (EmAreaWork*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    for (i = 0; i < (u16)count; i++, work++) {
        if (work->active != 0 && em_pl_slot_leaving_ck(work->field_0x008) == 0) {
            slot->work_0x03 = i;
            break;
        }
    }
    if (slot->work_0x03 == current || slot->work_0x03 == -1) {
        slot->armed_0x02 = 1;
    } else {
        slot->armed_0x02 = 0;
    }
    slot->field_0x0C = slot->work_0x03;
}

/* Picks the move work the slot follows: the free-slot scan, then the forward search for the record
 * whose area the slot was seeded with. */
extern "C" void eft_slot_work_update(EftSlot* slot) {
    u32 count;
    EmAreaWork* work;
    s8 current = ((s8 (*)(void))my_player_no)();
    EmAreaWork* record;
    u16 i;
    u8 next;
    u32 found;

    if ((eft_def_flags(slot) & 2) != 0) {
        return;
    }
    if (isServerSelectState() != 1) {
        return;
    }
    work = (EmAreaWork*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    if (eft_slot_armed_ck(slot) == 0) {
        if (isReadyCountOne() == 1) {
            found = 0;
            if (work[slot->work_0x03].active == 0 || em_pl_slot_leaving_ck(slot->work_0x03) != 0) {
                for (i = 0; i < (u16)count; i++) {
                    if (work[i].active != 0 && em_pl_slot_leaving_ck((u8)i) == 0) {
                        slot->work_0x03 = i;
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 1) {
                if (slot->work_0x03 == current) {
                    slot->armed_0x02 = 1;
                    eft_net_send(slot, 5, 0);
                    return;
                }
                eft_net_send(slot, 7, (u8)slot->work_0x03);
                return;
            }
            if (((u32 (*)(void))emc_host_ck)() == 1) {
                eft_net_send(slot, 7, (u8)slot->work_0x03);
            }
        } else {
            if (slot->field_0x06 < 900) {
                slot->field_0x06++;
                return;
            }
            eft_net_send(slot, 8, 0);
        }
    } else {
        if (((u32 (*)(void))emc_host_ck)() == 1) {
            slot->armed_0x02 = 1;
            eft_net_send(slot, 5, 0);
            return;
        }
        if (slot->field_0x14 != work[slot->work_0x03].area_no) {
            next = slot->work_0x03 + 1;
            for (i = 0; i < (u16)count - 1; i++, next++) {
                if (next >= (u16)count) {
                    next = 0;
                }
                record = &work[next];
                if (record->active != 0 && em_pl_slot_leaving_ck(next) == 0 && slot->field_0x14 == record->area_no) {
                    slot->work_0x03 = next;
                    slot->armed_0x02 = 0;
                    eft_net_send(slot, 5, 0);
                    return;
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
    for (i = 0; i < count; i++, work++) {
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
                if (slot->field_0x17 == other->field_0x17 && slot->field_0x17 != 0xFF && other->field_0x17 != 0xFF) {
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
                if (slot->field_0x17 == other->field_0x17 && slot->field_0x17 != 0xFF && other->field_0x17 != 0xFF) {
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
    s32 i;
    EftTargetRecord* record;
    u16 count = get_move_work_max(3);

    if (eft_slot_armed_ck(slot) == 0) {
        return 0;
    }
    for (i = 0; i < (s32)count; i++, work++) {
        if (eft_work_wide_ck(slot, work) == 1) {
            return 0;
        }
    }
    record = (EftTargetRecord*)em_area_entry_tbl;
    for (i = 0; i < 128; i++, record++) {
        if (eft_target_wide_ck(slot, record) == 1 && record->index_0x0A == get_now_areano()) {
            return 1;
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
            eft_slot_kind_shape_move(slot);
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
    f32 dist[32];
    s32 order[32];
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
    switch (slot->key_0x00) {
    case 2:
    case 8:
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
                work = works[i];
                if (work->team == def->team_0x01) {
                    work->state_0x46E = 0;
                    taken[i] = 1;
                } else {
                    work->state_0x46E = k;
                    if (work->state_0x46F != 0xFF) {
                        slot->field_0x38++;
                    }
                    k++;
                }
            }
        }
        break;
    default:
        for (i = 0; i < n; i++) {
            work = works[i];
            work->state_0x46E = i;
            if (work->state_0x46F != 0xFF) {
                slot->field_0x38++;
            }
        }
        break;
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
    EftSlot* slot = eft_slot_pool;
    EftTargetRecord* record;
    u8 done[10];
    s32 i;
    _ENEMY_WORK* base = (_ENEMY_WORK*)get_move_work_adrs(3);
    _ENEMY_WORK* list[32];
    u16 max = get_move_work_max(3);
    _ENEMY_WORK* work;
    EftSlot* other;
    s32 j;
    u8 key;
    s32 k;

    for (i = 0; i < 10; i++) {
        done[i] = 0;
    }
    for (i = 0; i < 10; i++, slot++) {
        if (done[i] == 0) {
            done[i] = 1;
            if (slot->key_0x00 == 0 || slot->field_0x16 == 1) {
                continue;
            }
            slot->field_0x0E = 0;
            for (work = base, j = 0; j < max; j++, work++) {
                if (eft_work_wide_ck(slot, work) == 1) {
                    work->state_0x46D = slot->field_0x0E;
                    list[slot->field_0x0E] = work;
                    slot->field_0x0E++;
                    key = work->field_0x46C;
                    if (key != slot->key_0x01) {
                        for (other = slot + 1, k = j + 1; k < 10; k++, other++) {
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
            for (record = (EftTargetRecord*)em_area_entry_tbl, j = 0; j < 0x80; j++, record++) {
                if (eft_target_wide_ck(slot, record) == 1) {
                    slot->field_0x0E++;
                    key = record->key_0x09;
                    if (key != slot->key_0x01) {
                        for (other = slot + 1, k = j + 1; k < 10; k++, other++) {
                            if (other->key_0x00 != slot->key_0x00 && key == other->key_0x01) {
                                done[k] = 1;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            for (k = 0, other = eft_slot_pool; k < i; k++, other++) {
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
    EftSlot* slot = eft_slot_pool;
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
            eft_slot_mode_set_local(slot, 0xFF, 0);
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
extern "C" void eft_slot_mode_set(EftSlot* slot, u8 mode, u8 value, u8 synced) {
    if (synced == 0 || eft_slot_live_ck(slot) == 1) {
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

/* The two thin wrappers: the synced one sends the pair to the peers, the local one only records it. */
extern "C" void eft_slot_mode_set_sync(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 1);
}

extern "C" void eft_slot_mode_set_local(void* slot, u8 mode, u8 value) {
    eft_slot_mode_set((EftSlot*)slot, mode, value, 0);
}

/* Maps a (mode, kind, value) triple onto the pair the slot records, then stamps it. */
extern "C" void eft_slot_mode_set_map(void* slot, u8 mode, u8 kind, u8 value, u8 local) {
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
    if (local == 0) {
        eft_slot_mode_set_sync(slot, pair_mode, pair_value);
        return;
    }
    eft_slot_mode_set_local(slot, pair_mode, pair_value);
}

/* Stamps a (kind, a, b) triple on the slot, after the enemy work's record rewrites it. */
extern "C" void eft_slot_mode_set_work(void* slot, _ENEMY_WORK* work, u8 kind, u8 a, u8 b, u8 local) {
    if (kind <= 4) {
        em_target_select(work, &kind, &a, &b);
        eft_slot_mode_set_map(slot, kind, a, b, local);
        return;
    }
    if (local == 0) {
        eft_slot_mode_set_sync(slot, 0, 0xFF);
        return;
    }
    eft_slot_mode_set_local(slot, 0, 0xFF);
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
extern "C" void eft_slot_kind_shape_move(EftSlot* slot)
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
    u32 max = get_move_work_max(3);
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u16 i;

    for (i = 0; i < (u16)max; i++, work++) {
        if ((u32)em_work_die_ck(work) != 1 && slot->field_0x14 == work->area_no && (work->field_0x1C8 & 1) != 0 &&
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
        if ((u32)em_work_die_ck(work) == 1 || work->team != def->team_0x01) {
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
extern "C" u8 eft_slot_entry3_has(EftSlot* slot, u8 key, u8 value)
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
    _ENEMY_WORK* work;
    u32 max;
    s32 i;
    u8 result = 2;
    EftDef* def = eft_def_get(slot);

    if (def != NULL && def->mask_0x02 != 0) {
        work = (_ENEMY_WORK*)get_move_work_adrs(3);
        max = get_move_work_max(3);
        for (i = 0; i < (u16)max; i++, work++) {
            if ((u32)em_work_die_ck(work) == 1 || work->action == 11) {
                continue;
            }
            if ((s32)strict == 1 && work->team == 3 && ((u32 (*)(_ENEMY_WORK*))em003_skip_ck)(work) == 1) {
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
    u8* area;
    s32 i;
    u8* list;

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

/* 0x80346268 (0x4C): records the marks and, when they changed, sends the slot's mark message. */
extern "C" void eft_slot_marks_set_sync(EftSlot* slot, u8 a, u8 b)
{
    if (eft_slot_marks_set(slot, a, b) == 1) {
        eft_net_send(slot, 6, 0);
    }
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
        switch (slot->field_0x14) {
        case 2:
            if (em_phase_get((_ENEMY_WORK*)slot->field_0x18) == 1) {
                return 0;
            }
            break;
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
    u8 shape;
    u32 max = get_move_work_max(3);
    _ENEMY_WORK* bound = (_ENEMY_WORK*)slot->field_0x18;
    s32 i;
    u8 index = 0;
    u8 state;
    _ENEMY_WORK* target = NULL;

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
            for (i = 0; i < (u16)max; work++, i++) {
                if ((u32)em_work_die_ck(work) != 1 && work->action != 11 &&
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
            } else if ((u32)em_action_mid_ck(bound) == 1) {
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
            eft_slot_mode_set_local(slot, 3, work->field_0x9F7);
        }
        break;
    case 1:
        if (work != NULL) {
            eft_slot_mode_set_work(slot, work, 4, 0, 0, 0);
        }
        break;
    }
}
#pragma pop

/* Moves the slot's hit shape to the block its key pair names (the definition's arm). */
extern "C" void eft_slot_key_shape_move(EftSlot* slot)
{
    eft_slot_kind_shape_move(slot);
}

/* Returns the slot's escape timer: 6300 frames, 3600 in map kind 1's area 1. */
extern "C" s32 eft_slot_escape_time_get(EftSlot* slot)
{
    s32 frames = 6300;

    switch (stage_map_kind_get(slot->field_0x13)) {
    case 1:
        switch (slot->field_0x14) {
        case 1:
            frames = 3600;
            break;
        }
        break;
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
    _ENEMY_WORK* work;
    s16 frames;
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
    u8 state;
    u32 max;
    u32 all_here;
    _ENEMY_WORK* works[32];
    u8 nw;
    u8 key;
    u8 nr;
    u8 found;
    EftTargetRecord* records[32];
    u8 here = 0;
    u8 force;
    u8 area;
    u8 flagged = 0;
    u8 pick;
    s32 i;

    for (i = 0; i < 32; i++) {
        works[i] = NULL;
    }
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    nw = 0;
    nr = 0;
    for (i = 0; i < (u16)max; i++, work++) {
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
        if (slot->field_0x3B == slot->field_0x14 && (u32)(slot->field_0x3A - 1) <= 1) {
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
        switch (state) {
        case 0:
        case 2:
            if (all_here == 1 && slot->field_0x34 > (s16)eft_slot_escape_time_get(slot) && flagged != 1) {
                state = 5;
            }
            break;
        }
        break;
    case 5:
        if (slot->field_0x3B == slot->field_0x14 && (u32)(slot->field_0x3A - 1) <= 1) {
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
        if (slot->field_0x3B != slot->field_0x14 && (u32)(slot->field_0x3A - 1) <= 1) {
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
            eft_slot_mode_set_sync(slot, 3, area);
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
            eft_slot_mode_set_local(slot, 3, eft_slot_flee_area_get(slot));
        }
        break;
    case 5:
        if (previous == 0) {
            slot->field_0x0B = 0;
        } else {
            slot->field_0x0B = 1;
        }
        eft_slot_mode_set_local(slot, 3, eft_slot_flee_area_get(slot));
        break;
    }
}

/* Binds the slot to the first enemy of its area its wider test accepts (when it is armed or its net timer ran out),
 * and resends it once the timer runs out. */
extern "C" void eft_slot_area_bind(EftSlot* slot)
{
    _ENEMY_WORK* work = (_ENEMY_WORK*)get_move_work_adrs(3);
    u32 max = get_move_work_max(3);
    s32 i;

    if (slot->field_0x1E >= 60) {
        slot->field_0x1E = -1;
    }
    slot->field_0x18 = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < (u16)max; i++, work++) {
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
    eft_slot_mode_set_local(slot, 4, slot->field_0x36);
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
    u32 max;
    u8 i;
    u8 count;
    u8 base;
    s32 state;
    u8 index;
    u8 n;
    EftShapeBlock* block;

    slot->field_0x18 = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < (u16)max; i++, work++) {
        if ((u32)em_work_die_ck(work) != 1 && slot->field_0x14 == work->area_no && eft_work_wide_ck(slot, work) == 1) {
            slot->field_0x18 = (u32)work;
            break;
        }
    }
    if (slot->field_0x18 == 0) {
        return;
    }
    eft_slot_link_marks_get(slot, &count, &base);
    if (count == 0 || base == 0) {
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
    if (slot->field_0x08 != (u8)state) {
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
                eft_slot_mode_set_sync(slot, 4, slot->field_0x36);
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
                eft_slot_mode_set_sync(slot, 4, index);
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
    u8 index;
    u8 best_free;
    VEC3 center;
    VEC3 best_pos;
    u8 free;
    f32 best;
    u8 take;
    u8 count;
    u8 base;
    EftShapeBlock* block;
    f32 dist;
    u8 i;

    VEC3_ctor(&center);
    VEC3_ctor(&best_pos);
    slot->field_0x0B = 0;
    eft_slot_link_marks_get(slot, &count, &base);
    i = count;
    if (i == 0 || base == 0) {
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
        eft_slot_mode_set_local(slot, 4, index);
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
        eft_slot_mode_set_local(slot, 4, index);
        break;
    }
}

/* Moves the slot's hit shape to block 0, requests the matching mode and sets its grade 2. */
extern "C" void eft_slot_shape_block0_set(EftSlot* slot)
{
    slot->field_0x36 = 0;
    eft_slot_shape_move(slot, 0);
    eft_slot_mode_set_local(slot, 4, slot->field_0x36);
    slot->field_0x39 = 2;
}

#pragma push
#pragma optimization_level 4

/* One frame of a roaming slot: binds the enemy it tracks, rates its area (1 when its own enemy or a matching record
 * is in it, 5 when all of them are and the slot is old, 0 when they left), requests the state, then rolls the next
 * block when an enemy reaches the current one. */
extern "C" void eft_slot_roam_step(EftSlot* slot)
{
    s32 i;
    s32 all_here;
    EftTargetRecord* record = (EftTargetRecord*)em_area_entry_tbl;
    _ENEMY_WORK* works[32];
    EftTargetRecord* records[32];
    u8 nw;
    u8 nr;
    u8 gone;
    u32 max;
    u8 found;
    _ENEMY_WORK* work;
    u8 key;
    u8 state;
    u8 here = 0;
    u8 index = 0;
    u8 roll;
    _ENEMY_WORK* target = NULL;

    slot->field_0x18 = 0;
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < (u16)max; i++, work++) {
        if ((u32)em_work_die_ck(work) != 1 && eft_work_wide_ck(slot, work) == 1) {
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
    for (i = 0; i < (u16)max; i++, work++) {
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
                    eft_slot_mode_set_sync(slot, 4, slot->field_0x36);
                    break;
                }
            }
        }
        break;
    case 1:
        if (slot->field_0x1C > 90) {
            for (i = 0; i < nw; i++) {
                if ((u32)em_work_die_ck(works[i]) != 1 && works[i]->action != 11 &&
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
    u32 index;
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
                if ((u8)index == 0xFF) {
                    best = dist;
                    index = i;
                } else if (best > dist) {
                    best = dist;
                    index = i;
                }
            }
            eft_slot_shape_move(slot, index);
            slot->field_0x36 = index;
            eft_slot_mode_set_local(slot, 4, index);
        } else {
            eft_slot_shape_move(slot, slot->field_0x36);
            eft_slot_mode_set_local(slot, 4, slot->field_0x36);
        }
        break;
    case 1:
        if (work != NULL) {
            eft_slot_mode_set_work(slot, work, 0, 0, 0, 0);
        } else if (slot->field_0x05 == 1) {
            eft_slot_mode_set_local(slot, 0, 0xFF);
        }
        break;
    case 5:
        slot->field_0x0B = 0;
        eft_slot_mode_set_local(slot, 3, eft_slot_flee_area_get(slot));
        break;
    }
}

/* Puts the slot's hit shape on block 11 and requests the matching mode. */
extern "C" void eft_slot_shape_block11_set(EftSlot* slot)
{
    slot->field_0x36 = 11;
    slot->field_0x22 = 0;
    eft_slot_shape_move(slot, 11);
    eft_slot_mode_set_local(slot, 4, slot->field_0x36);
    slot->field_0x0B = 0;
}

/* One frame of an arena slot: binds the enemies of its area, rolls its pattern (60 % wander, 20 % chase, 20 % rest)
 * and runs it: a random side block, the block nearest the slot until the enemy charges, a random corner until every
 * enemy rests, or a random end block. */
extern "C" void eft_slot_arena_step(EftSlot* slot)
{
    VEC3 center;
    _ENEMY_WORK* works[32];
    s32 i;
    _ENEMY_WORK* work;
    u32 max;
    u8 found;
    _ENEMY_WORK* bound = NULL;
    u8 mode;
    u8 n;
    f32 best;
    u16 roll;
    u8 index;
    f32 dist;

    VEC3_ctor(&center);
    work = (_ENEMY_WORK*)get_move_work_adrs(3);
    max = get_move_work_max(3);
    n = 0;
    for (i = 0; i < (u16)max; i++, work++) {
        if ((u32)em_work_die_ck(work) != 1 && eft_work_wide_ck(slot, work) != 0 && work->area_no == slot->field_0x14) {
            if (bound == NULL) {
                bound = work;
                slot->field_0x18 = (u32)work;
            }
            works[n++] = work;
        }
    }
    if (n == 0 || bound == NULL) {
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
            if (mode == slot->field_0x0B) {
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
            eft_slot_mode_set_local(slot, 4, slot->field_0x36);
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
            eft_slot_mode_set_local(slot, 4, slot->field_0x36);
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
            eft_slot_mode_set_local(slot, 4, slot->field_0x36);
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
            eft_slot_mode_set_local(slot, 4, slot->field_0x36);
        }
        break;
    }
}
#pragma pop

/* 0x803489F0 (0x4): the state-change entry of the definition `eft_slot_arena_step` drives: it does nothing. */
extern "C" void eft_slot_arena_state_noop(EftSlot* slot, u8 previous, _ENEMY_WORK* work)
{
}
