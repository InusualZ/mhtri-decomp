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
/* 0x800A7750 - `ef_emitter.cpp`'s per-emitter creation entry, called by the creation queue
 * (ef/ef_creationqueue.cpp): given the emitter form, the effect handle, the setting record, the
 * manager, a life and an optional VEC3 position.  No reconstructed owner yet, hence the band. */
void fn_800A7750(void* form, void* eh, const void* setting, void* manager, u16 life, const void* pos);
/* 0x800AB740 / 0x800AB658 - the particle owner's teardown entry and the sibling unit's ramp helper
 * (callers: ef/ef_particle.cpp). */
void* fn_800AB740(void* table, void* self);
f32 fn_800AB658(void* self, f32 v);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
void fn_801173AC(void* self);
void fn_80043EA8(Vec3* out);

/* 0x800CFA90 / 0x800CFAD0 - the move-work record table and its record count.  No registered unit
 * owns the run (it sits between `ef/eft001.cpp` and `ef/eft002.cpp`), so this band header is their
 * rule-2 home.  Added with `Pl/fn_80273B14.cpp`, which walks the records by their +0x008 slot byte. */
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);
struct Vec;

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

/* fn_803386C4 (0x803386C4) has an owner now - `hud/fn_80334568.cpp` registered the band that covers
 * it - so it does not belong in this fallback band (rule 2: the band is the home for a symbol no unit
 * owns).  Its call-site declaration is in `include/ef/eft_slot.h`, the calling unit's own header,
 * because the owner's header cannot be included from the ef band; that file records why. */

/* fn_8010BDE4..fn_8010C464 are owned by `ef/fn_8010BDE4.cpp` now - see `include/ef/fn_8010BDE4.h`
 * (rule 2: an owned symbol is declared in the owner's header, not here). */



#ifdef __cplusplus
}
#endif

/* The target objects reference this by its C++ mangling
 * (`vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`), so it is declared with C++ linkage (relocaudit). */
#ifdef __cplusplus
void vec_to_mh_vec3(Vec3* dst, struct Vec* src);   /* converts the engine vector to nw4r's */
#endif

#endif /* MHTRI_UNSPLIT_EF_H */
