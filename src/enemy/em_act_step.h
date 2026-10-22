/* The declarations `enemy/em_act_step.cpp` owns: the user-data accessor class whose table ends its `.data`. */
#ifndef MHTRI_ENEMY_EM_ACT_STEP_H
#define MHTRI_ENEMY_EM_ACT_STEP_H

#include "types.h"
#include "enemy/res_user_data_ac.h" /* ResUserDataAc (rule 1) */

#ifdef __cplusplus
/* The user-data accessor this unit's constructor builds (`.data` 0x805E04E0 is its table): the base accessor with
 * its own destructor, attach hook and matrix item hook - the range's own tail (0x8032FFEC..0x80330128), still
 * unwritten except the destructor.  The class name is a GUESS from the unit.  size: 0xC */
class EmActStepUserData : public ResUserDataAc {
public:
    EmActStepUserData();
    virtual ~EmActStepUserData();
    virtual void Attach();
    virtual void CalcMtxItemEx(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj, u32 node, UserDataItem* item);
};
#endif

#endif /* MHTRI_ENEMY_EM_ACT_STEP_H */
