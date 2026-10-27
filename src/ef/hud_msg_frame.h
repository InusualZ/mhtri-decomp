/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft052.cpp`'s `hud_msg_frame`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_HUD_MSG_FRAME_H
#define MHTRI_EF_HUD_MSG_FRAME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8035AB50 (0x14): the HUD message log's per-frame step. */
void hud_msg_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_HUD_MSG_FRAME_H */
