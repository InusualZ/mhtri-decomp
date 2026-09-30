/* ef/eft019.cpp - the `eft019` effect family, `.text` 0x801121DC..0x80114E34 (24 functions).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>`: the runtime dump resolves only the nine `eft019_*`
 * names - `eft019_set`, `eft019_set_core`, `_vec`, `_brethend`, `_tyakudan`, `_mazule`, `_subtype`,
 * `_ring`, `_smoke` - and every other address in the range is the dump's placeholder `zz_XXXXXXXX_`).
 *
 * Registration (docs/plan.md 12).  Class 2 evidence named it: the runtime dump's own name at
 * 0x801121DC is `eft019_set`, so the module is `ef` and the file is `eft019.cpp` - the scheme of its
 * neighbours `ef/eft001.cpp` ... `ef/eft009.cpp`.  Language: the object's own definition
 * `eft019_set__FPQ34nw4r4math4VEC3UcUc` is a C++ mangling (a `nw4r::math::VEC3*` plus `u8/u8`), so the
 * file is `.cpp` and every definition whose map name is plain (`fn_XXXXXXXX`) is `extern "C"` so its
 * emitted name stays the map's stem and objdiff can pair it (playbook row 42).  `extab` (0xA8 = 21
 * records) and `extabindex` (0xFC = 21 x 12 B) are the ranges the two neighbouring blocks leave
 * unclaimed; no `.ctors` word belongs to the range (no auto-split object of it carries one).
 *
 * What it is.  `fn_80114A1C` is the family allocator: it rejects a foreign area, runs
 * `fn_800F8788(0x58)` for the 0x48-byte `_EFT` record plus its 0x58-byte work block at +0x38, stamps
 * `field_0x03 = 19` (the eft019 family tag) and `type_0x02` with the caller's id, seeds the work
 * pool's capacity from the per-id count table `lbl_8059F670`, and installs the two hooks that travel
 * with the record - `fn_80112D1C` (the per-frame dispatcher, `dispatch_0x34`) and `fn_80112CE0` (the
 * pool release, `release_0x40`).  The `eft019_set_*` functions are the spawn entry points: each picks
 * an id, calls the allocator, fills `pos_0x18`/`rot_0x24`/`timer_0x0C`, and stores its variant id and
 * two scale factors into the work block.
 *
 * The state machine is `fn_80112D1C`: `state_0x05` 0 -> `fn_80112D58` (create the pooled effect
 * objects and place them), 1 -> `fn_8011392C` (the per-frame update), 2 -> `fn_8011484C` (advance the
 * state), 3 -> `fn_8011485C` (destroy the record through `fn_800F886C`).
 *
 * Work block.  The effects go through one `res_eft_create(u16 id, u16 param, u32 mode)` pool whose
 * capacity is the id's `lbl_8059F670` entry (maximum 7) and whose used length is `live_0x3C` while
 * `fn_80112D58` builds it, then `count`; `fn_80112CE0` hands the whole array back with
 * `push_eft_effect_heap_num` and `fn_8011392C` reaps the dead entries with `fn_800F9884` /
 * `RetireEffect`.
 *
 * Types.  `nw4r::math::VEC3`/`MTX34` come from `nw4r/math.h`; `_EFT`, `_CP_VECTOR` and the
 * `nw4r::ef::Effect` class from `ef.h`; `_PLW` and `MHchar` from `pl.h`.  The work block is typed per
 * family, so it is a unit-local `_EFT019_WORK` cast from `_EFT::work_0x38` (the pattern
 * `ef/eft001.cpp` uses for its `_EFT001_EFFECT_WORK`), and the actor record the setters hang effects
 * off is viewed through a unit-local `_EFT019_ACTOR`: the canonical `enemy.h` names the +0x1E1 byte
 * `act_id` while every ef consumer reads it as the area number (`ef/eft007.cpp` `area_no`,
 * `ef/eft009.cpp` `effect_type_0x1E1`, `enemy/fn_8012BA00.c` `area_no`, "the same value on both
 * records means 'same area'"), and pads +0x016/+0x1A4/+0x110 which this unit reads.
 *
 * Data.  The unit owns no pool section (the target object carries none): the `.data` tables, the
 * `.sdata` pointer and the `.sdata2` constants are `extern`-declared by their map names and never
 * defined (playbook 29).
 *
 * Result.  23 of the 24 functions are reconstructed; 16 are byte-identical and every one measures at
 * or above 80 % (size-weighted 95.01 % over the 7480 reconstructed bytes, 62.60 % of the range's
 * 11352).  Retail's frame/register layout pins three source shapes that are not the obvious ones:
 * the `_CP_VECTOR*` setters copy the rotation triple with word reads (`effect->rot_0x24 = *rot;`,
 * never through `nw4r::math::VEC3*` - that emits an `fctiwz` per component), `eft019_set` needs one
 * `VEC3` per camera case (nine of them; a case-local reuses one slot) and `fn_80114A1C` writes its two
 * seeds with a chained assignment so the constant is loaded once and 0x48 is stored first.
 *
 * Residuals.  `fn_8011392C` (0x8011392C, 3872 B) is not written - the family's per-frame update, the
 * one function of the range still missing; `m2c` decompiles it (`build/tmp/eft019/all.m2c.c`
 * lines 1019-1700) and the work-block field table above comes from it.  Of the 23 written:
 * `eft019_set` 98.14 (one case's branch layout), `eft019_set_brethend` 97.73 and `eft019_set_mazule`
 * 97.96 (the two bodies of their chains sit in the other order), `eft019_set_subtype` 98.14 (switch
 * tail layout), `fn_80112D58` 91.84 (`eft_control`, a `.bss` global, is addressed sda21 here where
 * retail emits `lis`/`addi`), `fn_80112B7C` 85.59 (retail puts the `type_0x02 = 103` body out of line
 * and neither the `else-if` nor the empty-`then` spelling reproduces it - rule 8 forbids forcing it
 * with a `goto`) and `fn_801148E4` 82.42 (its local frame is 0x10 larger than retail's: retail keeps
 * the projected vector at +0x20, the clip pair at +0x18, the colour word at +0x08 and the camera
 * handle at +0x0C).
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit ef/eft019.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "ef/effect.h"
#include "ef/fn_80114E34.h"
#include "g3d/g3d_calcworld.h"
#include "g3d/g3d_camera.h"
#include "main.h"
#include "Pl/pl_master.h"
#include "unsplit/unknown.h"
#include "unsplit/g3d.h"
#include "sound/fn_800D7F54.h"
#include "ef/ef_particlemanager.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the unit's own prototypes (plain C linkage: the map's stems must stay unmangled)
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void fn_80112B7C(void* source, u8 key, u8 subtype, nw4r::math::VEC3* pos, _CP_VECTOR* rot,
                 f32 scale);
void fn_80112CE0(_EFT* self);                                    /* pool release (`release_0x40`) */
void fn_80112D1C(_EFT* self);                                    /* per-frame dispatch (`dispatch_0x34`) */
void fn_80112D58(_EFT* self);                                    /* state 0: create and place */
void fn_8011392C(_EFT* self);                                    /* state 1: per-frame update */
void fn_8011484C(_EFT* self);                                    /* state 2: advance */
void fn_8011485C(_EFT* self);                                    /* state 3: destroy */
void fn_80114860(nw4r::math::VEC3* pos, u8 area);                /* the weapon-side `tyakudan` record */
void fn_801148E4(_EFT* self);                                    /* the on-screen impact sound */
_EFT* fn_80114A1C(nw4r::math::VEC3* pos, u8 area, u8 type);       /* the family allocator */
void fn_80114B20(_EFT* self, nw4r::math::MTX34* mtx);            /* place the record on its actor */
void ef_inst_spawn(void* actor, u8 key);                           /* eft020 wrappers, per actor kind */
void fn_80114C6C(void* actor, u8 key, s32 joint);
void fn_80114CC8(void* actor, u8 key);
void fn_80114D28(void* actor, u8 kind, u8 type, f32 scale);

/* unsplit ef-band helpers (the band's bracketing units disagree, so they have no owner header) */
void fn_80050850(nw4r::math::VEC3* v, const nw4r::math::VEC3* in);
void fn_800513F0(nw4r::math::VEC3* v, f32 angle);
void fn_8005696C(s32 id, s32 kind, s32 mode, s32* color, s32 timer, f32 x, f32 y);
void fn_80056A20(f32 a, f32 b);
_EFT* fn_800F8788(u32 pool);
void fn_800F886C(_EFT* self);
void fn_80306D6C(void* self, s32 a, void* b, void* c, u8 d, f32 e);
u8 fn_80331210(void* self);
void fn_800E0A14(void* chr, u32 joint, nw4r::math::MTX34* out);

#ifdef __cplusplus
}
#endif

/* The mangled callees and this unit's own mangled setters, declared with the signatures their map
 * names encode (rule 9: the real signature, never the mangled spelling as an identifier).  The target
 * object references event_demo_ck__Fv, so it is declared C++ here (relocaudit). */
u32 event_demo_ck(void);
void eft019_set(nw4r::math::VEC3* pos, u8 area, u8 type);
void eft019_set_core(nw4r::math::VEC3* pos, u8 area, u8 type, f32 scale_a, f32 scale_b);
void eft019_set_vec(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale);
void eft019_set_brethend(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale);
void eft019_set_tyakudan(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale);
void eft019_set_mazule(u8 id, nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot, _PLW* plw);
void eft019_set_subtype(u8 type, u8 subtype, nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot,
                        s32 timer, f32 scale);
void eft019_set_ring(nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot);
void eft019_set_smoke(nw4r::math::VEC3* pos, u8 area, u8 type, s32 timer);

/* The effect-manager id table and the sound-object hooks `fn_80112D58` case 0x88 drives: all
 * unsplit ef-band symbols the target references by their plain map names, so they carry C
 * linkage (relocaudit). */
extern "C" {
void** fn_800A4420(s32 id);
void* fn_800A51D8(nw4r::ef::Effect* effect, u32 arg);
u16 fn_800A970C(void);
void* fn_800A9714(void* obj, u16 index);
void fn_800A67E8(void* obj, void* value);
}

/* The object `fn_800A4420` returns: a vtable word at +0 and the 4th virtual slot at +0x0C, which
 * retail calls with the object itself as `this`. size: 0x04 */
struct _EFT019_SND_OBJ {
    /* +0x00 */ void** vtable;
};
struct _EFT019_SND_VTABLE {
    /* +0x00 */ void (*v0)(void);
    /* +0x04 */ void (*v1)(void);
    /* +0x08 */ void (*v2)(void);
    /* +0x0C */ void (*slot_0x0C)(_EFT019_SND_OBJ* self);
}; /* size: 0x10 */

/* The effect manager's control block `eft_control` (`.bss` 0x8062C000, 0xC44 B in `ef/effect.cpp`).
 * Only the +0x04 word this unit hands to `fn_800A4420` is named, but the type carries the record's
 * full size: MWCC picks sda21 addressing for a small extern it cannot see defined, and the record is
 * 0xC44 bytes (retail's `lis`/`addi` pair). size: 0xC44 */
struct _EFT019_CTRL {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ s32 field_0x04;
    /* +0x08 */ u8 pad_0x08[0xC44 - 0x08];
};
extern _EFT019_CTRL eft_control;

/* `calcVecAngXY(VEC3*, u32*, u32*)` and `rotVecY(VEC3*, u32)` are map-mangled free functions with no
 * registered owner; declaring them at global scope reproduces their map spellings exactly. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* The camera entries.  The map spells both `__Fv`, but every call site in this unit (and in
 * `ef/eft007.cpp`, which records the same finding) passes an out pointer, so the declaration carries
 * the pointer; the relocation name that emits differs from the map's `__Fv` spelling, which the
 * report's metric ignores (playbook 23). */
nw4r::math::VEC3 get_camera_direction(void); /* target references get_camera_direction__Fv (relocaudit) */
void get_camera_pos(nw4r::math::VEC3* out);

/* `get_enemy_data`'s map spelling is `get_enemy_data__FP11_ENEMY_WORK`; the tag is forward-declared
 * here so the declaration emits that exact mangling without pulling in `enemy.h`'s record view. */
struct _ENEMY_WORK;
void* get_enemy_data(struct _ENEMY_WORK* enemy);

/* ---------------------------------------------------------------------------------------------------
 * engine records
 * ------------------------------------------------------------------------------------------------- */

/* The effect pool the eft019 record carries at `_EFT::work_0x38`.  The size is `fn_800F8788`'s
 * argument, 0x58.  `count` is how many effects the id asks for (`lbl_8059F670[type]`, whose maximum
 * entry is 7), `live_0x3C` how many the creation path has built so far - `fn_80112D58` copies it into
 * `count` when it runs out, `fn_8011392C` reaps over it.  The per-variant state the setters leave
 * behind is `subtype_0x40` (the id `fn_80112D58` switches on to pick the colour table), the two scale
 * factors and the tint. size: 0x58 */
struct _EFT019_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[13];
    /* +0x38 */ void* unused_0x38;   /* cleared by the allocator; no function of this unit reads it */
    /* +0x3C */ s32 live_0x3C;
    /* +0x40 */ u8 subtype_0x40;
    /* +0x41 */ u8 pad_0x41[0x3];
    /* +0x44 */ f32 scale_a_0x44;
    /* +0x48 */ f32 scale_b_0x48;
    /* +0x4C */ _GXColor color_0x4C[3];  /* one tint per effect slot; `lbl_8059FB40` has 3 rows */
};

/* The eft020 sibling's work block.  `src/ef/fn_80114E34.cpp` owns the full view (its `_EFT_WORK_A`);
 * this unit only writes the joint id the sibling's `+0x18` carries. size: 0x20 */
struct _EFT020_WORK_A_JOINT {
    /* +0x00 */ u8 pad_0x00[0x18];
    /* +0x18 */ s32 joint_0x18;
    /* +0x1C */ u8 pad_0x1C[0x4];
};

/* The actor record a `fn_80114C*`/`fn_80114D28` wrapper hangs its effect off, as this unit reads it.
 * One record, several views: +0x016 is `pl.h`'s `_PLW::area_0x16`, +0x1E1 the area both the player and
 * the enemy record carry, +0x1A4 `pl.h`'s `_PLW::effect_key_0x1A4`, +0x204 the enemy's joint id
 * (`ef/eft001.cpp` `joint_0x204`) and +0x110 the word `fn_80114D28` falls back to when it is -1.
 * +0x009 is the kind byte the mazule path compares against 3 (`pl.h` still spells it `unk009`,
 * `enemy.h` names the same byte `state_0x009`). size: 0x208 - lower bound (the record continues past
 * what this unit reads). */
struct _EFT019_ACTOR {
    /* +0x000 */ u8 pad_0x000[0x009];
    /* +0x009 */ u8 kind_0x09;
    /* +0x00A */ u8 pad_0x00A[0x016 - 0x00A];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 pad_0x017[0x110 - 0x017];
    /* +0x110 */ u32 field_0x110;
    /* +0x114 */ u8 pad_0x114[0x1A4 - 0x114];
    /* +0x1A4 */ u8 effect_key_0x1A4;
    /* +0x1A5 */ u8 pad_0x1A5[0x1E1 - 0x1A5];
    /* +0x1E1 */ u8 area_no_0x1E1;
    /* +0x1E2 */ u8 pad_0x1E2[0x204 - 0x1E2];
    /* +0x204 */ u32 joint_0x204;
};

/* The physics sub-record the actor holds at +0x13C: its character model sits at +0x04.  `pl.h`
 * forward-declares the same layout as `_PLW_PHYSICS` (owned by `ef/fn_80114E34.cpp`), so this unit
 * carries only the offset it walks. size: 0x144 - lower bound. */
struct _EFT019_PHYSICS {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ MHchar chr_0x04;
};

/* The per-enemy data record `get_enemy_data` returns.  `enemy.h`'s canonical `EnemyData` leaves +0x9C
 * padding; this unit dereferences it and reads a float at +0x1C of what it points at. */
struct _EFT019_EMY_SCALE {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ f32 scale_0x1C;
}; /* size: 0x20 - lower bound (only the scale this unit reads is named) */
struct _EFT019_EMY_DATA {
    /* +0x00 */ u8 pad_0x00[0x9C];
    /* +0x9C */ _EFT019_EMY_SCALE* field_0x9C;
}; /* size: 0xA0 */

/* --- pooled data (declared, never defined: the pool belongs to the data pass) -------------------- */

extern s32 pRoot;          /* the model manager `fn_80075258` projects through (.sdata 0x807918E8) */
extern u8 lbl_8059F670[];  /* per-id effect count, indexed by `type_0x02` (.data 0x8059F670, 0x8C) */
extern u16 lbl_8059F6FC[]; /* the effect id table `res_eft_create` is fed (.data 0x8059F6FC, 0x114) */
extern u16 lbl_8059F810[]; /* its second field (.data 0x8059F810, 0x114) */
extern u16 lbl_8059F924[]; /* the two-model ids (.data 0x8059F924) */
extern s32 lbl_8059FB40[]; /* the three colour-table pointers the variant switch picks (.data) */
extern u8 lbl_8059FB4C[];  /* one 0x0C-byte colour record per variant (.data 0x8059FB4C) */
extern u8 lbl_8059FB5C[];  /* ditto */
extern u8 lbl_8059FB68[];  /* ditto */
extern u8 lbl_8059FB7C[];  /* ditto */
extern u8 lbl_8059FB88[];  /* ditto */
extern u8 lbl_8059FBB8[];  /* ditto */
extern u8 lbl_8059FBC4[];  /* ditto */
extern u8 lbl_8059FBD0[];  /* ditto */
extern u8 lbl_8059FBDC[];  /* ditto */

extern f32 lbl_807969D8; /* .sdata2 0x807969D8 .. 0x80796A24, the scale/colour constants */
extern f32 lbl_807969DC;
extern f32 lbl_807969E0;
extern f32 lbl_807969E4;
extern f32 lbl_807969E8;
extern f32 lbl_807969EC;
extern f32 lbl_807969F0;
extern f32 lbl_807969F4;
extern f32 lbl_807969F8;
extern f32 lbl_807969FC;
extern f32 lbl_80796A00;
extern f32 lbl_80796A04;
extern f32 lbl_80796A08;
extern f32 lbl_80796A0C;
extern f32 lbl_80796A10;
extern f32 lbl_80796A14;
extern f32 lbl_80796A18;
extern f32 lbl_80796A20;
extern f32 lbl_80796A24;

/* Playbook row 39: the peephole pass folds the u8 argument truncations and the byte masks that retail
 * keeps (`clrlwi r4,r5,24` before every `fn_80114A1C` call, `clrlwi r0,r6,24` before the `type - 3`
 * compares), and it fuses the record-form compares.  With the pass on, `eft019_set_core` loses both
 * `clrlwi`s (80 B vs the target's 84) and `eft019_set_vec` / `eft019_set_tyakudan` / `eft019_set_smoke`
 * lose the same pair; off, every one of them is byte-identical.  Measurement: see the unit report. */
#pragma peephole off

/* ===================================================================================================
 * 0x801121DC  eft019_set(nw4r::math::VEC3* pos, u8 area, u8 type)
 * =================================================================================================== */

void eft019_set(nw4r::math::VEC3* pos, u8 area, u8 type)
{
    nw4r::math::VEC3 v;
    nw4r::math::VEC3 d0, d1, d2, d3, d4, d5, d6, d7, d8;
    _EFT* effect;

    VEC3_ctor(&v);
    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect == 0) {
        return;
    }

    switch (type) {
    case 0: {
        d0 = get_camera_direction();
        copyVec3(&v, &d0);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 12, lbl_807969DC, lbl_807969DC);
        return;
    }
    case 1: {
        _EFT019_WORK* work;
        d1 = get_camera_direction();
        copyVec3(&v, &d1);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 12, lbl_807969E0, lbl_807969E0);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = lbl_807969DC;
        work->scale_a_0x44 = lbl_807969DC;
        return;
    }
    case 2: {
        _EFT019_WORK* work;
        d2 = get_camera_direction();
        copyVec3(&v, &d2);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 12, lbl_807969E4, lbl_807969E4);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = lbl_807969E8;
        work->scale_a_0x44 = lbl_807969E8;
        return;
    }
    case 14: {
        d3 = get_camera_direction();
        copyVec3(&v, &d3);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 13, lbl_807969DC, lbl_807969DC);
        return;
    }
    case 15: {
        d4 = get_camera_direction();
        copyVec3(&v, &d4);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 13, lbl_807969E0, lbl_807969E0);
        return;
    }
    case 16: {
        d5 = get_camera_direction();
        copyVec3(&v, &d5);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 13, lbl_807969EC, lbl_807969EC);
        return;
    }
    case 23: {
        _EFT019_WORK* work;
        eft019_set_core(pos, area, 12, lbl_807969F0, lbl_807969F0);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = lbl_807969F4;
        work->scale_a_0x44 = lbl_807969F4;
        return;
    }
    case 28: {
        _EFT019_WORK* work;
        d6 = get_camera_direction();
        copyVec3(&v, &d6);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 12, lbl_807969E8, lbl_807969E8);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = lbl_807969E8;
        work->scale_a_0x44 = lbl_807969E8;
        return;
    }
    case 29: {
        _EFT019_WORK* work;
        d7 = get_camera_direction();
        copyVec3(&v, &d7);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 13, lbl_807969E8, lbl_807969E8);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = lbl_807969E8;
        work->scale_a_0x44 = lbl_807969E8;
        return;
    }
    case 57: {
        d8 = get_camera_direction();
        copyVec3(&v, &d8);
        fn_80050850(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 58, lbl_807969F8, lbl_807969F8);
        break;
    }
    }
}

/* ===================================================================================================
 * 0x80112610  eft019_set_core(nw4r::math::VEC3* pos, u8 area, u8 type, f32 scale_a, f32 scale_b)
 * =================================================================================================== */

void eft019_set_core(nw4r::math::VEC3* pos, u8 area, u8 type, f32 scale_a, f32 scale_b)
{
    _EFT* effect;
    _EFT019_WORK* work;

    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect != 0) {
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = scale_a;
        work->scale_b_0x48 = scale_b;
    }
}

/* ===================================================================================================
 * 0x80112664  eft019_set_vec(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale)
 * =================================================================================================== */

void eft019_set_vec(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale)
{
    _EFT* effect;
    _EFT019_WORK* work;

    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect != 0) {
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = scale;
        work->scale_b_0x48 = scale;
        fn_800FC0D4(&effect->rot_0x24, rot);
    }
}

/* ===================================================================================================
 * 0x801126C4  eft019_set_brethend(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 s)
 * =================================================================================================== */

void eft019_set_brethend(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale)
{
    _EFT* effect;
    _EFT019_WORK* work;

    /* `type` arrives in r6 and is the register the result lives in on the path where no id matches, so
     * retail carries the raw id into the `effect != 0` test.  The three ids this function is called
     * with are the ones below (3..5 -> 11, 18/21/22/83 -> 17); the fall-through cannot be reached. */
    effect = (_EFT*)type;
    {
        u8 t = (u8)type;
        if ((u32)(t - 3) > 2) {
            if ((u32)(t - 21) <= 1 || (s32)t == 18 || (s32)t == 83) {
                effect = fn_80114A1C(pos, area, 17);
            }
        } else {
            effect = fn_80114A1C(pos, area, 11);
        }
    }
    if (effect != 0) {
        effect->rot_0x24 = *rot;
        effect->rot_0x24.y += 0x8000;
        work = (_EFT019_WORK*)effect->work_0x38;
        work->subtype_0x40 = type;
        fn_80114860(&effect->pos_0x18, effect->area_0x44);
        work->scale_b_0x48 = scale;
        work->scale_a_0x44 = scale;
    }
}

/* ===================================================================================================
 * 0x801127A4  eft019_set_tyakudan(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 s)
 * =================================================================================================== */

void eft019_set_tyakudan(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale)
{
    _EFT* effect;
    _EFT019_WORK* work;

    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect != 0) {
        effect->rot_0x24.x = 0;
        effect->rot_0x24.y = rot->y;
        effect->rot_0x24.z = 0;
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = scale;
        work->scale_a_0x44 = scale;
    }
}

/* ===================================================================================================
 * 0x8011280C  eft019_set_mazule(u8 id, nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot, _PLW* plw)
 * =================================================================================================== */

void eft019_set_mazule(u8 id, nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot, _PLW* plw)
{
    _EFT* effect;
    _EFT019_WORK* work;

    switch (id) {
    case 0:
        if (((_EFT019_ACTOR*)plw)->kind_0x09 != 3) {
            effect = fn_80114A1C(pos, area, 9);
        } else {
            effect = fn_80114A1C(pos, area, 79);
        }
        break;
    case 17:
        fn_80306D6C(plw, 11, pos, rot, plw->area_0x16, lbl_807969FC);
        effect = fn_80114A1C(pos, area, 19);
        break;
    case 18:
        fn_80306D6C(plw, 12, pos, rot, plw->area_0x16, lbl_80796A00);
        effect = fn_80114A1C(pos, area, 19);
        break;
    case 19:
        effect = fn_80114A1C(pos, area, 109);
        id = 0;
        break;
    case 20:
        effect = fn_80114A1C(pos, area, 110);
        id = 0;
        break;
    default:
        effect = fn_80114A1C(pos, area, 19);
        break;
    }
    if (effect != 0) {
        if (((_EFT019_ACTOR*)plw)->kind_0x09 != 3) {
            effect->demo_flag_0x08 = 0;
        } else {
            effect->demo_flag_0x08 = 1;
        }
        effect->source_0x30 = plw;
        effect->rot_0x24 = *rot;
        effect->rot_0x24.z = 0;
        work = (_EFT019_WORK*)effect->work_0x38;
        work->subtype_0x40 = id;
    }
}

/* ===================================================================================================
 * 0x80112994  eft019_set_subtype(u8 type, u8 subtype, nw4r::math::VEC3* pos, u8 area,
 *                                _CP_VECTOR* rot, s32 timer, f32 scale)
 * =================================================================================================== */

void eft019_set_subtype(u8 type, u8 subtype, nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot,
                        s32 timer, f32 scale)
{
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 v;
    _EFT* effect;
    _EFT019_WORK* work;

    VEC3_ctor(&v);
    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect != 0) {
        work = (_EFT019_WORK*)effect->work_0x38;
        switch (type) {
        case 0: {
            dir = get_camera_direction();
            copyVec3(&v, &dir);
            fn_80050850(&v, &v);
            fn_800513F0(&v, lbl_807969D8);
            addVec3To(pos, &v);
            eft019_set_core(pos, area, 12, lbl_807969DC, lbl_807969DC);
            break;
        }
        case 1: {
            dir = get_camera_direction();
            copyVec3(&v, &dir);
            fn_80050850(&v, &v);
            fn_800513F0(&v, lbl_807969D8);
            addVec3To(pos, &v);
            eft019_set_core(pos, area, 12, lbl_807969E0, lbl_807969E0);
            work->scale_b_0x48 = lbl_807969DC;
            work->scale_a_0x44 = lbl_807969DC;
            break;
        }
        case 2: {
            dir = get_camera_direction();
            copyVec3(&v, &dir);
            fn_80050850(&v, &v);
            fn_800513F0(&v, lbl_807969D8);
            addVec3To(pos, &v);
            eft019_set_core(pos, area, 12, lbl_807969E4, lbl_807969E4);
            work->scale_b_0x48 = lbl_807969E8;
            work->scale_a_0x44 = lbl_807969E8;
            break;
        }
        default:
            work->scale_b_0x48 = scale;
            work->scale_a_0x44 = scale;
            break;
        }
        effect->source_0x30 = 0;
        effect->rot_0x24.x = rot->x;
        effect->rot_0x24.y = rot->y;
        effect->rot_0x24.z = rot->z;
        effect->timer_0x0C = timer;
        work->subtype_0x40 = subtype;
    }
}

/* ===================================================================================================
 * 0x80112B7C  fn_80112B7C(void* source, u8 key, u8 subtype, VEC3* pos, VEC3* rot, f32 scale)
 * =================================================================================================== */

void fn_80112B7C(void* source, u8 key, u8 subtype, nw4r::math::VEC3* pos, _CP_VECTOR* rot,
                 f32 scale)
{
    _EFT* effect;
    _EFT019_WORK* work;
    u8 variant;

    variant = subtype;
    effect = fn_80114A1C(pos, ((_EFT019_ACTOR*)source)->area_0x16, key);
    if (effect != 0) {
        effect->rot_0x24 = *rot;
        effect->source_0x30 = source;
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = scale;
        work->scale_a_0x44 = scale;
        if ((s32)key == 102) {
            u8 t = fn_80331210(source);
            if ((u32)t > 2) {
                if ((s32)t != 3) {
                    /* 4 and up keep the record's own id */
                } else {
                    effect->type_0x02 = 103;
                }
            } else {
                variant = t;
            }
        }
        work->subtype_0x40 = variant;
    }
}

/* ===================================================================================================
 * 0x80112C48  eft019_set_ring(nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot)
 * =================================================================================================== */

void eft019_set_ring(nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot)
{
    _EFT* effect;

    effect = fn_80114A1C(pos, area, 10);
    if (effect != 0) {
        effect->rot_0x24.x = rot->x;
        effect->rot_0x24.y = rot->y;
        effect->rot_0x24.z = 0;
    }
}

/* ===================================================================================================
 * 0x80112C9C  eft019_set_smoke(nw4r::math::VEC3* pos, u8 area, u8 type, s32 timer)
 * =================================================================================================== */

void eft019_set_smoke(nw4r::math::VEC3* pos, u8 area, u8 type, s32 timer)
{
    _EFT* effect;

    effect = fn_80114A1C(pos, (u8)type, (u8)area);
    if (effect != 0) {
        effect->timer_0x0C = timer;
    }
}

/* ===================================================================================================
 * 0x80112CE0  fn_80112CE0(_EFT* self) - the pool release hook (`release_0x40`)
 * =================================================================================================== */

void fn_80112CE0(_EFT* self)
{
    _EFT019_WORK* work;

    work = (_EFT019_WORK*)self->work_0x38;
    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* ===================================================================================================
 * 0x80112D1C  fn_80112D1C(_EFT* self) - the per-frame dispatcher (`dispatch_0x34`)
 * =================================================================================================== */

void fn_80112D1C(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80112D58(self);
        break;
    case 1:
        fn_8011392C(self);
        break;
    case 2:
        fn_8011484C(self);
        break;
    case 3:
        fn_8011485C(self);
        break;
    }
}

/* ===================================================================================================
 * 0x80112D58  fn_80112D58(_EFT* self) - state 0: create the pool and place it
 *
 * `res_eft_create(lbl_8059F6FC[type], lbl_8059F810[type], 0)` makes slot 0 for every id; the switch on
 * `type_0x02` then adds the per-id extra objects (all of them checked, each failure destroying the
 * record through `fn_8011485C`) and, for some ids, the tint `color_0x4C`.  Every arm converges on the
 * shared tail, which places the pool on the record's position, runs the `type_0x02 - 0x66` camera
 * re-aim, flags the record live, dispatches the per-material impact sound and immediately advances the
 * state machine into `fn_8011392C`.
 * =================================================================================================== */

void fn_80112D58(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 cam;
    _EFT019_WORK* work;
    _EFT019_SND_OBJ* obj;
    s32 snd;
    s32 i;

    if (self->type_0x02 == 6) {
        self->state_0x05++;
        self->timer_0x0C = 2;
        return;
    }
    MTX34_ctor(&mtx);
    VEC3_ctor(&dir);
    work = (_EFT019_WORK*)self->work_0x38;
    snd = eft_control.field_0x04;
    self->state_0x05++;
    work->effects[0] = res_eft_create(lbl_8059F6FC[self->type_0x02], lbl_8059F810[self->type_0x02], 0);
    if (work->effects[0] == 0) {
        fn_8011485C(self);
        return;
    }

    switch (self->type_0x02) {
    case 7:
        self->timer_0x0C = 4;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        break;
    case 8:
        self->field_0x10 = 8;
        work->live_0x3C = 1;
        break;
    case 9:
    case 10:
    case 0x12:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x45:
    case 0x46:
    case 0x6C:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        break;
    case 0xE:
    case 0xF:
    case 0x10:
        work->effects[1] =
            res_eft_create((u16)(self->type_0x02 + 0x3FF), lbl_8059F810[self->type_0x02], 0);
        if (work->effects[1] == 0) {
            fn_8011485C(self);
            return;
        }
        break;
    case 0x11:
        eft019_set_tyakudan(&self->pos_0x18, &self->rot_0x24, self->area_0x44, work->subtype_0x40,
                            work->scale_a_0x44);
        break;
    case 0x13:
        switch (work->subtype_0x40) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 7:
        case 8:
            for (i = 1; i < work->count; i++) {
                work->effects[i] = res_eft_create(
                    (u16)(lbl_8059F6FC[self->type_0x02] + i), lbl_8059F810[self->type_0x02], 0);
                if (work->effects[i] == 0) {
                    fn_8011485C(self);
                    return;
                }
            }
            break;
        case 6:
            work->effects[1] = res_eft_create(
                (u16)(lbl_8059F6FC[self->type_0x02] + 1), lbl_8059F810[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(
                (u16)(lbl_8059F6FC[self->type_0x02] + 3), lbl_8059F810[self->type_0x02], 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            {
                u8* tint = ((u8**)lbl_8059FB40[0])[work->subtype_0x40];
                work->color_0x4C[0].r = tint[0];
                work->color_0x4C[0].g = tint[1];
                work->color_0x4C[0].b = tint[2];
                work->color_0x4C[0].a = 0xFF;
            }
            break;
        case 9:
        case 10:
        case 11:
            work->count = 2;
            work->effects[1] = res_eft_create(lbl_8059F924[work->subtype_0x40 - 9], 0x15, 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 12:
        case 13:
        case 14:
            work->count = 3;
            work->effects[1] = res_eft_create(lbl_8059F924[work->subtype_0x40 - 9], 0x15, 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(0x181, 0x15, 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 15:
            work->effects[1] = res_eft_create(
                (u16)(lbl_8059F6FC[self->type_0x02] + 1), lbl_8059F810[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(0x626, lbl_8059F810[self->type_0x02], 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 16:
            work->effects[1] = res_eft_create(
                (u16)(lbl_8059F6FC[self->type_0x02] + 1), lbl_8059F810[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(0x627, lbl_8059F810[self->type_0x02], 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 17:
        case 18:
            work->effects[1] = res_eft_create(
                (u16)(lbl_8059F6FC[self->type_0x02] + 1), lbl_8059F810[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->count--;
            break;
        default:
            break;
        }
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
            {
                u8* tint = ((u8**)lbl_8059FB40[i])[work->subtype_0x40];
                work->color_0x4C[i].r = tint[0];
                work->color_0x4C[i].g = tint[1];
                work->color_0x4C[i].b = tint[2];
                work->color_0x4C[i].a = 0xFF;
            }
        }
        break;

    case 0x14:
        work->color_0x4C[0].r = 0xC8;
        work->color_0x4C[0].g = 0xC8;
        work->color_0x4C[0].b = 0xA0;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x1B:
        work->color_0x4C[0].r = 0xFF;
        work->color_0x4C[0].g = 0xFF;
        work->color_0x4C[0].b = 0xFF;
        work->color_0x4C[0].a = 0x90;
        break;
    case 0x1D:
        work->effects[1] = res_eft_create(0x40D, lbl_8059F810[self->type_0x02], 0);
        if (work->effects[1] == 0) {
            fn_8011485C(self);
            return;
        }
        break;
    case 0x20: {
        u8* tint = lbl_8059FB4C + work->subtype_0x40 * 0xC;
        work->color_0x4C[0].r = tint[0];
        work->color_0x4C[0].g = tint[1];
        work->color_0x4C[0].b = tint[2];
        work->color_0x4C[0].a = 0xFF;
        break;
    }
    case 0x21:
        fn_800DA8AC(&self->pos_0x18);
        break;
    case 0x28:
    case 0x2A:
    case 0x2B:
    case 0x2C:
    case 0x30:
    case 0x5A:
    case 0x5B:
    case 0x5C:
    case 0x5D:
    case 0x65:
    case 0x68:
    case 0x69:
    case 0x6D:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x76:
    case 0x77:
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x81:
    case 0x85:
    case 0x86:
    case 0x87:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
        break;
    case 0x38:
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x3A:
        work->color_0x4C[0].r = 0xFF;
        work->color_0x4C[0].g = 0x80;
        work->color_0x4C[0].b = 0xFF;
        work->color_0x4C[0].a = 0xBF;
        break;
    case 0x3B:
        self->timer_0x0C = 4;
        work->color_0x4C[0].r = 0xDF;
        work->color_0x4C[0].g = 0x79;
        work->color_0x4C[0].b = 0xFF;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x40:
        work->color_0x4C[0].r = 0xA2;
        work->color_0x4C[0].g = 0xBF;
        work->color_0x4C[0].b = 0xFF;
        work->color_0x4C[0].a = 0xC8;
        break;
    case 0x41:
        work->color_0x4C[0].r = 0xA8;
        work->color_0x4C[0].g = 0x73;
        work->color_0x4C[0].b = 0xFC;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x42:
        work->color_0x4C[0].r = 0xE8;
        work->color_0x4C[0].g = 0xD7;
        work->color_0x4C[0].b = 0xBB;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x48:
    case 0x49:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x57:
    case 0x70:
    case 0x7F: {
        u32 col = get_stg_eft_col(self->area_0x44, 0);
        work->color_0x4C[0].r = (u8)(col >> 24);
        work->color_0x4C[0].g = (u8)(col >> 16);
        work->color_0x4C[0].b = (u8)(col >> 8);
        work->color_0x4C[0].a = (u8)col;
        break;
    }
    case 0x4A:
    case 0x52:
    case 0x53: {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        work->color_0x4C[0].r = (u8)(col >> 24);
        work->color_0x4C[0].g = (u8)(col >> 16);
        work->color_0x4C[0].b = (u8)(col >> 8);
        work->color_0x4C[0].a = (u8)col;
        break;
    }
    case 0x4E:
        work->color_0x4C[0].r = 0xA0;
        work->color_0x4C[0].g = 0xA3;
        work->color_0x4C[0].b = 0x4F;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x4F:
    case 0x50:
    case 0x6E:
        work->color_0x4C[0].r = 0x64;
        work->color_0x4C[0].g = 0xBF;
        work->color_0x4C[0].b = 0xFF;
        work->color_0x4C[0].a = 0xFF;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        work->effects[0]->SetRootMtx(mtx);
        break;
    case 0x55:
    case 0x56:
    case 0x5E:
    case 0x5F:
    case 0x60:
    case 0x62:
    case 0x6B:
        work->color_0x4C[0].r = 0x7F;
        work->color_0x4C[0].g = 0x7F;
        work->color_0x4C[0].b = 0x7F;
        work->color_0x4C[0].a = 0x7F;
        break;
    case 0x63:
        work->color_0x4C[0].r = 0xFF;
        work->color_0x4C[0].g = 0x80;
        work->color_0x4C[0].b = 0x40;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x64:
        work->color_0x4C[0].r = 0xD7;
        work->color_0x4C[0].g = 0xFF;
        work->color_0x4C[0].b = 0xFF;
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x66:
        work->effects[1] = res_eft_create(0x637, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_8011485C(self);
            return;
        }
        work->effects[2] = res_eft_create(0x635, 0x15, 0);
        if (work->effects[2] == 0) {
            fn_8011485C(self);
            return;
        }
        break;
    case 0x67:
        work->effects[1] = res_eft_create(0x638, 0x15, 0);
        if (work->effects[1] == 0) {
            fn_8011485C(self);
            return;
        }
        work->effects[2] = res_eft_create(0x639, 0x15, 0);
        if (work->effects[2] == 0) {
            fn_8011485C(self);
            return;
        }
        break;
    case 0x82:
    case 0x83:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
        {
            u32 col = get_stg_eft_col(self->area_0x44, 1);
            work->color_0x4C[0].r = (u8)(col >> 24);
            work->color_0x4C[0].g = (u8)(col >> 16);
            work->color_0x4C[0].b = (u8)(col >> 8);
            work->color_0x4C[0].a = (u8)col;
        }
        break;
    case 0x84: {
        u8* tint = lbl_8059FBDC + work->subtype_0x40 * 0xC;
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
        work->color_0x4C[0].r = tint[0];
        work->color_0x4C[0].g = tint[1];
        work->color_0x4C[0].b = tint[2];
        work->color_0x4C[0].a = 0xFF;
        break;
    }
    case 0x88:
        SetRootMtxTrans(work->effects[0], &self->pos_0x18);
        fn_800F996C(work->effects[0], 0);
        obj = (_EFT019_SND_OBJ*)fn_800A51D8(work->effects[0], 0);
        i = (s32)fn_800A9714(obj, (u16)(fn_800A970C() - 1));
        fn_800AB9F4((struct EfPmManager*)i);
        fn_800A67E8(obj, (void*)i);
        obj = (_EFT019_SND_OBJ*)fn_800A4420(snd);
        ((_EFT019_SND_VTABLE*)obj->vtable)->slot_0x0C(obj);
        break;
    }

    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
    }
    if ((u32)(self->type_0x02 - 0x66) <= 1) {
        cam = get_camera_direction();
        copyVec3(&dir, &cam);
        fn_80050850(&dir, &dir);
        fn_800513F0(&dir, lbl_807969D8);
        for (i = 1; i < work->count; i++) {
            fn_800F975C(work->effects[i], &dir);
        }
    }
    self->flag_0x01 = 1;
    switch (self->type_0x02) {
    case 0:
    case 0x88:
        if (work->subtype_0x40 == 0) {
            fn_800DA9B4(&self->pos_0x18);
        } else {
            fn_800DA9A4(&self->pos_0x18);
        }
        break;
    case 1:
    case 2:
        if (work->subtype_0x40 == 0) {
            fn_800DA9C4(&self->pos_0x18);
        }
        break;
    case 0xD:
        fn_800DA9E4(&self->pos_0x18);
        break;
    case 0xE:
        if (work->subtype_0x40 == 0) {
            fn_800DA9E4(&self->pos_0x18);
        } else {
            fn_800DA9D4(&self->pos_0x18);
        }
        break;
    case 0xF:
        fn_800DA9D4(&self->pos_0x18);
        break;
    case 0x10:
        fn_800DA9F4(&self->pos_0x18);
        break;
    case 0x2E:
        fn_800DC4B4(&self->pos_0x18);
        break;
    case 0x33:
    case 0x36:
    case 0x46:
        fn_800DC46C(&self->pos_0x18);
        break;
    case 0x39:
        fn_800DA9B4(&self->pos_0x18);
        break;
    case 0x83:
        fn_800DA72C(0, 0x14, &self->pos_0x18);
        break;
    }
    fn_8011392C(self);
}

/* ===================================================================================================
 * 0x8011484C  fn_8011484C(_EFT* self) - state 2
 * =================================================================================================== */

void fn_8011484C(_EFT* self)
{
    self->state_0x05++;
}

/* ===================================================================================================
 * 0x8011485C  fn_8011485C(_EFT* self) - state 3
 * =================================================================================================== */

void fn_8011485C(_EFT* self)
{
    fn_800F886C(self);
}

/* ===================================================================================================
 * 0x80114860  fn_80114860(nw4r::math::VEC3* pos, u8 area) - the weapon-side `tyakudan` record
 * =================================================================================================== */

void fn_80114860(nw4r::math::VEC3* pos, u8 area)
{
    _EFT* effect;

    effect = fn_800F8788(0);
    if (effect != 0) {
        effect->field_0x03 = 19;
        effect->type_0x02 = 6;
        copyVec3(&effect->pos_0x18, pos);
        effect->area_0x44 = area;
        effect->source_0x30 = 0;
        effect->dispatch_0x34 = fn_80112D1C;
    }
}

/* ===================================================================================================
 * 0x801148E4  fn_801148E4(_EFT* self) - the on-screen impact sound and its normalisation
 * =================================================================================================== */

void fn_801148E4(_EFT* self)
{
    f32 v[3];
    f32 max[2];
    s32 color;
    s32 handle;
    f32 rx;
    f32 ry;

    VEC3_ctor((nw4r::math::VEC3*)v);
    if (self->area_0x44 == get_now_areano() && self->timer_0x0C > 0) {
        fn_8004030C((struct _MH_VEC2*)max);
        handle = fn_80082BCC(pRoot);
        fn_80075258((nw4r::g3d::Camera*)&handle, (u8*)v, &self->pos_0x18);
        if (v[2] <= lbl_80796A08 || v[2] >= lbl_807969DC || v[0] < lbl_80796A08 ||
            v[0] > max[0] || v[1] < lbl_80796A08 || v[1] > max[1]) {
            /* the projected point is off-screen: the impact makes no sound */
        } else {
            rx = v[0] - max[0] * lbl_80796A10;
            ry = v[1] - max[1] * lbl_80796A10;
            v[0] = rx / max[0];
            v[1] = ry / max[1];
            color = 0xFF;
            fn_80056A20(ry, rx);
            fn_8005696C(128, 3, 32, &color, self->timer_0x0C, v[0], v[1]);
        }
    }
    self->state_0x05++;
}

/* ===================================================================================================
 * 0x80114A1C  fn_80114A1C(nw4r::math::VEC3* pos, u8 area, u8 type) - the family allocator
 * =================================================================================================== */

_EFT* fn_80114A1C(nw4r::math::VEC3* pos, u8 area, u8 type)
{
    _EFT* effect;
    _EFT019_WORK* work;

    if (area != get_now_areano()) {
        return 0;
    }
    effect = fn_800F8788(0x58);
    if (effect == 0) {
        return 0;
    }
    work = (_EFT019_WORK*)effect->work_0x38;
    work->count = lbl_8059F670[type];
    work->unused_0x38 = 0;
    work->scale_a_0x44 = work->scale_b_0x48 = lbl_807969DC;
    effect->field_0x03 = 19;
    effect->timer_0x0C = 0;
    effect->field_0x10 = 0;
    effect->type_0x02 = type;
    copyVec3(&effect->pos_0x18, pos);
    effect->area_0x44 = area;
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_80112CE0;
    effect->dispatch_0x34 = fn_80112D1C;
    effect->source_0x30 = 0;
    if (event_demo_ck() == 1U) {
        effect->field_0x07 = 1;
    }
    return effect;
}

/* ===================================================================================================
 * 0x80114B20  fn_80114B20(_EFT* self, nw4r::math::MTX34* mtx) - place the record on its actor
 * =================================================================================================== */

void fn_80114B20(_EFT* self, nw4r::math::MTX34* mtx)
{
    nw4r::math::VEC3 axis;
    nw4r::math::VEC3 spin;
    nw4r::math::VEC3 pos;
    _CP_VECTOR rot;

    VEC3_ctor(&axis);
    VEC3_ctor(&spin);
    VEC3_ctor(&pos);
    fn_800E0A14(&((_EFT019_PHYSICS*)((_PLW*)self->source_0x30)->physics_0x13C)->chr_0x04, 7, mtx);
    mtx34_trans_get(mtx, &pos);
    axis.x = -mtx->m[0][2];
    axis.y = -mtx->m[1][2];
    axis.z = -mtx->m[2][2];
    calcVecAngXY(&axis, &rot.x, &rot.y);
    rot.z = 0;
    self->rot_0x24.z = 0;
    setVector3(&spin, lbl_80796A14, lbl_80796A14, lbl_80796A18);
    rotVecX(&spin, rot.x);
    rotVecY(&spin, rot.y);
    addVec3To(&pos, &spin);
    cpSetRotMatrix(&rot, mtx);
    fn_800FBB90(mtx, &pos);
}

/* ===================================================================================================
 * 0x80114C20  ef_inst_spawn(void* actor, u8 key) - spawn an eft020 record for a per-key effect
 * =================================================================================================== */

void ef_inst_spawn(void* actor, u8 key)
{
    _EFT* effect;

    effect = fn_80114E34(key, ((_EFT019_ACTOR*)actor)->area_0x16, lbl_80796A20);
    if (effect != 0) {
        effect->source_0x30 = actor;
        effect->demo_flag_0x08 = 0;
    }
}

/* ===================================================================================================
 * 0x80114C6C  fn_80114C6C(void* actor, u8 key, s32 joint)
 * =================================================================================================== */

void fn_80114C6C(void* actor, u8 key, s32 joint)
{
    _EFT* effect;
    /* retail passes the third argument as whatever the caller left in `f1` (the original translation
     * unit reached this callee through a declaration with no prototype); an uninitialised local is
     * what reproduces it, and the callee only reads it when it has to place a scaled model. */
    f32 scale;

    effect = fn_80114E34(key, ((_EFT019_ACTOR*)actor)->area_no_0x1E1, scale);
    if (effect != 0) {
        effect->source_0x30 = actor;
        effect->demo_flag_0x08 = 1;
        ((_EFT020_WORK_A_JOINT*)effect->work_0x38)->joint_0x18 = joint;
    }
}

/* ===================================================================================================
 * 0x80114CC8  fn_80114CC8(void* actor, u8 key)
 * =================================================================================================== */

void fn_80114CC8(void* actor, u8 key)
{
    _EFT* effect;

    effect = fn_80114E34(key, ((_EFT019_ACTOR*)actor)->effect_key_0x1A4, lbl_80796A24);
    if (effect != 0) {
        effect->source_0x30 = actor;
        effect->demo_flag_0x08 = 2;
        effect->field_0x07 = 1;
        ((_EFT020_WORK_A_JOINT*)effect->work_0x38)->joint_0x18 = 3;
    }
}

/* ===================================================================================================
 * 0x80114D28  fn_80114D28(void* actor, u8 kind, u8 type, f32 scale)
 * =================================================================================================== */

void fn_80114D28(void* actor, u8 kind, u8 type, f32 scale)
{
    _EFT019_ACTOR* a;
    _EFT* effect;
    _EFT020_WORK_A_JOINT* work;
    u8 area;
    f32 scaleIn;

    a = (_EFT019_ACTOR*)actor;
    scaleIn = scale;
    switch (kind) {
    case 0:
        area = a->area_0x16;
        break;
    case 1:
        area = a->area_no_0x1E1;
        scaleIn = ((_EFT019_EMY_DATA*)get_enemy_data((_ENEMY_WORK*)actor))->field_0x9C->scale_0x1C;
        break;
    case 3:
        area = a->effect_key_0x1A4;
        break;
    }
    effect = fn_80114E34(type, area, scaleIn);
    if (effect != 0) {
        effect->source_0x30 = actor;
        work = (_EFT020_WORK_A_JOINT*)effect->work_0x38;
        switch (kind) {
        case 0:
            effect->demo_flag_0x08 = 0;
            break;
        case 1:
            effect->demo_flag_0x08 = 1;
            if (a->joint_0x204 != 0xFFFFFFFF) {
                work->joint_0x18 = a->joint_0x204;
            } else {
                work->joint_0x18 = a->field_0x110;
            }
            break;
        case 3:
            effect->demo_flag_0x08 = 2;
            work->joint_0x18 = 3;
            break;
        }
    }
}

#pragma peephole reset
