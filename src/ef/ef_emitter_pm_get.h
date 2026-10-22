/* ef/ef_emitter_pm_get.h - leaf header for `ef_emitter_pm_count`, `ef_emitter_pm_get` and `ef_word_copy`, which
 * `src/ef/ef_emitter.cpp` defines with C linkage (docs/plan.md 6.5 rule 2).  A leaf because consumers declare them
 * over their own emitter and handle views and `ef/ef_emitter.h` cannot take one spelling for all of them. */
#ifndef MHTRI_EF_EF_EMITTER_PM_GET_H
#define MHTRI_EF_EF_EMITTER_PM_GET_H

#include "types.h"

struct EfEmitterObj;          /* the nw4r emitter (`nw4r::ef::Emitter`) */
struct EfDrawParticleManager; /* `ef/ef_drawstrategy.h` */
struct EftResHandle;          /* a one-word resource handle */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A970C - the emitter's particle-manager count. */
u16 ef_emitter_pm_count(struct EfEmitterObj* self);
/* 0x800A9714 - the emitter's index-th particle manager (asserts the index against the count). */
struct EfDrawParticleManager* ef_emitter_pm_get(struct EfEmitterObj* self, u16 index);
/* 0x800A898C - copies one word from src to dst and returns dst. */
struct EftResHandle* ef_word_copy(struct EftResHandle* dst, const struct EftResHandle* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EMITTER_PM_GET_H */
