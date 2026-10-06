/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `ef_draw_info_projection`, owned by `ef/ef_effectsystem.cpp`; spelled as the draw strategies call
 * it.  GUESS names (from the callers' use): `ef_draw_info_projection`.
 */
#ifndef MHTRI_EF_EF_DRAW_INFO_PROJECTION_H
#define MHTRI_EF_EF_DRAW_INFO_PROJECTION_H

#include "types.h"
#include "ef/ef_drawstrategy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A60B8 - the projection block of a draw's view state. */
const MTX34* ef_draw_info_projection(const EfDrawInfo* info);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_DRAW_INFO_PROJECTION_H */
