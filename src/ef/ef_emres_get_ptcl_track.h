/*
 * Leaf header (docs/plan.md 6.5 rule 2) for three emitter-resource accessors `ef/ef_emitter.cpp` owns: the particle
 * track block and the emitter-track count and pointer table that follow it.
 */
#ifndef MHTRI_EF_EF_EMRES_GET_PTCL_TRACK_H
#define MHTRI_EF_EF_EMRES_GET_PTCL_TRACK_H

#include "types.h"

struct EfEmitterRes;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A8BC8 (0x30): Returns the number of emitter tracks the emitter resource holds. */
u16 ef_emres_num_emit_track(struct EfEmitterRes* res);
/* 0x800A8BF8 (0x2C): Returns the particle-track block (its u16 count first). */
u8* ef_emres_get_ptcl_track(struct EfEmitterRes* res);
/* 0x800A8CB8 (0x30): Returns the emitter-track pointer table (its size table follows it). */
u8** ef_emres_get_emit_track_tbl(struct EfEmitterRes* res);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_EMRES_GET_PTCL_TRACK_H */
