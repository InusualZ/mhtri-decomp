/* ef/em_effect_ctrl.cpp - the state handlers `ef/eft004_fx.cpp`'s `fn_80101DB8` dispatches on (state 0
 *   `fn_80101DF4` fills the state's `EftModelSet` from the per-type record runs and the per-map area tables; states
 *   1-3 `fn_80101FA4`/`fn_801025E8`/`fn_801025F8`) and the enemy-effect (`em`) controller: `EmEffectWork`/
 *   `EmEffectUnit` and `fn_80101FA4`'s per-frame screen-box test.
 * RANGE. .text 0x80101DF4-0x80102994 (7 functions); extab 0x8000BEF4-0x8000BF1C, extabindex 0x80025D58-0x80025D94,
 *   .data 0x8059C8E8-0x8059DAA0, .sdata 0x80791790-0x807917A0, .sdata2 0x807966E8-0x80796740.  One TU: the
 *   area-table family `eft_map_area_spheres_0..9` is one definition list whose 8-byte member `_9` lands in `.sdata`
 *   after `em_key_alpha_table`, and only `fn_80101DF4` reads the `.data` tables.  The player-weapon controller from
 *   0x80102994 is `ef/eft007.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` for `fn_80101DF4`, `fn_80101FA4`, `fn_801025FC` and `fn_801027D0`
 *   (retail keeps the unfused `clrlwi`+`slwi` index and `subi`/`clrlwi`/`extsb`/`rlwinm` + `cmpwi` forms; playbook
 *   39); `#pragma pool_data off` for the `fn_80101DF4` block (retail addresses each table with its own `lis`/`addi`);
 *   `#pragma fp_contract off` for `fn_80101FA4` (retail keeps `fmuls` + `fsubs`; playbook 40).
 * NAMES. The unit name is a GUESS (the range holds the `em` controller); the map has only `fn_` stems, so the
 *   definitions are `extern "C"` in a C++ unit.  GUESS: `em_key_alpha_table` (map `lbl_80791790`, the
 *   `eftGetKeyAlpha` table); every `fn_80101DF4` table, named for what it holds (map `lbl_8059C8E8`..`lbl_8059DA48`
 *   and `lbl_80791798`, renames filed): `eft_model_records_typeN`, `eft_model_record_runs`, `eft_model_effect_ids`,
 *   `eft_area_spheres_NN`, `eft_map_area_spheres_N`, `eft_map_area_table`; `EftAreaSphere`, `EftModelSet::area_spheres`.
 * RESIDUALS. none in bytes: every row, `.data`, `.sdata` and `.sdata2` match (the `.sdata2` claim's last word is the
 *   fill before `ef/eft007.cpp`'s pool).  Pool relocations name `@NNN` where retail names the map's `lbl_` rows.
 * SHAPES. `em_key_alpha_table` is defined before the area tables: `.sdata` follows definition order.
 *   `fn_80101DF4`'s record cursor is the source's `i * sizeof(EftModelRecord)` index, which MWCC strength-reduces into
 *   retail's separate byte-offset register (`addi r27,r27,20`); a byte-offset local swaps it with the `chars`
 *   pointer (playbook 19/20).
 *   `fn_80101FA4` scales by the dot product directly (`vec3_scale(&v64, &v100, vec3_dot(...))`) and by each
 *   entry's own `intensity` byte; its scalar declarations run `f31, f29, intensity, alpha, part, i, unit` (the order
 *   colours `unit` r25 and `i` r26).
 *   `fn_801027D0`'s segment is one `EmEffectSegment` local (from/to at +0x28/+0x34), declared first; the probe loop
 *   tests the literal (`scale > 0.0f`): a `zero` local schedules the `lfs f31` one slot early.
 *   `EmEffectUnit::handles` is an array member and `fn_80101FA4` writes `unit->handles[i]->field` inline:
 *   retail reloads the handle and `unit->entries[i]` at every access.
 *   The screen-box test and its off-screen accumulator use the negated comparisons (`!(x <= lo)`, `!(x >= hi)`,
 *   `x >= 640.0f`, `x <= 0.0f`): only the `<=`/`>=` spellings give retail's `fcmpo` + `cror`.
 *   `_EmHandle` here and `MHchar` (`pl.h`) are two views of one map name whose fields disagree; `Vec` comes from
 *   `ef.h` through `pl.h` (the `ef/eft007_types.h` copy would redefine it).
 */

#include "ef/eft_res_models_spawn.h" /* eft_res_models_spawn (rule 2: the owner's header) */
#include "ef/eft_res_slot_release.h" /* eft_res_slot_release (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "Pl/plw.h"
#include "Pl/pl_hit_sphere.h"
#include "gx.h"
#include "ef/eft004.h"
#include "ef/eft009.h"
#include "unsplit/ef.h"
#include "g3d/g3d_scnroot.h" /* fn_80082BCC (rule 2) */
#include "unsplit/sound.h"
#include "unsplit/unknown.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/pRoot.h"

#include "ef/ef_emitter.h" /* ef_store_word (rule 2) */
#include "pl.h"


#pragma peephole off
#pragma pool_data off

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* `MHchar`, the effect owner the library's `res_eft_model_create` works on, comes from `pl.h` (rule 1: one definition). */

/* One entry of the 0xFF-terminated run `EftModelSet::records` points at, 0x14 B.  Only the id and the
 * scale are read here; the rest of the entry is the create path's (the DOL's own records carry 1..15 in
 * +0x01..+0x03 and floats in +0x08/+0x0C, so those bytes are data, not padding). */
struct EftModelRecord {
    /* +0x00 */ u8 effect_id; /* index into the shared effect-id table `eft_model_effect_ids`; 0xFF ends the run */
    /* +0x01 */ u8 unused_0x01[3];
    /* +0x04 */ f32 scale; /* copied into the model's +0x1C and +0x20 */
    /* +0x08 */ f32 unused_0x08;
    /* +0x0C */ f32 unused_0x0C;
    /* +0x10 */ u32 unused_0x10;
}; /* size: 0x14 */

/* The per-state model set `fn_800F8B44` allocates; `fn_80101C74` passes 0x60 as its size to
 * `eft_res_slot_get`.  `fn_80101D70` releases `chars[0..char_count-1]` and then `main_char`. */
/* One entry of a per-area table, 0x10 B: a position and a radius (GUESS from the values); a zero entry
 * ends the table.  `EftModelSet::area_spheres` holds the current area's table. */
struct EftAreaSphere {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
    /* +0x0C */ f32 radius;
}; /* size: 0x10 */

struct EftModelSet {
    /* +0x00 */ u32 char_count;  /* 12, the size of `chars` */
    /* +0x04 */ u32 model_count; /* how many effect models this state created */
    /* +0x08 */ f32 field_08;    /* set to 66.0f here */
    /* +0x0C */ u32 field_0C;
    /* +0x10 */ MHchar* chars[12];
    /* +0x40 */ MHchar* main_char; /* the owner the fixed-id (14) model is created on */
    /* +0x44 */ u8 field_44;       /* written 0xFF here; no reader in this unit or its neighbours */
    /* +0x45 */ u8 field_45;       /* written 0xD0 */
    /* +0x46 */ u8 field_46;       /* written 0xE9 */
    /* +0x47 */ u8 field_47;       /* written 0x00 */
    /* +0x48 */ nw4r::math::VEC3 vec; /* set from the current map/area */
    /* +0x54 */ u32 field_54;
    /* +0x58 */ EftModelRecord* records; /* the type's effect-record run */
    /* +0x5C */ EftAreaSphere* area_spheres; /* the current map's table for the current area */
}; /* size: 0x60 */

/* The effect-state record `eft_res_slot_get` hands out of its pool (record stride 0x48).  `fn_80101DB8` is
 * installed at +0x34 as the state dispatcher and `fn_80101D70` at +0x40 as the update handler;
 * `fn_80101C74` is the constructor and `eft_res_slot_release` the release path. */
struct EftState {
    /* +0x00 */ u8 in_use;   /* the pool allocator's busy flag */
    /* +0x01 */ u8 field_01; /* set once the state has been set up */
    /* +0x02 */ u8 type_id;  /* selects the effect-record run */
    /* +0x03 */ u8 field_03;
    /* +0x04 */ u8 field_04;
    /* +0x05 */ u8 state; /* the dispatcher's state number */
    /* +0x06 */ u8 unused_0x06;       /* cleared by the pool allocator */
    /* +0x07 */ u8 unused_0x07;       /* cleared by the pool allocator */
    /* +0x08 */ u8 unused_0x08[0x0C]; /* +0x08 is cleared by the pool allocator */
    /* +0x14 */ u8 unused_0x14[0x04]; /* cleared by the pool allocator */
    /* +0x18 */ u8 unused_0x18[0x1C];
    /* +0x34 */ void (*dispatch)(EftState*); /* `fn_80101DB8` */
    /* +0x38 */ EftModelSet* set;
    /* +0x3C */ u32 set_blocks; /* the set's block count, kept for the release path */
    /* +0x40 */ void (*update)(EftState*); /* `fn_80101D70` */
    /* +0x44 */ u8 area; /* `get_now_areano()`, indexes the per-map area table */
    /* +0x45 */ u8 pad_0x45[0x03];
}; /* size: 0x48 (the pool's stride; the allocator writes no further field) */

/* ------------------------------------------------------------------------------------------------ */
/* externs                                                                                           */
/* ------------------------------------------------------------------------------------------------ */

/* Stores its second argument through its first. */
/* Returns the shared `ef_resource_singleton` block. */
extern "C" void* ef_resource_instance(void);
/* Bounds-checks `index` against `obj`'s +0x08 count, then forwards to `fn_80501C9C`. */
extern "C" void ef_resource_effect_project_at(void* obj, u16 index);
/* The `nw4r::math::VEC3` copy the map still spells `copyVec3`. */
/* The library's error path (`eft_res_slot_release`). */
/* The state-3 handler, defined below. */
extern "C" void fn_801025F8(void* self);
/* Builds the current area's effect vector from the per-map table at 0x806C87C0. */
extern "C" void fn_802B00AC(nw4r::math::VEC3* out, u8 area);

/* Creates the `id` effect model on `chr`; NULL on failure. */
void* res_eft_model_create(MHchar* chr, u16 id, u32 arg);
/* The current map number, narrowed to a byte by the caller. */
u8 get_now_mapno(void);


/* ------------------------------------------------------------------------------------------------ */
/* data (.data 0x8059C8E8-0x8059DAA0, in address order)                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The key-alpha table `eftGetKeyAlpha` walks; the first `.sdata` object, so it is defined first. */
u8 em_key_alpha_table[8] = { 0x00, 0x00, 0x0A, 0x80, 0x34, 0x00, 0xFF, 0x00 };

/* The per-type effect-record runs, each ended by an 0xFF id; `eft_model_record_runs` indexes them by
 * `EftState::type_id`, `eft_model_effect_ids` maps a record's id to the effect id. */
EftModelRecord eft_model_records_type0[6] = {
    { 0x00, { 0x00, 0x00, 0x01 }, 4.6f, 4.6f, 0.0f, 0x00375F2F },
    { 0x01, { 0x01, 0x02, 0x01 }, 4.0f, 4.0f, 0.0f, 0x0069D717 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 0.5f, 0.5f, -0.2f, 0x55DC5A70 },
    { 0x03, { 0x03, 0x0C, 0x00 }, 0.1f, 0.1f, 4.5f, 0x0000374D },
    { 0x03, { 0x03, 0x0F, 0x00 }, 0.3f, 0.3f, 7.9f, 0xE84B1961 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type1[13] = {
    { 0x00, { 0x00, 0x00, 0x01 }, 4.9f, 4.9f, 0.0f, 0x37578109 },
    { 0x05, { 0x05, 0x01, 0x01 }, 1.6f, 1.6f, 0.0f, 0x29313F11 },
    { 0x01, { 0x01, 0x02, 0x01 }, 4.6f, 4.6f, 0.0f, 0x00154613 },
    { 0x05, { 0x05, 0x04, 0x01 }, 11.5f, 0.2f, 0.0f, 0x00EB805D },
    { 0x06, { 0x06, 0x05, 0x01 }, 2.1f, 2.1f, 0.0f, 0x00000068 },
    { 0x05, { 0x05, 0x09, 0x00 }, 0.4f, 0.4f, -1.9f, 0x77E50047 },
    { 0x05, { 0x05, 0x0A, 0x00 }, 0.1f, 0.1f, -1.2f, 0x00003729 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 1.0f, 1.0f, 2.5f, 0xD19F805A },
    { 0x05, { 0x05, 0x0C, 0x00 }, 0.1f, 0.1f, 6.7f, 0x00003729 },
    { 0x07, { 0x07, 0x0D, 0x00 }, 4.1f, 4.1f, 21.4f, 0xC6D90075 },
    { 0x04, { 0x04, 0x0E, 0x00 }, 1.7f, 1.7f, 32.5f, 0xBA2C4C71 },
    { 0x05, { 0x05, 0x0F, 0x00 }, 0.3f, 0.3f, 7.9f, 0xBC620037 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type2[11] = {
    { 0x03, { 0x03, 0x00, 0x01 }, 6.0f, 6.0f, 0.0f, 0x00588114 },
    { 0x05, { 0x05, 0x01, 0x02 }, 1.6f, 1.6f, 0.0f, 0x29313F11 },
    { 0x01, { 0x01, 0x02, 0x01 }, 5.4f, 5.4f, 0.0f, 0x007D6E00 },
    { 0x06, { 0x06, 0x04, 0x01 }, 4.3f, 4.3f, 0.0f, 0x00000059 },
    { 0x03, { 0x03, 0x0A, 0x00 }, 0.1f, 0.1f, 0.8f, 0x00003729 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 0.4f, 0.4f, 5.0f, 0xA3F7016F },
    { 0x03, { 0x03, 0x0C, 0x00 }, 0.1f, 0.1f, 8.8f, 0x00003729 },
    { 0x07, { 0x07, 0x0D, 0x00 }, 5.0f, 5.0f, 23.5f, 0xF3F4666B },
    { 0x04, { 0x04, 0x0E, 0x00 }, 2.2f, 2.2f, 35.4f, 0xBA2C4C74 },
    { 0x04, { 0x04, 0x0F, 0x00 }, 1.0f, 1.0f, 10.0f, 0xFF511770 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type3[7] = {
    { 0x01, { 0x01, 0x00, 0x01 }, 6.9f, 6.9f, 0.0f, 0x001E9C00 },
    { 0x05, { 0x05, 0x01, 0x01 }, 2.6f, 2.6f, 0.0f, 0x091B8E2F },
    { 0x04, { 0x04, 0x0A, 0x00 }, 0.5f, 0.5f, -5.4f, 0x00AFCD4D },
    { 0x04, { 0x04, 0x0B, 0x00 }, 1.1f, 1.1f, -2.5f, 0xFFB79918 },
    { 0x04, { 0x04, 0x0C, 0x00 }, 0.5f, 0.5f, 3.4f, 0x00F5FF47 },
    { 0x05, { 0x05, 0x0F, 0x00 }, 0.1f, 0.1f, 0.1f, 0xFF007D39 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type4[3] = {
    { 0x02, { 0x02, 0x00, 0x01 }, 3.5f, 3.5f, 0.0f, 0x010F3739 },
    { 0x05, { 0x05, 0x02, 0x01 }, 5.5f, 5.5f, 0.0f, 0x0F13374D },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type5[13] = {
    { 0x00, { 0x00, 0x00, 0x01 }, 7.89f, 7.89f, 0.0f, 0x27509C42 },
    { 0x05, { 0x05, 0x01, 0x01 }, 4.19f, 4.19f, 0.0f, 0x091B8E5E },
    { 0x01, { 0x01, 0x02, 0x01 }, 4.5f, 4.5f, 0.0f, 0x0015461B },
    { 0x05, { 0x05, 0x04, 0x01 }, 11.5f, 0.2f, 0.0f, 0x12DE8057 },
    { 0x06, { 0x06, 0x05, 0x01 }, 2.4f, 2.4f, 0.0f, 0x00000068 },
    { 0x05, { 0x05, 0x09, 0x00 }, 0.4f, 0.4f, -1.9f, 0x77E50047 },
    { 0x05, { 0x05, 0x0A, 0x00 }, 0.1f, 0.1f, -1.2f, 0x00003729 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 1.0f, 1.0f, 2.5f, 0xD19F805A },
    { 0x05, { 0x05, 0x0C, 0x00 }, 0.1f, 0.1f, 6.7f, 0x00003729 },
    { 0x07, { 0x07, 0x0D, 0x00 }, 4.0f, 4.0f, 21.4f, 0xC6D9006D },
    { 0x04, { 0x04, 0x0E, 0x00 }, 2.3f, 2.3f, 32.5f, 0xBA2C4C71 },
    { 0x05, { 0x05, 0x0F, 0x00 }, 0.3f, 0.3f, 7.9f, 0xBC620037 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type6[5] = {
    { 0x02, { 0x02, 0x00, 0x01 }, 3.5f, 3.5f, 0.0f, 0x012D7311 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 0.6f, 0.6f, -8.28f, 0x9F41B470 },
    { 0x05, { 0x05, 0x0D, 0x00 }, 0.2f, 0.2f, -8.91f, 0x0A9FE44E },
    { 0x04, { 0x04, 0x0E, 0x00 }, 0.1f, 0.1f, -4.82f, 0xF5CD006B },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type7[6] = {
    { 0x00, { 0x00, 0x00, 0x01 }, 4.8f, 4.8f, 0.0f, 0x05374B00 },
    { 0x01, { 0x01, 0x02, 0x01 }, 4.0f, 4.0f, 0.0f, 0x005FAF17 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 0.7f, 0.7f, -0.2f, 0x44DC8267 },
    { 0x03, { 0x03, 0x0C, 0x00 }, 0.1f, 0.1f, 1.3f, 0x00003757 },
    { 0x03, { 0x03, 0x0F, 0x00 }, 0.3f, 0.3f, 2.0f, 0xE86F004F },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type8[3] = {
    { 0x02, { 0x02, 0x00, 0x01 }, 3.5f, 3.5f, 0.0f, 0x010F871B },
    { 0x05, { 0x05, 0x01, 0x01 }, 5.5f, 5.5f, 0.0f, 0x000F1939 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type9[5] = {
    { 0x02, { 0x02, 0x00, 0x01 }, 4.0f, 4.0f, 0.0f, 0x014BB91B },
    { 0x05, { 0x05, 0x02, 0x01 }, 5.5f, 5.5f, 0.0f, 0x000F8743 },
    { 0x04, { 0x04, 0x0B, 0x00 }, 0.6f, 0.6f, -8.28f, 0x9B0AA069 },
    { 0x03, { 0x03, 0x0D, 0x00 }, 0.2f, 0.2f, -8.91f, 0x0A69E44D },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type10[4] = {
    { 0x02, { 0x02, 0x00, 0x01 }, 3.5f, 3.5f, 0.0f, 0x0123694D },
    { 0x03, { 0x03, 0x0D, 0x00 }, 0.6f, 0.6f, -13.6f, 0xD623D96B },
    { 0x05, { 0x05, 0x0E, 0x00 }, 0.1f, 0.1f, -12.9f, 0xD229155E },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type11[3] = {
    { 0x02, { 0x02, 0x00, 0x01 }, 3.0f, 3.0f, 0.0f, 0x0137554D },
    { 0x03, { 0x03, 0x0D, 0x00 }, 0.2f, 0.2f, -15.7f, 0x9B23876B },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord eft_model_records_type12[5] = {
    { 0x03, { 0x03, 0x00, 0x01 }, 1.9f, 1.9f, 0.0f, 0x87873761 },
    { 0x05, { 0x05, 0x0B, 0x00 }, 0.5f, 0.5f, -10.0f, 0x0051036B },
    { 0x05, { 0x05, 0x0D, 0x00 }, 0.1f, 0.1f, -10.5f, 0xD623D970 },
    { 0x05, { 0x05, 0x0E, 0x00 }, 0.6f, 0.6f, -11.0f, 0xD2291561 },
    { 0xFF, { 0xFF, 0xFF, 0x00 }, 0.0f, 0.0f, 0.0f, 0x00000000 },
};

EftModelRecord* eft_model_record_runs[14] = {
    eft_model_records_type0, eft_model_records_type1, eft_model_records_type2, eft_model_records_type3, eft_model_records_type4, eft_model_records_type5, eft_model_records_type6, eft_model_records_type7, eft_model_records_type8, eft_model_records_type9, eft_model_records_type10, eft_model_records_type11, eft_model_records_type12, NULL,
};

u16 eft_model_effect_ids[8] = {
    0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 0x0010,
};

EftAreaSphere eft_area_spheres_00[3] = {
    { 18600.0f, 0.0f, 4420.0f, 20000.0f },
    { -5540.0f, 4040.0f, -4620.0f, 1500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_01[3] = {
    { 0.0f, 0.0f, 10830.0f, 10000.0f },
    { -4300.0f, -300.0f, -8700.0f, 1500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_02[2] = {
    { -9400.0f, 0.0f, 2000.0f, 10000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_03[5] = {
    { -2700.0f, 7150.0f, -3150.0f, 800.0f },
    { -2900.0f, 5750.0f, -3250.0f, 800.0f },
    { -13600.0f, -8670.0f, -14000.0f, 20000.0f },
    { -3280.0f, 4980.0f, 4060.0f, 800.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_04[6] = {
    { -5200.0f, 1650.0f, -6850.0f, 5000.0f },
    { -6350.0f, 500.0f, -3300.0f, 5000.0f },
    { -7800.0f, 800.0f, 1500.0f, 5000.0f },
    { -3450.0f, 1250.0f, 3100.0f, 2000.0f },
    { 7900.0f, 0.0f, -8300.0f, 5000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_05[4] = {
    { -6150.0f, 4400.0f, -6000.0f, 3000.0f },
    { -1500.0f, 2000.0f, -8800.0f, 5000.0f },
    { -13300.0f, 3000.0f, -1500.0f, 10000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_06[4] = {
    { -3700.0f, 3130.0f, 1350.0f, 2000.0f },
    { -3840.0f, 3470.0f, 3070.0f, 2500.0f },
    { -4480.0f, 3500.0f, -1550.0f, 3000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_07[6] = {
    { 3050.0f, 1780.0f, 5760.0f, 500.0f },
    { 2500.0f, 1800.0f, 5900.0f, 600.0f },
    { 1700.0f, 1900.0f, 6100.0f, 900.0f },
    { 5700.0f, 3000.0f, -2400.0f, 1000.0f },
    { 1250.0f, 1900.0f, 6900.0f, 900.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_08[3] = {
    { -8600.0f, 0.0f, 2200.0f, 10000.0f },
    { -2420.0f, 1000.0f, -4830.0f, 4000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_09[6] = {
    { -2700.0f, 7150.0f, -3150.0f, 800.0f },
    { -2900.0f, 5750.0f, -3250.0f, 800.0f },
    { -13600.0f, -8670.0f, -14000.0f, 20000.0f },
    { -3280.0f, 4980.0f, 4060.0f, 800.0f },
    { 5000.0f, -2900.0f, -3700.0f, 4000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_10[6] = {
    { -5200.0f, 1650.0f, -6850.0f, 5000.0f },
    { -8200.0f, 1400.0f, -18300.0f, 10000.0f },
    { -7800.0f, 800.0f, 1500.0f, 5000.0f },
    { -3450.0f, 1250.0f, 3100.0f, 2000.0f },
    { 7250.0f, 1500.0f, -9070.0f, 5000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_11[6] = {
    { -100.0f, -1900.0f, -3500.0f, 4000.0f },
    { 2970.0f, 600.0f, -1670.0f, 2000.0f },
    { -2900.0f, 1200.0f, -700.0f, 2000.0f },
    { -3550.0f, 500.0f, 3300.0f, 2000.0f },
    { 2800.0f, 1900.0f, 2200.0f, 1900.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_12[6] = {
    { 5400.0f, -2360.0f, -400.0f, 3200.0f },
    { 6600.0f, -800.0f, -1000.0f, 2400.0f },
    { -2000.0f, -1000.0f, -4000.0f, 2000.0f },
    { -6500.0f, -2000.0f, -3650.0f, 6000.0f },
    { -2600.0f, -3700.0f, 1600.0f, 5000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_13[6] = {
    { 3050.0f, 1780.0f, 5760.0f, 500.0f },
    { 2500.0f, 1800.0f, 5900.0f, 600.0f },
    { 1700.0f, 1900.0f, 6100.0f, 900.0f },
    { 8400.0f, 3000.0f, -4200.0f, 4000.0f },
    { 1250.0f, 1900.0f, 6900.0f, 900.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_14[4] = {
    { -2700.0f, 2330.0f, 48.0f, 2000.0f },
    { -2800.0f, -30.0f, 185.0f, 1000.0f },
    { -930.0f, 1770.0f, -870.0f, 1000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_15[5] = {
    { 4000.0f, -2000.0f, -1100.0f, 4000.0f },
    { 5400.0f, 3500.0f, 1300.0f, 4000.0f },
    { 970.0f, 500.0f, -1800.0f, 500.0f },
    { 560.0f, 650.0f, -1780.0f, 350.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_16[2] = {
    { 2700.0f, 1050.0f, -1700.0f, 500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_17[6] = {
    { -5460.0f, 0.0f, -1170.0f, 2000.0f },
    { -8650.0f, 2600.0f, -5540.0f, 2000.0f },
    { -12000.0f, 2900.0f, -5200.0f, 2000.0f },
    { -5950.0f, -1500.0f, 4140.0f, 2500.0f },
    { -1660.0f, 2070.0f, -5300.0f, 1500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_18[5] = {
    { 590.0f, 430.0f, 3380.0f, 530.0f },
    { 670.0f, 1010.0f, 3430.0f, 410.0f },
    { 1000.0f, 1560.0f, 3615.0f, 520.0f },
    { 1330.0f, 1840.0f, 3810.0f, 500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_19[3] = {
    { 1380.0f, 240.0f, -7340.0f, 2000.0f },
    { 3320.0f, 640.0f, -6940.0f, 1500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_20[4] = {
    { 3370.0f, -2100.0f, 0.0f, 1700.0f },
    { 360.0f, 60.0f, -1340.0f, 1000.0f },
    { 710.0f, 1020.0f, -1130.0f, 180.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_21[4] = {
    { 11500.0f, 5700.0f, -13300.0f, 6000.0f },
    { -1400.0f, 1300.0f, -3700.0f, 1500.0f },
    { -3000.0f, 1300.0f, -4500.0f, 1500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_22[3] = {
    { 3200.0f, 1300.0f, -4300.0f, 3900.0f },
    { -1500.0f, 800.0f, 4000.0f, 1500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_23[2] = {
    { 6100.0f, 12900.0f, -3140.0f, 16500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_24[6] = {
    { 4960.0f, -1655.0f, -4200.0f, 5000.0f },
    { 5300.0f, -545.0f, 8450.0f, 5000.0f },
    { 5620.0f, -2200.0f, -535.0f, 5000.0f },
    { 5500.0f, 925.0f, -10800.0f, 5000.0f },
    { 1725.0f, 1060.0f, -3890.0f, 1200.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_25[4] = {
    { 3000.0f, 3800.0f, 3675.0f, 2000.0f },
    { 3125.0f, 2570.0f, 4360.0f, 1400.0f },
    { 2560.0f, 1425.0f, 2200.0f, 1200.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_26[3] = {
    { 11000.0f, 1700.0f, 12000.0f, 12000.0f },
    { -8000.0f, -500.0f, 6300.0f, 6000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_27[4] = {
    { 31.0f, 1436.0f, -645.0f, 332.0f },
    { 450.0f, 1781.0f, -44.0f, 633.0f },
    { 503.0f, 2170.0f, 415.0f, 533.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_28[5] = {
    { 3042.0f, 2719.0f, 918.0f, 1100.0f },
    { 1706.0f, 1301.0f, 3662.0f, 3000.0f },
    { -1389.0f, 5724.0f, 1767.0f, 3000.0f },
    { -193.0f, 5796.0f, -1265.0f, 1300.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_29[5] = {
    { -5500.0f, 4300.0f, -6600.0f, 4000.0f },
    { 1706.0f, 1301.0f, 3662.0f, 3000.0f },
    { -1389.0f, 5724.0f, 1767.0f, 3000.0f },
    { -2080.0f, 750.0f, -200.0f, 600.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_30[5] = {
    { 2262.0f, 1350.0f, -2192.0f, 1400.0f },
    { 2284.0f, 2243.0f, -697.0f, 600.0f },
    { 2175.0f, 2227.0f, -118.0f, 500.0f },
    { 2151.0f, 1996.0f, 504.0f, 500.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_31[6] = {
    { 15484.0f, 7949.0f, 1107.0f, 1300.0f },
    { 13739.0f, 8033.0f, 4952.0f, 1300.0f },
    { 14725.0f, 8306.0f, 3050.0f, 1300.0f },
    { 5404.0f, 1918.0f, -5010.0f, 4000.0f },
    { 240.0f, 1490.0f, 7010.0f, 4000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_32[6] = {
    { 13270.0f, 7722.0f, 1298.0f, 1200.0f },
    { 11651.0f, 7838.0f, 4634.0f, 1200.0f },
    { 12556.0f, 8009.0f, 3026.0f, 1000.0f },
    { 5404.0f, 1918.0f, -5010.0f, 4000.0f },
    { 18000.0f, -1960.0f, -12000.0f, 8000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere eft_area_spheres_33[3] = {
    { 6240.0f, -3000.0f, -6080.0f, 8000.0f },
    { 0.0f, -3000.0f, 0.0f, 8000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

EftAreaSphere* eft_map_area_spheres_0[13] = {
    eft_area_spheres_00, eft_area_spheres_01, eft_area_spheres_02, eft_area_spheres_03,
    NULL, eft_area_spheres_04, NULL, NULL,
    NULL, eft_area_spheres_05, eft_area_spheres_06, NULL,
    eft_area_spheres_07,
};

EftAreaSphere* eft_map_area_spheres_1[13] = {
    eft_area_spheres_00, eft_area_spheres_01, eft_area_spheres_08, eft_area_spheres_09,
    NULL, eft_area_spheres_10, eft_area_spheres_11, eft_area_spheres_12,
    NULL, NULL, eft_area_spheres_06, NULL,
    eft_area_spheres_13,
};

EftAreaSphere* eft_map_area_spheres_2[12] = {
    eft_area_spheres_14, NULL, eft_area_spheres_15, eft_area_spheres_16,
    eft_area_spheres_17, NULL, NULL, eft_area_spheres_18,
    eft_area_spheres_19, NULL, NULL, NULL,
};

EftAreaSphere* eft_map_area_spheres_3[10] = {
    eft_area_spheres_20, eft_area_spheres_21, eft_area_spheres_22, eft_area_spheres_23,
    NULL, NULL, NULL, NULL,
    eft_area_spheres_24, eft_area_spheres_25,
};

EftAreaSphere* eft_map_area_spheres_4[10] = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, eft_area_spheres_26,
    NULL, NULL,
};

EftAreaSphere* eft_map_area_spheres_5[3] = {
    eft_area_spheres_27, NULL, NULL,
};

EftAreaSphere* eft_map_area_spheres_6[3] = {
    eft_area_spheres_28, NULL, NULL,
};

EftAreaSphere* eft_map_area_spheres_7[3] = {
    eft_area_spheres_29, NULL, NULL,
};

EftAreaSphere* eft_map_area_spheres_8[3] = {
    eft_area_spheres_30, eft_area_spheres_31, NULL,
};

/* The two-area table of one map: 8 bytes, so MWCC files it in `.sdata`. */
EftAreaSphere* eft_map_area_spheres_9[2] = {
    eft_area_spheres_32, eft_area_spheres_33,
};

EftAreaSphere** eft_map_area_table[22] = {
    NULL, eft_map_area_spheres_0, eft_map_area_spheres_2, NULL,
    eft_map_area_spheres_3, NULL, eft_map_area_spheres_5, eft_map_area_spheres_6,
    eft_map_area_spheres_8, eft_map_area_spheres_9, NULL, NULL,
    eft_map_area_spheres_1, NULL, NULL, eft_map_area_spheres_4,
    NULL, NULL, eft_map_area_spheres_7, NULL,
    NULL, NULL,
};

/* ------------------------------------------------------------------------------------------------ */

/* The state-0 handler: sets up the effect-model set and advances the state to 1. */
extern "C" void fn_80101DF4(EftState* self) {
    s32 i;
    EftModelSet* set;
    nw4r::math::VEC3 vec;
    void* unused_0x08;
    EftAreaSphere** area_table;

    set = self->set;
    ef_store_word(&unused_0x08, NULL);
    ef_resource_effect_project_at(ef_resource_instance(), 4);

    self->state++;

    set->records = eft_model_record_runs[self->type_id];
    set->field_08 = 66.0f;

    if (res_eft_model_create(set->main_char, 14, 16) == NULL) {
        fn_801025F8(self);
        return;
    }
    set->main_char->scale_0x1C.x = 7.2f;
    set->main_char->scale_0x1C.y = 7.2f;

    i = 0;
    while (set->records[i].effect_id != 0xFF) {
        if (i >= 12) {
            break;
        }
        if (res_eft_model_create(set->chars[i], eft_model_effect_ids[set->records[i].effect_id], 16) == NULL) {
            fn_801025F8(self);
            return;
        }
        set->chars[i]->scale_0x1C.x = set->records[i].scale;
        set->chars[i]->scale_0x1C.y = set->records[i].scale;
        i++;
    }

    set->field_44 = 0xFF;
    set->field_45 = 0xD0;
    set->field_46 = 0xE9;
    set->field_47 = 0x00;
    set->model_count = i;

    area_table = eft_map_area_table[get_now_mapno()];
    if (area_table != NULL) {
        set->area_spheres = area_table[self->area];
    }

    self->field_01 = 1;
    fn_802B00AC(&vec, self->area);
    copyVec3(&set->vec, &vec);
}


#pragma peephole on
#pragma pool_data reset


/* --- callees and pool constants the whole unit shares ------------------------------------------- */


/* ===================================================================================================
 * BLOCK A - the `em` (monster/effect) controller family, 0x80101FA4..0x80102994
 * =================================================================================================== */

/* --- types -------------------------------------------------------------------------------------- */

/* The GX colour record (`_GXColor`) comes from `gx.h` - one definition, in the owner's header
 * (rule 1). */

/* One emitter handle: the character record the effect drives.  Only the fields this block reads are
 * named; the rest is padding.
 * size: 0x24 - a lower bound; the record continues past what this block reads */
struct _EmHandle {
    /* +0x00 */ u32 pad_0x00;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ u8 pad_0x10[0xC];
    /* +0x1C */ f32 offset_x;
    /* +0x20 */ f32 offset_y;
};

/* One entry of the per-emitter table `EmEffectUnit::entries` (stride 0x14).
 * size: 0x14 */
struct EmEffectEntry {
    /* +0x00 */ u16 unused_0x00;
    /* +0x02 */ u8 intensity; /* 0..255, the emitter radius in pixels */
    /* +0x03 */ s8 follow;    /* 0: own position, 1: copy the first handle, 2: spawn no emitter */
    /* +0x04 */ f32 offset_x;
    /* +0x08 */ f32 offset_y;
    /* +0x0C */ f32 scale;
    /* +0x10 */ u8 color_r;
    /* +0x11 */ u8 color_g;
    /* +0x12 */ u8 color_b;
    /* +0x13 */ u8 alpha_base;
};

/* One entry of the per-emitter probe table `EmEffectUnit::probes` (stride 0x10): a candidate position
 * and the particle scale; a non-positive scale ends the table.
 * size: 0x10 */
struct EmEffectProbe {
    /* +0x00 */ nw4r::math::VEC3 pos;
    /* +0x0C */ f32 scale;
};

/* A line segment in nw4r space; the effect probes it for line of sight.
 * size: 0x18 */
struct EmEffectSegment {
    /* +0x00 */ nw4r::math::VEC3 from;
    /* +0x0C */ nw4r::math::VEC3 to;
};

/* Four positions a caller zeroes before the emitter is built; the effect uses them as a local frame.
 * size: 0x30 */
struct EmEffectQuad {
    /* +0x00 */ nw4r::math::VEC3 v[4];
};

/* The per-emitter record the controller's `+0x38` points at.  It is shared by every `em` shape, so the
 * fields are the union of the views those shapes take of it.
 * size: 0x60 - a lower bound */
struct EmEffectUnit {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 emitter_count;
    /* +0x08 */ f32 scale;      /* emitter radius multiplier */
    /* +0x0C */ f32 scale_rate; /* eased towards 1 while the effect is on the ground */
    /* +0x10 */ _EmHandle* handles[1]; /* `emitter_count` entries, then the tables below */
    /* +0x14 */ u8 pad_0x14[0x2C];
    /* +0x40 */ _EmHandle* self;
    /* +0x44 */ _GXColor color;
    /* +0x48 */ nw4r::math::VEC3 world_pos;
    /* +0x54 */ u8 part_flag;
    /* +0x55 */ u8 on_ground;
    /* +0x56 */ u8 pad_0x56[2];
    /* +0x58 */ EmEffectEntry* entries;
    /* +0x5C */ EmEffectProbe* probes;
};

/* The effect controller the unit's functions are handed.  It is allocated elsewhere with
 * `eft_res_slot_get(0x2C)` and holds the effect's state machine, its owner and two hooks.
 * size: 0x48 */
struct EmEffectWork {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 active;
    /* +0x02 */ u8 type;
    /* +0x03 */ u8 kind;
    /* +0x04 */ u8 unused_0x04;
    /* +0x05 */ u8 frame;
    /* +0x06 */ u8 state;
    /* +0x07 */ u8 last_emitting;
    /* +0x08 */ u8 emitting;
    /* +0x09 */ u8 pad_0x09[3];
    /* +0x0C */ s32 timer;
    /* +0x10 */ u8 pad_0x10[0x20];
    /* +0x30 */ void* owner;
    /* +0x34 */ void (*advance)(EmEffectWork*);
    /* +0x38 */ EmEffectUnit* unit;
    /* +0x3C */ u8 pad_0x3C[4];
    /* +0x40 */ void (*retire)(EmEffectWork*);
    /* +0x44 */ u8 area;
};

/* --- callees ------------------------------------------------------------------------------------ */
extern "C" f32 vec3_length_sq(const nw4r::math::VEC3* work);
extern "C" void subVec3(nw4r::math::VEC3* out, const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" f32 vec3_len(const nw4r::math::VEC3* in);
extern "C" f32 vec3_dot(const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" void vec3_normalize_into(nw4r::math::VEC3* v, const nw4r::math::VEC3* in);
extern "C" void vec3_scale(nw4r::math::VEC3* out, const nw4r::math::VEC3* in, f32 scale);
extern "C" void addVec3(nw4r::math::VEC3* out, const nw4r::math::VEC3* a, const nw4r::math::VEC3* b);
extern "C" void vec3_scale_in_place(nw4r::math::VEC3* v, f32 angle);
extern "C" void addVec3To(nw4r::math::VEC3* out, const nw4r::math::VEC3* in);
extern "C" void fn_80075258(s32* model, nw4r::math::VEC3* out, const nw4r::math::VEC3* pos);
extern "C" void fn_800FA3E8(EmEffectSegment* seg); /* seg = ((0,0,0), (0,0,0)) */
extern "C" void fn_800FA420(nw4r::math::VEC3* out); /* out = (0, 0, 0) */
extern "C" u8 fn_80290598(const EmEffectQuad* quad, const nw4r::math::VEC3* pos, s32 a, s32 b);
extern "C" s32 stage_water_enabled_ck(void);
extern "C" u32 fn_802B45D4(void);
extern "C" u32 fn_802BE39C(void);
extern "C" u8 get_option_21(void);
/* get_now_areano / get_now_mapno / eftGetKeyAlpha / vec_to_mh_vec3 come from `unsplit/unknown.h` as
 * their real declarations (rule 9).  get_camera_pos / get_camera_direction are NOT converted: their map
 * names are no-argument manglings (`__Fv`) but every call site passes an out pointer, so the only
 * declaration that reproduces the target's codegen is the `extern "C"` map spelling - the real
 * `nw4r::math::VEC3 get_camera_pos()` (struct return, sret) grows the frame 0x1C0 -> 0x1F0 and drops the
 * unit's score, so it is left and reported (rule 9, no real name expressible without a score change).
 * move/setTevKColor are `MHchar` members whose owner (the canonical `pl.h` struct) does not carry them
 * yet - left as the map spelling and reported. */
#include "camera/camera.h" /* get_camera_pos (rule 2: the owner's header) */
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_res_models_spawn_c1 ((void (*)(void*, void*, u32, s32, u32))eft_res_models_spawn)
/* The call keeps the out-pointer view (`void (VEC3*)`) the target's frame needs: a cast call to the owner's by-value `get_camera_pos` is the same direct call to the map symbol. */
#define get_camera_pos_c1 ((void (*)(nw4r::math::VEC3*))get_camera_pos)
extern "C" void get_camera_direction__Fv(nw4r::math::VEC3* out);
extern "C" void move__6MHcharFUs(_EmHandle* self, u32 motion);
extern "C" void setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(_EmHandle* self, u32 index, u32 id,
                                                                  _GXColor* color);


/* --- the functions, in address order ------------------------------------------------------------ */
extern "C" void fn_801025FC(EmEffectWork* work, u8 part, nw4r::math::VEC3* pos);
extern "C" void fn_801027D0(EmEffectWork* work);
extern "C" EmEffectQuad* fn_8010294C(EmEffectQuad* quad);

void vec_to_mh_vec3(nw4r::math::VEC3* dst, Vec* src);

#pragma fp_contract off
#pragma peephole off

extern "C" void fn_80101FA4(EmEffectWork* work) {
    nw4r::math::VEC3 v130; /* +0x130, the model's screen position */
    nw4r::math::VEC3 v124; /* +0x124, the camera-relative emitter position */
    nw4r::math::VEC3 v118; /* +0x118 */
    nw4r::math::VEC3 v10C; /* +0x10C */
    nw4r::math::VEC3 v100; /* +0x100 */
    nw4r::math::VEC3 vF4;  /* +0x0F4 */
    nw4r::math::VEC3 vE8;  /* +0x0E8 */
    nw4r::math::VEC3 vDC;  /* +0x0DC */
    nw4r::math::VEC3 vD0;  /* +0x0D0 */
    nw4r::math::VEC3 cam;  /* +0x0C4 */
    nw4r::math::VEC3 vB8;  /* +0x0B8 */
    nw4r::math::VEC3 vAC;  /* +0x0AC */
    nw4r::math::VEC3 vA0;  /* +0x0A0 */
    nw4r::math::VEC3 v94;  /* +0x094 */
    nw4r::math::VEC3 v88;  /* +0x088 */
    nw4r::math::VEC3 v7C;  /* +0x07C */
    nw4r::math::VEC3 v70;  /* +0x070 */
    nw4r::math::VEC3 v64;  /* +0x064 */
    nw4r::math::VEC3 v58;  /* +0x058 */
    nw4r::math::VEC3 v4C;  /* +0x04C */
    nw4r::math::VEC3 v40;  /* +0x040 */
    nw4r::math::VEC3 v34;  /* +0x034 */
    nw4r::math::VEC3 v28;  /* +0x028 */
    nw4r::math::VEC3 v1C;  /* +0x01C */
    nw4r::math::VEC3 v10;  /* +0x010 */
    _GXColor color;        /* +0x00C */
    s32 model;             /* +0x008 */
    f32 f31;
    f32 f29;
    s32 intensity;
    s32 alpha;
    u8 part;
    u32 i;
    EmEffectUnit* unit;
    f32 spread;
    f32 off_screen;
    f32 ease;
    f32 scale;

    VEC3_ctor(&v130);
    VEC3_ctor(&v124);
    VEC3_ctor(&v118);
    VEC3_ctor(&v10C);
    VEC3_ctor(&v100);
    VEC3_ctor(&vF4);
    VEC3_ctor(&vE8);
    VEC3_ctor(&vDC);
    VEC3_ctor(&vD0);
    unit = work->unit;
    if (work->area != get_now_areano()) {
        work->active = 0;
        work->frame++;
        return;
    }
    if (vec3_length_sq(&unit->world_pos) < 0.0001f) {
        return;
    }
    if (fn_802B45D4() == 1) {
        return;
    }
    if (stage_water_enabled_ck() != 0) {
        get_camera_pos_c1(&cam);
        copyVec3(&vD0, &cam);
        if (vD0.y < 0.0f) {
            return;
        }
    }
    get_camera_direction__Fv(&vB8);
    copyVec3(&v100, &vB8);
    get_camera_pos_c1(&vA0);
    subVec3(&vAC, &unit->world_pos, &vA0);
    copyVec3(&vF4, &vAC);
    model = scn_root_get_current_camera(pRoot);
    fn_80075258(&model, &v130, &unit->world_pos);
    f29 = vec3_dot(&v100, &vF4);
    vec3_normalize_into(&v100, &v100);
    vec3_normalize_into(&vF4, &vF4);
    get_camera_pos_c1(&v94);
    copyVec3(&v124, &v94);
    vec3_scale(&v88, &vF4, 50.0f);
    copyVec3(&vE8, &v88);
    addVec3To(&v124, &vE8);
    if (!(v130.x <= -100.0f) && !(v130.x >= 740.0f) && !(v130.y <= -100.0f)
        && !(v130.y >= 580.0f) && !(f29 < 0.0f)) {
        vec3_scale(&v64, &v100, vec3_dot(&vE8, &v100));
        get_camera_pos_c1(&v70);
        addVec3(&v7C, &v70, &v64);
        copyVec3(&v10C, &v7C);
        subVec3(&v58, &v10C, &v124);
        copyVec3(&vE8, &v58);
        vec3_scale_in_place(&vE8, 2.0f);
        addVec3(&v4C, &v124, &vE8);
        copyVec3(&v118, &v4C);
        f31 = (f32)(s32)vec3_len(&vE8);
        off_screen = 0.0f;
        if (v130.x >= 640.0f) {
            off_screen = v130.x - 640.0f;
        } else if (v130.x <= 0.0f) {
            off_screen = -v130.x;
        }
        if (v130.y >= 480.0f) {
            off_screen += v130.y - 480.0f;
        } else if (v130.y <= 0.0f) {
            off_screen += -v130.y;
        }
        ease = 1.0f - (0.01f * off_screen);
        if (ease < 0.0f) {
            ease = 0.0f;
        }
        intensity = (s32)(128.0f * ease);
        spread = 0.03125f;
        vec3_normalize_into(&vDC, &vE8);
        fn_801027D0(work);
        if (unit->scale_rate > 0.01f) {
            for (i = 0; i < unit->emitter_count; i++) {
                unit->handles[i]->offset_x = unit->entries[i].offset_x;
                unit->handles[i]->offset_y = unit->entries[i].offset_y;
                vec3_scale(&v28, &vE8, (f32)unit->entries[i].intensity);
                vec3_scale(&v34, &v28, spread);
                addVec3(&v40, &v124, &v34);
                copyVec3(&unit->handles[i]->pos, &v40);
                if (unit->entries[i].follow != 0) {
                    copyVec3(&unit->handles[i]->pos, &unit->handles[0]->pos);
                }
                vec3_scale(&v10, &vDC, unit->entries[i].scale);
                vec3_scale(&v1C, &v10, f31 / unit->scale);
                addVec3To(&unit->handles[i]->pos, &v1C);
                color.r = 0xFF - unit->entries[i].color_r;
                color.g = 0xFF - unit->entries[i].color_g;
                color.b = 0xFF - unit->entries[i].color_b;
                alpha = (u8)intensity - unit->entries[i].alpha_base;
                if (fn_802BE39C() == 1) {
                    alpha /= 2;
                }
                if (alpha <= 0) {
                    color.a = 0;
                } else {
                    color.a = (u8)((f32)alpha * unit->scale_rate);
                }
                setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(unit->handles[i], 0, 3, &color);
                move__6MHcharFUs(unit->handles[i], 0);
                if (unit->entries[i].follow != 2) {
                    eft_res_models_spawn_c1(work, &unit->handles[i], 2, 1, 0);
                }
            }
        }
        if (unit->on_ground == 1) {
            part = 0;
        } else {
            part = 1;
        }
    } else {
        part = 0;
    }
    if (fn_802BE39C() == 0) {
        work->emitting = 1;
    } else {
        work->emitting = 0;
    }
    fn_801025FC(work, part, &v124);
    work->last_emitting = work->emitting;
}

#pragma fp_contract on
#pragma peephole on

/* Counts the controller's own frames up; the effect family's per-frame tick. */
extern "C" void fn_801025E8(EmEffectWork* work) {
    work->frame++;
}

/* The family's "the effect is over" hook; forwards to the shared emitter teardown. */
extern "C" void fn_801025F8(void* self) {
    eft_res_slot_release(self);
}

#pragma peephole off

extern "C" void fn_801025FC(EmEffectWork* work, u8 part, nw4r::math::VEC3* pos) {
    nw4r::math::VEC3 local;
    EmEffectUnit* unit;

    VEC3_ctor(&local);
    unit = work->unit;
    unit->part_flag = part;
    if (work->emitting != work->last_emitting) {
        work->state = 3;
    }
    if (unit->part_flag == 1 && (work->state == 0 || work->state == 4)) {
        work->state = 1;
    }
    switch (work->state) {
    case 1:
        unit->color.a = 0;
        work->timer = 0;
        work->state++;
        /* fallthrough */
    case 2:
        unit->color.a = eftGetKeyAlpha(em_key_alpha_table, work->timer);
        work->timer++;
        if (work->timer > 52) {
            work->state++;
        }
        break;
    case 3:
        unit->color.a = 0;
        if (unit->part_flag == 0) {
            work->state = 4;
            work->timer = 7;
        }
        break;
    case 4:
        work->timer--;
        if (work->timer <= 0) {
            work->state = 0;
        }
        break;
    }
    if ((s32)work->type == 12) {
        unit->color.a = (u8)((u32)unit->color.a >> 1);
    }
    if (get_option_21() != 0) {
        unit->color.a = (u8)((u32)unit->color.a >> 1);
    }
    copyVec3(&unit->self->pos, pos);
    setTevKColor__6MHcharFUl14_GXTevKColorIDP8_GXColor(unit->self, 0, 3, &unit->color);
    move__6MHcharFUs(unit->self, 0);
    eft_res_models_spawn_c1(work, &unit->self, 2, 1, 0);
}

extern "C" void fn_801027D0(EmEffectWork* work) {
    EmEffectSegment seg;   /* +0x028, from the effect to the camera */
    EmEffectProbe point;   /* +0x018, the probe entry copied locally before it is tested */
    nw4r::math::VEC3 eye;  /* +0x008 */
    EmEffectQuad quad;     /* +0x040 */
    EmEffectUnit* unit;
    u8 hit;
    u32 i;

    unit = work->unit;
    fn_800FA3E8(&seg);
    fn_8010294C(&quad);
    fn_800FA420(&point.pos);
    if (unit->probes == 0) {
        unit->scale_rate = 1.0f;
        return;
    }
    copyVec3(&seg.from, &unit->world_pos);
    get_camera_pos_c1(&eye);
    copyVec3(&seg.to, &eye);
    pl_segment_box_set((const PlSegment*)&seg, (PlSegmentBox*)&quad);
    for (i = 0; unit->probes[i].scale > 0.0f; i++) {
        vec_to_mh_vec3(&point.pos, (Vec*)&unit->probes[i].pos);
        point.scale = unit->probes[i].scale;
        hit = fn_80290598(&quad, &point.pos, 0, 0);
        if (hit != 0) {
            break;
        }
    }
    if (hit != 0) {
        unit->on_ground = 1;
        if (unit->scale_rate > 0.0f) {
            unit->scale_rate -= 0.05f;
        }
        if (unit->scale_rate < 0.0f) {
            unit->scale_rate = 0.0f;
        }
    } else {
        unit->on_ground = 0;
        if (unit->scale_rate < 1.0f) {
            unit->scale_rate += 0.05f;
        }
        if (unit->scale_rate > 1.0f) {
            unit->scale_rate = 1.0f;
        }
    }
}

#pragma peephole on

/* Zeroes the four positions of a caller's frame; the effect's own vector initialiser. */
extern "C" EmEffectQuad* fn_8010294C(EmEffectQuad* quad) {
    VEC3_ctor(&quad->v[0]);
    VEC3_ctor(&quad->v[1]);
    VEC3_ctor(&quad->v[2]);
    VEC3_ctor(&quad->v[3]);
    return quad;
}
