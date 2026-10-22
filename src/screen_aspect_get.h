/* Leaf header (docs/plan.md 6.5 rule 2): `main`'s `screen_aspect_get`.  GUESS name (camera/camera_main.cpp, the unit header). */
#ifndef MHTRI_SCREEN_ASPECT_GET_H
#define MHTRI_SCREEN_ASPECT_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004029C (main): the current screen's aspect ratio (the split-screen player's in split screen). */
f32 screen_aspect_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SCREEN_ASPECT_GET_H */
