/*
 * g3d/g3d_scnroot.cpp - nw4r g3d `ScnRoot` scene-graph root (camera, fog, light, the draw buffers and the scnMdl
 *   list).
 * RANGE. .text 0x800827E4-0x8008452C (45 functions); extab, extabindex, .data 0x8058F530-0x8058F750 (opens on
 *   "g3d_scnroot.cpp"), .sdata 0x80791228-0x80791238, .sdata2 0x80795E68-0x80795E70.  The right edge is a byte cap:
 *   the "ScnRoot" name record (lbl_8056F6D0) is read by fn_8008452C and fn_80084600 in `g3d/g3d_state.cpp`, so the
 *   TU likely runs past 0x8008452C.
 * NAMES. Map stems, plus `nw4r::g3d::ScnRoot::GetCamera(int)` (0x80082B70) and `ScnRoot::SetCurrentCamera(int)`
 *   (0x80082C08).  The pool's asserts name `bufOpa`/`bufXlu` and `0 <= camID && camID < NUM_CAMERA`.
 * RESIDUALS. All 45 functions unwritten (objdiff scores them zero); flipcheck: the object emits no section.
 */

#include "types.h"
