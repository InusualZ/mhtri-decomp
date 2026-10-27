/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `demo_work_start`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_LOBBY_DEMO_WORK_START_H
#define MHTRI_LOBBY_DEMO_WORK_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C3A70 (0x304): starts the event demo work. */
void demo_work_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_DEMO_WORK_START_H */
