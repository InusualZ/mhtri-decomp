/*
 * g3d/g3d_dcc.cpp - nw4r g3d's DCC texture-matrix builders: the texture SRT matrix for the Maya/XSI/3ds Max modes
 *   (the Maya and 3ds Max paths assert they are compiled out) and its overload.
 * RANGE. .text 0x80075E9C-0x800761BC (2 functions); extab 0x80008200-0x80008210, extabindex 0x80020928-0x80020940,
 *   .data 0x8058E570-0x8058E608 (opens on "g3d_dcc.cpp", read by both functions; then the pointer error and the
 *   NW4R_G3D_MAYA_DISABLE / NW4R_G3D_3DSMAX_DISABLE messages).  No `.sdata2`: neither function loads a pool word.
 *   `g3d/g3d_camera.cpp`'s tail precedes it; `g3d/g3d_draw1mat1shp.cpp` follows at fn_800761BC, which only
 *   fn_800769F4 calls and whose inline copies read that unit's strings.
 * FLAGS. cflags_g3d (docs/g3d.md); `#pragma peephole off` throughout (playbook idea 106); `#pragma pool_data off`.
 * NAMES. Map stems (nw4r's CalcTexMtx overloads, by the asserts: the pointer parameter is `pM`).
 *   fn_800D79B4 is `g3d/g3d_basic.cpp`'s basic-mode texture matrix builder, declared here until that unit has a
 *   header.
 * RESIDUALS. none known beyond the declaration above.
 */

#include "types.h"
#include "nw4r/db_assert.h"        /* nw4r::db::Panic (rule 2) */
#include "fn_8004CAD8.h"           /* mtx34_identity (rule 2) */
#include "g3d/g3d_xsi.h"           /* g3d_calc_tex_mtx_xsi (rule 2) */
#include "g3d/g3d_pointer_assert.h"
#include "g3d/g3d_dcc.h"

#pragma peephole off
#pragma pool_data off

extern "C" {
BOOL fn_800D79B4(nw4r::math::MTX34* pMtx, BOOL bSet, const nw4r::g3d::TexSrt* pSrt, u32 flag);

/* 0x80075E9C (0x1B4): builds the texture matrix of `pSrt` in the given mode (only XSI is compiled in), or the
 * identity when the builder declines and `bSet` asks for a set rather than a concatenation. */
void g3d_calc_tex_mtx(nw4r::math::MTX34* pMtx, BOOL bSet, const nw4r::g3d::TexSrt* pSrt, u32 flag, int mode) {
    G3D_POINTER_ASSERT("g3d_dcc.cpp", pMtx, 0x23, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    BOOL bIdentity = TRUE;
    if (mode == 0) {
        nw4r::db::Panic("g3d_dcc.cpp", 0x2C, "NW4R_G3D_MAYA_DISABLE must be disabled.");
    } else if (mode == 1) {
        bIdentity = !g3d_calc_tex_mtx_xsi(pMtx, bSet, pSrt, flag);
    } else {
        nw4r::db::Panic("g3d_dcc.cpp", 0x3C, "NW4R_G3D_3DSMAX_DISABLE must be disabled.");
    }
    if (bIdentity && bSet) {
        mtx34_identity(pMtx);
    }
}

/* 0x80076050 (0x16C): builds the basic-mode texture matrix of `pSrt`, or the identity when the builder declines
 * and `bSet` asks for a set. */
void g3d_calc_tex_mtx_dcc(nw4r::math::MTX34* pMtx, BOOL bSet, const nw4r::g3d::TexSrt* pSrt, u32 flag) {
    G3D_POINTER_ASSERT("g3d_dcc.cpp", pMtx, 0x4C, "NW4R:Pointer Error\npM(=%p) is not valid pointer.");
    BOOL bIdentity = !fn_800D79B4(pMtx, bSet, pSrt, flag);
    if (bIdentity && bSet) {
        mtx34_identity(pMtx);
    }
}

} /* extern "C" */
