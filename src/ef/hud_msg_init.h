/* Leaf header (docs/plan.md 6.5 rule 2): `ef/eft052.cpp`'s `hud_msg_init`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_HUD_MSG_INIT_H
#define MHTRI_EF_HUD_MSG_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8035A9E4 (0x16C): sets the HUD message log up. */
void hud_msg_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_HUD_MSG_INIT_H */
