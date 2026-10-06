/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the particle-manager colour helpers, owned by `ef/ef_particlemanager.cpp`; spelled as the draw strategies call
 * them.  GUESS names (from the callers' use): `ef_pm_modulate_color`, `ef_pm_handle`.
 */
#ifndef MHTRI_EF_EF_PM_MODULATE_COLOR_H
#define MHTRI_EF_EF_PM_MODULATE_COLOR_H

#include "types.h"
#include "ef/ef_drawstrategy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AE6A8 - multiplies a particle's two colours of one layer by the emitter colour. */
void ef_pm_modulate_color(EfDrawParticleManager* pm, EfDrawParticle* pp, GXColor* color0, GXColor* color1);
/* 0x800AEE0C - builds the one-word resource handle of a manager. */
void ef_pm_handle(EfDrawParticleManager** handle, EfDrawParticleManager* pm);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_PM_MODULATE_COLOR_H */
