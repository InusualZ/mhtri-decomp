/* enemy/em_camera_req.h - the declaration of `em_camera_req`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_CAMERA_REQ_H
#define MHTRI_ENEMY_EM_CAMERA_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_camera_req(struct _ENEMY_WORK* self, u32 a, u32 b);
#ifdef __cplusplus
}
#endif

#endif
