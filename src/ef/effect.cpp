/* ef/effect.cpp - the game's effect manager: the effect handle helpers (root matrix, colour and scale changes,
 *   key-frame colour lookups) over `nw4r::ef::Effect`.
 * RANGE. .text 0x800F95A4-0x800FACAC (36 functions); extab 0x8000BC5C-0x8000BD4C, extabindex 0x80025974-0x80025ADC,
 *   .ctors 0x8056F30C-0x8056F310, .data 0x8059B5D0-0x8059B5F8, .bss 0x806A1400-0x806A20F0, .sdata2
 *   0x807965D8-0x80796608.  The name is the range's own `__FILE__` string `effect.cpp` (0x8059B5D0, passed to
 *   `nw4r::db::Panic` by `effect_emitter_alive_ck`).  `.sdata2` (the six float literals and the two int-to-float
 *   constants) and `.data` (the two assert strings) are emitted by this source and byte-identical to the target.
 * FLAGS. `cflags_main`; `#pragma fp_contract off` and `#pragma peephole off` for the whole file.
 *   `#pragma opt_common_subs off` (playbook 208) around `fn_800F97F0`: retail re-reads `node->field_0x10` after the
 *   calls, and indexes the name table through `eft_name_tbl_ptr`.
 *   `#pragma opt_optimizecontrolflow off` (playbook 148) around `fn_800F9E04` and `fn_800FA208`: retail tests both
 *   switches with a compare tree (`cmpwi 1; beq; bge` / `cmpwi 2; beq; bge`), the default control-flow pass makes
 *   a linear `beq` chain.
 * NAMES. `SetRootMtxTrans`, `effect_move`, `change_color_eff`, `change_paramscale_eff`, `change_paramscale_eff_vec3`,
 *   `eftGetKeyRGB` and `eftGetKeyAlpha` are the runtime dump's own names; the map has only `fn_` stems for the rest.
 *   GUESS (from the body): `effect_retire` (0x800F996C) - the slot it calls is +0x18 of the table at the effect's
 *   +0x1C, which `ef/ef_effect.h` lists as `Effect::Calc`, so `effect_calc` is the likelier name.
 *   GUESS: `change_color_scale_eff` (0x800F99D4), `eft_water_state_ck` (0x800F9D80: the water-flag legality test).
 *   GUESS (from each body and its callers): `effect_layer_reset`, `eft_rand_unit`, `eft_rand_signed_half`,
 *   `eft_shape_init`.
 * RESIDUALS. 5 partial rows:
 *  - `fn_800F9E04` (-4 B): retail hoists the address of `setMatColor`'s by-value colour copy (`addi r31,r1,8`)
 *    out of the material loop and keeps the loop index in r28, and builds `change_color_eff`'s colour argument
 *    before `r3`; ours materialises the copy's address in each iteration;
 *  - `fn_800FA450`: register colouring of `count`/`which`/`rec` after the emitter walk (r29/r31 in retail);
 *  - `effect_retire`, `effect_layer_reset`: retail's virtual calls stage the table in r12 (`lwz r12,0(r3);
 *    lwz r12,N(r12)`); the typed-table views here stage it in r4/r5 - the real classes (`nw4r::ef::Effect`,
 *    `MemoryManagerBase`) are not declarable from this unit without a second copy of their definitions;
 *  - `fn_800FA5D4`: the two `ResMat*` copy-constructor temporaries sit at 0xC/0x10 in retail (compiler temporaries
 *    bound to the copy constructors' reference parameters), ours names them, so 0x10/0xC.
 *   `fn_800FA208`: the frontend turns `if (hit == 1) return 1; return 0;` into `cntlzw` (the first backend dump
 *   already has it), so each case assigns `result` instead.
 *   flipcheck: `.bss` (the two static name arrays) and `.ctors` (`fn_800FAC1C`, the arrays' static initialiser)
 *   are claimed, not emitted: they are MWCC's `__sinit` for `static` arrays of a class whose constructors are
 *   `fn_800FAC78` and `eft_capsule_ctor`, and `eft_capsule_ctor` is called by its C name from eight other units,
 *   so emitting it as a constructor is a cross-unit rename (request ef-effect-2#2).
 */

#include "ef/eft_name_tbl_ptr.h" /* eft_name_tbl_ptr (rule 2: the owner's header) */
#include "ef/eft_scnbox_data_ptr.h" /* eft_scnbox_data_ptr (rule 2: the owner's header) */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/scnmdl.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_emitter.h" /* ef_store_word (rule 2) */
#include "pl.h"
#include "g3d/g3d_anmscn.h" /* the g3d_anmscn.cpp declarations (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMatTevColor (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "ef/effect_types.h"
#include "ef/eft_res.h" /* eft_pro_slot_assign (rule 2) */
#include "ef/ef_effect.h" /* ef_system_memory_manager (rule 2) */
#include "g3d/g3d_calcmaterial.h" /* res_mat_tev_color_copy_ctor (rule 2) */
#include "g3d/res_mat_chan_copy_ctor.h" /* res_mat_chan_copy_ctor (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the range's records
 * ------------------------------------------------------------------------------------------------- */

struct EftHandle;

/* The `nw4r::ef::Effect` virtual table as `effect_retire` reads it (the effect's class owns it): slot +0x18 is
 * `Calc`. */
typedef struct EftEffectTable {
    /* +0x00 */ u8 pad_0x00[0x18]; /* the RTTI word, the this-offset and the four slots before `Calc` */
    /* +0x18 */ void (*calc)(struct EftHandle* self, bool flag);
} EftEffectTable; /* size: 0x1C - lower bound, an approximation */

/* One pooled effect instance (`fn_800A51D8` returns it).  It carries its virtual table at +0x1C and its root
 * position at +0x9C. */
typedef struct EftHandle {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EftEffectTable* table_0x1C;
    /* +0x20 */ u8 pad_0x20[0x7C];
    /* +0x9C */ nw4r::math::VEC3 pos_0x9C;
} EftHandle; /* size: 0xA8 - lower bound, an approximation */

/* The effect system's memory manager as `effect_layer_reset` reads it: its virtual table (`MemoryManagerBase`'s,
 * not this unit's), whose slot +0x0C is `GarbageCollection`. */
struct EftMemoryManagerView;
typedef struct EftMemoryManagerTable {
    /* +0x00 */ u8 pad_0x00[0x0C]; /* the RTTI word, the this-offset and the destructor slot */
    /* +0x0C */ void (*garbage_collection)(struct EftMemoryManagerView* self);
} EftMemoryManagerTable; /* size: 0x10 - lower bound, an approximation */

typedef struct EftMemoryManagerView {
    /* +0x00 */ EftMemoryManagerTable* table;
} EftMemoryManagerView; /* size: 0x04 - lower bound, an approximation */

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

/* The capsule box `eft_shape_init` constructs and `pl_capsule_box_set` fills (the layout of `PlCapsuleBox`):
 * the capsule, its direction, its midpoint and the bounding reach. */
typedef struct EftShape {
    /* +0x00 */ EftBox box_0x00;
    /* +0x1C */ nw4r::math::VEC3 a_0x1C;
    /* +0x28 */ nw4r::math::VEC3 b_0x28;
    /* +0x34 */ f32 reach_0x34;
} EftShape; /* size: 0x38 */

/* The two-vector head `pl_segment_init` zeroes (the first 0x18 bytes of a box/shape record). */
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

/* The per-manager payload `change_color_scale_eff` builds and `fn_800F9A70` reads back. */
typedef struct EftParticleArgs {
    /* +0x00 */ u8 mode_0x00;
    /* +0x01 */ u8 color_0x01[4];
    /* +0x05 */ u8 color2_0x05[4];
    /* +0x09 */ u8 pad_0x09[0x03];
    /* +0x0C */ f32 scale_0x0C;
    /* +0x10 */ nw4r::math::VEC3 pos_0x10;
} EftParticleArgs; /* size: 0x1C */

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
 * +0x02 and two ten-byte name lists at +0x03 and +0x0D (`em_kind.cpp` strides the records by 0x17). */
typedef struct EftNameSet {
    /* +0x00 */ u8 count_0x00;
    /* +0x01 */ u8 count_0x01;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 names_0x03[10];
    /* +0x0D */ u8 names2_0x0D[10];
} EftNameSet; /* size: 0x17 */

/* One 8-byte node of the two name tables `fn_800FA9B8` walks. */
typedef struct EftNameNode {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 field_0x04;
} EftNameNode; /* size: 0x08 */

/* One 8-byte scenario-box descriptor (`eft_scnbox_load` walks the table). */
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

/* The unit's own symbols other units call. */
extern "C" void eft_state_flags_set(EftFrameState* self, u8 a, u8 b);
extern "C" EftVectors* pl_segment_init(EftVectors* self);
extern "C" nw4r::math::VEC3* eft_vec_ctor(nw4r::math::VEC3* self);

/* the unit's own helpers, used before their definitions below */
extern "C" u32 fn_800FA208(nw4r::math::VEC3* pos, EftEmitterRecord* rec);
extern "C" void fn_800FA318(EftBox* dst, const EftBox* src);
extern "C" void fn_800FA354(EftSphere* dst, const EftSphere* src);
extern "C" EftShape* eft_shape_init(EftShape* self);
extern "C" EftVectors* eft_capsule_ctor(EftVectors* self);
extern "C" void fn_800F9E04(EftSpawnOwner* owner, void* target, u8 mode, EftColorParams* params, u8 flag);

extern "C" {
/* the engine vector/matrix helpers */
void color_rgba_copy(void* dst, void* src);
void mtx34_copy(nw4r::math::MTX34* dst, const nw4r::math::MTX34* src);
void camera_proj_params_get(f32* out_a, f32* out_b);
void VEC2_ctor(void* out);
void* ef_emitter_tex_flags(void* self);
void ef_word_copy(void* dst, const void* src);
void ef_pm_handle(void* dst, void* src);
void ef_resource_find_emitter_handle(void* out, void* a, void* b, void* c);
void fn_800A602C(void* self, const nw4r::math::VEC3* pos, const nw4r::math::MTX34* mtx, f32 a, f32 b);
void fn_800AC100(void* mgr, u8 mode, u8* a, u8* b, f32 scale, const nw4r::math::VEC3* pos);

/* the effect pool / emitter walk */
void fn_800A5F4C(void* effect, u32 idx);
void fn_800A5E6C(void* effect, u32 idx);
void ef_system_retire_group(void* effect, u32 idx);
void* ef_effect_root_mtx(void* effect);
u16 ef_effect_emitter_count(void* effect);
void* ef_effect_emitter_get(void* effect, u16 idx);
u16 ef_emitter_pm_count(void* handle);
void* ef_emitter_pm_get(void* handle, u32 idx);
void fn_800A95D8(void* handle);

/* the resource / model helpers */
s32 ef_resource_instance(void);
void ef_resource_remove_all_effect_projects(void);
void ef_resource_remove_all_texture_projects(s32 handle);
s32 mhchar_mat_num_get(void* chr);
s32 copied_mat_access_valid_ck(void* access);

void* scn_mdl_mat_access_get_tev_color(void* access, u32 idx);
void* scn_mdl_mat_access_get_chan(void* access, u32 idx);

/* the emitter data helpers */
s32 eft_pro_slot_list_set_area(u32 a, u32 b, void* table, void* names);

void* stage_effect_block_get(void);
u32 stage_water_area_ck(void);
u32 stage_water_enabled_ck(void);
void pl_capsule_box_set(void* a, void* b);
u32 pl_capsule_point_ck(void* a, void* b);
f32 vec3_length_sq(void* v);

/* the file/alloc helpers */
void __construct_array(void* base, void* ctor, u32 a, u32 elemsize, u32 count);

/* the compiler-generated array constructors the unit hands to `__construct_array` */
EftNameArrayElem* fn_800FAC78(EftNameArrayElem* self);

/* the shared SDK entry points */
void* memset(void* dst, int v, u32 n);
void* memcpy(void* dst, const void* src, u32 n);

/* the unit's own symbols */
void fn_800F9A70(void* mgr, EftParticleArgs* data);
EftParticleArgs* fn_800F9A8C(EftParticleArgs* self);
void fn_800F9B68(EftHandle* handle, nw4r::math::VEC3* v);
void effect_retire(EftHandle* handle, u32 flag);
void change_color_scale_eff(void* effect, u8 mode, _GXColor* color, _GXColor* color2, f32 scale, nw4r::math::VEC3* pos, bool flag);
}

/* C++ callees: the target object references their manglings (`ran_suu__Fl`,
 * `work_mem_alloc__FUl`, `work_mem_free__FPv`, `load_file__FPcUll`,
 * `Panic__Q24nw4r2dbFPCciPCce`), so they are declared at C++ scope, outside the extern "C"
 * block above (relocaudit). */
s32 ran_suu(s32 max);
void* work_mem_alloc(u32 size);
void work_mem_free(void* p);
void load_file(char* name, u32 dst, s32 size);

namespace nw4r {

namespace db {
void Panic(const char* file, int line, const char* msg, ...);
}
}

/* the effect manager's control block and the two name tables live in the shared data run */
#include "unsplit/ef_control.h" /* eft_control (rule 2: the band) */
extern "C" void* lbl_8058A880[];
extern "C" void* lbl_8058A924[];
extern "C" u8 lbl_806A1400[];
extern "C" u8 lbl_806A2080[];

#pragma fp_contract off
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* 0x800F95A4 - build the camera-space root transform for one effect and hand it to the draw path. */
extern "C" void fn_800F95A4(void* self) {
    nw4r::math::MTX34 world;
    nw4r::math::VEC3 pos;
    f32 fov;
    f32 aspect;

    MTX34_ctor(&world);
    VEC3_ctor(&pos);
    copyVec3(&pos, &get_camera_pos());
    mtx34_copy(&world, &get_current_view_mtx());
    camera_proj_params_get(&fov, &aspect);
    fn_800A602C(self, &pos, &world, fov, aspect);
}

/* 0x800F9628 - retire the two live effect emitters and release the model. */
extern "C" void effect_layer_reset(void) {
    u32 i;
    void* effect = eft_control.effect_0x04;
    if (eft_control.initialised_0x00 == 0) {
        return;
    }
    if (effect == 0) {
        return;
    }
    for (i = 0; i < 2; i++) {
        fn_800A5F4C(effect, i);
        fn_800A5E6C(effect, i);
        ef_system_retire_group(effect, i);
    }
    EftMemoryManagerView* mm = (EftMemoryManagerView*)ef_system_memory_manager(effect);
    mm->table->garbage_collection(mm);
    s32 handle = ef_resource_instance();
    ef_resource_remove_all_effect_projects();
    ef_resource_remove_all_texture_projects(handle);
}

/* 0x800F96D4 - the 4-byte copy the transform setters share. */
extern "C" void eft_word_copy(u32* dst, const u32* src) {
    *dst = *src;
}

/* 0x800F96E0 - set the effect's root matrix translation from a world position. */
void SetRootMtxTrans(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos) {
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);
    if (effect != NULL) {
        const nw4r::math::MTX34* src = (const nw4r::math::MTX34*)ef_effect_root_mtx(effect);
        mtx34_copy(&mtx, src);
        mtx.m[0][3] = pos->x;
        mtx.m[1][3] = pos->y;
        mtx.m[2][3] = pos->z;
        effect->SetRootMtx(mtx);
    }
}

/* 0x800F975C - add a world position to the effect's root matrix translation. */
extern "C" void fn_800F975C(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos) {
    nw4r::math::MTX34 mtx;
    MTX34_ctor(&mtx);
    if (effect != NULL) {
        const nw4r::math::MTX34* src = (const nw4r::math::MTX34*)ef_effect_root_mtx(effect);
        mtx34_copy(&mtx, src);
        mtx.m[0][3] += pos->x;
        mtx.m[1][3] += pos->y;
        mtx.m[2][3] += pos->z;
        effect->SetRootMtx(mtx);
    }
}

#pragma opt_common_subs off
/* 0x800F97F0 - place one effect handle from the name table. */
extern "C" s32 fn_800F97F0(s32 idx, EftNameEntry* node, void* out) {
    if (node == NULL) {
        return 0;
    }
    if (node->field_0x10 == NULL) {
        return 0;
    }
    s32 frame;
    ef_resource_find_emitter_handle(&frame, (void*)ef_resource_instance(), eft_name_tbl_ptr[idx], node->field_0x10);
    ef_word_copy(out, &frame);
    return 1;
}
#pragma opt_common_subs reset

/* 0x800F9884 - whether every live emitter of the effect has a frame to move to. */
extern "C" s32 effect_emitter_alive_ck(nw4r::ef::Effect* effect) {
    s32 count;
    s32 i;
    s32 total;
    total = 0;
    count = ef_effect_emitter_count(effect);
    for (i = 0; i < count; i++) {
        void* handle = ef_effect_emitter_get(effect, (u16)i);
        if (handle == NULL) {
            nw4r::db::Panic("effect.cpp", 2854, "NW4R:Failed assertion 0");
        } else {
            total += ef_emitter_pm_count(handle);
        }
    }
    return total > 0;
}

/* 0x800F9920 - move the effect and report whether it is still alive. */
u32 effect_move(nw4r::ef::Effect* effect) {
    if (effect_emitter_alive_ck(effect) == 0) {
        return 0;
    }
    effect_retire((EftHandle*)(void*)effect, 0);
    return 1;
}

/* 0x800F996C - runs the effect's virtual `Calc` with `flag != 0`. */
extern "C" void effect_retire(EftHandle* handle, u32 flag) {
    handle->table_0x1C->calc(handle, flag != 0);
}

/* 0x800F9988 - colour the whole effect with a white default alpha. */
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color) {
    _GXColor c;
    c.r = 0;
    c.g = 0;
    c.b = 0;
    c.a = 0xFF;
    change_color_scale_eff(effect, 1, &color, &c, 5000.0f, pos, true);
}

/* 0x800F99D4 - hand a colour/scale request to every particle manager of the effect. */
extern "C" void change_color_scale_eff(void* effect, u8 mode, _GXColor* color, _GXColor* color2, f32 scale, nw4r::math::VEC3* pos, bool flag) {
    EftParticleArgs args;
    fn_800F9A8C(&args);
    args.mode_0x00 = mode;
    color_rgba_copy(args.color_0x01, color);
    color_rgba_copy(args.color2_0x05, color2);
    args.scale_0x0C = scale;
    copyVec3(&args.pos_0x10, pos);
    ((nw4r::ef::Effect*)effect)->ForeachParticleManager((void (*)(void*, u32))fn_800F9A70, (u32)&args, flag);
}

/* 0x800F9A70 - the per-manager callback `change_color_scale_eff` installs. */
extern "C" void fn_800F9A70(void* mgr, EftParticleArgs* data) {
    fn_800AC100(mgr, data->mode_0x00, data->color_0x01, data->color2_0x05, data->scale_0x0C, &data->pos_0x10);
}

/* 0x800F9A8C - zero a vector at +0x10 and return the record. */
extern "C" EftParticleArgs* fn_800F9A8C(EftParticleArgs* self) {
    VEC3_ctor(&self->pos_0x10);
    return self;
}

/* 0x800F9AC0 - scale the effect's first emitter uniformly. */
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale) {
    nw4r::math::VEC3 v;
    nw4r::math::VEC2 v2;
    VEC3_ctor(&v);
    VEC2_ctor(&v2);
    if (scale < 0.00001f) {
        return;
    }
    if (ef_effect_emitter_count(effect) != 0) {
        void* handle = ef_effect_emitter_get(effect, 0);
        if (handle != NULL) {
            setVector3(&v, scale, scale, scale);
            fn_800F9B68((EftHandle*)handle, &v);
        }
    }
}

/* 0x800F9B68 - install a scale vector on one handle. */
extern "C" void fn_800F9B68(EftHandle* handle, nw4r::math::VEC3* v) {
    copyVec3(&handle->pos_0x9C, v);
    fn_800A95D8(handle);
}

/* 0x800F9BA0 - scale the effect's first emitter by a vector. */
void change_paramscale_eff_vec3(nw4r::ef::Effect* effect, nw4r::math::VEC3* v) {
    nw4r::math::VEC3 tmp;
    VEC3_ctor(&tmp);
    if (vec3_length_sq(v) < 0.00001f) {
        return;
    }
    if (ef_effect_emitter_count(effect) != 0) {
        void* handle = ef_effect_emitter_get(effect, 0);
        if (handle != NULL) {
            fn_800F9B68((EftHandle*)handle, v);
        }
    }
}

/* 0x800F9C20 - load a 0xD0-byte scenario-box record for the area. */
extern "C" void eft_scnbox_load(void* dst, u8 area, u8 slot) {
    void* buf;
    u32 size;
    u32 rounded;
    EftScnboxEntry* entry;
    entry = (EftScnboxEntry*)eft_scnbox_data_ptr[area];
    if (entry == NULL) {
        memset(dst, 0, 0xD0);
        return;
    }
    size = entry[slot].size_0x00;
    if (size == 0) {
        memset(dst, 0, 0xD0);
        return;
    }
    rounded = (size + 0x1F) & ~0x1Fu;
    buf = work_mem_alloc(rounded);
    load_file(entry[slot].name_0x04, (u32)buf, (s32)rounded);
    memcpy(dst, buf, 0xD0);
    work_mem_free(buf);
}

/* 0x800F9CDC - a random 0.0..1.0 scale factor. */
extern "C" f32 eft_rand_unit(void) {
    u16 r = (u16)ran_suu(1);
    s32 v = (s32)(r & 0x3FF);
    return 0.001f * (f32)v;
}

/* 0x800F9D2C - a random -0.5..0.5 scale factor. */
extern "C" f32 eft_rand_signed_half(void) {
    u16 r = (u16)ran_suu(1);
    s32 v = (s32)(r & 0x3FF) - 0x200;
    return 0.002f * (f32)v;
}

/* 0x800F9D80 - whether the effect is still legal for its owner. */
extern "C" s32 eft_water_state_ck(EftFrameState* self) {
    if (self->value_0x1C < 0.0f) {
        u8 flags = self->flags_0x04;
        if ((flags & 0x4) != 0) {
            if (stage_water_area_ck() == 1) {
                return 1;
            }
        } else if ((flags & 0x2) != 0) {
            if (stage_water_enabled_ck() == 1) {
                return 1;
            }
        }
    }
    return 0;
}

/* 0x800F9DF4 - merge two flag bytes into the effect's state byte. */
extern "C" void eft_state_flags_set(EftFrameState* self, u8 a, u8 b) {
    self->flags_0x04 = (u8)(a | b);
}

/* Splits a packed 0xRRGGBBAA word into a colour. */
static inline void eft_color_from_u32(_GXColor* dst, u32 color) {
    dst->r = (u8)((color & 0xFF000000) >> 24);
    dst->g = (u8)((color & 0x00FF0000) >> 16);
    dst->b = (u8)((color & 0x0000FF00) >> 8);
    dst->a = (u8)(color & 0x000000FF);
}

#pragma push
#pragma opt_optimizecontrolflow off
/* 0x800F9E04 - colour/parameter update for one spawned effect. */
extern "C" void fn_800F9E04(EftSpawnOwner* owner, void* target, u8 mode, EftColorParams* params, u8 flag) {
    nw4r::math::VEC3 pos;
    _GXColor out;
    union {
        u32 word;
        _GXColor color;
    } stage;
    _GXColor cur;
    u32 v;
    VEC3_ctor(&pos);
    u32 color = get_stg_eft_col(get_now_areano(), flag);
    eft_color_from_u32(&stage.color, color);

    switch (mode) {
    case 0: {
        nw4r::ef::Effect* effect = (nw4r::ef::Effect*)target;
        const nw4r::math::MTX34* root = (const nw4r::math::MTX34*)ef_effect_root_mtx(effect);
        pos.x = root->m[0][3];
        pos.y = root->m[1][3];
        pos.z = root->m[2][3];
        v = (u32)((f32)stage.color.r * (0.01f * (f32)params->color_0x30[0]));
        if (v > 0xFF) v = 0xFF;
        out.r = (u8)v;
        v = (u32)((f32)stage.color.g * (0.01f * (f32)params->color_0x30[1]));
        if (v > 0xFF) v = 0xFF;
        out.g = (u8)v;
        v = (u32)((f32)stage.color.b * (0.01f * (f32)params->color_0x30[2]));
        if (v > 0xFF) v = 0xFF;
        out.b = (u8)v;
        v = (u32)((f32)stage.color.a * (0.01f * (f32)params->color_0x30[3]));
        if (v > 0xFF) v = 0xFF;
        out.a = (u8)v;
        change_color_eff(effect, &pos, out);
        break;
    }
    case 1: {
        MHchar* chr = (MHchar*)target;
        mhchar_mat_num_get(chr);
        v = (u32)((f32)owner->field_0x15 * ((f32)stage.color.r * (0.01f * (f32)params->color_0x30[0])) * 0.01f);
        if (v > 0xFF) v = 0xFF;
        out.r = (u8)v;
        v = (u32)((f32)owner->field_0x15 * ((f32)stage.color.g * (0.01f * (f32)params->color_0x30[1])) * 0.01f);
        if (v > 0xFF) v = 0xFF;
        out.g = (u8)v;
        v = (u32)((f32)owner->field_0x15 * ((f32)stage.color.b * (0.01f * (f32)params->color_0x30[2])) * 0.01f);
        if (v > 0xFF) v = 0xFF;
        out.b = (u8)v;
        out.a = 0xFF;
        if ((owner->flags_0x14 & 1) != 0) {
            for (s32 i = owner->field_0x16; i < owner->field_0x17; i++) {
                chr->getTevKColor(i, GX_KCOLOR3, &cur);
                out.a = cur.a;
                chr->setTevKColor(i, GX_KCOLOR3, &out);
            }
        } else if ((owner->flags_0x14 & 2) != 0) {
            for (s32 i = owner->field_0x16; i < owner->field_0x17; i++) {
                chr->getMatColor(i, GX_COLOR0A0, &cur);
                out.a = cur.a;
                chr->setMatColor(i, GX_COLOR0A0, out, false);
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
    u32 result;
    eft_vec_ctor((nw4r::math::VEC3*)&sphere);
    eft_capsule_ctor((EftVectors*)&box);
    eft_shape_init(&box2);
    switch (rec->mode_0x00) {
    case 0:
        result = 0;
        break;
    case 1:
        fn_800FA354(&sphere, (const EftSphere*)rec->sphere_0x04);
        if (hit_point_sphr(pos, &sphere.center_0x00, sphere.radius_0x0C) == 1) {
            result = 1;
        } else {
            result = 0;
        }
        break;
    case 2:
        fn_800FA318(&box, (const EftBox*)rec->box_0x14);
        pl_capsule_box_set(&box, &box2);
        if (pl_capsule_point_ck(pos, &box2) == 1) {
            result = 1;
        } else {
            result = 0;
        }
        break;
    case 3:
        if (pos->y < 0.0f) {
            result = 1;
            break;
        }
        /* a position above the plane falls into the default */
    default:
        result = 0;
        break;
    }
    return result;
}
#pragma pop

/* 0x800FA318 - copy a 0x1C-byte box record. */
extern "C" void fn_800FA318(EftBox* dst, const EftBox* src) {
    *dst = *src;
}

/* 0x800FA354 - copy a 0x10-byte sphere record. */
extern "C" void fn_800FA354(EftSphere* dst, const EftSphere* src) {
    *dst = *src;
}

/* 0x800FA378 - construct the box record and its two vectors. */
extern "C" EftShape* eft_shape_init(EftShape* self) {
    eft_capsule_ctor((EftVectors*)&self->box_0x00);
    VEC3_ctor(&self->a_0x1C);
    VEC3_ctor(&self->b_0x28);
    return self;
}

/* 0x800FA3B8 - construct the sphere record. */
extern "C" EftVectors* eft_capsule_ctor(EftVectors* self) {
    pl_segment_init(self);
    return self;
}

/* 0x800FA3E8 - zero the two vectors of a record. */
extern "C" EftVectors* pl_segment_init(EftVectors* self) {
    VEC3_ctor(&self->a_0x00);
    VEC3_ctor(&self->b_0x0C);
    return self;
}

/* 0x800FA420 - zero one vector. */
extern "C" nw4r::math::VEC3* eft_vec_ctor(nw4r::math::VEC3* self) {
    VEC3_ctor(self);
    return self;
}

/* 0x800FA450 - run the area effect set against the world. */
extern "C" void fn_800FA450(void* effect) {
    EftSphere sphere;
    nw4r::math::VEC3 pos;
    EftBox box;
    EftShape box2;
    u32 res;
    void* handle;
    u32 i;
    u32 count;
    u8 which;
    s32 alive;
    EftEmitterRecord* rec;
    s32 j;
    eft_vec_ctor((nw4r::math::VEC3*)&sphere);
    eft_capsule_ctor((EftVectors*)&box);
    VEC3_ctor(&pos);
    eft_shape_init(&box2);
    ef_store_word(&res, 0);
    count = ef_effect_emitter_count(effect);
    if (count == 0) {
        return;
    }
    for (i = 0; i < count; i++) {
        handle = ef_effect_emitter_get(effect, (u16)i);
        if (handle == NULL) {
            continue;
        }
        if (ef_emitter_pm_count(handle) != 0) {
            alive = 1;
            break;
        }
        alive = 0;
    }
    if (alive == 0) {
        return;
    }
    u32 pm;
    ef_pm_handle(&pm, ef_emitter_pm_get(handle, 0));
    ef_word_copy(&res, &pm);
    u8 bits = *(u8*)((u8*)ef_emitter_tex_flags(&res) + 3);
    if ((bits & 0x80) != 0) {
        which = 0;
    } else if ((bits & 0x40) != 0) {
        which = 1;
    } else {
        return;
    }
    const nw4r::math::MTX34* root = (const nw4r::math::MTX34*)ef_effect_root_mtx(effect);
    pos.x = root->m[0][3];
    pos.y = root->m[1][3];
    pos.z = root->m[2][3];
    rec = (EftEmitterRecord*)stage_effect_block_get();
    for (j = 0; j < 4; j++) {
        if (fn_800FA208(&pos, rec) == 1) {
            fn_800F9E04(NULL, effect, 0, (EftColorParams*)rec, which);
        }
        rec++;
    }
}

/* 0x800FA5D4 - run the weapon effect set against a character's model. */
extern "C" void fn_800FA5D4(EftSpawnOwner* owner, MHchar* chr) {
    EftSphere sphere;
    EftSphere sphere2;
    nw4r::math::VEC3 pos;
    EftBox box;
    EftShape shape;
    eft_vec_ctor((nw4r::math::VEC3*)&sphere);
    eft_vec_ctor((nw4r::math::VEC3*)&sphere2);
    eft_capsule_ctor((EftVectors*)&box);
    VEC3_ctor(&pos);
    eft_shape_init(&shape);
    s32 count = mhchar_mat_num_get(chr);
    u8 flags = owner->flags_0x14;
    if ((flags & 1) != 0) {
        for (s32 i = 0; i < count; i++) {
            nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)(u32)chr->field_0x118, (u32)i);
            if (copied_mat_access_valid_ck(&access) != 0) {
                void* tex;
                u32 handle = (u32)scn_mdl_mat_access_get_tev_color(&access, 0);
                res_mat_tev_color_copy_ctor((struct ResHandle*)&tex, (const struct ResHandle*)&handle);
                if (reinterpret_cast<const nw4r::g3d::ResMatTevColor*>(&tex)->IsValid() != 0) {
                    continue;
                }
                return;
            }
            return;
        }
    } else if ((flags & 2) != 0) {
        for (s32 i = 0; i < count; i++) {
            nw4r::g3d::ScnMdl::CopiedMatAccess access((nw4r::g3d::ScnMdl*)(u32)chr->field_0x118, (u32)i);
            if (copied_mat_access_valid_ck(&access) != 0) {
                void* tex;
                u32 handle = (u32)scn_mdl_mat_access_get_chan(&access, 0);
                res_mat_chan_copy_ctor((struct ResHandle*)&tex, (struct ResHandle*)&handle);
                if (reinterpret_cast<const nw4r::g3d::ResMatChan*>(&tex)->IsValid() != 0) {
                    continue;
                }
                return;
            }
            return;
        }
    } else {
        return;
    }
    s32 j;
    u8 flag = (owner->flags_0x14 & 0x80) != 0;
    EftEmitterRecord* rec = (EftEmitterRecord*)stage_effect_block_get();
    chr->get_joint_wpos(0, &pos);
    for (j = 0; j < 4; j++) {
        if (fn_800FA208(&pos, rec) == 1) {
            fn_800F9E04(owner, chr, 1, (EftColorParams*)rec, flag);
        }
        rec++;
    }
}

/* 0x800FA7A0 - sample an RGB key table at a frame. */
void eftGetKeyRGB(u8* keys, long frame, u8* r, u8* g, u8* b) {
    u8* p = keys;
    for (;;) {
        u8 end;
        u8 start = p[0];
        if (start == 0xFF) {
            *r = p[1];
            *g = p[2];
            *b = p[3];
            return;
        }
        if (frame >= (s32)start) {
            end = p[4];
            if (frame <= (s32)end) {
                f32 t = (f32)(frame - (s32)start) / (f32)((s32)end - (s32)start);
                *r = (u8)((f32)p[1] + t * (f32)((s32)p[5] - (s32)p[1]));
                *g = (u8)((f32)p[2] + t * (f32)((s32)p[6] - (s32)p[2]));
                *b = (u8)((f32)p[3] + t * (f32)((s32)p[7] - (s32)p[3]));
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
        u8 end;
        u8 start = p[0];
        if (start == 0xFF) {
            return p[1];
        }
        if (frame >= (s32)start) {
            end = p[2];
            if (frame <= (s32)end) {
                f32 t = (f32)(frame - (s32)start) / (f32)((s32)end - (s32)start);
                return (u8)((f32)p[1] + t * (f32)((s32)p[3] - (s32)p[1]));
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
        s32 m = 0;
        for (s32 i = 0; i < 11; i++) {
            names[i] = 0xFF;
        }
        while (node->field_0x00 != 0) {
            if (node->field_0x04 == 0) {
                node++;
                if (node->field_0x00 == 0) {
                    break;
                }
                if (node->field_0x04 != 0) {
                    u8 v = eft_pro_slot_assign(2, 2, area, 0, m++);
                    names[n++] = v;
                    out->field_0x02 = v;
                }
            } else {
                u8 v = eft_pro_slot_assign(2, 1, area, 0, out->count_0x00);
                names[n++] = v;
                out->names_0x03[out->count_0x00] = v;
                out->count_0x00++;
                node++;
                if (node->field_0x00 == 0) {
                    break;
                }
                if (node->field_0x04 != 0) {
                    u8 v2 = eft_pro_slot_assign(2, 1, area, 0, out->count_0x00);
                    names[n++] = v2;
                    out->names_0x03[out->count_0x00] = v2;
                    out->count_0x00++;
                }
            }
            node++;
        }
        eft_pro_slot_list_set_area(area, 0, lbl_8058A880[area], names);
    }
    out->count_0x01 = 0;
    node = (EftNameNode*)lbl_8058A924[area];
    if (node != NULL) {
        for (s32 i = 0; i < 11; i++) {
            names[i] = 0xFF;
        }
        while (node->field_0x00 != 0) {
            if (node->field_0x04 != 0) {
                u8 v = eft_pro_slot_assign(2, 3, area, 0, out->count_0x01);
                names[out->count_0x01] = v;
                out->names2_0x0D[out->count_0x01] = v;
                out->count_0x01++;
            }
            node++;
        }
        eft_pro_slot_list_set_area(area, 3, lbl_8058A924[area], names);
    }
}

/* 0x800FAC1C - construct the two static effect-name arrays. */
extern "C" void fn_800FAC1C(void) {
    __construct_array(lbl_806A1400, (void*)fn_800FAC78, 0, 0x20, 0x64);
    __construct_array(lbl_806A2080, (void*)eft_capsule_ctor, 0, 0x1C, 4);
}

/* 0x800FAC78 - construct one 0x20-byte name-array element. */
extern "C" EftNameArrayElem* fn_800FAC78(EftNameArrayElem* self) {
    eft_capsule_ctor(&self->vectors_0x04);
    return self;
}
