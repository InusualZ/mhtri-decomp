/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `g3d_state_set_render_mode` (0x80088574), owned by `g3d/g3d_state.cpp`:
 * G3DState's render-mode setter, called by the resource manager's start-up with the boot render mode.
 */
#ifndef MHTRI_G3D_G3D_STATE_SET_RENDER_MODE_H
#define MHTRI_G3D_G3D_STATE_SET_RENDER_MODE_H

#include "types.h"
#include "gx/GXRenderModeObj.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80088574 (0x10): Stores the render mode G3D draws with. */
void g3d_state_set_render_mode(const GXRenderModeObj* pRenderMode);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_SET_RENDER_MODE_H */
