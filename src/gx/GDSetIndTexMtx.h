/* gx/GDSetIndTexMtx.h - the leaf declarations of `GDSetIndTexMtx` and `GDResetCurrentMtx`, which `gx/fn_8009ACE4.c` defines (its caller is
 *   g3d/g3d_state.cpp). */
#ifndef MHTRI_GX_GDSETINDTEXMTX_H
#define MHTRI_GX_GDSETINDTEXMTX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8009ACE4 - writes the 2x3 indirect matrix `v` (a 3x4 matrix's first two rows) to BP 0x06+`base`. */
void GDSetIndTexMtx(int base, const f32* v);
/* 0x8009B0D8 - resets the current position/normal and texture matrix indices (caller: g3d/g3d_state.cpp). */
void GDResetCurrentMtx(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_GX_GDSETINDTEXMTX_H */
