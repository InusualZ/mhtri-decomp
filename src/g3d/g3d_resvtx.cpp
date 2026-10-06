/*
 * g3d/g3d_resvtx.cpp - nw4r g3d `ResVtx*` vertex-array accessors (`ResVtxPos`, `ResVtxNrm`, `ResVtxClr`,
 *   `ResVtxFurVec`, `ResVtxFurPos`, `ResVtxTexCoord`: `SetArray`/`GetArray`/`DCStore`-shaped bodies of 0x38..0x84
 *   bytes).
 * RANGE. .text 0x80088E24-0x800898B0 (35 functions); extab, extabindex, .data 0x8058FCE8-0x8058FDC8 (the
 *   `g3d_resvtx_ac.h`/`ResVtxFurVec`/`ResVtxTexCoord` "%s::%s: Object not valid." asserts), .sdata
 *   0x80791258-0x80791268.  The left seam is the second `.data` fragment after `g3d/g3d_state.cpp`'s.
 * NAMES. The file name is a GUESS from the class names (the `.data` pool carries only the header's name).
 * RESIDUALS. All 35 functions unwritten (objdiff scores them zero); flipcheck: the object emits no section.
 */

#include "types.h"
