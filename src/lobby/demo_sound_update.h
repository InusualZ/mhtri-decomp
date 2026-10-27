/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `demo_sound_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_LOBBY_DEMO_SOUND_UPDATE_H
#define MHTRI_LOBBY_DEMO_SOUND_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C3F6C (0x1FC): the event demo's per-frame sound step. */
void demo_sound_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_DEMO_SOUND_UPDATE_H */
