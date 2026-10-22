/* `atlas_icon_uv_get`, defined by `draw_shape.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ATLAS_ICON_UV_GET_H
#define MHTRI_ATLAS_ICON_UV_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80055D8C (0x3C): the two corner pairs of the 20-pixel icon atlas cell of tex_idx; the prototype is hud/layout.h's. */
/* untyped: byte range - the two s16 coordinate pairs the helper writes */
void atlas_icon_uv_get(void* uv0, void* uv1, u8 tex_idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ATLAS_ICON_UV_GET_H */
