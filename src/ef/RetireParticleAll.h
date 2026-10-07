/* ef/RetireParticleAll.h - leaf header: `RetireParticleAll` (0x800A4AF8), owned by `ef/ef_effect.cpp`. */
#ifndef MHTRI_EF_RETIREPARTICLEALL_H
#define MHTRI_EF_RETIREPARTICLEALL_H

#include "types.h"
#include "ef.h"

#ifdef __cplusplus
extern "C" {
#endif
/* Retires every particle on every live emitter of `effect`. */
void RetireParticleAll(nw4r::ef::Effect* effect);
#ifdef __cplusplus
}
#endif

#endif
