/* `Pl_frame_check_alt`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_PL_FRAME_CHECK_ALT_H
#define MHTRI_PL_PL_FRAME_CHECK_ALT_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* The same frame check as `Pl_frame_check` with the model layer's second flag set. */
u32 Pl_frame_check_alt(struct _PLW* self, u16 frame, f32 a, f32 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_FRAME_CHECK_ALT_H */
