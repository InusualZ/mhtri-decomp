/* ef/ef_effect.h - the symbols `ef/ef_effect.cpp` owns that other units call (C linkage). */
#ifndef MHTRI_EF_EF_EFFECT_H
#define MHTRI_EF_EF_EFFECT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The emitter description record a resource's body holds; each consumer reads it through its own view. */
struct EfEmitterDesc;

/* 0x800A4864 - the emitter description behind a resource (the record past its 8-byte header). */
/* untyped: opaque handle - the resource object is passed through under each caller's own view */
struct EfEmitterDesc* ef_res_emitter_desc(void* res);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EFFECT_H */
