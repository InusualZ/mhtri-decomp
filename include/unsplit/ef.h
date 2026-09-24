/* Not-yet-reconstructed `ef`-band symbols (the bracketing registered units both name `ef`).
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_UNSPLIT_EF_H
#define MHTRI_UNSPLIT_EF_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 fn_800C9DCC(f32 arg0);
/* 0x800AB740 / 0x800AB658 - the particle owner's teardown entry and the sibling unit's ramp helper
 * (callers: ef/ef_particle.cpp). */
void* fn_800AB740(void* table, void* self);
f32 fn_800AB658(void* self, f32 v);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void fn_80105314(void* self);
void fn_80105550(void* self);
void fn_80105560(void* self);
void fn_801173AC(void* self);
void fn_80043EA8(Vec3* out);
struct Vec;
void vec_to_mh_vec3(Vec3* dst, struct Vec* src);   /* converts the engine vector to nw4r's */                                  /* out = (0, 0, 0) */

/* Declarations moved here from `enemy/fn_80176C58.cpp` (docs/plan.md 6.5 rule 2). */
void fn_801057A4(void* self, u32 a, Vec3* v, f32 scale, u32 id);
void fn_8010A7D4(void* self, u32 a);
/* Declarations moved here from `ef/fn_800AEE48.cpp` (docs/plan.md 6.5 rule 2): no registered unit
 * owns `fn_800C5F74`, so its extern lives beside the other not-yet-attributed `ef`-band symbols. */
void fn_800C5F74(void* self);
/* 0x800C5F74..0x800C68E8 - the `nw4r::ef` DrawStrategy family helpers the free/line/point/smooth
 * strategies call (callers: ef/ef_drawfreestrategy.cpp).  fn_800C5F74 is the base constructor,
 * fn_800C6064 the per-particle GX state setup, fn_800C68E8 the per-particle draw. */
void fn_800C5F74(void* self);
void fn_800C6064(void* self, void* a, void* b, void* c);
void fn_800C68E8(void* self, void* particle, void* ed, void* a, u32 first, u32 arg);

/* Unsplit ef-band helpers the free-strategy draw calls (callers: ef/ef_drawfreestrategy.cpp). */
void fn_800B7DB0(void* a, MTX34* out);
void* fn_800B4B04(void* self, s16 flag);   /* the owner defines it; this is the ABI */
void fn_800B54B4(const void* src, Vec3* out);

/* Declarations moved here from `enemy/fn_80176C58.cpp` (docs/plan.md 6.5 rule 2). */
void fn_801057A4(void* self, u32 a, Vec3* v, f32 scale, u32 id);
void fn_8010A7D4(void* self, u32 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_EF_H */
