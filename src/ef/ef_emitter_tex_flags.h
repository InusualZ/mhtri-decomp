/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the emitter accessors, owned by `ef/ef_emitter.cpp`; spelled as the draw strategies call
 * them.  GUESS names (from the callers' use): `ef_emitter_tex_flags`, `ef_emitter_get_mtx`.
 */
#ifndef MHTRI_EF_EF_EMITTER_TEX_FLAGS_H
#define MHTRI_EF_EF_EMITTER_TEX_FLAGS_H

#include "types.h"
#include "ef/ef_drawstrategy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A8944 - the emitter resource's texture flag bytes (+1/+2 hold the layers' mirror bits). */
u8* ef_emitter_tex_flags(EfDrawParticleManager** handle);
/* 0x800A94A4 - the emitter's world matrix into `out`. */
MTX34* ef_emitter_get_mtx(EfDrawEmitter* emitter, MTX34* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EMITTER_TEX_FLAGS_H */
