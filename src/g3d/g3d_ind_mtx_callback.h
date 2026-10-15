/* g3d/g3d_ind_mtx_callback.h - the indirect-matrix operation and callback `g3d/g3d_state.cpp` defines (the members and
 *   the vtable) and `g3d/g3d_draw.cpp` / `g3d/g3d_draw1mat1shp.cpp` call through. */
#ifndef MHTRI_G3D_G3D_IND_MTX_CALLBACK_H
#define MHTRI_G3D_G3D_IND_MTX_CALLBACK_H

#include "types.h"
#include "gx.h"            /* _GXIndTexMtxID */
#include "nw4r/math.h"
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMatMiscData::IndirectMethod */

/* The indirect-matrix operation a material's `ResMatIndMtxAndScale` fills: a flag bit per matrix that is set,
 * and the three matrices.  size: 0x94 */
struct G3dIndMtxOp {
    /* +0x00 */ u32 flags;
    /* +0x04 */ nw4r::math::MTX34 mtx[3];
};

/* The callback the material's indirect matrices are offered to before they load: `Exec` adds the callback's
 * matrices to the operation, `Reset` drops them and `SetNrmMapMtx` builds one from a light.  size: 0x4 */
class G3dIndMtxCallback {
public:
    G3dIndMtxCallback();
    virtual void Exec(struct G3dIndMtxOp* pOp) = 0;
    virtual ~G3dIndMtxCallback();
    virtual void Reset() = 0;
    virtual void SetNrmMapMtx(_GXIndTexMtxID id, const nw4r::math::VEC3* pLightDir,
                              const nw4r::math::MTX34* pNrmMtx,
                              nw4r::g3d::ResMatMiscData::IndirectMethod method) = 0;
};

#endif /* MHTRI_G3D_G3D_IND_MTX_CALLBACK_H */
