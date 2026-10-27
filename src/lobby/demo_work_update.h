/* Leaf header (docs/plan.md 6.5 rule 2): `lobby/lb_server_sel_trans.cpp`'s `demo_work_update`, which `lobby/lb_menu_pos_tbl.cpp` calls. */
#ifndef MHTRI_LOBBY_DEMO_WORK_UPDATE_H
#define MHTRI_LOBBY_DEMO_WORK_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803C3F60 (0xC): the event demo's per-area work refresh. */
void demo_work_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_DEMO_WORK_UPDATE_H */
