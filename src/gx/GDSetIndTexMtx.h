/* gx/GDSetIndTexMtx.h - the leaf declaration of `GDSetIndTexMtx`, which `gx/fn_8009ACE4.c` defines (its caller is
 *   g3d/g3d_state.cpp). */
#ifndef MHTRI_GX_GDSETINDTEXMTX_H
#define MHTRI_GX_GDSETINDTEXMTX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009ACE4 - writes the 2x3 indirect matrix `v` (a 3x4 matrix's first two rows) to BP 0x06+`base`. */
void GDSetIndTexMtx(int base, const f32* v);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_GX_GDSETINDTEXMTX_H */
