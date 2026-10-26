/* Leaf header (docs/plan.md 6.5 rule 2): `camera_arena_mode_req`, `camera_quake_req`, which `camera/camera_main` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_CAMERA_CAMERA_QUAKE_REQ_H
#define MHTRI_CAMERA_CAMERA_QUAKE_REQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802BC6DC - requests the arena camera mode. */
void camera_arena_mode_req(void);

/* requests camera quake `kind`. */
void camera_quake_req(s32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CAMERA_CAMERA_QUAKE_REQ_H */
