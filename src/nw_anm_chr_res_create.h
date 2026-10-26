/* Leaf header (docs/plan.md 6.5 rule 2): the g3d object creators and the inline `FrameCtrl` setter copy that
 * `nw_resource.cpp` owns (the names are registry proposals of lane sound-mhchar).  C linkage. */
#ifndef MHTRI_NW_ANM_CHR_RES_CREATE_H
#define MHTRI_NW_ANM_CHR_RES_CREATE_H

#include "types.h"

#ifdef __cplusplus
#include "g3d/g3d_anmchr.h" /* nw4r::g3d::ResAnmChr, PlayPolicyFunc (rule 1) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 1) */

struct MHcharScnWork;

extern "C" {
/* 0x800D342C (0x94): constructs a `ScnMdl` over `mdl` with buffer option `option` and returns the scene work
 * holding it. */
struct MHcharScnWork* nw_scn_mdl_create(nw4r::g3d::ResMdl mdl, u32 option, int numView);
/* 0x800D34C0 (0x3C): constructs an `AnmObjChrBlend` over `mdl` with `numChildren` children. */
nw4r::g3d::AnmObjChrBlend* nw_anm_chr_blend_create(nw4r::g3d::ResMdl mdl, int numChildren);
/* 0x800D34FC (0x48): constructs an `AnmObjChrRes` playing `anm` on `mdl`. */
nw4r::g3d::AnmObjChrRes* nw_anm_chr_res_create(nw4r::g3d::ResAnmChr anm, nw4r::g3d::ResMdl mdl, bool bHasCache);
/* 0x800D3ACC (0x124): `FrameCtrl::SetPlayPolicy` - asserts `policy` and stores it. */
void frame_ctrl_set_play_policy(nw4r::g3d::FrameCtrl* self, nw4r::g3d::PlayPolicyFunc policy);
}
#endif

#endif /* MHTRI_NW_ANM_CHR_RES_CREATE_H */
