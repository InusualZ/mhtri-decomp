/*
 * nw4r g3d: g3d_scnmdlsmpl.cpp - the ScnMdlSimple scene-model object.
 * `.text` 0x8007F0E4-0x800813B8 (49 functions, 8916 B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal `8007C540` - this
 * unit is one of the four original translation units that proposal's range turned out to span.
 *
 * Name - evidence class 1, a `__FILE__` string: the unit's `.data` fragment (0x8058F0A0-0x8058F3D8)
 * opens on `g3d_scnmdlsmpl.cpp` (lbl_8058F0A0, referenced by fn_8007F0E4 at 0x8007F6xx, by
 * fn_80080064 and by dtor_80080F7C) and carries the class's own assert texts
 * ("ScnMdlSimple::SetAnmObj does not 'Bind' AnmObjChr now. ...", `!mpAnmObjChr`, `!mpAnmObjVis`,
 * `!mpAnmObjMatClr`, `!mpAnmObjTexPat`, `!mpAnmObjTexSrt`, `mdl.IsValid()`).  Module `g3d` (the
 * neighbouring units are all nw4r g3d), C++ (the file name is a `.cpp`; the bodies call the C++
 * `Panic` mangling).
 *
 * Seams.  Left: 0x8007F0E4, tudiscover's weak `codegen fingerprint change` cut and the extent of this
 * unit's proven match set (43 functions, "certainly one TU", anchored by the bare source name
 * `g3d_scnmdlsmpl.cpp`).  Right: 0x800813B8, the start of g3d/g3d_scnobj.cpp - also the
 * `codegen fingerprint change` cut and the boundary between the two units' `.data` fragments
 * (`g3d_scnobj.cpp` starts the next one at 0x8058F3D8).
 *
 * The 0x800810DC-0x800813B8 tail (7 functions) is allocated to this unit: its head function
 * fn_80081120 is the ScnMdlSimple name-record reader (`return *fn_800638B8(&local, lbl_8056F688)`
 * with lbl_8056F688 = "ScnMdlSimple"), which only the TU defining ScnMdlSimple can register.  The
 * extab/extabindex/text partition is contiguous with that allocation.  One caveat is recorded as a
 * residual: the 0x80081260 body (map name `TheBeatMatchOutput`, a runtime-dump name this project has
 * seen contradicted before) reads lbl_8058F4FC, which sits in the *next* unit's `.data` fragment -
 * consistent with a shared global record, but it is the one reference that does not fall in this
 * unit's own data.
 *
 * Sections claimed: `.text` 0x8007F0E4-0x800813B8, `extab` 0x80008674-0x800087D4,
 * `extabindex` 0x80020FB8-0x80021150.  The `.data` fragment 0x8058F0A0-0x8058F3D8 is not claimed
 * (docs/plan.md 8.4: a range is claimed when the object emits it); it is recorded for the data pass.
 *
 * rule 7 deferred: the symbol map has only `fn_XXXXXXXX` for this range except for the dump's
 * `TheBeatMatchOutput` (checked with `python tools/symbols/dumpmap.py lookup <addr>` over the range
 * and by reading every `.text` entry in 0x8007F0E4..0x800813B8 out of config/RMHE08/symbols.txt).
 *
 * Reconstruction status: registration only - the bodies are the next pass's work; the header records
 * the home and the evidence so that pass does not have to re-derive them.  Per-unit scores are in the
 * outbox.
 */

#include "types.h"
