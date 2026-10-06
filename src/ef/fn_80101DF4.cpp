/* ef/fn_80101DF4.cpp - the state-0 handler of the effect-state machine `ef/eft004_fx.cpp`'s `fn_80101DB8` dispatches
 *   on: it advances the state and fills the state's `EftModelSet` (the per-type record run `lbl_8059CF78[type_id]`,
 *   the main character's effect and one effect per record); a failed `res_eft_model_create` bails out through
 *   `ef/em_effect_ctrl.cpp`'s `fn_801025F8`.
 * RANGE. .text 0x80101DF4-0x80101FA4 (1 function); extab 0x8000BEF4-0x8000BEFC, extabindex 0x80025D58-0x80025D64,
 *   .data 0x8059C8E8-0x8059DAA0, .sdata2 0x807966E8-0x807966F0 (66.0f and 7.2f, the unit's own literals).
 * FLAGS. `cflags_main`; `#pragma peephole off` (retail keeps `clrlwi r0,r3,24` + `slwi r0,r0,2` unfused at the
 *   `get_now_mapno()` index; playbook 39).
 * NAMES. The map has only the `fn_` stem, so the definition is `extern "C"`; the unit is C++ (both callees mangled).
 * RESIDUALS. none in code: the row matches and the `.sdata2` pair is emitted.
 *   flipcheck: `.data` claimed, not emitted (the target relocates against `lbl_8059CF78` and the `lbl_8059D9xx`
 *   tables, ours against extern declarations).
 * SHAPES. The record cursor is the source's `i * sizeof(EftModelRecord)` index, which MWCC strength-reduces into
 *   retail's separate byte-offset register (`addi r27,r27,20`); a byte-offset local swaps it with the `chars`
 *   pointer (playbook 19/20).
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/ef_emitter.h" /* ef_store_word (rule 2) */
#include "pl.h"
#include "ef/eft007.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* `MHchar`, the effect owner the library's `res_eft_model_create` works on, comes from `pl.h` (rule 1: one definition). */

/* One entry of the 0xFF-terminated run `EftModelSet::records` points at, 0x14 B.  Only the id and the
 * scale are read here; the rest of the entry is the create path's (the DOL's own records carry 1..15 in
 * +0x01..+0x03 and floats in +0x08/+0x0C, so those bytes are data, not padding). */
struct EftModelRecord {
    /* +0x00 */ u8 effect_id; /* index into the shared effect-id table `lbl_8059CFB0`; 0xFF ends the run */
    /* +0x01 */ u8 unused_0x01[3];
    /* +0x04 */ f32 scale; /* copied into the model's +0x1C and +0x20 */
    /* +0x08 */ f32 unused_0x08;
    /* +0x0C */ f32 unused_0x0C;
    /* +0x10 */ u32 unused_0x10;
}; /* size: 0x14 */

/* The per-state model set `fn_800F8B44` allocates; `fn_80101C74` passes 0x60 as its size to
 * `eft_res_slot_get`.  `fn_80101D70` releases `chars[0..char_count-1]` and then `main_char`. */
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
    /* +0x5C */ u32 field_5C; /* per-map, per-area value */
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
/* fn_801025F8 comes from its owner's header (rule 2). */
/* Builds the current area's effect vector from the per-map table at 0x806C87C0. */
extern "C" void fn_802B00AC(nw4r::math::VEC3* out, u8 area);

/* Creates the `id` effect model on `chr`; NULL on failure. */
void* res_eft_model_create(MHchar* chr, u16 id, u32 arg);
/* The current map number, narrowed to a byte by the caller. */
u8 get_now_mapno(void);

/* The unit's pooled data, owned by the map (playbook 29): the per-type record-run table, the shared
 * effect-id table and the per-map area tables. */
extern u32 lbl_8059CF78[];  /* 14 record-run pointers, indexed by `EftState::type_id` */
extern u16 lbl_8059CFB0[];  /* effect ids, indexed by `EftModelRecord::effect_id` */
extern u32* lbl_8059DA48[]; /* per-map area tables, indexed by `get_now_mapno()` */

/* ------------------------------------------------------------------------------------------------ */

/* The state-0 handler: sets up the effect-model set and advances the state to 1. */
extern "C" void fn_80101DF4(EftState* self) {
    s32 i;
    EftModelSet* set;
    nw4r::math::VEC3 vec;
    void* unused_0x08;
    u32* area_table;

    set = self->set;
    ef_store_word(&unused_0x08, NULL);
    ef_resource_effect_project_at(ef_resource_instance(), 4);

    self->state++;

    set->records = (EftModelRecord*)lbl_8059CF78[self->type_id];
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
        if (res_eft_model_create(set->chars[i], lbl_8059CFB0[set->records[i].effect_id], 16) == NULL) {
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

    area_table = lbl_8059DA48[get_now_mapno()];
    if (area_table != NULL) {
        set->field_5C = area_table[self->area];
    }

    self->field_01 = 1;
    fn_802B00AC(&vec, self->area);
    copyVec3(&set->vec, &vec);
}
