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
extern const u8 scn_typename_ScnMdl[];
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
/* 0x8007BB94 (0x18): sets `mask` in material `matID`'s buffer-flag word of the ScnMdl. */
/* untyped: opaque handle passed through - the ScnMdl */
void scn_mdl_set_mat_buffer_flag(void* pMdl, s32 matID, s32 mask);

#ifdef __cplusplus
}

namespace nw4r { namespace g3d { class ScnMdl; } }

/* 0x8007C464 (0x10): marks the ScnMdl's visibility buffer for a refresh (bit 0 of +0x13C). */
extern "C" void scn_mdl_mark_vis_buffer_dirty(nw4r::g3d::ScnMdl* pSelf);

/* 0x8007E498 (0x364): copies the flagged blocks of material `matID` into the model's replacement buffers. */
extern "C" void scn_mdl_clean_mat_buffer(nw4r::g3d::ScnMdl* pMdl, u32 matID, u32 option);

/* The node-visibility accessor the game's character code builds on the stack: the model, the node and the node's
 * byte in the model's visibility buffer (nw4r's ScnMdl::CopiedVisAccess, built by scn_mdl_vis_access_ctor).
 * size: 0xC */
struct ScnMdlVisAccess {
    /* +0x00 */ nw4r::g3d::ScnMdl* mpScnMdl;
    /* +0x04 */ u32 mNodeID;
    /* +0x08 */ u8* mpVis;
};

extern "C" {
/* 0x8007C3CC (0x98): shows or hides the accessor's node; false without a model or buffer. */
bool scn_mdl_vis_access_set_visibility(ScnMdlVisAccess* pAccess, bool visible);
/* 0x8007C474 (0xCC): builds the accessor over node `nodeID` of `pMdl`. */
ScnMdlVisAccess* scn_mdl_vis_access_ctor(ScnMdlVisAccess* pAccess, nw4r::g3d::ScnMdl* pMdl, u32 nodeID);
}

/* The CopiedMatAccess block getters (C linkage: the game's effect code calls them by their plain names); declared
 * where the full ScnMdl class is visible. */
#ifdef MHTRI_NW4R_G3D_SCNMDL_H
extern "C" {
u32 scn_mdl_mat_access_get_chan(nw4r::g3d::ScnMdl::CopiedMatAccess* pAccess, bool markDirty);
u32 scn_mdl_mat_access_get_gen_mode(nw4r::g3d::ScnMdl::CopiedMatAccess* pAccess, bool markDirty);
u32 scn_mdl_mat_access_get_mat_misc(nw4r::g3d::ScnMdl::CopiedMatAccess* pAccess, bool markDirty);
u32 scn_mdl_mat_access_get_pix(nw4r::g3d::ScnMdl::CopiedMatAccess* pAccess, bool markDirty);
u32 scn_mdl_mat_access_get_tev_color(nw4r::g3d::ScnMdl::CopiedMatAccess* pAccess, bool markDirty);
}
#endif
#endif

#endif /* MHTRI_G3D_G3D_SCNMDL_H */
