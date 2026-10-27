/*
 * stage/stg_w.cpp - the stage work block `stage_w` (.bss 0x806B87C0, 0x2FE0 B): the area load slots and their
 * models, the area lights and fog, the breakable parts and their joint scatter, the warp zones, the area colour
 * cycle and the area effect starts.
 *
 * RANGE. `.text` 0x802AD9C0..0x802B5640 (131 functions); data, bss, sdata, sdata2, extab and extabindex as the unit's
 *   block in config/RMHE08/splits.txt.  The functions follow retail's address order (the extabindex records pair);
 *   every function reads the block as the one `StageWork` class (`stage/stage_work.h`); the functions after
 *   stage_colour_blend sit inside `namespace view_fn_802B2AA0`, which reads `stage/fn_802B2AA0.h`'s callee
 *   declarations (extern "C", so the symbols are unchanged).  Functions called above their definition are
 *   declared once after the type and data declarations.
 * FLAGS. `cflags_main`; `#pragma optimization_level 4` over the file (playbook 108: stage_part_break, the scatter
 *   step, stage_parts_seat, stage_parts_check, stage_part_joints_hide and three more rows rise, none falls);
 *   `#pragma peephole off` over the file (retail keeps the unfused `clrlwi`+`slwi`/`mulli`, the
 *   duplicated `lis`, and the `li`+`psq_lx` epilogue that the peephole pass folds); `#pragma fp_contract off` over the
 *   file (the target object has no fused multiply-add; with contraction on, the colour and scatter rows fuse);
 *   `#pragma pool_data off` over the file (retail's `__sinit` addresses each `.bss` object by its own symbol, playbook 43).
 * NAMES. Module from the range's own entry points (`get_stg_w`, `get_now_mapno`, `get_worldworld_pos`, ...): no
 *   `__FILE__` string covers the range.  GUESS (from each body and its callers): every `stage_*`/`stg_*` function
 *   and data name of the range that is not a mangled name, the class names (`StageWork`, `StageSlot`, `StageArea`,
 *   `StageAreaRow`, `StagePart`, `StageEftSeat`, whose constructor manglings name the map rows), and the record field
 *   names; the
 *   foreign callee names proposed in config/RMHE08/name_registry.jsonl by lane stage-stg-w (`eft_res_file_load`,
 *   `map_com_eft_file_tbl`, `map_com_efm_file_tbl`, ...) are GUESSes too.
 * RESIDUALS.
 *   - stage_map_obj_load reads `map_com_eft_name_ptr`/`eft_map_common_mdl_name_ptr` (.sbss 0x80794A88/0x80794A8C,
 *     between ef/eft_res.cpp's and enemy/enemy_control.cpp's claims; no registered unit owns them, rule 12); retail
 *     keeps `mapno * 4` live across the eft block (`_savegpr_24`), ours recomputes it.
 *   - Data: `.data`, `.sdata`, `.bss`, `.sbss` and `.ctors` are emitted byte for byte.  Each section of ours ends at
 *     its last object (`.bss` 12316, `.sdata` 461, `.sbss` 4 B), short of the split's alignment gap.
 *   - `.sdata2`: the float pool is MWCC's own, from literals (140 of 144 B); retail allocates the unsigned
 *     int->float magic (0x4330000000000000) between 1.0f and -1.0f (both first used by stage_area_models_seat),
 *     ours after 360.0f (its first user, stage_area_point_get, in `.text` order), and pads the pool's end to 8.
 *   - stage_area_models_seat: retail keeps six zero registers (r19..r31) live across the loop and `i * 4` as an
 *     offset (ours strength-reduces it to a pointer); `opt_common_subs off` separates the zeros but costs more.
 *   - stage_part_break: retail loads the koware degrees before the argument registers and indexes the koware table
 *     with two `slwi 2`; a `f32*` table view gives the shifts but not the schedule.
 *   - stage_area_visibility_build: retail branches from the matched-event test straight to the next kept joint;
 *     the `hit` flag adds its stores (an inline match helper is worse; the jump needs a `goto`).
 *   - stage_demo_markers_place: retail computes the marker point (offsets, count, mod) before `&pos`; every argument
 *     spelling and an inline helper give `&pos` first.
 *   - stage_colour_cycle_update: the second and third colour lerps colour their float registers in order, the first
 *     interleaved; one inline helper gives all three the first one's colouring.
 *   - stage_area_load: retail narrows `areano` after the stage_map_group_get call and reuses it as the r7 argument;
 *     its id buffer size (0x28) is read from the frame.
 *   - stage_parts_seat / stage_part_scatter_step: retail keeps `part + k * 12` live across mhchar_res_node_get, ours
 *     re-derives it after the call.
 *   - stage_eft_strips_spawn (the two hoisted float constants swap f30/f31), stage_part_bodies_set (r27..r29
 *     permuted), stage_area_model_load_req, stage_part_joints_hide, stage_parts_check, stage_area_timer_arm,
 *     stage_area_colour_update (a 4-byte stack slot): register allocation and scheduling.
 * SHAPES.
 *   - `stage_w` is a `StageWork` object and the three entry points `StageVec3` objects, so `__sinit` constructs them.
 *     The member classes with constructors (`StageSlot`, `StageArea`, `StageAreaRow`, `StagePart`, `StageEftSeat` and
 *     `StageWork` itself) keep them out of line with `__attribute__((never_inline))`, which emits them after `__sinit`
 *     in retail's order (playbook 193, 242); the leaf constructors (`StageVec3`, `StageChar`, `StageEftSphere`,
 *     `StageEftCapsule`) are inline wrappers of the out-of-line `VEC3_ctor`/`setVec3`/`mhchar_construct`/
 *     `eft_vec_ctor`/`eft_capsule_ctor` calls retail makes.
 *   - A function that reads several `stage_w` fields takes a local `StageWork* w = SW` first; retail materialises
 *     the block once (`lis`/`addi` into a saved register) and a direct `SW->` spelling re-materialises it per use.
 *     stage_work_init additionally writes its first field through `SW->` (the `stb rN,stage_w@l(rM)` form).
 *   - Loop counters over byte-indexed tables are `u8` (stage_parts_break_all, stage_part_joints_hide): an `s32`
 *     counter is fully unrolled, a `u8` one keeps retail's 4x unroll with the counter live.
 *   - A two-slot lookup is a two-iteration loop (stage_area_slot_get, stage_area_free_slot_get): the unrolled loop
 *     hoists the parameter narrowing that two written-out tests repeat.
 *   - A `u8` compared with `cmpwi` in retail is a switch case or a `(s32)` cast (get_stg_weapon_work,
 *     stage_water_area_ck, stg_ground_impact_set); a switch whose cases are all `cmplwi` switches on `(u32)`
 *     (stage_area_effects_start, playbook 207).
 *   - The scatter definition tables are flat `f32`/`u16` runs indexed `i * 3 + n` (retail's `slwi`/`subf`/`slwi`).
 *   - A shared tail reached from two branches and skipped from a third is a `do { ... break; ... } while (0)`
 *     (stage_light_update, stage_area_colour_update).
 *   - get_worldworld_pos returns a copy-constructible `VEC3` view so the result is built in the return slot
 *     (playbook 176).
 *   - stage_land_grid_load_done indexes the pack entries as `i * 2` / `i * 2 + 1`, not a second induction variable.
 *   - The `shell_set_func_ptr` job-table callers (stg_barrel_bomb_place, stg_trap_place, stg_slinger_special_fire,
 *     stg_ground_impact_set, stage_demo_markers_place) are bracketed by `#pragma opt_common_subs off` (playbook 208):
 *     retail loads the table pointer after the arguments.
 *   - stg_ground_impact_set's joint order is two 2-iteration loops (the hoisted `li r0,0`/`li r0,2`) and its joint
 *     byte is a `u8` local cast to `s8` at each use; stage_blend_table_get returns from each case, default first.
 *   - stage_parts_check and stage_part_bodies_set index `w->bodies[b]` with a `u8 b` read before the calls (retail
 *     keeps `b * 64`, not a body pointer); StageChar's animation count is a `u32` (`lwz` + `cmplw`).
 *   - A float argument retail loads before the integer ones is either a local read first (stage_light_blend_set's
 *     `frame`) or a float-third parameter in the owner's prototype (eft_area_part_break_set).
 *   - stage_area_load's effect-slot assignment is the inline helper stage_area_eft_assign (its early returns are
 *     retail's exits; no `goto`).
 */

#include "types.h"

#include "pl.h"

#include "g3d/g3d_obj.h"

#include "nw4r/math.h"

#include "gx.h"

#include "g3d/g3d_resmat.h"             /* nw4r::g3d::ResMdl (rule 2) */

#include "quest/quest_item_slot.h"   /* quest_id_head_ck, quest_id_tail_ck (rule 2) */

#include "lobby/lb_quest_screen.h"   /* the quest-work time and clock accessors (rule 2) */

#include "ef.h"

#include "g3d/g3d_calcview.h"

#include "ef/effect.h"                /* eft_vec_ctor (rule 2) */

#include "ef/eft_capsule_ctor.h"      /* eft_capsule_ctor (rule 2) */

#include "lobby/mhchar_construct.h"   /* mhchar_construct (rule 2) */

/* The stage kind of each map id (`stage_map_kind_get`); 0xFF maps to itself. */
u8 stg_map_kind_table[23] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x0B, 0xFF, 0xFF,
};

/* The unit's pooled constants the map already names (playbook 29). */









/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

#include "stage/stage_work.h"            /* StageWork and the records it holds (rule 1) */

/* The stage-work block (0x806B87C0); `get_stg_w` is its accessor. */
extern "C" StageWork stage_w;

#define SW (&stage_w)

typedef struct StageResFile StageResFile;

/* A record `stage_map_group_get` selects: one loadable effect/resource descriptor per id.  Only the fields
 * this range reads are named; the size is traced from the 0x24/0x28 bytes the accessors touch. */
typedef struct StageElem {
    /* +0x00 */ u32 size;           /* the area model file and its size */
    /* +0x04 */ char* name;
    /* +0x08 */ u32 size2;          /* the second model file and its size */
    /* +0x0C */ char* name2;
    /* +0x10 */ u8 pad_0x10[0x08];
    /* +0x18 */ VEC3 entry_pos;
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 field_0x28;
    /* +0x29 */ u8 pad_0x29[0x03];
    /* +0x2C */ s32 eft_col[2];
    /* +0x34 */ f32 field_0x34;
    /* +0x38 */ u8 field_0x38;
    /* +0x39 */ u8 pad_0x39[0x03];
} StageElem; /* size: 0x3C */

/* One area origin of a map: the area's x/z offset in world space. */
typedef struct StageAreaOrigin {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 z;
    /* +0x08 */ u8 pad_0x08[0x10];
} StageAreaOrigin; /* size: 0x18 */

/* The map's screen record: the projection the quest screen uses and the area origins. */
typedef struct StageMapInfo {
    /* +0x00 */ u8 screen[0x10];     /* the `QuestScreen` projection */
    /* +0x10 */ StageAreaOrigin* origins;
} StageMapInfo; /* size: 0x14 */

/* A map's start point: its position and facing. */
typedef struct StageStart {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ s32 angle;
} StageStart; /* size: 0x10 */

/* The head `stage_map_group_get` returns: `kind` at +0, a word at +4 that indexes an array of descriptors,
 * an inline word table at +0x0C, a byte-indexed word array at +0x18 and a 16-byte-stride base at
 * +0x14. */
typedef struct StageGroup {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01[0x03];
    /* +0x04 */ StageElem** elems;
    /* +0x08 */ StageMapInfo* map;  /* screen_projection_get hands out its head */
    /* +0x0C */ struct StageZone** zones; /* the warp zone list of each area */
    /* +0x10 */ char** area_files; /* each area's record file */
    /* +0x14 */ StagePartDef** part_defs; /* four per area */
    /* +0x18 */ u32* field_0x18;
    /* +0x1C */ StageStart* start;
} StageGroup; /* size: 0x20 */

/* ------------------------------------------------------------------------------------------------ */
/* foreign declarations                                                                              */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void* stage_map_group_get(u8 id);

extern "C" u32 stage_map_kind_get(u32 kind);

extern "C" bool stg_area_flag_ck(u8 index, u8 bit);

extern "C" u32 stage_map_area_count_get(u8 id);

extern "C" void stage_part_bodies_set(StageWork* w, u8 areano);

extern "C" void stage_parts_check(StageWork* w, u8 areano);

extern "C" void stage_area_slot_reset(u8 index);

extern "C" void stage_arena_land_grid_load(u8 a, u8 b);

extern "C" void stage_part_break(StageWork* w, u8 index);

extern "C" void* stage_res_offset_get(void* base, s32 off);

extern "C" void* stage_res_body_get(void* self);

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* The record `stage_char_res_body_get` returns the body of. */
typedef struct StageObject {
    /* +0x000 */ u8 head[0x114];
    /* +0x114 */ u8 body[];
} StageObject; /* size: 0x114 (flexible body tail) */

/* The header of the resource `fn_800700C0` resolves (`stage_res_body_get` reads its size at +0x48). */
typedef struct ResolverView {
    /* +0x00 */ u8 pad_0x00[0x48];
    /* +0x48 */ s32 size;
} ResolverView; /* size: 0x4C */

#include "mh3_pad.h" /* the owner header (rule 2) */

#include "sound/mhchar.h"

#include "sound/fn_800D7F54.h"

#include "unsplit/ef.h"

#include "unsplit/g3d.h"

#include "unsplit/Pl.h"

#include "unsplit/unknown.h"

#include "Pl/Pl_master_ck.h"

#include "fn_8004CAD8.h"

#include "menu/menu_item.h"

#include "ef/eft028_set_koware.h"

#include "g3d/g3d_resnode.h"

#include "mh3_pad/system_w.h"

#include "ef/fn_800CDB2C.h"

#pragma fp_contract off
#pragma peephole off
#pragma optimization_level 4
#pragma pool_data off

/* ---- this unit's later functions, declared ahead of their callers ---- */
extern "C" {
u32 stage_warp_zone_check(void);
void stage_warp_zone_check_split(void);
void stage_area_anim_cb0(void);
void stage_area_anim_cb1(void);
void stage_area_timers_update(StageWork* st);
void stage_anim_rate_update(StageWork* st);
void stage_area_break_effects_spawn(StageWork* st);
void stage_joint_se_update(StageWork* st);
void stage_demo_markers_place(StageWork* st);
void stg_place_timers_update(void);
u32 stage_area_alt_model_get(u8 mapno, u8 areano);
void stage_colour_cycle_update(void);
void stage_light_blend_set(StageLightSet* set, VEC3* dir, u8 store);
s8 stage_area_slot_get(u8 mapno, u8 areano);
void stage_area_effects_start(u8 mapno, u8 areano);
void stage_parts_seat(StageWork* w);
void stage_area_kind_joints_hide(StageWork* w, StageChar* chr);
void stage_area_joint_visibility_set(StageWork* w, StageChar* chr, u8 index);
void stage_area_light_setup(StageWork* w);
void stage_area_light_apply(void);
void stage_area_models_seat(void);
void stage_light_set_copy(StageLightSet* dst, const StageLightSet* src);
u32 stage_area_model_load_req(char* name, u32 size, u8 slot, u8 mapno, u8 areano, u8 kind);
void stage_area_ambient_apply(void);
}

extern "C" {
void stage_part_scatter_step(StageWork* w, StagePart* part, MHchar* chr, u8* joints, u8 kind);
void stage_part_scatter_start(StageWork* w, StagePart* part, MHchar* chr, u8* joints, u8 kind);
void stage_part_joints_hide(StageWork* w, MHchar* chr, u8* ids);
void stage_part_update(StageWork* w, StagePart* part, u8* joints, u8* vis_ids, u8 kind);
}

/* ---- the callees' owner headers (rule 2) ---- */
#include "mh3_pad.h"

#include "ef/system_core.h"

#include "sqrtf_thunk.h"

#include "fn_8004CAD8.h"

#include "ef/eft_res.h"

#include "ef/eft_res_release.h"

#include "Pl/pl_coll.h"

#include "light/light.h"

#include "lobby/lb_menu_pos_tbl.h"

#include "camera/camera_angle_x_get.h"

#include "g3d/g3d_obj.h"

#include "g3d/g3d_scnroot.h"

#include "nw_resource.h"

#include "res_mem_index_get.h"

#include "draw_shape.h"

#include "ef/load_file_req.h"

#include "ef/nw_res_manager.h"

#include "ef/pRoot.h"

#include "mh3_pad/Screen_w.h"

#include "camera/camera_mode_cur_get.h"

#include "stage/shell_set_func_ptr.h"

#include "menu/hit_attack_list_push.h"

#include "ef/get_move_work_adrs.h"

#include "ef/eft052.h"

#include "g3d/g3d_root_model_bind.h"

#include "sound/mhchar_model_release.h"

#include "sound/mhchar_model_create.h"

#include "Pl/pl_act_stage_latch_set.h"

#include "Pl/fn_80288CEC.h"

#include "g3d/g3d_calcworld.h"

#include "lobby/lb_quest_ui.h"

#include "screen_split_ck.h"

#include "light_mode_set.h"

#include "g3d/fog_set_color.h"

#include "g3d/fog_type_set.h"

#include "g3d/res_node_trans_set.h"

#include "ef/system_work_mem_reset.h"

#include "nw_res_stage_free.h"

#include "sound/se_area_part_play.h"

#include "sound/mhchar_draw_callback_set.h"

#include "sound/mhchar_motion_speed_set.h"

#include "ef/eft_area_effects_set.h"

#include "ef/eft_res_bind.h"

#include "ef/eft_scnbox_load.h"

#include "ef/eft004_area_set.h"

#include "ef/eft_area_part_break_set.h"

#include "ef/eft023_area_fx_set.h"

#include "enemy/em_res_file_row_copy.h"

#include "Pl/pl_warp_ready_ck.h"

#include "light/light_area_setup.h"

#include "enemy/em_sub_area_fx_set.h"

#include "lobby/lb_area_fx_set.h"

#include "menu/menu_area_fx_set.h"

#include "map_area_eft_list_tbl.h"

#include "menu/place_model_step.h"

#include "camera/camera_quake_req.h"

#include "lobby/lb_interior_fx_spawn.h"

#include "ef/eft030_set.h"

#include "ef/eft052_set.h"

#include "ef/eft053_set.h"

#include "lobby/lb_sub11_send.h"

#include "Pl/pl_land_grid_a_load.h"

#include "vec3_segment_box_ck.h"

#include "enemy/em_res_node_trans_get.h"

#include "Runtime.PPCEABI.H/memset.h"

#include "Runtime.PPCEABI.H/memcpy.h"

#include "ef/work_mem_free.h"

#include "ef/eft_res_file_load.h"

#include "ef/eft_map_com_proID_tbl_ptr.h"

#include "map_com_eft_file_tbl.h"

/* ---- stage work reset, per-frame driver and area load ---- */

/* The three place-act cooldowns (barrel bomb, trap, slinger shell) are `stg_place_timers`, the band's `PlSlotGate`
 * records (`unsplit/Pl.h`): the armed flag and the start time. */

/* The pooled g3d work a stage model holds; only the byte the model load sets is named. */
typedef struct StageG3dWork {
    /* +0x0 */ u8 pad_0x0[3];
    /* +0x3 */ u8 field_0x3;
} StageG3dWork; /* size: 0x4 (approximate: the touched byte) */

/* The move work `get_move_work_adrs(0)` returns; only the state byte the cooldowns test is named. */
typedef struct StageMoveView {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u16 field_0x04;
    /* +0x06 */ u16 field_0x06;
    /* +0x08 */ u8 pad_0x08[0xF2];
    /* +0xFA */ u8 state;
} StageMoveView; /* size: 0xFB (approximate: the touched byte) */

#include "stage/get_stg_weapon_work.h" /* StageWeaponWork, the placed-weapon record (rule 1) */

/* The placed weapons of each map area `get_stg_weapon_work` hands out. */
extern "C" {

StageWeaponWork stg_weapon_work_6_1[6] = {
    {1, 0, {-280.0f, 248.0f, -124.0f}, 200.0f, 0.0f, 0xC000},
    {1, 1, {280.0f, 248.0f, -124.0f}, 200.0f, 0.0f, 0x4000},
    {3, 0, {-257.0f, 250.0f, 415.0f}, 150.0f, 0.0f, 0xC000},
    {3, 1, {257.0f, 250.0f, 415.0f}, 150.0f, 0.0f, 0x4000},
    {2, 0, {0.0f, 511.0f, -1422.0f}, 180.0f, 800.0f, 0x0},
    {4, 0, {0.0f, 290.0f, 550.0f}, 150.0f, 0.0f, 0x0},
};

StageWeaponWork stg_weapon_work_6_2[4] = {
    {1, 0, {9675.0f, 248.0f, -124.0f}, 200.0f, 0.0f, 0xC000},
    {3, 0, {9701.0f, 250.0f, 415.0f}, 150.0f, 0.0f, 0xC000},
    {2, 0, {9958.0f, 511.0f, -1422.0f}, 180.0f, 800.0f, 0x0},
    {4, 0, {9958.0f, 290.0f, 550.0f}, 150.0f, 0.0f, 0x0},
};

StageWeaponWork stg_weapon_work_7_3[3] = {
    {5, 0, {-5597.0f, -25020.0f, 4579.0f}, 200.0f, 300.0f, 0x4000},
    {5, 1, {-3208.0f, -26100.0f, -4281.0f}, 200.0f, 300.0f, 0x2CCD},
    {2, 0, {-6289.0f, -24800.0f, -848.0f}, 250.0f, 400.0f, 0x0},
};

StageWeaponWork stg_weapon_work_8_1[1] = {
    {2, 0, {-2625.0f, 400.0f, -3721.0f}, 400.0f, 200.0f, 0x0},
};

StageWeaponWork stg_weapon_work_9_0[1] = {
    {2, 0, {-2625.0f, 400.0f, -3721.0f}, 400.0f, 200.0f, 0x0},
};

StageWeaponWork stg_weapon_work_9_1[1] = {
    {2, 0, {-11.0f, -2377.0f, -2796.0f}, 250.0f, 400.0f, 0x0},
};

StageWeaponWork stg_weapon_work_11_1[1] = {
    {2, 0, {-11.0f, -2377.0f, -2796.0f}, 250.0f, 400.0f, 0x0},
};

}


/* One entry of an area effect list: a terminator/limit word and an enable word. */
typedef struct StageEftEntry {
    /* +0x0 */ u32 limit;
    /* +0x4 */ u32 enabled;
} StageEftEntry; /* size: 0x8 */

/* Assigns the effect slots of area list `list` (the 1-based pro effects) into the caller's `ids` and hands them to
 * the effect resources; a failed assignment releases the slots taken so far. */
static inline void stage_area_eft_assign(StageEftEntry** list_tbl, u8 mapno, u8 areano, u8* ids)
{
    StageEftEntry* list;
    s32 count;
    s32 i;
    u8 id;

    if (list_tbl == 0)
        return;
    list = list_tbl[areano];
    if (list == 0 || list[0].limit == 0)
        return;
    count = 0;
    if (list[1].limit == 0)
        return;
    if (list[1].enabled != 0) {
        id = eft_pro_slot_assign(1, 2, mapno, areano, 1);
        if (id == 0xFF)
            return;
        count = 1;
        ids[0] = id;
    }
    for (i = 2; i < list[i].limit; i++) {
        if (list[i].enabled != 0) {
            id = eft_pro_slot_assign(1, 1, mapno, areano, i);
            if (id == 0xFF) {
                for (i = 0; i < count; i++)
                    eft_res_release(ids[i]);
                return;
            }
            ids[count++] = id;
        }
    }
    ids[count] = 0xFF;
    eft_pro_slot_list_set(0, list_tbl[areano], ids);
}

extern "C" StageVec3 stg_entry_pos_none;

extern "C" StageVec3 stg_entry_pos_menu;

extern "C" StageVec3 stg_entry_pos_work;

/* The returned-vector view of `VEC3`: the inline copy constructor lets MWCC build the result in the return slot
 * (playbook 176); the layout and the mangling are VEC3's. */
/* size: 0xC */
struct StageRetVec3 : VEC3 {
    StageRetVec3() {}
    StageRetVec3(const StageRetVec3& o) { x = o.x; y = o.y; z = o.z; }
};

/* A fixed start point: its position and its facing in degrees. */
typedef struct StagePoint {
    /* +0x00 */ VEC3 pos;
    /* +0x0C */ f32 degrees;
} StagePoint; /* size: 0x10 */

extern "C" StagePoint stg_pit_point_a;

extern "C" StagePoint stg_pit_point_b;

extern "C" StagePoint stg_area_start_a;

extern "C" StagePoint stg_area_start_b;

extern "C" f32 stg_anim_rate_blend;

/* One map resource file: its size and name. */
typedef struct StageResFile {
    /* +0x0 */ u32 size;
    /* +0x4 */ char* name;
} StageResFile; /* size: 0x8 */

/* One warp zone of an area: a cylinder (kind 0) or a box segment (kind 1), and where it warps to. */
typedef struct StageZone {
    /* +0x00 */ u8 dest;           /* 0xFF ends the list */
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ s16 kind;
    /* +0x04 */ VEC3 pos;
    /* +0x10 */ f32 radius;
    /* +0x14 */ f32 height;
    /* +0x18 */ VEC3 pos2;
    /* +0x24 */ VEC3 warp_pos;
    /* +0x30 */ u32 warp_angle;
} StageZone; /* size: 0x34 */

/* One breakable part's scatter definition: the joints' start positions, rotations, a random-spin flag and the
 * fall time. */
typedef struct StageScatterDef {
    /* +0x00 */ f32* pos;         /* the joints' start velocities, three per joint */
    /* +0x04 */ f32* gravity;     /* three per joint */
    /* +0x08 */ f32* damp;        /* three per joint */
    /* +0x0C */ u16* rot;         /* three per joint */
    /* +0x10 */ f32* rot_damp;    /* three per joint */
    /* +0x14 */ f32 floor_y;      /* the scatter ends below it */
    /* +0x18 */ u8 random_spin;
    /* +0x19 */ u8 time;
    /* +0x1A */ u8 pad_0x1A[2];
} StageScatterDef; /* size: 0x1C */

extern "C" StageScatterDef* stg_scatter_defs[];

extern "C" StagePoint stg_koware_points[];

extern "C" u8 stg_scatter_jitter_xz;

extern "C" u8 stg_scatter_jitter_y;

/* The C++-linkage callees the view below calls: declared at global scope so the calls resolve to the map's
 * manglings (a declaration inside the view namespace would mangle the namespace in). */
nw4r::math::VEC3 get_camera_pos(void);

u32 LbCheckKujiraEvent(void);

void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 area, long param);

namespace view_fn_802B2AA0 {

namespace nw4r = ::nw4r;  /* the view headers below reopen it */

/* data keeps its map name (a view namespace would mangle it): the header is read with C linkage */
extern "C" {
#include "stage/fn_802B2AA0.h"
}

#include "stage/stg_w.h"

#include "g3d/g3d_scnroot.h" /* nw4r::g3d::ScnRoot::GetFog (rule 2) */

extern "C" {
/* The shell-set job table pointer (`.sbss` 0x80794B60); an RSO stores it, nothing in the DOL does. */
ShellSetFuncs* shell_set_func_ptr;
}

/* Blends colour `from` towards `to` by `t`, channel by channel. */
static inline u32 stage_colour_lerp(const u32* from, const u32* to, f32 t)
{
    f32 tb;
    f32 fb;
    f32 tg;
    f32 fg;
    f32 tr;
    f32 fr;
    f32 ta;
    f32 fa;
    s32 a;
    s32 r;
    s32 g;
    s32 b;

    fa = (f32)(*from >> 24);
    fr = (f32)((*from >> 16) & 0xFF);
    fg = (f32)((*from >> 8) & 0xFF);
    fb = (f32)(*from & 0xFF);
    ta = (f32)(*to >> 24);
    tr = (f32)((*to >> 16) & 0xFF);
    tg = (f32)((*to >> 8) & 0xFF);
    tb = (f32)(*to & 0xFF);
    fa = fa + t * (ta - fa);
    fr = fr + t * (tr - fr);
    fg = fg + t * (tg - fg);
    fb = fb + t * (tb - fb);
    a = (s32)fa;
    r = (s32)fr;
    g = (s32)fg;
    b = (s32)fb;
    return ((u8)b | ((u8)g << 8)) | (((u8)a << 24) | ((u8)r << 16));
}

}  /* namespace view_fn_802B2AA0 */

/* Forward declarations of the unit's functions that are called above their definition. */
extern "C" void* stage_fog_copy(void* dst, const void* src);
extern "C" void stage_word_copy(void* dst, const void* src);
extern "C" void stage_ambient_copy(StageAmbient* dst, const StageAmbient* src);
extern "C" u32 stage_entry_eft_kind_get(u8 id, u8 j);
extern "C" bool stage_water_enabled_ck();
extern "C" u32 stage_pos_z_low_ck(void* self);
extern "C" StagePartDef** stage_part_defs_get(StageWork* work, u8 j);
extern "C" bool stage_part_def_ck(u8 id, u8 j);
extern "C" void stage_part_bodies_set_all(StageWork* self);
extern "C" void stage_parts_check_all(StageWork* self);
extern "C" void stage_res_node_trans_set(nw4r::g3d::ResNode* node, VEC3* pos);
extern "C" void stage_arena_land_grid_load_req(u32 a, u32 b);
extern "C" void stage_light_reset();
namespace view_fn_802B2AA0 { extern "C" void stage_fog_rec_copy(StageFogRec* dst, const StageFogRec* src); }

/* Returns the stage-work block. */
void* get_stg_w()
{
    return &stage_w;
}

/* Resets the stage work block, its bodies, events, timers and the place-act cooldowns. */
extern "C" void stage_work_init(void)
{
    StageWork* w = SW;
    PlSlotGate* t = stg_place_timers;
    s32 i;

    SW->init_count = 0;
    w->field_0x0001 = 0;
    w->field_0x0002 = 0;
    w->field_0x0003 = 0;
    for (i = 0; i < 4; i++) {
        w->chars[i] = 0;
        w->anims[i] = 0;
    }
    w->cur_slot = -1;
    w->mapno = 0;
    w->areano = 0;
    stage_light_reset();
    w->flags = 0;
    for (i = 0; i < 4; i++) {
        w->joint_state[0][i] = 0;
        w->joint_state[1][i] = 0;
    }
    w->field_0x2804 = 0.0f;
    setVector3(&w->field_0x2808, 0.0f, 0.0f, 0.0f);
    for (i = 0; i < 15; i++)
        w->bitmask[i] = 0;
    for (i = 0; i < 16; i++) {
        w->bodies[i].active = 0;
        hit_id_list_clear(w->bodies[i].hit_ids, &w->bodies[i].hit_count);
    }
    for (i = 0; i < 8; i++)
        w->vis_ids[i] = 0xFF;
    w->event_pending = 0;
    w->alt_model = 0;
    w->oneshot_flags = 0;
    for (i = 0; i < 3; i++) {
        t[i].flag_0x00 = 0;
        t[i].length_0x04 = 0;
    }
    w->area_mask = 0;
    for (i = 0; i < 16; i++)
        w->area_timers[i] = 0;
    w->demo_countdown = 0;
    place_model_work_init();
}

/* Drives the stage each frame: the area models, their animations and the stage sub-systems. */
extern "C" void stage_frame_update(void)
{
    StageWork* w = SW;
    u16 move_flags = 4;
    s32 i;

    if (pRoot == 0)
        return;
    if (GameMode_ck() != 2) {
        if (PlayMode_ck() != 2)
            stage_warp_zone_check();
        else
            stage_warp_zone_check_split();
    }
    if (w->oneshot_flags & 0x80)
        w->oneshot_flags = 1;
    else
        w->oneshot_flags = 0;
    for (i = 0; i < 4; i++) {
        if (w->chars[i] == 0)
            continue;
        if (stage_water_enabled_ck() == 1 || w->mapno == 9) {
            switch (i) {
            case 0:
                mhchar_draw_callback_set(&w->chars[i]->chr, stage_area_anim_cb0);
                break;
            case 1:
                mhchar_draw_callback_set(&w->chars[i]->chr, stage_area_anim_cb1);
                break;
            default:
                mhchar_draw_callback_set(&w->chars[i]->chr, 0);
                break;
            }
        } else {
            mhchar_draw_callback_set(&w->chars[i]->chr, 0);
        }
        if (i == 3)
            move_flags = 0;
        if (w->mapno == 22 && w->areano == 0 && camera_mode_cur_get() == 1)
            w->chars[i]->chr.field_0xF1 = 1;
        w->chars[i]->chr.field_0xF2 = 1;
        w->chars[i]->chr.move(move_flags);
        if (w->anims[i] != 0)
            w->anims[i]->UpdateFrame();
        g3d_root_model_bind(pRoot, w->chars[i]->chr.field_0x118);
    }
    stage_parts_check_all(w);
    stage_area_timers_update(w);
    stage_anim_rate_update(w);
    stage_area_break_effects_spawn(w);
    stage_joint_se_update(w);
    stg_place_timers_update();
    stage_demo_markers_place(w);
    place_model_step();
}

#pragma opt_common_subs off
/* Arms the barrel-bomb cooldown and places the actor's barrel bomb (the place act's variant 0). */
extern "C" void stg_barrel_bomb_place(_PLW* self)
{
    PlSlotGate* t = &stg_place_timers[0];
    if (t->flag_0x00 == 0) {
        s32 limit;
        t->flag_0x00 = 1;
        limit = quest_time_limit_get();
        t->length_0x04 = limit - quest_time_elapsed_get();
        shell_set_func_ptr->method_0x78(self, 0, shell_set_func_ptr);
        if (Pl_master_ck(self) == 1)
            camera_arena_mode_req();
    }
}
#pragma opt_common_subs reset

#pragma opt_common_subs off
/* Arms the trap cooldown and places the actor's trap (the place act's variant 1). */
extern "C" void stg_trap_place(_PLW* self)
{
    PlSlotGate* t = &stg_place_timers[0];
    if (t[1].flag_0x00 == 0) {
        s32 limit;
        t[1].flag_0x00 = 1;
        limit = quest_time_limit_get();
        t[1].length_0x04 = limit - quest_time_elapsed_get();
        shell_set_func_ptr->method_0x78(self, 1, shell_set_func_ptr);
    }
}
#pragma opt_common_subs reset

#pragma opt_common_subs off
/* Arms the slinger cooldown and fires the slinger's special shell. */
extern "C" void stg_slinger_special_fire(_PLW* self)
{
    PlSlotGate* t = &stg_place_timers[0];
    if (t[2].flag_0x00 == 0) {
        s32 limit;
        t[2].flag_0x00 = 1;
        limit = quest_time_limit_get();
        t[2].length_0x04 = limit - quest_time_elapsed_get();
        shell_set_func_ptr->slinger_special_fire(self, shell_set_func_ptr);
    }
}
#pragma opt_common_subs reset

/* Expires the place-act cooldowns (10 s, 5 s and 3 s) and posts each one's ready message. */
extern "C" void stg_place_timers_update(void)
{
    PlSlotGate* t = &stg_place_timers[0];
    s32 limit = quest_time_limit_get();
    s32 now = limit - quest_time_elapsed_get();
    StageMoveView* move = (StageMoveView*)get_move_work_adrs(0);

    if (move != 0 && move->state == 2) {
        if (t[0].flag_0x00 != 0 && (f32)(now - t[0].length_0x04) >= 10.0f * (60.0f * Screen_w.frame_scale)) {
            t[0].flag_0x00 = 0;
            t[0].length_0x04 = 0;
            hud_item_msg_push(1, 0x2A, 0);
        }
        if (t[1].flag_0x00 != 0 && (f32)(now - t[1].length_0x04) >= 5.0f * (60.0f * Screen_w.frame_scale)) {
            t[1].flag_0x00 = 0;
            t[1].length_0x04 = 0;
            hud_item_msg_push(1, 0x2B, 0);
        }
        if (t[2].flag_0x00 != 0 && (f32)(now - t[2].length_0x04) >= 3.0f * (60.0f * Screen_w.frame_scale)) {
            t[2].flag_0x00 = 0;
            t[2].length_0x04 = 0;
            hud_item_msg_push(1, 0x2C, 0);
        }
    }
}

/* Returns the placed-weapon table of the actor's map and area, and its entry count in `count`. */
StageWeaponWork* get_stg_weapon_work(_PLW* plw, long* count)
{
    *count = 0;
    switch (plw->kind_0x015) {
    case 6:
    case 17:
        switch (plw->area_0x16) {
        case 1:
            *count = 6;
            return stg_weapon_work_6_1;
        case 2:
            *count = 4;
            return stg_weapon_work_6_2;
        }
        break;
    case 7:
    case 18:
        switch (plw->area_0x16) {
        case 3:
            *count = 3;
            return stg_weapon_work_7_3;
        }
        break;
    case 8:
    case 19:
        switch (plw->area_0x16) {
        case 1:
            *count = 1;
            return stg_weapon_work_8_1;
        }
        break;
    case 9:
        switch (plw->area_0x16) {
        case 0:
            *count = 1;
            return stg_weapon_work_9_0;
        case 1:
            *count = 1;
            return stg_weapon_work_9_1;
        }
        break;
    case 11:
    case 20:
        switch (plw->area_0x16) {
        case 1:
            *count = 1;
            return stg_weapon_work_11_1;
        }
        break;
    }
    return 0;
}

/* Applies the fog record `rec` to the scene root's fog 0: type, range and colour. */
extern "C" void stage_fog_apply(StageFogRec* rec)
{
    s32 fog;
    s32 handle;
    u32 colour;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetFog(0);
    stage_fog_copy(&fog, &handle);
    fog_type_set(&fog, rec->type);
    fog_range_set(&fog, rec->start, rec->end);
    colour = rec->colour;
    fog_set_color(&fog, (GXColor*)&colour);
}

/* Copies `src`'s 4-byte word over `dst` and returns `dst`. */
extern "C" void* stage_fog_copy(void* dst, const void* src)
{
    stage_word_copy(dst, src);
    return dst;
}

/* Copies the 4-byte word at `src` over the one at `dst`. */
extern "C" void stage_word_copy(void* dst, const void* src)
{
    *(u32*)dst = *(const u32*)src;
}

/* Completes an area model file load: registers the file and builds the slot's model(s) from it. */
extern "C" void stage_area_model_load_done(char* name, u8* data, u32 size, u32* ctx)
{
    u32 file;
    u32 file2;
    u8 extra[0x4C];
    s32 index;
    StageSlot* slot;
    s32 i;
    StageWork* w;
    u8 mapno;
    u8 areano;
    u8 kind;
    ResEntry* entry;
    s32 count;
    u8 second;

    res_file_ctor(&file, 0);
    w = SW;
    count = 1;
    second = 0;
    mapno = ctx[0];
    areano = ctx[1];
    kind = ctx[3];
    nwAddResource(name, data);
    slot = &w->slots[ctx[2]];
    entry = ckResourceName(name);
    index = res_mem_index_get(name);
    em_res_file_row_copy(extra, (u8*)nw_res_manager + index * 0x4C + 0x3070);
    if (kind == 0 && stage_part_def_ck(mapno, areano) == 1 && (u8)stage_area_alt_model_get(mapno, areano) == 1) {
        count = 2;
        second = 3;
        w->alt_model = 1;
    } else if (kind == 1 && (u8)stage_area_alt_model_get(mapno, areano) == 2) {
        count = 2;
        second = 2;
    }
    if (entry != 0) {
        for (i = 0; i < count; i++) {
            MHchar* chr;
            if (i == 1)
                kind = second;
            res_file_assign(&file, res_file_ctor(&file2, (u32)entry->data));
            chr = &slot->models[kind].chr;
            mhchar_reset(chr);
            mhchar_model_create(chr, &file, 0x2F, 2);
            slot->models[kind].chr.field_0x40 = 0;
            if (slot->models[kind].chr.g3d_0x10C != 0)
                ((StageG3dWork*)slot->models[kind].chr.g3d_0x10C)->field_0x3 = 1;
            slot->res_file[kind] = (s32)data;
            slot->res_id[kind] = entry->index;
            slot->mem_index[kind] = index;
            slot->model_count++;
        }
    }
    slot->mapno = mapno;
    slot->areano = areano;
    slot->index = areano;
    slot->state = 2;
}

/* Requests the load of area model file `name` into slot `slot`'s model `kind`; returns its buffer. */
extern "C" u32 stage_area_model_load_req(char* name, u32 size, u8 slot, u8 mapno, u8 areano, u8 kind)
{
    u32 ctx[5];
    s32 index;
    u32 buf;

    SW->slots[slot].res_id[kind] = -1;
    SW->slots[slot].res_file[kind] = 0;
    SW->slots[slot].mem_index[kind] = -1;
    SW->chars[kind] = 0;
    index = pull_res_mem(name, size, 1);
    if (index < 0)
        return 0;
    ctx[4] = 0;
    ctx[0] = mapno;
    ctx[1] = areano;
    ctx[2] = slot;
    ctx[3] = kind;
    buf = (u32)getResMemAdrs(index);
    load_file_req(name, buf, size, (u32)stage_area_model_load_done, 5, ctx);
    return buf;
}

/* Selects one of the two record groups by whether the id was remapped. */
extern "C" void* stage_map_group_get(u8 id)
{
    u8 mapped = stage_map_kind_get((u8)id);
    return mapped == (u8)id ? SW->map_group_direct : SW->map_group_remapped;
}

/* Loads area `areano` of map `mapno` into slot `slot`: its model files, the play-mode tables and its effect slots. */
extern "C" void stage_area_load(u8 slot, u8 mapno, u8 areano)
{
    StageElem* files = ((StageGroup*)stage_map_group_get(mapno))->elems[areano];
    StageEftEntry** list_tbl;
    StageEftEntry* list;
    u8 ids[0x28];  /* size from retail's 0x60-byte frame (approximate) */
    s32 i;

    if (files->size != 0)
        stage_area_model_load_req(files->name, files->size, slot, mapno, areano, 0);
    if (files->size2 != 0)
        stage_area_model_load_req(files->name2, files->size2, slot, mapno, areano, 1);
    if (GameMode_ck() == 2) {
        system_work_mem_reset();
        pl_coll_table_clear();
        stage_arena_land_grid_load_req(mapno, areano);
    }
    place_model_area_load(mapno, areano);
    stage_area_eft_assign(map_area_eft_list_tbl[mapno], mapno, areano, ids);
    list_tbl = map_area_eft_list2_tbl[mapno];
    if (list_tbl != 0) {
        list = list_tbl[areano];
        if (list != 0) {
            for (i = 0; list[i].limit != 0; i++) {
                if (list[i].enabled != 0)
                    ids[i] = eft_pro_slot_assign(1, 3, mapno, areano, i);
            }
            eft_pro_slot_list_set(3, list_tbl[areano], ids);
        }
    }
}

/* Selects the area's light record (the under-water one in the water views) and applies it to the stage light. */
extern "C" void stage_area_light_apply(void)
{
    StageWork* w = SW;
    StageLight* light;

    if (GameMode_ck() == 2) {
        if (getItemListSelection() == 0)
            light = &w->areas[w->areano].light;
        else
            light = &w->areas[w->areano].light_water;
    } else if (camera_underwater_ck() == 0) {
        light = &w->areas[w->areano].light;
    } else {
        light = &w->areas[w->areano].light_water;
    }
    stage_ambient_copy(&w->ambient, &light->ambient);
    light_mode_set(light->ambient.rgb[0], light->ambient.rgb[1], light->ambient.rgb[2]);
    light_mode_rgb_set(light->ambient.rgb2[0], light->ambient.rgb2[1], light->ambient.rgb2[2]);
    stage_light_set_copy(&w->light, &light->set);
    setVector3(&w->light_dir, light->dir.x, light->dir.y, light->dir.z);
}

/* Copies the light parameter set `src` into `dst` member by member. */
extern "C" void stage_light_set_copy(StageLightSet* dst, const StageLightSet* src)
{
    s32 i;
    dst->kind = src->kind;
    dst->intensity = src->intensity;
    for (i = 0; i < 12; i++)
        dst->colours[i] = src->colours[i];
}

/* Copies six bytes. */
extern "C" void stage_ambient_copy(StageAmbient* dst, const StageAmbient* src)
{
    dst->rgb[0] = src->rgb[0];
    dst->rgb[1] = src->rgb[1];
    dst->rgb[2] = src->rgb[2];
    dst->rgb2[0] = src->rgb2[0];
    dst->rgb2[1] = src->rgb2[1];
    dst->rgb2[2] = src->rgb2[2];
}

/* Re-applies the area light each frame (the blend driver on map 21) and arms the light colours for each view. */
extern "C" void stage_light_update(void)
{
    StageWork* w = SW;
    StageLightSet* set = &w->light;
    StageLightSet tmp;
    StageLight* light;
    u8 player;

    do {
        if (GameMode_ck() == 2) {
            if (w->mapno == 21) {
                stage_colour_cycle_update();
                return;
            }
            if (getItemListSelection() == 0)
                light = &w->areas[w->areano].light;
            else
                light = &w->areas[w->areano].light_water;
        } else {
            if (stage_water_enabled_ck() == 0)
                break;
            if (camera_underwater_ck() == 1)
                light = &w->areas[w->areano].light_water;
            else
                light = &w->areas[w->areano].light;
        }
        w->ambient.rgb[0] = light->ambient.rgb[0];
        w->ambient.rgb[1] = light->ambient.rgb[1];
        w->ambient.rgb[2] = light->ambient.rgb[2];
        light_mode_set(w->ambient.rgb[0], w->ambient.rgb[1], w->ambient.rgb[2]);
        light_mode_rgb_set(w->ambient.rgb2[0], w->ambient.rgb2[1], w->ambient.rgb2[2]);
        stage_light_set_copy(set, &light->set);
    } while (0);
    player = my_player_no();
    my_player_no_set(0);
    stage_light_blend_set(&w->light, &w->light_dir, 0);
    if (screen_split_ck() == 1) {
        my_player_no_set(1);
        if (stage_water_enabled_ck() == 0)
            return;
        if (camera_underwater_ck() == 1)
            light = &w->areas[w->areano].light_water;
        else
            light = &w->areas[w->areano].light;
        stage_light_set_copy(&tmp, &light->set);
        stage_light_blend_set(&tmp, &w->light_dir, 1);
    }
    my_player_no_set(player);
}

/* Applies the current area's fog and ambient colour and seats its three light objects. */
extern "C" void stage_area_light_setup(StageWork* w)
{
    VEC3 pos;
    StageArea* area;
    u32 colour;
    u8 i;

    VEC3_ctor(&pos);
    area = &w->areas[w->areano];
    stage_fog_apply(&area->fog);
    stage_area_ambient_apply();
    for (i = 0; i < 3; i++) {
        u8 index = i == 0 ? 0 : i;
        colour = area->rows[0].words[i];
        light_obj_init(light_obj_area_select(index), (VEC3*)&area->rows[0].vecs[i], *(GXColor*)&colour, 0);
    }
}

/* Applies the current area's ambient colour to light 0. */
extern "C" void stage_area_ambient_apply(void)
{
    set_amblight(0, SW->areas[SW->areano].color);
}

/* Seats the current slot's four models as the stage's models, builds their animations and resets the light
 * state for the area. */
extern "C" void stage_area_models_seat(void)
{
    StageWork* w = SW;
    s32 i;
    u32 frame;
    EftResFileHandle mdl;
    EftResFileHandle res;
    EftResFileHandle mdl2;
    EftResFileHandle res2;
    struct _g3d_work* work;

    if (w->cur_slot == -1)
        return;
    for (i = 0; i < 4; i++) {
        if (w->slots[w->cur_slot].res_file[i] != 0) {
            w->chars[i] = &w->slots[w->cur_slot].models[i];
            switch (i) {
            case 0:
                stage_area_kind_joints_hide(w, w->chars[i]);
                break;
            case 1:
                switch ((u8)stage_area_alt_model_get(w->mapno, w->areano)) {
                case 0:
                case 2:
                    w->flags |= 1;
                    w->chars[i]->chr.ready = 4;
                    break;
                case 1:
                    w->flags |= 2;
                    w->chars[i]->chr.ready = 5;
                    break;
                }
                break;
            case 2:
                w->flags |= 2;
                w->chars[i]->chr.ready = 5;
                break;
            }
            stage_area_joint_visibility_set(w, w->chars[i], i);
            for (frame = 0; frame < w->chars[i]->anim_count; frame++)
                w->chars[i]->chr.frame_init(frame, frame, 0.0f, 0, 1.0f);
            res.data_0x00 = w->chars[i]->chr.field_0x114;
            mdl.data_0x00 = w->chars[i]->chr.field_0x110;
            if (nw_res_anm_work_num(mdl, res, 1, &work) == 0) {
                w->anims[i] = 0;
            } else {
                res2.data_0x00 = w->chars[i]->chr.field_0x114;
                mdl2.data_0x00 = w->chars[i]->chr.field_0x110;
                w->anims[i] = (nw4r::g3d::AnmObj*)nw_res_anm_create(mdl2, res2,
                    (nw4r::g3d::ScnMdlSimple*)w->chars[i]->chr.field_0x118, 1, work, 0, 1.0f);
            }
        } else {
            w->chars[i] = 0;
            w->anims[i] = 0;
            if (i == 1) {
                if (w->mapno == 7 || w->mapno == 18) {
                    switch (w->areano) {
                    case 1:
                    case 2:
                    case 3:
                        w->flags = 1;
                        break;
                    }
                } else {
                    w->flags = 0;
                }
            }
            if (i == 3)
                w->alt_model = 0;
        }
    }
    w->field_0x2804 = 10.0f;
    setVector3(&w->field_0x2808, -1.0f, 0.0f, 0.0f);
    w->anim_rate = 1.0f;
    w->anim_rate_target = 1.0f;
    for (i = 0; i < 4; i++) {
        w->joint_state[0][i] = 0;
        w->joint_state[1][i] = 0;
    }
}

/* Enters area `areano` of map `mapno`: seats the slot, the models, effects, lights and the area's bodies. */
extern "C" void stage_area_enter(u8 mapno, u8 areano)
{
    StageWork* w = SW;
    u32 kind;

    w->cur_slot = stage_area_slot_get(mapno, areano);
    w->mapno = mapno;
    w->areano = areano;
    if (SW->init_count == 0) {
        stage_part_bodies_set_all(w);
        w->init_count++;
    }
    stage_area_models_seat();
    eft_scnbox_load((u8*)w->eft_seats, mapno, areano);
    eft_res_bind(0);
    stage_area_light_setup(w);
    stage_area_light_apply();
    light_area_setup();
    kind = stage_entry_eft_kind_get(mapno, areano);
    if ((u8)kind != 0xFF)
        eft004_area_set((s8)kind);
    stage_area_effects_start(mapno, areano);
    stage_parts_seat(w);
    eft_area_effects_set();
    lb_interior_fx_spawn(mapno, areano);
}

/* Starts the area effects and placed models of area `areano` of map `mapno`. */
extern "C" void stage_area_effects_start(u8 mapno, u8 areano)
{
    if (((mapno == 1) && (areano == 1)) || ((mapno == 0xC) && (areano == 1)) || ((mapno == 6) && (areano == 1)) || ((mapno == 0x11) && (areano == 1)) || ((mapno == 6) && (areano == 2)) || ((mapno == 0x11) && (areano == 2))) {
        eft023_area_fx_set();
    }
    if (((mapno == 1) && (areano == 2)) || ((mapno == 0xC) && (areano == 2))) {
        eft023_area_fx2_set();
    }
    if (((mapno == 1) && (areano == 2)) || ((mapno == 0xC) && (areano == 2)) || ((mapno == 1) && (areano == 9)) || ((mapno == 0xC) && (areano == 9)) || ((mapno == 1) && (areano == 0xA)) || ((mapno == 0xC) && (areano == 0xA)) || ((mapno == 1) && (areano == 0xB)) || ((mapno == 0xC) && (areano == 0xB)) || ((mapno == 2) && (areano == 9)) || ((mapno == 0xD) && (areano == 9)) || ((mapno == 2) && (areano == 0xB)) || ((mapno == 0xD) && (areano == 0xB)) || ((mapno == 5) && (areano == 2)) || ((mapno == 6) && (areano == 2)) || ((mapno == 0x11) && (areano == 2)) || ((mapno == 7) && (areano == 1)) || ((mapno == 0x12) && (areano == 1)) || ((mapno == 7) && (areano == 2)) || ((mapno == 0x12) && (areano == 2)) || ((mapno == 7) && (areano == 3)) || ((mapno == 0x12) && (areano == 3)) || ((mapno == 0xA) && (areano == 0)) || ((mapno == 0x15) && (areano == 5)) || ((mapno == 0x15) && (areano == 6)) || ((mapno == 0x15) && (areano == 7)) || ((mapno == 0x16) && (areano == 1)) || ((mapno == 0x16) && (areano == 2))) {
        em_sub_area_fx_set(0);
    }
    if (((mapno == 1) && (areano == 0)) || ((mapno == 0xC) && (areano == 0)) || ((mapno == 1) && (areano == 2)) || ((mapno == 0xC) && (areano == 2)) || ((mapno == 1) && (areano == 3)) || ((mapno == 0xC) && (areano == 3)) || ((mapno == 1) && (areano == 4)) || ((mapno == 0xC) && (areano == 4)) || ((mapno == 1) && (areano == 5)) || ((mapno == 0xC) && (areano == 5)) || ((mapno == 1) && (areano == 6)) || ((mapno == 0xC) && (areano == 6)) || ((mapno == 1) && (areano == 7)) || ((mapno == 0xC) && (areano == 7)) || ((mapno == 1) && (areano == 8)) || ((mapno == 0xC) && (areano == 8)) || ((mapno == 1) && (areano == 0xC)) || ((mapno == 0xC) && (areano == 0xC)) || ((mapno == 3) && (areano == 0)) || ((mapno == 0xE) && (areano == 0)) || ((mapno == 3) && (areano == 2)) || ((mapno == 0xE) && (areano == 2)) || ((mapno == 3) && (areano == 3)) || ((mapno == 0xE) && (areano == 3)) || ((mapno == 3) && (areano == 4)) || ((mapno == 0xE) && (areano == 4)) || ((mapno == 3) && (areano == 6)) || ((mapno == 0xE) && (areano == 6)) || ((mapno == 3) && (areano == 7)) || ((mapno == 0xE) && (areano == 7)) || ((mapno == 3) && (areano == 8)) || ((mapno == 0xE) && (areano == 8)) || ((mapno == 3) && (areano == 0xA)) || ((mapno == 5) && (areano == 6)) || ((mapno == 0x10) && (areano == 6))) {
        em_sub_area_fx_set(1);
    }
    if (((mapno == 7) && (areano == 3)) || ((mapno == 0x12) && (areano == 3))) {
        em_sub_area_fx_set(2);
        eft053_set();
    }
    if (((mapno == 1) && (areano == 0xC)) || ((mapno == 0xC) && (areano == 0xC)) || ((mapno == 3) && (areano == 2)) || ((mapno == 0xE) && (areano == 2)) || ((mapno == 3) && (areano == 3)) || ((mapno == 0xE) && (areano == 3)) || ((mapno == 3) && (areano == 4)) || ((mapno == 0xE) && (areano == 4)) || ((mapno == 3) && (areano == 6)) || ((mapno == 0xE) && (areano == 6)) || ((mapno == 3) && (areano == 8)) || ((mapno == 0xE) && (areano == 8))) {
        menu_area_fx_set();
    }
    if ((mapno == 3) && ((areano <= 5U) || ((u8) (areano + 0xF9) <= 3U))) {
        em_sub_area_fx2_set(0);
    }
    if ((mapno == 3) && ((u8) (areano + 0xFE) <= 1U)) {
        eft052_set();
    }
    if (mapno == 0x15) {
        switch ((u32)areano) {
        case 0:
        case 1:
        case 3:
            em_sub_area_fx2_set(1);
            break;
        case 5:
            place_model_create_pending();
            break;
        case 6:
            place_model_create_pending();
            break;
        case 7:
            place_model_create_pending();
            break;
        }
    }
    if (mapno == 0x16) {
        if (areano == 0) {
            em_sub_area_fx_set(3);
            lb_scene_eft_spawn();
        } else if (areano == 1) {
            place_model_create_pending();
        }
    }
    if (((mapno == 5) && (areano == 0)) || ((mapno == 5) && (areano == 1)) || ((mapno == 5) && (areano == 3)) || ((mapno == 5) && (areano == 6))) {
        eft030_set();
    }
    if (((mapno == 2) && (areano == 0xA)) || ((mapno == 0xD) && (areano == 0xA))) {
        lb_area_fx_set();
    }
    if ((mapno == 6) || (mapno == 0x11)) {
        if (areano == 1) {
            eft053_set();
        } else if (areano == 2) {
            eft053_set();
            em_sub_area_fx2_set(2);
        }
    }
    if (((mapno == 8) || (mapno == 0x13)) && (areano == 1)) {
        eft053_set();
    }
    if ((mapno == 9) && (areano <= 1U)) {
        eft053_set();
    }
    if (((mapno == 0xB) || (mapno == 0x14)) && (areano == 1)) {
        eft053_set();
    }
}

/* Calls stage_area_slot_reset for the two side indices. */
extern "C" void stage_area_slots_reset()
{
    u32 i;
    for (i = 0; i < 2; i++)
        stage_area_slot_reset((u8)i);
}

/* Frees slot `index`: marks it unused, clears its map/area and resets its four models. */
extern "C" void stage_area_slot_reset(u8 index)
{
    StageSlot* slot = &SW->slots[index];
    s32 i;

    slot->index = index;
    slot->state = 0;
    slot->field_0x598 = 0;
    slot->mapno = 0xFF;
    slot->areano = 0xFF;
    slot->model_count = 0;
    for (i = 0; i < 4; i++) {
        mhchar_reset(&slot->models[i].chr);
        slot->res_id[i] = -1;
        slot->res_file[i] = 0;
    }
}

/* Returns the free stage area slot (0 or 1), -1 when both are taken. */
extern "C" s8 stage_area_free_slot_get(void)
{
    StageWork* w = SW;
    s32 i;

    for (i = 0; i < 2; i++) {
        if (w->slots[i].state == 0)
            return i;
    }
    return -1;
}

/* Returns the slot holding area `areano` of map `mapno` (0 or 1), -1 when neither does. */
extern "C" s8 stage_area_slot_get(u8 mapno, u8 areano)
{
    StageWork* w = SW;
    s32 i;

    for (i = 0; i < 2; i++) {
        if (mapno == w->slots[i].mapno && areano == w->slots[i].areano)
            return i;
    }
    return -1;
}

/* Releases stage area slot `slot`: its models and their resources. */
extern "C" void stage_area_slot_release(s8 slot)
{
    s32 i;
    StageWork* w = SW;

    if (slot < 0)
        return;
    nw_res_stage_free();
    for (i = 0; i < 4; i++) {
        mhchar_model_release(&w->slots[slot].models[i].chr);
        if (w->slots[slot].res_file[i] != 0) {
            if (w->slots[slot].mem_index[i] >= 0)
                push_res_mem(w->slots[slot].mem_index[i]);
            nwDelResource(w->slots[slot].res_id[i]);
            w->slots[slot].res_id[i] = -1;
            w->slots[slot].res_file[i] = 0;
            w->slots[slot].mem_index[i] = -1;
        }
    }
    w->slots[slot].field_0x598 = 0;
    w->slots[slot].mapno = 0xFF;
    w->slots[slot].areano = 0xFF;
    w->slots[slot].state = 0;
    w->slots[slot].model_count = 0;
    eft_res_stage_release_all();
}

/* Returns the current map number. */
u8 get_now_mapno()
{
    return SW->mapno;
}

/* Returns the current area number. */
u8 get_now_areano()
{
    return SW->areano;
}

/* Returns the position of entry point `entry` of map `mapno` (a static vector). */
extern "C" VEC3* stage_entry_pos_get(u8 mapno, u8 entry)
{
    VEC3 pos;
    StageElem* e;

    if (mapno == 0xFF)
        return &stg_entry_pos_none;
    if ((move_work_state_ck() != 0 || quest_select_ready_ck() != 0) && entry == 1)
        return &stg_entry_pos_menu;
    e = ((StageGroup*)stage_map_group_get(mapno))->elems[entry];
    copyVec3(&stg_entry_pos_work, setVec3(&pos, e->entry_pos.x, e->entry_pos.y, e->entry_pos.z));
    return &stg_entry_pos_work;
}

/* Writes the start position and facing of the stage quest phase `phase` names. */
extern "C" void stage_start_get(u8 phase, VEC3* pos, s32* angle)
{
    VEC3 tmp;
    StageStart* start;

    if (phase == 0xFF) {
        copyVec3(pos, &stg_entry_pos_none);
        *angle = 0;
        return;
    }
    start = ((StageGroup*)stage_map_group_get(phase))->start;
    if (start == 0) {
        copyVec3(pos, &stg_entry_pos_none);
        *angle = 0;
        return;
    }
    copyVec3(pos, setVec3(&tmp, start->pos.x, start->pos.y, start->pos.z));
    *angle = start->angle;
}

/* Returns the record's +0x24 word, or 0 for the unmapped id. */
extern "C" u32 stage_entry_angle_get(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)stage_map_group_get((u8)id);
    return g->elems[(u8)j]->field_0x24;
}

/* Returns the j-th word of the group's inline table, or NULL for the unmapped id. */
extern "C" struct StageZone* stage_group_word_get(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)stage_map_group_get((u8)id);
    return g->zones[(u8)j];
}

/* Returns the record's +0x28 byte, or 0xFF for the unmapped id. */
extern "C" u32 stage_entry_eft_kind_get(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0xFF;
    g = (StageGroup*)stage_map_group_get((u8)id);
    return g->elems[(u8)j]->field_0x28;
}

/* Returns the selected group's head byte, or 0 for the unmapped id. */
extern "C" u32 stage_map_area_count_get(u8 id)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)stage_map_group_get((u8)id);
    return g->kind;
}

/* Tests bit 0 of the stage flag byte. */
extern "C" bool stage_water_enabled_ck()
{
    return SW->flags & 1;
}

/* Tests bit 1 of the stage flag byte. */
extern "C" bool stage_water_b_enabled_ck()
{
    return SW->flags & 2;
}

/* Returns which alternative model area `areano` of map `mapno` uses: 2 on the map-1 water area, 1 on the
 * map-3/4 cave areas, 0 otherwise. */
extern "C" u32 stage_area_alt_model_get(u8 mapno, u8 areano)
{
    switch (mapno) {
    case 1:
    case 12:
        if (areano == 6)
            return 2;
        break;
    case 3:
    case 14:
        if (areano == 5)
            return 1;
        break;
    case 4:
    case 15:
        if (areano == 5)
            return 1;
        break;
    }
    return 0;
}

/* Whether the current area is under water (the stage's water bit, minus each map's dry areas). */
extern "C" u32 stage_water_area_ck(void)
{
    if (SW->flags & 1) {
        switch (SW->mapno) {
        case 1:
        case 12:
            if ((u32)(SW->areano - 2) <= 4U || (s32)SW->areano == 9)
                return 0;
            break;
        case 2:
        case 13:
            switch (SW->areano) {
            case 3:
            case 9:
                return 0;
            }
            break;
        case 3:
        case 14:
            switch (SW->areano) {
            case 5:
            case 7:
                return 0;
            }
            break;
        case 5:
        case 16:
            switch (SW->areano) {
            case 6:
                return 0;
            }
            break;
        }
        return 1;
    }
    return 0;
}

/* Writes the current area's position (the area light's position) into `out`. */
extern "C" void get_now_area_pos(VEC3* out, u8 area)
{
    StageArea* a;

    VEC3_ctor(out);
    a = &SW->areas[SW->areano];
    setVector3(out, a->light.dir.x, a->light.dir.y, a->light.dir.z);
}

/* Returns the world position of the area-local point `pos` in area `area`. */
StageRetVec3 get_worldworld_pos(VEC3* pos, u8 area)
{
    StageRetVec3 out;
    VEC3 origin;
    StageAreaOrigin* o;

    if (get_now_mapno() == 0xFF) {
        setVec3(&out, 0.0f, 0.0f, 0.0f);
    } else {
        o = &((StageGroup*)stage_map_group_get(get_now_mapno()))->map->origins[area];
        setVec3(&origin, o->x, 0.0f, o->z);
        addVec3(&out, pos, &origin);
    }
    return out;
}

/* Writes the area-local position of the world point `pos` in area `area` into `out`. */
extern "C" void get_worldlocal_pos(VEC3* out, VEC3* pos, u8 area)
{
    VEC3 origin;
    StageAreaOrigin* o;

    o = &((StageGroup*)stage_map_group_get(get_now_mapno()))->map->origins[area];
    setVec3(&origin, o->x, 0.0f, o->z);
    subVec3(out, pos, &origin);
}

/* Returns the word at +8 of the current map/area group. */
extern "C" u32 screen_projection_get()
{
    return (u32)((StageGroup*)stage_map_group_get((u8)get_now_mapno()))->map;
}

/* Returns the current area's word [i][j] lookup. */
extern "C" u32 stage_cell_get(u8 i, u8 j)
{
    StageWork* w = SW;
    return w->areas[w->areano].rows[(u8)i].words[(u8)j];
}

/* Same lookup, but the area is an argument. */
extern "C" u32 stage_area_cell_get(u8 i, u8 j, u8 area)
{
    StageArea* a = &SW->areas[(u8)area];
    StageAreaRow* r = &a->rows[(u8)i];
    return r->words[(u8)j];
}

/* Copies the current area's row [i] 12-byte element [k] into `out`. */
extern "C" void stage_cell_vec_get(void* out, u8 i, u8 k)
{
    assignVec3((Vec*)out, (Vec*)&SW->areas[SW->areano].rows[(u8)i].vecs[(u8)k]);
}

/* Returns effect colour `which` (0 or 1) of the current map's record `index`, -1 when there is none. */
u32 get_stg_eft_col(u8 index, u8 which)
{
    StageWork* w = SW;
    StageElem* e;

    if (index == 0xFF)
        return -1;
    e = ((StageGroup*)stage_map_group_get(w->mapno))->elems[index];
    if (e == 0)
        return -1;
    switch (which) {
    case 0:
        return e->eft_col[0];
    case 1:
        return e->eft_col[1];
    default:
        return -1;
    }
}

/* Returns the pit map's effect colour of record `index` for `pos`: the deep colour below the floor line. */
extern "C" u32 get_stg_eft_col_pos(u8 index, VEC3* pos)
{
    StageElem* e;

    if (index == 0xFF)
        return -1;
    if (stage_pos_z_low_ck(pos) != 0)
        return 0x544E4096;
    e = ((StageGroup*)stage_map_group_get(9))->elems[index];
    if (e == 0)
        return -1;
    return e->eft_col[0];
}

/* Returns the four area effect emitters. */
extern "C" void* stage_effect_block_get()
{
    return stage_w.eft_seats;
}

/* Returns the current map's descriptor field +0x34 for `id`, or a default float. */
extern "C" f32 stg_area_ceiling_get(u8 id)
{
    StageWork* w = SW;
    StageElem* e;
    if ((u8)id == 0xFF)
        return 10000.0f;
    e = ((StageGroup*)stage_map_group_get(w->mapno))->elems[(u8)id];
    if (e == 0)
        return 10000.0f;
    return e->field_0x34;
}

/* Returns the current map's +0x18 word array entry for `id`, or 0. */
extern "C" u32 stage_area_word_get(u8 id)
{
    StageWork* w = SW;
    u32* arr;
    if ((u8)id == 0xFF)
        return 0;
    arr = ((StageGroup*)stage_map_group_get(w->mapno))->field_0x18;
    if (arr == 0)
        return 0;
    return arr[(u8)id];
}

/* Returns the entry `index` points at, biased by its 0x114-byte header, or NULL. */
extern "C" void* stage_char_res_body_get(u8 index)
{
    StageChar** chars = &SW->chars[0];
    StageObject* p = (StageObject*)chars[(u8)index];
    if (p == 0)
        return 0;
    return stage_res_body_get(p->body);
}

/* Returns the record at `self` plus the resource size from fn_800700C0(). */
extern "C" void* stage_res_body_get(void* self)
{
    return stage_res_offset_get(self, ((const ResolverView*)&reinterpret_cast<const nw4r::g3d::ResMdl*>(self)->ref())->size);
}

/* Returns `base`'s word pointer plus `off`; a zero offset yields NULL. */
extern "C" void* stage_res_offset_get(void* base, s32 off)
{
    u8* p = *(u8**)base;
    if (off != 0)
        return p + off;
    return 0;
}

/* Returns the current map's descriptor field +0x38 for `id`, or 0. */
extern "C" u32 pl_act_kind_get(u8 id)
{
    StageWork* w = SW;
    StageElem* e;
    if ((u8)id == 0xFF)
        return 0;
    e = ((StageGroup*)stage_map_group_get(w->mapno))->elems[(u8)id];
    if (e == 0)
        return 0;
    return e->field_0x38;
}

/* Classifies the current map number into a load category. */
extern "C" u32 stage_climate_kind_get()
{
    s32 m = SW->mapno;
    switch (m) {
    case 2:
    case 5:
    case 16:
    case 10:
        return 1;
    case 13:
    case 4:
    case 15:
        return 2;
    }
    return 0;
}

/* Remaps an id through stg_map_kind_table; an unmapped id (0xFF) is returned unchanged.  The parameter is
 * wider than a byte: retail masks it (`clrlwi`) before the table index, so the original signature
 * was not the byte one `unsplit/unknown.h` guessed. */
extern "C" u32 stage_map_kind_get(u32 kind)
{
    u8 mapped = stg_map_kind_table[(u8)kind];
    if (mapped != 0xFF)
        return mapped;
    return kind;
}

/* Whether the float at `self`+8 is below the pooled constant. */
extern "C" u32 stage_pos_z_low_ck(void* self)
{
    return ((f32*)self)[2] < (-10000.0f);
}

/* The pit map's two entry points and the area start points. */
extern "C" {

StagePoint stg_pit_point_a = {{0.0f, 0.0f, 3300.0f}, 180.0f};

StagePoint stg_pit_point_b = {{1670.0f, -1500.0f, 1110.0f}, 230.0f};

StagePoint stg_area_start_a = {{10900.0f, 0.0f, -13743.0f}, 210.0f};

StagePoint stg_area_start_b = {{5180.0f, 0.0f, -13385.0f}, 210.0f};

}

/* Writes entry point `idx` of area `area`: the pit map's entry for `idx` 0, a fixed point otherwise. */
extern "C" void stage_area_point_get(u8 area, u8 idx, VEC3* pos, u32* angle)
{
    StagePoint* p = 0;

    if (idx == 0) {
        copyVec3(pos, stage_entry_pos_get(9, area));
        *angle = stage_entry_angle_get(9, area);
        return;
    }
    switch (area) {
    case 0:
        p = &stg_pit_point_a;
        break;
    case 1:
        p = &stg_pit_point_b;
        break;
    }
    if (p != 0) {
        pos->x = p->pos.x;
        pos->y = p->pos.y;
        pos->z = p->pos.z;
        *angle = (u16)(0.5f + (65536.0f * (f32)(u32)p->degrees) / 360.0f);
    }
}

/* Writes the start position and facing of area `area` (two fixed points). */
extern "C" void stage_area_start_get(u8 area, VEC3* pos, u32* angle)
{
    StagePoint* p;

    if (area == 0)
        p = &stg_area_start_a;
    else
        p = &stg_area_start_b;
    pos->x = p->pos.x;
    pos->y = p->pos.y;
    pos->z = p->pos.z;
    *angle = (u16)(0.5f + (65536.0f * (f32)(u32)p->degrees) / 360.0f);
}

/* Returns the stage's own bonus pick for the map kind of `name`. */
extern "C" u8 stage_bonus_pick(u8 name)
{
    switch ((u8)stage_map_kind_get(name)) {
    case 1:
        return 4;
    case 2:
        return 11;
    case 3:
        return 8;
    case 4:
        return 1;
    case 5:
        return 10;
    default:
        return 0;
    }
}

/* The per-frame blend of the area animation rate towards its target. */
extern "C" {

f32 stg_anim_rate_blend = 0.1f;

}

/* Stores `f` into the current area's record when the map/area pair matches. */
extern "C" void stage_anim_rate_target_set(f32 f)
{
    StageWork* w = SW;
    if ((w->mapno == 6 || w->mapno == 17) && w->areano == 1)
        w->anim_rate_target = f;
}

/* Eases the area model's animation rate toward its target on the map-6 area 1 and applies it. */
extern "C" void stage_anim_rate_update(StageWork* w)
{
    if ((w->mapno == 6 || w->mapno == 17) && w->areano == 1) {
        w->anim_rate = w->anim_rate + stg_anim_rate_blend * (w->anim_rate_target - w->anim_rate);
        mhchar_motion_speed_set(&w->chars[0]->chr, w->anim_rate);
        w->anims[0]->SetUpdateRate(2.0f * w->anim_rate);
    }
}

/* The (index, bit) area event pairs `stage_area_event_pair_raise` raises. */
extern "C" {

u8 stg_area_event_pairs[12][2] = {
    {12, 0}, {12, 1}, {4, 1}, {4, 2}, {6, 3}, {1, 0},
    {1, 0}, {2, 1}, {2, 2}, {0, 0}, {0, 1}, {6, 2},
};

}

/* Tail-calls stg_area_flag_ck with the byte pair stg_area_event_pairs[2*index], [2*index+1]. */
extern "C" u32 stg_area_flag_index_ck(u8 index)
{
    const u8* p = &stg_area_event_pairs[0][(u8)index * 2];
    return stg_area_flag_ck(p[0], p[1]);
}

/* Sets bit (4 + j) of bitmask[id] when the current map's j-th table word is set. */
extern "C" void stage_area_flag_set(u8 id, u8 j)
{
    StageWork* w = SW;
    StagePartDef** p = stage_part_defs_get(w, id);
    if (p[(u8)j] != 0)
        w->bitmask[(u8)id] |= (u8)(0x10 << (u8)j);
}

/* Raises area event `index`/`bit`: marks it pending, sets its area flag and sends it to the partner. */
extern "C" void stage_area_event_raise(u8 index, u8 bit)
{
    SW->event_pending = 1;
    stage_area_flag_set(index, bit);
    lb_sub11_send(index, bit);
}

/* Raises the area event of pair `idx` and stores `value` as the pending byte. */
extern "C" void stage_area_event_pair_raise(u8 idx, u8 value)
{
    const u8* pair = stg_area_event_pairs[idx];
    stage_area_event_raise(pair[0], pair[1]);
    SW->event_pending = value;
}

/* Returns the id-remapped byte at stage_w.bitmask[index]. */
extern "C" u8 stage_area_bit_get(u8 index)
{
    const u8* p = &SW->bitmask[0];
    return p[(u8)index];
}

/* Tests bit (4 + bit) of stage_w.bitmask[index]. */
extern "C" bool stg_area_flag_ck(u8 index, u8 bit)
{
    StageWork* w = SW;
    return w->bitmask[(u8)index] & (0x10 << (u8)bit);
}

/* Whether `self`'s map id remaps to a "loadable" category. */
extern "C" u32 stage_map_kind_loadable_ck(void* self)
{
    s32 mapped = (u8)stage_map_kind_get(((StageWork*)self)->mapno);
    if (mapped == 4 || mapped == 10)
        return 1;
    return 0;
}

/* Remaps the stage's current map id and returns the j-th element of the group's 16-byte-stride
 * table. */
extern "C" StagePartDef** stage_part_defs_get(StageWork* work, u8 j)
{
    StageGroup* g = (StageGroup*)stage_map_group_get(work->mapno);
    return &g->part_defs[(u8)j * 4];
}

/* Whether the j-th element of the group's 16-byte-stride table is non-zero. */
extern "C" bool stage_part_def_ck(u8 id, u8 j)
{
    StageGroup* g = (StageGroup*)stage_map_group_get((u8)id);
    return g->part_defs[(u8)j * 4];
}

/* Seats the current area's breakable parts on the area model: resets each part's joints from the model's
 * nodes, shows or hides them by the area flags and queues the hidden ones. */
extern "C" void stage_parts_seat(StageWork* w)
{
    nw4r::g3d::ResNode node(0);
    VEC3 rot;
    MHchar* chr;
    MHchar* water;
    MHchar* alt;
    StagePartDef** defs;
    u8 hidden;
    u8 j;
    u8 k;

    VEC3_ctor(&rot);
    chr = &w->chars[0]->chr;
    water = &w->chars[1]->chr;
    alt = 0;
    defs = stage_part_defs_get(w, w->areano);
    hidden = 0;
    for (k = 0; k < 8; k++)
        w->vis_ids[k] = 0xFF;
    if (w->alt_model == 1) {
        s32 count;
        alt = &w->chars[3]->chr;
        if (alt == 0)
            return;
        count = mhchar_node_count(alt);
        for (k = 0; k < count; k++)
            alt->setVisibility(k, 0);
    }
    for (j = 0; j < 4; j++) {
        StagePart* part;
        if (defs[j] == 0)
            continue;
        if (w->alt_model == 1) {
            for (k = 0; k < 3; k++) {
                if (defs[j]->joints[k] == 0xFF)
                    break;
                chr->setVisibility(defs[j]->joints[k], 0);
            }
            for (k = 0; k < 2; k++) {
                if (defs[j]->vis_ids[k] == 0xFF)
                    break;
                chr->setVisibility(defs[j]->vis_ids[k], 0);
            }
        }
        part = &w->parts[j];
        part->state = 0;
        part->field_0x9A = 0;
        if (stg_area_flag_ck(w->areano, j) != 0)
            part->visible = 0;
        else
            part->visible = 1;
        if (w->alt_model == 1)
            chr = alt;
        for (k = 0; k < 3; k++) {
            setVector3(&part->joint_pos[k], 0.0f, 0.0f, 0.0f);
            part->joint_rot[k][0] = 0;
            part->joint_rot[k][1] = 0;
            part->joint_rot[k][2] = 0;
            part->joint_done[k] = 0;
            part->joint_time[k] = 0;
            if (defs[j]->joints[k] == 0xFF)
                break;
            mhchar_res_node_get(chr, &node, defs[j]->joints[k]);
            copyVec3(&part->joint_base[k], (VEC3*)em_res_node_trans_get(&node));
            copyVec3(&rot, (VEC3*)em_res_node_rot_get(&node));
            part->joint_angle[k][0] = (u16)(0.5f + (65536.0f * rot.x) / 360.0f);
            part->joint_angle[k][1] = (u16)(0.5f + (65536.0f * rot.y) / 360.0f);
            part->joint_angle[k][2] = (u16)(0.5f + (65536.0f * rot.z) / 360.0f);
            chr->setVisibility(defs[j]->joints[k], part->visible);
        }
        for (k = 0; k < 2; k++) {
            if (defs[j]->vis_ids[k] == 0xFF)
                break;
            if ((w->mapno == 3 || w->mapno == 14) && w->areano == 5) {
                chr = &w->chars[0]->chr;
                chr->setVisibility(defs[j]->vis_ids[k], part->visible);
            } else if (stage_water_enabled_ck() == 1) {
                water->setVisibility(defs[j]->vis_ids[k], part->visible);
            }
            if (part->visible == 0) {
                w->vis_ids[hidden] = defs[j]->vis_ids[k];
                hidden++;
            }
        }
    }
}

/* Arms the four breakable-part bodies of area `areano` and raises the events their maps start with. */
extern "C" void stage_part_bodies_set(StageWork* w, u8 areano)
{
    StagePartDef** defs = stage_part_defs_get(w, areano);
    s32 kind = (stage_map_kind_loadable_ck(w) == 1) + 4;
    u8 j;

    for (j = 0; j < 4; j++) {
        StagePartDef* def = defs[j];
        if (def != 0) {
            u8 body = def->body;
            body_set((_BODY_W*)&w->bodies[body], def->body_data, kind, (u32)w, 0);
            w->bodies[body].area = areano;
            switch (w->mapno) {
            case 1:
            case 12:
                if (areano == 1 && quest_id_tail_ck() == 0)
                    stage_area_event_raise(areano, j);
                break;
            case 8:
            case 19:
                if (areano == 0 && j == 0)
                    stage_area_event_raise(areano, j);
                break;
            case 11:
            case 20:
                if (areano == 0 && j == 1)
                    stage_area_event_raise(areano, j);
                break;
            }
        }
    }
}

/* Calls stage_part_bodies_set for each index below the current map's count. */
extern "C" void stage_part_bodies_set_all(StageWork* self)
{
    u32 n = stage_map_area_count_get(self->mapno);
    u8 i;
    for (i = 0; i < (u8)n; i++)
        stage_part_bodies_set(self, i);
}

/* Checks the four breakable parts of area `areano` for a break (damage past the part's limit, or the area flag
 * already set) and drives each part's state. */
extern "C" void stage_parts_check(StageWork* w, u8 areano)
{
    StagePartDef** defs = stage_part_defs_get(w, areano);
    u8 j;

    for (j = 0; j < 4; j++) {
        if (defs[j] != 0) {
            u8 b = defs[j]->body;
            u32 forced;

            if (stg_area_flag_ck(areano, j) == 1 && w->bodies[b].active == 1)
                forced = 1;
            else
                forced = 0;
            if (w->areano == w->bodies[b].area) {
                if ((w->bodies[b].hit != 0 && w->bodies[b].active == 1) || forced == 1) {
                    s16 prev = w->parts[j].damage;
                    if (w->bodies[b].damage >= defs[j]->hp || forced == 1) {
                        w->bodies[b].active = 0;
                        if (forced == 0)
                            stage_area_event_raise(areano, j);
                        stage_part_break(w, j);
                    } else {
                        w->bodies[b].hit = 0;
                        if ((s16)(w->bodies[b].damage - prev) > 0)
                            camera_quake_req(0x42);
                    }
                }
                stage_part_update(w, &w->parts[j], defs[j]->joints, defs[j]->vis_ids, defs[j]->kind);
            } else if (w->bodies[b].hit != 0 || forced == 1) {
                if (w->bodies[b].damage >= defs[j]->hp || forced == 1) {
                    if (w->bodies[b].active == 1) {
                        w->bodies[b].active = 0;
                        stage_area_event_raise(areano, j);
                    } else if ((system_w.field_0x0c & 0x7FF) == 0) {
                        stage_area_event_raise(areano, j);
                    }
                } else {
                    w->bodies[b].hit = 0;
                }
            }
            if (w->bodies[b].data != 0)
                hit_body_list_push((_HIT_W*)&w->bodies[b]);
        }
    }
}

/* Calls stage_parts_check for each index below the current map's count. */
extern "C" void stage_parts_check_all(StageWork* self)
{
    u32 n = stage_map_area_count_get(self->mapno);
    u8 i;
    for (i = 0; i < (u8)n; i++)
        stage_parts_check(self, i);
}

/* Drives part `part` of the current model: arms the break (joint scatter and hidden joints) and steps it. */
extern "C" void stage_part_update(StageWork* w, StagePart* part, u8* joints, u8* vis_ids, u8 kind)
{
    u32 i;
    MHchar* chr;

    if (w->alt_model == 1)
        chr = &w->chars[3]->chr;
    else
        chr = &w->chars[0]->chr;
    if (chr == 0)
        return;
    switch (part->state) {
    case 1:
        part->state++;
        stage_part_scatter_start(w, part, chr, joints, kind);
        if ((w->mapno == 3 || w->mapno == 14) && w->areano == 5) {
            chr = &w->chars[0]->chr;
            stage_part_joints_hide(w, chr, vis_ids);
        } else if (stage_water_enabled_ck() == 1) {
            stage_part_joints_hide(w, &w->chars[1]->chr, vis_ids);
        }
    case 2:
        stage_part_scatter_step(w, part, chr, joints, kind);
        break;
    case 3:
        for (i = 0; i < 3; i++, joints++) {
            if (*joints == 0xFF)
                break;
            chr->setVisibility(*joints, 0);
        }
        part->state++;
        break;
    }
}

/* The two break points `stage_part_break` seats the koware effect on. */
extern "C" {

StagePoint stg_koware_points[2] = {
    {{16400.0f, -22900.0f, 4400.0f}, 180.0f},
    {{15000.0f, -23400.0f, 4800.0f}, 380.0f},
};

}

/* Breaks part `index` of the current area: starts its fall, shakes the camera, spawns its break effect and
 * plays its sound. */
extern "C" void stage_part_break(StageWork* w, u8 index)
{
    VEC3 kpos;
    VEC3 ppos;
    VEC3 epos;
    VEC3 tmp0;
    VEC3 tmp1;
    VEC3 tmp2;
    StagePartDef** defs;
    s32 quake = 0;

    VEC3_ctor(&kpos);
    VEC3_ctor(&ppos);
    VEC3_ctor(&epos);
    defs = stage_part_defs_get(w, w->areano);
    if (defs[index] == 0)
        return;
    if (w->parts[index].state != 0)
        return;
    if (defs[index]->kind == 7)
        w->parts[index].state = 4;
    else
        w->parts[index].state = 1;
    switch (defs[index]->kind) {
    case 0:
        quake = 4;
        break;
    case 2:
    case 3:
    case 6:
        quake = 2;
        break;
    case 4:
        quake = 5;
        break;
    case 5:
        quake = 6;
        break;
    case 1:
        quake = 6;
        break;
    case 7:
        quake = 0;
        break;
    }
    if (quake != 0 && w->event_pending == 1)
        camera_quake_req(quake | 0x40);
    switch (defs[index]->eft_kind) {
    case 0:
        copyVec3(&epos, setVec3(&tmp0, defs[index]->eft_pos.x, defs[index]->eft_pos.y, defs[index]->eft_pos.z));
        eft_area_part_break_set(0, &epos, defs[index]->eft_scale, w->areano, defs[index]->eft_param);
        break;
    case 1:
        copyVec3(&epos, setVec3(&tmp1, defs[index]->eft_pos.x, defs[index]->eft_pos.y, defs[index]->eft_pos.z));
        eft028_set_koware(1, &epos, w->areano, defs[index]->eft_param);
        break;
    case 2:
        copyVec3(&epos, setVec3(&tmp2, defs[index]->eft_pos.x, defs[index]->eft_pos.y, defs[index]->eft_pos.z));
        eft_area_part_break_set(6, &epos, defs[index]->eft_scale, w->areano, defs[index]->eft_param);
        break;
    }
    if ((w->mapno == 7 || w->mapno == 18) && w->areano == 2 && index == 0) {
        StagePoint* p = &stg_koware_points[index];
        kpos.x = p[0].pos.x;
        kpos.y = p[0].pos.y;
        kpos.z = p[0].pos.z;
        eft028_set_koware(1, &kpos, w->areano, p[0].degrees);
        kpos.x = p[1].pos.x;
        kpos.y = p[1].pos.y;
        kpos.z = p[1].pos.z;
        eft028_set_koware(1, &kpos, w->areano, p[1].degrees);
    }
    copyVec3(&ppos, &w->parts[index].joint_base[0]);
    se_area_part_play(index, &ppos);
}

/* The joint scatter of the seven breakable-part kinds: start velocities, gravity, damping, spin and
 * spin damping, three per joint, and the floor and timing of each kind. */
extern "C" {

u8 stg_scatter_jitter_xz = 4;

u8 stg_scatter_jitter_y = 2;

f32 stg_scatter_pos_0[9] = {
    3.0f, 0.0f, -3.0f,
    -3.0f, 0.0f, 3.0f,
    1.0f, 0.0f, -1.0f,
};

f32 stg_scatter_gravity_0[9] = {
    0.0f, -4.0f, 0.0f,
    0.0f, -4.0f, 0.0f,
    0.0f, -4.0f, 0.0f,
};

f32 stg_scatter_damp_0[9] = {
    0.98f, 0.5f, 1.0f,
    0.98f, 0.4f, 1.0f,
    0.98f, 0.2f, 1.0f,
};

u16 stg_scatter_rot_0[9] = {
    0xFFD0, 0x0, 0x0,
    0x10, 0x0, 0x0,
    0x0, 0x0, 0x0,
};

f32 stg_scatter_rot_damp_0[9] = {
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

StageScatterDef stg_scatter_def_0 = {stg_scatter_pos_0, stg_scatter_gravity_0, stg_scatter_damp_0, stg_scatter_rot_0, stg_scatter_rot_damp_0, -5000.0f, 0, 4};

f32 stg_scatter_gravity_1[9] = {
    0.0f, -12.0f, 0.0f,
    0.0f, -12.0f, 0.0f,
    0.0f, -12.0f, 0.0f,
};

StageScatterDef stg_scatter_def_1 = {stg_scatter_pos_0, stg_scatter_gravity_1, stg_scatter_damp_0, stg_scatter_rot_0, stg_scatter_rot_damp_0, -40000.0f, 0, 4};

f32 stg_scatter_pos_2[9] = {
    -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 10.0f,
    0.0f, 0.0f, 0.0f,
};

f32 stg_scatter_gravity_2[9] = {
    0.0f, -0.4f, 0.0f,
    0.0f, -0.8f, 0.0f,
    0.0f, -1.0f, 0.0f,
};

f32 stg_scatter_damp_2[9] = {
    1.05f, 1.12f, 0.95f,
    0.95f, 1.0f, 0.92f,
    1.0f, 1.0f, 1.0f,
};

u16 stg_scatter_rot_2[9] = {
    0x20, 0x0, 0x15,
    0x200, 0x0, 0x50,
    0x0, 0x0, 0x0,
};

f32 stg_scatter_rot_damp_2[9] = {
    1.1f, 1.0f, 1.18f,
    1.0f, 1.0f, 1.1f,
    1.0f, 1.0f, 1.0f,
};

StageScatterDef stg_scatter_def_2 = {stg_scatter_pos_2, stg_scatter_gravity_2, stg_scatter_damp_2, stg_scatter_rot_2, stg_scatter_rot_damp_2, -1000.0f, 1, 4};

f32 stg_scatter_pos_3[9] = {
    -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 10.0f,
    0.0f, 0.0f, 0.0f,
};

f32 stg_scatter_gravity_3[9] = {
    0.0f, -0.4f, 0.0f,
    0.0f, -0.8f, 0.0f,
    0.0f, -1.0f, 0.0f,
};

f32 stg_scatter_damp_3[9] = {
    1.05f, 1.12f, 0.95f,
    0.95f, 1.0f, 0.92f,
    1.0f, 1.0f, 1.0f,
};

u16 stg_scatter_rot_3[9] = {
    0x20, 0x0, 0x15,
    0x200, 0x0, 0x50,
    0x0, 0x0, 0x0,
};

f32 stg_scatter_rot_damp_3[9] = {
    1.1f, 1.0f, 1.18f,
    1.0f, 1.0f, 1.1f,
    1.0f, 1.0f, 1.0f,
};

StageScatterDef stg_scatter_def_3 = {stg_scatter_pos_3, stg_scatter_gravity_3, stg_scatter_damp_3, stg_scatter_rot_3, stg_scatter_rot_damp_3, -1000.0f, 0, 4};

f32 stg_scatter_pos_4[9] = {
    -80.0f, 40.0f, 20.0f,
    -40.0f, 60.0f, -30.0f,
    0.0f, -200.0f, 0.0f,
};

f32 stg_scatter_gravity_4[9] = {
    -3.0f, -5.0f, 0.0f,
    0.0f, -5.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
};

f32 stg_scatter_damp_4[9] = {
    0.98f, 1.0f, 1.0f,
    0.98f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

u16 stg_scatter_rot_4[9] = {
    0x0, 0x0, 0xFA00,
    0xFA00, 0x100, 0x50,
    0x80, 0x0, 0x0,
};

f32 stg_scatter_rot_damp_4[9] = {
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

StageScatterDef stg_scatter_def_4 = {stg_scatter_pos_4, stg_scatter_gravity_4, stg_scatter_damp_4, stg_scatter_rot_4, stg_scatter_rot_damp_4, -1000.0f, 0, 0};

f32 stg_scatter_pos_5[9] = {
    -1.0f, 0.0f, 2.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, -1.0f,
};

f32 stg_scatter_gravity_5[9] = {
    0.0f, -0.2f, 0.0f,
    0.0f, -0.3f, 0.0f,
    0.0f, -0.3f, 0.0f,
};

f32 stg_scatter_damp_5[9] = {
    0.98f, 1.08f, 1.0f,
    1.0f, 1.05f, 1.0f,
    0.98f, 1.1f, 1.0f,
};

u16 stg_scatter_rot_5[9] = {
    0x20, 0x0, 0x60,
    0x10, 0x0, 0xFF00,
    0x80, 0x0, 0xFF80,
};

f32 stg_scatter_rot_damp_5[9] = {
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

StageScatterDef stg_scatter_def_5 = {stg_scatter_pos_5, stg_scatter_gravity_5, stg_scatter_damp_5, stg_scatter_rot_5, stg_scatter_rot_damp_5, -5000.0f, 1, 4};

f32 stg_scatter_pos_6[9] = {
    -1.0f, 0.0f, 2.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, -1.0f,
};

f32 stg_scatter_gravity_6[9] = {
    0.0f, -0.3f, 0.0f,
    0.0f, -0.4f, 0.0f,
    0.0f, -0.4f, 0.0f,
};

f32 stg_scatter_damp_6[9] = {
    0.98f, 1.08f, 1.0f,
    1.0f, 1.05f, 1.0f,
    0.98f, 1.1f, 1.0f,
};

u16 stg_scatter_rot_6[9] = {
    0x20, 0x0, 0x60,
    0x10, 0x0, 0xFF00,
    0x80, 0x0, 0xFF80,
};

f32 stg_scatter_rot_damp_6[9] = {
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

StageScatterDef stg_scatter_def_6 = {stg_scatter_pos_6, stg_scatter_gravity_6, stg_scatter_damp_6, stg_scatter_rot_6, stg_scatter_rot_damp_6, -2000.0f, 1, 4};

StageScatterDef* stg_scatter_defs[7] = {
    &stg_scatter_def_0,
    &stg_scatter_def_1,
    &stg_scatter_def_2,
    &stg_scatter_def_3,
    &stg_scatter_def_4,
    &stg_scatter_def_5,
    &stg_scatter_def_6,
};

}

/* Steps part `part`'s joint scatter: jitters the joints while they shake, then flies and spins them, and
 * writes each joint's position and rotation into the model's nodes. */
extern "C" void stage_part_scatter_step(StageWork* w, StagePart* part, MHchar* chr, u8* joints, u8 kind)
{
    StageScatterDef* def = stg_scatter_defs[kind];
    nw4r::g3d::ResNode node(0);
    u8 i;

    if (part->joint_base[0].y < def->floor_y)
        part->state++;
    for (i = 0; i < 3; i++, joints++) {
        f32 d;
        switch (part->joint_done[i]) {
        case 0:
            if (*joints == 0xFF)
                break;
            d = (s32)(ran_suu(0) % stg_scatter_jitter_xz);
            if (ran_suu(0) & 1)
                d *= -1.0f;
            part->joint_base[i].x += d;
            d = (s32)(ran_suu(0) % stg_scatter_jitter_xz);
            if (ran_suu(0) & 1)
                d *= -1.0f;
            part->joint_base[i].z += d;
            d = (s32)(ran_suu(0) % stg_scatter_jitter_y);
            if (ran_suu(0) & 1)
                d *= -1.0f;
            part->joint_base[i].y += d;
            if (--part->joint_time[i] < 0)
                part->joint_done[i]++;
            mhchar_res_node_get(chr, &node, *joints);
            stage_res_node_trans_set(&node, &part->joint_base[i]);
            res_node_rot_set(&node, (360.0f * part->joint_angle[i][0]) / 65536.0f,
                             (360.0f * part->joint_angle[i][1]) / 65536.0f,
                             (360.0f * part->joint_angle[i][2]) / 65536.0f);
            break;
        case 1:
            if (*joints == 0xFF)
                break;
            part->joint_pos[i].x *= def->damp[i * 3];
            part->joint_pos[i].y *= def->damp[i * 3 + 1];
            part->joint_pos[i].z *= def->damp[i * 3 + 2];
            part->joint_pos[i].x += def->gravity[i * 3];
            part->joint_pos[i].y += def->gravity[i * 3 + 1];
            part->joint_pos[i].z += def->gravity[i * 3 + 2];
            addVec3To(&part->joint_base[i], &part->joint_pos[i]);
            part->joint_rot[i][0] = (u16)(s32)(part->joint_rot[i][0] * def->rot_damp[i * 3]);
            part->joint_rot[i][1] = (u16)(s32)(part->joint_rot[i][1] * def->rot_damp[i * 3 + 1]);
            part->joint_rot[i][2] = (u16)(s32)(part->joint_rot[i][2] * def->rot_damp[i * 3 + 2]);
            part->joint_angle[i][0] = (u16)(part->joint_angle[i][0] + part->joint_rot[i][0]);
            part->joint_angle[i][1] = (u16)(part->joint_angle[i][1] + part->joint_rot[i][1]);
            part->joint_angle[i][2] = (u16)(part->joint_angle[i][2] + part->joint_rot[i][2]);
            mhchar_res_node_get(chr, &node, *joints);
            stage_res_node_trans_set(&node, &part->joint_base[i]);
            res_node_rot_set(&node, (360.0f * part->joint_angle[i][0]) / 65536.0f,
                             (360.0f * part->joint_angle[i][1]) / 65536.0f,
                             (360.0f * part->joint_angle[i][2]) / 65536.0f);
            break;
        }
    }
}

/* Writes `pos` as the translation of the resource node `node`. */
extern "C" void stage_res_node_trans_set(nw4r::g3d::ResNode* node, VEC3* pos)
{
    res_node_trans_set(node, pos->x, pos->y, pos->z);
}

/* Arms the scatter of part `part`'s three joints from the scatter definition of `kind`. */
extern "C" void stage_part_scatter_start(StageWork* w, StagePart* part, MHchar* chr, u8* joints, u8 kind)
{
    nw4r::g3d::ResNode node(0);
    VEC3 v;
    u8 i;
    StageScatterDef* def;

    VEC3_ctor(&v);
    def = stg_scatter_defs[kind];
    for (i = 0; i < 3; i++) {
        part->joint_done[i] = 0;
        part->joint_pos[i].x = def->pos[i * 3];
        part->joint_pos[i].y = def->pos[i * 3 + 1];
        part->joint_pos[i].z = def->pos[i * 3 + 2];
        part->joint_rot[i][0] = def->rot[i * 3];
        part->joint_rot[i][1] = def->rot[i * 3 + 1];
        part->joint_rot[i][2] = def->rot[i * 3 + 2];
        if (def->random_spin != 0) {
            u16 spin = ran_suu(1) & 0x5FF;
            if (ran_suu(1) & 1)
                part->joint_angle[i][0] = spin;
            else
                part->joint_angle[i][0] = -spin;
            spin = ran_suu(1) & 0x5FF;
            if (ran_suu(1) & 1)
                part->joint_angle[i][2] = spin;
            else
                part->joint_angle[i][2] = -spin;
        }
        part->joint_time[i] = def->time;
    }
}

/* Hides the joints `ids` (two, 0xFF unused) of `chr` and queues them as the map's visible-joint events. */
extern "C" void stage_part_joints_hide(StageWork* w, MHchar* chr, u8* ids)
{
    nw4r::g3d::ResNode node(0);
    VEC3 v;
    u8 k;
    u32 i;
    u8 j;

    VEC3_ctor(&v);
    k = 0;
    for (i = 0; i < 2; i++, ids++) {
        if (*ids == 0xFF)
            continue;
        chr->setVisibility(*ids, 0);
        for (j = 0; j < 8; j++) {
            if (w->vis_ids[k] != 0xFF) {
                k++;
            } else {
                w->vis_ids[k] = *ids;
                w->vis_ids[k + 1] = 0xFF;
                break;
            }
        }
    }
}

/* Tail-calls stage_part_break(stage_w, 1). */
extern "C" void stage_demo_event_set()
{
    stage_part_break(SW, 1);
}

/* Breaks every live part at once (the map's demo event 12/1). */
extern "C" void stage_parts_break_all(void)
{
    StageWork* w = SW;
    u8 i;

    stage_area_flag_set(12, 1);
    stage_part_break(w, 1);
    for (i = 0; i < 4; i++) {
        if (w->parts[i].state != 0)
            w->parts[i].state = 3;
    }
}

/* Whether at least two of the three map flags (ids 3, 11, 4) are set. */
extern "C" u32 stage_gate_a_ck()
{
    u8 count = 0;
    if (stg_area_flag_index_ck(3) == 1)
        count = 1;
    if (stg_area_flag_index_ck(11) == 1)
        count++;
    if (stg_area_flag_index_ck(4) == 1)
        count++;
    if (count >= 2)
        return 1;
    return 0;
}

/* Completes the map's land-grid file: loads each pair of pack entries into the two land grids. */
extern "C" void stage_land_grid_load_done(char* name, u8* data)
{
    s32 count = pack_entry_count(data) / 2;
    s32 i;
    s32 entry;

    for (i = 0; i < count; i++) {
        pl_land_grid_a_load((u8*)pack_entry_data(data, i * 2), i);
        pl_land_grid_b_load((u8*)pack_entry_data(data, i * 2 + 1), i);
    }
}

/* Requests the land-grid file of map `mapno`'s kind. */
extern "C" void stage_map_res_load(u8 mapno)
{
    StageResFile* f = &map_land_grid_file_tbl[(u8)stage_map_kind_get(mapno)];
    system_work_mem_free_get();
    load_file_req(f->name, system_work_mem_ptr_get(), f->size, (u32)stage_land_grid_load_done, 0, 0);
}

/* Completes one area land-grid file of the arena maps: loads its two pack entries into area `ctx[0]`. */
extern "C" void stage_arena_land_grid_load_done(char* name, u8* data, u32 size, u32* ctx)
{
    u8 area = ctx[0];
    pl_land_grid_a_load((u8*)pack_entry_data(data, 0), area);
    pl_land_grid_b_load((u8*)pack_entry_data(data, 1), area);
}

/* Requests the land-grid file of area `areano` of the arena maps 21 and 22. */
extern "C" void stage_arena_land_grid_load(u8 mapno, u8 areano)
{
    u32 ctx = areano;
    StageResFile* f;

    if (mapno == 21) {
        f = &arena_a_land_grid_file_tbl[areano];
    } else if (mapno == 22) {
        f = &arena_b_land_grid_file_tbl[areano];
    } else {
        return;
    }
    system_work_mem_free_get();
    load_file_req(f->name, system_work_mem_ptr_get(), f->size, (u32)stage_arena_land_grid_load_done, 1, &ctx);
}

/* Narrows both arguments to a byte and forwards to stage_arena_land_grid_load. */
extern "C" void stage_arena_land_grid_load_req(u32 a, u32 b)
{
    stage_arena_land_grid_load((u8)a, (u8)b);
}

/* Clears the area records of map `mapno`, reads each area's record file into them, and loads the map's common eft
 * and efm archives into freshly assigned effect slots. */
extern "C" void stage_map_obj_load(u8 mapno)
{
    StageGroup* g = (StageGroup*)stage_map_group_get(mapno);
    s32 count = g->kind;
    StageArea* areas;
    StageArea* area;
    u8* buf;
    s32 i;
    EftResFile* file;
    u8* proids;
    char** names;
    EftResFile* mfile;
    u8* mproids;
    char** mnames;
    u8 mdl_ids[16];
    u8 ids[88];
    s32 j;

    i = 0;
    areas = SW->areas;
    area = areas;
    for (; i < count; i++) {
        memset(area, 0, sizeof(StageArea));
        area++;
    }
    buf = (u8*)work_mem_alloc(0x1E0);
    if (buf == 0)
        return;
    for (i = 0; i < count; i++) {
        if (g->area_files[i] != 0 && load_file(g->area_files[i], (u32)buf, 0x1E0) > 0)
            memcpy(&areas[i], buf, 0x1E0);
    }
    work_mem_free(buf);

    file = map_com_eft_file_tbl[mapno];
    proids = eft_map_com_proID_tbl_ptr[mapno];
    names = map_com_eft_name_ptr[mapno];
    if (file != 0 && proids != 0 && names != 0) {
        ids[0] = eft_pro_slot_assign(0, 2, 2, mapno, 0);
        for (j = 1, i = 2; proids[i - 1] != 0xFF; j += 2, i += 2) {
            ids[i - 1] = eft_pro_slot_assign(0, 1, 2, mapno, j);
            ids[i] = eft_pro_slot_assign(0, 1, 3, mapno, i);
        }
        eft_res_file_load(file, 0, 0, ids, proids, names);
    }

    mfile = map_com_efm_file_tbl[mapno];
    mproids = efm_map_com_proID_tbl_ptr[mapno];
    mnames = eft_map_common_mdl_name_ptr[mapno];
    if (mfile != 0 && mproids != 0 && mnames != 0) {
        for (j = 0; mproids[j] != 0xFF; j++) {
            u8 id = eft_pro_slot_assign(0, 3, 2, mapno, j);
            if (id == 0xFF)
                break;
            mdl_ids[j] = id;
        }
        eft_res_file_load(mfile, 3, 0, mdl_ids, mproids, mnames);
    }
}

/* Whether `pos` lies in one of the zones of list `zone`; `out` receives the zone hit. */
extern "C" u32 stage_zone_hit_ck(StageZone* zone, StageZone** out, VEC3* pos)
{
    s32 hit = 0;
    VEC3 a;
    VEC3 b;

    VEC3_ctor(&a);
    VEC3_ctor(&b);
    if (zone == 0)
        return 0;
    for (; zone->dest != 0xFF; zone++) {
        switch (zone->kind) {
        case 0:
            if (pos->y >= zone->pos.y - 0.1f && pos->y < zone->pos.y + zone->height) {
                f32 dx = pos->x - zone->pos.x;
                f32 dz = pos->z - zone->pos.z;
                if (sqrtf_thunk(dx * dx + dz * dz) <= zone->radius)
                    hit = 1;
                else
                    hit = 0;
            }
            break;
        case 1:
            setVector3(&a, zone->pos.x, zone->pos.y, zone->pos.z);
            setVector3(&b, zone->pos2.x, zone->pos2.y, zone->pos2.z);
            hit = vec3_segment_box_ck(pos, &a, &b, zone->height, zone->radius);
            break;
        }
        if (hit != 0) {
            *out = zone;
            return 1;
        }
    }
    return 0;
}

/* Warps the local player when it stands in one of the current area's warp zones. */
extern "C" u32 stage_warp_zone_check(void)
{
    VEC3 pos;
    StageZone* zone;
    _PLW* plw;
    u8 area;

    VEC3_ctor(&pos);
    plw = (_PLW*)get_move_work_adrs(2);
    plw = &plw[(s8)my_player_no()];
    if (plw == 0)
        return 0;
    if (pl_warp_ready_ck(plw) == 0)
        return 0;
    area = get_now_areano();
    if (area == 0xFF)
        return 0;
    if (stage_zone_hit_ck(stage_group_word_get(get_now_mapno(), area), &zone, &plw->vec_0x03C) == 1) {
        setVector3(&pos, zone->warp_pos.x, zone->warp_pos.y, zone->warp_pos.z);
        pl_act_stage_latch_set(plw, 1);
        pl_warp_start(zone->dest, &pos, zone->warp_angle);
        return 1;
    }
    return 0;
}

/* Warps each player standing in one of the current area's warp zones (the two-player view). */
extern "C" void stage_warp_zone_check_split(void)
{
    VEC3 pos;
    StageZone* zone;
    StageMoveView* move;
    _PLW* plw;
    u8 area;
    u8 player;
    s8 i;

    VEC3_ctor(&pos);
    move = (StageMoveView*)get_move_work_adrs(0);
    if (move == 0)
        return;
    area = get_now_areano();
    if (area == 0xFF)
        return;
    player = my_player_no();
    plw = (_PLW*)get_move_work_adrs(2);
    for (i = 0; i < 2; i++, plw++) {
        my_player_no_set(i);
        if (plw != 0 && pl_warp_ready_ck(plw) != 0 &&
            stage_zone_hit_ck(stage_group_word_get(get_now_mapno(), area), &zone, &plw->vec_0x03C) == 1) {
            setVector3(&pos, zone->warp_pos.x, zone->warp_pos.y, zone->warp_pos.z);
            pl_act_stage_latch_set(plw, 1);
            pl_warp_start(zone->dest, &pos, zone->warp_angle);
            if (plw->chunk_ofs != 0) {
                move->field_0x04 = 0;
                move->field_0x06 = 10;
            }
        }
    }
    my_player_no_set(player);
}

/* Resets the stage's per-area cursor block and re-seats the world vector. */
extern "C" void stage_light_reset()
{
    s32 i;
    SW->light.kind = 0;
    SW->light.intensity = 0.5f;
    for (i = 3; i >= 0; i--)
        SW->light.colours[i] = 0xFFFFFF00;
    for (i = 11; i >= 4; i--)
        SW->light.colours[i] = 0xFFFFFF00;
    setVector3(&SW->light_dir, 5000.0f, 5000.0f, 5000.0f);
}

/* Blends the two packed colours channel by channel and returns the packed result. */
extern "C" u32 stage_colour_blend(u32 from, u32 to, f32 t)
{
    u8 a, b;
    u32 r0, r1, r2, r3;

    a = from >> 24; b = to >> 24; r0 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from >> 16; b = to >> 16; r1 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from >> 8;  b = to >> 8;  r2 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from;       b = to;       r3 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;

    return (r0 << 24) | (r1 << 16) | (r2 << 8) | r3;
}

namespace view_fn_802B2AA0 {

/* Blends the area's three colour groups by the camera/area angle and arms the six results as one
 * sound/effect frame set. */
extern "C" void stage_light_blend_set(StageBlendRec* rec, void* obj, u8 store)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 world;
    nw4r::math::VEC3 cam;
    u32 ang_x;
    u32 ang_y;
    s32 base;
    u32 colour[6];
    u16 delta;
    f32 t;

    VEC3_ctor(&poly);
    base = camera_angle_y_get();
    subVec3(&world, &::get_camera_pos(), obj);
    copyVec3(&poly, &world);
    calcVecAngXY(&poly, &ang_x, &ang_y);

    delta = (u16)(ang_y - base);
    if (delta < 0x4000U) {
        t = (f32)delta / 16384.0f;
        colour[0] = stage_colour_blend(rec->group[0].base, rec->group[0].target_a, t);
        colour[1] = stage_colour_blend(rec->group[0].base, rec->group[0].target_b, t);
        colour[2] = stage_colour_blend(rec->group[2].base, rec->group[2].target_a, t);
        colour[3] = stage_colour_blend(rec->group[2].base, rec->group[2].target_b, t);
        colour[4] = stage_colour_blend(rec->group[1].base, rec->group[1].target_a, t);
        colour[5] = stage_colour_blend(rec->group[1].base, rec->group[1].target_b, t);
    } else if (delta < 0x8000U) {
        t = (f32)(s32)(delta - 0x4000) / 16384.0f;
        colour[0] = stage_colour_blend(rec->group[0].target_a, rec->group[0].axis, t);
        colour[1] = stage_colour_blend(rec->group[0].target_b, rec->group[0].axis, t);
        colour[2] = stage_colour_blend(rec->group[2].target_a, rec->group[2].axis, t);
        colour[3] = stage_colour_blend(rec->group[2].target_b, rec->group[2].axis, t);
        colour[4] = stage_colour_blend(rec->group[1].target_a, rec->group[1].axis, t);
        colour[5] = stage_colour_blend(rec->group[1].target_b, rec->group[1].axis, t);
    } else if (delta < 0xC000U) {
        t = (f32)(s32)(delta - 0x8000) / 16384.0f;
        colour[0] = stage_colour_blend(rec->group[0].axis, rec->group[0].target_b, t);
        colour[1] = stage_colour_blend(rec->group[0].axis, rec->group[0].target_a, t);
        colour[2] = stage_colour_blend(rec->group[2].axis, rec->group[2].target_b, t);
        colour[3] = stage_colour_blend(rec->group[2].axis, rec->group[2].target_a, t);
        colour[4] = stage_colour_blend(rec->group[1].axis, rec->group[1].target_b, t);
        colour[5] = stage_colour_blend(rec->group[1].axis, rec->group[1].target_a, t);
    } else {
        t = (f32)(s32)(delta - 0xC000) / 16384.0f;
        colour[0] = stage_colour_blend(rec->group[0].target_b, rec->group[0].base, t);
        colour[1] = stage_colour_blend(rec->group[0].target_a, rec->group[0].base, t);
        colour[2] = stage_colour_blend(rec->group[2].target_b, rec->group[2].base, t);
        colour[3] = stage_colour_blend(rec->group[2].target_a, rec->group[2].base, t);
        colour[4] = stage_colour_blend(rec->group[1].target_b, rec->group[1].base, t);
        colour[5] = stage_colour_blend(rec->group[1].target_a, rec->group[1].base, t);
    }

    if (rec->enabled == 0) {
        colour[4] = 0;
        colour[5] = 0;
    }
    if (store == 0) {
        f32 frame = rec->se_frame;
        filter_panel_upper_set(0, 6, colour, frame);
        return;
    }
    {
        f32 frame = rec->se_frame;
        filter_panel_lower_set(0, 6, colour, frame);
    }
}

/* Copies the current area's staged colour record out of the stage block and drives it. */
extern "C" void stage_area_colour_update(void)
{
    StageWork* st = (StageWork*)&stage_w;
    s32 handle;
    StageFogRec rec;
    StageFogRec fog;

    handle = ((nw4r::g3d::ScnRoot*)pRoot)->GetFog(0);
    stage_fog_copy(&fog, &handle);
    fn_8007A1B8(&fog, 0, &fog.colour, &fog.end, 0, 0, &fog.start);

    do {
        if (GameMode_ck() == 2) {
            if (getItemListSelection() == 0) {
                stage_fog_rec_copy(&rec, &st->areas[st->areano].fog);
            } else {
                stage_fog_rec_copy(&rec, &st->areas[st->areano].fog_b);
            }
        } else {
            if (stage_water_enabled_ck() == 0) {
                break;
            }
            if (camera_underwater_ck() == 1U) {
                stage_fog_rec_copy(&rec, &st->areas[st->areano].fog_b);
            } else {
                stage_fog_rec_copy(&rec, &st->areas[st->areano].fog);
            }
        }
        stage_fog_apply(&rec);
    } while (0);
}

/* Copies the fog record at `src` into `dst`. */
extern "C" void stage_fog_rec_copy(StageFogRec* dst, const StageFogRec* src)
{
    dst->type = src->type;
    dst->start = src->start;
    dst->end = src->end;
    dst->colour = src->colour;
}

/* The joint lists (show, hide) the area models switch per map and area: `stg_vis_*` for the area
 * model (by map, then area), `stg_model_vis_*` for the map model, `stg_toggle_*` for the
 * quest- and event-driven toggles. */
extern "C" {

extern u8 stg_model_vis_m3a2_hide[];

extern u8 stg_model_vis_m3a4_hide[];

extern u8 stg_model_vis_m3a8_hide[];

extern u8 stg_toggle_m22a1_show[];

u8 stg_vis_m0a5_show[2] = {0x01, 0xFF};

u8 stg_vis_m0a5_hide[2] = {0x02, 0xFF};

StageJointLists stg_vis_m0a5 = {stg_vis_m0a5_show, stg_vis_m0a5_hide};

u8 stg_vis_m1a0_show[2] = {0x01, 0xFF};

u8 stg_vis_m1a0_hide[2] = {0x02, 0xFF};

StageJointLists stg_vis_m1a0 = {stg_vis_m1a0_show, stg_vis_m1a0_hide};

u8 stg_model_vis_m1a7_show[1] = {0xFF};

u8 stg_model_vis_m1a7_hide[2] = {0x0E, 0xFF};

StageJointLists stg_model_vis_m1a7 = {stg_model_vis_m1a7_show, stg_model_vis_m1a7_hide};

u8 stg_vis_m1a12_show[4] = {0x01, 0x02, 0x03, 0xFF};

u8 stg_vis_m1a12_hide[4] = {0x04, 0x05, 0x06, 0xFF};

StageJointLists stg_vis_m1a12 = {stg_vis_m1a12_show, stg_vis_m1a12_hide};

u8 stg_model_vis_m1a12_show[1] = {0xFF};

u8 stg_model_vis_m1a12_hide[2] = {0x0D, 0xFF};

StageJointLists stg_model_vis_m1a12 = {stg_model_vis_m1a12_show, stg_model_vis_m1a12_hide};

u8 stg_vis_m3a0_show[2] = {0x01, 0xFF};

u8 stg_vis_m3a0_hide[2] = {0x02, 0xFF};

StageJointLists stg_vis_m3a0 = {stg_vis_m3a0_show, stg_vis_m3a0_hide};

u8 stg_model_vis_m3a2_show[3] = {0x01, 0x02, 0xFF};

StageJointLists stg_model_vis_m3a2 = {stg_model_vis_m3a2_show, stg_model_vis_m3a2_hide};

u8 stg_model_vis_m3a3_show[4] = {0x01, 0x02, 0x03, 0xFF};

u8 stg_model_vis_m3a3_hide[8] = {0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF};

StageJointLists stg_model_vis_m3a3 = {stg_model_vis_m3a3_show, stg_model_vis_m3a3_hide};

u8 stg_model_vis_m3a4_show[2] = {0x01, 0xFF};

StageJointLists stg_model_vis_m3a4 = {stg_model_vis_m3a4_show, stg_model_vis_m3a4_hide};

u8 stg_model_vis_m3a6_show[2] = {0x08, 0xFF};

u8 stg_model_vis_m3a6_hide[1] = {0xFF};

StageJointLists stg_model_vis_m3a6 = {stg_model_vis_m3a6_show, stg_model_vis_m3a6_hide};

u8 stg_model_vis_m3a8_show[2] = {0x01, 0xFF};

StageJointLists stg_model_vis_m3a8 = {stg_model_vis_m3a8_show, stg_model_vis_m3a8_hide};

StageJointLists* stg_vis_m9[2] = {NULL, &stg_vis_m1a0};

u8 stg_model_vis_m9a0_show[2] = {0x04, 0xFF};

u8 stg_model_vis_m9a0_hide[2] = {0x05, 0xFF};

StageJointLists stg_model_vis_m9a0 = {stg_model_vis_m9a0_show, stg_model_vis_m9a0_hide};

u8 stg_model_vis_m9a1_show[2] = {0x01, 0xFF};

u8 stg_model_vis_m9a1_hide[2] = {0x02, 0xFF};

StageJointLists stg_model_vis_m9a1 = {stg_model_vis_m9a1_show, stg_model_vis_m9a1_hide};

StageJointLists* stg_model_vis_m9[2] = {&stg_model_vis_m9a0, &stg_model_vis_m9a1};

StageJointLists* stg_vis_m11[2] = {NULL, &stg_vis_m1a0};

u8 stg_toggle_m1a0_show[3] = {0x01, 0x03, 0xFF};

u8 stg_toggle_m1a0_hide[2] = {0x02, 0xFF};

StageJointLists stg_toggle_m1a0 = {stg_toggle_m1a0_show, stg_toggle_m1a0_hide};

u8 stg_toggle_m1a1_show[2] = {0x01, 0xFF};

u8 stg_toggle_m1a1_hide[2] = {0x02, 0xFF};

StageJointLists stg_toggle_m1a1 = {stg_toggle_m1a1_show, stg_toggle_m1a1_hide};

u8 stg_toggle_m1a6_show[2] = {0x01, 0xFF};

u8 stg_toggle_m1a6_hide[2] = {0x02, 0xFF};

StageJointLists stg_toggle_m1a6 = {stg_toggle_m1a6_show, stg_toggle_m1a6_hide};

u8 stg_toggle_m8a0_show[2] = {0x01, 0xFF};

u8 stg_toggle_m8a0_hide[2] = {0x02, 0xFF};

StageJointLists stg_toggle_m8a0 = {stg_toggle_m8a0_show, stg_toggle_m8a0_hide};

u8 stg_toggle_m21a0_b[4] = {0x02, 0x12, 0x13, 0xFF};

u8 stg_toggle_m21a1_b[4] = {0x06, 0x11, 0x12, 0xFF};

u8 stg_toggle_m21a2_b[5] = {0x01, 0x04, 0x0C, 0x0D, 0xFF};

u8 stg_toggle_m21a3_a[5] = {0x01, 0x03, 0x05, 0x06, 0xFF};

u8 stg_toggle_m21a3_b[5] = {0x02, 0x04, 0x07, 0x08, 0xFF};

u8 stg_toggle_m21a5_show[2] = {0x07, 0xFF};

u8 stg_toggle_m21a5_hide[2] = {0x08, 0xFF};

StageJointLists stg_toggle_m21a5 = {stg_toggle_m21a5_show, stg_toggle_m21a5_hide};

u8 stg_toggle_m21a6_show[2] = {0x04, 0xFF};

u8 stg_toggle_m21a6_hide[2] = {0x05, 0xFF};

StageJointLists stg_toggle_m21a6 = {stg_toggle_m21a6_show, stg_toggle_m21a6_hide};

u8 stg_toggle_m21a7_b[8] = {0x17, 0x18, 0x19, 0x1D, 0x1E, 0x1F, 0x23, 0xFF};

u8 stg_toggle_m22a0_on[4] = {0x02, 0x03, 0x04, 0xFF};

u8 stg_toggle_m22a0_off[4] = {0x05, 0x06, 0x07, 0xFF};

u8 stg_toggle_m22a1_hide[4] = {0x0F, 0x11, 0x13, 0xFF};

StageJointLists stg_toggle_m22a1 = {stg_toggle_m22a1_show, stg_toggle_m22a1_hide};

StageJointLists* stg_vis_m0[7] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &stg_vis_m0a5,
    &stg_vis_m0a5,
};

StageJointLists* stg_vis_m1[13] = {
    &stg_vis_m1a0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &stg_vis_m1a0,
    NULL,
    NULL,
    NULL,
    NULL,
    &stg_vis_m1a12,
};

u8 stg_model_vis_m3a2_hide[9] = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0xFF};

u8 stg_model_vis_m3a4_hide[10] = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF};

u8 stg_model_vis_m3a8_hide[12] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0xFF};

StageJointLists* stg_vis_m3[11] = {
    &stg_vis_m3a0,
    &stg_vis_m3a0,
    &stg_vis_m3a0,
    &stg_vis_m3a0,
    &stg_vis_m3a0,
    NULL,
    &stg_vis_m3a0,
    NULL,
    &stg_vis_m3a0,
    NULL,
    &stg_vis_m3a0,
};

StageJointLists* stg_model_vis_m3[11] = {
    NULL,
    NULL,
    &stg_model_vis_m3a2,
    &stg_model_vis_m3a3,
    &stg_model_vis_m3a4,
    NULL,
    &stg_model_vis_m3a6,
    NULL,
    &stg_model_vis_m3a8,
    NULL,
    NULL,
};

StageJointLists* stg_vis_m8[3] = {
    NULL,
    NULL,
    &stg_vis_m1a0,
};

StageJointLists** stg_vis_maps[23] = {
    stg_vis_m0,
    stg_vis_m1,
    NULL,
    stg_vis_m3,
    NULL,
    NULL,
    NULL,
    NULL,
    stg_vis_m8,
    stg_vis_m9,
    NULL,
    stg_vis_m11,
    stg_vis_m1,
    NULL,
    stg_vis_m3,
    NULL,
    NULL,
    NULL,
    NULL,
    stg_vis_m8,
    stg_vis_m11,
    NULL,
    NULL,
};

StageJointLists* stg_toggle_m1[13] = {
    &stg_toggle_m1a0,
    &stg_toggle_m1a1,
    NULL,
    NULL,
    NULL,
    NULL,
    &stg_toggle_m1a6,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

u8 stg_toggle_m21a0_a[17] = {0x01, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0xFF};

u8 stg_toggle_m21a1_a[11] = {0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0xFF};

u8 stg_toggle_m21a2_a[10] = {0x02, 0x03, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF};

u8 stg_toggle_m21a7_a[24] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x1A, 0x1B, 0x1C, 0x22, 0xFF};

u8 stg_toggle_m22a1_show[14] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x10, 0x12, 0xFF};

}

/* Builds the current area's visibility set from the map/area tables: shows the kept joints that are not
 * visible-joint events and hides the dropped ones. */
extern "C" void stage_area_visibility_build(StageWork* st, u8 kind)
{
    nw4r::math::VEC3 poly;
    u8* keep;
    StageJointLists* list;
    MHchar* chr;
    u8 group;
    u8* drop;
    u8 player;
    u8 changed;
    u8 found;

    list = NULL;
    group = 0;
    player = 0;
    changed = 0;
    chr = &st->chars[kind]->chr;
    VEC3_ctor(&poly);

    if (kind == 1) {
        if (stg_vis_maps[st->mapno] != NULL) {
            list = stg_vis_maps[st->mapno][st->areano];
        }
    } else if (kind == 0) {
        switch (st->mapno) {
        case 1:
        case 12:
            if (st->areano == 7) {
                list = &stg_model_vis_m1a7;
            } else if (st->areano == 0xC) {
                list = &stg_model_vis_m1a12;
            }
            break;
        case 3:
        case 14:
            list = stg_model_vis_m3[st->areano];
            break;
        case 9:
            list = stg_model_vis_m9[st->areano];
            group = 1;
            break;
        }
    } else {
        return;
    }

    if (list == NULL) {
        return;
    }
    if (group == 1U && kind == 0) {
        copyVec3(&poly, &::get_camera_pos());
        player = my_player_no();
        if (stage_pos_z_low_ck(&poly) == 1U) {
            keep = list->show;
            drop = list->hide;
            found = 1;
        } else {
            keep = list->hide;
            drop = list->show;
            found = 2;
        }
        if (st->joint_state[0][kind] != st->joint_state[1][kind]) {
            changed = 1;
        }
    } else {
        if (::get_camera_pos().y >= 0.0f) {
            keep = list->show;
            drop = list->hide;
            found = 1;
        } else {
            keep = list->hide;
            drop = list->show;
            found = 2;
        }
        if (found != st->joint_state[0][kind]) {
            changed = 1;
        }
    }

    stage_part_defs_get(st, st->areano);

    if (changed != 0) {
        for (; *keep != 0xFF; keep++) {
            u8 hit = 0;
            u8 j;
            for (j = 0; j < 8; j++) {
                if (st->vis_ids[j] == 0xFF)
                    break;
                if (*keep == st->vis_ids[j]) {
                    hit = 1;
                    break;
                }
            }
            if (hit == 0) {
                chr->setVisibility(*keep, 1);
            }
        }
        for (; *drop != 0xFF; drop++) {
            chr->setVisibility(*drop, 0);
        }
    }
    st->joint_state[player][kind] = found;
}

/* Tail-calls the colour/visibility driver for the map's area animation set 0. */
extern "C" void stage_area_anim_cb0(void)
{
    stage_area_visibility_build((StageWork*)&stage_w, 0);
}

/* Tail-calls the colour/visibility driver for the map's area animation set 1. */
extern "C" void stage_area_anim_cb1(void)
{
    stage_area_visibility_build((StageWork*)&stage_w, 1);
}

/* Builds the current area's joint visibility set for the map's animation mode and applies it to the
 * area's character: one list of joint ids is shown, the other hidden. */
extern "C" void stage_area_joint_visibility_set(StageWork* st, MHchar* chr, u8 mode)
{
    StageJointLists local;
    u8 hide_buf[0x24];
    u8 show_buf[0x24];
    StageJointLists* lists = NULL;
    s32 flag = 0;
    u8 hide_i = 0;
    u8 show_i = 0;

    switch (st->mapno) {
    case 1:
    case 12: {
        u8 area = st->areano;

        lists = stg_toggle_m1[area];
        switch (area) {
        case 0:
            if (mode == 1) {
                return;
            }
            if (quest_id_head_ck() == 1) {
                flag = 1;
            }
            break;
        case 1:
            if (quest_id_tail_ck() == 1) {
                flag = 1;
            }
            break;
        case 6:
            if (mode == 0) {
                return;
            }
            if (mode == 1) {
                flag = 1;
            }
            break;
        }
        break;
    }
    case 8:
    case 19:
        if (st->areano == 0) {
            lists = &stg_toggle_m8a0;
            flag = 0;
        }
        break;
    case 11:
    case 20:
        if (st->areano == 0) {
            lists = &stg_toggle_m8a0;
            flag = 1;
        }
        break;
    case 22: {
        u8 area = st->areano;

        if (area == 0) {
            const u8* src;

            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (lb_event_state_ck() == 1) {
                src = stg_toggle_m22a0_on;
            } else {
                src = stg_toggle_m22a0_off;
            }
            while (*src != 0xFF) {
                local.show[show_i] = *src;
                src++;
                show_i++;
            }
            if (getItemListSelection() == 0) {
                local.show[show_i] = 1;
                show_i++;
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
        } else if (area == 1) {
            lists = &stg_toggle_m22a1;
            if (getItemListSelection() == 1) {
                flag = 1;
            }
        } else if (area == 2) {
            const u8* src;

            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            switch (lb_world_upgrade_get(0)) {
            case 0:
            case 1:
                local.show[0] = 3;
                local.show[1] = 4;
                show_i = 2;
                break;
            case 2:
                local.show[0] = 1;
                local.show[1] = 4;
                show_i = 2;
                break;
            case 3:
                local.show[0] = 1;
                local.show[1] = 2;
                show_i = 2;
                break;
            }
            switch (lb_world_upgrade_get(2)) {
            case 0:
                local.show[show_i] = 8;
                show_i++;
                local.show[show_i] = 9;
                show_i++;
                local.show[show_i] = 10;
                show_i++;
                break;
            case 1:
                local.show[show_i] = 9;
                show_i++;
                local.show[show_i] = 10;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 10;
                show_i++;
                break;
            }
            switch (lb_world_upgrade_get(3)) {
            case 0:
                local.show[show_i] = 11;
                show_i++;
                local.show[show_i] = 12;
                show_i++;
                local.show[show_i] = 13;
                show_i++;
                break;
            case 1:
                local.show[show_i] = 12;
                show_i++;
                local.show[show_i] = 13;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 13;
                show_i++;
                break;
            }
            switch (lb_world_upgrade_get(1)) {
            case 0:
            case 1:
                local.show[show_i] = 14;
                show_i++;
                local.show[show_i] = 15;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 15;
                show_i++;
                break;
            }
            if (lb_unlock_cond5_get() == 0) {
                local.show[show_i] = 6;
                show_i++;
            } else {
                local.show[show_i] = 5;
                show_i++;
            }
            if (lb_unlock_cond22_flag_ck() == 1) {
                local.show[show_i] = 0x10;
                show_i++;
            }
            if (getItemListSelection() == 0) {
                local.show[show_i] = 7;
                show_i++;
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
        }
        break;
    }
    case 21: {
        u8 area = st->areano;

        switch (area) {
        case 0:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = stg_toggle_m21a0_a;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a0_b;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 0x12;
                    show_i++;
                    local.show[show_i] = 0x13;
                    show_i++;
                }
            } else {
                const u8* src;

                src = stg_toggle_m21a0_b;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a0_a;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 0x10;
                    show_i++;
                    local.show[show_i] = 0x11;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 1:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = stg_toggle_m21a1_a;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a1_b;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 0x11;
                    show_i++;
                    local.show[show_i] = 0x12;
                    show_i++;
                }
            } else {
                const u8* src;

                src = stg_toggle_m21a1_b;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a1_a;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 0x0F;
                    show_i++;
                    local.show[show_i] = 0x10;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 2:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = stg_toggle_m21a2_a;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a2_b;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 4;
                    show_i++;
                    local.show[show_i] = 0x0C;
                    show_i++;
                    local.show[show_i] = 0x0D;
                    show_i++;
                } else if (lb_event_gate2_ck() == 1) {
                    local.show[show_i] = 4;
                    show_i++;
                }
            } else {
                const u8* src;

                src = stg_toggle_m21a2_b;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a2_a;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 2;
                    show_i++;
                    local.show[show_i] = 0x0A;
                    show_i++;
                    local.show[show_i] = 0x0B;
                    show_i++;
                } else if (lb_event_gate2_ck() == 1) {
                    local.show[show_i] = 2;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 3:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = stg_toggle_m21a3_a;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a3_b;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 7;
                    show_i++;
                    local.show[show_i] = 8;
                    show_i++;
                } else {
                    local.show[show_i] = 4;
                    show_i++;
                }
            } else {
                const u8* src;

                src = stg_toggle_m21a3_b;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a3_a;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 5;
                    show_i++;
                    local.show[show_i] = 6;
                    show_i++;
                } else {
                    local.show[show_i] = 3;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 5:
            lists = &stg_toggle_m21a5;
            if (getItemListSelection() == 1) {
                flag = 1;
            }
            break;
        case 6:
            lists = &stg_toggle_m21a6;
            if (getItemListSelection() == 1) {
                flag = 1;
            }
            break;
        case 7:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = stg_toggle_m21a7_a;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a7_b;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 0x19;
                    show_i++;
                    local.show[show_i] = 0x1D;
                    show_i++;
                    local.show[show_i] = 0x1E;
                    show_i++;
                    local.show[show_i] = 0x23;
                    show_i++;
                } else {
                    if (lb_event_gate2_ck() == 1) {
                        local.show[show_i] = 0x19;
                        show_i++;
                    }
                    local.show[show_i] = 0x17;
                    show_i++;
                    local.show[show_i] = 0x18;
                    show_i++;
                    local.show[show_i] = 0x1F;
                    show_i++;
                }
            } else {
                const u8* src;

                src = stg_toggle_m21a7_b;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = stg_toggle_m21a7_a;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && lb_event_gate2_ck() == 0) {
                    local.show[show_i] = 5;
                    show_i++;
                    local.show[show_i] = 0x1B;
                    show_i++;
                    local.show[show_i] = 0x1C;
                    show_i++;
                    local.show[show_i] = 0x22;
                    show_i++;
                } else {
                    if (lb_event_gate2_ck() == 1) {
                        local.show[show_i] = 5;
                        show_i++;
                    }
                    local.show[show_i] = 4;
                    show_i++;
                    local.show[show_i] = 0x16;
                    show_i++;
                    local.show[show_i] = 0x1A;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        }
        break;
    }
    }
    if (lists != NULL) {
        const u8* shown;
        const u8* hidden;

        if (flag != 0) {
            shown = lists->hide;
            hidden = lists->show;
        } else {
            shown = lists->show;
            hidden = lists->hide;
        }
        while (*shown != 0xFF) {
            chr->setVisibility(*shown, true);
            shown++;
        }
        while (*hidden != 0xFF) {
            chr->setVisibility(*hidden, false);
            hidden++;
        }
    }
}

/* Hides every joint the current area's kind list names. */
extern "C" void stage_area_kind_joints_hide(StageWork* st, MHchar* chr)
{
    u8* p;

    chr->get_joint_num();
    p = stage_area_word_get(st->areano);
    if (p == NULL) {
        return;
    }
    while (*p != 0xFF) {
        chr->setVisibility(*p, 0);
        p++;
    }
}

/* The frame strips (start, step, end; -1 ends) and the points the map 10 koware effects spawn at. */
extern "C" {

f32 stg_eft_strip_frames[15] = {
    320.0f, 2000.0f, 20000.0f,
    620.0f, 2000.0f, 20000.0f,
    890.0f, 2000.0f, 20000.0f,
    350.0f, 2000.0f, 20000.0f,
    -1.0f, -1.0f, -1.0f,
};

Vec stg_eft_strip_points[4] = {
    {2267.0f, 240.0f, -9440.0f},
    {-9270.0f, 200.0f, -1290.0f},
    {6075.0f, 307.0f, -7450.0f},
    {130.0f, -40.0f, 7100.0f},
};

}

/* Spawns the `eft028` break effects along the map's effect strips. */
extern "C" void stage_eft_strips_spawn(StageWork* st)
{
    nw4r::math::VEC3 pos;
    f32 limit;
    s32 i;
    f32* strip;
    u8* slot;

    VEC3_ctor(&pos);
    if (st->mapno == 0xA && st->areano == 1) {
        strip = stg_eft_strip_frames;
        slot = (u8*)stg_eft_strip_points;
        limit = 0.0f;
        for (; strip[0] > limit; strip += 3, slot += 0x0C) {
            for (i = 0; (f32)i < strip[2] / strip[1]; i++) {
                f32 frame = strip[0] + (f32)i * strip[1];
                if (mhchar_frame_check((void*)st->chars[0], 0, 0, frame, 0.0f) != 0) {
                    vec_to_mh_vec3(&pos, (struct Vec*)slot);
                    ::eft028_set_koware(0xB, &pos, st->areano, 0);
                }
            }
        }
    }
}

/* Spawns the map/area's fixed break effects and sound. */
extern "C" void stage_area_break_effects_spawn(StageWork* st)
{
    nw4r::math::VEC3 pos;
    u8 mapno;
    u8 areano;

    VEC3_ctor(&pos);
    stage_eft_strips_spawn(st);
    mapno = st->mapno;
    if (mapno != 5 && mapno != 0x10) {
        return;
    }
    areano = st->areano;
    switch (areano) {
    case 9:
        if ((s32)(system_w.field_0x0c % 170) == 0) {
            setVector3(&pos, 23700.0f, (-25000.0f), 16900.0f);
            ::eft028_set_koware(6, &pos, st->areano, 0);
            return;
        }
        break;
    case 10:
        if ((s32)(system_w.field_0x0c % 160) == 0) {
            setVector3(&pos, 38400.0f, (-32900.0f), (-44000.0f));
            ::eft028_set_koware(6, &pos, st->areano, 0);
            camera_quake_req(0x43);
            fn_800DD38C();
        }
        break;
    }
}

#pragma opt_common_subs off
/* The ground impact heights, points and the joint lists of each height band. */
extern "C" {

f32 stg_impact_heights[3] = {-2500.0f, -600.0f, 1400.0f};

Vec stg_impact_points[19] = {
    {0.0f, 0.0f, 0.0f},
    {1312.0f, 1700.0f, -3235.0f},
    {-213.0f, 1700.0f, -2980.0f},
    {600.0f, 1700.0f, -1965.0f},
    {1460.0f, 1700.0f, -2760.0f},
    {510.0f, 1700.0f, -2759.0f},
    {-677.0f, 1700.0f, -1983.0f},
    {-620.0f, 1700.0f, -1738.0f},
    {-790.0f, 1700.0f, 253.0f},
    {319.0f, 1700.0f, -515.0f},
    {-1044.0f, 1700.0f, -634.0f},
    {30.0f, 1700.0f, -1238.0f},
    {55.0f, 1700.0f, 317.0f},
    {104.0f, 1700.0f, 1200.0f},
    {-975.0f, 1700.0f, 2000.0f},
    {-38.0f, 1700.0f, 2482.0f},
    {-1000.0f, 1700.0f, 3226.0f},
    {-536.0f, 1700.0f, 2040.0f},
    {-804.0f, 1700.0f, 900.0f},
};

u8 stg_impact_joints_a0[4] = {0x01, 0x02, 0x03, 0xFF};

u8 stg_impact_joints_a1[4] = {0x06, 0x09, 0x0A, 0xFF};

u8 stg_impact_joints_a2[4] = {0x09, 0x0A, 0x12, 0xFF};

u8 stg_impact_joints_a3[4] = {0x0D, 0x0E, 0x0F, 0xFF};

u8 stg_impact_joints_b0[4] = {0x04, 0x05, 0x06, 0xFF};

u8 stg_impact_joints_b1[4] = {0x03, 0x07, 0x0B, 0xFF};

u8 stg_impact_joints_b2[4] = {0x08, 0x0C, 0x0D, 0xFF};

u8 stg_impact_joints_b3[4] = {0x10, 0x11, 0x12, 0xFF};

u8* stg_impact_joints_a[4] = {stg_impact_joints_a0, stg_impact_joints_a1, stg_impact_joints_a2, stg_impact_joints_a3};

u8* stg_impact_joints_b[4] = {stg_impact_joints_b0, stg_impact_joints_b1, stg_impact_joints_b2, stg_impact_joints_b3};

}

/* Seats the map/area's joint effects on the `shell_set_func_ptr` job table. */
extern "C" void stg_ground_impact_set(MHchar* chr, u8 kind, u16 flags)
{
    nw4r::math::VEC3 pos;
    s8 order[3];
    f32* thresholds;
    u8* joints;
    u8* strip;
    u8** joint_tables;
    s32 band;
    u8 joint;
    s32 i;
    u8 index;
    u8 mapno;

    VEC3_ctor(&pos);
    strip = NULL;
    mapno = get_now_mapno();
    if (((s32)mapno == 4 || (s32)mapno == 0xF) && (s32)kind == 5) {
        strip = (u8*)stg_impact_points;
        thresholds = stg_impact_heights;
        joint_tables = (flags & 1) ? stg_impact_joints_a : stg_impact_joints_b;
    }
    if (strip == NULL) {
        return;
    }
    for (band = 0; band < 3; band++) {
        if (chr->pos_0x04.y < thresholds[band]) {
            break;
        }
    }
    joints = joint_tables[band];
    order[0] = flags % 3;
    if ((flags & 0x10) != 0) {
        for (i = 1; i < 3; i++) {
            order[i] = order[i - 1] + 1;
            if (order[i] >= 3) {
                order[i] = 0;
            }
        }
    } else {
        for (i = 1; i < 3; i++) {
            order[i] = order[i - 1] - 1;
            if (order[i] < 0) {
                order[i] = 2;
            }
        }
    }
    for (index = 0; (s8)(joint = *joints) > 0; joints++, index++) {
        vec_to_mh_vec3(&pos, (struct Vec*)(strip + (s8)joint * 0xC));
        shell_set_func_ptr->method_0x88(&pos, kind, (s16)(order[index] * 0xA), shell_set_func_ptr);
    }
}
#pragma opt_common_subs reset

/* Raises the stage block's one-shot flag byte. */
extern "C" void stage_demo_flags_set(void)
{
    ((StageWork*)&stage_w)->oneshot_flags |= 0x81;
}

/* Returns whether the stage block's one-shot flag byte has bit 0 clear. */
extern "C" s32 stage_oneshot_clear_ck(void)
{
    return (((StageWork*)&stage_w)->oneshot_flags & 1) != 0;
}

/* Arms area `index`'s timer on first use and keeps its 4-second window. */
extern "C" void stage_area_timer_arm(u8 index)
{
    StageWork* st = (StageWork*)&stage_w;
    u16 bit = 1 << index;
    s32 now;

    if ((st->area_mask & bit) == 0) {
        st->area_mask |= bit;
        eft038_set();
    }
    now = quest_time_limit_get();
    st->area_timers[index] = now - quest_time_elapsed_get();
}

/* Dispatches an area-index event to either the local stage or the caller's own handler. */
extern "C" void stage_area_event_dispatch(void* plw, u8 index)
{
    if (index >= 0x10U) {
        return;
    }
    if (plw == NULL) {
        stage_area_timer_arm(index);
        return;
    }
    if (Pl_master_ck((struct _PLW*)plw) == 1U) {
        lb_sub12_send(index);
    }
}

/* Tests area bit `index` of the stage block's 16-bit mask and reports whether it is clear. */
extern "C" s32 stage_torch_unlit_ck(u8 index)
{
    return (((StageWork*)&stage_w)->area_mask & (1 << index)) == 0;
}

/* Ages the stage block's per-area timers and clears the bits whose 4-second window has lapsed. */
extern "C" void stage_area_timers_update(StageWork* st)
{
    s32 now;
    u8 index;
    s32 span;

    now = quest_time_limit_get();
    span = now - quest_time_elapsed_get();
    if (Pl_motion_input_ck(NULL) == 1U) {
        return;
    }
    for (index = 0; index < 0x10U; index++) {
        if (stage_torch_unlit_ck(index) == 0) {
            if (span - (s32)st->area_timers[index] >= 0x2A30) {
                st->area_mask ^= 1 << index;
                st->area_timers[index] = 0;
            }
        }
    }
}

/* Reports whether every area from 1 to 4 is armed for the current map's stage kind 4. */
extern "C" u32 stage_gate_b_ck(void)
{
    StageWork* st = (StageWork*)&stage_w;
    u8 index;

    if (stage_map_kind_get(st->mapno) != 4) {
        return 0;
    }
    index = 1;
    while (index < 5U) {
        if (stage_torch_unlit_ck(index) == 1U) {
            return 0;
        }
        index++;
    }
    return 1;
}

/* Re-drives every live area actor's per-joint frame init for the palette the kind selects. */
extern "C" void stage_demo_start(u8 kind)
{
    f32 scale;
    f32 amount;
    u8 index;
    u8 joint;
    StageWork* st = (StageWork*)&stage_w;

    switch (kind) {
    case 18:
        amount = 3028.0f;
        break;
    case 33:
        stage_area_event_dispatch(NULL, 0);
        return;
    case 47:
        amount = 200.0f;
        break;
    default:
        return;
    }
    index = 0;
    scale = 0.5f;
    for (; index < 4U; index++) {
        if (st->chars[index] != NULL) {
            for (joint = 0; joint < st->chars[index]->anim_count; joint++) {
                st->chars[index]->chr.frame_init(joint, (u16)joint, amount * scale, 0, 1.0f);
            }
        }
    }
}

/* The joint lists the joint sound effects watch. */
extern "C" {

u8 stg_joint_se_list_0[2] = {0x0D, 0xFF};

u8 stg_joint_se_list_1[2] = {0x0C, 0xFF};

u8 stg_joint_se_list_2[2] = {0x08, 0xFF};

u8 stg_joint_se_list_3[2] = {0x0A, 0xFF};

u8 stg_joint_se_list_4[5] = {0x0A, 0x0B, 0x0C, 0x0D, 0xFF};

}

/* Arms the map's joint sound effects for the current area. */
extern "C" void stage_joint_se_update(StageWork* st)
{
    nw4r::math::VEC3 pos;
    u8* list;
    s32 sound;
    s32 target;
    u8* p;

    VEC3_ctor(&pos);
    list = NULL;
    switch (st->mapno) {
    case 21:
        if (getItemListSelection() == 0) {
            switch (st->areano) {
            case 0:
                list = stg_joint_se_list_0;
                sound = 1;
                target = 0;
                break;
            case 7:
                list = stg_joint_se_list_1;
                sound = 0;
                target = 0;
                break;
            }
        }
        break;
    case 8:
    case 11:
        if (st->areano == 0) {
            list = stg_joint_se_list_2;
            sound = 0;
            target = 0;
        }
        break;
    case 9:
        if (st->areano == 0) {
            list = stg_joint_se_list_3;
            sound = 0;
            target = 0;
        }
        break;
    case 10:
        if (st->areano == 1) {
            sound = 0;
            list = stg_joint_se_list_4;
            target = 1;
        }
        break;
    }
    if (list == NULL) {
        return;
    }
    for (p = list; (s8)*p > 0; p++) {
        st->chars[0]->chr.get_joint_wpos((s8)*p, &pos);
        switch (target) {
        case 0:
            shell_se_req(NULL, &pos, 0x10, sound);
            break;
        case 1:
            shell_se_req(NULL, &pos, 0x11, sound);
            break;
        }
    }
}

#pragma opt_common_subs off
/* The demo marker points of each map and their (kind, count, points) groups. */
extern "C" {

f32 stg_demo_points_0[18] = {
    -258.0f, 61.0f, -1955.0f,
    -818.0f, 29.0f, 428.0f,
    726.0f, 17.0f, 850.0f,
    -553.0f, 19.0f, 2575.0f,
    977.0f, 30.0f, 3602.0f,
    -2034.0f, -43.0f, 2972.0f,
};

f32 stg_demo_points_1[12] = {
    2865.0f, 24.0f, -416.0f,
    -280.0f, -6.0f, -101.0f,
    957.0f, 19.0f, 43.0f,
    -1634.0f, 21.0f, -39.0f,
};

f32 stg_demo_points_2[6] = {
    4075.0f, 2.0f, 1196.0f,
    -990.0f, 2.0f, 805.0f,
};

f32 stg_demo_points_3[12] = {
    -68.0f, 1.0f, -1963.0f,
    331.0f, 21.0f, -647.0f,
    1488.0f, 3.0f, 1102.0f,
    -166.0f, 21.0f, 1431.0f,
};

f32 stg_demo_points_4[16] = {
    4088.0f, 232.0f, -2526.0f,
    3025.0f, 234.0f, -3085.0f,
    -816.0f, 232.0f, -3941.0f,
    -2924.0f, 237.0f, -2488.0f,
    -5866.0f, 232.0f, -1020.0f,
    0.0f,
};

StageDemoEntry stg_demo_groups_0[5] = {
    {4, 6, {0, 0}, (u8*)stg_demo_points_0},
    {7, 4, {0, 0}, (u8*)stg_demo_points_1},
    {8, 2, {0, 0}, (u8*)stg_demo_points_2},
    {9, 4, {0, 0}, (u8*)stg_demo_points_3},
    {255, 0, {0, 0}, NULL},
};

StageDemoEntry stg_demo_groups_1[2] = {
    {1, 5, {0, 0}, (u8*)stg_demo_points_4},
    {255, 0, {0, 0}, NULL},
};

StageDemoEntry stg_demo_maps[4] = {
    {5, 0, {0, 0}, (u8*)stg_demo_groups_0},
    {16, 0, {0, 0}, (u8*)stg_demo_groups_0},
    {10, 0, {0, 0}, (u8*)stg_demo_groups_1},
    {255, 0, {0, 0}, NULL},
};

}

/* Places the map's demo marker objects on their scripted offsets. */
extern "C" void stage_demo_markers_place(StageWork* st)
{
    nw4r::math::VEC3 pos;
    StageDemoEntry* entry;
    StageDemoEntry* p;
    int mapno;
    u16 secs;
    s32 now;
    s32 base;

    VEC3_ctor(&pos);
    entry = NULL;
    mapno = get_now_mapno();
    if (mapno != 5 && mapno != 0x10 && mapno != 0xA) {
        return;
    }
    now = quest_time_limit_get();
    base = quest_time_elapsed_get();
    now = now - base;
    secs = (u16)(now / 300);
    secs += quest_clock_byte_get(0);
    if (--st->demo_countdown > 0) {
        return;
    }
    if ((now & 0x40) != 0x40) {
        if ((now & 0x30) == 0) {
            if (event_demo_ck() == 1U) {
                return;
            }
            for (p = stg_demo_maps; p->mapno != 0xFF; p++) {
                if (p->mapno == get_now_mapno()) {
                    entry = (StageDemoEntry*)p->offsets;
                    break;
                }
            }
            if (entry == NULL) {
                return;
            }
            for (; entry->mapno != 0xFF; entry++) {
                vec_to_mh_vec3(&pos, (struct Vec*)(entry->offsets + (secs % entry->count) * 0xC));
                shell_set_func_ptr->method_0x80(&pos, entry->mapno, 0x8C, shell_set_func_ptr);
            }
            st->demo_countdown = 0x12C;
        } else {
            st->demo_countdown = 0;
        }
    } else {
        st->demo_countdown = 0;
    }
}
#pragma opt_common_subs reset

/* Re-drives every live area object's own per-frame update. */
extern "C" void stage_area_objects_refresh(void)
{
    StageWork* st = (StageWork*)&stage_w;
    u32 index;

    if (GameMode_ck() == 1) {
        return;
    }
    index = 0;
    do {
        if (st->chars[(u8)index] != NULL) {
            stage_area_joint_visibility_set(st, &st->chars[(u8)index]->chr, index);
        }
        index++;
    } while (index < 4U);
}

/* The colour cycle weights and the three area blend tables. */
extern "C" {

f32 stg_colour_cycle_weights[4] = {0.875f, 0.0f, 0.33333334f, 0.5f};

StageBlendEntry stg_blend_table_a[8] = {
    {0x1, 0xFFFFFF8C, 0xF0F0F586, 0xD0D0E078},
    {0x6, 0x3CFF1E, 0x45E21B, 0x5AA014},
    {0x2, 0xFFF0BE14, 0xD0C6A311, 0x6464640A},
    {0x5, 0xFF64001E, 0xDD5B001E, 0x8C46001E},
    {0x3, 0x50AE6, 0x50AE0, 0x50AD2},
    {0x6, 0xA14AA, 0x3214003C, 0xF1EA0},
    {0x0, 0xA1478, 0x3214003C, 0xF1E82},
    {0x5, 0xA146E, 0x32140032, 0xF1E78},
};

StageBlendEntry stg_blend_table_b[8] = {
    {0x1, 0xFFFFFF8C, 0xF0F0F586, 0xD0D0E078},
    {0x6, 0x3CFF1E, 0x45E21B, 0x5AA014},
    {0x2, 0xFFF0BE14, 0xD0C6A311, 0x6464640A},
    {0x5, 0xFF64001E, 0xDD5B001E, 0x8C46001E},
    {0x3, 0x50AE6, 0x50AE0, 0x50AD2},
    {0x6, 0xA28, 0x51643, 0x1E3282},
    {0x0, 0xA28, 0x91643, 0x1E3282},
    {0x5, 0xA1E, 0x91639, 0x1E3278},
};

StageBlendEntry stg_blend_table_c[8] = {
    {0x1, 0xFFFFFF8C, 0xF0F0F586, 0xD0D0E078},
    {0x6, 0xFF8C5014, 0xC29A8511, 0x32B9FF0A},
    {0x2, 0xFF8C5014, 0xC29A8511, 0x32B9FF0A},
    {0x5, 0xFF8C5014, 0xC29A8511, 0x32B9FF0A},
    {0x3, 0x50AE6, 0x50AE0, 0x50AD2},
    {0x6, 0xFF8C5014, 0xC29A8511, 0x32B9FF0A},
    {0x0, 0xFF8C5014, 0xC29A8511, 0x32B9FF0A},
    {0x5, 0xFF8C5014, 0xC29A8511, 0x32B9FF0A},
};

}

/* Returns the blend table for the map's current area group. */
extern "C" StageBlendEntry* stage_blend_table_get(void)
{
    u8 areano;

    areano = ::get_now_areano();
    switch (areano) {
    default:
        return stg_blend_table_a;
    case 4:
        return stg_blend_table_c;
    case 5:
    case 6:
    case 7:
        return stg_blend_table_b;
    }
}

/* Blends the two areas the 30-second colour cycle straddles into a wrapped 0..1 value. */
extern "C" f32 stage_colour_cycle_weight_get(s16 index, f32 elapsed)
{
    StageBlendEntry* table;
    u32 kind;
    f32 value;

    table = stage_blend_table_get();
    kind = ((u32*)table)[index * 4];
    if (kind < 4U) {
        value = stg_colour_cycle_weights[kind];
    } else if (kind == 5) {
        if (index >= 7) {
            value = stg_colour_cycle_weights[table->kind] - elapsed;
        } else {
            value = stg_colour_cycle_weights[((u32*)table)[(index + 1) * 4]] - elapsed;
        }
    } else if (index == 0) {
        value = elapsed + stg_colour_cycle_weights[table[1].colour_c];
    } else {
        value = elapsed + stg_colour_cycle_weights[((u32*)table)[(index - 1) * 4]];
    }
    if (value < 0.0f) {
        value += 1.0f;
    } else if (value > 1.0f) {
        value -= 1.0f;
    }
    return value;
}

/* Interpolates the two areas' colour records by the cycle weight and arms the result. */
extern "C" void stage_colour_cycle_update(void)
{
    StageBlendEntry* table;
    StageBlendEntry* from;
    StageBlendEntry* to;
    u32 colour[6];
    u32 count;
    f32 peak;
    f32 span;
    s16 i;
    f32 step;
    s16 high;
    f32 next;
    f32 prev;
    f32 frac;
    f32 t;
    s16 low;

    count = lb_item_list_step_get();
    peak = lb_item_list_depth_get(count);
    table = stage_blend_table_get();
    step = 30.0f / (f32)count;
    low = 7;
    high = 8;
    for (i = 0; i < 8; i++) {
        next = stage_colour_cycle_weight_get(i, step);
        if (next > peak) {
            high = i;
            break;
        }
        low = i;
        prev = next;
    }
    if (high == 0) {
        prev = stage_colour_cycle_weight_get(low, step);
    }
    if (high >= 8) {
        high = 0;
        next = stage_colour_cycle_weight_get(0, step);
    }
    from = (StageBlendEntry*)((u32*)table + low * 4);
    to = (StageBlendEntry*)((u32*)table + high * 4);
    if (high <= 0) {
        span = (1.0f - prev) + next;
        frac = peak - prev;
        if (frac < 0.0f) {
            frac += 1.0f;
        }
    } else {
        span = next - prev;
        frac = peak - prev;
    }
    t = frac / span;
    colour[0] = stage_colour_lerp(&from->colour_a, &to->colour_a, t);
    colour[1] = colour[0];
    colour[2] = stage_colour_lerp(&from->colour_b, &to->colour_b, t);
    colour[3] = colour[2];
    colour[4] = stage_colour_lerp(&from->colour_c, &to->colour_c, t);
    colour[5] = colour[4];
    filter_panel_upper_set(0, 6, colour, 0.3f);
}


}  /* namespace view_fn_802B2AA0 */

/* ------------------------------------------------------------------------------------------------ */
/* the block and the constructed globals (`.bss`; MWCC's `__sinit` constructs them)                  */
/* ------------------------------------------------------------------------------------------------ */

extern "C" {

StageWork stage_w;

PlSlotGate stg_place_timers[3];

StageVec3 stg_entry_pos_none(0.0f, 0.0f, 0.0f);

StageVec3 stg_entry_pos_menu(52.0f, (-573.0f), 1465.0f);

StageVec3 stg_entry_pos_work(0.0f, 0.0f, 0.0f);

}
