/*
 * g3d/g3d_scnobj.cpp - nw4r g3d `ScnObj` base object and its `ScnLeaf`/`ScnGroup` subclasses.
 * RANGE. .text 0x800813B8-0x800827E4 (36 functions); extab, extabindex, .data 0x8058F3D8-0x8058F530 (opens on
 *   "g3d_scnobj.cpp"; the `!GetParent()` assert and jumptable_8058F40C/jumptable_8058F434), .sdata2
 *   0x80795E64-0x80795E68.  Both edges are tudiscover's weak cuts and `.data` fragment boundaries; the
 *   0x80082668-0x800827E4 tail is here because fn_800826AC/fn_80082714/fn_8008277C read the ScnObj/ScnLeaf/ScnGroup
 *   name records (lbl_8056F6A0/lbl_8056F6B0/lbl_8056F6C0).
 * NAMES. Map stems; the two deleting destructors are the map's `dtor_` placeholders.
 * RESIDUALS. All 36 functions unwritten (objdiff scores them zero); flipcheck: the object emits no section.
 */

#include "types.h"
