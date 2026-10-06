/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the emitter-resource accessor `ef/ef_particle.cpp` owns.
 */
#ifndef MHTRI_EF_EF_EMRES_GET_PTCL_PARAM_H
#define MHTRI_EF_EF_EMRES_GET_PTCL_PARAM_H

#include "types.h"

struct EfEmitterRes;
struct EfPtclParam;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AB3D8 (0x24): Returns the emitter resource's particle parameters. */
struct EfPtclParam* ef_emres_get_ptcl_param(struct EfEmitterRes* res);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EMRES_GET_PTCL_PARAM_H */
