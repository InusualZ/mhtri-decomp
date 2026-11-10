/* ef/ef_animcurve.h - the symbols `ef/ef_animcurve.cpp` owns that other units call (C linkage): the curve random
 * generator's step and the name hash that seeds it. */
#ifndef MHTRI_EF_EF_ANIMCURVE_H
#define MHTRI_EF_EF_ANIMCURVE_H

#include "types.h"

/* The per-channel state a pattern curve leaves behind: the key count, the channel, the key's type and
 * its name-table index (ef_anim_tex_ramp and ef_anim_latch_tex_type read it back). */
struct EfAnimDivider {
    /* +0x00 */ u16 mKeyCount;
    /* +0x02 */ u8 pad_0x02[0x2];
    /* +0x04 */ u32 mChannel;
    /* +0x08 */ u8 mType;
    /* +0x09 */ u8 pad_0x09[0x3];
    /* +0x0C */ u32 mSlotBase;
}; /* size: 0x10 */

struct EfAnimParticle;
struct EfAnimRamp;
struct EfAnimNameTable;
struct EfPmManager;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009DEB0 - evaluates a u8 curve at `step` into the channels its mask selects. */
void ef_anim_curve_u8(const u8* mCmdList, u8* target, u32 step, u16 seed, u32 mode);
/* 0x8009F85C - evaluates an f32 curve at `step` into the channels its mask selects. */
void ef_anim_curve_f32(const u8* mCmdList, f32* target, u32 step, u16 seed, u32 mode);
/* 0x800A02F8 - the signed f32 curve (a rotation) evaluated at `step`. */
void ef_anim_curve_rotate(const u8* mCmdList, f32* target, u32 step, u16 seed, u32 mode);
/* 0x800A0D04 - evaluates a texture-pattern curve at `step` into the particle's channel. */
void ef_anim_curve_texture(const u8* mCmdList, struct EfAnimParticle* pp, u32 step, u16 seed, u32 mode,
                           const u8** nameTableOut, u32** target, struct EfAnimDivider* divider);
/* 0x800A14A4 - latches the particle's texture key type once. */
void ef_anim_latch_tex_type(struct EfAnimParticle* self, const struct EfAnimDivider* arg);
/* 0x800A1504 - advances the texture pattern through its ramp. */
void ef_anim_tex_ramp(struct EfAnimParticle* self, const struct EfAnimDivider* divider,
                      const struct EfAnimRamp* ramp, struct EfPmManager* manager,
                      const struct EfAnimNameTable* nameTable, u32* target);
/* 0x800A1CA8 - fires the child-creation keys passed between `step` and `step + 1`. */
void ef_anim_curve_child(const u8* mCmdList, struct EfAnimParticle* pp, u32 step, u16 seed, u32 mode);

/* 0x8009EEDC - one step of the curve random generator's LCG (`seed * 0x343FD + 0x269EC3`). */
u32 ef_anim_rand_next(u32 seed);

/* 0x8009EEF4 - the four-word hash a random draw is seeded from (the seed, the curve's id, the key and the
 * division). */
u32 ef_anim_name_hash(u16 a, u16 b, u16 c, u32 d);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_ANIMCURVE_H */
