/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `tex_palette_bind`, for `font/flfnt.cpp`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_EF_TEX_PALETTE_BIND_H
#define MHTRI_EF_TEX_PALETTE_BIND_H

#include "types.h"

struct TPLPalette;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D0110 (0x1B0): binds palette `pal` and builds its textures into the slots from `index`. */
s32 tex_palette_bind(struct TPLPalette* pal, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_TEX_PALETTE_BIND_H */
