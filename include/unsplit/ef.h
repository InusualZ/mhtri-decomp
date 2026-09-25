/* Not-yet-reconstructed `ef`-band symbols (the bracketing registered units both name `ef`).
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 *
 * An *owned* symbol is NOT declared here: `fn_800C5F74`/`fn_800C6064` live in
 * `include/ef/ef_drawstrategyimpl.h` and `fn_800B5A64` in `include/ef/fn_800AEE48.h`, so the callers
 * generic `void(...)` copies cannot collide with the owners' typed definitions in C++.
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
void fn_801173AC(void* self);
void fn_80043EA8(Vec3* out);
struct Vec;
void vec_to_mh_vec3(Vec3* dst, struct Vec* src);   /* converts the engine vector to nw4r's */

/* Declarations moved here from `enemy/fn_80176C58.cpp` (docs/plan.md 6.5 rule 2). */

/* fn_80105314 / fn_80105550 / fn_80105560 / fn_801057A4 / fn_8010A7D4 are owned by
 * `ef/fn_80105314.cpp` now - see `include/ef/fn_80105314.h`. */

/* 0x800C68E8 - the per-particle draw helper the free/line/point/smooth strategies call.  It has no
 * reconstructed owner yet, so it stays here.  `fn_800C5F74` (the base texture-set constructor) and
 * `fn_800C6064` (the per-draw setup) are owned by `ef/ef_drawstrategyimpl.cpp` and declared in
 * `include/ef/ef_drawstrategyimpl.h`. */
void fn_800C68E8(void* self, void* particle, void* ed, void* em, u32 first, u32 rebindColor);

/* Unsplit ef-band helpers the free-strategy draw calls (callers: ef/ef_drawfreestrategy.cpp). */
void fn_800B7DB0(void* a, MTX34* out);
void* fn_800B4B04(void* self, s16 flag);   /* the owner defines it; this is the ABI */
void fn_800B54B4(const void* src, Vec3* out);

/* 0x800FE978 - the state-0 handler of the map/area family's dispatcher (`ef/fn_800FD864.cpp`)
 * tail-calls.  It sits at the head of the next unclaimed range, so it has no registered owner yet. */
struct _EFT;

/* 0x8010BDE4..0x8010C0E0 run - the state-0 handler of `ef/fn_80105314.cpp`'s `fn_8010BDA8`
 * dispatcher and its siblings.  They sit at the head of the next unclaimed range, so they have no
 * registered owner yet (callers: ef/fn_80105314.cpp). */
struct _EFT013;
void fn_8010BDE4(struct _EFT013* self);
void fn_8010C0E0(struct _EFT013* self);
void fn_8010C454(struct _EFT013* self);
void fn_8010C464(struct _EFT013* self);


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_EF_H */
