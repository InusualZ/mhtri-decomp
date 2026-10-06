/*
 * g3d/g3d_scnmdlsmpl.cpp - nw4r g3d `ScnMdlSimple` scene model.
 * RANGE. .text 0x8007F0E4-0x80081188 (46 functions); extab, extabindex, .data 0x8058F0A0-0x8058F3D8 (opens on
 *   "g3d_scnmdlsmpl.cpp" with the class's assert texts), .sdata 0x80791210-0x80791228.  Both edges are tudiscover's
 *   weak codegen-fingerprint cuts and `.data` fragment boundaries; the 0x800810DC-0x80081188 tail is here because
 *   fn_80081120 reads the "ScnMdlSimple" name record (lbl_8056F688).  The right edge 0x80081188 is where
 *   `g3d/g3d_scnobj.cpp` opens: ScnObj::CalcWorldMtx, ScnObj::CalcViewMtx and the ScnObj constructor (it stores that
 *   unit's ScnObj vtable and reads the `.sdata2` word 0x80795E60, which is its claim).
 * NAMES. Map stems.  The pool's asserts name the members: `ScnMdlSimple::SetAnmObj` ("does not 'Bind' AnmObjChr
 *   now"), `mpAnmObjChr`, `mpAnmObjVis`, `mpAnmObjMatClr`, `mpAnmObjTexPat`, `mpAnmObjTexSrt`, `mdl.IsValid()`.
 *   AABB_ctor (0x80080F44) is a GUESS in the `MTX34_ctor`/`VEC3_ctor` scheme: it runs the two corner records'
 *   constructors, the element constructor of ScnObj's bounding-box array.
 * RESIDUALS. 45 functions unwritten (objdiff scores them zero): 0x8007F0E4-0x80080F44 and 0x80080F7C-0x80081188.
 *   flipcheck: `.data` and `.sdata` are claimed and not emitted.
 */

#include "types.h"
#include "g3d/g3d_scnmdlsmpl.h" /* this unit's own declarations (rule 1) */
#include "mh3_pad.h"            /* VEC3_ctor (rule 2) */

/* 0x80080F44 (0x38): constructs the box's two corner records. */
extern "C" nw4r::math::AABB* AABB_ctor(nw4r::math::AABB* pBox)
{
    VEC3_ctor(&pBox->min);
    VEC3_ctor(&pBox->max);
    return pBox;
}
