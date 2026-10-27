/* ef/fn_8030681C.cpp - the tail of the eft041 effect machine (states 1-3), the whole eft042 effect family (its
 *   spawners, record pools, release hooks, the per-kind state bodies and their tables) and the two status-screen
 *   entry points at the end of the range.
 * RANGE. .text 0x8030681C-0x80308FB4 (26 functions); extab 0x80015AB4-0x80015B54, extabindex 0x80034674-0x80034764,
 *   .data 0x805DC5E8-0x805DCC50, .sdata 0x80792B78-0x80792BB8, .sdata2 0x8079ADE0-0x8079AE30.  The range is not one
 *   TU: the `.sdata2` pool pools 0.0f twice (0x8079ADE4 for `eft041_move`, 0x8079AE18 for
 *   `eft042_model_create_kind1`) and 1.0f twice (0x8079ADE0 for `ef/fn_803066F0.c`'s `eft041_model_create`,
 *   0x8079AE24), so eft041 (`lobby/fn_8030121C.cpp`'s `lb_area_fx_set` 0x80306524 .. `eft041_kill`, ending
 *   0x80306A98) and eft042 (0x80306A98 ..) are two TUs; `menu_status_page_open`/`menu_status_page_step`
 *   (0x80308EC0..) read `menu/menu_infomation.cpp`'s `StatusScreenWork` and are likely that unit's head (seam
 *   request filed).
 * FLAGS. `cflags_main` with `#pragma peephole off`, `optimization_level 4`, `fp_contract off` and `pool_data off`
 *   for the whole file: retail keeps the unfused `clrlwi` narrowing and compares, `fmuls` + `fadds`, and each `.data`
 *   table its own `lis`/`addi` (each pragma measured).
 * NAMES. `eft042_set2` is the runtime dump's own name; `eft041_move`/`eft041_end`/`eft041_kill` are the registry's.
 *   GUESSes from the bodies, in the eft041 scheme (state 0 `model_create`, 1 `move`, 2 `end`, 3 `kill`; the
 *   `+0x40`/`+0x34` hooks `release`/`dispatch`): `eft_spawn_type_at_area`, `eft_area_slot_bind`,
 *   `eft042_slot_get`/`eft042_slot_get_typed` (the two record pools), `eft042_set_placed` (the caller-placed spawn),
 *   `eft042_release`/`eft042_release_kind0`/`eft042_release_models`, `eft042_dispatch`,
 *   `eft042_model_create_kind0..2` and `eft042_move_kind0..2` (the bodies `eft042_kind_tbl` selects),
 *   `eft042_move`, `eft042_end`, `eft042_kill`, `eft042_joint_show`.  Data GUESSes from their readers:
 *   `eft041_place_pos_tbl`, `eft042_kind_tbl`, `eft042_model_id_tbl`, `eft042_scale_tbl`, `eft042_timer_tbl`,
 *   `eft042_joint_tbl`, `eft042_offset_tbl`, `eft042_color_tbl`, `eft042_alpha_key_tbl`, the `getKeyData3` lists
 *   `eft042_scale_keys_type<N>`, the `getKeyData` lists `eft042_height_keys_type<N>`, `eft042_mat_color_keys`/
 *   `eft042_tev_color_keys` (type 17) and the kind-2 material keys `eft042_mat<0|1>_<rgb|alpha|scroll>_keys`.
 * RESIDUALS. Every row is written; 24 of 26 match.
 *  - `eft041_move`: retail keeps the constant 200 in a register for the `++timer > 200` compare (`cmpw r0,r23`, hoisted
 *    out of the loop) where ours compares the immediate; the callee-saved registers are numbered differently
 *    (a local `limit` and the reversed compare measure the same);
 *  - `eft042_dispatch`: retail gives the case-0 arm its own `b` into the nested kind switch; every spelling tried
 *    falls straight into it (docs/ef.md, "The case-0 nested dispatch"; the `return` form is the closest).
 *   flipcheck: `.data` and `.sdata` are emitted and byte-identical (the claim's last 4 `.data` bytes are the link's
 *   alignment gap); `.sdata2` is the eft042 pool only - the eft041 pool words 0x8079ADE0..0x8079ADF0 (1.0f for
 *   `ef/fn_803066F0.c`, 0.0f/20.0f/4.0f for `eft041_move`) belong to the other TU, which also leaves
 *   `lbl_8079ADE0` undefined for a flip; `.text` is 8 bytes short (the two rows above).
 *   `ef/eft019.cpp` still calls `eft042_set_placed` by its old `fn_` stem (landing respell).
 * SHAPES. A `u8` compared inside a `switch` keeps retail's `cmpwi` (an `if` gives `cmplwi`): the type tests in
 *   `eft_spawn_type_at_area`, `eft042_set2`, `eft042_end`, `eft042_move_kind0` and `eft042_move_kind2`.
 *   The kind-0 work area's `created` is `volatile` (retail stores the create result and reads it back) and the move
 *   bodies copy it to a local before the `update` call (one load).
 *   `eft042_release`: `kind` is `u32` and the release arms are written models-last, as retail lays them out.
 *   `eft042_release_kind0`: `i` is declared before `work` (the register order follows).
 *   `Eft042Work`'s unions name the per-family tails of the 64-byte `_EFT::work_0x38` block; `Eft042WorkKind0`/
 *   `Eft042WorkKind1` are the per-kind views.
 */
#pragma peephole off
#pragma optimization_level 4
#pragma fp_contract off
#pragma pool_data off
#include "types.h"
#include "fn_8004CAD8/mtx.h" /* the owner header (rule 2) */
#include "mh3_pad.h" /* the owner header (rule 2) */

#include "nw4r/math.h"
#include "gx.h"

#include "ef.h"
#include "unsplit/ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "stage/stg_w.h"
#include "stage/get_now_mapno.h"
#include "sound/fn_800DD1F0.h"
#include "menu/menu_message.h"
#include "pl.h"
#include "fn_8004CAD8.h"
#include "Pl/fn_8028F66C.h"
#include "g3d/g3d_calcworld.h"
#include "enemy/fn_8012BDF4.h"
#include "unsplit/enemy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "menu/menu_infomation.h"
#include "ef/eft004_set.h"
#include "rotLocalMatX.h"
#include "sound/fn_800D7F54.h"
#include "stage/get_stg_eft_col.h"
#include "enemy/fn_8012EC74.h"
#include "nw4r/g3d/scnmdl.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_calcmaterial.h"
#include "sound/copied_mat_access_valid_ck.h"
#include "ef/eft_stage_spawn_flag.h"
#include "Pl/Pl_master_ck.h"

/* The screen work record this range's tail reads (`menu_status_page_open`/`menu_status_page_step`) and the page count
 * they arm it from (`menu_equip_page_count`) live with their owner, `menu/menu_infomation.cpp`'s header: the
 * two screen bodies below the seam are all this unit needs from that unit.  The `EquipListWork`
 * record moved with `equip_list_page_count`, its only reader.
 */

/* One model record of the work area: the pooled model handle the spawner stores and the create
 * result the state-0 bodies file.  Two families disagree about the second word - the kind-2 bodies
 * (eft042_set_placed stores, eft042_model_create_kind2 reads) use it as the model's scale factor - so it is a union
 * (rule 5: each member names the site that uses it, and both keep the +0x04 offset).
 * size: 0x08 */
typedef struct Eft042Model {
    MHchar* model;      /* +0x00  `eft_res_model_get`'s pooled model handle */
    union {
        void* created;  /* +0x04  the `res_eft_UV_model_create*` result (effect 41, kind 0) */
        f32   scale;    /* +0x04  the model's uniform scale (kind-2 bodies) */
    } v_0x04;
} Eft042Model;

/* The work area.  `count`/`models` are the head every family shares; the 0x10 bytes at +0x0C are the
 * four `_g3d_work` handles for effect 41 and the kind-0 bodies (which use the last two), a placement
 * offset plus an angle for effect 42's kind-1 body.  size: 0x20 */
typedef struct Eft042Work {
    u32         count;       /* +0x00  the model count `fn_800F8B44`'s record was taken for */
    Eft042Model models[1];   /* +0x04 */
    union {
        /* effect 41 (`ef/fn_803066F0.c`'s `EftWorkData::works`) and the kind-0 bodies, which use
         * only the last two slots (`eft042_set`/`eft042_slot_get_typed` clear them, `eft042_release_kind0` releases
         * them, `eft042_model_create_kind0` fills them). */
        void* works[4];      /* +0x0C */
        struct {             /* size: 0x10 */
            nw4r::math::VEC3 pos;  /* +0x0C  the model's placement offset (kind 1) */
            f32              angle;/* +0x18  its yaw (kind 1) */
        } kind1;
    } v_0x0C;
    void*       resource;    /* +0x1C  the area resource `lb_area_fx_set` stores */
} Eft042Work;

/* The kind-1 work area's own view (the kind-1 types' state bodies): the placement offset and its
 * scale sit in the same 0x10 bytes the effect-41 view calls `works`, and the model's material colour
 * is what the type-7 state-2 body walks.
 * size: 0x1C (the extent the kind-1 bodies prove) */
typedef struct Eft042WorkKind1 {
    u32             count;        /* +0x00 */
    MHchar*         model;        /* +0x04 */
    u8              pad_0x08[0x4];/* +0x08 */
    union {
        nw4r::math::VEC3 offset;  /* +0x0C  the placement offset (kind-1 state-0/1 bodies) */
        _GXColor          color;  /* +0x0C  the material colour (the type-7 state-2 body) */
    } v_0x0C;
    f32             field_0x18;   /* +0x18  the offset's own scale */
} Eft042WorkKind1;

/* The kind-1 state-1 body's view of its source record: the height the falling model is retired
 * below (GUESS: a floor height; `_ENEMY_WORK` declares a pointer at +0x60, so the record type is not
 * settled).  size: 0x64 - a lower bound, an approximation */
typedef struct Eft042Kind1Source {
    u8              pad_0x00[0x60];  /* +0x00 */
    f32             floor_y_0x60;    /* +0x60 */
} Eft042Kind1Source;

/* The kind-0 work area's own view (the kind-0 types' state bodies): the pooled model, the UV model the
 * state-0 body creates over it (retail stores the create result and reads it back), the TEV colour the
 * state bodies feed the model, its scale and the two `_g3d_work` handles the create call fills.
 * size: 0x1C */
typedef struct Eft042WorkKind0 {
    s32                         count;       /* +0x00 */
    MHchar*                     models[1];   /* +0x04 */
    EftUvModelObject* volatile  created;     /* +0x08 */
    _GXColor                    color;       /* +0x0C */
    f32                         scale;       /* +0x10 */
    struct _g3d_work*           works[2];    /* +0x14 */
} Eft042WorkKind0;

/* The `+0x13C` word of the source record as the kind-1 body uses it: a pointer to a record whose
 * four-byte header is followed by the model itself (`eft042_model_create_kind1` calls `get_joint_wpos` on
 * `+0x04` of it).  size: 0x04 + the embedded `MHchar` - a lower bound, an approximation */
typedef struct EnemyModelRef {
    u32     pad_0x00;   /* +0x00 */
    MHchar  model;      /* +0x04 */
} EnemyModelRef;

/* ---------------------------------------------------------------------------------------------------
 * the unit's own data (.data 0x805DC5E8-0x805DCC50, .sdata 0x80792B78-0x80792BB8), in retail's order; the jump
 * tables follow them
 * ------------------------------------------------------------------------------------------------- */

/* the stage-2/13 area-10 placements `eft041_move` picks from */
Vec eft041_place_pos_tbl[4] = {
    {98.0f, -105.0f, -500.0f}, {-950.0f, -30.0f, 1400.0f},
    {-3040.0f, -65.0f, -85.0f}, {-4900.0f, 0.0f, -1000.0f},
};

/* per-frame colours (type 0) */
_GXColor eft042_color_seq_type0[6] = {
    {0xF2, 0xFE, 0xFF, 0xFF}, {0x8E, 0xCE, 0xEF, 0xFF},
    {0x39, 0x72, 0xB0, 0xFF}, {0x21, 0x45, 0x85, 0xFF},
    {0x12, 0x18, 0x3A, 0xFF}, {0x00, 0x00, 0x00, 0x00},
};

/* per-frame colours (type 1) */
_GXColor eft042_color_seq_type1[6] = {
    {0xF2, 0xFE, 0xFF, 0xFF}, {0x8E, 0xCE, 0xEF, 0xFF},
    {0x39, 0x72, 0xB0, 0xFF}, {0x21, 0x45, 0x85, 0xFF},
    {0x12, 0x18, 0x3A, 0xFF}, {0x00, 0x00, 0x00, 0x00},
};

/* type 3's colour */
_GXColor eft042_color_type3 = {0xA0, 0x91, 0x75, 0x20};

/* type 4's colour */
_GXColor eft042_color_type4 = {0xBE, 0xA0, 0x82, 0x20};

/* per-frame colours (type 5) */
_GXColor eft042_color_seq_type5[4] = {
    {0xF2, 0xFE, 0xFF, 0xFF}, {0x39, 0x72, 0xB0, 0xFF},
    {0x12, 0x18, 0x3A, 0xFF}, {0x00, 0x00, 0x00, 0x00},
};

/* type 7's RGB key list (frame, r, g, b) */
u8 eft042_rgb_keys_type7[16] = {
    0x00, 0x8E, 0xCE, 0xEF, 0x0F, 0x6E, 0xA0, 0xAA,
    0x1F, 0x8E, 0xCE, 0xEF, 0xFF, 0x8E, 0xCE, 0xEF,
};

/* type 16's colour */
_GXColor eft042_color_type16[2] = {
    {0x64, 0x58, 0x4D, 0xFF}, {0x00, 0x00, 0x00, 0x00},
};

/* per-type colour sequence (type 7: its RGB key list) */
_GXColor* eft042_color_tbl[19] = {
    eft042_color_seq_type0, eft042_color_seq_type1, NULL, &eft042_color_type3,
    &eft042_color_type4, eft042_color_seq_type5, NULL, (_GXColor*)eft042_rgb_keys_type7,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    eft042_color_type16, NULL, NULL,
};

/* alpha key lists (frame, alpha) */
u8 eft042_alpha_keys_type3[12] = {
    0x00, 0x20, 0x04, 0xB4, 0x06, 0x87, 0x0A, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};

u8 eft042_alpha_keys_type4[12] = {
    0x00, 0x20, 0x02, 0xB4, 0x06, 0x87, 0x0A, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};

u8 eft042_alpha_keys_type7[8] = {
    0x00, 0xC8, 0x0F, 0xA0, 0x1F, 0xC8, 0xFF, 0xC8,
};

u8 eft042_alpha_keys_type8[8] = {
    0x00, 0xFF, 0x0C, 0xFF, 0x12, 0x00, 0xFF, 0x00,
};

/* (types 9 and 13) */
u8 eft042_alpha_keys_type9[8] = {
    0x00, 0xFF, 0x0F, 0xFF, 0x23, 0x00, 0xFF, 0x00,
};

u8 eft042_alpha_keys_type14[12] = {
    0x00, 0x00, 0x02, 0xFF, 0x03, 0xFF, 0x06, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};

u8 eft042_alpha_keys_type18[8] = {
    0x00, 0x20, 0x02, 0x88, 0x04, 0x00, 0xFF, 0x00,
};

/* per-type alpha key list (NULL: no fade) */
u8* eft042_alpha_key_tbl[19] = {
    NULL, NULL, NULL, eft042_alpha_keys_type3,
    eft042_alpha_keys_type4, NULL, NULL, eft042_alpha_keys_type7,
    eft042_alpha_keys_type8, eft042_alpha_keys_type9, NULL, NULL,
    NULL, eft042_alpha_keys_type9, eft042_alpha_keys_type14, NULL,
    NULL, NULL, eft042_alpha_keys_type18,
};

/* per-type model/animation id */
u16 eft042_model_id_tbl[19] = {
    0x0022, 0x0035, 0x003A, 0x003F, 0x003F, 0x0035, 0x0044, 0x0038,
    0x0084, 0x008F, 0x0092, 0x0052, 0x0052, 0x008F, 0x009D, 0x00A1,
    0x0036, 0x0092, 0x0023,
};

/* per-type base scale of the kind-0 UV models */
f32 eft042_scale_tbl[19] = {
    1.8f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

/* per-type step-timer length */
u32 eft042_timer_tbl[19] = {
    5, 5, 0, 12, 12, 3, 48, 50,
    18, 35, 5, 10, 10, 35, 6, 52,
    110, 5, 4,
};

/* per-type kind: 0/1/2 select the state bodies */
u8 eft042_kind_tbl[19] = {
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
};

/* `getKeyData3` scale key lists (frame, x, y, z; -1 ends) */
f32 eft042_scale_keys_type3[24] = {
    0.0f, 0.7f, 0.5f, 0.3f,
    4.0f, 1.15f, 2.0f, 0.8f,
    6.0f, 1.35f, 1.8f, 1.0f,
    10.0f, 1.7f, 1.2f, 1.4f,
    12.0f, 1.8f, 0.9f, 1.5f,
    -1.0f, 1.8f, 0.9f, 1.5f,
};

f32 eft042_scale_keys_type8[20] = {
    0.0f, 0.7f, 0.0f, 0.2f,
    3.0f, 0.76f, 0.6f, 0.6f,
    9.0f, 0.9f, 1.0f, 0.9f,
    18.0f, 1.1f, 1.2f, 1.2f,
    -1.0f, 1.1f, 1.2f, 1.2f,
};

f32 eft042_scale_keys_type14[12] = {
    0.0f, 0.1f, 0.1f, 0.1f,
    6.0f, 2.0f, 2.0f, 2.0f,
    -1.0f, 2.0f, 2.0f, 2.0f,
};

f32 eft042_scale_keys_type15[16] = {
    0.0f, 0.5f, 0.3f, 0.5f,
    20.0f, 1.0f, 1.0f, 1.0f,
    52.0f, 1.2f, 0.3f, 1.3f,
    -1.0f, 1.2f, 0.3f, 1.3f,
};

f32 eft042_scale_keys_type18[12] = {
    0.0f, 0.7f, 1.6f, 0.7f,
    4.0f, 1.4f, 3.0f, 1.4f,
    -1.0f, 1.4f, 3.0f, 1.4f,
};

/* `getKeyData` height key lists (frame, value; -1 ends) */
f32 eft042_height_keys_type8[10] = {
    0.0f, -100.0f, 3.0f, 5.0f,
    14.0f, 5.0f, 18.0f, 0.0f,
    -1.0f, 0.0f,
};

f32 eft042_height_keys_type15[10] = {
    0.0f, -200.0f, 20.0f, 0.0f,
    26.0f, 0.0f, 52.0f, -100.0f,
    -1.0f, -100.0f,
};

/* the kind-2 materials' RGB/alpha keys and texture-scroll keys */
u8 eft042_mat0_rgb_keys[16] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x03, 0xFF, 0xFF, 0xFF,
    0x04, 0x50, 0x50, 0xFF, 0xFF, 0x50, 0x50, 0xFF,
};

u8 eft042_mat1_rgb_keys[20] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x06, 0xFF, 0xFF, 0xFF,
    0x08, 0x50, 0x50, 0xFF, 0x0A, 0x50, 0x50, 0xFF,
    0xFF, 0x50, 0x50, 0xFF,
};

/* type 17's material-colour key list (frame, r, g, b) */
u8 eft042_mat_color_keys[20] = {
    0x00, 0x9B, 0xB0, 0xFF, 0x02, 0x45, 0x55, 0xAA,
    0x04, 0x24, 0x32, 0x79, 0x05, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};

/* type 17's TEV-colour key list (frame, r, g, b) */
u8 eft042_tev_color_keys[20] = {
    0x00, 0x7B, 0xA1, 0xFF, 0x02, 0x30, 0x38, 0xBA,
    0x04, 0x08, 0x0F, 0x69, 0x05, 0x00, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0x00,
};

u8 eft042_mat0_alpha_keys[12] = {
    0x00, 0xFF, 0x03, 0xFF, 0x04, 0x61, 0x06, 0x10,
    0xFF, 0x00, 0x00, 0x00,
};

u8 eft042_mat1_alpha_keys[16] = {
    0x00, 0xFF, 0x06, 0xFF, 0x08, 0x4C, 0x0A, 0x10,
    0xFF, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

f32 eft042_mat0_scroll_keys[6] = {
    0.0f, 0.6f, 6.0f, 0.0f,
    -1.0f, 0.0f,
};

f32 eft042_mat1_scroll_keys[8] = {
    0.0f, 0.5f, 2.0f, 0.5f,
    9.0f, 0.0f, -1.0f, 0.0f,
};

u8 eft042_mat0_rgb_keys_type12[8] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

u8 eft042_mat1_rgb_keys_type12[8] = {
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

/* per-type placement offset from the source joint */
Vec eft042_offset_tbl[19] = {
    {0.0f, 150.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 200.0f},
    {0.0f, 0.0f, 200.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, -20.0f, 20.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 3850.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f}, {500.0f, 0.0f, 0.0f},
    {0.0f, 20.0f, 50.0f}, {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 250.0f},
};

/* per-type source joint */
u32 eft042_joint_tbl[19] = {
    3, 3, 7, 20, 20, 3, 26, 3,
    3, 25, 3, 255, 255, 25, 255, 3,
    21, 3, 255,
};

/* ---------------------------------------------------------------------------------------------------
 * this unit's own bodies, forward-declared (address order)
 * ------------------------------------------------------------------------------------------------- */

extern "C" void eft041_move(_EFT* self);
extern "C" void eft041_end(_EFT* self);
extern "C" void eft041_kill(_EFT* self);
extern "C" void eft_spawn_type_at_area(_ENEMY_WORK* source, u8 type);
extern "C" void eft_area_slot_bind(_ENEMY_WORK* source, u8 type, s32 timer);
extern "C" _EFT* eft042_slot_get(u8 type, u8 area);
extern "C" void eft042_set(MHchar* model, u8 type);
extern "C" void eft042_set_placed(void* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale,
                            u8 area);
extern "C" _EFT* eft042_slot_get_typed(u8 type, u8 area);
extern "C" void eft042_release(_EFT* self);
extern "C" void eft042_release_kind0(_EFT* self);
extern "C" void eft042_release_models(_EFT* self);
extern "C" void eft042_dispatch(_EFT* self);
extern "C" void eft042_model_create_kind0(_EFT* self);
extern "C" void eft042_model_create_kind1(_EFT* self);
extern "C" void eft042_model_create_kind2(_EFT* self);
extern "C" void eft042_move(_EFT* self);
extern "C" void eft042_move_kind0(_EFT* self);
extern "C" void eft042_move_kind1(_EFT* self);
extern "C" void eft042_move_kind2(_EFT* self);
extern "C" void eft042_end(_EFT* self);
extern "C" void eft042_kill(_EFT* self);
extern "C" void eft042_joint_show(_EFT* self, u32 visible, u8 from, u8 to);
extern "C" void menu_status_page_open(StatusScreenWork* self);
extern "C" s32  menu_status_page_step(StatusScreenWork* self);


/* `eft_res_slot_get`'s working area (64 bytes per record) and the model pool are the resource manager's;
 * the two entry points here are the ones this unit pools from. */

/* ---------------------------------------------------------------------------------------------------
 * 0x8030681C - the effect-41 machine's state-1 step
 * ------------------------------------------------------------------------------------------------- */

/* Walks the work area's model records: places each model when its step timer expires, reveals its joints, walks it
 * and retires it after the second timer, then hands its position to `eft004_set`. */
extern "C" void eft041_move(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    u8 mapno = get_now_mapno();
    nw4r::math::VEC3 pos;
    Vec* pos_tbl;
    u32 pos_num;
    u32 i;

    VEC3_ctor(&pos);
    pos_tbl = NULL;
    if ((mapno == 2 || mapno == 13) && self->area_0x44 == 10) {
        pos_tbl = eft041_place_pos_tbl;
        pos_num = 4;
    }

    for (i = 0; i < work->count; i++) {
        switch (self->field_0x06) {
        case 0:
            if (--self->timer_0x0C < 0) {
                u8 idx;
                u32 joint;
                u32 joints;

                self->field_0x06++;
                ((EftUvModelObject*)work->models[i].v_0x04.created)->set_frame(0.0f);

                idx = ran_suu(1) % pos_num;
                vec_to_mh_vec3(&work->models[i].model->pos_0x04, &pos_tbl[idx]);

                joints = work->models[i].model->get_joint_num();
                for (joint = 0; joint < joints; joint++) {
                    if (joint == ((u8*)work->resource)[idx]) {
                        work->models[i].model->setVisibility(joint, true);
                    } else {
                        work->models[i].model->setVisibility(joint, false);
                    }
                }
            }
            break;
        case 1:
            if (++self->timer_0x0C > 200) {
                self->field_0x06 = 0;
                self->timer_0x0C = ran_suu(1) & 0x1F;
            } else {
                work->models[i].model->move(0);
                ((EftUvModelObject*)work->models[i].v_0x04.created)->update();
                eft_res_models_spawn(self, (void**)&work->models[i], 2, 1, NULL);
            }
            break;
        }

        switch (200 - self->timer_0x0C) {
        case 15:
        case 20:
        case 25:
        case 30:
        case 35:
            se_bank46_id13_req(&work->models[i].model->pos_0x04);
            break;
        }

        copyVec3(&pos, &work->models[i].model->pos_0x04);
        pos.y -= 20.0f;
        eft004_set(0, &pos, 4.0f, 0, self->area_0x44);
    }
}

/* The machine's state-2 step: one step further into the effect. */
extern "C" void eft041_end(_EFT* self)
{
    self->state_0x05++;
}

/* The machine's state-3 step (and every failure path): retire the effect. */
extern "C" void eft041_kill(_EFT* self)
{
    eft_res_slot_release(self);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80306A98, 0x80306B10, 0x80306B60 - the effect-42 spawn entries
 * ------------------------------------------------------------------------------------------------- */

/* Spawns effect 42 for `source` (type 3 becomes type 4 when the source's +0x228 bits 1-2 are set) and files it. */
extern "C" void eft_spawn_type_at_area(_ENEMY_WORK* source, u8 type)
{
    _EFT* eft = eft042_slot_get(type, source->area_no);

    if (eft != NULL) {
        switch (type) {
        case 3:
            if ((source->field_0x228 & 6) != 0) {
                eft->type_0x02 = 4;
            }
            break;
        }
        eft->source_0x30 = source;
        eft->timer_0x0C = 0;
    }
}

/* The same spawn, but with the step timer the caller picks (the state-0 body's own timer). */
extern "C" void eft_area_slot_bind(_ENEMY_WORK* source, u8 type, s32 timer)
{
    _EFT* eft = eft042_slot_get(type, source->area_no);

    if (eft != NULL) {
        eft->source_0x30 = source;
        eft->timer_0x0C = timer;
    }
}

/* Takes a work block from the effect pool, fills its model handle and places the effect from the source record. */
void eft042_set2(_ENEMY_WORK* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot)
{
    _EFT* eft = eft042_slot_get(type, source->area_no);

    if (eft != NULL) {
        switch (type) {
        case 3:
            if ((source->field_0x228 & 6) != 0) {
                eft->type_0x02 = 4;
            }
            break;
        }
        copyVec3(&eft->pos_0x18, pos);
        eft_rot_vec_copy(&eft->rot_0x24, rot);
        eft->source_0x30 = source;
        eft->timer_0x0C = 0;
    }
}

/* Pools an effect-42 record and its work area, checks the model handle and stamps the record and its hooks;
 * returns NULL outside the current area or when a pool is exhausted. */
extern "C" _EFT* eft042_slot_get(u8 type, u8 area)
{
    _EFT* eft;
    Eft042WorkKind0* work;
    s32 i;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)eft_res_slot_get(28);
    if (eft == NULL) {
        return NULL;
    }

    eft->type_0x02 = type;
    eft->release_0x40 = eft042_release;
    eft->dispatch_0x34 = eft042_dispatch;

    work = (Eft042WorkKind0*)eft->work_0x38;
    work->count = 1;
    memset(work->works, 0, 8);

    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 42;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft->field_0x10 = 0;
    eft_state_flags_set(eft, 0, 0);

    return eft;
}

/* Spawns effect 42 from a model: the record is placed at the model's rotation and the model is filed
 * as the effect's origin. */
extern "C" void eft042_set(MHchar* model, u8 type)
{
    _EFT* eft = eft042_slot_get_typed(type, model->area_0x16);

    if (eft != NULL) {
        eft_rot_vec_copy(&eft->rot_0x24, &model->rot_0x54);
        eft->source_0x30 = model;
    }
}

/* The same spawn, placed by the caller: the position, the rotation, the model scale kept in the work
 * area's second word, and the origin record. */
extern "C" void eft042_set_placed(void* source, u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale,
                            u8 area)
{
    _EFT* eft = eft042_slot_get_typed(type, area);

    if (eft != NULL) {
        copyVec3(&eft->pos_0x18, pos);
        eft_rot_vec_copy(&eft->rot_0x24, rot);
        eft->source_0x30 = NULL;
        eft->source_0x30 = source;
        ((Eft042Work*)eft->work_0x38)->models[0].v_0x04.scale = scale;
    }
}

/* Pools an effect-42 record like `eft042_slot_get`, stamping the requested type and the `eft_state_flags_set` mask 4. */
extern "C" _EFT* eft042_slot_get_typed(u8 type, u8 area)
{
    _EFT* eft;
    Eft042WorkKind0* work;
    s32 i;

    if (area != get_now_areano()) {
        return NULL;
    }
    eft = (_EFT*)eft_res_slot_get(28);
    if (eft == NULL) {
        return NULL;
    }

    work = (Eft042WorkKind0*)eft->work_0x38;
    work->count = 1;

    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }

    eft->field_0x03 = 42;
    eft->type_0x02 = type;
    eft->field_0x04 = 0;
    eft->area_0x44 = area;
    eft->flag_0x01 = 1;
    eft->timer_0x0C = 0;
    eft_state_flags_set(eft, 0, 4);

    eft->release_0x40 = eft042_release;
    eft->dispatch_0x34 = eft042_dispatch;

    return eft;
}

/* ---------------------------------------------------------------------------------------------------
 * the work area's release paths (the record's `release_0x40` hook and its two arms)
 * ------------------------------------------------------------------------------------------------- */

/* Destroys the record by kind: kinds 1 and 2 release the model handles, kind 0 also the `_g3d_work` handles. */
extern "C" void eft042_release(_EFT* self)
{
    u32 kind = eft042_kind_tbl[self->type_0x02];

    if (kind - 1 > 1) {
        if (kind == 0) {
            eft042_release_kind0(self);
        }
    } else {
        eft042_release_models(self);
    }
}

/* The kind-0 release: give the two `_g3d_work` handles the state-0 body took back, then release the
 * model handles and empty the work area. */
extern "C" void eft042_release_kind0(_EFT* self)
{
    s32 i;
    Eft042WorkKind0* work = (Eft042WorkKind0*)self->work_0x38;

    for (i = 1; i >= 0; i--) {
        if (work->works[i] != NULL) {
            push_g3d_wk(work->works[i]);
        }
    }
    eft_res_models_release(work->models, work->count);
    work->count = 0;
}

/* The kinds-1/2 release: the model handles only. */
extern "C" void eft042_release_models(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;

    eft_res_models_release(&work->models[0].model, work->count);
    work->count = 0;
}

/* The record's update hook: the outer machine is the state byte (`_EFT::state_0x05`), and its first
 * state is the per-type machine the kind table selects. */
extern "C" void eft042_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        eft042_move(self);
        return;
    case 2:
        eft042_end(self);
        return;
    case 3:
        eft042_kill(self);
        return;
    default:
        return;
    }
    switch (eft042_kind_tbl[self->type_0x02]) {
    case 0:
        eft042_model_create_kind0(self);
        break;
    case 1:
        eft042_model_create_kind1(self);
        break;
    case 2:
        eft042_model_create_kind2(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80307068 - the kind-0 state-0 body
 * ------------------------------------------------------------------------------------------------- */

/* Kind 0, state 0: create the type's model (a UV model for the colour-cycled types), place, turn and
 * colour it per type, arm the step timer from the type table and run the state-1 dispatch. */
extern "C" void eft042_model_create_kind0(_EFT* self)
{
    Eft042WorkKind0* work = (Eft042WorkKind0*)self->work_0x38;
    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;
    nw4r::math::VEC3 unused;
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    _GXColor key;
    _GXColor* color;
    u32 col;
    s32 i;

    VEC3_ctor(&unused);
    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);
    self->state_0x05++;

    switch (self->type_0x02) {
    case 0:
        work->created = (EftUvModelObject*)res_eft_UV_model_create(
            work->models[0], eft042_model_id_tbl[self->type_0x02], 280, 0, work->works, 1, 0);
        if (work->created == NULL) {
            eft042_kill(self);
            return;
        }
        work->scale = eft042_scale_tbl[self->type_0x02] + 0.1f * (f32)(ran_suu(0) & 3);
        self->rot_0x24.x = source->field_0x1BC;
        self->rot_0x24.y = source->field_0x1C0;
        self->rot_0x24.z = source->field_0x1C4 + ran_suu(0);
        setVector3(&work->models[0]->scale_0x1C, work->scale, work->scale, work->scale);
        break;
    case 1:
    case 5:
    case 10:
    case 17:
        work->created = (EftUvModelObject*)res_eft_UV_model_create(
            work->models[0], eft042_model_id_tbl[self->type_0x02], 348, 0, work->works, 1, 0);
        if (work->created == NULL) {
            eft042_kill(self);
            return;
        }
        switch (self->type_0x02) {
        case 1:
        case 5:
            for (i = 1; i <= 4; i++) {
                if (i == 1 || i == 4) {
                    work->models[0]->setVisibility(i, false);
                } else {
                    work->models[0]->setVisibility(i, true);
                }
            }
            break;
        }
        work->scale = eft042_scale_tbl[self->type_0x02] + 0.1f * (f32)(ran_suu(0) & 3);
        self->rot_0x24.x = 0;
        self->rot_0x24.y = 0;
        self->rot_0x24.z = ran_suu(0);
        setVector3(&work->models[0]->scale_0x1C, work->scale, work->scale, work->scale);
        switch (self->type_0x02) {
        case 10:
            work->color.r = 255;
            work->color.g = 0;
            work->color.b = 0;
            work->color.a = 255;
            for (i = 0; i < 2; i++) {
                work->models[0]->setTevKColor(i, GX_KCOLOR3, &work->color);
            }
            break;
        case 17:
            key.r = eft042_mat_color_keys[1];
            key.g = eft042_mat_color_keys[2];
            key.b = eft042_mat_color_keys[3];
            key.a = 255;
            work->color.r = eft042_tev_color_keys[1];
            work->color.g = eft042_tev_color_keys[2];
            work->color.b = eft042_tev_color_keys[3];
            work->color.a = 255;
            for (i = 0; i < 2; i++) {
                work->models[0]->setMatAlphaBlendMode(i, GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
                work->models[0]->setTevKColor(i, GX_KCOLOR3, &work->color);
                work->models[0]->setMatColor(i, GX_COLOR0A0, key, false);
            }
            break;
        }
        break;
    case 3:
    case 4:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 16) == NULL) {
            eft042_kill(self);
            return;
        }
        if (em_work_die_ck(source)) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        self->rot_0x24.x = 0;
        self->rot_0x24.y = source->field_0x1C0;
        self->rot_0x24.z = source->field_0x1C4 + (u16)(0.5f + 65536.0f * (f32)(ran_suu(0) & 0x1F) / 360.0f) - 2731;
        work->created = NULL;
        get_joint_wpos_em(source, eft042_joint_tbl[self->type_0x02], &self->pos_0x18);
        mtx34_identity(&mtx);
        rotMatrixY(self->rot_0x24.y, &mtx);
        rotLocalMatZ(source->field_0x1C4, &mtx);
        vec_to_mh_vec3(&offset, &eft042_offset_tbl[self->type_0x02]);
        mulVecMat(&offset, &mtx);
        addVec3To(&self->pos_0x18, &offset);
        switch (self->type_0x02) {
        case 3:
            col = get_stg_eft_col(self->area_0x44, 0);
            work->color.r = (col & 0xFF000000) >> 24;
            work->color.g = (col & 0xFF0000) >> 16;
            work->color.b = (col & 0xFF00) >> 8;
            work->color.a = 255;
            self->pos_0x18.y = 5.0f + source->field_0x20C;
            self->color_flags_0x14 = 1;
            self->field_0x15 = 100;
            self->field_0x16 = 0;
            self->field_0x17 = 1;
            break;
        case 4:
            col = get_stg_eft_col(self->area_0x44, 1);
            work->color.r = (col & 0xFF000000) >> 24;
            work->color.g = (col & 0xFF0000) >> 16;
            work->color.b = (col & 0xFF00) >> 8;
            work->color.a = 255;
            self->pos_0x18.y = 5.0f + source->field_0x210;
            self->color_flags_0x14 = 129;
            self->field_0x15 = 100;
            self->field_0x16 = 0;
            self->field_0x17 = 1;
            break;
        }
        copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
        work->models[0]->field_0x2C = self->rot_0x24.y;
        break;
    case 6:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 24) == NULL) {
            eft042_kill(self);
            return;
        }
        for (i = 1; i <= 12; i++) {
            if (i == 1) {
                work->models[0]->setVisibility(i, true);
            } else {
                work->models[0]->setVisibility(i, false);
            }
        }
        self->rot_0x24.x = 0xE000;
        self->rot_0x24.y = 0x8000;
        self->rot_0x24.z = 0x4000;
        work->created = NULL;
        setVector3(&work->models[0]->scale_0x1C, 1.8f, 1.8f, 1.8f);
        break;
    case 7:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 4) == NULL) {
            eft042_kill(self);
            return;
        }
        get_joint_wpos_em(source, eft042_joint_tbl[self->type_0x02], &self->pos_0x18);
        se_req_pos_ps(source->se_0xB14, 21, 2, &self->pos_0x18);
        work->created = NULL;
        break;
    case 15:
        work->color.a = 255;
    case 8:
        work->created = (EftUvModelObject*)res_eft_UV_model_create(
            work->models[0], eft042_model_id_tbl[self->type_0x02], 280, 0, work->works, 1, 0);
        if (work->created == NULL) {
            eft042_kill(self);
            return;
        }
        col = get_stg_eft_col(self->area_0x44, 0);
        work->color.r = (col & 0xFF000000) >> 24;
        work->color.g = (col & 0xFF0000) >> 16;
        work->color.b = (col & 0xFF00) >> 8;
        work->color.a = 255;
        break;
    case 9:
    case 13:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 24) == NULL) {
            eft042_kill(self);
            return;
        }
        self->field_0x07 = 1;
        eft042_joint_show(self, 1, 1, 5);
        get_joint_wpos_em(source, eft042_joint_tbl[self->type_0x02], &self->pos_0x18);
        work->models[0]->getTevKColor(0, GX_KCOLOR3, &work->color);
        eft_rot_vec_copy(&self->rot_0x24, (_CP_VECTOR*)&source->field_0x1BC);
        switch (self->type_0x02) {
        case 9:
            self->pos_0x18.y = 5.0f + source->field_0x20C;
            break;
        case 13:
            if (source->field_0x23C != 0) {
                self->pos_0x18.y = source->ceiling_0x240 - 5.0f;
            }
            self->rot_0x24.x += 0x8000;
            break;
        }
        eft_rot_vec_copy(&work->models[0]->rot_0x28, &self->rot_0x24);
        copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
        work->created = NULL;
        break;
    case 14:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 16) == NULL) {
            eft042_kill(self);
            return;
        }
        eft_rot_vec_copy(&work->models[0]->rot_0x28, &self->rot_0x24);
        copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
        work->color.r = 255;
        work->color.g = 255;
        work->color.b = 255;
        work->color.a = 0;
        work->created = NULL;
        break;
    case 16:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 24) == NULL) {
            eft042_kill(self);
            return;
        }
        work->models[0]->setVisibility(1, false);
        work->models[0]->setVisibility(2, true);
        self->rot_0x24.x = 10923;
        self->rot_0x24.y = 0;
        self->rot_0x24.z = 0;
        work->created = NULL;
        setVector3(&work->models[0]->scale_0x1C, 5.0f, 5.0f, 5.0f);
        color = eft042_color_tbl[self->type_0x02];
        work->color.r = color->r;
        work->color.g = color->g;
        work->color.b = color->b;
        work->color.a = color->a;
        break;
    case 18:
        if (res_eft_model_create(work->models[0], eft042_model_id_tbl[self->type_0x02], 16) == NULL) {
            eft042_kill(self);
            return;
        }
        self->rot_0x24.x += 0xC000;
        eft_rot_vec_copy(&work->models[0]->rot_0x28, &self->rot_0x24);
        copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
        work->color.r = 221;
        work->color.g = 175;
        work->color.b = 137;
        work->color.a = 0;
        work->created = NULL;
        break;
    }

    if (self->timer_0x0C == 0) {
        self->timer_0x0C = eft042_timer_tbl[self->type_0x02];
    }
    eft042_move(self);
}

/* Kind 1, state 0: bind the type's animation to the pooled model, place the effect on the source
 * model's joint 7, and give the placement offset its angle, scale and start size. */
extern "C" void eft042_model_create_kind1(_EFT* self)
{
    Eft042WorkKind1* work = (Eft042WorkKind1*)self->work_0x38;
    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;
    nw4r::math::VEC3 offset;

    VEC3_ctor(&offset);
    self->state_0x05++;

    if (res_eft_model_create(work->model, eft042_model_id_tbl[self->type_0x02], 0) == NULL) {
        eft042_kill(self);
        return;
    }
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }

    ((EnemyModelRef*)source->field_0x13C)->model.get_joint_wpos(7, &self->pos_0x18);

    setVector3(&offset, -5.0f, 10.0f, 0.0f);
    rotVecY(&offset, self->rot_0x24.y);
    copyVec3(&work->v_0x0C.offset, &offset);
    work->field_0x18 = 2.5f;

    if ((u32)eft_water_state_ck(self) == 1) {
        vec3_scale_in_place(&work->v_0x0C.offset, 0.5f);
        work->field_0x18 *= 0.25f;
    }

    setVector3(&work->model->scale_0x1C, 1.0f, 1.0f, 1.0f);
    self->timer_0x0C = 40;
    work->model->field_0x28 = 0;
    work->model->field_0x2C = self->rot_0x24.y;
    work->model->field_0x30 = 0;
    eft042_move(self);
}

/* Binds the type's animation to the pooled model, places, spins and scales it, and sets the colour types' blend
 * and TEV colours (kind 2, state 0). */
extern "C" void eft042_model_create_kind2(_EFT* self)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    _GXColor color;
    s32 i;

    self->state_0x05++;

    if (res_eft_model_create(work->models[0].model, eft042_model_id_tbl[self->type_0x02], 340) == NULL) {
        eft042_kill(self);
        return;
    }

    copyVec3(&work->models[0].model->pos_0x04, &self->pos_0x18);
    eft_rot_vec_copy((_CP_VECTOR*)&work->models[0].model->field_0x28, &self->rot_0x24);
    work->models[0].model->field_0x2C += 16384;
    work->models[0].model->field_0x28 += ran_suu(0);
    work->models[0].model->setVisibility(1, false);
    setVector3(&work->models[0].model->scale_0x1C, work->models[0].v_0x04.scale, work->models[0].v_0x04.scale,
               work->models[0].v_0x04.scale);

    self->flag_0x01 = 1;
    self->timer_0x0C = eft042_timer_tbl[self->type_0x02];

    switch (self->type_0x02) {
    case 11:
        color.r = 102;
        color.g = 238;
        color.b = 255;
        color.a = 255;
        break;
    case 12:
        color.r = 255;
        color.g = 0;
        color.b = 0;
        color.a = 255;
        for (i = 0; i < 2; i++) {
            work->models[0].model->setMatAlphaBlendMode(i, GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
                                        GX_LO_CLEAR);
        }
        break;
    }

    for (i = 0; i < 2; i++) {
        work->models[0].model->setTevKColor(i, GX_KCOLOR3, &color);
    }

    eft042_move(self);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80307E08..0x80308A30 - the second state's per-kind bodies
 * ------------------------------------------------------------------------------------------------- */

/* The state-1 dispatch: the same kind table as the state-0 machine, one body per kind. */
extern "C" void eft042_move(_EFT* self)
{
    switch (eft042_kind_tbl[self->type_0x02]) {
    case 0:
        eft042_move_kind0(self);
        break;
    case 1:
        eft042_move_kind1(self);
        break;
    case 2:
        eft042_move_kind2(self);
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80307E44 - the kind-0 state-1 body
 * ------------------------------------------------------------------------------------------------- */

/* Kind 0, state 1: step the type's colour keys, count the step timer down (type 7 then fades out in
 * state 2), and follow the source model's joint with the model's matrix, scale keys and UV step. */
extern "C" void eft042_move_kind0(_EFT* self)
{
    nw4r::math::VEC3 offset;
    nw4r::math::MTX34 mtx;
    Eft042WorkKind0* work;
    _ENEMY_WORK* source;
    EftUvModelObject* created;
    _GXColor key;
    _GXColor* color;
    u8* alpha;
    s32 i;

    VEC3_ctor(&offset);
    MTX34_ctor(&mtx);
    work = (Eft042WorkKind0*)self->work_0x38;
    source = (_ENEMY_WORK*)self->source_0x30;

    if (em_work_die_ck(source)) {
        eft042_kill(self);
        return;
    }

    switch (self->type_0x02) {
    case 0:
    case 1:
    case 5:
        color = eft042_color_tbl[self->type_0x02];
        work->color.r = color[self->field_0x10].r;
        work->color.g = color[self->field_0x10].g;
        work->color.b = color[self->field_0x10].b;
        work->color.a = color[self->field_0x10].a;
        break;
    case 3:
    case 4:
    case 8:
    case 9:
    case 13:
    case 14:
    case 18:
        alpha = eft042_alpha_key_tbl[self->type_0x02];
        if (alpha != NULL) {
            work->color.a = eftGetKeyAlpha(alpha, self->field_0x10);
        }
        break;
    case 7:
        if (self->field_0x10 > 30) {
            self->field_0x10 = 0;
        }
        eftGetKeyRGB((u8*)eft042_color_tbl[self->type_0x02], self->field_0x10, &work->color.r, &work->color.g,
                     &work->color.b);
        work->color.a = eftGetKeyAlpha(eft042_alpha_key_tbl[self->type_0x02], self->field_0x10);
        break;
    case 17:
        eftGetKeyRGB(eft042_tev_color_keys, self->field_0x10, &work->color.r, &work->color.g, &work->color.b);
        eftGetKeyRGB(eft042_mat_color_keys, self->field_0x10, &key.r, &key.g, &key.b);
        key.a = 255;
        break;
    }

    self->field_0x10++;
    if (--self->timer_0x0C < 0) {
        switch (self->type_0x02) {
        case 7:
            self->state_0x05++;
            self->timer_0x0C = 30;
            self->demo_flag_0x08 = work->color.a / 30;
            eft042_end(self);
            return;
        }
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }

    switch (self->type_0x02) {
    case 0:
        get_joint_wmat_em(source, eft042_joint_tbl[self->type_0x02], &mtx);
        vec_to_mh_vec3(&offset, &eft042_offset_tbl[self->type_0x02]);
        mulVecMat(&offset, &mtx);
        mtx34_trans_add(&mtx, &offset);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        rotLocalMatZ(self->rot_0x24.z, &mtx);
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        for (i = 0; i < work->count; i++) {
            work->models[i]->move2(&mtx, 0);
        }
        created = work->created;
        if (created != NULL) {
            created->update();
        }
        break;
    case 1:
    case 5:
    case 10:
    case 17:
        get_joint_wmat_em(source, eft042_joint_tbl[self->type_0x02], &mtx);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        rotLocalMatZ(self->rot_0x24.z, &mtx);
        switch (self->type_0x02) {
        case 1:
        case 5:
            for (i = 2; i < 4; i++) {
                work->models[0]->setTevKColor(i, GX_KCOLOR3, &work->color);
            }
            break;
        case 17:
            for (i = 0; i < 2; i++) {
                work->models[0]->setTevKColor(i, GX_KCOLOR3, &work->color);
                work->models[0]->setMatColor(i, GX_COLOR0A0, key, false);
            }
            break;
        }
        for (i = 0; i < work->count; i++) {
            work->models[i]->move2(&mtx, 0);
        }
        created = work->created;
        if (created != NULL) {
            created->update();
        }
        break;
    case 3:
    case 4:
        getKeyData3(eft042_scale_keys_type3, (f32)self->field_0x10, &work->models[0]->scale_0x1C.x,
                    &work->models[0]->scale_0x1C.y, &work->models[0]->scale_0x1C.z);
        if (self->field_0x06 == 0) {
            self->field_0x06++;
            vec3_scale_in_place(&work->models[0]->scale_0x1C, 0.9f + 0.1f * (f32)(ran_suu(0) & 3));
        }
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        mtx34_identity(&mtx);
        rotMatrixY(self->rot_0x24.y, &mtx);
        rotLocalMatZ(source->field_0x1C4, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->models[i]->move2(&mtx, 0);
        }
        break;
    case 6:
        if (source->action != 1 || (source->state_sub != 4 && source->state_sub != 5)) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        get_joint_wmat_em(source, eft042_joint_tbl[self->type_0x02], &mtx);
        vec_to_mh_vec3(&offset, &eft042_offset_tbl[self->type_0x02]);
        mulVecMat(&offset, &mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        rotLocalMatZ(self->rot_0x24.z, &mtx);
        mtx34_trans_add(&mtx, &offset);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->models[i]->move2(&mtx, 0);
        }
        break;
    case 7:
        get_joint_wpos_em(source, eft042_joint_tbl[self->type_0x02], &self->pos_0x18);
        copyVec3(&work->models[0]->pos_0x04, &self->pos_0x18);
        eft_rot_vec_copy(&work->models[0]->rot_0x28, (_CP_VECTOR*)&source->field_0x1BC);
        work->models[0]->setMatColor(0, GX_COLOR0A0, work->color, false);
        work->models[0]->move(0);
        break;
    case 8:
    case 15:
        switch (self->type_0x02) {
        case 8:
            getKeyData3(eft042_scale_keys_type8, (f32)self->field_0x10, &work->models[0]->scale_0x1C.x,
                        &work->models[0]->scale_0x1C.y, &work->models[0]->scale_0x1C.z);
            break;
        case 15:
            getKeyData3(eft042_scale_keys_type15, (f32)self->field_0x10, &work->models[0]->scale_0x1C.x,
                        &work->models[0]->scale_0x1C.y, &work->models[0]->scale_0x1C.z);
            break;
        }
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        eft_rot_vec_copy(&self->rot_0x24, (_CP_VECTOR*)&source->field_0x1BC);
        get_joint_wmat_em(source, eft042_joint_tbl[self->type_0x02], &mtx);
        vec_to_mh_vec3(&offset, &eft042_offset_tbl[self->type_0x02]);
        mulVecMat(&offset, &mtx);
        mtx34_trans_add(&mtx, &offset);
        switch (self->type_0x02) {
        case 8:
            offset.y = getKeyData(eft042_height_keys_type8, (f32)self->field_0x10);
            break;
        case 15:
            offset.y = getKeyData(eft042_height_keys_type15, (f32)self->field_0x10);
            break;
        }
        mtx.m[1][3] = source->field_0x20C + offset.y;
        mtx34_trans_get(&mtx, &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->models[i]->move2(&mtx, 0);
        }
        created = work->created;
        if (created != NULL) {
            created->update();
        }
        break;
    case 9:
    case 13:
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        work->models[0]->move(0);
        if (self->field_0x07 < 6) {
            eft042_joint_show(self, self->field_0x07, 1, 5);
            self->field_0x07++;
        }
        break;
    case 14:
        self->rot_0x24.z += 5461;
        getKeyData3(eft042_scale_keys_type14, (f32)self->field_0x10, &work->models[0]->scale_0x1C.x,
                    &work->models[0]->scale_0x1C.y, &work->models[0]->scale_0x1C.z);
        eft_rot_vec_copy(&work->models[0]->rot_0x28, &self->rot_0x24);
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        work->models[0]->move(0);
        break;
    case 16:
        if (source->action != 1 || source->state_sub != 4) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        get_joint_wmat_em(source, eft042_joint_tbl[self->type_0x02], &mtx);
        vec_to_mh_vec3(&offset, &eft042_offset_tbl[self->type_0x02]);
        mulVecMat(&offset, &mtx);
        rotLocalMatX(self->rot_0x24.x, &mtx);
        mtx34_trans_add(&mtx, &offset);
        mtx34_trans_get(&mtx, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->models[i]->move2(&mtx, 0);
        }
        break;
    case 18:
        self->rot_0x24.y += 5461;
        getKeyData3(eft042_scale_keys_type18, (f32)self->field_0x10, &work->models[0]->scale_0x1C.x,
                    &work->models[0]->scale_0x1C.y, &work->models[0]->scale_0x1C.z);
        mtx34_identity(&mtx);
        rotMatrixX(self->rot_0x24.x, &mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->models[0]->setTevKColor(0, GX_KCOLOR3, &work->color);
        work->models[0]->move2(&mtx, 0);
        break;
    }

    eft_res_models_spawn(self, (void**)work->models, 2, work->count, NULL);
}

/* ---------------------------------------------------------------------------------------------------
 * 0x803088FC, 0x80308A30, 0x80308D00 - the kind-1/kind-2 state-1 bodies and the type-7 state-2 body
 * ------------------------------------------------------------------------------------------------- */

/* Kind 1, state 1: walk the placement offset down to the source model's joint every frame, rotate
 * its matrix by the effect's two angles and hand it to the model. */
extern "C" void eft042_move_kind1(_EFT* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::MTX34 mtx;
    Eft042WorkKind1* work;
    Eft042Kind1Source* source;

    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    work = (Eft042WorkKind1*)self->work_0x38;
    source = (Eft042Kind1Source*)self->source_0x30;

    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
    } else {
        self->timer_0x0C--;
        if (self->pos_0x18.y < source->floor_y_0x60 || self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
        } else {
            work->v_0x0C.offset.y -= work->field_0x18;
            addVec3To(&self->pos_0x18, &work->v_0x0C.offset);
            self->rot_0x24.x -= 2185;
            mtx34_identity(&mtx);
            rotLocalMatY(self->rot_0x24.y, &mtx);
            rotLocalMatX(self->rot_0x24.x, &mtx);
            mtx34_set_trans(&mtx, &self->pos_0x18);
            work->model->move2(&mtx, 0);
            eft_res_models_spawn(self, (void**)&work->model, 2, work->count, NULL);
        }
    }
}

/* Kind 2, state 1: count the step timer down, scroll both materials' texture matrices by their key
 * lists, colour both materials from their RGB/alpha keys and walk the model (types 11/12 spawn with
 * the stage's flag when the source is the master player). */
extern "C" void eft042_move_kind2(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    Eft042Work* work;
    _PLW* source;
    u8* rgb0;
    u8* rgb1;
    _GXColor color;
    s32 mode;

    MTX34_ctor(&mtx);
    work = (Eft042Work*)self->work_0x38;
    source = (_PLW*)self->source_0x30;

    if (--self->timer_0x0C < 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    self->field_0x10++;

    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)work->models[0].model->field_0x118, 0);
        if (copied_mat_access_valid_ck(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx);
            mtx.m[0][3] = getKeyData(eft042_mat0_scroll_keys, (f32)self->field_0x10);
            srt.SetEffectMtx(0, &mtx);
        }
    }
    {
        nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)work->models[0].model->field_0x118, 1);
        if (copied_mat_access_valid_ck(&access) != 0) {
            nw4r::g3d::ResTexSrt srt;
            res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
            srt.GetEffectMtx(0, &mtx);
            mtx.m[0][3] = getKeyData(eft042_mat1_scroll_keys, (f32)self->field_0x10);
            srt.SetEffectMtx(0, &mtx);
        }
    }

    switch (self->type_0x02) {
    default:
        rgb0 = eft042_mat0_rgb_keys;
        rgb1 = eft042_mat1_rgb_keys;
        break;
    case 12:
        rgb0 = eft042_mat0_rgb_keys_type12;
        rgb1 = eft042_mat1_rgb_keys_type12;
        break;
    }
    if (rgb0 != NULL) {
        eftGetKeyRGB(rgb0, self->field_0x10, &color.r, &color.g, &color.b);
        color.a = eftGetKeyAlpha(eft042_mat0_alpha_keys, self->field_0x10);
        work->models[0].model->setMatColor(0, GX_COLOR0A0, color, false);
    }
    if (rgb1 != NULL) {
        eftGetKeyRGB(rgb1, self->field_0x10, &color.r, &color.g, &color.b);
        color.a = eftGetKeyAlpha(eft042_mat1_alpha_keys, self->field_0x10);
        work->models[0].model->setMatColor(1, GX_COLOR0A0, color, false);
    }
    work->models[0].model->move(0);

    mode = 2;
    if ((u32)(self->type_0x02 - 11) <= 1 && source != NULL && Pl_master_ck(source) == 1) {
        mode = eft_stage_spawn_flag(source) | 2;
    }
    eft_res_models_spawn(self, (void**)&work->models[0].model, mode, 1, NULL);
}

/* The type-7 state-2 body: follow the source model's joint, set the material's colour from the work
 * area's alpha (dropped by the effect's own +0x08 byte every frame) and walk the model along. */
extern "C" void eft042_end(_EFT* self)
{
    Eft042WorkKind1* work = (Eft042WorkKind1*)self->work_0x38;
    _ENEMY_WORK* source = (_ENEMY_WORK*)self->source_0x30;

    switch (self->type_0x02) {
    case 7:
        if (em_work_die_ck(source)) {
            self->state_0x05++;
            break;
        }
        if (--self->timer_0x0C > 0) {
            work->v_0x0C.color.a -= self->demo_flag_0x08;
            get_joint_wpos_em(source, eft042_joint_tbl[self->type_0x02], &self->pos_0x18);
            copyVec3(&work->model->pos_0x04, &self->pos_0x18);
            eft_rot_vec_copy(&work->model->rot_0x28, (_CP_VECTOR*)&source->field_0x1BC);
            work->model->setMatColor(0, GX_COLOR0A0, work->v_0x0C.color, false);
            work->model->move(0);
            eft_res_models_spawn(self, (void**)&work->model, 2, work->count, NULL);
        } else {
            self->state_0x05++;
        }
        break;
    default:
        self->state_0x05++;
        break;
    }
}

/* ---------------------------------------------------------------------------------------------------
 * 0x80308E34..0x80308F1C - the machine's retire arm and the screen tail
 * --------------------------------------------------------------------------------------------------- */

/* The state-3 arm: retire the effect record. */
extern "C" void eft042_kill(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Makes exactly one joint of the work area's model visible and hides every joint in `[from, to]` -
 * the joint-walk the state-1 body and `eft035.cpp`'s `fn_802F3954` share. */
extern "C" void eft042_joint_show(_EFT* self, u32 visible, u8 from, u8 to)
{
    Eft042Work* work = (Eft042Work*)self->work_0x38;
    s32 joint;

    for (joint = from; joint <= to; joint++) {
        if (joint == visible) {
            work->models[0].model->setVisibility(joint, true);
        } else {
            work->models[0].model->setVisibility(joint, false);
        }
    }
}

/* Arms the screen: the mode byte, the cursor reset, the equipment page the `+0x190` record hands out
 * (the returned count picks the last-page byte) and the page index. */
extern "C" void menu_status_page_open(StatusScreenWork* self)
{
    self->screen_mode = 7;
    self->field_0x23A = 0;
    self->field_0x1AF = (s8)(menu_equip_page_count(self->equip, 0) + 2);
    self->field_0x1AE = 0;
}

/* Folds the screen's flag words into one mask: the SE bit plays the confirm SE (returns 2), the scroll bits step the
 * page through `menu_cursor_step` (returns 0). */
extern "C" s32 menu_status_page_step(StatusScreenWork* self)
{
    s32 result = 0;
    u16 flags = (u16)(self->pad_trig | (self->pad_repeat & 0xF));

    if ((flags & 0x20) != 0) {
        result = 2;
        sysSE_req(1);
    } else if ((flags & 0xC) != 0) {
        self->field_0x1AE = (s8)menu_cursor_step(self->field_0x1AE, self->field_0x1AF, flags, 4, 8);
    }
    return result;
}
