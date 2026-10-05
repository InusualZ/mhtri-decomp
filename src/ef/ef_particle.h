#ifndef MHTRI_EF_EF_PARTICLE_H
#define MHTRI_EF_EF_PARTICLE_H

#include "types.h"
#include "ef.h"

/* Declarations for the symbols `src/ef/ef_particle.cpp` owns (docs/plan.md 6.5, rule 2).  The signatures
 * are the owner's, spelled exactly as its definitions; C-visible, kept minimal.  `EfParticle` is the
 * engine's particle record defined in `ef.h`, so consumers include this header, not a local copy. */
#ifdef __cplusplus
extern "C" {
#endif

u8* fn_800AB388(void* p);          /* the particle's colour block (chain head + 0x94) */
f32 fn_800AB3AC(EfParticle* self); /* the record's scale product for the current phase */
f32 fn_800AB2DC(EfParticle* self); /* the record's scale for the current emitter phase */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PARTICLE_H */
