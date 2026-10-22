/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `camera/camera_main.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_CAMERA_CAMERA_LOBBY_MODE_OFFSET_REQ_H
#define MHTRI_CAMERA_CAMERA_LOBBY_MODE_OFFSET_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BC820 - requests camera event mode `base` + 17 for `enemy`. */
void camera_lobby_mode_offset_req(u32 base, struct _ENEMY_WORK* enemy);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_LOBBY_MODE_OFFSET_REQ_H */
