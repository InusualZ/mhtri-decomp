/* g3d/g3d_state.h - the texture/state helpers of `g3d/g3d_state.cpp` that `g3d/g3d_resmat.cpp` resolves through,
 *   and the resource-range store `g3d/g3d_resfile.cpp` calls (C linkage). */
#ifndef MHTRI_G3D_G3D_STATE_H
#define MHTRI_G3D_G3D_STATE_H

#include "types.h"
#include "nw4r/math.h"

/* The render-mode record `g3d_state_get_render_mode` returns; only used through a pointer. */
struct RenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif


/* 0x80088584 - returns the render mode the state keeps (caller: g3d_camera.cpp). */
struct RenderModeObj* g3d_state_get_render_mode(void);
/* 0x80088590 - forgets the cached state the `flag` bits select (callers: g3d/fn_80075DCC.cpp,
 * sound/fn_800E3CBC.cpp). */
void g3d_state_invalidate(u32 flag);

/* 0x800868A0 - the pipe-command writer (callers: gx/fn_8009AA78.c, gx/fn_8009ACE4.c). */
void fn_800868A0(u32 value);

/* 0x8079124C - the `.sdata` word the unit hands back the address of. */
extern u32 lbl_8079124C;

#ifdef __cplusplus
}
#endif

/* More of the unit's cross-unit declarations (rule 2). */
#include "g3d/mtx34_inverse.h"

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
class ResMatTexCoordGen;
class ResMatTevColor;
}  // namespace g3d
}  // namespace nw4r

struct G3dIndMtxOp;
class G3dIndMtxCallback;
struct Mat33;

/* The material loaders `g3d/fn_80075DCC.cpp`'s draw path calls (C linkage: the map's names are plain). */
extern "C" {
void g3d_state_set_mat_misc(nw4r::g3d::ResMatMisc misc);
void g3d_state_load_tex_obj(nw4r::g3d::ResTexObj texObj);
void g3d_state_load_tlut_obj(nw4r::g3d::ResTlutObj tlutObj);
void g3d_state_set_gen_mode(nw4r::g3d::ResGenMode genMode);
void g3d_state_load_tev(nw4r::g3d::ResTev tev);
void g3d_state_load_mat_pix(nw4r::g3d::ResMatPix pix);
void g3d_state_load_mat_tev_color(nw4r::g3d::ResMatTevColor tevColor);
void g3d_state_load_mat_ind_mtx_dl(nw4r::g3d::ResMatIndMtxAndScale ind);
void g3d_state_load_mat_ind_mtx(nw4r::g3d::ResMatIndMtxAndScale ind, G3dIndMtxCallback* pCallback);
void g3d_ind_mtx_op_load(G3dIndMtxOp* pSelf);
void g3d_tex_coord_scale_load(struct StatePairTable* pSelf, u8 count);
void g3d_state_load_tex_coord_gen(nw4r::g3d::ResMatTexCoordGen texCoordGen);
Mat33* g3d_state_get_nrm_mtx(u32 idx);
G3dIndMtxCallback* g3d_state_get_ind_mtx_hook(void);
nw4r::math::MTX34* g3d_state_get_camera_mtx(void);
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_H */
