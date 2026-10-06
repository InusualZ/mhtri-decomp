/* ef/fn_8030681C.h - the declarations of `ef/fn_8030681C.cpp`'s symbols its consumers call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_FN_8030681C_H
#define MHTRI_EF_FN_8030681C_H

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "camera/camera.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void eft_spawn_type_at_area(struct _ENEMY_WORK* self, u32 a);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_FN_8030681C_H */
