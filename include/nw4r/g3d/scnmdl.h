/*
 * The nw4r `g3d::ScnMdl` material-access type the game's effect code builds on the stack.
 *
 * `__ct__Q44nw4r3g3d6ScnMdl15CopiedMatAccessFPQ34nw4r3g3d6ScnMdlUl` (the map's mangling) is
 * `nw4r::g3d::ScnMdl::CopiedMatAccess::CopiedMatAccess(ScnMdl*, unsigned long)`, so the class and its
 * nested accessor must be declared for the constructor to mangle correctly (docs/plan.md 6.5 rule 9:
 * call the owner, never the mangled spelling).  The other two accessors the effect code calls arrive
 * with the map's own `fn_XXXXXXXX` spelling, so they stay free `extern "C"` functions.
 */
#ifndef MHTRI_NW4R_G3D_SCNMDL_H
#define MHTRI_NW4R_G3D_SCNMDL_H

#include "types.h"

namespace nw4r {
namespace g3d {

struct ScnMdl { /* size: 0x04 - lower bound, an approximation (only ever used through a pointer) */
    struct CopiedMatAccess { /* size: 0x34 - lower bound, an approximation */
        /* +0x00 */ u32 handle_0x00;
        /* +0x04 */ u8 pad_0x04[0x30];

        CopiedMatAccess(ScnMdl* mdl, u32 idx);

        /* 0x8007BBAC - the material handle accessor, defined by `g3d/fn_80075DCC.cpp` (rule 2 owner). */
        u32 GetResTexSrt(bool arg1);
    };
};

}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_NW4R_G3D_SCNMDL_H */
