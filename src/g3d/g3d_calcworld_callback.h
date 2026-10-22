/* g3d/g3d_calcworld_callback.h - nw4r g3d's `ICalcWorldCallback`, the world-matrix callback interface a model runs
 *   per node; `g3d/g3d_calcworld.cpp` dispatches it and the game's model users derive from it.  A header of its
 *   own so a derived class can name it without the scene-model declarations of `g3d/g3d_scnmdlsmpl.h`.
 */
#ifndef MHTRI_G3D_G3D_CALCWORLD_CALLBACK_H
#define MHTRI_G3D_G3D_CALCWORLD_CALLBACK_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class ResMdl;
struct ChrAnmResult;
struct WorldMtxManip;
struct FuncObjCalcWorld;

/* The world-matrix callback a model runs for one node (its interface is implemented by the game).  size: 0x4 */
class ICalcWorldCallback {
public:
    /* Out of line in every retail user (the kept copy is `sound/mhchar.cpp`'s 0x800E3B2C). */
    ICalcWorldCallback();
    virtual ~ICalcWorldCallback() {}
    /* +0x0C/+0x10/+0x14 of the table: before a node's matrix (its animation result), after it (its matrix, scale
     * and attribute word), and after the whole tree; `g3d/g3d_calcworld.cpp` dispatches them. */
    virtual void ExecCallbackA(ChrAnmResult* pResult, ResMdl mdl, FuncObjCalcWorld* pFuncObj);
    virtual void ExecCallbackB(WorldMtxManip* pManip, ResMdl mdl, FuncObjCalcWorld* pFuncObj);
    virtual void ExecCallbackC(math::MTX34* pMtxArray, ResMdl mdl, FuncObjCalcWorld* pFuncObj);
};

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_CALCWORLD_CALLBACK_H */
