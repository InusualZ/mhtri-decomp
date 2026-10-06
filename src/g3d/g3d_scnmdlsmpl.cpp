/*
 * g3d/g3d_scnmdlsmpl.cpp - nw4r g3d `ScnMdlSimple` scene model.
 * RANGE. .text 0x8007F0E4-0x800813B8 (49 functions); extab, extabindex, .data 0x8058F0A0-0x8058F3D8 (opens on
 *   "g3d_scnmdlsmpl.cpp" with the class's assert texts), .sdata 0x80791210-0x80791228, .sdata2
 *   0x80795E60-0x80795E64.  Both edges are tudiscover's weak codegen-fingerprint cuts and `.data` fragment
 *   boundaries; the 0x800810DC-0x800813B8 tail is here because fn_80081120 reads the "ScnMdlSimple" name record
 *   (lbl_8056F688).
 * NAMES. Map stems.  The pool's asserts name the members: `ScnMdlSimple::SetAnmObj` ("does not 'Bind' AnmObjChr
 *   now"), `mpAnmObjChr`, `mpAnmObjVis`, `mpAnmObjMatClr`, `mpAnmObjTexPat`, `mpAnmObjTexSrt`, `mdl.IsValid()`.
 *   0x80081188, 0x80081250 and 0x80081260 are ScnObj::CalcWorldMtx, ScnObj::CalcViewMtx and the ScnObj constructor
 *   (it stores `g3d/g3d_scnobj.cpp`'s ScnObj vtable): the head of that unit's TU, left of its registered seam.
 * RESIDUALS. 47 functions unwritten (objdiff scores them zero): 0x8007F0E4-0x80081188 and 0x80081260-0x800813B8 (the
 *   ScnObj constructor: its MTX34/AABB member arrays are built by out-of-line element constructors the math types do
 *   not declare).
 *   flipcheck: `.data`, `.sdata` and `.sdata2` are claimed and not emitted.
 */

#include "types.h"
#include "g3d/g3d_scnobj.h"    /* nw4r::g3d::ScnObj, owner g3d/g3d_scnobj.cpp (rule 1) */
#include "g3d/g3d_calcview.h"  /* mtx34_copy_ps / mtx34_concat, owner g3d/g3d_calcview.cpp (rule 2) */

/* 0x80081188 (0xC8): composes the world matrix from the parent's and the local one, and the world box; bit 0 of the
 * parameter skips one pass. */
#pragma peephole off
void nw4r::g3d::ScnObj::CalcWorldMtx(const math::MTX34* pParent, u32* pParam)
{
    if (pParam != NULL && (*pParam & 1)) {
        *pParam &= ~1;
        return;
    }
    if (pParent != NULL) {
        if (TestScnObjFlag(SCNOBJFLAG_MTX_LOCAL_IDENTITY)) {
            mtx34_copy_ps(&mMtxArray[MTX_WORLD], pParent);
        } else {
            mtx34_concat(&mMtxArray[MTX_WORLD], pParent, &mMtxArray[MTX_LOCAL]);
        }
    } else {
        mtx34_copy_ps(&mMtxArray[MTX_WORLD], &mMtxArray[MTX_LOCAL]);
    }
    if (TestScnObjFlag(SCNOBJFLAG_ENABLE_CULLING)) {
        mAABB[BOUNDINGVOLUME_AABB_WORLD].Set(&mAABB[BOUNDINGVOLUME_AABB_LOCAL], &mMtxArray[MTX_WORLD]);
    }
}
#pragma peephole on

/* 0x80081250 (0x10): composes the view matrix from the camera and the world matrix. */
void nw4r::g3d::ScnObj::CalcViewMtx(const math::MTX34* pCamera)
{
    mtx34_concat(&mMtxArray[MTX_VIEW], pCamera, &mMtxArray[MTX_WORLD]);
}
