/*
 * nw4r g3d: g3d_resvtx.cpp - the `ResVtx*` vertex-array accessors (`ResVtxPos`, `ResVtxNrm`, `ResVtxClr`, `ResVtxFurVec`, `ResVtxFurPos`,
 * `ResVtxTexCoord` `SetArray`/`GetArray`/`DCStore`-shaped bodies, 35 functions of 0x38..0x84 bytes), `.text` 0x80088E24..0x800898B0,
 * `.data` 0x8058FCE8..0x8058FDC8 (the `g3d_resvtx_ac.h` / `ResVtxFurVec` / `ResVtxTexCoord` assert strings), `.sdata`
 * 0x80791258..0x80791268, extab 0x80008DE0..0x80008EB0, extabindex 0x800218AC..0x800219E4.
 *
 * Stub: the range was the tail of the old `g3d/g3d_state.cpp` (a second `.data` fragment opens at 0x8058FCE8, and the
 * strings there are the `ResVtx*` "%s::%s: Object not valid." asserts of nw4r's `g3d_resvtx.cpp`); the reconciled candidate cuts it
 * out.  No body is decompiled: the 35 functions are unwritten (0 %), none was in `g3d_state.cpp`'s source.
 *
 * Name: `g3d_resvtx` is the nw4r file the `g3d_resvtx_ac.h` strings and `ResVtx*` class names belong to (the placeholder stem was
 * `fn_80088E24`); the `.data` pool carries only the header's name, so the file name is a GUESS from the class names.
 * The unit keeps the g3d lib's flags.
 */

#include "types.h"
