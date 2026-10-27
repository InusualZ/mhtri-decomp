/*
 * Pl/pl_joint_body.h - the player body sub-object `_PLW::physics_0x13C` points at, as the hit and push units read it: its
 *   `MHchar` block sits at +0x04 (the layout `ef/fn_80114E34.cpp` owns as `_PLW_PHYSICS` in its own source; this is the
 *   Pl band's view of it, shared by `Pl/pl_body_push.cpp` and `Pl/pl_hit.cpp`, rule 1).
 */
#ifndef MHTRI_PL_PL_JOINT_BODY_H
#define MHTRI_PL_PL_JOINT_BODY_H

#include "types.h"
#include "pl.h"

/* size: 0x144 (lower bound: the `MHchar` it holds is 0x140) */
struct PlJointBody {
    /* +0x000 */ u8 pad_0x000[0x4];
    /* +0x004 */ MHchar chr_0x04;
};

#endif /* MHTRI_PL_PL_JOINT_BODY_H */
