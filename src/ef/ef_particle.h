/* ef/ef_particle.h - the symbols `ef/ef_particle.cpp` owns, spelled as its definitions (C linkage);
 * `EfParticle` is `ef.h`'s. */
#ifndef MHTRI_EF_EF_PARTICLE_H
#define MHTRI_EF_EF_PARTICLE_H

#include "types.h"
#include "ef.h"

/* The particle-parameter descriptor of an emitter resource (`ppd`), which ef_particle_param_init copies into the
 * particle's `EfParticleParams`; the layout matches the record up to +0x79. */
typedef struct EfParticleParamDesc {
    /* +0x00 */ u8 colors[2][2][4];
    /* +0x10 */ EfParticleScale size;
    /* +0x18 */ EfParticleScale scale;
    /* +0x20 */ VEC3 rotate;
    /* +0x2C */ EfParticleScale tex_scale[3];
    /* +0x44 */ f32 tex_rotate[3];
    /* +0x50 */ EfParticleScale tex_translate[3];
    /* +0x68 */ u32 textures[3];
    /* +0x74 */ u16 tex_wrap;
    /* +0x76 */ u8 tex_reverse;
    /* +0x77 */ u8 alpha_ref0;
    /* +0x78 */ u8 alpha_ref1;
    /* +0x79 */ u8 rotate_offset_random[3]; /* percent of the offset drawn at random */
    /* +0x7C */ f32 rotate_offset[3];        /* radians */
} EfParticleParamDesc; /* size: 0x88 (lower bound: +0x7C is the highest offset ef_particle_param_init reads) */

/* What a child particle takes over from its parent particle (ef_particle_init's `inherit`): percentages of the
 * parent's velocity, size, alpha and colour, the flag byte (bit 1: the rotation) and, per colour index, which
 * alpha sources multiply in (bit 0: index 0, bit 1: index 1, bit 2: the flicker). */
typedef struct EfParticleInherit {
    /* +0x00 */ s16 velocity;
    /* +0x02 */ u8 scale;
    /* +0x03 */ u8 alpha;
    /* +0x04 */ u8 color;
    /* +0x05 */ u8 pad_0x05[0x02];
    /* +0x07 */ u8 flags;
    /* +0x08 */ u8 alpha_flags[2];
} EfParticleInherit; /* size: 0x0A (lower bound: +0x09 is the highest offset ef_particle_init reads) */

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((never_inline)) u8* ef_resource_draw_setting(void* p);        /* the particle's colour block (chain head + 0x94) */
__attribute__((never_inline)) f32 ef_particle_get_scale(EfParticle* self); /* the record's scale product for the current phase */
__attribute__((never_inline)) f32 ef_particle_get_scale_y(EfParticle* self); /* the record's scale for the current emitter phase */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLE_H */
