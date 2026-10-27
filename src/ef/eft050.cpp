/* ef/eft050.cpp - the eft050 fish family and the lobby screen-13 status page and item box draws that precede it.
 * RANGE. .text 0x8033F270-0x80342B70 (35 functions, in address order); extab 0x80016CA4-0x80016D84,
 *   extabindex 0x8003615C-0x800362AC, .data 0x805E7410-0x805E7A80, .sdata 0x8079308C-0x807930F0, .sdata2
 *   0x8079B2C0-0x8079B310.  The right edge is the seam to `ef/eft051.cpp`: the `.sdata2` 1.0f repeats at 0x8079B310
 *   (`eft051_create`) and 0x8079B318 (`ef/eft050_set.cpp`), `.data` pads 4 bytes after the last jump table, and the
 *   extabindex record at 0x800362AC names `eft051_spawn`.  0x8033F270-0x8033FDA8 is lobby screen-13 code
 *   (`lb_screen13_draw` is the draw task `lobby/lb_companion_ui.cpp` queues).
 * FLAGS. `cflags_main`; `#pragma peephole off`, `optimization_level 4` and `fp_contract off` for the file (the target
 *   has no fused multiply-add anywhere), `peephole on` only around `eft050_catch_limit_get`..`eft050_act_table_ck`.
 * NAMES. GUESS (from each body, its callers and its data):
 *   `lb_s13_status_values_draw`, `lb_s13_status_skills_draw`, `lb_s13_status_dance_draw`, `lb_s13_status_page_draw`,
 *   `lb_item_box_cell_pos_get`, `lb_item_box_grid_draw`, `lb_item_box_page_draw`, `eft050_fish_spawn`/`_release`/
 *   `_dispatch`/`_create`/`_alive`/`_state_next`/`_retire`/`_kind_get`/`_kind_set`/`_school_spawn`, `eft050_xz_angle_get`,
 *   `eft050_angle_step`, `eft050_hold_item_ck`, `eft050_model_joint_show`, `eft050_catch_limit_get`,
 *   `eft050_bite_strength_get`, `eft050_float_find`, `eft050_fish_appear`/`_swim`/`_bite`/`_hooked`/`_respawn`/`_flee`,
 *   `eft050_kind_roll`, `eft050_act_table_ck`, `eft050_interior_fx_spawn`, the data tables
 *   `lb_s13_status_*_ids`, `lb_item_box_*_ids`, `lb_s13_status_icon_ids`, `eft050_fish_kinds`,
 *   `eft050_rolls_school_0`/`_12`.  `CockpitSharedDb` +0x51A4/+0x51A5 are the two dances
 *   picked on screen 13 (`lobby/lb_companion_ui.cpp` copies them to and from `LbScreen13Work::dance_0x016` and hands
 *   the same pair to `ai_npc_pair_values_get`).
 * RESIDUALS. 4 partial rows:
 *  - `lb_s13_status_skills_draw`: retail keeps the skill row address as base + offset (`add r25, r29, r30`, frame 0x30)
 *    where ours strength-reduces it to one pointer (frame 0x20); retail reads `lb_s13_status_skill_name_ids` `@sda21`
 *    (a 6-byte table; declaring it `[3]` gives the `@sda21` but scores lower, so the `[6]` spelling stays);
 *  - `eft050_fish_appear`: register assignment only (retail: the shared zero in r31, `work` r30, `self` r29);
 *  - `eft050_kind_roll`: retail reloads `tbl->weight` in both loop bodies (`lwz r0, 0(r3)`) where ours reuses the
 *    condition's load; `optimization_level` 2/3 and `while`/index forms do not reproduce it;
 *  - `lb_item_box_grid_draw`: retail narrows the sprite-id ternary with a `clrlwi r3, r3, 16`; u16/s32 locals and a cast
 *    do not reproduce it.
 *   Data: `.sdata2` (0x50) is byte-identical to the claim; `.data` (0x670: the sprite-id and layout tables, the fish
 *   kinds, the roll tables and two jump tables) and `.sdata` (0x64: the roll pointer pairs and small id tables) are
 *   claimed but not emitted (the tables are `extern`; the object emits only the two jump tables, 0x90).  flipcheck
 *   names `.text` 0x38F0 against 0x3900, `.data`, `.sdata`, `extab`/`extabindex` (the partial rows) and the `.text`
 *   relocations into the unemitted tables.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "pl.h"
#include "hud/cockpit_quest.h" /* hud/layout.h + pl.h: the 2D element library and its records */
#include "stage/stg_w.h"     /* get_now_areano (the owner header, rule 2) */
#include "ef/fn_800CDB2C.h" /* push_g3d_wk */
#include "Runtime.PPCEABI.H/memset.h" /* memset - owner Runtime.PPCEABI.H/memset.c */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "nw4r/math_types.h" /* mtx34_rotate_vec3 (rule 2) */
#include "fn_8004CAD8.h" /* the vector helpers (rule 2) */
#include "Pl/pl_hit_sphere.h" /* hit_point_sphr (rule 2) */
#include "Pl/fn_80273B14.h" /* fn_802754C0 */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2) */
#include "unsplit/ef_control.h" /* eft_control */
#include "enemy/enemy_control.h" /* fn_80143174 */
#include "sound/fn_800D7F54.h" /* se_req_orb (rule 2) */
#include "Pl/Pl_master_ck.h" /* Pl_master_ck, Pl_act_ck (rule 2) */
#include "Pl/pl_skill.h" /* Pl_Skill_ck (rule 2) */
#include "lobby/lb_screen13_work.h" /* LbScreen13Work (rule 1) */
#include "lobby/lb_s13_help_line_draw.h" /* the screen-13 draw helpers (rule 2) */
#include "lobby/lobby_w.h" /* lobby_w (rule 2) */
#include "hud/cockpit.h" /* menu_money_draw (rule 2) */
#include "hud/draw_lsp_sprite_variant.h" /* draw_lsp_sprite_variant (rule 2) */
#include "sound/set_zmode.h" /* set_zmode (rule 2) */
#include "lobby/lb_s13_status_icon_ids.h" /* lb_s13_status_icon_ids (rule 2) */
#include "camera/camera.h" /* get_camera_pos / get_camera_direction / get_current_view_mtx (rule 2) */
#include "camera/camera_angle_x_get.h" /* camera_angle_x_get / camera_angle_y_get / camera_underwater_ck (rule 2) */
#include "rotLocalMatX.h" /* rotLocalMatZ (rule 2) */
#include "sound/mhchar_mat_alpha_set.h" /* mhchar_mat_alpha_set (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */
#include "ef/eft001.h" /* mtx34_set_trans (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the records this unit reads
 * ------------------------------------------------------------------------------------------------- */

/* The `.sbss` pointer 0x80794880 (the lobby band's owner header spells it a byte array; this band
 * loads the 4-byte pointer the map records) and the block it points at.  Only the bytes this band
 * reads are named, so the size is an approximation. size: 0x522A (approximation) */
typedef struct CockpitSharedDb {
    /* +0x0000 */ u8 unused_0x0000[0x3E03];
    /* +0x3E03 */ u8 player_slot_0x3E03; /* & 0x7F selects which player's hold is drawn */
    /* +0x3E04 */ u8 unused_0x3E04[0x13A0];
    /* +0x51A4 */ u8 dance_a_0x51A4; /* the two dances picked on screen 13's dance page: `lobby/lb_companion_ui.cpp`
                                     * copies them to and from `LbScreen13Work::dance_0x016` */
    /* +0x51A5 */ u8 dance_b_0x51A5;
    /* +0x51A6 */ u8 unused_0x51A6[0x62];
    /* +0x5208 */ u16 hold_flag_0x5208;
    /* +0x520A */ u8 unused_0x520A[0x1E];
    /* +0x5228 */ u16 busy_flag_0x5228;
} CockpitSharedDb;

extern CockpitSharedDb* lobby_world_block;

/* One slot of the hold list `CockpitItemBlock::items_0x24` points at: the item id the icon is drawn
 * from and the word behind it. size: 0x4 */
typedef struct CockpitItemSlot {
    /* +0x00 */ u16 item_id_0x00;
    /* +0x02 */ u16 unused_0x02;
} CockpitItemSlot;

/* The `.bss` hold block at 0x806BF310, the record `lb_item_box_grid_draw` walks (the map makes it 0x58 = 88
 * bytes; only the fields this band reads are named). size: 0x28 (approximation) */
typedef struct CockpitItemBlock {
    /* +0x00 */ u8 state_0x00;      /* 1/2 draw the selection frame, `lb_item_box_grid_draw` tests `+0xFF` */
    /* +0x01 */ u8 unused_0x01[0x07];
    /* +0x08 */ u16 frame_id_0x08;  /* handed to `spr_anim_draw` as the frame's part */
    /* +0x0A */ u16 sel_id_0x0A;    /* the same, for the two cursor sprites */
    /* +0x0C */ u16 sel_flag_0x0C;  /* set while a selection is drawn */
    /* +0x0E */ u16 cursor_0x0E;    /* the slot the cursor sits on */
    /* +0x10 */ u16 cursor2_0x10;   /* the slot the second cursor marker sits on */
    /* +0x12 */ u8 unused_0x12[0x12];
    /* +0x24 */ const CockpitItemSlot* items_0x24; /* the 50-slot hold list */
} CockpitItemBlock;

/* The two-line panel `lb_s13_status_values_draw`/`lb_s13_status_skills_draw`/`lb_s13_status_dance_draw` draw; `lb_s13_status_page_draw` addresses the one
 * at +0x9C of its record.  The field names come from the sprite id each value is drawn at (the
 * `draw_font_idx_num(sprite, value, mode, pos)` calls). size: 0x1A */
typedef struct CockpitPanel {
    /* +0x00 */ u8 value_0x00;  /* drawn at sprite 0x201C */
    /* +0x01 */ u8 value_0x01;  /* 0x201D */
    /* +0x02 */ s16 count_0x02; /* + count_delta_0x04 -> 0x201E */
    /* +0x04 */ s16 count_delta_0x04;
    /* +0x06 */ s16 count_0x06; /* + count_delta_0x08 -> 0x201F */
    /* +0x08 */ s16 count_delta_0x08;
    /* +0x0A */ u8 icon_0x0A;   /* index into the icon table lb_s13_status_icon_ids, 0xFF = none */
    /* +0x0B */ u8 unused_0x0B[0x01];
    /* +0x0C */ s16 gauge_0x0C; /* + gauge_delta_0x0E -> 0x2020 (>= 100 moves the panel) */
    /* +0x0E */ s16 gauge_delta_0x0E;
    /* +0x10 */ s16 timer_0x10; /* + timer_delta_0x12 -> 0x2022 */
    /* +0x12 */ s16 timer_delta_0x12;
    /* +0x14 */ u8 mode_0x14;   /* 0x2044 */
    /* +0x15 */ u8 unused_0x15[0x01];
    /* +0x16 */ s16 value_0x16; /* + value_delta_0x18 -> 0x205F */
    /* +0x18 */ s16 value_delta_0x18;
    /* +0x1A */ u8 label_0x1A;  /* + 0x1C2 is the `LbStr` id drawn at 0x2060 */
} CockpitPanel;

/* The `eft050` fish's work block (the 100-byte block `eft050_fish_spawn` pools): its model, its home, scale, swim speed and
 * range, the float (`ef/eft035.cpp`'s tag-34 record) it is after, its heading and the bite state.  size: 0x64 */
typedef struct Eft050Work {
    /* +0x00 */ MHchar* models_0x00[1]; /* the pooled handle list */
    /* +0x04 */ u8 count_0x04;          /* its used length */
    /* +0x05 */ u8 unused_0x05[0x03];
    /* +0x08 */ VEC3 pos_0x08;          /* the home the fish swims around */
    /* +0x14 */ VEC3 scale_0x14;        /* the model's scale, grown and shrunk by the states */
    /* +0x20 */ f32 speed_0x20;         /* the swim step */
    /* +0x24 */ f32 scale_0x24;         /* the spawner's first scale argument: the swim range */
    /* +0x28 */ u8 unused_0x28[0x04];
    /* +0x2C */ f32 accel_0x2C;         /* the speed change of the dart and flee states */
    /* +0x30 */ f32 scale_0x30;         /* the spawner's second scale argument: the scare radius */
    /* +0x34 */ u8 unused_0x34[0x02];
    /* +0x36 */ s16 nibbles_0x36;       /* the nibbles left before the bite */
    /* +0x38 */ _EFT* float_0x38;       /* the float the fish has reached */
    /* +0x3C */ u32 param_0x3C;         /* the spawner's 5th argument: the kind table */
    /* +0x40 */ u32 yaw_goal_0x40;      /* the heading the fish turns to */
    /* +0x44 */ s32 turn_0x44;          /* the turn step */
    /* +0x48 */ s32 wiggle_0x48;        /* the swim wiggle amplitude */
    /* +0x4C */ s32 rot_x_0x4C;         /* the model's rotation */
    /* +0x50 */ s32 rot_y_0x50;
    /* +0x54 */ s32 rot_z_0x54;
    /* +0x58 */ s8 kind_0x58;           /* the fish kind (`eft050_fish_kind_get`'s table index) */
    /* +0x59 */ u8 hooked_0x59;         /* set while it is hooked */
    /* +0x5A */ u8 slot_0x5A;           /* its slot in the player's catch list, 0xFF = none */
    /* +0x5B */ u8 respawn_0x5B;        /* respawn soon */
    /* +0x5C */ u16 act_0x5C;           /* the fishing act it was spawned for */
    /* +0x5E */ s8 dart_0x5E;           /* frames until the hooked fish darts */
    /* +0x5F */ u8 biting_0x5F;         /* set while it bites */
    /* +0x60 */ u8 flag_0x60;           /* the kind came from the fishing act's own table */
} Eft050Work;

/* The item record `eft050_hold_item_ck` tests (a flag byte that disables the gauge and the item id).
 * size: 0x10 (approximation: only the bytes this band reads are named) */
typedef struct EftHoldItem {
    /* +0x00 */ u8 unused_0x00[0x0A];
    /* +0x0A */ u8 gauge_off_0x0A; /* nonzero: the id is not drawn as a gauge */
    /* +0x0B */ u8 unused_0x0B[0x01];
    /* +0x0C */ u16 item_id_0x0C;
} EftHoldItem;

/* One hold row of `lb_s13_status_skills_draw`: the three sprite ids a row draws, left to right. size: 0x6 */
typedef struct CockpitRow {
    /* +0x00 */ u16 icons_0x00[3];
} CockpitRow;

/* One fish kind of the table `eft050_fish_kind_get` addresses by the kind byte: the catch id, the bite strength and nibble
 * count, and the adult scale. size: 0x14 */
typedef struct Eft050Slot {
    /* +0x00 */ u16 id_0x00;            /* the catch id the player's catch list holds */
    /* +0x02 */ s8 bite_0x02;           /* the base tug strength */
    /* +0x03 */ s8 nibbles_0x03;        /* the most nibbles before the bite */
    /* +0x04 */ u8 unused_0x04[0x04];
    /* +0x08 */ VEC3 scale_0x08;        /* the adult scale */
} Eft050Slot;

/* One roll of a spot's kind table: the weight and the fish kind; a weight of 0xFF ends the table. size: 0x8 */
typedef struct EftFishRoll {
    /* +0x00 */ u32 weight;
    /* +0x04 */ u32 kind;
} EftFishRoll;

/* The fishing bytes of the player work `Pl/plw.h` keeps inside a padding run: the line is taut while the player reels,
 * and the catch count.  size: 0x5A2 - lower bound, an approximation (a view of `_PLW`) */
typedef struct _EFT050_PL_FISH {
    /* +0x000 */ u8 unused_0x000[0x596];
    /* +0x596 */ u8 reeling_0x596;
    /* +0x597 */ u8 catch_count_0x597;
    /* +0x598 */ u16 unused_0x598;
    /* +0x59A */ u16 tug_0x59A;
    /* +0x59C */ u16 catches_0x59C[3];
} _EFT050_PL_FISH;

/* The record `eft050_catch_limit_get` reads its draw mode from and reaches the player work through.
 * size: 0x34 (approximation: only the bytes this band reads are named) */
typedef struct HoldOwner {
    /* +0x00 */ u8 unused_0x00[0x08];
    /* +0x08 */ s8 mode_0x08;   /* 2 selects the skill-dependent draw state */
    /* +0x09 */ u8 unused_0x09[0x27];
    /* +0x30 */ _PLW* plw_0x30;
} HoldOwner;

/* ---------------------------------------------------------------------------------------------------
 * callees no header carries (rule 2 debt: their owner ranges are unregistered).  The `fn_` stems keep
 * C linkage so the call pairs by name.
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void draw_font_idx_num(u16 id, s32 value, u32 mode, const _mh_ivec2_* pos);
void addVec3To(VEC3* dst, const VEC3* src);
u32 calc_xz_angle(f32 a, f32 b, f32 c, f32 d);
const u8* ai_npc_behaviour_get(u8 index);
void ai_npc_pair_values_get(u8 a, u8 b, u16* out);
const u8* lb_entry_id_get(u8 index);
s8* get_item_name_str(u8 id);
s8* get_player_name_str(u8 index);
s8* get_digit_str(u8 id);
void lb_item_box_count_draw(void* work, u16 id, u32 arg);
void eft052_box_list_draw(u16 id, const void* tbl, const void* pos, u16 id2, u32 arg);
void lb_item_box_grid_draw(CockpitItemBlock* self);
/* this unit's own bodies, forward-declared (address order) */
void lb_s13_status_values_draw(CockpitPanel* self, const _mh_ivec2_* pos);
void lb_s13_status_skills_draw(CockpitPanel* self, const _mh_ivec2_* pos);
void lb_s13_status_dance_draw(CockpitPanel* self, const _mh_ivec2_* pos);
void lb_s13_status_page_draw(LbScreen13Work* self);
void lb_item_box_cell_pos_get(s32 value, _mh_ivec2_* out);
void lb_item_box_page_draw(void);
s32 lb_npc_mark_state_get(void);
void eft050_fish_release(_EFT* self);
void eft050_fish_spawn(u8 type, VEC3* pos, f32 scale, f32 scale2, u32 param, u8 area);
void eft050_fish_dispatch(_EFT* self);
void eft050_fish_create(_EFT* self);
void eft050_fish_alive(_EFT* self);
void eft050_fish_state_next(_EFT* self);
void eft050_fish_retire(_EFT* self);
Eft050Slot* eft050_fish_kind_get(s8 index);
u32 eft050_xz_angle_get(const VEC3* a, const VEC3* b);
s32 eft050_angle_step(s32* cur, u32* target, s32 step);
s32 eft050_hold_item_ck(EftHoldItem* self);
void eft050_model_joint_show(MHchar* model, u32 index);
s32 eft050_catch_limit_get(HoldOwner* state, HoldOwner* owner);
void eft050_fish_school_spawn(u32 kind, VEC3* pos, f32 scale, u8 flag);
s32 eft050_bite_strength_get(_PLW* plw, s8 kind, _EFT* fish);
_EFT* eft050_float_find(_EFT* self, s32 mode);
void eft050_fish_appear(_EFT* self);
void eft050_fish_swim(_EFT* self);
void eft050_fish_bite(_EFT* self);
void eft050_fish_hooked(_EFT* self);
void eft050_fish_respawn(_EFT* self);
void eft050_fish_flee(_EFT* self);
u32 eft050_kind_roll(_EFT* self);
u32 eft050_act_table_ck(_EFT* self);
void eft050_fish_state_next(_EFT* self);
void eft050_fish_retire(_EFT* self);
void eft050_fish_kind_set(_EFT* self, MHchar* model);

/* This unit's own `.data` run (0x805E7400..0x805E7A70), its `.sdata` tables and its `.sdata2` pool -
 * referenced by name, never defined (playbook 29). */
extern const u16 lb_s13_status_values_sprite_ids[];
extern const u16 lb_s13_status_skills_sprite_ids[];
extern CockpitRow lb_s13_status_skill_rows[];
extern const u16 lb_s13_status_dance_sprite_ids[];
extern const u16 lb_s13_status_page_sprite_ids[];
extern const u16 lb_item_box_col_layout_ids[];
extern const u16 lb_item_box_row_layout_ids[];
extern const u16 lb_item_box_grid_sprite_ids[];
extern const u16 lb_item_box_list_layout_ids[];
extern const u16 lb_item_box_page_sprite_ids[];
extern Eft050Slot eft050_fish_kinds[];
extern const u16 eft050_rolls_school_0[];
extern const u16 eft050_rolls_school_12[];
extern const u16 lb_s13_status_skill_name_ids[6];

/* The fish tables of this unit's `.data`/`.sdata` (declared until the unit emits its data). */
extern f32 eft050_bite_speed[];
extern u16 eft050_tug_offsets[4];
extern s8 eft050_respawn_waits[4];
extern EftFishRoll eft050_rolls_act389[];
extern EftFishRoll eft050_rolls_act351[];
extern EftFishRoll eft050_rolls_act351_near[];
extern EftFishRoll eft050_rolls_act353[];
extern EftFishRoll eft050_rolls_act353_m4[];
extern EftFishRoll eft050_rolls_act353_m5[];
extern EftFishRoll eft050_rolls_act390[];
extern EftFishRoll eft050_rolls_act355[];
extern EftFishRoll* eft050_rolls_m1_0[2];
extern EftFishRoll* eft050_rolls_m1_12[2];
extern EftFishRoll* eft050_rolls_m2_9[2];
extern EftFishRoll* eft050_rolls_m3_0[2];
extern EftFishRoll* eft050_rolls_m3_3[2];
extern EftFishRoll* eft050_rolls_m4_0[2];
extern EftFishRoll* eft050_rolls_m5_6[2];

extern const u16 lb_s13_status_dance_value_ids[4];
extern const u16 lb_item_box_list_ids[4];

/* The hold block at 0x806BF310 (`lb_item_box_page_draw` hands its address to `lb_item_box_grid_draw`). */
extern CockpitItemBlock eft052_item_box;

#ifdef __cplusplus
}
#endif

/* C++-scope callees (their map names are manglings, so the front-end has to reproduce them). */
s8* LbStr(u8 kind, u16 id);
void draw_font_idx(u16 id, s8* str, u32 mode, const _mh_ivec2_* pos);
u16 ran_suu(long index);
s32 chk_pointer(void);
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag);

#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off

/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)
/* `em_vec3_copy` returns its argument's copy by value. */
#define EFT_VEC3_COPY ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*))em_vec3_copy)
/* The bite body reads `eft050_catch_limit_get`'s result as the byte it is (the caller narrows it, not the callee). */
#define EFT050_CATCH_LIMIT ((u8 (*)(HoldOwner*, HoldOwner*))eft050_catch_limit_get)

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* Draws one hold slot's panel: the frame, the two counters with their deltas, the gauge bar and the
 * four value/timer pairs, each at its own sprite id. */
extern "C" void lb_s13_status_values_draw(CockpitPanel* self, const _mh_ivec2_* pos)
{
    _mh_ivec2_ anchor = *pos;
    draw_sprite_ary(lb_s13_status_values_sprite_ids, pos);
    _SPR_DATA_ frame;
    spr_data_copy(&frame, get_lsp_data(0x2019, 0));
    frame.pos.x += 6;
    draw_sprite(frame, pos);
    if (self->icon_0x0A != 0) {
        u8 icon = lb_s13_status_icon_ids[self->icon_0x0A];
        if (icon != 0xFF) {
            draw_tex_icon_square((s16)(pos->x + 0x3A), (s16)(pos->y + 0x6E), 0x12, 0xFFFFFFFF, icon);
        }
        if (self->gauge_0x0C + self->gauge_delta_0x0E >= 0x64) {
            anchor.x = (s16)(anchor.x + 6);
        }
        draw_font_idx_num(0x2020, self->gauge_0x0C + self->gauge_delta_0x0E, 2, &anchor);
    } else {
        draw_font_idx(0x2021, LbStr(0, 0x1B8), 2, pos);
    }
    draw_font_idx_num(0x201C, self->value_0x00, 2, pos);
    draw_font_idx_num(0x201D, self->value_0x01, 2, pos);
    draw_font_idx_num(0x201E, self->count_0x02 + self->count_delta_0x04, 2, pos);
    draw_font_idx_num(0x201F, self->count_0x06 + self->count_delta_0x08, 2, pos);
    draw_font_idx_num(0x2022, self->timer_0x10 + self->timer_delta_0x12, 2, pos);
}

/* Draws the hold slot list the selected player carries: one row of three sprites and the entry's name
 * per item, or the "no items" label when the list is empty. */
extern "C" void lb_s13_status_skills_draw(CockpitPanel* self, const _mh_ivec2_* pos)
{
    draw_sprite_ary(lb_s13_status_skills_sprite_ids, pos);
    u8 sel = lobby_world_block->player_slot_0x3E03 & 0x7F;
    const u8* rows = ai_npc_behaviour_get(sel);
    const u8* slots = lb_entry_id_get(sel);
    draw_font_idx(0x2043, get_player_name_str(sel), 1, pos);
    draw_font_idx_num(0x2044, self->mode_0x14, 1, pos);
    if (rows[slots[0] + 4] == 0) {
        draw_font_idx(0x2046, LbStr(0, 0x1C1), 1, pos);
    } else {
        for (s8 i = 0; i < rows[slots[0] + 4]; i++) {
            draw_sprite_idx(lb_s13_status_skill_rows[i].icons_0x00[0], pos);
            draw_sprite_idx(lb_s13_status_skill_rows[i].icons_0x00[1], pos);
            draw_sprite_idx(lb_s13_status_skill_rows[i].icons_0x00[2], pos);
            draw_font_idx(lb_s13_status_skill_name_ids[i], get_item_name_str(slots[i + 1]), 1, pos);
        }
    }
}

/* Draws the status page's dance row: its value, its label and the four values of the dance pair the shared block
 * carries (`ai_npc_pair_values_get`). */
extern "C" void lb_s13_status_dance_draw(CockpitPanel* self, const _mh_ivec2_* pos)
{
    draw_sprite_ary(lb_s13_status_dance_sprite_ids, pos);
    draw_font_idx_num(0x205F, self->value_0x16 + self->value_delta_0x18, 1, pos);
    draw_font_idx(0x2060, LbStr(0, (u16)(self->label_0x1A + 0x1C2)), 1, pos);
    u16 values[4];
    ai_npc_pair_values_get(lobby_world_block->dance_a_0x51A4, lobby_world_block->dance_b_0x51A5, values);
    for (u16 i = 0; i < 4; i++) {
        draw_font_idx(lb_s13_status_dance_value_ids[i], get_digit_str((u8)values[i]), 0, pos);
    }
}

/* Draws the three panels of one hold record: the frame sprite, then each panel at the anchor the
 * preceding `get_lsp_data` wrote. */
extern "C" void lb_s13_status_page_draw(LbScreen13Work* self)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x1FE1, &anchor);
    draw_sprite_ary(lb_s13_status_page_sprite_ids, &anchor);
    get_lsp_data(0x200D, &anchor);
    lb_s13_status_values_draw((CockpitPanel*)self->status_0x09C, &anchor);
    get_lsp_data(0x200E, &anchor);
    lb_s13_status_skills_draw((CockpitPanel*)self->status_0x09C, &anchor);
    get_lsp_data(0x200F, &anchor);
    lb_s13_status_dance_draw((CockpitPanel*)self->status_0x09C, &anchor);
}

/* Places the two digit sprites of a two-digit value: the ones digit's for `value % 10` and the tens
 * digit's for `value / 10`, then writes the ones digit's anchor, moved right by 4, into `out`. */
extern "C" void lb_item_box_cell_pos_get(s32 value, _mh_ivec2_* out)
{
    const _SPR_DATA_* ones = get_lsp_data(lb_item_box_col_layout_ids[value % 10], 0);
    const _SPR_DATA_* tens = get_lsp_data(lb_item_box_row_layout_ids[value / 10], 0);
    out->x = (s16)(ones->pos.x + 4);
    out->y = tens->pos.y;
}

/* Draws the hold list of the block at 0x806BF310: its frame, its 50 item icons with the selected
 * slot's frame around them, and - while a selection is active - the two cursor sprites. */
extern "C" void lb_item_box_grid_draw(CockpitItemBlock* self)
{
    _SPR_DATA_ rec;
    _SPR_DATA_ frame;
    _SPR_DATA_ sel;
    _mh_ivec2_ digit;
    _mh_ivec2_ frame_pos;
    _mh_ivec2_ cursor;
    _mh_ivec2_ anchor;
    s32 i;
    s32 found;
    u16 id;

    get_lsp_data(0x2081, &anchor);
    draw_sprite_anim_ary(lb_item_box_grid_sprite_ids, 0, &anchor);
    found = 0;
    spr_data_copy(&frame, get_lsp_data(0x1307, 0));
    uv_pair_copy(&frame_pos, &frame);
    for (i = 0; i < 0x32; i++) {
        lb_item_box_cell_pos_get(i, &digit);
        id = (i < 0x14) ? 0x2092 : 0x209B;
        spr_data_copy(&rec, get_lsp_data(id, 0));
        uv_pair_copy(&rec, &digit);
        frame.pos.x = (s16)(digit.x + frame_pos.x);
        frame.pos.y = (s16)(digit.y + frame_pos.y);
        draw_sprite(rec, &anchor);
        if (self->cursor_0x0E == i) {
            found = 1;
            uv_pair_copy(&cursor, &digit);
        }
        if ((u8)(self->state_0x00 + 0xFF) <= 1) {
            if (chk_pointer() == 0) {
                if (self->cursor2_0x10 == i) {
                    spr_data_copy(&sel, get_lsp_data(0x1304, 0));
                    sel.pos.x += digit.x;
                    sel.pos.y += digit.y;
                    set_blendmode(4, 1, 1);
                    spr_anim_draw(&sel, self->frame_id_0x08, &anchor);
                    set_blendmode(4, 5, 1);
                }
            }
        }
        if (self->items_0x24[i].item_id_0x00 != 0) {
            draw_itemicon_item_id(frame, self->items_0x24[i].item_id_0x00, &anchor);
        }
    }
    if (self->sel_flag_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        spr_data_copy(&sel, get_lsp_data(0x1305, 0));
        sel.pos.x += cursor.x;
        sel.pos.y += cursor.y;
        spr_anim_draw(&sel, self->sel_id_0x0A, &anchor);
        set_blendmode(4, 5, 1);
        spr_data_copy(&sel, get_lsp_data(0x1306, 0));
        sel.pos.x += cursor.x;
        sel.pos.y += cursor.y;
        spr_anim_draw(&sel, self->sel_id_0x0A, &anchor);
    }
}

/* Draws the hold's own frame and the item-icon panel the shared block's selector points at. */
extern "C" void lb_item_box_page_draw(void)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x2066, &anchor);
    draw_sprite_ary(lb_item_box_page_sprite_ids, &anchor);
    lb_item_box_grid_draw(&eft052_item_box);
    lb_item_box_count_draw(&eft052_item_box, 0x2090, 0);
    eft052_box_list_draw(0x209F, lb_item_box_list_layout_ids, lb_item_box_list_ids, 0x2065, 0);
}

/* Draws screen 13 every frame: the main choice with its help (states 0/1), or the page of the state - the entry, dance,
 * skill-slot or status page, or the item box - and the help line under it. */
extern "C" void lb_screen13_draw(void)
{
    LbScreen13Work* work = lobby_w.screen13_0x0AC;

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);
    switch (work->state_0x000) {
    case 0:
    case 1:
        if (work->help_0x003 == 0) {
            lb_subpage_arrow_draw();
            lb_s13_menu_draw(work);
            lb_s13_help_draw();
            if (get_option_cfg(7) == 0) {
                draw_lsp_sprite_variant(0x1FE0, 2);
            } else {
                draw_lsp_sprite_variant(0x1FE0, 16);
            }
        } else {
            if (get_option_cfg(7) == 0) {
                draw_lsp_sprite_variant(0x1FE0, 3);
            } else {
                draw_lsp_sprite_variant(0x1FE0, 17);
            }
            return;
        }
        break;
    case 7:
        menu_money_draw(0x14FC);
        lb_item_box_page_draw();
        break;
    case 3:
        lb_subpage_arrow_draw();
        switch (work->sub_0x001) {
        case 0:
            lb_s13_entry_list_draw(work, true);
            lb_s13_entry_status_draw(work->ids_0x008[work->list_0x070.cursor_0x00]);
            break;
        case 1:
            lb_s13_entry_list_draw(work, false);
            lb_s13_entry_status_draw(work->ids_0x008[work->list_0x070.cursor_0x00]);
            break;
        }
        break;
    case 4:
        lb_subpage_arrow_draw();
        switch (work->sub_0x001) {
        case 0:
            lb_s13_dance_list_draw(work, false);
            lb_s13_dance_pick_draw(work);
            lb_s13_dance_values_draw(work);
            break;
        case 1:
            lb_s13_dance_list_draw(work, true);
            lb_s13_dance_pick_draw(work);
            break;
        case 2:
        case 3:
            lb_s13_dance_list_draw(work, false);
            lb_s13_dance_pick_draw(work);
            lb_s13_dance_values_draw(work);
            break;
        }
        break;
    case 5:
        lb_subpage_arrow_draw();
        switch (work->sub_0x001) {
        case 0:
            lb_s13_entry_list_draw(work, true);
            lb_s13_entry_status_draw(work->ids_0x008[work->list_0x070.cursor_0x00]);
            break;
        case 1:
            lb_s13_slot_list_draw(work, 1);
            break;
        case 2:
            lb_s13_slot_list_draw(work, 0);
            lb_s13_slot_action_draw(work);
            break;
        case 4:
        case 5:
            lb_s13_slot_list_draw(work, 0);
            break;
        case 3:
            switch (work->picking_0x002) {
            case 0:
                lb_s13_skill_list_draw(work, true);
                break;
            case 1:
                lb_s13_skill_list_draw(work, false);
                break;
            }
            break;
        }
        break;
    case 6:
        lb_s13_status_page_draw(work);
        lb_subpage_arrow_draw();
        break;
    }
    lb_s13_help_line_draw(work);
}

/* Reports the hold's state: 0 while the busy flag is set, -1 while the hold flag is clear, 1
 * otherwise. */
extern "C" s32 lb_npc_mark_state_get(void)
{
    CockpitSharedDb* db = lobby_world_block;
    if (db->busy_flag_0x5228 != 0) {
        return 0;
    }
    if (db->hold_flag_0x5208 != 0) {
        return 1;
    }
    return -1;
}

/* Spawns the family's effect in its area: pools the record and work block, seeds the model handle, stamps the
 * type, area and hooks, and installs the state machine through `eft_state_flags_set`. */
extern "C" void eft050_fish_spawn(u8 type, VEC3* pos, f32 scale, f32 scale2, u32 param, u8 area)
{
    s32 i;
    _EFT* eft;
    Eft050Work* work;

    if ((u8)get_now_areano() != (u8)area) {
        return;
    }
    eft = (_EFT*)eft_res_slot_get(0x64);
    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = type;
    eft->release_0x40 = eft050_fish_release;
    eft->dispatch_0x34 = eft050_fish_dispatch;
    work = (Eft050Work*)eft->work_0x38;
    work->count_0x04 = 1;
    for (i = 0; i < work->count_0x04; i++) {
        work->models_0x00[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x00[i] == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    work->scale_0x24 = scale;
    work->param_0x3C = param;
    work->flag_0x60 = 0;
    work->scale_0x30 = scale2;
    copyVec3(&eft->pos_0x18, copyVec3(&work->pos_0x08, pos));
    eft->rot_0x24.x = 0;
    eft->rot_0x24.y = 0;
    eft->rot_0x24.z = 0;
    eft->field_0x03 = 0x30;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft_state_flags_set(eft, 0, 0);
}

/* Releases the pooled handle list of a work block and clears its used length. */
extern "C" void eft050_fish_release(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    eft_res_models_release(work->models_0x00, work->count_0x04);
    work->count_0x04 = 0;
}

/* The state machine of the effect `eft050_fish_spawn` spawns: 0 the model-create pass, 1 the alive body, 2
 * the state advance and 3 the pool release. */
extern "C" void eft050_fish_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return eft050_fish_create(self);
    case 1: return eft050_fish_alive(self);
    case 2: return eft050_fish_state_next(self);
    case 3: return eft050_fish_retire(self);
    }
}

/* State 0: creates the family's model for every pooled handle and hands each to the alive-state body,
 * or retires the effect when the create fails, then advances to state 1. */
extern "C" void eft050_fish_create(_EFT* self)
{
    s32 i;
    Eft050Work* work = (Eft050Work*)self->work_0x38;

    self->state_0x05++;
    self->field_0x06 = 0;
    for (i = 0; i < work->count_0x04; i++) {
        if (res_eft_model_create(work->models_0x00[i], 0x44, 0xC) == NULL) {
            eft050_fish_retire(self);
            return;
        }
        eft050_fish_kind_set(self, work->models_0x00[i]);
    }
    eft050_fish_alive(self);
}

/* Runs the fish's state, keeps the swimming states inside the swim range, and pushes the rotation (with its wiggle),
 * position and scale onto the model. */
extern "C" void eft050_fish_alive(_EFT* self)
{
    Eft050Work* work;
    nw4r::math::VEC3 ofs;

    VEC3_ctor(&ofs);
    work = (Eft050Work*)self->work_0x38;
    switch (self->field_0x06) {
    case 0:
        eft050_fish_appear(self);
        break;
    case 1:
        eft050_fish_swim(self);
        break;
    case 2:
        eft050_fish_bite(self);
        break;
    case 3:
        eft050_fish_hooked(self);
        break;
    case 4:
        eft050_fish_respawn(self);
        break;
    case 5:
        eft050_fish_flee(self);
        break;
    }
    if (self->field_0x06 < 3) {
        copyVec3(&ofs, &VEC3_SUB(&self->pos_0x18, &work->pos_0x08));
        ofs.y = 0.0f;
        if (vec3_len((f32*)&ofs) > work->scale_0x24) {
            vec3_normalize_into(&ofs, &ofs);
            copyVec3(&self->pos_0x18, &VEC3_ADD(&work->pos_0x08, &VEC3_SCALE(&ofs, work->scale_0x24)));
        }
    }
    self->rot_0x24.x = work->rot_x_0x4C;
    self->rot_0x24.y = work->rot_y_0x50 + work->wiggle_0x48;
    self->rot_0x24.z = work->rot_z_0x54;
    copyVec3(&work->models_0x00[0]->pos_0x04, &self->pos_0x18);
    eft_rot_vec_copy(&work->models_0x00[0]->rot_0x28, &self->rot_0x24);
    copyVec3(&work->models_0x00[0]->scale_0x1C, &work->scale_0x14);
    work->models_0x00[0]->move(0);
    eft_res_models_spawn(self, (void**)work->models_0x00, 2, work->count_0x04, 0);
}

/* State 2 of the family's state machine: advances the state counter to the next body. */
extern "C" void eft050_fish_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* State 3 of the family's state machine: releases the pooled effect record. */
extern "C" void eft050_fish_retire(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Addresses one 0x14-byte slot of the table by its signed index. */
extern "C" Eft050Slot* eft050_fish_kind_get(s8 index)
{
    return &eft050_fish_kinds[index];
}

/* The squared distance of the two points' x/z pairs, narrowed to the u16 the caller wants. */
extern "C" u32 eft050_xz_angle_get(const VEC3* a, const VEC3* b)
{
    return (u16)calc_xz_angle(a->x, a->z, b->x, b->z);
}

/* Moves `cur` towards `target` by at most `step`, returning the step actually applied. */
extern "C" s32 eft050_angle_step(s32* cur, u32* target, s32 step)
{
    *target = (u16)*target;
    s32 delta = (u16)(*target - *cur);
    s32 mag = (delta >= 0x8000) ? (u16)-delta : delta;
    if (mag < step) {
        step = mag;
    }
    if (delta >= 0x8000) {
        *cur -= step;
    } else {
        *cur += step;
    }
    return step;
}

/* How strongly the fish takes the player's float: 0 not at all, 1 with the plain rods (acts 202-205), 2 or 3 for the
 * fishing acts by the fish kind; a master player whose catch slot already holds another kind gets 0. */
extern "C" s32 eft050_bite_strength_get(_PLW* plw, s8 kind, _EFT* fish)
{
    Eft050Slot* slot = eft050_fish_kind_get(((Eft050Work*)fish->work_0x38)->kind_0x58);

    if (Pl_master_ck(plw) == 0) {
        return 0;
    }
    if (plw->armed_motion_0x59C != 0 && plw->armed_motion_0x59C != slot->id_0x00) {
        return 0;
    }
    switch (plw->field_0x306) {
    case 206:
        return 0;
    case 202:
    case 203:
    case 204:
        return 1;
    case 205:
        return 1;
    case 389:
        return (kind == 1) ? 2 : 3;
    case 351:
        if (kind == 4 || kind == 10) {
            return 2;
        }
        return 3;
    case 353:
        return ((u32)(kind - 5) <= 1) ? 2 : 3;
    case 390:
        return (kind == 2) ? 2 : 3;
    case 355:
        return (kind == 7) ? 2 : 3;
    }
    return 0;
}

/* Finds the live float the fish reacts to: with `mode` 0 a float that has settled (state 2) and whose catch list has
 * room, turning the fish towards it; otherwise any float of a master player that takes the fish. */
extern "C" _EFT* eft050_float_find(_EFT* self, s32 mode)
{
    s32 i;
    _EFT* rec;
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    s32 level;
    _PLW* plw;

    rec = (_EFT*)eft_control.slots_0x10;
    for (i = 0; i < (s32)eft_control.slot_count_0x0C; i++, rec++) {
        if (rec->field_0x03 != 34 || rec->type_0x02 == 1) {
            continue;
        }
        if (rec->used_0x00 == 0 || rec->state_0x05 >= 2) {
            return NULL;
        }
        self->demo_flag_0x08 = eft050_bite_strength_get((_PLW*)rec->source_0x30, work->kind_0x58, self);
        level = eft050_catch_limit_get((HoldOwner*)self, (HoldOwner*)rec);
        plw = (_PLW*)rec->source_0x30;
        if (mode == 0) {
            if (rec->field_0x06 != 2) {
                return NULL;
            }
            if (rec->demo_flag_0x08 != 0 && self->demo_flag_0x08 != 2) {
                continue;
            }
            if (((_EFT050_PL_FISH*)plw)->catch_count_0x597 >= (u8)level) {
                return NULL;
            }
            if (Pl_master_ck(plw) == 1 && self->demo_flag_0x08 != 0) {
                work->yaw_goal_0x40 = eft050_xz_angle_get(&self->pos_0x18, &rec->pos_0x18);
                return rec;
            }
        } else if (Pl_master_ck(plw) == 1 && self->demo_flag_0x08 != 0) {
            return rec;
        }
    }
    return NULL;
}

/* Whether the record's item id is one of the five ids the hold bar draws as a gauge. */
extern "C" s32 eft050_hold_item_ck(EftHoldItem* self)
{
    if (self->gauge_off_0x0A == 0) {
        if ((u16)(self->item_id_0x0C + 0xFFC1) <= 4) {
            return 1;
        }
    }
    return 0;
}

/* The fish appears (sub-state 0: a random spot and heading inside 70-100 % of its range, small) and grows to its kind's
 * scale over ten frames before it starts to swim. */
extern "C" void eft050_fish_appear(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    Eft050Slot* slot = eft050_fish_kind_get(work->kind_0x58);
    nw4r::math::VEC3 ofs;
    nw4r::math::MTX34 mtx;
    f32 inner;
    f32 dist;
    u32 yaw;

    VEC3_ctor(&ofs);
    MTX34_ctor(&mtx);
    switch (self->field_0x07) {
    case 0:
        self->field_0x07++;
        work->hooked_0x59 = 0;
        if (work->kind_0x58 == 0) {
            self->state_0x05++;
            self->field_0x06 = 0;
            self->flag_0x01 = 0;
            return;
        }
        inner = 0.7f * work->scale_0x24;
        dist = inner + ((ran_suu(1) & 0x3F) / 100.0f) * (work->scale_0x24 - inner);
        ofs.x = 0.0f;
        ofs.y = 0.0f;
        ofs.z = dist;
        yaw = ran_suu(1);
        rotVecY(&ofs, yaw);
        copyVec3(&self->pos_0x18, &work->pos_0x08);
        addVec3To(&self->pos_0x18, &ofs);
        self->timer_0x0C = 10;
        work->rot_x_0x4C = 0;
        work->rot_z_0x54 = 0;
        work->yaw_goal_0x40 = work->rot_y_0x50 = yaw + (ran_suu(0) & 0x1FFF) + 0x7000;
        work->turn_0x44 = 0x20;
        work->wiggle_0x48 = 0;
        work->respawn_0x5B = 0;
        if (work->flag_0x60 == 0) {
            work->act_0x5C = 0xFFFF;
        }
        work->slot_0x5A = 0xFF;
        setVector3(&work->scale_0x14, 0.1f, 0.1f, 0.1f);
        break;
    case 1:
        work->scale_0x14.x += slot->scale_0x08.x / 10.0f;
        work->scale_0x14.y += slot->scale_0x08.y / 10.0f;
        work->scale_0x14.z += slot->scale_0x08.z / 10.0f;
        if (--self->timer_0x0C <= 0.0f) {
            self->field_0x06 = 1;
            self->field_0x07 = 0;
            self->timer_0x0C = (ran_suu(0) & 0x1F) + 10;
            copyVec3(&work->scale_0x14, &EFT_VEC3_COPY(&slot->scale_0x08));
        }
        break;
    }
}

/* The fish swims: it cruises (sub-state 0) towards a float that takes it or a random heading, darts forward (1) with a
 * decaying wiggle, starts to bite (state 2) when its float's player is reeling, and flees (state 5) from a player who
 * walks into its scare radius or changes the fishing act. */
extern "C" void eft050_fish_swim(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    nw4r::math::VEC3 step;
    nw4r::math::VEC3 move;
    nw4r::math::VEC3 unused;
    nw4r::math::MTX34 mtx;
    _CP_VECTOR rot;
    s32 bite;
    _EFT* flt;
    _EFT* rec;
    _PLW* plw;
    s32 frames;
    u32 yaw;

    VEC3_ctor(&step);
    VEC3_ctor(&move);
    VEC3_ctor(&unused);
    MTX34_ctor(&mtx);
    bite = 0;
    flt = eft050_float_find(self, 0);
    switch (self->field_0x07) {
    case 0:
        if (--self->timer_0x0C <= 0 || (s32)(work->rot_y_0x50 - work->yaw_goal_0x40) == 0) {
            self->field_0x07++;
            self->demo_flag_0x08 = 0;
            work->wiggle_0x48 = 0x100;
            if (flt == NULL) {
                yaw = eft050_xz_angle_get(&self->pos_0x18, &work->pos_0x08);
                work->yaw_goal_0x40 = yaw + ((ran_suu(1) & 0xF) << 11) - 0x4000;
            }
            work->speed_0x20 = 3.0f * eft050_bite_speed[self->demo_flag_0x08];
        }
        work->turn_0x44 += (0x20 - work->turn_0x44) / 8;
        eft050_angle_step((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, work->turn_0x44);
        work->biting_0x5F = 0;
        break;
    case 1:
        step.x = 0.0f;
        step.y = 0.0f;
        step.z = work->speed_0x20;
        rot.x = work->rot_x_0x4C;
        rot.y = work->rot_y_0x50;
        rot.z = work->rot_z_0x54;
        cpSetRotMatrix(&rot, &mtx);
        mtx34_rotate_vec3(&move, &mtx, &step);
        addVec3To(&self->pos_0x18, &move);
        work->wiggle_0x48 -= work->wiggle_0x48 * 2;
        work->wiggle_0x48 -= work->wiggle_0x48 / 2;
        if (flt != NULL) {
            if (calcVecDistXZ(&self->pos_0x18, &flt->pos_0x18) < 21.0f) {
                flt->demo_flag_0x08 |= 1;
                work->float_0x38 = flt;
            } else {
                work->float_0x38 = NULL;
            }
        } else {
            work->float_0x38 = NULL;
        }
        if (work->float_0x38 != NULL) {
            plw = (_PLW*)work->float_0x38->source_0x30;
            if (plw != NULL && ((_EFT050_PL_FISH*)plw)->reeling_0x596 != 0) {
                self->field_0x06 = 2;
                self->field_0x07 = 0;
                self->timer_0x0C = 3;
                work->wiggle_0x48 = 0;
                bite = 1;
            }
        }
        if ((work->speed_0x20 += -0.1f) <= 0.0f && bite == 0) {
            self->field_0x07 = 0;
            frames = ran_suu(1) % 20 + 10;
            self->timer_0x0C = frames;
            self->timer_0x0C = frames * (1.0f - (eft050_bite_speed[self->demo_flag_0x08] - 1.0f));
            work->wiggle_0x48 = 0;
        }
        work->turn_0x44 += (0x100 - work->turn_0x44) / 8;
        eft050_angle_step((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, work->turn_0x44);
        break;
    }
    plw = (_PLW*)get_move_work_adrs(2);
    plw += (s8)my_player_no();
    if (eft050_hold_item_ck((EftHoldItem*)plw) == 0 && (plw->kind_0x09 == 3 || (plw->field_0x5A4 & 2) != 0) &&
        hit_point_sphr(&plw->vec_0x03C, &work->pos_0x08, work->scale_0x30) == 1) {
        self->field_0x06 = 5;
        self->field_0x07 = 0;
        self->demo_flag_0x08 = 0;
        work->respawn_0x5B = 0;
        work->act_0x5C = 0xFFFF;
    }
    if (work->flag_0x60 == 1 && flt != NULL) {
        plw = (_PLW*)flt->source_0x30;
        if (work->act_0x5C != plw->field_0x306) {
            self->field_0x06 = 5;
            self->field_0x07 = 0;
            self->demo_flag_0x08 = 0;
            work->respawn_0x5B = 1;
            work->act_0x5C = plw->field_0x306;
        }
    }
    rec = eft050_float_find(self, 1);
    if (self->demo_flag_0x08 == 3) {
        self->field_0x06 = 5;
        self->field_0x07 = 0;
        self->demo_flag_0x08 = 0;
        if (rec != NULL) {
            work->act_0x5C = ((_PLW*)rec->source_0x30)->field_0x306;
        }
        work->respawn_0x5B = 1;
    }
}

/* Sends the fish back to swimming after a random 10-41 frame wait. */
static inline void eft050_fish_swim_again(_EFT* self)
{
    self->field_0x06 = 1;
    self->field_0x07 = 0;
    self->timer_0x0C = (ran_suu(1) & 0x1F) + 10;
}

/* The fish bites: it nibbles at the float a random number of times, then takes it - darting off sideways while the
 * player strikes - and sets the line's tug from its kind; it gives up when the float leaves or the player's catch
 * slots are full. */
extern "C" void eft050_fish_bite(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    Eft050Slot* slot = eft050_fish_kind_get(work->kind_0x58);
    nw4r::math::VEC3 step;
    nw4r::math::VEC3 move;
    nw4r::math::MTX34 mtx;
    _CP_VECTOR rot;
    _EFT* flt;
    _PLW* plw;
    _EFT050_PL_FISH* fish;
    u8 level;
    s32 div;
    u32 yaw;

    VEC3_ctor(&step);
    VEC3_ctor(&move);
    MTX34_ctor(&mtx);
    flt = eft050_float_find(self, 1);
    if (flt != NULL) {
        if (Pl_act_ck((_PLW*)flt->source_0x30, 0, 0x42) != 0 || Pl_act_ck((_PLW*)flt->source_0x30, 0, 0x43) != 0) {
            eft050_fish_swim_again(self);
            return;
        }
    } else {
        eft050_fish_swim_again(self);
        return;
    }
    plw = (_PLW*)flt->source_0x30;
    fish = (_EFT050_PL_FISH*)plw;
    switch (self->demo_flag_0x08) {
    case 2:
        Pl_Skill_ck(plw, 0x95);
        if (work->slot_0x5A != 0xFF && fish->catches_0x59C[work->slot_0x5A] != 0 && Pl_act_ck(plw, 0, 0x41) != 0) {
            self->field_0x06 = 3;
            self->field_0x07 = 0;
            return;
        }
        break;
    default:
        if (work->slot_0x5A != 0xFF && fish->catches_0x59C[0] != 0 && Pl_act_ck(plw, 0, 0x41) != 0) {
            self->field_0x06 = 3;
            self->field_0x07 = 0;
            fish->catches_0x59C[0] = slot->id_0x00;
            return;
        }
        break;
    }
    if (Pl_act_ck(plw, 0, 0x41) != 0) {
        eft050_fish_swim_again(self);
        return;
    }
    switch (self->field_0x07) {
    case 0:
        self->field_0x07++;
        if (plw->field_0x306 == 205) {
            work->nibbles_0x36 = 0;
        } else if (self->demo_flag_0x08 >= 2 || plw->field_0x306 != 206) {
            if (Pl_Skill_ck((_PLW*)flt->source_0x30, 0x95) == 1 && fish->catch_count_0x597 == 0) {
                work->nibbles_0x36 = 0;
            } else {
                work->nibbles_0x36 = ran_suu(0) % slot->nibbles_0x03 + 1;
            }
        } else {
            work->nibbles_0x36 = ran_suu(0) % slot->nibbles_0x03 + 1;
        }
        break;
    case 1:
        if (--work->nibbles_0x36 <= 0) {
            self->field_0x07 = 5;
        } else {
            self->field_0x07++;
        }
        self->timer_0x0C = 5;
        work->speed_0x20 = calcVecDistXZ(&self->pos_0x18, &flt->pos_0x18) / self->timer_0x0C;
        yaw = eft050_xz_angle_get(&self->pos_0x18, &flt->pos_0x18);
        work->rot_y_0x50 = yaw;
        work->yaw_goal_0x40 = yaw;
        break;
    case 2:
    case 5:
        step.x = 0.0f;
        step.y = 0.0f;
        step.z = work->speed_0x20;
        rot.x = work->rot_x_0x4C;
        rot.y = work->rot_y_0x50;
        rot.z = work->rot_z_0x54;
        cpSetRotMatrixYXZ(&rot, &mtx);
        mtx34_rotate_vec3(&move, &mtx, &step);
        addVec3To(&self->pos_0x18, &move);
        if (--self->timer_0x0C <= 0) {
            if (fish->catches_0x59C[0] != 0 && fish->catches_0x59C[0] != slot->id_0x00) {
                eft050_fish_swim_again(self);
                break;
            }
            if (self->field_0x07 == 5) {
                level = EFT050_CATCH_LIMIT((HoldOwner*)self, (HoldOwner*)flt);
                switch (level) {
                default:
                    if (fish->catch_count_0x597 <= level - 1 && fish->reeling_0x596 != 0) {
                        if ((ran_suu(0) & 3) == 0) {
                            fish->reeling_0x596 = 0;
                        }
                        fish->tug_0x59A = slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3];
                        if (Pl_Skill_ck(plw, 0x95) == 1) {
                            fish->tug_0x59A *= 2;
                        }
                    } else {
                        eft050_fish_swim_again(self);
                        return;
                    }
                    break;
                case 2:
                    if (fish->catch_count_0x597 <= level - 1 && fish->reeling_0x596 != 0) {
                        if (fish->catch_count_0x597 == level - 1) {
                            if (ran_suu(0) % 3 == 0) {
                                fish->reeling_0x596 = 0;
                                fish->tug_0x59A = slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3];
                            } else if (fish->tug_0x59A == 0) {
                                fish->tug_0x59A = (self->demo_flag_0x08 - 1) * 200 +
                                                  (slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3]);
                            }
                        } else {
                            if ((ran_suu(0) & 3) == 0) {
                                fish->reeling_0x596 = 0;
                                fish->tug_0x59A = slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3];
                            } else if (fish->tug_0x59A == 0) {
                                fish->tug_0x59A = (self->demo_flag_0x08 - 1) * 200 +
                                                  (slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3]);
                            }
                        }
                    } else {
                        eft050_fish_swim_again(self);
                        return;
                    }
                    break;
                case 3:
                    if (fish->catch_count_0x597 <= level - 1 && fish->reeling_0x596 != 0) {
                        if (fish->catch_count_0x597 == level - 1) {
                            if (ran_suu(0) % 2 == 0) {
                                fish->reeling_0x596 = 0;
                                fish->tug_0x59A = slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3];
                                fish->tug_0x59A *= 2;
                            } else if (fish->tug_0x59A == 0) {
                                fish->tug_0x59A = (self->demo_flag_0x08 - 1) * 200 +
                                                  (slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3]);
                                fish->tug_0x59A *= 2;
                            }
                        } else {
                            div = (fish->catch_count_0x597 == 0) ? 5 : 3;
                            if (ran_suu(0) % div == 0) {
                                fish->reeling_0x596 = 0;
                                fish->tug_0x59A = slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3];
                                fish->tug_0x59A *= 2;
                            } else if (fish->tug_0x59A == 0) {
                                fish->tug_0x59A = (self->demo_flag_0x08 - 1) * 200 +
                                                  (slot->bite_0x02 + eft050_tug_offsets[ran_suu(0) & 3]);
                                fish->tug_0x59A *= 2;
                            }
                        }
                    } else {
                        eft050_fish_swim_again(self);
                        return;
                    }
                    break;
                }
                work->slot_0x5A = fish->catch_count_0x597;
                fish->catches_0x59C[work->slot_0x5A] = slot->id_0x00;
                fish->catch_count_0x597++;
                work->dart_0x5E = (ran_suu(0) & 0xF) + 20;
                self->field_0x10 = 5;
                work->biting_0x5F = 1;
                pl_act_hold_notify(plw, 1);
            } else {
                level = EFT050_CATCH_LIMIT((HoldOwner*)self, (HoldOwner*)flt);
                if (fish->catch_count_0x597 <= level - 1 && fish->reeling_0x596 != 0) {
                    step.x = flt->pos_0x18.x;
                    step.y = 10.0f + plw->field_0x064;
                    step.z = flt->pos_0x18.z;
                    eft004_set(2, &step, 0.7f, 0, plw->area_0x16);
                    se_req_orb(0, &step);
                    pl_act_hold_notify(plw, 4);
                } else {
                    eft050_fish_swim_again(self);
                    return;
                }
            }
            self->field_0x07++;
            work->speed_0x20 = -3.5f;
            work->accel_0x2C = 0.5f;
        }
        break;
    case 3:
    case 6:
        move.x = 0.0f;
        move.y = 0.0f;
        move.z = work->speed_0x20;
        rot.x = work->rot_x_0x4C;
        rot.y = work->rot_y_0x50;
        rot.z = work->rot_z_0x54;
        rotVecY(&move, rot.y);
        addVec3To(&self->pos_0x18, &move);
        if ((work->speed_0x20 += work->accel_0x2C) >= 0.0f) {
            self->field_0x07++;
            level = EFT050_CATCH_LIMIT((HoldOwner*)self, (HoldOwner*)flt);
            if (self->field_0x07 == 7) {
                switch (level) {
                case 2:
                case 3:
                    if (fish->catch_count_0x597 <= level) {
                        self->timer_0x0C = fish->tug_0x59A;
                    } else {
                        self->field_0x06 = 1;
                        self->field_0x07 = 0;
                        fish->catch_count_0x597--;
                        fish->catches_0x59C[work->slot_0x5A] = 0;
                        work->slot_0x5A = 0xFF;
                        self->timer_0x0C = (ran_suu(1) & 0x1F) + 10;
                    }
                    break;
                default:
                    self->timer_0x0C = 20;
                    break;
                }
            } else {
                self->timer_0x0C = 20;
            }
        }
        break;
    case 4:
        if (--self->timer_0x0C <= 0) {
            self->field_0x07 = 1;
        }
        break;
    case 7:
        if (fish->tug_0x59A == 0) {
            self->field_0x07++;
            self->timer_0x0C = (ran_suu(0) & 0x1F) + 60;
            work->yaw_goal_0x40 = work->rot_y_0x50 + ((ran_suu(0) & 7) << 12);
            fish->catches_0x59C[work->slot_0x5A] = 0;
            work->slot_0x5A = 0xFF;
            fish->catch_count_0x597--;
            flt->demo_flag_0x08 = 0;
            work->speed_0x20 = 8.0f;
            work->accel_0x2C = -0.3f;
        } else if (work->dart_0x5E <= 0) {
            if (--self->field_0x10 >= 0) {
                work->yaw_goal_0x40 = eft050_xz_angle_get(&self->pos_0x18, &flt->pos_0x18);
                work->yaw_goal_0x40 -= 0x400;
                work->yaw_goal_0x40 += ran_suu(0) & 0x7FF;
                eft050_angle_step((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, 0x800);
            } else {
                work->yaw_goal_0x40 = eft050_xz_angle_get(&self->pos_0x18, &flt->pos_0x18);
                eft050_angle_step((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, 0xFFFF);
                work->dart_0x5E = (ran_suu(0) & 0xF) + 20;
                self->field_0x10 = 5;
            }
        } else {
            work->dart_0x5E--;
        }
        break;
    case 8:
        move.x = 0.0f;
        move.y = 0.0f;
        move.z = work->speed_0x20;
        rot.x = work->rot_x_0x4C;
        rot.y = work->rot_y_0x50;
        rot.z = work->rot_z_0x54;
        rotVecY(&move, rot.y);
        addVec3To(&self->pos_0x18, &move);
        if ((work->speed_0x20 += work->accel_0x2C) <= 0.0f) {
            eft050_fish_swim_again(self);
        }
        break;
    }
    eft050_angle_step((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, 0x100);
}

/* The fish on the line: it follows its float (pulling sideways by its catch slot) and tugs the rod every ten frames
 * until the float is reeled in, then hangs from the float (sub-state 2). */
extern "C" void eft050_fish_hooked(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    _EFT* flt = work->float_0x38;
    nw4r::math::VEC3 pos;
    _PLW* plw;

    VEC3_ctor(&pos);
    plw = (_PLW*)flt->source_0x30;
    if (flt->field_0x03 != 34 || flt->used_0x00 == 0 || flt->state_0x05 >= 2) {
        self->field_0x06 = 4;
        self->field_0x07 = 0;
        return;
    }
    switch (self->field_0x07) {
    case 0:
        self->field_0x07++;
        self->timer_0x0C = 10;
    case 1:
        work->yaw_goal_0x40 = eft050_xz_angle_get(&self->pos_0x18, &flt->pos_0x18);
        work->yaw_goal_0x40 += work->slot_0x5A << 12;
        eft050_angle_step((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, 0x800);
        self->pos_0x18.x = flt->pos_0x18.x;
        self->pos_0x18.z = flt->pos_0x18.z;
        if (--self->timer_0x0C <= 0) {
            self->timer_0x0C = 10;
            pl_act_hold_notify(plw, 1);
        }
        if (flt->field_0x06 > 3) {
            self->field_0x07++;
            work->hooked_0x59 = 1;
            pl_act_hold_notify(plw, 3);
        }
        break;
    case 2:
        work->rot_x_0x4C = -0x4000;
        switch (work->slot_0x5A) {
        case 1:
            work->rot_x_0x4C += 0x1000;
            break;
        case 2:
            work->rot_x_0x4C -= 0x1000;
            break;
        }
        copyVec3(&pos, &flt->pos_0x18);
        pos.y -= 10.0f * work->slot_0x5A;
        copyVec3(&self->pos_0x18, &pos);
        break;
    }
}

/* The fish respawns: a fishing-act fish first re-rolls its kind (or leaves when the act's table rules the spot out),
 * then it waits hidden at home for a while and comes back when no player stands in its scare radius. */
extern "C" void eft050_fish_respawn(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    _PLW* plw;

    switch (self->field_0x07) {
    case 0:
        eft050_fish_kind_set(self, work->models_0x00[0]);
        if (work->flag_0x60 == 1) {
            if (eft050_act_table_ck(self) == 1) {
                self->field_0x07++;
            } else if (eft050_float_find(self, 2) != NULL) {
                return;
            } else {
                work->respawn_0x5B = 0;
                work->act_0x5C = 0xFF;
                eft050_fish_kind_set(self, work->models_0x00[0]);
            }
        } else {
            self->field_0x07++;
        }
        copyVec3(&self->pos_0x18, &work->pos_0x08);
        if (work->respawn_0x5B == 1) {
            self->timer_0x0C = 10;
        } else {
            self->timer_0x0C = (s8)eft050_respawn_waits[ran_suu(0) & 3] * 30;
        }
        work->hooked_0x59 = 0;
        work->scale_0x14.x = 0.0f;
        work->scale_0x14.y = 0.0f;
        work->scale_0x14.z = 0.0f;
        break;
    case 1:
        if (--self->timer_0x0C <= 0.0f) {
            plw = (_PLW*)get_move_work_adrs(2);
            plw += (s8)my_player_no();
            if (eft050_hold_item_ck((EftHoldItem*)plw) == 0 && (plw->kind_0x09 == 3 || (plw->field_0x5A4 & 2) != 0) &&
                hit_point_sphr(&plw->vec_0x03C, &work->pos_0x08, work->scale_0x24) == 1) {
                self->timer_0x0C = 0;
                return;
            }
            self->field_0x06 = 0;
            self->field_0x07 = 0;
        }
        break;
    }
}

/* The fish flees: it turns away from the player, speeds off for 30 frames shrinking over the last ten, then hides
 * (state 4) after a short or a long wait. */
extern "C" void eft050_fish_flee(_EFT* self)
{
    nw4r::math::VEC3 step;
    nw4r::math::VEC3 away;
    nw4r::math::MTX34 mtx;
    Eft050Work* work;
    Eft050Slot* slot;
    _CP_VECTOR rot;
    _PLW* players;
    f32 shrink;

    VEC3_ctor(&step);
    VEC3_ctor(&away);
    MTX34_ctor(&mtx);
    work = (Eft050Work*)self->work_0x38;
    slot = eft050_fish_kind_get(work->kind_0x58);
    switch (self->field_0x07) {
    case 0:
        self->field_0x07++;
        self->timer_0x0C = 30;
        work->speed_0x20 = 24.0f;
        work->hooked_0x59 = 0;
        players = (_PLW*)get_move_work_adrs(2);
        copyVec3(&away, &VEC3_SUB(&self->pos_0x18, &players[(s8)my_player_no()].vec_0x03C));
        copyVec3(&step, &VEC3_SCALE(&away, work->scale_0x24));
        work->rot_y_0x50 = eft050_xz_angle_get(&self->pos_0x18, &step);
        work->rot_y_0x50 -= 0x4000;
        work->rot_y_0x50 += ran_suu(0) & 0x7FFF;
    case 1:
        if (--self->timer_0x0C < 0) {
            if (work->respawn_0x5B == 1) {
                self->timer_0x0C = 10;
            } else {
                self->timer_0x0C = 300;
            }
            self->field_0x07++;
            setVector3(&work->scale_0x14, 0.0f, 0.0f, 0.0f);
            return;
        }
        step.x = 0.0f;
        step.y = 0.0f;
        step.z = work->speed_0x20;
        rot.x = work->rot_x_0x4C;
        rot.y = work->rot_y_0x50;
        rot.z = work->rot_z_0x54;
        cpSetRotMatrix(&rot, &mtx);
        mulVecMat(&step, &mtx);
        addVec3To(&self->pos_0x18, &step);
        if (self->timer_0x0C < 10) {
            shrink = 0.1f * (slot->scale_0x08.x / 10.0f);
            work->scale_0x14.x -= shrink;
            work->scale_0x14.y -= shrink;
            work->scale_0x14.z -= shrink;
        }
        break;
    case 2:
        if (--self->timer_0x0C < 0) {
            self->field_0x06 = 4;
            self->field_0x07 = 0;
        }
        break;
    }
}

/* The table of kinds the spawn point rolls from: a fishing act's own table by the map kind and area, otherwise the
 * spawner's table (`flag_0x60` says which). */
static inline EftFishRoll* eft050_roll_table(_EFT* self, Eft050Work* work, u8 map)
{
    EftFishRoll* tbl;

    switch (work->act_0x5C) {
    case 0x185:
        tbl = eft050_rolls_act389;
        break;
    case 0x15F:
        switch (stage_map_kind_get(map)) {
        case 1:
            if ((s32)self->type_0x02 == 0) {
                if ((s32)self->area_0x44 == 0) {
                    tbl = eft050_rolls_act351_near;
                } else {
                    tbl = eft050_rolls_act351;
                }
            } else if ((s32)self->area_0x44 == 12) {
                tbl = eft050_rolls_act351_near;
            } else {
                tbl = eft050_rolls_act351;
            }
            break;
        case 4:
            if ((s32)self->area_0x44 == 0) {
                tbl = eft050_rolls_act351_near;
            } else {
                tbl = eft050_rolls_act351;
            }
            break;
        default:
            tbl = eft050_rolls_act351;
            break;
        }
        break;
    case 0x161:
        switch (stage_map_kind_get(map)) {
        case 4:
            if ((s32)self->area_0x44 == 0) {
                tbl = eft050_rolls_act353_m4;
            } else {
                tbl = eft050_rolls_act353;
            }
            break;
        case 5:
            if ((s32)self->area_0x44 == 6) {
                tbl = eft050_rolls_act353_m5;
            } else {
                tbl = eft050_rolls_act353;
            }
            break;
        default:
            tbl = eft050_rolls_act353;
            break;
        }
        break;
    case 0x186:
        tbl = eft050_rolls_act390;
        break;
    case 0x163:
        tbl = eft050_rolls_act355;
        break;
    default:
        tbl = (EftFishRoll*)work->param_0x3C;
        break;
    }
    return tbl;
}

/* Rolls the fish's kind: the weights of the spot's table summed, a random pick among them. */
extern "C" u32 eft050_kind_roll(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    EftFishRoll* tbl;
    u8 map = get_now_mapno();
    u32 total;
    u32 roll;
    u32 sum;

    switch (work->act_0x5C) {
    case 0x185:
        tbl = eft050_rolls_act389;
        work->flag_0x60 = 1;
        break;
    case 0x15F:
        switch (stage_map_kind_get(map)) {
        case 1:
            if ((s32)self->type_0x02 == 0) {
                if ((s32)self->area_0x44 == 0) {
                    tbl = eft050_rolls_act351_near;
                } else {
                    tbl = eft050_rolls_act351;
                }
            } else if ((s32)self->area_0x44 == 12) {
                tbl = eft050_rolls_act351_near;
            } else {
                tbl = eft050_rolls_act351;
            }
            break;
        case 4:
            if ((s32)self->area_0x44 == 0) {
                tbl = eft050_rolls_act351_near;
            } else {
                tbl = eft050_rolls_act351;
            }
            break;
        default:
            tbl = eft050_rolls_act351;
            break;
        }
        work->flag_0x60 = 1;
        break;
    case 0x161:
        switch (stage_map_kind_get(map)) {
        case 4:
            if ((s32)self->area_0x44 == 0) {
                tbl = eft050_rolls_act353_m4;
            } else {
                tbl = eft050_rolls_act353;
            }
            break;
        case 5:
            if ((s32)self->area_0x44 == 6) {
                tbl = eft050_rolls_act353_m5;
            } else {
                tbl = eft050_rolls_act353;
            }
            break;
        default:
            tbl = eft050_rolls_act353;
            break;
        }
        work->flag_0x60 = 1;
        break;
    case 0x186:
        tbl = eft050_rolls_act390;
        work->flag_0x60 = 1;
        break;
    case 0x163:
        tbl = eft050_rolls_act355;
        work->flag_0x60 = 1;
        break;
    default:
        tbl = (EftFishRoll*)work->param_0x3C;
        work->flag_0x60 = 0;
        break;
    }
    for (total = 0; tbl->weight != 0xFF; tbl++) {
        total += tbl->weight;
    }
    roll = ran_suu(0) % total;
    tbl = eft050_roll_table(self, work, map);
    for (sum = 0; tbl->weight != 0xFF; tbl++) {
        sum += tbl->weight;
        if (roll < sum) {
            return tbl->kind;
        }
    }
    return 0;
}

/* Rolls a kind and shows its model joint (two kinds share a model). */
extern "C" void eft050_fish_kind_set(_EFT* self, MHchar* model)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;

    switch (eft050_kind_roll(self)) {
    case 0xCF:
        eft050_model_joint_show(model, 1);
        work->kind_0x58 = 1;
        break;
    case 0xD1:
        eft050_model_joint_show(model, 4);
        work->kind_0x58 = 4;
        break;
    case 0xD0:
        eft050_model_joint_show(model, 2);
        work->kind_0x58 = 2;
        break;
    case 0xD6:
        eft050_model_joint_show(model, 10);
        work->kind_0x58 = 10;
        break;
    case 0xD2:
        eft050_model_joint_show(model, 3);
        work->kind_0x58 = 3;
        break;
    case 0xD3:
        eft050_model_joint_show(model, 8);
        work->kind_0x58 = 8;
        break;
    case 0xD7:
        eft050_model_joint_show(model, 9);
        work->kind_0x58 = 9;
        break;
    case 0xD4:
        eft050_model_joint_show(model, 5);
        work->kind_0x58 = 5;
        break;
    case 0xD5:
        eft050_model_joint_show(model, 6);
        work->kind_0x58 = 6;
        break;
    case 0xD9:
        eft050_model_joint_show(model, 7);
        work->kind_0x58 = 7;
        break;
    case 0xD8:
        eft050_model_joint_show(model, 7);
        work->kind_0x58 = 11;
        break;
    case 0xE6:
        eft050_model_joint_show(model, 12);
        work->kind_0x58 = 12;
        break;
    case 0xE9:
        eft050_model_joint_show(model, 11);
        work->kind_0x58 = 13;
        break;
    default:
        eft050_model_joint_show(model, 1);
        work->kind_0x58 = 1;
        break;
    }
}

/* Shows or hides the model's twelve joints, leaving the one `index` visible. */
extern "C" void eft050_model_joint_show(MHchar* model, u32 index)
{
    for (s32 i = 1; i <= 0xC; i++) {
        if (i == index) {
            model->setVisibility(i, TRUE);
        } else {
            model->setVisibility(i, FALSE);
        }
    }
}

/* How many fish the float's player can have on the line: 1, or 2/3 for a bite of strength 2 while the player has
 * skill 0x95 (3 when `Pl_Skill_ck` reports 1). */
#pragma peephole on
extern "C" s32 eft050_catch_limit_get(HoldOwner* state, HoldOwner* owner)
{
    return (state->mode_0x08 == 2) ? (Pl_Skill_ck(owner->plw_0x30, 0x95) == 1) + 2 : 1;
}

/* Whether the fishing act's own table covers the fish's spot (0 for the spots whose map kind and area keep the
 * spawner's table). */
extern "C" u32 eft050_act_table_ck(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    u8 map = get_now_mapno();

    if ((s32)self->type_0x02 == 0) {
        switch (work->act_0x5C) {
        case 0x185:
            switch (stage_map_kind_get(map)) {
            case 3:
                if ((s32)self->area_0x44 == 3) {
                    return 0;
                }
                break;
            case 5:
                if ((s32)self->area_0x44 == 6) {
                    return 0;
                }
                break;
            }
            break;
        case 0x15F:
            switch (stage_map_kind_get(map)) {
            case 2:
                if ((s32)self->area_0x44 == 9) {
                    return 0;
                }
                break;
            case 3:
                switch (self->area_0x44) {
                case 0:
                    return 0;
                case 3:
                    return 0;
                }
                break;
            }
            break;
        case 0x161:
            switch (stage_map_kind_get(map)) {
            case 1:
                switch (self->area_0x44) {
                case 0:
                    return 0;
                case 12:
                    return 0;
                }
                break;
            case 2:
                if ((s32)self->area_0x44 == 9) {
                    return 0;
                }
                break;
            }
            break;
        case 0x186:
            switch (stage_map_kind_get(map)) {
            case 1:
                if ((s32)self->area_0x44 == 0) {
                    return 0;
                }
                break;
            case 3:
                if ((s32)self->area_0x44 == 3) {
                    return 0;
                }
                break;
            case 4:
                if ((s32)self->area_0x44 == 0) {
                    return 0;
                }
                break;
            }
            break;
        case 0x163:
            switch (stage_map_kind_get(map)) {
            case 1:
                if ((s32)self->area_0x44 == 0) {
                    return 0;
                }
                break;
            case 3:
                if ((s32)self->area_0x44 == 0) {
                    return 0;
                }
                break;
            case 4:
                if ((s32)self->area_0x44 == 0) {
                    return 0;
                }
                break;
            case 5:
                if ((s32)self->area_0x44 == 6) {
                    return 0;
                }
                break;
            }
            break;
        }
    } else {
        switch (work->act_0x5C) {
        case 0x161:
            switch (self->area_0x44) {
            case 0:
                return 0;
            case 12:
                return 0;
            }
            break;
        case 0x186:
            if ((s32)self->area_0x44 == 0) {
                return 0;
            }
            break;
        case 0x163:
            switch (self->area_0x44) {
            case 0:
                return 0;
            case 12:
                return 0;
            }
            break;
        }
    }
    return 1;
}

/* Spawns the five fish of an interior water spot: in a quest move it runs the plain spawner, otherwise the map kind
 * and spot pick the kind table (by `size`) and the scare-radius factor. */
#pragma peephole off
extern "C" void eft050_interior_fx_spawn(u32 id, nw4r::math::VEC3* pos, f32 scale, u32 size)
{
    nw4r::math::VEC3 unused;
    u8 map;
    f32 radius;
    s32 i;
    EftFishRoll* tbl;

    VEC3_ctor(&unused);
    map = get_now_mapno();
    if (move_work_state_ck() == 1) {
        eft050_fish_school_spawn((u8)id, pos, scale, 0);
        return;
    }
    radius = 1.0f;
    switch (stage_map_kind_get(map)) {
    case 1:
        switch ((u8)id) {
        case 0:
            tbl = eft050_rolls_m1_0[(u8)size];
            break;
        case 12:
            tbl = eft050_rolls_m1_12[(u8)size];
            break;
        default:
            return;
        }
        break;
    case 2:
        switch ((u8)id) {
        case 9:
            tbl = eft050_rolls_m2_9[(u8)size];
            radius = 0.5f;
            break;
        default:
            return;
        }
        break;
    case 3:
        switch ((u8)id) {
        case 0:
            tbl = eft050_rolls_m3_0[(u8)size];
            radius = 0.8f;
            break;
        case 3:
            tbl = eft050_rolls_m3_3[(u8)size];
            radius = 0.5f;
            break;
        default:
            return;
        }
        break;
    case 4:
        switch ((u8)id) {
        case 0:
            tbl = eft050_rolls_m4_0[(u8)size];
            break;
        default:
            return;
        }
        break;
    case 5:
        switch ((u8)id) {
        case 6:
            tbl = eft050_rolls_m5_6[(u8)size];
            radius = 0.5f;
            break;
        default:
            return;
        }
        break;
    default:
        return;
    }
    for (i = 5; i > 0; i--) {
        eft050_fish_spawn(0, pos, scale, scale * radius, (u32)tbl, (u8)id);
    }
}

/* Spawns five of the family's effects around one position, one per rotation step. */
extern "C" void eft050_fish_school_spawn(u32 kind, VEC3* pos, f32 scale, u8 flag)
{
    s32 i;
    const u16* ids;

    switch ((u8)kind) {
    case 0:
        ids = eft050_rolls_school_0;
        break;
    case 12:
        ids = eft050_rolls_school_12;
        break;
    default:
        return;
    }
    for (i = 5; i > 0; i--) {
        eft050_fish_spawn(1, pos, scale, scale, (u32)ids, kind);
    }
}
