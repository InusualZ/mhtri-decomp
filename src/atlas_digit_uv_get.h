/* `atlas_digit_uv_get`, defined by `draw_shape.cpp` (leaf header, rule 2).  The prototype is the one `hud/layout.h`
 * carries, so a consumer may include both. */
#ifndef MHTRI_ATLAS_DIGIT_UV_GET_H
#define MHTRI_ATLAS_DIGIT_UV_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80055C5C (0x68): the two corner pairs of the digit atlas cell for the two-digit value `tex_idx`. */
/* untyped: byte range - the two s16 coordinate pairs the helper writes */
void atlas_digit_uv_get(void* uv0, void* uv1, u8 tex_idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ATLAS_DIGIT_UV_GET_H */
