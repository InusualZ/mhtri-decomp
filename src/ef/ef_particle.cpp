/*
 * ef/ef_particle.cpp - the nw4r effect particle record (`EfParticle`, `ef.h`: a table pointer at +0x1C, the
 *   emitter-parameter sub-record at +0x20, the position at +0xCC, the phase index at +0xDC): its constructors and
 *   deleting destructor, the phase scale getters, the colour getters and the colour ramp.
 * RANGE. .text 0x800AA18C-0x800AB658 (19 functions); extab 0x80009E04-0x80009E6C, extabindex 0x800230AC-0x80023148,
 *   .data 0x80592D50-0x80592F78 (the `__FILE__` string "ef_particle.cpp" first; the inline asserts of
 *   `particle.h` pass that header's name), .sdata2 0x80796060-0x807960A0.
 * FLAGS. `cflags_main`; file-wide `#pragma fp_contract off`; `#pragma peephole off` through `ef_particle_param_init`
 *   (the table store, the `extsh` + `cmpwi` pair), on for `ef_rad_to_byte_angle`..`fn_800AA78C`, off for
 *   `ef_particle_init` (its kept `clrlwi` + `cmpwi` and `clrlwi` stores), on for the getters, off again for
 *   `ef_particle_flick_alpha`.
 * NAMES. The source spells the names the map takes from requests ef-emit#1..#27 (the map still has `fn_`/`lbl_`
 *   stems for them):
 *   GUESS: `ef_particle_ctor`, `ef_particle_params_ctor`, `ef_particle_delete` (constructor, member construction,
 *   deleting destructor), `ef_particle_send_closing`, `ef_particle_destroy_func`, `ef_particle_init` (class-table
 *   slots 0..2), `ef_particle_param_init` (copies the resource descriptor `ppd`), `ef_rad_to_byte_angle`,
 *   `ef_pm_scale_b`, `ef_effect_get_velocity` (returns effect + 0x88).
 *   GUESS: the `.data` labels `ef_particle_file_name`, `ef_particle_err_*`, `ef_particle_class_table`,
 *   `ef_particle_color_*`, `ef_particle_alpha_*` (their text; `color`/`alpha` say which getter passes them).
 *   GUESS (from the body and its callers): `ef_particle_get_color`, `ef_particle_get_alpha`,
 *   GUESS: `ef_particle_get_scale_y`, `ef_resource_draw_setting`, `ef_particle_get_scale`,
 *   GUESS: `ef_particle_flick_alpha`.
 *   GUESS (the particle parameters `ef/ef_resource.cpp` binds textures into): `ef_emres_get_ptcl_param`.
 *   GUESS: `ef_pm_scale_a` (0x800AB3C4, 0xC): the map folds it into `ef_particle_get_scale` (0x24); it is the
 *   function `ef_particle_get_scale` tail-calls with `b` (retail's `b` to the next instruction).
 * RESIDUALS. 3 partial rows:
 *  - `ef_particle_params_ctor`: the second 3-element loop's start and end pointers swap r30/r31 (retail's loops are the
 *    compiler's member-array construction);
 *  - `ef_particle_get_scale`: the map's row covers `ef_pm_scale_a` too, so the row reads 6 of 9 instructions until
 *    the row is split at +0x18;
 *  - `ef_particle_flick_alpha`: case 1's falling ramp keeps the amplitude in r3 and the partial sum in r5 where
 *    retail has them the other way round.
 *   flipcheck: `.data` claimed, not emitted (the strings are declared, and the class table at 0x80592E30 is not
 *   emitted - `EfParticle` is not yet a class); `.sdata2` byte-identical; `lbl_807960A0` (2pi) is
 *   `ef/ef_particlemanager.cpp`'s pool, read by `ef_particle_flick_alpha`.
 * SHAPES. The `.sdata2` constants are literals, which gives retail's pool order (2.0f, 1.0f, 100.0f, the `ef_particle_param_init` u32 bias,
 *   128.0f, pi, 0.5f, the two white colours, 0.0f, 0.01f, pi/128, the `ef_particle_init` s32 bias).
 * SHAPES. `ef_particle_init` takes the momentum after `space` (retail moves f1 between r8 and r9); its three `VEC3`
 *   locals are declared in retail's stack order; the colour share loads `inherit->color` once per entry
 *   (`percent = inherit->color` inside the first product); each rotation is a `rotate +=` accumulation.
 * SHAPES. `ef_particle_get_color`/`ef_particle_get_alpha` take `int` indices: `layer >= 0 && layer < 2` on an
 *   `int` is retail's materialised `li 0; cmplwi 1; bgt; li 1` test.
 * SHAPES. The `.data` order (TU strings, the class table, then `ef_particle_get_color`'s and `ef_particle_get_alpha`'s
 *   strings in reverse) and the `.text` order after `ef_particle_param_init`/`ef_particle_init` say the getters,
 *   `ef_rad_to_byte_angle` and the two-float copies are `particle.h` inlines emitted as weak copies after their first
 *   referencer, in reverse reference order: the way to emit `.data` is the `nw4r::ef::Particle` class with those inlines.
 * SHAPES. `ef_particle_flick_alpha` computes `step * count` and case 1's falling ramp in their own statements.
 * SHAPES. `ef_random_u16` returns `s32` (`ef/ef_emitter.h`): retail narrows the result at each store and divides
 *   it signed (`% 254`).
 */

#include "types.h"
#include "ef.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "ef/ef_emitter.h" /* ef_truncate_float/ef_res_block_body (rule 2) */
#include "ef/ef_particlemanager.h" /* fn_800AB740 / fn_800AB658 (rule 2) */
#include "g3d/g3d_scnroot.h" /* VEC2_ctor (rule 2) */
#include "ef/ef_particle.h"
#include "ef/ef_effect.h" /* ef_ref_object_init (rule 2) */
#include "ef/ef_creationqueue.h" /* ef_ref_object_add_ref (rule 2) */
#include "ef/ef_resource.h" /* ef_resource_instance (rule 2) */
#include "ef/ef_pm_modulate_color.h" /* ef_pm_modulate_color (rule 2) */
#include "ef/ef_emitter_tex_flags.h" /* ef_emitter_get_mtx (rule 2) */
#include "mh3_pad/vec3.h" /* copyVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "nw4r/mtx34_mult_vec3.h" /* mtx34_mult_vec3 (rule 2) */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */

#pragma fp_contract off
#pragma peephole off

/* This unit's own strings (its claimed `.data`), declared, never defined. */
extern char ef_particle_file_name[]; /* "ef_particle.cpp"                                      .data */
extern char ef_particle_err_ppd[]; /* "NW4R:Pointer Error\nppd(=%p) is not valid pointer."    .data */
extern char ef_particle_err_pm[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."     .data */
extern char ef_particle_err_manager_ef[]; /* "NW4R:Pointer Error\npm->mManagerEM->mManagerEF(=%p) ..." .data */
extern char ef_particle_color_err_color[]; /* "NW4R:Pointer Error\ncolor(=%p) is not valid pointer."  .data */
extern char ef_particle_color_file_color[]; /* "particle.h"                                            .data */
extern char ef_particle_color_err_index[]; /* "NW4R:Failed assertion index >= 0 && index < 2"         .data */
extern char ef_particle_color_file_index[]; /* "particle.h"                                            .data */
extern char ef_particle_color_err_layer[]; /* "NW4R:Failed assertion layer >= 0 && layer < 2"         .data */
extern char ef_particle_color_file_layer[]; /* "particle.h"                                            .data */
extern char ef_particle_err_false[];
extern char ef_particle_alpha_err_index[]; /* "NW4R:Failed assertion index >= 0 && index < 2"         .data */
extern char ef_particle_alpha_file_index[]; /* "particle.h"                                            .data */
extern char ef_particle_alpha_err_layer[]; /* "NW4R:Failed assertion layer >= 0 && layer < 2"         .data */
extern char ef_particle_alpha_file_layer[]; /* "particle.h"                                            .data */ /* "NW4R:Failed assertion false"                           .data */

/* The class dispatch table (data another unit owns; the constructor stores its address at +0x1C). */
extern char ef_particle_class_table[];

/* nw4r helpers, declared locally with C linkage. */
extern "C" void fn_800A4080(void* self); /* the particle's base constructor */
extern f32 lbl_807960A0; /* 2pi  .sdata2 (ef/ef_particlemanager.cpp's pool) */

/* nw4r::db::Panic.  The map already carries its real C++ mangling, and declaring the C++ spelling is
 * what reproduces the map's symbol exactly (tools/units/mangle.py). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (six materialised BOOLs), as ef/ef_emitter.cpp spells it. */
#define NW4R_POINTER_ASSERT(file, ptr, msg)                                                  \
    {                                                                                        \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;   \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                 \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))          \
            ok6_ = FALSE;                                                                    \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                           \
            ok5_ = FALSE;                                                                    \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                 \
            ok4_ = FALSE;                                                                    \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                           \
            ok3_ = FALSE;                                                                    \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                           \
            ok2_ = FALSE;                                                                    \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                           \
            ok1_ = FALSE;                                                                    \
        if (!ok1_)                                                                           \
            nw4r::db::Panic(file, __LINE__, msg, (ptr));                                     \
    }

/* Forward declarations: the unit's own functions, in address order, since a later one is called by an
 * earlier one (the object lays them out the same way). */
extern "C" EfParticleParams* ef_particle_params_ctor(EfParticleParams* self);
extern "C" u8 ef_rad_to_byte_angle(f32 angle);
extern "C" f32* fn_800AA75C(f32* dst, f32* src);
extern "C" void fn_800AA78C(f32* dst, f32* src);
extern "C" f32 ef_pm_scale_b(EfParticleMgr* mgr, EfParticle* self, f32 v);
extern "C" f32 ef_pm_scale_a(EfParticleMgr* mgr, EfParticle* self, f32 v);
extern "C" VEC3* ef_effect_get_velocity(nw4r::ef::Effect* effect);
extern "C" u8* ef_emres_get_ptcl_param(void* p);
extern "C" void ef_particle_get_color(EfParticle* self, int layer, int index, u8* color);
extern "C" u8 ef_particle_get_alpha(EfParticle* self, int layer, int index);
extern "C" u8 ef_particle_flick_alpha(EfParticle* self);

/* Constructor.  Installs the dispatch table, builds the parameter sub-record and clears the position. */
extern "C" EfParticle* ef_particle_ctor(EfParticle* self)
{
    fn_800A4080(self);
    self->slots = (EfParticleSlots*)ef_particle_class_table;
    ef_particle_params_ctor(&self->params);
    VEC3_ctor(&self->field_0xCC);
    return self;
}

/* Builds the emitter-parameter sub-record: two single sub-objects, one vector, two arrays of three
 * sub-objects and three vectors. */
extern "C" EfParticleParams* ef_particle_params_ctor(EfParticleParams* self)
{
    EfParticleNode* p;
    EfParticleNode* end;

    VEC2_ctor(&self->scale_0x10);
    VEC2_ctor(&self->scale_0x18);
    VEC3_ctor(&self->field_0x20);

    p = &self->field_0x2C[0];
    end = &self->field_0x2C[3];
    do {
        VEC2_ctor(p);
        p++;
    } while (p < end);
    p = &self->field_0x50[0];
    end = &self->field_0x50[3];
    do {
        VEC2_ctor(p);
        p++;
    } while (p < end);

    VEC3_ctor(&self->field_0x80);
    VEC3_ctor(&self->field_0x8C);
    VEC3_ctor(&self->field_0x98);
    return self;
}

/* Frees the record when the caller asks for it (positive flag); always returns the pointer so a
 * deleting expression can keep using it. */
extern "C" EfParticle* ef_particle_delete(EfParticle* p, s32 flag)
{
    if (p != 0) {
        if ((s16)flag > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* The destructor slot: hands the record to its owner for the actual teardown. */
extern "C" s32 ef_particle_send_closing(EfParticle* self)
{
    return fn_800AB740((struct EfPmManager*)self->params.manager, (struct EfPmParticle*)self);
}

/* The empty slot. */
extern "C" void ef_particle_destroy_func(EfParticle* self)
{
}

/* Copies the particle parameters out of the emitter resource's descriptor `ppd`; each rotate offset gets its
 * random share drawn from the owning emitter's random block before it is turned into a byte angle. */
extern "C" void ef_particle_param_init(EfParticleParams* self, EfParticleParamDesc* ppd, EfParticleMgr* pm)
{
    int i;

#line 55
    NW4R_POINTER_ASSERT(ef_particle_file_name, ppd, ef_particle_err_ppd);
    NW4R_POINTER_ASSERT(ef_particle_file_name, pm, ef_particle_err_pm);

    self->colors[0][0][0] = ppd->colors[0][0][0];
    self->colors[0][0][1] = ppd->colors[0][0][1];
    self->colors[0][0][2] = ppd->colors[0][0][2];
    self->colors[0][0][3] = ppd->colors[0][0][3];
    self->colors[0][1][0] = ppd->colors[0][1][0];
    self->colors[0][1][1] = ppd->colors[0][1][1];
    self->colors[0][1][2] = ppd->colors[0][1][2];
    self->colors[0][1][3] = ppd->colors[0][1][3];
    self->colors[1][0][0] = ppd->colors[1][0][0];
    self->colors[1][0][1] = ppd->colors[1][0][1];
    self->colors[1][0][2] = ppd->colors[1][0][2];
    self->colors[1][0][3] = ppd->colors[1][0][3];
    self->colors[1][1][0] = ppd->colors[1][1][0];
    self->colors[1][1][1] = ppd->colors[1][1][1];
    self->colors[1][1][2] = ppd->colors[1][1][2];
    self->colors[1][1][3] = ppd->colors[1][1][3];
    fn_800AA75C(&self->scale_0x10.x, &ppd->size.x);
    fn_800AA75C(&self->scale_0x18.x, &ppd->scale.x);
    copyVec3(&self->field_0x20, &ppd->rotate);

    for (i = 0; i < 3; i++) {
        f32 offset = ppd->rotate_offset[i];
        if (ppd->rotate_offset_random[i] != 0) {
            offset += ppd->rotate_offset_random[i]
                      * (offset * (2.0f * ef_random_float(&pm->emitter->progress) - 1.0f))
                      / 100.0f;
        }
        self->rotate_offset[i] = ef_rad_to_byte_angle(offset);
    }

    for (i = 0; i < 3; i++) {
        self->textures[i] = ppd->textures[i];
        fn_800AA75C((f32*)self->field_0x2C[i].data, &ppd->tex_scale[i].x);
        self->tex_rotate[i] = ppd->tex_rotate[i];
        fn_800AA75C((f32*)self->field_0x50[i].data, &ppd->tex_translate[i].x);
    }

    self->tex_wrap = ppd->tex_wrap;
    self->tex_reverse = ppd->tex_reverse;
    self->alpha_ref0 = ppd->alpha_ref0;
    self->alpha_ref1 = ppd->alpha_ref1;
    self->field_0x7D = 0;
}

#pragma peephole on

/* Maps a radian angle into the particle's byte angle: radians -> half-turns -> [-0.5, 127.5] -> u8. */
extern "C" u8 ef_rad_to_byte_angle(f32 angle)
{
    return (u8)((s32)ef_truncate_float(128.0f * (angle / 3.1415927f) - 0.5f) % 256);
}

/* Copies a two-float record and returns the destination. */
extern "C" f32* fn_800AA75C(f32* dst, f32* src)
{
    fn_800AA78C(dst, src);
    return dst;
}

/* Copies the two floats of a two-float record. */
extern "C" void fn_800AA78C(f32* dst, f32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

#pragma peephole off

/* Initialises a new particle of manager `pm`: life (at least 1), random seeds, the parameters from the emitter
 * resource, the position and velocity (the owning effect's velocity added, both moved into `space`), and the
 * share of the parent particle's velocity, size, alpha, colour and rotation `inherit` asks for. */
extern "C" bool ef_particle_init(EfParticle* self, u16 life, VEC3* pos, VEC3* vel, EfParticleMgr* pm,
                            const MTX34* space, f32 momentum, EfParticleInherit* inherit, EfParticle* parent)
{
    VEC3* effect_vel;
    VEC3 effect_v;
    VEC3 parent_v;
    VEC3 tmp;

#line 114
    NW4R_POINTER_ASSERT(ef_particle_file_name, pm, ef_particle_err_pm);
    ef_ref_object_init(self);
    if (life == 0) {
        life = 1;
    }
    self->params.manager = pm;
    ef_ref_object_add_ref((EffectManager*)pm);
    self->field_0xD8 = 0;
    self->field_0xE2 = 0;
    ef_resource_instance();
    ef_res_emitter_desc(pm->context);
    ef_particle_param_init(&self->params, (EfParticleParamDesc*)ef_emres_get_ptcl_param(pm->context), pm);
    self->params.momentum = momentum;
    self->field_0xDC = 0;
    self->random_seed = ef_random_u16(&pm->emitter->progress);
    self->life = life;
    self->random_byte = ef_random_u16(&pm->emitter->progress);
    self->params.field_0x79 = ef_random_u16(&pm->emitter->progress) % 254 - 126;
    copyVec3(&self->params.field_0x8C, pos);
    copyVec3(&self->params.field_0x98, &self->params.field_0x8C);
    copyVec3(&self->params.field_0x80, vel);

#line 148
    NW4R_POINTER_ASSERT(ef_particle_file_name, pm->emitter->effect, ef_particle_err_manager_ef);
    effect_vel = ef_effect_get_velocity((nw4r::ef::Effect*)pm->emitter->effect);
    if (0.0f != effect_vel->x || 0.0f != effect_vel->y || 0.0f != effect_vel->z) {
        MTX34 mtx;
        MTX34_ctor(&mtx);
        ef_emitter_get_mtx((EfDrawEmitter*)pm->emitter, &mtx);
        mtx34_inverse(&mtx, &mtx);
        mtx.m[0][3] = mtx.m[1][3] = mtx.m[2][3] = 0.0f;
        VEC3_ctor(&effect_v);
        mtx34_mult_vec3(&effect_v, &mtx, effect_vel);
        addVec3To(&self->params.field_0x80, &effect_v);
    }

    if (space != NULL) {
        MTX34 mtx;
        mtx34_mult_vec3(&self->params.field_0x8C, space, &self->params.field_0x8C);
        ef_mtx34_copy(&mtx, space);
        mtx.m[0][3] = mtx.m[1][3] = mtx.m[2][3] = 0.0f;
        mtx34_mult_vec3(&self->params.field_0x80, &mtx, &self->params.field_0x80);
    }

    copyVec3(&self->field_0xCC, setVec3(&tmp, 2.0f, 0.0f, 0.0f));

    if (parent != NULL && inherit != NULL) {
        if (inherit->velocity != 0) {
            MTX34 mtx;
            VEC3_ctor(&parent_v);
            MTX34_ctor(&mtx);
            ef_pm_get_mtx(parent->params.manager, &mtx);
            mtx.m[0][3] = mtx.m[1][3] = mtx.m[2][3] = 0.0f;
            mtx34_mult_vec3(&parent_v, &mtx, &parent->params.field_0x80);
            ef_pm_get_mtx(self->params.manager, &mtx);
            mtx.m[0][3] = mtx.m[1][3] = mtx.m[2][3] = 0.0f;
            mtx34_inverse(&mtx, &mtx);
            mtx34_mult_vec3(&parent_v, &mtx, &parent_v);
            s16 percent = inherit->velocity;
            self->params.field_0x80.x += 0.01f * (parent_v.x * percent);
            self->params.field_0x80.y += 0.01f * (parent_v.y * percent);
            self->params.field_0x80.z += 0.01f * (parent_v.z * percent);
        }

        if (inherit->scale != 0) {
            self->params.scale_0x10.x = 0.01f * (inherit->scale * ef_particle_get_scale(parent));
            self->params.scale_0x10.y = 0.01f * (inherit->scale * ef_particle_get_scale_y(parent));
        }

        if (inherit->alpha != 0) {
            GXColor color0 = {255, 255, 255, 255};
            GXColor color1 = {255, 255, 255, 255};
            int layer;

            ef_pm_modulate_color((EfDrawParticleManager*)parent->params.manager, (EfDrawParticle*)parent, &color0,
                                 &color1);
            for (layer = 0; layer < 2; layer++) {
                if (inherit->alpha_flags[0] != 0) {
                    u32 alpha = 255;
                    if (inherit->alpha_flags[0] & 1) {
                        alpha = alpha * ef_particle_get_alpha(parent, layer, 0) + 255;
                        alpha >>= 8;
                    }
                    if (inherit->alpha_flags[0] & 2) {
                        alpha = alpha * ef_particle_get_alpha(parent, layer, 1) + 255;
                        alpha >>= 8;
                    }
                    if (inherit->alpha_flags[0] & 4) {
                        alpha = alpha * ef_particle_flick_alpha(parent) + 255;
                        alpha = alpha * color0.a + 255;
                        alpha >>= 16;
                    }
                    self->params.colors[layer][0][3] = alpha * inherit->alpha / 100;
                }
                if (inherit->alpha_flags[1] != 0) {
                    u32 alpha = 255;
                    if (inherit->alpha_flags[1] & 1) {
                        alpha = alpha * ef_particle_get_alpha(parent, layer, 0) + 255;
                        alpha >>= 8;
                    }
                    if (inherit->alpha_flags[1] & 2) {
                        alpha = alpha * ef_particle_get_alpha(parent, layer, 1) + 255;
                        alpha >>= 8;
                    }
                    if (inherit->alpha_flags[1] & 4) {
                        alpha = alpha * ef_particle_flick_alpha(parent) + 255;
                        alpha = alpha * color0.a + 255;
                        alpha >>= 16;
                    }
                    self->params.colors[layer][1][3] = alpha * inherit->alpha / 100;
                }
            }
        }

        if (inherit->color != 0) {
            int layer;
            int index;
            for (layer = 0; layer < 2; layer++) {
                for (index = 0; index < 2; index++) {
                    u8 rgb[4];
                    u8 percent;
                    ef_particle_get_color(parent, layer, index, rgb);
                    self->params.colors[layer][index][0] = rgb[0] * (percent = inherit->color) / 100;
                    self->params.colors[layer][index][1] = rgb[1] * percent / 100;
                    self->params.colors[layer][index][2] = rgb[2] * percent / 100;
                }
            }
        }

        if (inherit->flags & 2) {
            f32 rotate;
            rotate = parent->params.field_0x20.x;
            rotate += (3.1415927f / 128.0f) * parent->params.rotate_offset[0];
            self->params.rotate_offset[0] = ef_rad_to_byte_angle(rotate);
            rotate = parent->params.field_0x20.y;
            rotate += (3.1415927f / 128.0f) * parent->params.rotate_offset[1];
            self->params.rotate_offset[1] = ef_rad_to_byte_angle(rotate);
            rotate = parent->params.field_0x20.z;
            rotate += (3.1415927f / 128.0f) * parent->params.rotate_offset[2];
            self->params.rotate_offset[2] = ef_rad_to_byte_angle(rotate);
        }
    }
    return true;
}
#pragma peephole on

/* Copies one colour entry out of the 2x2 table.  `layer` and `index` are both asserted into [0, 2). */
extern "C" void ef_particle_get_color(EfParticle* self, int layer, int index, u8* color)
{
    int ok;

    ok = (layer >= 0 && layer < 2);
    if (!ok) {
        nw4r::db::Panic(ef_particle_color_file_layer, 329, ef_particle_color_err_layer);
    }
    ok = (index >= 0 && index < 2);
    if (!ok) {
        nw4r::db::Panic(ef_particle_color_file_index, 330, ef_particle_color_err_index);
    }
    ok = IsValidPointer((u32)color);
    if (!ok) {
        nw4r::db::Panic(ef_particle_color_file_color, 331, ef_particle_color_err_color, color);
    }

    color[0] = self->params.colors[layer][index][0];
    color[1] = self->params.colors[layer][index][1];
    color[2] = self->params.colors[layer][index][2];
}

/* Returns one colour entry's alpha byte. */
extern "C" u8 ef_particle_get_alpha(EfParticle* self, int layer, int index)
{
    int ok;

    ok = (layer >= 0 && layer < 2);
    if (!ok) {
        nw4r::db::Panic(ef_particle_alpha_file_layer, 340, ef_particle_alpha_err_layer);
    }
    ok = (index >= 0 && index < 2);
    if (!ok) {
        nw4r::db::Panic(ef_particle_alpha_file_index, 341, ef_particle_alpha_err_index);
    }
    return self->params.colors[layer][index][3];
}

/* The record's scale for the current emitter phase: the phase bits select which pair of the four
 * scale factors is multiplied, and the owner's own factor scales the result again. */
extern "C" f32 ef_particle_get_scale_y(EfParticle* self)
{
    f32 v;
    s32 bits;

    bits = *(u16*)ef_resource_draw_setting(self->params.manager->context);
    bits &= 0x6000;

    switch (bits) {
    case 0x4000:
        v = self->params.scale_0x10.y * self->params.scale_0x18.x;
        break;
    case 0x2000:
        v = self->params.scale_0x10.x * self->params.scale_0x18.y;
        break;
    case 0x6000:
        v = self->params.scale_0x10.x * self->params.scale_0x18.x;
        break;
    default:
        v = self->params.scale_0x10.y * self->params.scale_0x18.y;
        break;
    }
    return ef_pm_scale_b(self->params.manager, self, v);
}

/* The owner's factor the scale is finally multiplied by. */
extern "C" f32 ef_pm_scale_b(EfParticleMgr* mgr, EfParticle* self, f32 v)
{
    return v * mgr->scale_b;
}

/* The owner's colour block: the chain head plus its colour-table offset. */
extern "C" u8* ef_resource_draw_setting(void* p)
{
    return (u8*)ef_res_emitter_desc(p) + 148;
}

/* The record's scale product for the current phase, with the owner's factor folded in. */
extern "C" f32 ef_particle_get_scale(EfParticle* self)
{
    f32 v;

    v = self->params.scale_0x10.x * self->params.scale_0x18.x;
    return ef_pm_scale_a(self->params.manager, self, v);
}

/* The owner's first factor the scale product is multiplied by. */
extern "C" f32 ef_pm_scale_a(EfParticleMgr* mgr, EfParticle* self, f32 v)
{
    return v * mgr->scale_a;
}

/* The effect's own velocity. */
extern "C" VEC3* ef_effect_get_velocity(nw4r::ef::Effect* effect)
{
    return &effect->velocity;
}

/* The owner's colour block, relocated to its table. */
extern "C" u8* ef_emres_get_ptcl_param(void* p)
{
    return (u8*)ef_res_block_body(p) + 4;
}

#pragma peephole off
/* Maps the record's phase into one of five byte colour ramps (mode 0: white), period +0xDC, amplitude and
 * step from the parameter chain, clamped to a byte. */
extern "C" u8 ef_particle_flick_alpha(EfParticle* self)
{
    EfParticleChain* p;
    u8 mode;
    u32 n;
    u16 count;
    u32 period;
    u16 rem;
    s16 out;

    p = (EfParticleChain*)ef_res_emitter_desc(self->params.manager->context);
    mode = p->mode;
    if (mode == 0) {
        return 255;
    }

    n = p->count_step * p->count;
    n = p->count + (s32)n * self->params.field_0x79 / 12700;
    period = n > 65535 ? 65535 : (u16)n;
    rem = (self->field_0xDC - 1) % (u16)period;
    count = period;

    out = 0;
    switch (mode) {
    case 1:
        if (rem * 2 <= count) {
            out = (s16)((128 - p->amplitude) + (p->amplitude * (rem * 4)) / count);
        } else {
            s32 v = p->amplitude * 4 - p->amplitude;
            v += -(p->amplitude * rem * 4) / count;
            out = (s16)(v + 128);
        }
        break;
    case 2:
        out = (s16)(p->amplitude + -(p->amplitude * rem * 2) / count + 128);
        break;
    case 3:
        out = (s16)((128 - p->amplitude) + (p->amplitude * (rem * 2)) / count);
        break;
    case 4:
        if (rem * 2 <= count) {
            out = (s16)(p->amplitude + 128);
        } else {
            out = (s16)(128 - p->amplitude);
        }
        break;
    case 5:
        out = (s16)(128.0f + (f32)p->amplitude * fn_800AB658((lbl_807960A0 * (f32)rem) / (f32)count));
        break;
    default:
        nw4r::db::Panic(ef_particle_file_name, 353, ef_particle_err_false);
        break;
    }

    if (out < 0) {
        out = 0;
    }
    if (out > 255) {
        out = 255;
    }
    return (u8)out;
}
