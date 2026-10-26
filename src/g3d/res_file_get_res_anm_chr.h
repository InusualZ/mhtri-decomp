/* Leaf header (docs/plan.md 6.5 rule 2): the `ResFile` accessors `g3d/g3d_resfile.cpp` owns that the character
 * model code calls by their C names (the names are registry proposals of lane sound-mhchar).  C linkage. */
#ifndef MHTRI_G3D_RES_FILE_GET_RES_ANM_CHR_H
#define MHTRI_G3D_RES_FILE_GET_RES_ANM_CHR_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class ResFile;
class ResAnmChr;
}
}

extern "C" {
/* 0x80092B9C (0x64): `ResAnmChr(void*)` - stores the block address and asserts its alignment. */
nw4r::g3d::ResAnmChr* res_anm_chr_ctor(nw4r::g3d::ResAnmChr* self, void* pData); /* untyped: opaque handle */
/* 0x800924CC (0xB8): model `idx` of the file, as the `ResMdl` handle word. */
u32 res_file_get_res_mdl(nw4r::g3d::ResFile* self, u32 idx);
/* 0x80092C08 (0xB8): character animation `idx` of the file, as the `ResAnmChr` handle word. */
u32 res_file_get_res_anm_chr(nw4r::g3d::ResFile* self, u32 idx);
/* 0x80092CC4 (0x8C): the number of character animations in the file. */
u32 res_file_get_res_anm_chr_num(void* self); /* untyped: opaque handle - the file handle, spelled as g3d/g3d_resfile.h declares it */
/* 0x8009371C (0x78): whether the file has a character-animation category. */
u32 res_file_has_anm_chr(nw4r::g3d::ResFile* self);
/* 0x8009380C (0xE0): binds the file's models and scene animations against `file`. */
u32 res_file_bind(nw4r::g3d::ResFile* self, nw4r::g3d::ResFile* file);
/* 0x80093990 (0x108): initialises the file's models, textures and palettes. */
void res_file_init(nw4r::g3d::ResFile* self);
/* 0x80093AA0 (0x6C): terminates every model of the file. */
void res_file_terminate(nw4r::g3d::ResFile* self);
}
#endif

#endif /* MHTRI_G3D_RES_FILE_GET_RES_ANM_CHR_H */
