/* camera/camera_demo_event_set.h - the declarations of `camera_demo_event_set`, which `camera/camera_main.cpp` owns (docs/plan.md 6.5 rule 2), with the
 * argument views `enemy/em020_prog.cpp` calls them through. */
#ifndef MHTRI_CAMERA_CAMERA_DEMO_EVENT_SET_H
#define MHTRI_CAMERA_CAMERA_DEMO_EVENT_SET_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif
void camera_demo_event_set(u32 id, struct _ENEMY_WORK* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_DEMO_EVENT_SET_H */
