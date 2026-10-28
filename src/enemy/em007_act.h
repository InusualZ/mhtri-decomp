/* enemy/em007_act.h - the declarations `enemy/em007_act.cpp` owns that other units call: the per-part
 * damage/effect helpers and its shared pool entries. */
#ifndef MHTRI_ENEMY_FN_801D80EC_H
#define MHTRI_ENEMY_FN_801D80EC_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
#include "g3d/g3d_calcworld_callback.h"

struct _EFT;
struct MHchar;

/* The model copy the Cha-Cha mask effect clones its model into: a world-matrix callback that pins each node of the
 * copy to the matched node of the source model, deleted through its virtual destructor (defined here).  Its table
 * (`.data` 0x805B76B8) and ExecCallbackB (0x801E28F4) are `lobby/fn_801E0ADC.cpp`'s; ExecCallbackB is declared
 * first so the table is emitted with it.  size: 0x54 */
class Em007ModelCopy : public nw4r::g3d::ICalcWorldCallback {
public:
    Em007ModelCopy();
    virtual void ExecCallbackB(nw4r::g3d::WorldMtxManip* pManip, nw4r::g3d::ResMdl mdl,
                               nw4r::g3d::FuncObjCalcWorld* pFuncObj);
    virtual ~Em007ModelCopy();

    /* +0x04 */ struct _EFT* eft_0x04;        /* the mask effect that owns the copy */
    /* +0x08 */ struct MHchar* model_0x08;    /* the model whose world matrices the copy's nodes take */
    /* +0x0C */ u32 nodes_0x0C[18];           /* per node of the copy: the source node's matrix id, 0xFF for none */
};
#endif

/* The unit's `.sdata2` pool entries `enemy/em005_act.cpp` also reads; declared, not defined. */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801E01BC - "action 0xD with sub-state <= 5" (`enemy/em030_prog.cpp`'s team-7 checker compares the
 * answer against 1). */
u32 em007_act13_early_ck(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801D80EC_H */
