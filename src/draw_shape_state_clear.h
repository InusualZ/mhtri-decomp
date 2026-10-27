/* Leaf header (docs/plan.md 6.5 rule 2): `draw_shape_arm.cpp`'s `draw_shape_state_clear`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_DRAW_SHAPE_STATE_CLEAR_H
#define MHTRI_DRAW_SHAPE_STATE_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80056E00 (0x1C): clears the draw-shape state block's +0x58/+0x5C/+0x5D bytes and re-arms it. GUESS name. */
void draw_shape_state_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DRAW_SHAPE_STATE_CLEAR_H */
