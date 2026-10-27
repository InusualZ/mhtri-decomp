/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `rso_overlay_mode1_load`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_RSO_OVERLAY_MODE1_LOAD_H
#define MHTRI_RSO_OVERLAY_MODE1_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80040B98 (0x110): loads the game-mode overlay set. */
void rso_overlay_mode1_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RSO_OVERLAY_MODE1_LOAD_H */
