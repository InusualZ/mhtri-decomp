/* enemy/res_user_data_ac.h - the enemy user-data accessor class `ResUserDataAc` (`enemy/fn_80138074.cpp` defines
 *   its members) and the user-data item record its hooks take; every monster unit derives its own accessor.
 */
#ifndef MHTRI_ENEMY_RES_USER_DATA_AC_H
#define MHTRI_ENEMY_RES_USER_DATA_AC_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
#include "g3d/g3d_calcworld_callback.h" /* nw4r::g3d::ICalcWorldCallback (rule 1) */

struct KeyFrameSet;

/* One 0x18-byte entry of the enemy data's user-data item list (a key of -1 ends it): the node it applies to, its
 * kind (0/3/4 rotation keys, 1 a turn joint, 2 a sway joint) and the kind's operands.  size: 0x18 */
struct UserDataItem {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 pad_0x05[0x03];
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u8 field_0x10;
    /* +0x11 */ u8 pad_0x11[0x03];
    /* +0x14 */ KeyFrameSet* frames;
};

/* The enemy's world-matrix callback: the model runs ExecCallbackA/B/C per node, and the accessor hands every
 * user-data item of that node to its item hooks.  Each monster derives its own class (`Em009UserData` ...); the
 * table is `.data` 0x805A1368 (dtor, the three nw4r callbacks, then the six hooks).  size: 0xC */
class ResUserDataAc : public nw4r::g3d::ICalcWorldCallback {
public:
    ResUserDataAc();
    virtual ~ResUserDataAc();
    virtual void ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj);
    virtual void ExecCallbackB(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj);
    virtual void ExecCallbackC(nw4r::math::MTX34* pMtxArray, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj);
    /* +0x18: run once the accessor is installed on its work. */
    virtual void Attach();
    /* +0x1C/+0x20: one item of the node ExecCallbackA runs (kinds 0, 3 and 4 / every other kind). */
    virtual void CalcAnmItem(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                             nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
    virtual void CalcAnmItemEx(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
    /* +0x24/+0x28: one item of the node ExecCallbackB runs (kinds 0..3 / every other kind). */
    virtual void CalcMtxItem(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                             nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
    virtual void CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
    /* +0x2C: run once ExecCallbackC has rebuilt the matrix array. */
    virtual void CalcWorldEnd(nw4r::math::MTX34* pMtxArray, nw4r::g3d::ResMdl mdl,
                              nw4r::g3d::FuncObjCalcWorld* pFuncObj);

    /* +0x04 */ struct _ENEMY_WORK* work;
    /* +0x08 */ u32 flags;
};
#endif

#endif /* MHTRI_ENEMY_RES_USER_DATA_AC_H */
