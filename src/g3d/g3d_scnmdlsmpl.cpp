/*
 * g3d/g3d_scnmdlsmpl.cpp - nw4r g3d `ScnMdlSimple` scene model.
 * RANGE. .text 0x8007F0E4-0x800813B8 (49 functions); extab, extabindex, .data 0x8058F0A0-0x8058F3D8 (opens on
 *   "g3d_scnmdlsmpl.cpp" with the class's assert texts), .sdata 0x80791210-0x80791228, .sdata2
 *   0x80795E60-0x80795E64.  Both edges are tudiscover's weak codegen-fingerprint cuts and `.data` fragment
 *   boundaries; the 0x800810DC-0x800813B8 tail is here because fn_80081120 reads the "ScnMdlSimple" name record
 *   (lbl_8056F688).
 * NAMES. Map stems.  The pool's asserts name the members: `ScnMdlSimple::SetAnmObj` ("does not 'Bind' AnmObjChr
 *   now"), `mpAnmObjChr`, `mpAnmObjVis`, `mpAnmObjMatClr`, `mpAnmObjTexPat`, `mpAnmObjTexSrt`, `mdl.IsValid()`.
 *   `TheBeatMatchOutput` (0x80081260) is a runtime-dump name, likely wrong (dump names have been contradicted
 *   before); it reads lbl_8058F4FC in `g3d/g3d_scnobj.cpp`'s `.data` fragment.
 * RESIDUALS. All 49 functions unwritten (objdiff scores them zero); flipcheck: the object emits no section.
 */

#include "types.h"
