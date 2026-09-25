/* effect.cpp - the game's effect manager, `.text` 0x800F95A4..0x800FAE08 (41 functions).
 *
 * rule 7 deferred: the symbol map spells 34 of this range's 41 functions as bare `fn_XXXXXXXX`
 * (checked with symedit: each is a bare .text entry in config/RMHE08/symbols.txt), so those
 * definitions have to keep the map's own spelling; the other seven arrive mangled and are written
 * through their owners.
 *
 * Final home, evidence class 1 (a `__FILE__` string).  fn_800F9884's `nw4r::db::Panic` assert passes
 * the bare string `effect.cpp` (0x8059B5D0, 0x0B bytes) as its file argument - the retired gap object
 * `build/RMHE08/obj/auto_fn_800F9884_text.o` relocates `lbl_8059B5D0` into `Panic`'s r3 (line 2854)
 * and `lbl_8059B5E0` (`NW4R:Failed assertion 0`) into its r5.  The `.cpp` suffix decides the language
 * and `tools/units/langcheck.py` agrees (seven definitions arrive mangled:
 * `SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3`, `effect_move__FPQ34nw4r2ef6Effect`,
 * `change_color_eff__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC38_GXColor`,
 * `change_paramscale_eff__FPQ34nw4r2ef6Effectf`,
 * `change_paramscale_eff_vec3__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3`,
 * `eftGetKeyRGB__FPUclPUcPUcPUc`, `eftGetKeyAlpha__FPUcl`).  The module is `ef`: every sibling in the
 * band is `ef/*` and the range's callees are `nw4r::ef::Effect` members.  The seam is unproven (the
 * proposal boundary, 0x800F95A4 / 0x800FAE08, cuts at function boundaries).
 *
 * Evidence: the retired per-function targets `build/RMHE08/obj/auto_fn_800F*_text.o` (MAIN),
 * `python tools/flags/infer.py build/RMHE08/obj/auto_fn_800F9E04_text.o` (peephole off) and the
 * DOL's extabindex table (extab 0x8000BC5C..0x8000BD6C, extabindex 0x80025974..0x80025B0C).
 *
 * The `nw4r::db::Panic` file/message pool, the `lbl_807965xx` float constants, the two name tables
 * (`lbl_8058A880`, `lbl_8058A924`), the `.bss` `eft_control`/`lbl_806A1400`/`lbl_806A2080` runs are
 * declared, never defined here - the data pass claims them once a definition would make the object
 * emit them (docs/plan.md 8.4).
 */

#include "types.h"

#include "nw4r/math.h"
#include "nw4r/g3d/scnmdl.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "g3d/fn_80063888.h" /* fn_80064820, owned by g3d/fn_80063888.cpp (rule 2) */

/* The retail object keeps the unfused forms (a `rlwinm` + `cmpwi` where the pass would emit a
 * record-form `rlwinm.`); the whole file is compiled with the peephole pass off (playbook 39). */
#pragma peephole off

/* The key-sampling arithmetic keeps the unfused `fmuls` + `fadds` chain (the default contracts it
 * into `fmadds`, playbook 40). */
#pragma fp_contract off

/* ---------------------------------------------------------------------------------------------------
 * the range's records
 * ------------------------------------------------------------------------------------------------- */

/* The effect manager's control block (`eft_control`, 0xC44 B in `.bss`).  Only the "initialised" byte
 * and the active effect handle are read here.  `ef/eft004.cpp` carries a second view of the same record
 * (its `EftControl`); the two move to one header in wave 2, so this one keeps a distinct local name. */
typedef struct EftManager {
    /* +0x000 */ u8 initialised_0x00;
    /* +0x001 */ u8 pad_0x01[0x3];
    /* +0x004 */ void* effect_0x04;
    /* +0x008 */ u8 pad_0x08[0xC44 - 0x008];
} EftManager; /* size: 0xC44 */

/* One pooled effect instance (`fn_800A51D8` returns it).  It carries a vtable at +0x1C (the emitter
 * retire call in `fn_800F996C` is vtable slot 6) and its root position at +0x9C. */
typedef struct EftHandle {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ void** vtable_0x1C;
    /* +0x20 */ u8 pad_0x20[0x7C];
    /* +0x9C */ nw4r::math::VEC3 pos_0x9C;
} EftHandle; /* size: 0xA8 - lower bound, an approximation */

/* The effect-record the emitter loop walks (`fn_802B0420` returns the base of four 0x34-byte slots).
 * `mode_0x00` is the shape id `fn_800FA208` switches on; +0x04 / +0x14 are the per-mode parameter
 * blocks the two copy helpers move. */
typedef struct EftEmitterRecord {
    /* +0x00 */ s8 mode_0x00;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ u8 sphere_0x04[0x10];
    /* +0x14 */ u8 box_0x14[0x20];
} EftEmitterRecord; /* size: 0x34 */

/* The 0x10-byte sphere record `fn_800FA354` copies and `hit_point_sphr` tests. */
typedef struct EftSphere {
    /* +0x00 */ nw4r::math::VEC3 center_0x00;
    /* +0x0C */ f32 radius_0x0C;
} EftSphere; /* size: 0x10 */

/* The 0x1C-byte box record `fn_800FA318` copies. */
typedef struct EftBox {
    /* +0x00 */ u32 v_0x00[6];
    /* +0x18 */ f32 radius_0x18;
} EftBox; /* size: 0x1C */

/* The 0x34-byte record `fn_800FA378` constructs: a box plus two more vectors. */
typedef struct EftShape {
    /* +0x00 */ EftBox box_0x00;
    /* +0x1C */ nw4r::math::VEC3 a_0x1C;
    /* +0x28 */ nw4r::math::VEC3 b_0x28;
} EftShape; /* size: 0x34 */

/* The two-vector head `fn_800FA3E8` zeroes (the first 0x18 bytes of a box/shape record). */
typedef struct EftVectors {
    /* +0x00 */ nw4r::math::VEC3 a_0x00;
    /* +0x0C */ nw4r::math::VEC3 b_0x0C;
} EftVectors; /* size: 0x18 */

/* One 0x20-byte element of the static name array `fn_800FAC1C` constructs. */
typedef struct EftNameArrayElem {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ EftVectors vectors_0x04;
    /* +0x1C */ u8 pad_0x1C[0x04];
} EftNameArrayElem; /* size: 0x20 */

/* The name-table node `fn_800F97F0` reads its resource word from. */
typedef struct EftNameEntry {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ void* field_0x10;
} EftNameEntry; /* size: 0x14 */

/* The per-manager payload `fn_800F99D4` builds and `fn_800F9A70` reads back. */
typedef struct EftParticleArgs {
    /* +0x00 */ u8 mode_0x00;
    /* +0x01 */ u8 color_0x01[4];
    /* +0x05 */ u8 color2_0x05[4];
    /* +0x09 */ u8 pad_0x09[0x03];
    /* +0x0C */ f32 scale_0x0C;
    /* +0x10 */ nw4r::math::VEC3 pos_0x10;
} EftParticleArgs; /* size: 0x1C */

/* The effect state `fn_800F9D80`/`fn_800F9DF4` read and write. */
typedef struct EftFrameState {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 flags_0x04;
    /* +0x05 */ u8 mode_0x05;
    /* +0x06 */ u8 pad_0x06[0x16];
    /* +0x1C */ f32 value_0x1C;
} EftFrameState; /* size: 0x20 */

/* One pool slot `fn_800FBD68` returns. */
typedef struct EftPoolSlot {
    /* +0x00 */ u8 pad_0x00[0x03];
    /* +0x03 */ u8 state_0x03;
    /* +0x04 */ u8 pad_0x04[0x2C];
    /* +0x30 */ void* owner_0x30;
    /* +0x34 */ void* handler_0x34;
} EftPoolSlot; /* size: 0x38 */

/* The model record `fn_800FAD90` releases. */
typedef struct EftJointModel {
    /* +0x00 */ void* field_0x00;
    /* +0x04 */ void* field_0x04;
} EftJointModel; /* size: 0x08 */

/* The owner whose model pointer sits at +0x38. */
typedef struct EftModelOwner {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ EftJointModel* model_0x38;
} EftModelOwner; /* size: 0x3C */

/* The colour-parameter block `fn_800F9E04` reads at +0x30 (four `u8` multipliers). */
typedef struct EftColorParams {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8 color_0x30[4];
} EftColorParams; /* size: 0x34 - lower bound, an approximation */

/* The spawn owner `fn_800F9E04` reads its flags from (+0x14..+0x17). */
typedef struct EftSpawnOwner {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ u8 flags_0x14;
    /* +0x15 */ u8 field_0x15;
    /* +0x16 */ u8 field_0x16;
    /* +0x17 */ u8 field_0x17;
} EftSpawnOwner; /* size: 0x18 - lower bound, an approximation */

/* The name record `fn_800FA9B8` fills: a count at +0x00, a second count at +0x01, a name byte at
 * +0x02, six name bytes at +0x03 and eight at +0x0C. */
typedef struct EftNameSet {
    /* +0x00 */ u8 count_0x00;
    /* +0x01 */ u8 count_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 names_0x03[6];
    /* +0x09 */ u8 pad_0x09[0x03];
    /* +0x0C */ u8 names2_0x0C[8];
} EftNameSet; /* size: 0x14 */

/* One 8-byte node of the two name tables `fn_800FA9B8` walks. */
typedef struct EftNameNode {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
} EftNameNode; /* size: 0x08 */

/* One 8-byte scenario-box descriptor (`fn_800F9C20` walks the table). */
typedef struct EftScnboxEntry {
    /* +0x00 */ u32 size_0x00;
    /* +0x04 */ char* name_0x04;
} EftScnboxEntry; /* size: 0x08 */

/* ---------------------------------------------------------------------------------------------------
 * the externs
 * ------------------------------------------------------------------------------------------------- */

/* the two camera helpers are global-scope C++ functions returning by value (`get_camera_pos__Fv` /
 * `get_current_view_mtx__Fv`): the map spells no argument list, so the struct return is the real
 * shape and the hidden return pointer is what the call site passes in r3 (rule 9, the owner's name). */
nw4r::math::VEC3 get_camera_pos();
nw4r::math::MTX34 get_current_view_mtx();
u8 get_now_areano();
u32 get_stg_eft_col(u8 area, u8 index);
u32 hit_point_sphr(nw4r::math::VEC3* a, nw4r::math::VEC3* b, f32 r);

/* the unit's own owned symbols other units call (declared in the owners' headers in wave 2) */
extern "C" void fn_800F9DF4(EftFrameState* self, u8 a, u8 b);
extern "C" EftVectors* fn_800FA3E8(EftVectors* self);
extern "C" nw4r::math::VEC3* fn_800FA420(nw4r::math::VEC3* self);

/* the unit's own helpers, used before their definitions below */
extern "C" u32 fn_800FA208(nw4r::math::VEC3* pos, EftEmitterRecord* rec);
extern "C" void fn_800FA318(EftBox* dst, const EftBox* src);
extern "C" void fn_800FA354(EftSphere* dst, const EftSphere* src);
extern "C" EftShape* fn_800FA378(EftShape* self);
extern "C" EftVectors* fn_800FA3B8(EftVectors* self);
extern "C" void fn_800F9E04(EftSpawnOwner* owner, void* target, u8 mode, EftColorParams* params, u8 flag);

extern "C" {

/* the engine vector/matrix helpers */
void fn_8005050C(void* mtx);
void fn_80043EA8(nw4r::math::VEC3* out);
void fn_80041E40(nw4r::math::VEC3* dst, const nw4r::math::VEC3* src);
void fn_8004C4F0(void* dst, void* src);
void fn_800532DC(nw4r::math::MTX34* dst, const nw4r::math::MTX34* src);
void fn_802BDE90(f32* out_a, f32* out_b);
void fn_800834F0(void* out);
void fn_800A8998(void* out, u32 mode);
void* fn_800A8944(void* self);
void fn_800A898C(void* dst, const void* src);
void fn_800AEE0C(void* dst, void* src);
void fn_800B38C0(void* out, void* a, void* b, void* c);
void fn_800A602C(void* self, const nw4r::math::VEC3* pos, const nw4r::math::MTX34* mtx, f32 a, f32 b);
void fn_800AC100(void* mgr, u8 mode, u8* a, u8* b, u8* c, f32 scale);

/* the effect pool / emitter walk */
void fn_800A5F4C(void* effect, u32 idx);
void fn_800A5E6C(void* effect, u32 idx);
void fn_800A5D8C(void* effect, u32 idx);
void* fn_800A4420(void* effect);
void* fn_800A60C0(void* effect);
u16 fn_800A51D0(void* effect);
void* fn_800A51D8(void* effect, u16 idx);
u16 fn_800A970C(void* handle);
void* fn_800A9714(void* handle, u32 idx);
void fn_800A95D8(void* handle);

/* the resource / model helpers */
s32 fn_800B2878(void);
void fn_800B483C(void);
void fn_800B4A14(s32 handle);
s32 fn_800E28E4(void* chr);
s32 fn_800E2994(void* access);
void fn_800E2AF4(void* chr, u32 idx, u32 id, _GXColor* out);
void fn_800E2CD4(void* chr, u32 idx, u32 id, _GXColor* out);
void fn_800E2D60(void* chr, u32 idx, u32 id, _GXColor* out);
void fn_800E28EC(void* chr, u32 idx, u32 id, _GXColor* color, u32 keep);
void fn_800F8A44(void* a, void* b);
void* fn_8007BE2C(void* access, u32 idx);
void* fn_8007BC2C(void* access, u32 idx);
void fn_8006F0E8(void* out, void* handle);
void fn_8005A8E0(void* out, void* handle);
s32 fn_80076750(void* out);

/* the emitter data helpers */
s32 fn_800F6984(u32 a, u32 b, void* table, void* names);
s32 fn_800F6B6C(u32 a, u32 b, u8 idx, u32 c, u8 d);
void* fn_800FBD68(s32 a);
void* fn_802B0420(void);
u32 fn_802AFFF4(void);
u32 fn_802AFF38(void);
void fn_8028F558(void* a, void* b);
u32 fn_802907BC(void* a, void* b);
f32 fn_80050EDC(void* v);

/* the file/alloc helpers */
void __construct_array(void* base, void* ctor, u32 a, u32 elemsize, u32 count);

/* the compiler-generated array constructors the unit hands to `__construct_array` */
EftNameArrayElem* fn_800FAC78(EftNameArrayElem* self);

/* the next proposal's dispatcher entries `fn_800FADCC` tail-calls */
void fn_800FAE08(void* self);
void fn_800FB160(void* self);
void fn_800FBBAC(void* self);
void fn_800FBBBC(void* self);

/* the shared SDK entry points */
void* memset(void* dst, int v, u32 n);
void* memcpy(void* dst, const void* src, u32 n);

/* the unit's own symbols */
void fn_800F9A70(void* mgr, EftParticleArgs* data);
EftParticleArgs* fn_800F9A8C(EftParticleArgs* self);
void fn_800F9B68(EftHandle* handle, nw4r::math::VEC3* v);
void fn_800F996C(EftHandle* handle, u32 flag);
void fn_800F99D4(void* effect, u8 mode, _GXColor* color, _GXColor* color2, nw4r::math::VEC3* pos, u8 flag, f32 scale);
void fn_800FADCC(EftFrameState* self);

}  // extern "C"

/* C++ callees: the target object references their manglings (`ran_suu__Fl`,
 * `work_mem_alloc__FUl`, `work_mem_free__FPv`, `load_file__FPcUll`,
 * `Panic__Q24nw4r2dbFPCciPCce`), so they are declared at C++ scope, outside the extern "C"
 * block above (relocaudit). */
s32 ran_suu(s32 max);
void* work_mem_alloc(u32 size);
void work_mem_free(void* p);
void load_file(char* name, u32 dst, s32 size);
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* msg, ...); } }

/* the effect manager's control block and the two name tables live in the shared data run */
extern "C" EftManager eft_control;
extern "C" void* lbl_8058A880[];
extern "C" void* lbl_8058A924[];
extern "C" void** eft_scnbox_data_ptr;
extern "C" u8 lbl_8059B5D0[];
extern "C" u8 lbl_8059B5E0[];
extern "C" u8 lbl_806A1400[];
extern "C" u8 lbl_806A2080[];
extern f32 lbl_807965D8;
extern f32 lbl_807965DC;
extern f32 lbl_807965E0;
extern f32 lbl_807965F0;
extern f32 lbl_807965F4;
extern f32 lbl_807965F8;

/* ---------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x800F95A4 - build the camera-space root transform for one effect and hand it to the draw path. */
extern "C" void fn_800F95A4(void* self) {
    nw4r::math::MTX34 world;
    nw4r::math::VEC3 pos;
    f32 aspect;
    f32 fov;

    fn_8005050C(&world);
    fn_80043EA8(&pos);
    fn_80041E40(&pos, &get_camera_pos());
    fn_800532DC(&world, &get_current_view_mtx());
    fn_802BDE90(&fov, &aspect);
    fn_800A602C(self, &pos, &world, fov, aspect);
}

/* 0x800F9628 - retire the two live effect emitters and release the model. */
extern "C" void fn_800F9628(void) {
    void* effect = eft_control.effect_0x04;
    if (eft_control.initialised_0x00 == 0 || effect == 0) {
        return;
    }
    for (u32 i = 0; i < 2; i++) {
        fn_800A5F4C(effect, i);
        fn_800A5E6C(effect, i);
        fn_800A5D8C(effect, i);
    }
    void** vt = (void**)fn_800A4420(effect);
    ((void (*)(void*))vt[3])(fn_800A4420(effect));
    s32 handle = fn_800B2878();
    fn_800B483C();
    fn_800B4A14(handle);
}

/* 0x800F96D4 - the 4-byte copy the transform setters share. */
extern "C" void fn_800F96D4(u32* dst, const u32* src) {
    *dst = *src;
}

/* 0x800F96E0 - set the effect's root matrix translation from a world position. */
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos) {
    nw4r::math::MTX34 mtx;
    fn_8005050C(&mtx);
    if (effect != NULL) {
        const nw4r::math::MTX34* src = (const nw4r::math::MTX34*)fn_800A60C0(effect);
        fn_800532DC(&mtx, src);
        mtx.m[0][3] = pos->x;
        mtx.m[1][3] = pos->y;
        mtx.m[2][3] = pos->z;
        effect->SetRootMtx(mtx);
    }
}

/* 0x800F975C - add a world position to the effect's root matrix translation. */
extern "C" void fn_800F975C(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos) {
    nw4r::math::MTX34 mtx;
    fn_8005050C(&mtx);
    if (effect != NULL) {
        const nw4r::math::MTX34* src = (const nw4r::math::MTX34*)fn_800A60C0(effect);
        fn_800532DC(&mtx, src);
        mtx.m[0][3] += pos->x;
        mtx.m[1][3] += pos->y;
        mtx.m[2][3] += pos->z;
        effect->SetRootMtx(mtx);
    }
}

/* 0x800F97F0 - place one effect handle from the name table. */
extern "C" s32 fn_800F97F0(s32 idx, EftNameEntry* node, void* out) {
    if (node == NULL) {
        return 0;
    }
    if (node->field_0x10 == NULL) {
        return 0;
    }
    s32 frame;
    fn_800B38C0(&frame, (void*)fn_800B2878(), lbl_8058A880[idx], node->field_0x10);
    fn_800A898C(out, &frame);
    return 1;
}

/* 0x800F9884 - whether every live emitter of the effect has a frame to move to. */
extern "C" s32 fn_800F9884(void* effect) {
    s32 total = 0;
    u16 count = fn_800A51D0(effect);
    for (s32 i = 0; i < (s32)count; i++) {
        void* handle = fn_800A51D8(effect, (u16)i);
        if (handle == NULL) {
            nw4r::db::Panic((const char*)lbl_8059B5D0, 2854, (const char*)lbl_8059B5E0);
        } else {
            total += fn_800A970C(handle);
        }
    }
    return total > 0;
}

/* 0x800F9920 - move the effect and report whether it is still alive. */
u32 effect_move(nw4r::ef::Effect* effect) {
    if (fn_800F9884(effect) == 0) {
        return 0;
    }
    fn_800F996C((EftHandle*)(void*)effect, 0);
    return 1;
}

/* 0x800F996C - call vtable slot 6 of the effect with `flag != 0`. */
extern "C" void fn_800F996C(EftHandle* handle, u32 flag) {
    u32 b = (flag != 0);
    ((void (*)(void*, u32))handle->vtable_0x1C[6])(handle, b);
}

/* 0x800F9988 - colour the whole effect with a white default alpha. */
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color) {
    _GXColor c;
    c.r = 0;
    c.g = 0;
    c.b = 0;
    c.a = 0xFF;
    fn_800F99D4(effect, 1, &color, &c, pos, 1, lbl_807965D8);
}

/* 0x800F99D4 - hand a colour/scale request to every particle manager of the effect. */
extern "C" void fn_800F99D4(void* effect, u8 mode, _GXColor* color, _GXColor* color2, nw4r::math::VEC3* pos, u8 flag, f32 scale) {
    EftParticleArgs args;
    fn_800F9A8C(&args);
    args.mode_0x00 = mode;
    fn_8004C4F0(args.color_0x01, color);
    fn_8004C4F0(args.color2_0x05, color2);
    args.scale_0x0C = scale;
    fn_80041E40(&args.pos_0x10, pos);
    ((nw4r::ef::Effect*)effect)->ForeachParticleManager((void (*)(void*, u32))fn_800F9A70, (u32)&args, flag != 0);
}

/* 0x800F9A70 - the per-manager callback `fn_800F99D4` installs. */
extern "C" void fn_800F9A70(void* mgr, EftParticleArgs* data) {
    fn_800AC100(mgr, data->mode_0x00, data->color_0x01, data->color2_0x05, (u8*)&data->pos_0x10,
                data->scale_0x0C);
}

/* 0x800F9A8C - zero a vector at +0x10 and return the record. */
extern "C" EftParticleArgs* fn_800F9A8C(EftParticleArgs* self) {
    fn_80043EA8(&self->pos_0x10);
    return self;
}

/* 0x800F9AC0 - scale the effect's first emitter uniformly. */
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale) {
    nw4r::math::VEC3 v;
    fn_80043EA8(&v);
    fn_800834F0(&v);
    if (scale >= lbl_807965DC) {
        if (fn_800A51D0(effect) != 0) {
            void* handle = fn_800A51D8(effect, 0);
            if (handle != NULL) {
                setVector3(&v, scale, scale, scale);
                fn_800F9B68((EftHandle*)handle, &v);
            }
        }
    }
}

/* 0x800F9B68 - install a scale vector on one handle. */
extern "C" void fn_800F9B68(EftHandle* handle, nw4r::math::VEC3* v) {
    fn_80041E40(&handle->pos_0x9C, v);
    fn_800A95D8(handle);
}

/* 0x800F9BA0 - scale the effect's first emitter by a vector. */
void change_paramscale_eff_vec3(nw4r::ef::Effect* effect, nw4r::math::VEC3* v) {
    nw4r::math::VEC3 tmp;
    fn_80043EA8(&tmp);
    if (fn_80050EDC(v) >= lbl_807965DC) {
        if (fn_800A51D0(effect) != 0) {
            void* handle = fn_800A51D8(effect, 0);
            if (handle != NULL) {
                fn_800F9B68((EftHandle*)handle, v);
            }
        }
    }
}

/* 0x800F9C20 - load a 0xD0-byte scenario-box record for the area. */
extern "C" void fn_800F9C20(void* dst, u8 area, u8 slot) {
    EftScnboxEntry* entry = (EftScnboxEntry*)eft_scnbox_data_ptr[area];
    if (entry == NULL) {
        memset(dst, 0, 0xD0);
        return;
    }
    u32 size = entry[slot].size_0x00;
    if (size == 0) {
        memset(dst, 0, 0xD0);
        return;
    }
    u32 rounded = (size + 0x1F) & ~0x1Fu;
    void* buf = work_mem_alloc(rounded);
    load_file(entry[slot].name_0x04, (u32)buf, (s32)rounded);
    memcpy(dst, buf, 0xD0);
    work_mem_free(buf);
}

/* 0x800F9CDC - a random 0.0..1.0 scale factor. */
extern "C" f32 fn_800F9CDC(void) {
    u16 r = (u16)ran_suu(1);
    u32 v = (u32)(r & 0x3FF);
    return (f32)v * lbl_807965E0;
}

/* 0x800F9D2C - a random -0.5..0.5 scale factor. */
extern "C" f32 fn_800F9D2C(void) {
    u16 r = (u16)ran_suu(1);
    s32 v = (s32)(r & 0x3FF) - 0x200;
    return (f32)v * lbl_807965F0;
}

/* 0x800F9D80 - whether the effect is still legal for its owner. */
extern "C" s32 fn_800F9D80(EftFrameState* self) {
    s32 result = 0;
    if (self->value_0x1C < lbl_807965F4) {
        u8 flags = self->flags_0x04;
        if ((flags & 0x4) != 0) {
            if (fn_802AFFF4() == 1) {
                result = 1;
            }
        } else if ((flags & 0x2) != 0) {
            if (fn_802AFF38() == 1) {
                result = 1;
            }
        }
    }
    return result;
}

/* 0x800F9DF4 - merge two flag bytes into the effect's state byte. */
extern "C" void fn_800F9DF4(EftFrameState* self, u8 a, u8 b) {
    self->flags_0x04 = (u8)(a | b);
}

/* 0x800F9E04 - colour/parameter update for one spawned effect. */
extern "C" void fn_800F9E04(EftSpawnOwner* owner, void* target, u8 mode, EftColorParams* params, u8 flag) {
    nw4r::math::VEC3 pos;
    u8 rgb[4];
    u8 out[4];
    u32 v;
    u32 color = get_stg_eft_col(get_now_areano(), flag);
    rgb[0] = (u8)(color >> 24);
    rgb[1] = (u8)(color >> 16);
    rgb[2] = (u8)(color >> 8);
    rgb[3] = (u8)color;

    switch (mode) {
    case 0: {
        nw4r::ef::Effect* effect = (nw4r::ef::Effect*)target;
        const nw4r::math::MTX34* root = (const nw4r::math::MTX34*)fn_800A60C0(effect);
        pos.x = root->m[0][3];
        pos.y = root->m[1][3];
        pos.z = root->m[2][3];
        v = (u32)((f32)rgb[0] * (lbl_807965F8 * (f32)params->color_0x30[0]));
        if (v > 0xFF) v = 0xFF;
        out[0] = (u8)v;
        v = (u32)((f32)rgb[1] * (lbl_807965F8 * (f32)params->color_0x30[1]));
        if (v > 0xFF) v = 0xFF;
        out[1] = (u8)v;
        v = (u32)((f32)rgb[2] * (lbl_807965F8 * (f32)params->color_0x30[2]));
        if (v > 0xFF) v = 0xFF;
        out[2] = (u8)v;
        v = (u32)((f32)rgb[3] * (lbl_807965F8 * (f32)params->color_0x30[3]));
        if (v > 0xFF) v = 0xFF;
        out[3] = (u8)v;
        change_color_eff(effect, &pos, *(_GXColor*)out);
        break;
    }
    case 1: {
        MHchar* chr = (MHchar*)target;
        fn_800E28E4(chr);
        v = (u32)((f32)owner->field_0x15 * ((f32)rgb[0] * (lbl_807965F8 * (f32)params->color_0x30[0])) * lbl_807965F8);
        if (v > 0xFF) v = 0xFF;
        out[0] = (u8)v;
        v = (u32)((f32)owner->field_0x15 * ((f32)rgb[1] * (lbl_807965F8 * (f32)params->color_0x30[1])) * lbl_807965F8);
        if (v > 0xFF) v = 0xFF;
        out[1] = (u8)v;
        v = (u32)((f32)owner->field_0x15 * ((f32)rgb[2] * (lbl_807965F8 * (f32)params->color_0x30[2])) * lbl_807965F8);
        if (v > 0xFF) v = 0xFF;
        out[2] = (u8)v;
        out[3] = 0xFF;
        if ((owner->flags_0x14 & 1) != 0) {
            for (u8 i = owner->field_0x16; i < owner->field_0x17; i++) {
                chr->getTevKColor(i, GX_KCOLOR0, (_GXColor*)out);
                out[3] = ((_GXColor*)out)->a;
                chr->setTevKColor(i, GX_KCOLOR0, (_GXColor*)out);
            }
        } else if ((owner->flags_0x14 & 2) != 0) {
            for (u8 i = owner->field_0x16; i < owner->field_0x17; i++) {
                chr->getMatColor(i, GX_COLOR0A0, (_GXColor*)out);
                out[3] = ((_GXColor*)out)->a;
                chr->setMatColor(i, GX_COLOR0A0, *(_GXColor*)out, false);
            }
        }
        break;
    }
    default:
        break;
    }
}

/* 0x800FA208 - test one effect emitter record against a world position. */
extern "C" u32 fn_800FA208(nw4r::math::VEC3* pos, EftEmitterRecord* rec) {
    EftSphere sphere;
    EftBox box;
    EftShape box2;
    u32 result = 0;
    fn_800FA420((nw4r::math::VEC3*)&sphere);
    fn_800FA3B8((EftVectors*)&box);
    fn_800FA378(&box2);
    switch (rec->mode_0x00) {
    case 0:
        break;
    case 1:
        fn_800FA354(&sphere, (const EftSphere*)rec->sphere_0x04);
        if (hit_point_sphr(pos, &sphere.center_0x00, sphere.radius_0x0C) == 1) {
            result = 1;
        }
        break;
    case 2:
        fn_800FA318(&box, (const EftBox*)rec->box_0x14);
        fn_8028F558(&box, &box2);
        if (fn_802907BC(pos, &box2) == 1) {
            result = 1;
        }
        break;
    case 3:
        if (pos->y < lbl_807965F4) {
            result = 1;
        }
        break;
    }
    return result;
}

/* 0x800FA318 - copy a 0x1C-byte box record. */
extern "C" void fn_800FA318(EftBox* dst, const EftBox* src) {
    *dst = *src;
}

/* 0x800FA354 - copy a 0x10-byte sphere record. */
extern "C" void fn_800FA354(EftSphere* dst, const EftSphere* src) {
    *dst = *src;
}

/* 0x800FA378 - construct the box record and its two vectors. */
extern "C" EftShape* fn_800FA378(EftShape* self) {
    fn_800FA3B8((EftVectors*)&self->box_0x00);
    fn_80043EA8(&self->a_0x1C);
    fn_80043EA8(&self->b_0x28);
    return self;
}

/* 0x800FA3B8 - construct the sphere record. */
extern "C" EftVectors* fn_800FA3B8(EftVectors* self) {
    fn_800FA3E8(self);
    return self;
}

/* 0x800FA3E8 - zero the two vectors of a record. */
extern "C" EftVectors* fn_800FA3E8(EftVectors* self) {
    fn_80043EA8(&self->a_0x00);
    fn_80043EA8(&self->b_0x0C);
    return self;
}

/* 0x800FA420 - zero one vector. */
extern "C" nw4r::math::VEC3* fn_800FA420(nw4r::math::VEC3* self) {
    fn_80043EA8(self);
    return self;
}

/* 0x800FA450 - run the area effect set against the world. */
extern "C" void fn_800FA450(void* effect) {
    nw4r::math::VEC3 pos;
    EftSphere sphere;
    EftBox box;
    EftShape box2;
    u8 mode;
    void* handle = NULL;
    s32 alive = 0;
    fn_800FA420((nw4r::math::VEC3*)&sphere);
    fn_800FA3B8((EftVectors*)&box);
    fn_80043EA8(&pos);
    fn_800FA378(&box2);
    fn_800A8998(&mode, 0);
    u16 count = fn_800A51D0(effect);
    if (count == 0) {
        return;
    }
    u16 i;
    for (i = 0; i < count; i++) {
        handle = fn_800A51D8(effect, i);
        if (handle == NULL) {
            continue;
        }
        if (fn_800A970C(handle) != 0) {
            alive = 1;
            break;
        }
        alive = 0;
    }
    if (alive == 0) {
        return;
    }
    u8 flags;
    fn_800AEE0C(&flags, fn_800A9714(handle, 0));
    fn_800A898C(&mode, &flags);
    u8 bits = *(u8*)((u8*)fn_800A8944(&mode) + 3);
    u8 which;
    if ((bits & 0x80) != 0) {
        which = 0;
    } else if ((bits & 0x40) != 0) {
        which = 1;
    } else {
        return;
    }
    const nw4r::math::MTX34* root = (const nw4r::math::MTX34*)fn_800A60C0(effect);
    pos.x = root->m[0][3];
    pos.y = root->m[1][3];
    pos.z = root->m[2][3];
    EftEmitterRecord* rec = (EftEmitterRecord*)fn_802B0420();
    for (s32 j = 0; j < 4; j++) {
        if (fn_800FA208(&pos, rec) == 1) {
            fn_800F9E04(NULL, effect, 0, (EftColorParams*)rec, which);
        }
        rec++;
    }
}

/* 0x800FA5D4 - run the weapon effect set against a character's model. */
extern "C" void fn_800FA5D4(EftSpawnOwner* owner, MHchar* chr) {
    nw4r::math::VEC3 pos;
    EftSphere sphere;
    EftSphere sphere2;
    EftBox box;
    EftShape shape;
    fn_800FA420((nw4r::math::VEC3*)&sphere);
    fn_800FA420((nw4r::math::VEC3*)&sphere2);
    fn_800FA3B8((EftVectors*)&box);
    fn_80043EA8(&pos);
    fn_800FA378(&shape);
    s32 count = fn_800E28E4(chr);
    u8 flags = owner->flags_0x14;
    if ((flags & 1) != 0) {
        for (s32 i = 0; i < count; i++) {
            nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)(u32)chr->field_0x118, (u32)i);
            if (fn_800E2994(&access) == 0) {
                return;
            }
            u32 handle = (u32)fn_8007BE2C(&access, 0);
            void* tex;
            fn_8006F0E8(&tex, &handle);
            if (fn_80076750(&tex) == 0) {
                return;
            }
        }
    } else if ((flags & 2) != 0) {
        for (s32 i = 0; i < count; i++) {
            nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)(u32)chr->field_0x118, (u32)i);
            if (fn_800E2994(&access) == 0) {
                return;
            }
            u32 handle = (u32)fn_8007BC2C(&access, 0);
            void* tex;
            fn_8005A8E0(&tex, &handle);
            if (fn_80064820(&tex) == 0) {
                return;
            }
        }
    } else {
        return;
    }
    s32 flag = flags & 0x80;
    EftEmitterRecord* rec = (EftEmitterRecord*)fn_802B0420();
    chr->get_joint_wpos(0, &pos);
    for (s32 j = 0; j < 4; j++) {
        if (fn_800FA208(&pos, rec) == 1) {
            fn_800F9E04(owner, chr, 1, (EftColorParams*)rec, flag != 0);
        }
        rec++;
    }
}

/* 0x800FA7A0 - sample an RGB key table at a frame. */
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b) {
    u8* p = keys;
    for (;;) {
        u8 start = p[0];
        if (start == 0xFF) {
            *r = p[1];
            *g = p[2];
            *b = p[3];
            return;
        }
        if (frame >= (s32)start) {
            u8 end = p[4];
            if (frame <= (s32)end) {
                f32 t = (f32)(frame - (s32)start) / (f32)((s32)end - (s32)start);
                *r = (u8)(s32)((f32)p[1] + t * (f32)((s32)p[5] - (s32)p[1]));
                *g = (u8)(s32)((f32)p[2] + t * (f32)((s32)p[6] - (s32)p[2]));
                *b = (u8)(s32)((f32)p[3] + t * (f32)((s32)p[7] - (s32)p[3]));
                return;
            }
        }
        p += 4;
    }
}

/* 0x800FA8F8 - sample an alpha key table at a frame. */
u8 eftGetKeyAlpha(u8* keys, long frame) {
    u8* p = keys;
    for (;;) {
        u8 start = p[0];
        if (start == 0xFF) {
            return p[1];
        }
        if (frame >= (s32)start) {
            u8 end = p[2];
            if (frame <= (s32)end) {
                f32 t = (f32)(frame - (s32)start) / (f32)((s32)end - (s32)start);
                return (u8)(s32)((f32)p[1] + t * (f32)((s32)p[3] - (s32)p[1]));
            }
        }
        p += 2;
    }
}

/* 0x800FA9B8 - build the two effect-name sets for an area. */
extern "C" void fn_800FA9B8(EftNameSet* out, u8 area) {
    u8 names[11];
    out->count_0x00 = 0;
    EftNameNode* node = (EftNameNode*)lbl_8058A880[area];
    if (node != NULL) {
        s32 n = 0;
        u8 m = 0;
        for (s32 i = 0; i < 11; i++) {
            names[i] = 0xFF;
        }
        while (node->field_0x00 != 0) {
            if (node->field_0x04 != 0) {
                s32 v = fn_800F6B6C(2, 1, area, 0, out->count_0x00);
                names[n++] = (u8)v;
                out->names_0x03[out->count_0x00] = (u8)v;
                out->count_0x00++;
            } else {
                node++;
                if (node->field_0x00 == 0) {
                    break;
                }
                if (node->field_0x04 != 0) {
                    s32 v = fn_800F6B6C(2, 2, area, 0, m++);
                    names[n++] = (u8)v;
                    out->field_0x02 = (u8)v;
                }
            }
            node++;
        }
        fn_800F6984(area, 0, lbl_8058A880[area], names);
    }
    out->count_0x01 = 0;
    node = (EftNameNode*)lbl_8058A924[area];
    if (node != NULL) {
        for (s32 i = 0; i < 11; i++) {
            names[i] = 0xFF;
        }
        while (node->field_0x00 != 0) {
            if (node->field_0x04 != 0) {
                s32 v = fn_800F6B6C(2, 3, area, 0, out->count_0x01);
                names[out->count_0x01] = (u8)v;
                out->names2_0x0C[out->count_0x01] = (u8)v;
                out->count_0x01++;
            }
            node++;
        }
        fn_800F6984(area, 3, lbl_8058A924[area], names);
    }
}

/* 0x800FAC1C - construct the two static effect-name arrays. */
extern "C" void fn_800FAC1C(void) {
    __construct_array(lbl_806A1400, (void*)fn_800FAC78, 0, 0x20, 0x64);
    __construct_array(lbl_806A2080, (void*)fn_800FA3B8, 0, 0x1C, 4);
}

/* 0x800FAC78 - construct one 0x20-byte name-array element. */
extern "C" EftNameArrayElem* fn_800FAC78(EftNameArrayElem* self) {
    fn_800FA3B8(&self->vectors_0x04);
    return self;
}

/* 0x800FACAC - register the area owner on pool slot 0. */
extern "C" void fn_800FACAC(void* owner) {
    EftPoolSlot* slot = (EftPoolSlot*)fn_800FBD68(0);
    if (slot != NULL) {
        slot->state_0x03 = 0;
        slot->owner_0x30 = owner;
    }
}

/* 0x800FACF0 - register the area owner and the slot-1 handler. */
extern "C" void fn_800FACF0(void* owner) {
    EftPoolSlot* slot = (EftPoolSlot*)fn_800FBD68(1);
    if (slot != NULL) {
        slot->state_0x03 = 0;
        slot->owner_0x30 = owner;
        slot->handler_0x34 = (void*)fn_800FADCC;
    }
}

/* 0x800FAD40 - register the area owner and the slot-2 handler. */
extern "C" void fn_800FAD40(void* owner) {
    EftPoolSlot* slot = (EftPoolSlot*)fn_800FBD68(2);
    if (slot != NULL) {
        slot->state_0x03 = 0;
        slot->owner_0x30 = owner;
        slot->handler_0x34 = (void*)fn_800FADCC;
    }
}

/* 0x800FAD90 - release the model held by the owner. */
extern "C" void fn_800FAD90(EftModelOwner* self) {
    EftJointModel* model = self->model_0x38;
    fn_800F8A44(&model->field_0x04, model->field_0x00);
    model->field_0x00 = NULL;
}

/* 0x800FADCC - dispatch on the effect type byte. */
extern "C" void fn_800FADCC(EftFrameState* self) {
    switch (self->mode_0x05) {
    case 0:
        fn_800FAE08(self);
        break;
    case 1:
        fn_800FB160(self);
        break;
    case 2:
        fn_800FBBAC(self);
        break;
    case 3:
        fn_800FBBBC(self);
        break;
    default:
        break;
    }
}
