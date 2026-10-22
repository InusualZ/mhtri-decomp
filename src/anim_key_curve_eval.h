/* anim_key_curve_eval.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `anim_key_curve_eval` (0x80052370), which `fn_8004CAD8.cpp` owns;
 * it the animation key-frame reader. */
#ifndef MHTRI_ANIM_KEY_CURVE_EVAL_H
#define MHTRI_ANIM_KEY_CURVE_EVAL_H

#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif

f32 anim_key_curve_eval(f32* a, f32* b, f32* c, f32 frame);

#ifdef __cplusplus
}
#endif

#endif
