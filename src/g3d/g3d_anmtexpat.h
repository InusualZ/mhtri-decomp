/* g3d/g3d_anmtexpat.h - the cross-unit declarations of `g3d/g3d_anmtexpat.cpp` (C linkage): the checked
 *   `ResAnmTexPat` resolver `g3d/g3d_resfile.cpp` calls and the texture-pattern result applier. */
#ifndef MHTRI_G3D_G3D_ANMTEXPAT_H
#define MHTRI_G3D_G3D_ANMTEXPAT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006993C - the checked `ResAnmTexPat` resource resolver (a `ResFile` revision-check getter). */
u32 fn_8006993C(void* p);
/* 0x80069BD8 / 0x80069C14 - `ResPltt`'s and `ResTex`'s copy constructors: copy the handle word from `src` and return
 * `self`. */
/* untyped: opaque handle - the destination handle, as the owner's definition spells it */
void* res_pltt_copy_ctor(void* self, void* src);
/* untyped: opaque handle - the destination handle, as the owner's definition spells it */
void* res_tex_copy_ctor(void* self, void* src);

#ifdef __cplusplus
}

struct ResHandle;
namespace nw4r { namespace g3d { struct TexPatAnmResult; } }

/* 0x800699C8 (0x210): applies a texture-pattern result to a material's texture and palette objects. */
extern "C" void apply_tex_pat_anm_result(struct ResHandle texObj, struct ResHandle tlutObj,
                                         const nw4r::g3d::TexPatAnmResult* pResult);
#endif

#endif /* MHTRI_G3D_G3D_ANMTEXPAT_H */
