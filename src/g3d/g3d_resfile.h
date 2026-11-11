/* g3d/g3d_resfile.h - the cross-unit declarations of `g3d/g3d_resfile.cpp`: `nw4r::g3d::ResFile`, a one-word
 *   `ResCommon<ResFileData>` handle whose lookups `GetResTex`/`GetResPltt` return the found `ResTex`/`ResPltt` handle,
 *   and the C-linkage helpers. */
#ifndef MHTRI_G3D_G3D_RESFILE_H
#define MHTRI_G3D_G3D_RESFILE_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class ResName;
class ResTex;
class ResPltt;

class ResFile {
public:
    u32 GetResPltt(const char* pName) const;
    u32 GetResTex(const char* pName) const;
    ResPltt GetResPltt(const ResName name) const;
    ResTex GetResTex(const ResName name) const;

    /* +0x0 */ void* mpData;
}; /* size: 0x4 (a one-word `ResCommon<ResFileData>` handle) */

}  // namespace g3d
}  // namespace nw4r
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80092584..0x80093690 - the ten resource-category count/item accessors of the `ResFile` container
 * (caller: g3d_resfile.cpp): each pair is `<category> count(self)` and `<category> item(self, i)`; the item
 * feeds `ResHandle::mpData`. */
u32 fn_80092588(void* p);
void* fn_80092584(void* p, u32 i);
u32 fn_80092B10(void* p);
void* fn_80092B0C(void* p, u32 i);
u32 fn_8009284C(void* p);
void* fn_80092848(void* p, u32 i);
u32 res_file_get_res_anm_chr_num(void* p);
void* fn_80092CC0(void* p, u32 i);
u32 fn_80092E78(void* p);
void* fn_80092E74(void* p, u32 i);
u32 fn_8009302C(void* p);
void* fn_80093028(void* p, u32 i);
u32 fn_800931E0(void* p);
void* fn_800931DC(void* p, u32 i);
u32 res_file_get_res_anm_tex_srt_num(void* p);
void* res_file_get_res_anm_tex_srt(void* p, u32 i);
u32 fn_80093548(void* p);
void* fn_80093544(void* p, u32 i);
u32 fn_80093690(void* p);
void* fn_8009368C(void* p, u32 i);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_RESFILE_H */
