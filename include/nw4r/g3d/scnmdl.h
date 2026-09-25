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

/*
 * The 0x40-byte replacement record the ScnMdl constructor copies in, word by word, from its caller's
 * argument (`fn_8007ED58`'s 40 bytes of `lwz`/`stw` at +0x144).  The range reads only two parts of it: the
 * node-visibility byte array at +0x04 (`fn_8007E7FC` writes one byte per node, `fn_8007D47C` hands it to
 * the two visibility walkers) and the three tables at +0x34..+0x3C, which `fn_8007DD94` passes on as the
 * vertex-position/colour/tex-coord tables of the shape-blend driver `fn_8007270C`.  The rest is copied but
 * never read, so it keeps its offset as unused space.
 */
struct ReplacementBlock {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u8* mpNodeVisible;
    /* +0x08 */ u8 unused_0x08[0x2C];
    /* +0x34 */ const void* mpVtxPosTable;
    /* +0x38 */ const void* mpClrTable;
    /* +0x3C */ const void* mpTexTable;
}; /* size: 0x40 */

struct ScnMdl { /* size: 0x188 - lower bound, an approximation (only ever used through a pointer) */
    /* The vtable the ScnMdl unit's bodies dispatch through (slots +0x08, +0x14, +0x34, +0x38).  The
     * two entries at +0x00/+0x04 are MWCC's hidden vtable words (the offset-to-top and the RTTI
     * pointer), so the first declared virtual lands at +0x08.  The twelve slots ahead of the first
     * called one are un-evidenced occupancy: they are never called and never defined, so no vtable is
     * emitted for this type.  Defined here, once, because `g3d/g3d_scnmdl.cpp` (the owner) and the
     * effect units that include this header both use it (docs/plan.md 6.5 rule 1). */
    virtual u32 mpfn_0x08(void* arg);             /* +0x08 */
    virtual u32 mpfn_0x0C();                      /* +0x0C */
    virtual u32 mpfn_0x10();                      /* +0x10 */
    virtual u32 mpfn_0x14();                      /* +0x14 */
    virtual u32 mpfn_0x18();                      /* +0x18 */
    virtual u32 mpfn_0x1C();                      /* +0x1C */
    virtual u32 mpfn_0x20();                      /* +0x20 */
    virtual u32 mpfn_0x24();                      /* +0x24 */
    virtual u32 mpfn_0x28();                      /* +0x28 */
    virtual u32 mpfn_0x2C();                      /* +0x2C */
    virtual u32 mpfn_0x30();                      /* +0x30 */
    virtual u32 mpfn_0x34(u32 handle, u32 id);    /* +0x34 */
    virtual u32 mpfn_0x38(u32 value);             /* +0x38 */

    /* The data members are the offsets `g3d/g3d_scnmdl.cpp`'s bodies touch; every gap between two of
     * them is padding, because no body in that unit reads it. */
    /* +0x04 */ u8 pad_0x04[0xE0];   /* the G3dObj/ScnObj/ScnLeaf/ScnMdlSimple base chain */
    /* +0xE4 */ u32 mResMdl;         /* the embedded ResMdl handle's resource pointer */
    /* +0xE8 */ u8 pad_0xE8[0x28];
    /* +0x110 */ u32 mNumTexMtx;     /* the counts the replacement arrays are sized from */
    /* +0x114 */ u32 mNumTexSrt;
    /* +0x118 */ u8 pad_0x118[0x10];
    /* +0x128 */ u32 mNumMat;
    /* +0x12C */ u32 mNumPixDL;
    /* +0x130 */ u32 mNumTevColorDL;
    /* +0x134 */ u32 mNumIndMtxAndScaleDL;
    /* +0x138 */ u32 mpAnmObjShp;    /* the shape animation object the destructor releases */
    /* +0x13C */ u32 mFlags;         /* bit 0: visible; bit 1: the shape-animation option (inverted) */
    /* +0x140 */ u32* mpDLBuffer;    /* the DL-buffer word array the option query indexes */
    /* +0x144 */ ReplacementBlock mReplacement; /* the record the constructor copies in from its caller */
    /* +0x184 */ u32 field_0x184;    /* the constructor's last argument; never read by this unit */

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
