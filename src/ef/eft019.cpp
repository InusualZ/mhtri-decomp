/* ef/eft019.cpp - the eft019 effect family: its spawn entry points, the family allocator and the record's state
 *   machine.
 * RANGE. .text 0x801121DC-0x80114E34 (24 functions); extab 0x8000C32C-0x8000C3D4, extabindex 0x800263AC-0x800264A8,
 *   .data 0x8059F670-0x805A00B8, .sdata 0x80791840-0x80791930, .sdata2 0x807969D8-0x80796A28.  `fn_80114A1C` is the
 *   allocator (`eft_res_slot_get(0x58)`, family tag `field_0x03 = 19`, capacity from `lbl_8059F670`, hooks
 *   `fn_80112D1C` as `dispatch_0x34` and `fn_80112CE0` as `release_0x40`); `fn_80112D1C` dispatches `state_0x05` to
 *   `fn_80112D58` (create), `fn_8011392C` (per-frame update), `fn_8011484C` (advance) and `fn_8011485C` (release).
 * FLAGS. `cflags_main`; `#pragma peephole off` from `eft019_set` to the end of the file (playbook 39: with the pass on,
 *   the `u8` argument `clrlwi`s fold away).
 * NAMES. The nine `eft019_set*` entries are the runtime dump's own names; the map has only `fn_` stems for the rest,
 *   so plain definitions are `extern "C"`.  `_EFT019_ACTOR` reads the actor's +0x1E1 byte as the area number (the
 *   canonical `enemy.h` calls it `act_id`; every ef consumer compares it as an area).
 * RESIDUALS. 1 row unwritten: 0x8011392C-0x8011484C (`fn_8011392C`, the per-frame update).
 *  - `eft019_set` and `eft019_set_subtype` load the pooled constants `lbl_807969DC`, `lbl_807969E8` and
 *    `lbl_807969F4` by name where retail's loads carry no symbol (retail folds them into its own pool).
 *   7 partial rows:
 *  - `eft019_set`: frame 0x100 against retail's 0x90; ours copies each case's `VEC3` through a second stack slot;
 *  - `fn_80112D58`: frame 0x80 against 0x70, and ours places the `lbl_8059FB40` colour block inline;
 *  - `eft019_set_brethend`, `fn_80112B7C`: ours inverts the last test of the chain, so the two bodies sit in the other
 *    order;
 *  - `fn_80112B7C`: retail puts the `type_0x02 = 103` body out of line (the `else-if` and empty-`then` spellings
 *    do not reproduce it);
 *  - `eft019_set_mazule`, `eft019_set_subtype`, `fn_801148E4` have no recorded cause (`symdiff.py -u ef/eft019`).
 *   flipcheck: `.sdata`/`.sdata2` claimed, not emitted; `.text` (0x1E60 of 0x2C58), extab (0xA0 of 0xA8), extabindex
 *   (0xF0 of 0xFC) and `.data` (0x254 of 0xA48) short of the claim and differing.
 * SHAPES. The `_CP_VECTOR*` setters copy the rotation triple with word reads (`effect->rot_0x24 = *rot;`): through a
 *   `nw4r::math::VEC3*` it emits an `fctiwz` per component.
 *   `eft019_set` keeps one `VEC3` per camera case (nine).
 *   `fn_80114A1C` writes its two seeds with a chained assignment, so the constant is loaded once and 0x48 stored first.
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
#include "Pl/Pl_master_ck.h"
#include "unsplit/unknown.h"
#include "g3d/g3d_scnroot.h" /* fn_80082BCC (rule 2) */
#include "sound/fn_800D7F54.h"
#include "ef/ef_particlemanager.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/pRoot.h"

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

/* Callees other units own, declared locally (a rule-2 residual): `vec3_normalize_into`/`fn_800513F0` are
 * `fn_8004CAD8.cpp`'s, `fn_8005696C`/`fn_80056A20` `draw_shape_arm.cpp`'s, `eft_res_slot_get`/`eft_res_slot_release`
 * `ef/eft_res.cpp`'s, `fn_80306D6C` `ef/fn_8030681C.cpp`'s, `fn_80331210` `hud/pl_frame_sync.cpp`'s and
 * `fn_800E0A14` `sound/mhchar.cpp`'s. */
void vec3_normalize_into(nw4r::math::VEC3* v, const nw4r::math::VEC3* in);
void fn_800513F0(nw4r::math::VEC3* v, f32 angle);
void fn_8005696C(s32 id, s32 kind, s32 mode, s32* color, s32 timer, f32 x, f32 y);
void fn_80056A20(f32 a, f32 b);
_EFT* eft_res_slot_get(u32 pool);
void eft_res_slot_release(_EFT* self);
void fn_80306D6C(void* self, s32 a, void* b, void* c, u8 d, f32 e);
u8 fn_80331210(void* self);
void mhchar_joint_mtx_get(void* chr, u32 joint, nw4r::math::MTX34* out);

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

/* The effect-manager id table and the sound-object hooks `fn_80112D58` case 0x88 drives, declared locally with C
 * linkage (the target references their plain names): `ef_system_memory_manager`/`fn_800A51D8` are `ef/ef_effect.cpp`'s,
 * `fn_800A970C`/`fn_800A9714`/`fn_800A67E8` `ef/ef_emitter.cpp`'s (a rule-2 residual). */
extern "C" {
void** ef_system_memory_manager(s32 id);
void* fn_800A51D8(nw4r::ef::Effect* effect, u32 arg);
u16 fn_800A970C(void);
void* fn_800A9714(void* obj, u16 index);
void fn_800A67E8(void* obj, void* value);
}

/* The object `ef_system_memory_manager` returns: a vtable word at +0 and the 4th virtual slot at +0x0C, which
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

/* The effect manager's control block `eft_control` is the band's one record (`unsplit/ef_control.h`); this unit only hands its +0x04 word to `ef_system_memory_manager`. */
#include "unsplit/ef_control.h"

/* `calcVecAngXY(VEC3*, u32*, u32*)` and `rotVecY(VEC3*, u32)` are map-mangled free functions with no
 * registered owner; declaring them at global scope reproduces their map spellings exactly. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* The camera entries: the map spells both `__Fv`, but every call site passes an out pointer, so the declaration
 * carries it (the relocation name differs from the map's; playbook 23). */
nw4r::math::VEC3 get_camera_direction(void); /* target references get_camera_direction__Fv (relocaudit) */
void get_camera_pos(nw4r::math::VEC3* out);

/* `get_enemy_data`'s map spelling is `get_enemy_data__FP11_ENEMY_WORK`; the tag is forward-declared
 * here so the declaration emits that exact mangling without pulling in `enemy.h`'s record view. */
struct _ENEMY_WORK;
void* get_enemy_data(struct _ENEMY_WORK* enemy);

/* ---------------------------------------------------------------------------------------------------
 * engine records
 * ------------------------------------------------------------------------------------------------- */

/* The effect pool the eft019 record carries at `_EFT::work_0x38`.  The size is `eft_res_slot_get`'s
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

/* --- the unit's own pool, declared, never defined (playbook 29) ---------------------------------- */

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

/* The bodies keep retail's `u8` argument masks and unfused record-form compares (playbook 39). */
#pragma peephole off

/* 0x801121DC (0x434): Spawns an eft019 record and orients it per camera case. */

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
        vec3_normalize_into(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 12, lbl_807969DC, lbl_807969DC);
        return;
    }
    case 1: {
        _EFT019_WORK* work;
        d1 = get_camera_direction();
        copyVec3(&v, &d1);
        vec3_normalize_into(&v, &v);
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
        vec3_normalize_into(&v, &v);
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
        vec3_normalize_into(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 13, lbl_807969DC, lbl_807969DC);
        return;
    }
    case 15: {
        d4 = get_camera_direction();
        copyVec3(&v, &d4);
        vec3_normalize_into(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 13, lbl_807969E0, lbl_807969E0);
        return;
    }
    case 16: {
        d5 = get_camera_direction();
        copyVec3(&v, &d5);
        vec3_normalize_into(&v, &v);
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
        vec3_normalize_into(&v, &v);
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
        vec3_normalize_into(&v, &v);
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
        vec3_normalize_into(&v, &v);
        fn_800513F0(&v, lbl_807969D8);
        addVec3To(pos, &v);
        eft019_set_core(pos, area, 58, lbl_807969F8, lbl_807969F8);
        break;
    }
    }
}

/* 0x80112610 (0x54): Spawns an eft019 record with its two scale factors. */

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

/* 0x80112664 (0x60): Spawns an eft019 record with one scale and a copied rotation. */

void eft019_set_vec(nw4r::math::VEC3* pos, _CP_VECTOR* rot, u8 area, u8 type, f32 scale)
{
    _EFT* effect;
    _EFT019_WORK* work;

    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect != 0) {
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = scale;
        work->scale_b_0x48 = scale;
        eft_rot_vec_copy(&effect->rot_0x24, rot);
    }
}

/* 0x801126C4 (0xE0): Spawns the breath-end record: variant 11 for ids 3-5, variant 17 for ids 18, 21, 22 and 83. */

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

/* 0x801127A4 (0x68): Spawns a landing record that keeps only the rotation's yaw, with a scale. */

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

/* 0x8011280C (0x188): Spawns the muzzle record for a weapon id, the variant picked by the player's kind. */

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

/* 0x80112994 (0x1E8): Spawns an eft019 record of a subtype with its rotation, timer and scale. */

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
            vec3_normalize_into(&v, &v);
            fn_800513F0(&v, lbl_807969D8);
            addVec3To(pos, &v);
            eft019_set_core(pos, area, 12, lbl_807969DC, lbl_807969DC);
            break;
        }
        case 1: {
            dir = get_camera_direction();
            copyVec3(&v, &dir);
            vec3_normalize_into(&v, &v);
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
            vec3_normalize_into(&v, &v);
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

/* 0x80112B7C (0xCC): Spawns an eft019 record in a source actor's area with its rotation, source and variant. */

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

/* 0x80112C48 (0x54): Spawns the ring record (type 10) with the rotation's pitch and yaw. */

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

/* 0x80112C9C (0x44): Spawns a smoke record with a timer. */

void eft019_set_smoke(nw4r::math::VEC3* pos, u8 area, u8 type, s32 timer)
{
    _EFT* effect;

    effect = fn_80114A1C(pos, (u8)type, (u8)area);
    if (effect != 0) {
        effect->timer_0x0C = timer;
    }
}

/* 0x80112CE0 (0x3C): Hands the record's effect pool back to the heap (the `release_0x40` hook). */

void fn_80112CE0(_EFT* self)
{
    _EFT019_WORK* work;

    work = (_EFT019_WORK*)self->work_0x38;
    push_eft_effect_heap_num(work->effects, work->count);
    work->count = 0;
}

/* 0x80112D1C (0x3C): Dispatches `state_0x05` to the four state bodies (the `dispatch_0x34` hook). */

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

/* 0x80112D58 (0xBD4): Creates the record's effect pool for its type, places and re-aims it, plays the impact
 * sound and advances into `fn_8011392C`. */

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
        obj = (_EFT019_SND_OBJ*)ef_system_memory_manager(snd);
        ((_EFT019_SND_VTABLE*)obj->vtable)->slot_0x0C(obj);
        break;
    }

    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
    }
    if ((u32)(self->type_0x02 - 0x66) <= 1) {
        cam = get_camera_direction();
        copyVec3(&dir, &cam);
        vec3_normalize_into(&dir, &dir);
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

/* 0x8011484C (0x10): Advances the record's state. */

void fn_8011484C(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011485C (0x4): Releases the record to the slot pool. */

void fn_8011485C(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x80114860 (0x84): Spawns the weapon-side landing record (type 6) without a work block. */

void fn_80114860(nw4r::math::VEC3* pos, u8 area)
{
    _EFT* effect;

    effect = eft_res_slot_get(0);
    if (effect != 0) {
        effect->field_0x03 = 19;
        effect->type_0x02 = 6;
        copyVec3(&effect->pos_0x18, pos);
        effect->area_0x44 = area;
        effect->source_0x30 = 0;
        effect->dispatch_0x34 = fn_80112D1C;
    }
}

/* 0x801148E4 (0x138): Plays the on-screen impact sound and normalises its screen position. */

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

/* 0x80114A1C (0x104): Allocates an eft019 record in the current area, tags it 19 and installs its hooks. */

_EFT* fn_80114A1C(nw4r::math::VEC3* pos, u8 area, u8 type)
{
    _EFT* effect;
    _EFT019_WORK* work;

    if (area != get_now_areano()) {
        return 0;
    }
    effect = eft_res_slot_get(0x58);
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
    eft_state_flags_set(effect, 0, 0);
    effect->release_0x40 = fn_80112CE0;
    effect->dispatch_0x34 = fn_80112D1C;
    effect->source_0x30 = 0;
    if (event_demo_ck() == 1U) {
        effect->field_0x07 = 1;
    }
    return effect;
}

/* 0x80114B20 (0x100): Places the record on its actor's joint 7 and spins it. */

void fn_80114B20(_EFT* self, nw4r::math::MTX34* mtx)
{
    nw4r::math::VEC3 axis;
    nw4r::math::VEC3 spin;
    nw4r::math::VEC3 pos;
    _CP_VECTOR rot;

    VEC3_ctor(&axis);
    VEC3_ctor(&spin);
    VEC3_ctor(&pos);
    mhchar_joint_mtx_get(&((_EFT019_PHYSICS*)((_PLW*)self->source_0x30)->physics_0x13C)->chr_0x04, 7, mtx);
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

/* 0x80114C20 (0x4C): Spawns an eft020 record for a per-key effect on an actor. */

void ef_inst_spawn(void* actor, u8 key)
{
    _EFT* effect;

    effect = fn_80114E34(key, ((_EFT019_ACTOR*)actor)->area_0x16, lbl_80796A20);
    if (effect != 0) {
        effect->source_0x30 = actor;
        effect->demo_flag_0x08 = 0;
    }
}

/* 0x80114C6C (0x5C): Spawns an eft020 record on an actor's joint in the actor's area. */

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

/* 0x80114CC8 (0x60): Spawns an eft020 record on an actor's joint 3 under its effect key. */

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

/* 0x80114D28 (0x10C): Spawns an eft020 record on an actor, its area, scale and joint picked by the actor kind. */

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
