/* unsplit/ef.h - the ef band header: the declarations the ef consumers still read from the band (rule 2's residual):
 * `fn_801173AC`, whose owner has no header yet (one would carry a generated file name), two particle-manager entries
 * whose owner spells them differently, the move-work accessors and
 * `vec_to_mh_vec3`.  The other ef symbols are declared in their owners' headers (`ef/ef_emitter.h`, `ef/ef_torus.h`,
 * `ef/ef_particlemanager.h`, `ef/ef_drawstrategyimpl.h`, `ef/ef_drawstripestrategy.h`). */
#ifndef MHTRI_UNSPLIT_EF_H
#define MHTRI_UNSPLIT_EF_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AB740 / 0x800AB658 - `ef/ef_particlemanager.cpp`'s teardown entry and ramp helper, in the spelling of their
 * caller `ef/ef_particle.cpp`; the owner defines them `s32 (EfPmManager*, EfPmParticle*)` and `f32 (f32)`, and its
 * header cannot carry this spelling until the two agree. */
void* fn_800AB740(void* table, void* self);
f32 fn_800AB658(void* self, f32 v);

/* `ef/fn_801173AC.cpp`'s eft024 kind-1 state-1 handler. */
void fn_801173AC(void* self);
/* `VEC3_ctor` (0x80043EA8) is `src/mh3_pad.cpp`'s and is declared in `mh3_pad.h`, which the
 * consumers of this band include (docs/plan.md 6.5 rule 2: a band header declares no owned symbol). */

/* 0x800CFA90 / 0x800CFAD0 - `ef/system_core.cpp`'s move-work record table and its record count; `Pl/fn_80273B14.cpp`
 * walks the records by their +0x008 slot byte. */
#ifdef __cplusplus
extern "C++" { /* the map spells both `__FUc`: C++ free functions, not C names */
#endif
void* get_move_work_adrs(u8 kind);
u32 get_move_work_max(u8 kind);
#ifdef __cplusplus
}
#endif
struct Vec;

/* The effect record the band's callers pass. */
struct _EFT;

/* `eft_net_send` (0x803386C4) is `hud/pl_frame_sync.cpp`'s, declared in `hud/eft_net_send.h`. */

#ifdef __cplusplus
}
#endif

/* The target objects reference this by its C++ mangling
 * (`vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec`), so it is declared with C++ linkage (relocaudit). */
#ifdef __cplusplus
void vec_to_mh_vec3(Vec3* dst, struct Vec* src);   /* converts the engine vector to nw4r's */
#endif

#endif /* MHTRI_UNSPLIT_EF_H */
