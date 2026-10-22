/* enemy/em_curve_key.h - one keyframe of the motion curves `em_key_curve_eval` (`enemy/em_common.cpp`) evaluates:
 * the frame number, then the channel value.  A row whose frame is -1.0 ends the curve.  The enemy programs define
 * their curve tables as arrays of it. */
#ifndef MHTRI_ENEMY_EM_CURVE_KEY_H
#define MHTRI_ENEMY_EM_CURVE_KEY_H

#include "types.h"

/* size: 0x08 */
struct EmCurveKey {
    /* +0x00 */ f32 frame_0x00;
    /* +0x04 */ f32 value_0x04;
};

#endif /* MHTRI_ENEMY_EM_CURVE_KEY_H */
