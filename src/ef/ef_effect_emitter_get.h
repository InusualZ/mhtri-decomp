/* ef/ef_effect_emitter_get.h - leaf header for `ef_effect_emitter_count` and `ef_effect_emitter_get`, which
 * `src/ef/ef_effect.cpp` defines with C linkage (docs/plan.md 6.5 rule 2).  A leaf because consumers declare the two
 * over their own effect views and `ef/ef_effect.h` cannot take one spelling for all of them; the effect and the
 * emitter are opaque here. */
#ifndef MHTRI_EF_EF_EFFECT_EMITTER_GET_H
#define MHTRI_EF_EF_EFFECT_EMITTER_GET_H

#include "types.h"

struct EfEff;        /* the nw4r effect (`nw4r::ef::Effect`) */
struct EfEmitterObj; /* the nw4r emitter (`nw4r::ef::Emitter`) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A51D0 - the effect's live-emitter count. */
u16 ef_effect_emitter_count(struct EfEff* self);
/* 0x800A51D8 - the effect's idx-th live emitter (asserts idx is below the count). */
struct EfEmitterObj* ef_effect_emitter_get(struct EfEff* self, u16 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EFFECT_EMITTER_GET_H */
