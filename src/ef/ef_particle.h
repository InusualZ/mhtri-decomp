/* ef/ef_particle.h - the symbols `ef/ef_particle.cpp` owns, spelled as its definitions (C linkage);
 * `EfParticle` is `ef.h`'s. */
#ifndef MHTRI_EF_EF_PARTICLE_H
#define MHTRI_EF_EF_PARTICLE_H

#include "types.h"
#include "ef.h"

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
