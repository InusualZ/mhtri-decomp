/* g3d/g3d_state.h - the texture/state helpers of `g3d/g3d_state.cpp` that `g3d/g3d_resmat.cpp` resolves through,
 *   and the resource-range store `g3d/g3d_resfile.cpp` calls (C linkage). */
#ifndef MHTRI_G3D_G3D_STATE_H
#define MHTRI_G3D_G3D_STATE_H

#include "types.h"
#include "nw4r/math.h"

/* The render-mode record `fn_80088584` returns; only used through a pointer. */
struct RenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif


/* 0x80088584 - the render-mode helper (caller: g3d_camera.cpp). */
struct RenderModeObj* fn_80088584(void);

/* 0x800868A0 - the pipe-command writer (callers: gx/fn_8009AA78.c, gx/fn_8009ACE4.c). */
void fn_800868A0(u32 value);

/* 0x8079124C - the `.sdata` word the unit hands back the address of. */
extern u32 lbl_8079124C;

#ifdef __cplusplus
}
#endif

/* More of the unit's cross-unit declarations (rule 2). */
#ifdef __cplusplus
extern "C" {
#endif

void mtx34_inverse(Mtx34* out, const Mtx34* src);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class ResMatMisc;
class ResTexObj;
class ResTlutObj;
class ResGenMode;
class ResTev;
class ResMatPix;
class ResMatIndMtxAndScale;
}  // namespace g3d
}  // namespace nw4r

struct G3dIndMtxOp;
class G3dIndMtxCallback;

/* The material loaders `g3d/fn_80075DCC.cpp`'s draw path calls (C linkage: the map's names are plain). */
extern "C" {
void g3d_state_set_mat_misc(nw4r::g3d::ResMatMisc misc);
void g3d_state_load_tex_obj(nw4r::g3d::ResTexObj texObj);
void g3d_state_load_tlut_obj(nw4r::g3d::ResTlutObj tlutObj);
void g3d_state_set_gen_mode(nw4r::g3d::ResGenMode genMode);
void g3d_state_load_tev(nw4r::g3d::ResTev tev);
void g3d_state_load_mat_pix(nw4r::g3d::ResMatPix pix);
void g3d_state_load_mat_ind_mtx_dl(nw4r::g3d::ResMatIndMtxAndScale ind);
void g3d_state_load_mat_ind_mtx(nw4r::g3d::ResMatIndMtxAndScale ind, G3dIndMtxCallback* pCallback);
void g3d_ind_mtx_op_load(G3dIndMtxOp* pSelf);
void g3d_tex_coord_scale_load(struct StatePairTable* pSelf, u8 count);
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_H */
