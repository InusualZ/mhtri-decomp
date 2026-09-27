/* auto/80101DF4_fn_80101DF4.cpp - the effect-state machine's state-0 handler: `fn_80101DF4`,
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * `.text` 0x80101DF4-0x80101FA4.
 *
 * One C++ TU of the `ef` (effect) library, `auto` / `Wii/1.3` / `cflags_main` (`-O3 -inline noauto`).
 * The object's language is C++ (langcheck: both callees are mangled), so the file is `.cpp` and the
 * map's plain `fn_80101DF4` definition carries `extern "C"` (playbook 42).
 *
 * What it is: the state-0 handler of the effect-state machine `fn_80101DB8` dispatches on
 * (`EftState::state`, +0x05).  It advances the state to 1 and fills the state's `EftModelSet`
 * (`EftState::set`, +0x38): the per-type effect-record run `lbl_8059CF78[type_id]`, the set's scale,
 * the main character's effect (fixed id 14) and one effect per record of the run (id through the
 * shared table `lbl_8059CFB0`, scale copied into the model's +0x1C/+0x20).  A failed
 * `res_eft_model_create` bails out through `fn_801025F8` (a tail call to the library's error path
 * `fn_800F886C`).
 *
 * Flags: retail keeps the peephole's `clrlwi r0,r3,24` + `slwi r0,r0,2` pair unfused at the
 * `get_now_mapno()` index, so the file needs `#pragma peephole off` (playbook 39); with the peephole
 * on the function is 97.87 %.
 *
 * The source shape that is load-bearing:
 *   * the record cursor is the **source's** `i * sizeof(EftModelRecord)` element index - MWCC
 *     strength-reduces it into the separate byte-offset register retail has (`addi r27,r27,20`).
 *     Declaring that offset as its own local instead swaps it with the `chars` index pointer and costs
 *     0.56 points (playbook 19/20).
 *
 * Pool: the two scale constants are literals, so this object emits the unit's 8-byte `.sdata2`
 * (`42840000 40e66666`, i.e. 66.0f then 7.2f), and the DOL holds exactly those bytes at
 * 0x807966E8-0x807966F0.  That range is claimed: it is the unit's **own** pool entry, so no other
 * registered unit's object references it, the claim pairs the target object's `.sdata2` with ours and
 * the unit is linked (`Object(Matching, ...)`).  A claim on a *shared* pool entry does not work this
 * way - `Pl/fn_8026FFBC.cpp` is the measured counter-example.
 *
 * The types below are shared with the sibling state handlers of this library (`fn_80101DB8`,
 * `fn_80101C74`, `fn_80101D70` are the evidence for `EftState`); they move to a shared `ef` header the
 * second time another unit needs them (rule 1).
 *
 * Residual: none - all 108 rows match, `.text` 0x1B0/0x1B0 and `extab`/`extabindex` byte-equal.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/80101DF4_fn_80101DF4.cpp`.
 * The name is provisional (`auto/` plus the first symbol's address) because nothing in the object names
 * the original source file.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/eft007.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

#pragma peephole off

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The effect owner the library's `res_eft_model_create` works on.  This unit only sets its two scale
 * floats; the class is larger than this lower bound (the create path also writes +0x35 and +0x10C), so
 * the tail is opaque.
 * size: 0x110 (lower bound - `res_eft_model_create_light` writes +0x10C) */
struct MHchar {
    /* +0x000 */ u8 pad_0x000[0x1C];
    /* +0x01C */ f32 scale_x; /* set from the effect record once the model exists */
    /* +0x020 */ f32 scale_y;
    /* +0x024 */ u8 pad_0x024[0xEC];
};

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
 * `fn_800F8788`.  `fn_80101D70` releases `chars[0..char_count-1]` and then `main_char`. */
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

/* The effect-state record `fn_800F8788` hands out of its pool (record stride 0x48).  `fn_80101DB8` is
 * installed at +0x34 as the state dispatcher and `fn_80101D70` at +0x40 as the update handler;
 * `fn_80101C74` is the constructor and `fn_800F886C` the release path. */
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
extern "C" void fn_800A8998(void** dst, void* value);
/* Returns the shared `lbl_80694598` block. */
extern "C" void* fn_800B2878(void);
/* Bounds-checks `index` against `obj`'s +0x08 count, then forwards to `fn_80501C9C`. */
extern "C" void fn_800B4A70(void* obj, u16 index);
/* The `nw4r::math::VEC3` copy the map still spells `copyVec3`. */
/* The library's error path (`fn_800F886C`). */
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
    fn_800A8998(&unused_0x08, NULL);
    fn_800B4A70(fn_800B2878(), 4);

    self->state++;

    set->records = (EftModelRecord*)lbl_8059CF78[self->type_id];
    set->field_08 = 66.0f;

    if (res_eft_model_create(set->main_char, 14, 16) == NULL) {
        fn_801025F8(self);
        return;
    }
    set->main_char->scale_x = 7.2f;
    set->main_char->scale_y = 7.2f;

    i = 0;
    while (set->records[i].effect_id != 0xFF) {
        if (i >= 12) {
            break;
        }
        if (res_eft_model_create(set->chars[i], lbl_8059CFB0[set->records[i].effect_id], 16) == NULL) {
            fn_801025F8(self);
            return;
        }
        set->chars[i]->scale_x = set->records[i].scale;
        set->chars[i]->scale_y = set->records[i].scale;
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
