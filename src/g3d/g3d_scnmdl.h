/* g3d/g3d_scnmdl.h - the cross-unit declarations of `g3d/g3d_scnmdl.cpp`. */
#ifndef MHTRI_G3D_G3D_SCNMDL_H
#define MHTRI_G3D_G3D_SCNMDL_H

#include "types.h"
#include "nw4r/math.h"
#include "ef/pRoot.h"

/* The unit's cross-unit declarations (rule 2). */
#ifdef __cplusplus
extern "C" {
#endif

/* 0x8056F678 - the "ScnMdl" type-name record (`.rodata`: a length word, then the NUL-terminated name) the
 * class's run-time type members read. */
extern u8 scn_typename_ScnMdl[];
void g3d_root_model_bind(s32 root, u32 id);
/* 0x8007D404 - the `ResMdlInfo` handle's block (asserting the handle is valid). */
u32 res_mdl_info_ref(const void* pInfo); /* untyped: opaque handle - the info handle */
/* The tev, texture-coordinate-generator, pixel-engine, misc and gen-mode blocks' `EndEdit` hooks, each on the
 * block's one-word handle (the first three store the display list back, the last two are empty). */
void res_tev_end_edit(struct ResHandle* pSelf);
void res_mat_tex_coord_gen_end_edit(struct ResHandle* pSelf);
void res_mat_pix_end_edit(struct ResHandle* pSelf);
void res_mat_misc_end_edit(struct ResHandle* pSelf);
void res_gen_mode_end_edit(struct ResHandle* pSelf);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ScnMdl; } }

/* 0x8007E498 (0x364): copies the flagged blocks of material `matID` into the model's replacement buffers. */
extern "C" void scn_mdl_clean_mat_buffer(nw4r::g3d::ScnMdl* pMdl, u32 matID, u32 option);
#endif

#endif /* MHTRI_G3D_G3D_SCNMDL_H */
