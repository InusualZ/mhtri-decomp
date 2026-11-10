/* ef/eft019.cpp - the eft019 effect family: its spawn entry points, the family allocator and the record's state
 *   machine.
 * RANGE. .text 0x801121DC-0x80114C20 (20 functions); extab 0x8000C32C-0x8000C3B4, extabindex 0x800263AC-0x80026478,
 *   .data 0x8059F670-0x805A00B8, .sdata 0x80791840-0x80791930, .sdata2 0x807969D8-0x80796A20 (17 words, the
 *   pad word 0x80796A1C is the link's 8-byte fill before `ef/fn_80114E34.cpp`'s pool).  `fn_80114A1C` is the
 *   allocator (`eft_res_slot_get(0x58)`, family tag `field_0x03 = 19`, capacity from `eft019_effect_counts`, hooks
 *   `fn_80112D1C` as `dispatch_0x34` and `fn_80112CE0` as `release_0x40`); `fn_80112D1C` dispatches `state_0x05` to
 *   `fn_80112D58` (create), `fn_8011392C` (per-frame update), `fn_8011484C` (advance) and `fn_8011485C` (release).
 * FLAGS. `cflags_main`; `#pragma peephole off` from `eft019_set` to the end of the file (playbook 39: with the pass on,
 *   the `u8` argument `clrlwi`s fold away); `#pragma pool_data off` over the tables (playbook 194: with data pooling
 *   on, `fn_80112D58`/`fn_8011392C` reach the tables through one `.data` anchor where retail names each table).
 * NAMES. The nine `eft019_set*` entries are the runtime dump's own names; the map has only `fn_` stems for the rest,
 *   so plain definitions are `extern "C"`.
 *   GUESS (the data, from the readers): `eft019_effect_counts`/`_groups`/`_ids` (the per-type `res_eft_create`
 *   arguments), `eft019_pair_effect_ids` (subtypes 9..11's second effect), `eft019_rgb_keys_19_{a,b,c}N` and
 *   `eft019_rgb_key_tables_19_{a,b,c}`/`eft019_rgb_key_table_set_19` (the per-slot colour strips type 0x13 picks by
 *   subtype), `eft019_tint_rgb_32`/`_132` (the RGB triples of types 0x20/0x84), `eft019_rgb_keys_56`,
 *   `eft019_rgb_keys_0_28`/`eft019_alpha_keys_0_28`, `eft019_rgb_keys_57`, `eft019_rgb_keys_102_*` and
 *   `eft019_rgb_key_tables_102_{a,b,c}` (the strips of the state those types run).
 * RESIDUALS. none.
 * SHAPES. The `_CP_VECTOR*` setters copy the rotation triple with word reads (`effect->rot_0x24 = *rot;`): through a
 *   `nw4r::math::VEC3*` it emits an `fctiwz` per component.
 *   `eft019_set` keeps one `VEC3` per camera case (nine).
 *   `fn_80114A1C` writes its two seeds with a chained assignment, so the constant is loaded once and 0x48 stored first.
 *   `fn_801148E4` halves the screen size with `max[0] / 2.0f`: the strength-reduced multiply keeps the operand order
 *   `fmuls f0, f5, f1` that a `* 0.5f` literal (canonicalised constant-first) does not.
 *   The data is defined in one interleaved order so both `.sdata` (8-byte strips) and `.data` come out in retail's
 *   order; `eft019_tint_rgb_32` is 15 bytes and `eft019_rgb_keys_57` 12, because MWCC 8-aligns a 16-byte array and
 *   retail's next objects are 4-aligned.
 */
#include "types.h"
#include "nw4r/math.h"
#include "pl.h"
#include "ef/eft001.h"
#include "ef/eft004.h"
#include "camera/camera.h" /* get_camera_pos (rule 2) */
#include "ef/effect.h"
#include "ef/fn_80114E34.h" /* the eft020 creator the shot setter spawns through (rule 2) */
#include "g3d/g3d_calcworld.h"
#include "g3d/g3d_camera.h"
#include "main.h"
#include "Pl/Pl_master_ck.h"
#include "unsplit/unknown.h"
#include "g3d/g3d_scnroot.h" /* fn_80082BCC (rule 2) */
#include "sound/fn_800D7F54.h"
#include "ef/ef_particlemanager.h"
#include "mh3_pad.h" /* VEC3::VEC3() / VEC3::operator= / VEC3(x, y, z) (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/pRoot.h"
#include "ef/eft002.h" /* eft_effect_foreach_pm_scale (rule 2) */
#include "ef/eft_stage_spawn_flag.h" /* eft_stage_spawn_flag (rule 2) */
#include "rotLocalMatX.h" /* rotLocalMatY (rule 2) */
#include "ef/eft_res_models_spawn.h" /* eft_res_models_spawn (rule 2) */
#include "fn_8004CAD8.h" /* mtx34_identity (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the unit's own prototypes (plain C linkage: the map's stems must stay unmangled)
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void eft019_shot_set(void* source, u8 key, u8 subtype, nw4r::math::VEC3* pos, _CP_VECTOR* rot,
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

/* Callees other units own, declared locally (a rule-2 residual): `vec3_normalize_into`/`vec3_scale_in_place` are
 * `fn_8004CAD8.cpp`'s, `fn_8005696C`/`fn_80056A20` `draw_shape_arm.cpp`'s, `eft_res_slot_get`/`eft_res_slot_release`
 * `ef/eft_res.cpp`'s, `fn_80306D6C` `ef/fn_8030681C.cpp`'s, `pl_frame_eft_color_get` `hud/pl_frame_sync.cpp`'s and
 * `fn_800E0A14` `sound/mhchar.cpp`'s. */
void draw_sprite_set(s32 id, s32 kind, s32 mode, f32 x, f32 y, _GXColor color, s32 timer);
void draw_sprite_start(f32 a, f32 b);
_EFT* eft_res_slot_get(u32 pool);
void eft_res_slot_release(_EFT* self);
void eft042_set_placed(void* self, s32 a, void* b, void* c, u8 d, f32 e);
u8 pl_frame_eft_color_get(void* self);
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
 * linkage (the target references their plain names): `ef_system_memory_manager`/`ef_effect_emitter_get` are `ef/ef_effect.cpp`'s,
 * `ef_emitter_pm_count`/`ef_emitter_pm_get`/`fn_800A67E8` `ef/ef_emitter.cpp`'s (a rule-2 residual). */
extern "C" {
void** ef_system_memory_manager(s32 id);
void* ef_effect_emitter_get(nw4r::ef::Effect* effect, u32 arg);
u16 ef_emitter_pm_count(void);
void* ef_emitter_pm_get(void* obj, u16 index);
void fn_800A67E8(void* obj, void* value);
}

/* The memory manager `ef_system_memory_manager` returns, reached through its second virtual slot (+0x0C); declared
 * only, so this unit emits no table of its own.  size: 0x04 - lower bound, an approximation (the vtable word) */
class EftMemoryManager {
public:
    virtual void virtual_0x08();
    virtual void virtual_0x0C();
};

/* The effect manager's control block `eft_control` is the band's one record (`unsplit/ef_control.h`); this unit only hands its +0x04 word to `ef_system_memory_manager`. */
#include "unsplit/ef_control.h"

/* `calcVecAngXY(VEC3*, u32*, u32*)` and `rotVecY(VEC3*, u32)` are map-mangled free functions with no
 * registered owner; declaring them at global scope reproduces their map spellings exactly. */
void calcVecAngXY(nw4r::math::VEC3* v, u32* x, u32* y);
void rotVecY(nw4r::math::VEC3* v, u32 angle);

/* `get_camera_pos__Fv` returns its vector through the caller's slot: every call site passes it in r3, which the
 * by-value spelling only reproduces with a temporary, so the call goes through the out-pointer view. */
#define EFT_CAMERA_POS(out) ((void (*)(nw4r::math::VEC3*))get_camera_pos)(out)
/* The camera direction entry (`get_camera_direction__Fv`). */
nw4r::math::VEC3 get_camera_direction(void); /* target references get_camera_direction__Fv (relocaudit) */

/* ---------------------------------------------------------------------------------------------------
 * engine records
 * ------------------------------------------------------------------------------------------------- */

/* The effect pool the eft019 record carries at `_EFT::work_0x38`.  The size is `eft_res_slot_get`'s
 * argument, 0x58.  `count` is how many effects the id asks for (`eft019_effect_counts[type]`, whose maximum
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
    /* +0x4C */ _GXColor color_0x4C[3];  /* one tint per effect slot; `eft019_rgb_key_table_set_19` has 3 rows */
};


/* The actor record `eft019_set_mazule` and `eft019_shot_set` read: +0x009 is the kind byte the mazule path compares
 * against 3 (`pl.h` still spells it `unk009`, `enemy.h` names the same byte `state_0x009`) and +0x016 is `pl.h`'s
 * `_PLW::area_0x16`. size: 0x208 - lower bound (the record continues past what this unit reads). */
struct _EFT019_ACTOR {
    /* +0x000 */ u8 pad_0x000[0x009];
    /* +0x009 */ u8 kind_0x09;
    /* +0x00A */ u8 pad_0x00A[0x016 - 0x00A];
    /* +0x016 */ u8 area_0x16;
    /* +0x017 */ u8 pad_0x017[0x208 - 0x017];
};

/* The physics sub-record the actor holds at +0x13C: its character model sits at +0x04.  `pl.h`
 * forward-declares the same layout as `_PLW_PHYSICS` (owned by `ef/fn_80114E34.cpp`), so this unit
 * carries only the offset it walks. size: 0x144 - lower bound. */
struct _EFT019_PHYSICS {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ MHchar chr_0x04;
};


#pragma pool_data off /* each table is reached by name, never through a shared .data anchor (playbook 194) */

/* --- the unit's own data, in retail's emission order ---------------------------------------------
 * The effect counts, `res_eft_create` groups and ids per type, the pair ids of subtypes 9..11, and the colour
 * key strips (`{frame, r, g, b}` keys, `0xFF` ends; alpha strips are `{frame, a}` pairs) with the per-slot
 * tables that pick a strip by subtype.  A strip of 8 bytes lands in `.sdata`, a longer one in `.data`,
 * so the definition order below is the one both sections share.
 * ------------------------------------------------------------------------------------------------- */

u8 eft019_effect_counts[140] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x07, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
};
u16 eft019_effect_groups[138] = {
    0x0030, 0x0031, 0x0032, 0x00E9, 0x00EA, 0x00EB, 0x0000, 0x0033, 0x0036, 0x00D5, 0x00D6, 0x00EC,
    0x00F2, 0x0115, 0x0116, 0x0117, 0x0118, 0x013F, 0x0140, 0x0180, 0x01A1, 0x01A7, 0x01A8, 0x0032,
    0x01E6, 0x01E7, 0x01F4, 0x01A1, 0x0030, 0x0116, 0x019B, 0x028A, 0x0298, 0x02B0, 0x02C1, 0x02B9,
    0x02BD, 0x02BE, 0x02BF, 0x02F2, 0x0338, 0x0339, 0x0340, 0x0341, 0x0342, 0x0363, 0x0367, 0x0398,
    0x0399, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x02C0, 0x03FA, 0x0286, 0x0030, 0x00F2, 0x0033,
    0x0413, 0x0414, 0x0425, 0x0426, 0x042B, 0x042B, 0x01A1, 0x0483, 0x035B, 0x035C, 0x035D, 0x035E,
    0x04C4, 0x04C5, 0x04C8, 0x04C7, 0x04C6, 0x04CF, 0x042B, 0x00D5, 0x01F4, 0x04D5, 0x04D6, 0x0513,
    0x013F, 0x02BE, 0x02BF, 0x05EE, 0x0612, 0x0615, 0x061C, 0x061D, 0x061E, 0x061F, 0x02BD, 0x02B9,
    0x0363, 0x0625, 0x0625, 0x042B, 0x042B, 0x0695, 0x0636, 0x0636, 0x0696, 0x0697, 0x06AA, 0x06AA,
    0x06B7, 0x072B, 0x072C, 0x014C, 0x0805, 0x088F, 0x0890, 0x0891, 0x0887, 0x0888, 0x0889, 0x088A,
    0x08F8, 0x0996, 0x0995, 0x09F1, 0x09F2, 0x09F4, 0x019C, 0x0A13, 0x0A14, 0x0A1D, 0x0138, 0x01A3,
    0x0A2A, 0x0A36, 0x052B, 0x0A4B, 0x00F2, 0x0000,
};
u16 eft019_effect_ids[138] = {
    0x0008, 0x0008, 0x0008, 0x0003, 0x0003, 0x0003, 0x0000, 0x0008, 0x0008, 0x0015, 0x0015, 0x0003,
    0x0008, 0x0014, 0x0014, 0x0014, 0x0014, 0x0026, 0x0026, 0x0015, 0x0020, 0x0026, 0x0026, 0x0008,
    0x0033, 0x0033, 0x0015, 0x0020, 0x0008, 0x0014, 0x0015, 0x0032, 0x003F, 0x0041, 0x0032, 0x0043,
    0x0043, 0x0043, 0x0043, 0x0015, 0x0026, 0x0026, 0x0014, 0x0014, 0x0014, 0x0043, 0x0025, 0x0014,
    0x0006, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x0033, 0x0032, 0x0025, 0x0008, 0x0008, 0x0008,
    0x0003, 0x0003, 0x0007, 0x0003, 0x005A, 0x005A, 0x0020, 0x005A, 0x004C, 0x004C, 0x004C, 0x004C,
    0x005E, 0x005E, 0x005E, 0x005E, 0x005E, 0x005A, 0x005A, 0x0015, 0x0015, 0x0026, 0x0026, 0x0026,
    0x0026, 0x0043, 0x0043, 0x006F, 0x0071, 0x0071, 0x0072, 0x0072, 0x0072, 0x0072, 0x0043, 0x0043,
    0x0043, 0x0043, 0x0043, 0x005A, 0x005A, 0x0075, 0x0015, 0x0015, 0x0075, 0x0075, 0x0043, 0x0043,
    0x0025, 0x0015, 0x0015, 0x0022, 0x005A, 0x0062, 0x0062, 0x0062, 0x0062, 0x0062, 0x0062, 0x0062,
    0x0062, 0x0062, 0x0062, 0x0062, 0x0062, 0x0062, 0x0015, 0x006F, 0x006F, 0x00A1, 0x0013, 0x0028,
    0x003F, 0x004C, 0x0062, 0x0062, 0x0008, 0x0000,
};
u16 eft019_pair_effect_ids[6] = {
    0x019C, 0x019D, 0x019E, 0x0182, 0x0182, 0x0182,
};
u8 eft019_rgb_keys_19_a1[8] = {
    0x00, 0xBF, 0xFF, 0x7F, 0xFF, 0xBF, 0xFF, 0x7F,
};
u8 eft019_rgb_keys_19_a2[8] = {
    0x00, 0xAF, 0x8F, 0xFF, 0xFF, 0xAF, 0x8F, 0xFF,
};
u8 eft019_rgb_keys_19_a3[8] = {
    0x00, 0xFF, 0xDF, 0x7F, 0xFF, 0xFF, 0xDF, 0x7F,
};
u8 eft019_rgb_keys_19_a4[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a5[8] = {
    0x00, 0xFF, 0xAF, 0xAF, 0xFF, 0xFF, 0xAF, 0xAF,
};
u8 eft019_rgb_keys_19_a6[8] = {
    0x00, 0xFF, 0xBF, 0x3F, 0xFF, 0xFF, 0xBF, 0x3F,
};
u8 eft019_rgb_keys_19_a7[8] = {
    0x00, 0xFF, 0xBF, 0xBF, 0xFF, 0xFF, 0xBF, 0xBF,
};
u8 eft019_rgb_keys_19_a8[8] = {
    0x00, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0x7F,
};
u8 eft019_rgb_keys_19_a9[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a10[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a11[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a12[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a13[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a14[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_a15[8] = {
    0x00, 0xAF, 0xAF, 0xAF, 0xFF, 0xAF, 0xAF, 0xAF,
};
u8 eft019_rgb_keys_19_a16[8] = {
    0x00, 0xAF, 0xAF, 0xAF, 0xFF, 0xAF, 0xAF, 0xAF,
};
u8 eft019_rgb_keys_19_a17[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x02, 0x7F, 0x8F, 0xFF, 0xFF, 0x7F, 0x8F, 0xFF,
};
u8 eft019_rgb_keys_19_a18[8] = {
    0x00, 0xFF, 0x4F, 0x4F, 0xFF, 0xFF, 0x4F, 0x4F,
};
u8* eft019_rgb_key_tables_19_a[19] = {
    eft019_rgb_keys_19_a11, eft019_rgb_keys_19_a1, eft019_rgb_keys_19_a2, eft019_rgb_keys_19_a3,
    eft019_rgb_keys_19_a4, eft019_rgb_keys_19_a5, eft019_rgb_keys_19_a6, eft019_rgb_keys_19_a7,
    eft019_rgb_keys_19_a8, eft019_rgb_keys_19_a9, eft019_rgb_keys_19_a10, eft019_rgb_keys_19_a11,
    eft019_rgb_keys_19_a12, eft019_rgb_keys_19_a13, eft019_rgb_keys_19_a14, eft019_rgb_keys_19_a15,
    eft019_rgb_keys_19_a16, eft019_rgb_keys_19_a17, eft019_rgb_keys_19_a18,
};
u8 eft019_rgb_keys_19_b1[12] = {
    0x00, 0xFF, 0xFF, 0xBF, 0x02, 0xDF, 0xFF, 0xBF, 0xFF, 0xDF, 0xFF, 0xBF,
};
u8 eft019_rgb_keys_19_b2[12] = {
    0x00, 0xDC, 0xD2, 0xFF, 0x02, 0x73, 0x50, 0xFF, 0xFF, 0x73, 0x50, 0xFF,
};
u8 eft019_rgb_keys_19_b3[12] = {
    0x00, 0xFF, 0xFF, 0xDF, 0x02, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0x7F,
};
u8 eft019_rgb_keys_19_b4[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x02, 0x8F, 0x8F, 0xFF, 0xFF, 0x8F, 0x8F, 0xFF,
};
u8 eft019_rgb_keys_19_b5[12] = {
    0x00, 0xFF, 0xBF, 0xA4, 0x02, 0xFF, 0x7F, 0xA4, 0xFF, 0xFF, 0x7F, 0xA4,
};
u8 eft019_rgb_keys_19_b6[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x02, 0xFF, 0x7F, 0x40, 0xFF, 0xFF, 0x7F, 0x40,
};
u8 eft019_rgb_keys_19_b7[12] = {
    0x00, 0xB6, 0xFF, 0xFF, 0x02, 0xFF, 0x35, 0x2C, 0xFF, 0xFF, 0x35, 0x2C,
};
u8 eft019_rgb_keys_19_b8[12] = {
    0x00, 0xFF, 0xFF, 0xBF, 0x02, 0xFF, 0x9A, 0x00, 0xFF, 0xFF, 0x9A, 0x00,
};
u8 eft019_rgb_keys_19_b9[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_b10[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_b11[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_b12[12] = {
    0x00, 0x94, 0x57, 0x28, 0x02, 0x46, 0x46, 0x46, 0xFF, 0x46, 0x46, 0x46,
};
u8 eft019_rgb_keys_19_b13[12] = {
    0x00, 0x94, 0x57, 0x28, 0x02, 0x46, 0x46, 0x46, 0xFF, 0x46, 0x46, 0x46,
};
u8 eft019_rgb_keys_19_b14[12] = {
    0x00, 0x94, 0x57, 0x28, 0x02, 0x46, 0x46, 0x46, 0xFF, 0x46, 0x46, 0x46,
};
u8 eft019_rgb_keys_19_b15[12] = {
    0x00, 0xBF, 0xBF, 0xBF, 0x02, 0x91, 0xA6, 0xFF, 0xFF, 0x91, 0xA6, 0xFF,
};
u8 eft019_rgb_keys_19_b16[12] = {
    0x00, 0xCF, 0xCF, 0xCF, 0x02, 0xB7, 0xBF, 0xBF, 0xFF, 0xB7, 0xBF, 0xBF,
};
u8 eft019_rgb_keys_19_b17[12] = {
    0x00, 0xAF, 0xBF, 0xFF, 0x02, 0x5F, 0x7F, 0xFF, 0xFF, 0x5F, 0x7F, 0xFF,
};
u8 eft019_rgb_keys_19_b18[12] = {
    0x00, 0xFF, 0xBF, 0xBF, 0x02, 0xFF, 0x2F, 0x2F, 0xFF, 0xFF, 0x2F, 0x2F,
};
u8* eft019_rgb_key_tables_19_b[19] = {
    eft019_rgb_keys_19_b11, eft019_rgb_keys_19_b1, eft019_rgb_keys_19_b2, eft019_rgb_keys_19_b3,
    eft019_rgb_keys_19_b4, eft019_rgb_keys_19_b5, eft019_rgb_keys_19_b6, eft019_rgb_keys_19_b7,
    eft019_rgb_keys_19_b8, eft019_rgb_keys_19_b9, eft019_rgb_keys_19_b10, eft019_rgb_keys_19_b11,
    eft019_rgb_keys_19_b12, eft019_rgb_keys_19_b13, eft019_rgb_keys_19_b14, eft019_rgb_keys_19_b15,
    eft019_rgb_keys_19_b16, eft019_rgb_keys_19_b17, eft019_rgb_keys_19_b18,
};
u8 eft019_rgb_keys_19_c0[8] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_c1[12] = {
    0x00, 0xBF, 0xFF, 0x8F, 0x0B, 0xBF, 0xFF, 0xAF, 0xFF, 0xBF, 0xFF, 0xAF,
};
u8 eft019_rgb_keys_19_c2[12] = {
    0x00, 0xFF, 0x5F, 0xFF, 0x0B, 0xBF, 0x7F, 0xFF, 0xFF, 0xBF, 0x7F, 0xFF,
};
u8 eft019_rgb_keys_19_c3[12] = {
    0x00, 0xFF, 0xFF, 0xAA, 0x0B, 0xFF, 0xFF, 0xA4, 0xFF, 0xFF, 0xFF, 0xA4,
};
u8 eft019_rgb_keys_19_c4[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x0B, 0xBF, 0xBF, 0xFF, 0xFF, 0xBF, 0xBF, 0xFF,
};
u8 eft019_rgb_keys_19_c5[12] = {
    0x00, 0xFF, 0xDF, 0xAF, 0x0B, 0xFF, 0x9F, 0x8F, 0xFF, 0xFF, 0x9F, 0x8F,
};
u8 eft019_rgb_keys_19_c15[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
u8 eft019_rgb_keys_19_c7[12] = {
    0x00, 0xFF, 0xBF, 0xBF, 0x0B, 0xFF, 0x6F, 0x6F, 0xFF, 0xFF, 0x6F, 0x6F,
};
u8 eft019_rgb_keys_19_c8[12] = {
    0x00, 0xFF, 0xDF, 0x8F, 0x0B, 0xFF, 0xFA, 0xA7, 0xFF, 0xFF, 0xFA, 0xA7,
};
u8 eft019_rgb_keys_19_c6[12] = {
    0x00, 0x64, 0xBF, 0xFF, 0x0B, 0x64, 0xBF, 0xFF, 0xFF, 0x64, 0xBF, 0xFF,
};
u8* eft019_rgb_key_tables_19_c[19] = {
    eft019_rgb_keys_19_c0, eft019_rgb_keys_19_c1, eft019_rgb_keys_19_c2, eft019_rgb_keys_19_c3,
    eft019_rgb_keys_19_c4, eft019_rgb_keys_19_c5, eft019_rgb_keys_19_c6, eft019_rgb_keys_19_c7,
    eft019_rgb_keys_19_c8, eft019_rgb_keys_19_c0, eft019_rgb_keys_19_c0, eft019_rgb_keys_19_c0,
    eft019_rgb_keys_19_c0, eft019_rgb_keys_19_c0, eft019_rgb_keys_19_c0, eft019_rgb_keys_19_c15,
    eft019_rgb_keys_19_c15, eft019_rgb_keys_19_c15, eft019_rgb_keys_19_c15,
};
u8** eft019_rgb_key_table_set_19[3] = {
    eft019_rgb_key_tables_19_a, eft019_rgb_key_tables_19_b, eft019_rgb_key_tables_19_c,
};
u8 eft019_tint_rgb_32[15] = {
    0xC8, 0xC8, 0xC8, 0x64, 0xC8, 0x64, 0x90, 0x80, 0x10, 0x71, 0x61, 0xA5,
    0xC8, 0xF0, 0xFF,
};
u8 eft019_rgb_keys_56[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x22, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
u8 eft019_rgb_keys_0_28[20] = {
    0x00, 0xBD, 0x56, 0x16, 0x06, 0xBD, 0x56, 0x16, 0x0A, 0xA6, 0x3C, 0x18,
    0x0E, 0x1A, 0x1A, 0x1A, 0xFF, 0x1A, 0x1A, 0x1A,
};
u8 eft019_alpha_keys_0_28[12] = {
    0x00, 0x40, 0x06, 0x40, 0x07, 0xFF, 0x12, 0xFF, 0x19, 0x00, 0xFF, 0x00,
};
u8 eft019_alpha_keys_32[8] = {
    0x00, 0x00, 0x14, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
};
u8 eft019_alpha_keys_57[8] = {
    0x00, 0x7F, 0x0A, 0xEF, 0x19, 0x00, 0xFF, 0x00,
};
u8 eft019_rgb_keys_57[12] = {
    0x00, 0xF4, 0xBF, 0x9F, 0x14, 0xFF, 0x45, 0x58, 0xFF, 0xFF, 0x45, 0x58,
};
u8 eft019_rgb_keys_102_b0[16] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0x0C, 0xFF, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00,
};
u8 eft019_rgb_keys_102_c0[16] = {
    0x00, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0x0C, 0xFF, 0x00, 0x00,
    0xFF, 0xFF, 0x00, 0x00,
};
u8 eft019_rgb_keys_102_a0[8] = {
    0x00, 0xFF, 0xC8, 0xC8, 0xFF, 0xFF, 0xC8, 0xC8,
};
u8 eft019_rgb_keys_102_a1[8] = {
    0x00, 0xA1, 0xFF, 0xEE, 0xFF, 0xA1, 0xFF, 0xEE,
};
u8 eft019_rgb_keys_102_b1[8] = {
    0x00, 0x64, 0xFF, 0xDE, 0xFF, 0x64, 0xFF, 0xDE,
};
u8 eft019_rgb_keys_102_c1[8] = {
    0x00, 0xD2, 0xFF, 0xE4, 0xFF, 0xD2, 0xFF, 0xE4,
};
u8 eft019_rgb_keys_102_a2[8] = {
    0x00, 0xFF, 0xFF, 0xDE, 0xFF, 0xFF, 0xFF, 0xDE,
};
u8 eft019_rgb_keys_102_b2[8] = {
    0x00, 0xFF, 0xFF, 0x92, 0xFF, 0xFF, 0xFF, 0x92,
};
u8 eft019_rgb_keys_102_c2[8] = {
    0x00, 0xF0, 0xFF, 0xBF, 0xFF, 0xF0, 0xFF, 0xBF,
};
u8* eft019_rgb_key_tables_102_a[3] = {
    eft019_rgb_keys_102_a0, eft019_rgb_keys_102_a1, eft019_rgb_keys_102_a2,
};
u8* eft019_rgb_key_tables_102_b[3] = {
    eft019_rgb_keys_102_b0, eft019_rgb_keys_102_b1, eft019_rgb_keys_102_b2,
};
u8* eft019_rgb_key_tables_102_c[3] = {
    eft019_rgb_keys_102_c0, eft019_rgb_keys_102_c1, eft019_rgb_keys_102_c2,
};
u8 eft019_tint_rgb_132[20] = {
    0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0x00, 0xFF, 0x1F, 0x00,
    0x00, 0x7F, 0xFF, 0x71, 0x61, 0xA5, 0x00, 0x00,
};

/* The bodies keep retail's `u8` argument masks and unfused record-form compares (playbook 39). */
#pragma peephole off
#pragma optimization_level 4

/* 0x801121DC (0x434): Spawns an eft019 record and orients it per camera case. */

void eft019_set(nw4r::math::VEC3* pos, u8 area, u8 type)
{
    nw4r::math::VEC3 v;
    _EFT* effect;

    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect == 0) {
        return;
    }

    switch (type) {
    case 0: {
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 12, 1.0f, 1.0f);
        return;
    }
    case 1: {
        _EFT019_WORK* work;
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 12, 1.5f, 1.5f);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = work->scale_b_0x48 = 1.0f;
        return;
    }
    case 2: {
        _EFT019_WORK* work;
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 12, 1.9f, 1.9f);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = work->scale_b_0x48 = 0.7f;
        return;
    }
    case 14: {
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 13, 1.0f, 1.0f);
        return;
    }
    case 15: {
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 13, 1.5f, 1.5f);
        return;
    }
    case 16: {
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 13, 2.25f, 2.25f);
        return;
    }
    case 23: {
        _EFT019_WORK* work;
        eft019_set_core(pos, area, 12, 4.0f, 4.0f);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = work->scale_b_0x48 = 2.0f;
        return;
    }
    case 28: {
        _EFT019_WORK* work;
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 12, 0.7f, 0.7f);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = work->scale_b_0x48 = 0.7f;
        return;
    }
    case 29: {
        _EFT019_WORK* work;
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 13, 0.7f, 0.7f);
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_a_0x44 = work->scale_b_0x48 = 0.7f;
        return;
    }
    case 57: {
        v = *(&get_camera_direction());
        vec3_normalize_into(&v, &v);
        vec3_scale_in_place(&v, -5.0f);
        *pos += v;
        eft019_set_core(pos, area, 58, 0.85f, 0.85f);
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

    /* The three ids this function is called with are the ones below (3..5 -> 11, 18/21/22/83 -> 17); on any other
     * id `effect` is never set (retail tests the id's register). */
    effect = (_EFT*)type;
    switch (type) {
    case 3:
    case 4:
    case 5:
        effect = fn_80114A1C(pos, area, 11);
        break;
    case 18:
    case 21:
    case 22:
    case 83:
        effect = fn_80114A1C(pos, area, 17);
        break;
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
        eft042_set_placed(plw, 11, pos, rot, plw->area_0x16, 0.68f);
        effect = fn_80114A1C(pos, area, 19);
        break;
    case 18:
        eft042_set_placed(plw, 12, pos, rot, plw->area_0x16, 0.87f);
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
        effect->rot_0x24.x = rot->x;
        effect->rot_0x24.y = rot->y;
        effect->rot_0x24.z = 0;
        work = (_EFT019_WORK*)effect->work_0x38;
        work->subtype_0x40 = id;
    }
}

/* 0x80112994 (0x1E8): Spawns an eft019 record of a subtype with its rotation, timer and scale. */

void eft019_set_subtype(u8 type, u8 subtype, nw4r::math::VEC3* pos, u8 area, _CP_VECTOR* rot,
                        s32 timer, f32 scale)
{
    nw4r::math::VEC3 v;
    _EFT* effect;
    _EFT019_WORK* work;

    effect = fn_80114A1C(pos, (u8)area, (u8)type);
    if (effect != 0) {
        work = (_EFT019_WORK*)effect->work_0x38;
        switch (type) {
        case 0: {
            v = *(&get_camera_direction());
            vec3_normalize_into(&v, &v);
            vec3_scale_in_place(&v, -5.0f);
            *pos += v;
            eft019_set_core(pos, area, 12, 1.0f, 1.0f);
            break;
        }
        case 1: {
            v = *(&get_camera_direction());
            vec3_normalize_into(&v, &v);
            vec3_scale_in_place(&v, -5.0f);
            *pos += v;
            eft019_set_core(pos, area, 12, 1.5f, 1.5f);
            work->scale_a_0x44 = work->scale_b_0x48 = 1.0f;
            break;
        }
        case 2: {
            v = *(&get_camera_direction());
            vec3_normalize_into(&v, &v);
            vec3_scale_in_place(&v, -5.0f);
            *pos += v;
            eft019_set_core(pos, area, 12, 1.9f, 1.9f);
            work->scale_a_0x44 = work->scale_b_0x48 = 0.7f;
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

void eft019_shot_set(void* source, u8 key, u8 subtype, nw4r::math::VEC3* pos, _CP_VECTOR* rot,
                 f32 scale)
{
    _EFT* effect;
    _EFT019_WORK* work;
    u8 t;

    effect = fn_80114A1C(pos, ((_EFT019_ACTOR*)source)->area_0x16, key);
    if (effect != 0) {
        effect->rot_0x24 = *rot;
        effect->source_0x30 = source;
        work = (_EFT019_WORK*)effect->work_0x38;
        work->scale_b_0x48 = scale;
        work->scale_a_0x44 = scale;
        if ((s32)key == 102) {
            t = pl_frame_eft_color_get(source);
            switch (t) {
            case 0:
            case 1:
            case 2:
                subtype = t;
                break;
            case 3:
                effect->type_0x02 = 103;
                break;
            }
        }
        work->subtype_0x40 = subtype;
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
    _EFT019_WORK* work;
    struct EfPmManager* pm;
    void* emitter;
    s32 snd;
    s32 i;
    u16 snd_count;

    if (self->type_0x02 == 6) {
        self->state_0x05++;
        self->timer_0x0C = 2;
        return;
    }
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 dir;
    work = (_EFT019_WORK*)self->work_0x38;
    snd = eft_control.field_0x04;
    self->state_0x05++;
    work->effects[0] = res_eft_create(eft019_effect_groups[self->type_0x02], eft019_effect_ids[self->type_0x02], 0);
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
            res_eft_create((u16)(self->type_0x02 + 0x3FF), eft019_effect_ids[self->type_0x02], 0);
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
                    (u16)(eft019_effect_groups[self->type_0x02] + i), eft019_effect_ids[self->type_0x02], 0);
                if (work->effects[i] == 0) {
                    fn_8011485C(self);
                    return;
                }
            }
            break;
        case 6:
            work->effects[1] = res_eft_create(
                (u16)(eft019_effect_groups[self->type_0x02] + 1), eft019_effect_ids[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(
                (u16)(eft019_effect_groups[self->type_0x02] + 3), eft019_effect_ids[self->type_0x02], 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 9:
        case 10:
        case 11:
            work->count = 2;
            work->effects[1] = res_eft_create(eft019_pair_effect_ids[work->subtype_0x40 - 9], 0x15, 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 12:
        case 13:
        case 14:
            work->count = 3;
            work->effects[1] = res_eft_create(eft019_pair_effect_ids[work->subtype_0x40 - 9], 0x15, 0);
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
                (u16)(eft019_effect_groups[self->type_0x02] + 1), eft019_effect_ids[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(0x626, eft019_effect_ids[self->type_0x02], 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 16:
            work->effects[1] = res_eft_create(
                (u16)(eft019_effect_groups[self->type_0x02] + 1), eft019_effect_ids[self->type_0x02], 0);
            if (work->effects[1] == 0) {
                fn_8011485C(self);
                return;
            }
            work->effects[2] = res_eft_create(0x627, eft019_effect_ids[self->type_0x02], 0);
            if (work->effects[2] == 0) {
                fn_8011485C(self);
                return;
            }
            break;
        case 17:
        case 18:
            work->effects[1] = res_eft_create(
                (u16)(eft019_effect_groups[self->type_0x02] + 1), eft019_effect_ids[self->type_0x02], 0);
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
                u8* tint = eft019_rgb_key_table_set_19[i][work->subtype_0x40];
                work->color_0x4C[i].r = tint[1];
                work->color_0x4C[i].g = tint[2];
                work->color_0x4C[i].b = tint[3];
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
        work->effects[1] = res_eft_create(0x40D, eft019_effect_ids[self->type_0x02], 0);
        if (work->effects[1] == 0) {
            fn_8011485C(self);
            return;
        }
        break;
    case 0x20:
        work->color_0x4C[0].r = eft019_tint_rgb_32[work->subtype_0x40 * 3];
        work->color_0x4C[0].g = eft019_tint_rgb_32[work->subtype_0x40 * 3 + 1];
        work->color_0x4C[0].b = eft019_tint_rgb_32[work->subtype_0x40 * 3 + 2];
        work->color_0x4C[0].a = 0xFF;
        break;
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
        work->color_0x4C[0].g = (col >> 16) & 0xFF;
        work->color_0x4C[0].b = (col >> 8) & 0xFF;
        work->color_0x4C[0].a = (u8)col;
        break;
    }
    case 0x4A:
    case 0x52:
    case 0x53: {
        u32 col = get_stg_eft_col(self->area_0x44, 1);
        work->color_0x4C[0].r = (u8)(col >> 24);
        work->color_0x4C[0].g = (col >> 16) & 0xFF;
        work->color_0x4C[0].b = (col >> 8) & 0xFF;
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
            work->color_0x4C[0].g = (col >> 16) & 0xFF;
            work->color_0x4C[0].b = (col >> 8) & 0xFF;
            work->color_0x4C[0].a = (u8)col;
        }
        break;
    case 0x84:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
        work->color_0x4C[0].r = eft019_tint_rgb_132[work->subtype_0x40 * 3];
        work->color_0x4C[0].g = eft019_tint_rgb_132[work->subtype_0x40 * 3 + 1];
        work->color_0x4C[0].b = eft019_tint_rgb_132[work->subtype_0x40 * 3 + 2];
        work->color_0x4C[0].a = 0xFF;
        break;
    case 0x88:
        SetRootMtxTrans(work->effects[0], &self->pos_0x18);
        effect_retire(work->effects[0], 0);
        emitter = ef_effect_emitter_get(work->effects[0], 0);
        snd_count = ef_emitter_pm_count();
        pm = (struct EfPmManager*)ef_emitter_pm_get(emitter, snd_count - 1);
        fn_800AB9F4(pm);
        fn_800A67E8(emitter, pm);
        ((EftMemoryManager*)ef_system_memory_manager(snd))->virtual_0x0C();
        break;
    }

    for (i = 0; i < work->count; i++) {
        SetRootMtxTrans(work->effects[i], &self->pos_0x18);
    }
    if ((u32)(self->type_0x02 - 0x66) <= 1) {
        dir = *(&get_camera_direction());
        vec3_normalize_into(&dir, &dir);
        vec3_scale_in_place(&dir, -5.0f);
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
        if (work->subtype_0x40 == 0) {
            fn_800DA9C4(&self->pos_0x18);
        }
        break;
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
        se_slot_req_pos(0, 0x14, &self->pos_0x18);
        break;
    }
    fn_8011392C(self);
}

#pragma fp_contract off
/* 0x8011392C (0xF20): Runs the record one frame by type: moves the effects, steps the colour keys and the per-type
 * placement, ends the record when its effects finish, and spawns the models (the player's stage flag joins the mode). */
void fn_8011392C(_EFT* self)
{
    u8 done = 0;
    nw4r::math::MTX34 mtx;
    _EFT019_WORK* work;
    _PLW* src;
    nw4r::ef::EffectSystem* sys;
    s32 mode;
    s32 i;
    s32 j;
    s32 hold;
    _GXColor col;
    u8* keys;
    s32 frame;

    nw4r::math::VEC3 pos;
    work = (_EFT019_WORK*)self->work_0x38;
    src = (_PLW*)self->source_0x30;
    sys = (nw4r::ef::EffectSystem*)eft_control.field_0x04;
    if (self->type_0x02 == 6) {
        fn_801148E4(self);
        return;
    }
    if (self->field_0x07 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    switch (self->type_0x02) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x1C:
    case 0x39:
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        self->field_0x10++;
        switch (self->type_0x02) {
        case 0x00:
        case 0x1C:
            eftGetKeyRGB(eft019_rgb_keys_0_28, self->field_0x10, &work->color_0x4C[0].r, &work->color_0x4C[0].g,
                         &work->color_0x4C[0].b);
            work->color_0x4C[0].a = eftGetKeyAlpha(eft019_alpha_keys_0_28, self->field_0x10);
            change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
            break;
        case 0x39:
            eftGetKeyRGB(eft019_rgb_keys_57, self->field_0x10, &work->color_0x4C[0].r, &work->color_0x4C[0].g,
                         &work->color_0x4C[0].b);
            work->color_0x4C[0].a = eftGetKeyAlpha(eft019_alpha_keys_57, self->field_0x10);
            change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
            break;
        }
        pos = self->pos_0x18;
        pos.y += 50.0f * work->scale_a_0x44;
        eft_res_models_spawn(self, (void**)work->effects, 1, work->count, &pos);
        return;
    case 0x03:
    case 0x04:
    case 0x05:
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        mtx34_identity(&mtx);
        rotLocalMatY(self->rot_0x24.y, &mtx);
        mtx.m[0][3] = self->pos_0x18.x;
        mtx.m[1][3] = self->pos_0x18.y;
        mtx.m[2][3] = self->pos_0x18.z;
        work->effects[0]->SetRootMtx(mtx);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        break;
    case 0x07:
    case 0x3B:
        if (--self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        for (i = 0; i < work->count; i++) {
            effect_retire(work->effects[i], 0);
        }
        if ((s32)self->type_0x02 == 0x3B) {
            change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
        }
        break;
    case 0x08:
        if (--self->timer_0x0C < 0) {
            work->count = work->live_0x3C;
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        for (i = 0; i < work->live_0x3C; i++) {
            if (effect_emitter_alive_ck(work->effects[i]) == 0) {
                sys->RetireEffect(work->effects[i]);
                for (j = i; j < work->live_0x3C - 1; j++) {
                    work->effects[j] = work->effects[j + 1];
                }
                work->live_0x3C--;
                work->effects[work->live_0x3C] = NULL;
            }
        }
        if (--self->field_0x10 < 0) {
            if (work->live_0x3C <= work->count) {
                work->effects[work->live_0x3C] = res_eft_create(0x36, 8, 0);
                if (work->effects[work->live_0x3C] == NULL) {
                    self->flag_0x01 = 0;
                    self->state_0x05++;
                    return;
                }
                SetRootMtxTrans(work->effects[work->live_0x3C++], &self->pos_0x18);
            }
            self->field_0x10 = 8;
        }
        for (i = 0; i < work->live_0x3C; i++) {
            effect_retire(work->effects[i], 0);
        }
        break;
    case 0x0B:
        if (++self->field_0x10 == 5) {
            eft019_set_tyakudan(&self->pos_0x18, &self->rot_0x24, self->area_0x44, work->subtype_0x40,
                                work->scale_a_0x44);
        }
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        break;
    case 0x0C:
    case 0x3A:
        eft_effect_foreach_pm_scale(work->effects[0], (u32)&work->scale_a_0x44, 1);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        pos = self->pos_0x18;
        switch (self->type_0x02) {
        case 0x0C:
            pos.y += 50.0f * work->scale_a_0x44;
            break;
        case 0x3A:
            pos.y += 50.0f * work->scale_a_0x44;
            change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
            break;
        }
        eft_res_models_spawn(self, (void**)work->effects, 1, work->count, &pos);
        return;
    case 0x0E:
    case 0x0F:
    case 0x10:
    case 0x1D:
        hold = 0;
        for (i = 0; i < work->count; i++) {
            if (effect_move(work->effects[i]) == 0) {
                hold++;
            }
        }
        if (hold == work->count) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        eft_res_models_spawn(self, (void**)&work->effects[0], 1, 1, NULL);
        {
            nw4r::math::_VEC3 cam;
            EFT_CAMERA_POS((nw4r::math::VEC3*)&cam);
            if (cam.y < 0.0f) {
                eft_res_models_spawn(self, (void**)&work->effects[1], 1, 1, NULL);
            }
        }
        return;
    case 0x13:
        self->field_0x10++;
        for (i = 0; i < work->count; i++) {
            if (effect_move(work->effects[i]) == 0) {
                done++;
            }
        }
        if (done >= work->count) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        switch (work->subtype_0x40) {
        case 9:
        case 10:
        case 11:
            hold = 0xFF;
            break;
        case 12:
        case 13:
        case 14:
            hold = 3;
            for (i = 1; i < work->count; i++) {
                change_paramscale_eff(work->effects[i], work->scale_a_0x44);
            }
            break;
        default:
            hold = 0;
            break;
        }
        if (self->field_0x06 != 0) {
            keys = eft019_rgb_key_table_set_19[1][work->subtype_0x40];
            if (self->field_0x10 > hold) {
                eftGetKeyRGB(keys, self->field_0x10 - hold, &work->color_0x4C[1].r, &work->color_0x4C[1].g,
                             &work->color_0x4C[1].b);
            }
            keys = eft019_rgb_key_table_set_19[2][work->subtype_0x40];
            if (self->field_0x10 > 3) {
                eftGetKeyRGB(keys, self->field_0x10 - hold, &work->color_0x4C[2].r, &work->color_0x4C[2].g,
                             &work->color_0x4C[2].b);
            }
        }
        switch (work->subtype_0x40) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 7:
        case 8:
        case 12:
        case 13:
        case 14:
            fn_80114B20(self, &mtx);
            switch (work->subtype_0x40) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 7:
            case 8:
                mtx34_trans_get(&mtx, &pos);
                SetRootMtxTrans(work->effects[2], &pos);
                work->color_0x4C[1].a = 0x87;
                break;
            case 12:
            case 13:
            case 14:
                mtx34_trans_get(&mtx, &pos);
                for (i = 1; i < work->count; i++) {
                    SetRootMtxTrans(work->effects[i], &pos);
                }
                work->color_0x4C[2].a = 0x87;
                break;
            }
            for (i = 0; i < work->count; i++) {
                change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x4C[i]);
            }
            break;
        case 6:
            work->color_0x4C[1].a = 0x87;
            if (self->demo_flag_0x08 == 1) {
                for (i = 0; i < work->count; i++) {
                    change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x4C[i]);
                }
            } else {
                for (i = 0; i < work->count - 1; i++) {
                    change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x4C[i]);
                }
            }
            break;
        case 15:
        case 16:
            work->color_0x4C[1].a = 0x87;
            for (i = 0; i < work->count - 1; i++) {
                change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x4C[i]);
            }
            break;
        case 17:
        case 18:
            work->color_0x4C[1].a = 0x87;
            for (i = 0; i < work->count; i++) {
                change_color_eff(work->effects[i], &self->pos_0x18, work->color_0x4C[i]);
            }
            break;
        }
        if (self->field_0x06 == 0) {
            self->field_0x06++;
        }
        break;
    case 0x14:
    case 0x1B:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x5E:
    case 0x5F:
    case 0x60:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x6B:
    case 0x70:
    case 0x7F:
    case 0x82:
    case 0x83:
    case 0x84:
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
        break;
    case 0x20:
        if (--self->timer_0x0C <= 0) {
            if (self->field_0x06 != 0) {
                self->flag_0x01 = 0;
                self->state_0x05++;
                return;
            }
            self->timer_0x0C = 20;
            self->field_0x06++;
        }
        if (self->field_0x06 != 0) {
            work->color_0x4C[0].a = eftGetKeyAlpha(eft019_alpha_keys_32, self->timer_0x0C);
        }
        if (self->field_0x10 % 30 == 0) {
            se_req_eft019_tick(&self->pos_0x18);
        }
        self->field_0x10++;
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        effect_move(work->effects[0]);
        change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
        break;
    case 0x2F:
        if (--self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        effect_move(work->effects[0]);
        break;
    case 0x38:
        frame = ++self->field_0x10;
        eftGetKeyRGB(eft019_rgb_keys_56, frame, &work->color_0x4C[0].r, &work->color_0x4C[0].g, &work->color_0x4C[0].b);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
        break;
    case 0x52:
    case 0x53:
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        {
            nw4r::math::_VEC3 cam;
            EFT_CAMERA_POS((nw4r::math::VEC3*)&cam);
            if (cam.y < 0.0f) {
                change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
                eft_res_models_spawn(self, (void**)work->effects, 1, work->count, NULL);
            }
        }
        return;
    case 0x66:
        self->field_0x10++;
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        change_paramscale_eff(work->effects[1], work->scale_a_0x44);
        change_paramscale_eff(work->effects[2], 1.6f * work->scale_a_0x44);
        for (i = 0; i < work->count; i++) {
            if (effect_move(work->effects[i]) == 0) {
                done++;
            }
        }
        if (done >= work->count) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        col.a = 0xFF;
        eftGetKeyRGB(eft019_rgb_key_tables_102_a[work->subtype_0x40], self->field_0x10, &col.r, &col.g, &col.b);
        change_color_eff(work->effects[0], &self->pos_0x18, col);
        eftGetKeyRGB(eft019_rgb_key_tables_102_b[work->subtype_0x40], self->field_0x10, &col.r, &col.g, &col.b);
        change_color_eff(work->effects[1], &self->pos_0x18, col);
        eftGetKeyRGB(eft019_rgb_key_tables_102_c[work->subtype_0x40], self->field_0x10, &col.r, &col.g, &col.b);
        change_color_eff(work->effects[2], &self->pos_0x18, col);
        break;
    case 0x67:
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        change_paramscale_eff(work->effects[1], work->scale_a_0x44);
        change_paramscale_eff(work->effects[2], 1.6f * work->scale_a_0x44);
        for (i = 0; i < work->count; i++) {
            if (effect_move(work->effects[i]) == 0) {
                done++;
            }
        }
        if (done >= work->count) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
        break;
    case 0x6E:
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        switch (get_now_mapno()) {
        case 7:
        case 18:
            change_color_eff(work->effects[0], &self->pos_0x18, work->color_0x4C[0]);
            break;
        }
        break;
    case 0x88:
        self->field_0x10++;
        eft_effect_foreach_pm_scale(work->effects[0], (u32)&work->scale_a_0x44, 1);
        if (self->field_0x10 > 5) {
            if (effect_move(work->effects[0]) == 0) {
                self->flag_0x01 = 0;
                self->state_0x05++;
                return;
            }
        } else {
            effect_retire(work->effects[0], 0);
        }
        pos = self->pos_0x18;
        pos.y += 50.0f * work->scale_a_0x44;
        eft_res_models_spawn(self, (void**)work->effects, 1, work->count, &pos);
        return;
    default:
        change_paramscale_eff(work->effects[0], work->scale_a_0x44);
        if (effect_move(work->effects[0]) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        break;
    }
    mode = 1;
    if (src != NULL && Pl_master_ck(src) == 1) {
        mode = eft_stage_spawn_flag(src) | 1;
    }
    eft_res_models_spawn(self, (void**)work->effects, mode, work->count, NULL);
}
#pragma fp_contract reset

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
        effect->pos_0x18 = *pos;
        effect->area_0x44 = area;
        effect->source_0x30 = 0;
        effect->dispatch_0x34 = fn_80112D1C;
    }
}

/* 0x801148E4 (0x138): Plays the on-screen impact sound and normalises its screen position. */

#pragma fp_contract off
void fn_801148E4(_EFT* self)
{
    nw4r::math::VEC3 v;
    f32 max[2];
    _GXColor color;
    s32 handle;
    f32 rx;
    f32 ry;

    if (self->area_0x44 == get_now_areano() && self->timer_0x0C > 0) {
        screen_size_float_get((struct _MH_VEC2*)max);
        handle = scn_root_get_current_camera(pRoot);
        camera_world_to_screen((nw4r::g3d::Camera*)&handle, (u8*)&v, &self->pos_0x18);
        if (v.z <= 0.0f || v.z >= 1.0f || v.x < 0.0f ||
            v.x > max[0] || v.y < 0.0f || v.y > max[1]) {
            /* the projected point is off-screen: the impact makes no sound */
        } else {
            rx = v.x - max[0] / 2.0f;
            ry = v.y - max[1] / 2.0f;
            v.x = rx / max[0];
            v.y = ry / max[1];
            color.r = 0xFF;
            color.g = 0;
            color.b = 0;
            draw_sprite_start(ry, rx);
            draw_sprite_set(128, 3, 32, v.x, v.y, color, self->timer_0x0C);
        }
    }
    self->state_0x05++;
}
#pragma fp_contract reset

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
    work->count = eft019_effect_counts[type];
    work->unused_0x38 = 0;
    work->scale_a_0x44 = work->scale_b_0x48 = 1.0f;
    effect->field_0x03 = 19;
    effect->timer_0x0C = 0;
    effect->field_0x10 = 0;
    effect->type_0x02 = type;
    effect->pos_0x18 = *pos;
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

    mhchar_joint_mtx_get(&((_EFT019_PHYSICS*)((_PLW*)self->source_0x30)->physics_0x13C)->chr_0x04, 7, mtx);
    mtx34_trans_get(mtx, &pos);
    axis.x = -mtx->m[0][2];
    axis.y = -mtx->m[1][2];
    axis.z = -mtx->m[2][2];
    calcVecAngXY(&axis, &rot.x, &rot.y);
    rot.z = 0;
    self->rot_0x24.z = 0;
    setVector3(&spin, -10.0f, -10.0f, 87.0f);
    rotVecX(&spin, rot.x);
    rotVecY(&spin, rot.y);
    pos += spin;
    cpSetRotMatrix(&rot, mtx);
    mtx34_set_trans(mtx, &pos);
}

#pragma peephole reset
