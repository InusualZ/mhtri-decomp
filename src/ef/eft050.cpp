/* ef/eft050.cpp - the eft050 effect family and the cockpit item-hold band it spawns from.
 * RANGE. .text 0x8033F270-0x803432B4 (46 functions); extab 0x80016CA4-0x80016DB4, extabindex 0x8003615C-0x800362F4,
 *   .data 0x805E7410-0x805E7B50, .sdata 0x8079308C-0x807930F0, .sdata2 0x8079B2C0-0x8079B320.  One maximal run:
 *   `tudiscover.py at 0x8033F270` must-links only `fn_8033F270` and offers no anchored seam, so the band may hold
 *   more than one original TU.  The item-hold band (0x8033F270-0x8033FDA8) draws the hold through the `hud`
 *   sprite calls and reads `lobby_world_block` and the `_PLW` weapon state; the rest is the family's spawners
 *   (`eft050_set`, `fn_8033FDA8`), its `release_0x40`/`dispatch_0x34` hooks (`fn_8034305C`, `fn_803430F4`) and its
 *   `state_0x05` machine.
 * FLAGS. `cflags_main`; `#pragma peephole off` around `fn_80342AC0` (retail keeps the unfused countdown, `subi` +
 *   `cmpwi`, not the peephole's `subic.`).
 * NAMES. `eft050_set` is the runtime dump's own name; the map has only `fn_` stems for the other 45 rows, so plain
 *   definitions are `extern "C"`.  `Eft050Work`, `Eft051Work` and `Eft050SetWork` type `_EFT::work_0x38` per family
 *   from the offsets each reads; the first family's count byte at +0x04 overlaps its handle list above a count of 1.
 *   GUESS (from each body and its callers): eft050_interior_fx_spawn
 *   GUESS (from each body and its callers): `eft050_bite_strength_get`, `eft050_float_find`, `eft050_fish_appear`,
 *   `eft050_fish_swim`, `eft050_fish_bite`, `eft050_fish_hooked`, `eft050_fish_respawn`, `eft050_fish_flee`,
 *   `eft050_kind_roll`, `eft050_act_table_ck`, `eft051_spawn`.
 * RESIDUALS. 17 rows unwritten: 0x8033FAA4-0x8033FD78, 0x80340010-0x803401B8, 0x8034028C-0x80340594,
 *   0x803405C8-0x80342504, 0x803425C8-0x80342AC0, 0x80342B70-0x80342C60, 0x80342CAC-0x80342E80,
 *   0x80343130-0x803432B4.
 *   15 partial rows, including:
 *  - `fn_8033F40C`, `fn_8033F560`, `fn_8033FA2C`: retail addresses the `.sdata` tables `@sda21`
 *    (`lbl_8079308C`, `lbl_80793098`, `lbl_807930A0`), ours with `lis`/`addi` because the tables are `extern` here;
 *    `fn_8033F40C` also has a 0x20 frame against retail's 0x30;
 *  - `fn_8033F270`: retail narrows `anchor.x` with an `extsh` before the store; ours omits it;
 *  - `fn_80342E80`: ours narrows the sum (`extsh`) before the `sth`, retail only for the compare;
 *  - `fn_80340218`: ours sign-extends before the `neg` where retail masks after it (`clrlwi`).
 *   The other 9 partial rows have no recorded cause (`symdiff.py -u ef/eft050 --all`).
 *   flipcheck: `.data`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0x1024 of 0x4044), extab (0x88 of 0x110) and
 *   extabindex (0xCC of 0x198) short of the claim and differing.
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
    /* +0x51A4 */ u8 clock_hour_0x51A4;
    /* +0x51A5 */ u8 clock_minute_0x51A5;
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

/* The `.bss` hold block at 0x806BF310, the record `fn_8033F788` walks (the map makes it 0x58 = 88
 * bytes; only the fields this band reads are named). size: 0x28 (approximation) */
typedef struct CockpitItemBlock {
    /* +0x00 */ u8 state_0x00;      /* 1/2 draw the selection frame, `fn_8033F788` tests `+0xFF` */
    /* +0x01 */ u8 unused_0x01[0x07];
    /* +0x08 */ u16 frame_id_0x08;  /* handed to `spr_anim_draw` as the frame's part */
    /* +0x0A */ u16 sel_id_0x0A;    /* the same, for the two cursor sprites */
    /* +0x0C */ u16 sel_flag_0x0C;  /* set while a selection is drawn */
    /* +0x0E */ u16 cursor_0x0E;    /* the slot the cursor sits on */
    /* +0x10 */ u16 cursor2_0x10;   /* the slot the second cursor marker sits on */
    /* +0x12 */ u8 unused_0x12[0x12];
    /* +0x24 */ const CockpitItemSlot* items_0x24; /* the 50-slot hold list */
} CockpitItemBlock;

/* The two-line panel `fn_8033F270`/`fn_8033F40C`/`fn_8033F560` draw; `fn_8033F638` addresses the one
 * at +0x9C of its record.  The field names come from the sprite id each value is drawn at (the
 * `draw_font_idx_num(sprite, value, mode, pos)` calls). size: 0x1A */
typedef struct CockpitPanel {
    /* +0x00 */ u8 value_0x00;  /* drawn at sprite 0x201C */
    /* +0x01 */ u8 value_0x01;  /* 0x201D */
    /* +0x02 */ s16 count_0x02; /* + count_delta_0x04 -> 0x201E */
    /* +0x04 */ s16 count_delta_0x04;
    /* +0x06 */ s16 count_0x06; /* + count_delta_0x08 -> 0x201F */
    /* +0x08 */ s16 count_delta_0x08;
    /* +0x0A */ u8 icon_0x0A;   /* index into the icon table lbl_805E7180, 0xFF = none */
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

/* The record `fn_8033F638` walks: one panel at +0x9C of it. size: 0xB6 (approximation) */
typedef struct CockpitRecord {
    /* +0x00 */ u8 unused_0x00[0x9C];
    /* +0x9C */ CockpitPanel panel_0x9C;
} CockpitRecord;

/* The `eft050` fish's work block (the 100-byte block `fn_8033FDA8` pools): its model, its home, scale, swim speed and
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
    /* +0x58 */ s8 kind_0x58;           /* the fish kind (`fn_803401CC`'s table index) */
    /* +0x59 */ u8 hooked_0x59;         /* set while it is hooked */
    /* +0x5A */ u8 slot_0x5A;           /* its slot in the player's catch list, 0xFF = none */
    /* +0x5B */ u8 respawn_0x5B;        /* respawn soon */
    /* +0x5C */ u16 act_0x5C;           /* the fishing act it was spawned for */
    /* +0x5E */ s8 dart_0x5E;           /* frames until the hooked fish darts */
    /* +0x5F */ u8 biting_0x5F;         /* set while it bites */
    /* +0x60 */ u8 flag_0x60;           /* the kind came from the fishing act's own table */
} Eft050Work;

/* The family `fn_80342C70` drives (`eft051_spawn` pools 0x0C bytes): the pooled models, the spot and kind the source
 * picked and its countdown.  size: 0x0C */
typedef struct Eft051Work {
    /* +0x00 */ s32 count_0x00;         /* the pooled handle list's used length */
    /* +0x04 */ MHchar* models_0x04[1]; /* the pooled handle list `fn_80342C60` releases */
    /* +0x08 */ u8 spot_0x08;           /* the spot record of the map's table */
    /* +0x09 */ u8 kind_0x09;           /* the joint the model shows (kind + 1) */
    /* +0x0A */ s16 timer_0x0A;         /* the countdown `fn_80342E80` runs to 0xB4 */
} Eft051Work;

/* The block `eft050_set` pools for the family `fn_803430F4` drives: a word count at +0x00, the four
 * pooled handles at +0x04, the extra record the fourth `eft_res_model_get` hands back at +0x14 and the
 * eight-byte run the spawner zeroes at +0x1C. size: 0x24 (the bytes this spawner writes) */
typedef struct Eft050SetWork {
    /* +0x00 */ s32 count_0x00;
    /* +0x04 */ MHchar* models_0x04[4];
    /* +0x14 */ void* extra_0x14;   /* the record the model create fills; NULL retires the effect */
    /* +0x18 */ void* created_0x18; /* what `res_eft_UV_model_create` returned */
    /* +0x1C */ struct _g3d_work* works_0x1C[2]; /* the create's handle list, pushed back in reverse */
    /* +0x24 */ u8 unused_0x24[0x04];
    /* +0x28 */ f32 scale_0x28;     /* the UV model's scale */
} Eft050SetWork;

/* The item record `fn_80340594` tests (a flag byte that disables the gauge and the item id).
 * size: 0x10 (approximation: only the bytes this band reads are named) */
typedef struct EftHoldItem {
    /* +0x00 */ u8 unused_0x00[0x0A];
    /* +0x0A */ u8 gauge_off_0x0A; /* nonzero: the id is not drawn as a gauge */
    /* +0x0B */ u8 unused_0x0B[0x01];
    /* +0x0C */ u16 item_id_0x0C;
} EftHoldItem;

/* One hold row of `fn_8033F40C`: the three sprite ids a row draws, left to right. size: 0x6 */
typedef struct CockpitRow {
    /* +0x00 */ u16 icons_0x00[3];
} CockpitRow;

/* One fish kind of the table `fn_803401CC` addresses by the kind byte: the catch id, the bite strength and nibble
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

/* One decoration spot of the eft051 tables: the position and the yaw. size: 0x10 */
typedef struct EftSpot {
    /* +0x00 */ Vec pos;
    /* +0x0C */ u32 yaw;
} EftSpot;

/* The lobby object `eft051_spawn` decorates: its spot index.  size: 0xB7 - lower bound, an approximation */
typedef struct EftLobbySpot {
    /* +0x00 */ u8 unused_0x00[0xB6];
    /* +0xB6 */ u8 spot_0xB6;
} EftLobbySpot;

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

/* The record `fn_8034257C` reads its draw mode from and reaches the player work through.
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
const u8* fn_802D773C(u8 index);
void fn_802D7754(u8 a, u8 b, u16* out);
const u8* lb_entry_id_get(u8 index);
s8* get_item_name_str(u8 id);
s8* get_player_name_str(u8 index);
s8* get_digit_str(u8 id);
void lb_item_box_count_draw(void* work, u16 id, u32 arg);
void eft052_box_list_draw(u16 id, const void* tbl, const void* pos, u16 id2, u32 arg);
void fn_8033F788(CockpitItemBlock* self);
/* this unit's own bodies, forward-declared (address order) */
void fn_8033F270(CockpitPanel* self, const _mh_ivec2_* pos);
void fn_8033F40C(CockpitPanel* self, const _mh_ivec2_* pos);
void fn_8033F560(CockpitPanel* self, const _mh_ivec2_* pos);
void fn_8033F638(CockpitRecord* self);
void fn_8033F6C4(s32 value, _mh_ivec2_* out);
void fn_8033FA2C(void);
s32 fn_8033FD78(void);
void fn_8033FEE8(_EFT* self);
void fn_8033FDA8(u8 type, VEC3* pos, f32 scale, f32 scale2, u32 param, u8 area);
void fn_8033FF24(_EFT* self);
void fn_8033FF60(_EFT* self);
void fn_80340010(_EFT* self);
void fn_803401B8(_EFT* self);
void fn_803401C8(_EFT* self);
Eft050Slot* fn_803401CC(s8 index);
u16 fn_803401E4(const VEC3* a, const VEC3* b);
s32 fn_80340218(s32* cur, u32* target, s32 step);
s32 fn_80340594(EftHoldItem* self);
void fn_80342504(MHchar* model, u32 index);
s32 fn_8034257C(HoldOwner* state, HoldOwner* owner);
void fn_80342AC0(u32 kind, VEC3* pos, f32 scale, u8 flag);
void fn_80342C60(_EFT* self);
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
void eft051_spawn(struct EftLobbySpot* src, u8 kind);
void fn_80342C70(_EFT* self);
void fn_80342E80(_EFT* self);
void fn_80342F20(_EFT* self);
void fn_80342F30(_EFT* self);
void fn_803430F4(_EFT* self);
void fn_803401B8(_EFT* self);
void fn_803401C8(_EFT* self);
void fn_8034235C(_EFT* self, MHchar* model);
void fn_80342CAC(_EFT* self);
void fn_8034305C(_EFT* self);
void fn_80343130(_EFT* self);
void eft_instance_release(_EFT* self);
void fn_803432B4(_EFT* self);
void eft_state_advance(_EFT* self);
void eft_instance_release(_EFT* self);

/* This unit's own `.data` run (0x805E7400..0x805E7A70), its `.sdata` tables and its `.sdata2` pool -
 * referenced by name, never defined (playbook 29). */
extern const u16 lbl_805E7410[];
extern const u8 lbl_805E7180[];
extern const u16 lbl_805E7438[];
extern CockpitRow lbl_805E7450[];
extern const u16 lbl_805E7464[];
extern const u16 lbl_805E7498[];
extern const u16 lbl_805E74F0[];
extern const u16 lbl_805E7508[];
extern const u16 lbl_805E7514[];
extern const u16 lbl_805E7528[];
extern const u16 lbl_805E753C[];
extern Eft050Slot lbl_805E7578[];
extern const u16 lbl_805E7760[];
extern const u16 lbl_805E7794[];
extern const u16 lbl_8079308C[6];

/* The fish tables of this unit's `.data`/`.sdata` (declared until the unit emits its data). */
extern f32 eft050_bite_speed[];
extern s16 eft050_tug_offsets[4];
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
extern EftSpot eft051_spots_m22[];
extern EftSpot eft051_spots_m21[];
extern f32 eft050_set_scales[];

extern const u16 lbl_80793098[4];
extern const u16 lbl_807930A0[4];

/* The hold block at 0x806BF310 (`fn_8033FA2C` hands its address to `fn_8033F788`). */
extern CockpitItemBlock eft052_item_box;

#ifdef __cplusplus
}
#endif

/* C++-scope callees (their map names are manglings, so the front-end has to reproduce them). */
s8* LbStr(u8 kind, u16 id);
void draw_font_idx(u16 id, s8* str, u32 mode, const _mh_ivec2_* pos);
u16 ran_suu(long index);
s32 chk_pointer(void);
u8 Pl_act_ck(_PLW* plw, u8 a, u16 b);
u32 Pl_Skill_ck(_PLW* plw, u16 id);
u8 Pl_master_ck(_PLW* plw);
void eft004_set(u8 type, nw4r::math::VEC3* pos, f32 scale, u32 param, u8 flag);

#pragma peephole off
#pragma optimization_level 4

/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)
/* `em_vec3_copy` returns its argument's copy by value. */
#define EFT_VEC3_COPY ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*))em_vec3_copy)

/* ---------------------------------------------------------------------------------------------------
 * bodies, in address order
 * ------------------------------------------------------------------------------------------------- */

/* Draws one hold slot's panel: the frame, the two counters with their deltas, the gauge bar and the
 * four value/timer pairs, each at its own sprite id. */
extern "C" void fn_8033F270(CockpitPanel* self, const _mh_ivec2_* pos)
{
    _mh_ivec2_ anchor = *pos;
    draw_sprite_ary(lbl_805E7410, pos);
    _SPR_DATA_ frame;
    spr_data_copy(&frame, get_lsp_data(0x2019, 0));
    frame.pos.x += 6;
    draw_sprite(frame, pos);
    if (self->icon_0x0A != 0) {
        u8 icon = lbl_805E7180[self->icon_0x0A];
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
extern "C" void fn_8033F40C(CockpitPanel* self, const _mh_ivec2_* pos)
{
    draw_sprite_ary(lbl_805E7438, pos);
    u8 sel = lobby_world_block->player_slot_0x3E03 & 0x7F;
    const u8* rows = fn_802D773C(sel);
    const u8* slots = lb_entry_id_get(sel);
    draw_font_idx(0x2043, get_player_name_str(sel), 1, pos);
    draw_font_idx_num(0x2044, self->mode_0x14, 1, pos);
    if (rows[slots[0] + 4] == 0) {
        draw_font_idx(0x2046, LbStr(0, 0x1C1), 1, pos);
    } else {
        for (s8 i = 0; i < rows[slots[0] + 4]; i++) {
            draw_sprite_idx(lbl_805E7450[i].icons_0x00[0], pos);
            draw_sprite_idx(lbl_805E7450[i].icons_0x00[1], pos);
            draw_sprite_idx(lbl_805E7450[i].icons_0x00[2], pos);
            draw_font_idx(lbl_8079308C[i], get_item_name_str(slots[i + 1]), 1, pos);
        }
    }
}

/* Draws the hold's gauge row, its label and the four clock digits the shared block carries. */
extern "C" void fn_8033F560(CockpitPanel* self, const _mh_ivec2_* pos)
{
    draw_sprite_ary(lbl_805E7464, pos);
    draw_font_idx_num(0x205F, self->value_0x16 + self->value_delta_0x18, 1, pos);
    draw_font_idx(0x2060, LbStr(0, (u16)(self->label_0x1A + 0x1C2)), 1, pos);
    u16 digits[4];
    fn_802D7754(lobby_world_block->clock_hour_0x51A4, lobby_world_block->clock_minute_0x51A5, digits);
    for (u16 i = 0; i < 4; i++) {
        draw_font_idx(lbl_80793098[i], get_digit_str((u8)digits[i]), 0, pos);
    }
}

/* Draws the three panels of one hold record: the frame sprite, then each panel at the anchor the
 * preceding `get_lsp_data` wrote. */
extern "C" void fn_8033F638(CockpitRecord* self)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x1FE1, &anchor);
    draw_sprite_ary(lbl_805E7498, &anchor);
    get_lsp_data(0x200D, &anchor);
    fn_8033F270(&self->panel_0x9C, &anchor);
    get_lsp_data(0x200E, &anchor);
    fn_8033F40C(&self->panel_0x9C, &anchor);
    get_lsp_data(0x200F, &anchor);
    fn_8033F560(&self->panel_0x9C, &anchor);
}

/* Places the two digit sprites of a two-digit value: the ones digit's for `value % 10` and the tens
 * digit's for `value / 10`, then writes the ones digit's anchor, moved right by 4, into `out`. */
extern "C" void fn_8033F6C4(s32 value, _mh_ivec2_* out)
{
    const _SPR_DATA_* ones = get_lsp_data(lbl_805E74F0[value % 10], 0);
    const _SPR_DATA_* tens = get_lsp_data(lbl_805E7508[value / 10], 0);
    out->x = (s16)(ones->pos.x + 4);
    out->y = tens->pos.y;
}

/* Draws the hold list of the block at 0x806BF310: its frame, its 50 item icons with the selected
 * slot's frame around them, and - while a selection is active - the two cursor sprites. */
#pragma peephole on
extern "C" void fn_8033F788(CockpitItemBlock* self)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x2081, &anchor);
    draw_sprite_anim_ary(lbl_805E7514, 0, &anchor);
    s32 found = 0;
    _SPR_DATA_ frame;
    spr_data_copy(&frame, get_lsp_data(0x1307, 0));
    _mh_ivec2_ frame_pos;
    uv_pair_copy(&frame_pos, &frame);
    _mh_ivec2_ cursor = { 0, 0 };
    for (s32 i = 0; i < 0x32; i++) {
        _mh_ivec2_ digit;
        fn_8033F6C4(i, &digit);
        u16 id = (i < 0x14) ? 0x2092 : 0x209B;
        _SPR_DATA_ rec;
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
                    _SPR_DATA_ sel;
                    spr_data_copy(&sel, get_lsp_data(0x1304, 0));
                    sel.pos.x = (s16)(sel.pos.x + digit.x);
                    sel.pos.y = (s16)(sel.pos.y + digit.y);
                    set_blendmode(4, 1, 1);
                    spr_anim_draw(&sel, self->frame_id_0x08, &anchor);
                    set_blendmode(4, 5, 1);
                }
            }
        }
        u16 item = self->items_0x24[i].item_id_0x00;
        if (item != 0) {
            draw_itemicon_item_id(frame, item, &anchor);
        }
    }
    if (self->sel_flag_0x0C != 0 && found != 0) {
        set_blendmode(4, 1, 1);
        _SPR_DATA_ sel;
        spr_data_copy(&sel, get_lsp_data(0x1305, 0));
        sel.pos.x = (s16)(sel.pos.x + cursor.x);
        sel.pos.y = (s16)(sel.pos.y + cursor.y);
        spr_anim_draw(&sel, self->sel_id_0x0A, &anchor);
        set_blendmode(4, 5, 1);
        spr_data_copy(&sel, get_lsp_data(0x1306, 0));
        sel.pos.x = (s16)(sel.pos.x + cursor.x);
        sel.pos.y = (s16)(sel.pos.y + cursor.y);
        spr_anim_draw(&sel, self->sel_id_0x0A, &anchor);
    }
}
#pragma peephole off

/* Draws the hold's own frame and the item-icon panel the shared block's selector points at. */
extern "C" void fn_8033FA2C(void)
{
    _mh_ivec2_ anchor;
    get_lsp_data(0x2066, &anchor);
    draw_sprite_ary(lbl_805E753C, &anchor);
    fn_8033F788(&eft052_item_box);
    lb_item_box_count_draw(&eft052_item_box, 0x2090, 0);
    eft052_box_list_draw(0x209F, lbl_805E7528, lbl_807930A0, 0x2065, 0);
}

/* Reports the hold's state: 0 while the busy flag is set, -1 while the hold flag is clear, 1
 * otherwise. */
extern "C" s32 fn_8033FD78(void)
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

/* Releases the pooled handle list of a work block and clears its used length. */
extern "C" void fn_8033FEE8(_EFT* self)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    eft_res_models_release(work->models_0x00, work->count_0x04);
    work->count_0x04 = 0;
}

/* Spawns the family's effect in its area: pools the record and work block, seeds the model handle, stamps the
 * type, area and hooks, and installs the state machine through `eft_state_flags_set`. */
extern "C" void fn_8033FDA8(u8 type, VEC3* pos, f32 scale, f32 scale2, u32 param, u8 area)
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
    eft->release_0x40 = fn_8033FEE8;
    eft->dispatch_0x34 = fn_8033FF24;
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

/* The state machine of the effect `fn_8033FDA8` spawns: 0 the model-create pass, 1 the alive body, 2
 * the state advance and 3 the pool release. */
extern "C" void fn_8033FF24(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return fn_8033FF60(self);
    case 1: return fn_80340010(self);
    case 2: return fn_803401B8(self);
    case 3: return fn_803401C8(self);
    }
}

/* State 0: creates the family's model for every pooled handle and hands each to the alive-state body,
 * or retires the effect when the create fails, then advances to state 1. */
extern "C" void fn_8033FF60(_EFT* self)
{
    s32 i;
    Eft050Work* work = (Eft050Work*)self->work_0x38;

    self->state_0x05++;
    self->field_0x06 = 0;
    for (i = 0; i < work->count_0x04; i++) {
        if (res_eft_model_create(work->models_0x00[i], 0x44, 0xC) == NULL) {
            fn_803401C8(self);
            return;
        }
        fn_8034235C(self, work->models_0x00[i]);
    }
    fn_80340010(self);
}

/* Runs the fish's state, keeps the swimming states inside the swim range, and pushes the rotation (with its wiggle),
 * position and scale onto the model. */
extern "C" void fn_80340010(_EFT* self)
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
extern "C" void fn_803401B8(_EFT* self)
{
    self->state_0x05++;
}

/* State 3 of the family's state machine: releases the pooled effect record. */
extern "C" void fn_803401C8(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Addresses one 0x14-byte slot of the table by its signed index. */
extern "C" Eft050Slot* fn_803401CC(s8 index)
{
    return &lbl_805E7578[index];
}

/* The squared distance of the two points' x/z pairs, narrowed to the u16 the caller wants. */
extern "C" u16 fn_803401E4(const VEC3* a, const VEC3* b)
{
    return (u16)calc_xz_angle(a->x, a->z, b->x, b->z);
}

/* Moves `cur` towards `target` by at most `step`, returning the step actually applied. */
extern "C" s32 fn_80340218(s32* cur, u32* target, s32 step)
{
    *target = (u16)*target;
    s32 delta = (u16)(*target - *cur);
    s32 mag = (delta >= 0x8000) ? -(s16)delta : delta;
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
    Eft050Slot* slot = fn_803401CC(((Eft050Work*)fish->work_0x38)->kind_0x58);

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
        return 3 - (kind == 1);
    case 351:
        if (kind == 4 || kind == 10) {
            return 2;
        }
        return 3;
    case 353:
        return 3 - ((u32)(kind - 5) <= 1);
    case 390:
        return 3 - (kind == 2);
    case 355:
        return 3 - (kind == 7);
    }
    return 0;
}

/* Finds the live float the fish reacts to: with `mode` 0 a float that has settled (state 2) and whose catch list has
 * room, turning the fish towards it; otherwise any float of a master player that takes the fish. */
extern "C" _EFT* eft050_float_find(_EFT* self, s32 mode)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;
    _EFT* rec;
    s32 i;
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
        level = fn_8034257C((HoldOwner*)self, (HoldOwner*)rec);
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
                work->yaw_goal_0x40 = fn_803401E4(&self->pos_0x18, &rec->pos_0x18);
                return rec;
            }
        } else if (Pl_master_ck(plw) == 1 && self->demo_flag_0x08 != 0) {
            return rec;
        }
    }
    return NULL;
}

/* Whether the record's item id is one of the five ids the hold bar draws as a gauge. */
extern "C" s32 fn_80340594(EftHoldItem* self)
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
    Eft050Slot* slot = fn_803401CC(work->kind_0x58);
    nw4r::math::VEC3 ofs;
    nw4r::math::MTX34 mtx;
    f32 inner;
    f32 dist;
    u16 yaw;
    u32 goal;

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
        goal = yaw + (ran_suu(0) & 0x1FFF) + 0x7000;
        work->rot_y_0x50 = goal;
        work->yaw_goal_0x40 = goal;
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
    s32 bite = 0;
    _EFT* flt;
    _EFT* rec;
    _PLW* plw;
    s32 frames;
    u16 yaw;

    VEC3_ctor(&step);
    VEC3_ctor(&move);
    VEC3_ctor(&unused);
    MTX34_ctor(&mtx);
    flt = eft050_float_find(self, 0);
    switch (self->field_0x07) {
    case 0:
        if (--self->timer_0x0C <= 0 || (s32)(work->rot_y_0x50 - work->yaw_goal_0x40) == 0) {
            self->field_0x07++;
            self->demo_flag_0x08 = 0;
            work->wiggle_0x48 = 0x100;
            if (flt == NULL) {
                yaw = fn_803401E4(&self->pos_0x18, &work->pos_0x08);
                work->yaw_goal_0x40 = yaw + ((ran_suu(1) & 0xF) << 11) - 0x4000;
            }
            work->speed_0x20 = 3.0f * eft050_bite_speed[self->demo_flag_0x08];
        }
        work->turn_0x44 += (0x20 - work->turn_0x44) / 8;
        fn_80340218((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, work->turn_0x44);
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
        fn_80340218((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, work->turn_0x44);
        break;
    }
    plw = (_PLW*)get_move_work_adrs(2) + (s8)my_player_no();
    if (fn_80340594((EftHoldItem*)plw) == 0 && (plw->kind_0x09 == 3 || (plw->field_0x5A4 & 2) != 0) &&
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
        work->yaw_goal_0x40 = fn_803401E4(&self->pos_0x18, &flt->pos_0x18);
        work->yaw_goal_0x40 += work->slot_0x5A << 12;
        fn_80340218((s32*)&work->rot_y_0x50, &work->yaw_goal_0x40, 0x800);
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
        fn_8034235C(self, work->models_0x00[0]);
        if (work->flag_0x60 == 1) {
            if (eft050_act_table_ck(self) == 1) {
                self->field_0x07++;
            } else if (eft050_float_find(self, 2) == NULL) {
                work->respawn_0x5B = 0;
                work->act_0x5C = 0xFF;
                fn_8034235C(self, work->models_0x00[0]);
            } else {
                return;
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
            plw = (_PLW*)get_move_work_adrs(2) + (s8)my_player_no();
            if (fn_80340594((EftHoldItem*)plw) == 0 && (plw->kind_0x09 == 3 || (plw->field_0x5A4 & 2) != 0) &&
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
    slot = fn_803401CC(work->kind_0x58);
    switch (self->field_0x07) {
    case 0:
        self->field_0x07++;
        self->timer_0x0C = 30;
        work->speed_0x20 = 24.0f;
        work->hooked_0x59 = 0;
        players = (_PLW*)get_move_work_adrs(2);
        copyVec3(&away, &VEC3_SUB(&self->pos_0x18, &players[(s8)my_player_no()].vec_0x03C));
        copyVec3(&step, &VEC3_SCALE(&away, work->scale_0x24));
        work->rot_y_0x50 = fn_803401E4(&self->pos_0x18, &step);
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
    s32 total;
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
extern "C" void fn_8034235C(_EFT* self, MHchar* model)
{
    Eft050Work* work = (Eft050Work*)self->work_0x38;

    switch (eft050_kind_roll(self)) {
    case 0xCF:
        fn_80342504(model, 1);
        work->kind_0x58 = 1;
        break;
    case 0xD1:
        fn_80342504(model, 4);
        work->kind_0x58 = 4;
        break;
    case 0xD0:
        fn_80342504(model, 2);
        work->kind_0x58 = 2;
        break;
    case 0xD6:
        fn_80342504(model, 10);
        work->kind_0x58 = 10;
        break;
    case 0xD2:
        fn_80342504(model, 3);
        work->kind_0x58 = 3;
        break;
    case 0xD3:
        fn_80342504(model, 8);
        work->kind_0x58 = 8;
        break;
    case 0xD7:
        fn_80342504(model, 9);
        work->kind_0x58 = 9;
        break;
    case 0xD4:
        fn_80342504(model, 5);
        work->kind_0x58 = 5;
        break;
    case 0xD5:
        fn_80342504(model, 6);
        work->kind_0x58 = 6;
        break;
    case 0xD9:
        fn_80342504(model, 7);
        work->kind_0x58 = 7;
        break;
    case 0xD8:
        fn_80342504(model, 7);
        work->kind_0x58 = 11;
        break;
    case 0xE6:
        fn_80342504(model, 12);
        work->kind_0x58 = 12;
        break;
    case 0xE9:
        fn_80342504(model, 11);
        work->kind_0x58 = 13;
        break;
    default:
        fn_80342504(model, 1);
        work->kind_0x58 = 1;
        break;
    }
}

/* Shows or hides the model's twelve joints, leaving the one `index` visible. */
extern "C" void fn_80342504(MHchar* model, u32 index)
{
    for (s32 i = 1; i <= 0xC; i++) {
        if (i == index) {
            model->setVisibility(i, TRUE);
        } else {
            model->setVisibility(i, FALSE);
        }
    }
}

/* The hold slot's draw state: 1 normally, 2/3 while the player carries skill 0x95. */
#pragma peephole on
extern "C" s32 fn_8034257C(HoldOwner* state, HoldOwner* owner)
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
extern "C" void eft050_interior_fx_spawn(u32 id, nw4r::math::VEC3* pos, f32 scale, u32 size)
{
    nw4r::math::VEC3 unused;
    u8 map;
    EftFishRoll* tbl;
    f32 radius;
    s32 i;

    VEC3_ctor(&unused);
    map = get_now_mapno();
    if (move_work_state_ck() == 1) {
        fn_80342AC0((u8)id, pos, scale, 0);
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
        if ((s32)(u8)id != 9) {
            return;
        }
        tbl = eft050_rolls_m2_9[(u8)size];
        radius = 0.5f;
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
        if ((s32)(u8)id != 0) {
            return;
        }
        tbl = eft050_rolls_m4_0[(u8)size];
        break;
    case 5:
        if ((s32)(u8)id != 6) {
            return;
        }
        tbl = eft050_rolls_m5_6[(u8)size];
        radius = 0.5f;
        break;
    default:
        return;
    }
    for (i = 5; i > 0; i--) {
        fn_8033FDA8(0, pos, scale, scale * radius, (u32)tbl, (u8)id);
    }
}

#pragma peephole off

/* Spawns five of the family's effects around one position, one per rotation step. */
#pragma peephole off
extern "C" void fn_80342AC0(u32 kind, VEC3* pos, f32 scale, u8 flag)
{
    s32 i;
    const u16* ids;

    switch ((u8)kind) {
    case 0:
        ids = lbl_805E7760;
        break;
    case 12:
        ids = lbl_805E7794;
        break;
    default:
        return;
    }
    for (i = 5; i > 0; i--) {
        fn_8033FDA8(1, pos, scale, scale, (u32)ids, kind);
    }
}

/* Spawns the eft051 decoration of a lobby object `src` (kinds 0-2): one pooled model, the source's spot index. */
extern "C" void eft051_spawn(EftLobbySpot* src, u8 kind)
{
    _EFT* rec;
    Eft051Work* work;
    s32 i;

    if (kind > 2) {
        return;
    }
    rec = (_EFT*)eft_res_slot_get(0xC);
    if (rec == NULL) {
        return;
    }
    rec->release_0x40 = fn_80342C60;
    work = (Eft051Work*)rec->work_0x38;
    work->count_0x00 = 1;
    rec->area_0x44 = get_now_areano();
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x04[i] == NULL) {
            eft_res_slot_release(rec);
            return;
        }
    }
    rec->source_0x30 = NULL;
    rec->dispatch_0x34 = fn_80342C70;
    rec->field_0x03 = 49;
    eft_state_flags_set(rec, 8, 0);
    work->spot_0x08 = src->spot_0xB6;
    work->kind_0x09 = kind;
}


/* Releases a second family's pooled handle list, from its word behind the count. */
extern "C" void fn_80342C60(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    eft_res_models_release(work->models_0x04, work->count_0x00);
}

/* The second family's state machine: 0 the model-create pass, 1 the alive body, 2 the state advance
 * and 3 the pool release. */
extern "C" void fn_80342C70(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return fn_80342CAC(self);
    case 1: return fn_80342E80(self);
    case 2: return fn_80342F20(self);
    case 3: return fn_80342F30(self);
    }
}

/* Builds the eft051 models at the spot of the map's table (map 22 area 0, map 21 area 2), each showing the kind's
 * joint; no table ends the record. */
extern "C" void fn_80342CAC(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    EftSpot* spots = NULL;
    s32 i;
    u32 j;
    u32 joints;

    self->state_0x05++;
    for (i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create(work->models_0x04[i], 71, 8) == NULL) {
            fn_80342F30(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    self->field_0x10 = 0;
    switch (get_now_mapno()) {
    case 22:
        if ((s32)self->area_0x44 == 0) {
            spots = eft051_spots_m22;
        }
        break;
    case 21:
        if (self->area_0x44 == 2) {
            spots = eft051_spots_m21;
        }
        break;
    }
    if (spots == NULL) {
        fn_80342F30(self);
        return;
    }
    for (i = 0; i < work->count_0x00; i++) {
        vec_to_mh_vec3(&work->models_0x04[i]->pos_0x04, &spots[work->spot_0x08].pos);
        setVector3(&work->models_0x04[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        work->models_0x04[i]->field_0x28 = 0;
        work->models_0x04[i]->field_0x2C = spots[work->spot_0x08].yaw;
        work->models_0x04[i]->field_0x28 = 0;
        work->models_0x04[i]->ready = 0;
        joints = work->models_0x04[i]->get_joint_num();
        for (j = 0; j < joints; j++) {
            if (j == work->kind_0x09 + 1) {
                work->models_0x04[i]->setVisibility(j, 1);
            } else {
                work->models_0x04[i]->setVisibility(j, 0);
            }
        }
    }
    fn_80342E80(self);
}


/* State 1 of the second family: retires the effect once its timer runs past 0xB4, otherwise moves
 * every pooled model to its idle state and updates it. */
extern "C" void fn_80342E80(_EFT* self)
{
    Eft051Work* work = (Eft051Work*)self->work_0x38;
    s16 timer = (s16)(work->timer_0x0A + 1);
    work->timer_0x0A = timer;
    if (timer > 0xB4) {
        fn_80342F30(self);
        return;
    }
    for (s32 i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i]->move(0);
        eft_res_models_spawn(self, (void**)&work->models_0x04[i], 2, 1, NULL);
    }
}

/* State 2 of the second family's state machine: advances to the next body. */
extern "C" void fn_80342F20(_EFT* self)
{
    self->state_0x05++;
}

/* State 3 of the second family's state machine: releases the pooled effect record. */
extern "C" void fn_80342F30(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Releases the `eft050` family's pools: every model handle one at a time, the two `_g3d_work`
 * handles in reverse order, then the record the create filled. */
extern "C" void fn_8034305C(_EFT* self)
{
    s32 i;
    Eft050SetWork* work = (Eft050SetWork*)self->work_0x38;

    for (i = 0; i < work->count_0x00; i++) {
        eft_res_models_release(&work->models_0x04[i], 1);
    }
    for (i = 1; i >= 0; i--) {
        if (work->works_0x1C[i] != NULL) {
            push_g3d_wk(work->works_0x1C[i]);
        }
    }
    eft_res_models_release(&work->extra_0x14, 1);
}

/* Pools the `eft050` record: the effect, its work block, four pooled model handles, the extra record
 * the fourth pool call hands back, and the position/source/area seeds. */
void eft050_set(_PLW* plw, nw4r::math::VEC3* pos, u8 flag)
{
    s32 i;
    _EFT* eft;
    Eft050SetWork* work;

    eft = (_EFT*)eft_res_slot_get(0x2C);
    if (eft == NULL) {
        return;
    }
    eft->type_0x02 = 0;
    eft->dispatch_0x34 = fn_803430F4;
    eft->release_0x40 = fn_8034305C;
    work = (Eft050SetWork*)eft->work_0x38;
    work->count_0x00 = 4;
    for (i = 0; i < work->count_0x00; i++) {
        work->models_0x04[i] = (MHchar*)eft_res_model_get();
        if (work->models_0x04[i] == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    work->extra_0x14 = eft_res_model_get();
    if (work->extra_0x14 == NULL) {
        eft_res_slot_release(eft);
        return;
    }
    memset(work->works_0x1C, 0, 8);
    eft->field_0x03 = 0x32;
    eft_state_flags_set(eft, 1, 0);
    copyVec3(&eft->pos_0x18, pos);
    eft->area_0x44 = flag;
    eft->source_0x30 = plw;
    eft->flag_0x01 = 1;
    eft->timer_0x0C = 0;
    eft->field_0x10 = 0;
}

/* The `eft050` family's own state machine (its three bodies sit in the band above this one). */
extern "C" void fn_803430F4(_EFT* self)
{
    switch (self->state_0x05) {
    case 0: return fn_80343130(self);
    case 1: return fn_803432B4(self);
    case 2: return eft_state_advance(self);
    case 3: return eft_instance_release(self);
    }
}

/* Builds the eft050_set record's models: the UV model of the pool and the four plain models with their scales and a
 * random roll. */
extern "C" void fn_80343130(_EFT* self)
{
    Eft050SetWork* work = (Eft050SetWork*)self->work_0x38;
    u32 i;
    f32* scales;

    self->state_0x05++;
    work->created_0x18 = res_eft_UV_model_create((MHchar*)work->extra_0x14, 73, 332, 0, work->works_0x1C, 1, 0);
    if (work->created_0x18 == NULL) {
        eft_instance_release(self);
        return;
    }
    for (i = 0; i < (u32)work->count_0x00; i++) {
        if (res_eft_model_create(work->models_0x04[i], 74, 76) == NULL) {
            eft_instance_release(self);
            return;
        }
    }
    work->scale_0x28 = 1.0f;
    setVector3(&((MHchar*)work->extra_0x14)->scale_0x1C, 1.5f, 1.5f, 1.5f);
    ((MHchar*)work->extra_0x14)->field_0x28 = 0;
    ((MHchar*)work->extra_0x14)->field_0x2C = 0;
    ((MHchar*)work->extra_0x14)->field_0x30 = 0;
    ((MHchar*)work->extra_0x14)->ready = 3;
    scales = eft050_set_scales;
    for (i = 0; i < (u32)work->count_0x00; i++) {
        setVector3(&work->models_0x04[i]->scale_0x1C, *scales, *scales, *scales);
        work->models_0x04[i]->field_0x28 = 0;
        work->models_0x04[i]->field_0x2C = 0;
        work->models_0x04[i]->field_0x30 = ran_suu(0);
        work->models_0x04[i]->ready = 1;
        scales++;
    }
    fn_803432B4(self);
}
