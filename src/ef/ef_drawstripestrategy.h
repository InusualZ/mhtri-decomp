/*
 * Declarations for the symbols `src/ef/ef_drawstripestrategy.cpp` owns (docs/plan.md 6.5 rule 2): the draw-time particle copies and ahead-vector
 * builders (0x800B8788..0x800B882C) that the next unit's walker dispatch (`ef/ef_drawbillboardstrategy.cpp`) hands out as function pointers.
 * The strip unit's other declarations are still in `ef/fn_800AEE48.h` (the header of the registration it was cut from).
 */
#ifndef MHTRI_EF_EF_DRAWSTRIPESTRATEGY_H
#define MHTRI_EF_EF_DRAWSTRIPESTRATEGY_H

#include "types.h"
#include "nw4r/math.h"

struct EfParticleState; /* ef/fn_800AEE48.h */

#ifdef __cplusplus
extern "C" {
#endif

/* A node whose +0xAC vector the ahead builder reads. size: 0xB8 */
typedef struct EfAheadItem {
    u8 pad_0x00[0xAC]; /* +0x00 */
    Vec field_0xAC;    /* +0xAC */
} EfAheadItem;

/* 0x800B8788 - builds the particle's +0xB0 transform into a local and copies it into `dst`. */
void fn_800B8788(nw4r::math::VEC3* dst, struct EfParticleState* particle);
/* 0x800B87C8 - copies the particle's +0x98 block into `dst`. */
void fn_800B87C8(nw4r::math::VEC3* dst, struct EfParticleState* particle);
/* 0x800B87D0 - subtracts the two ahead vectors; falls back to the particle's reference block when the result is degenerate. */
void fn_800B87D0(Vec* a, struct EfParticleState* particle, EfAheadItem* item);
/* 0x800B882C - builds the ahead vector; `arg` is the effect's ahead-context object, handed through to `fn_800A7F00`. */
/* untyped: opaque handle passed through to fn_800A7F00 */
void fn_800B882C(Vec* a, struct EfParticleState* particle, void* arg);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_DRAWSTRIPESTRATEGY_H */
