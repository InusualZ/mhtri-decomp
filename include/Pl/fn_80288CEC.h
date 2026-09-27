/*
 * `Pl/fn_80288CEC.cpp`'s shared record (docs/plan.md 6.5 rule 1: a type more than one unit uses is
 * defined once and included where needed).  `Pl/fn_8028F66C.cpp`'s point-vs-box distance and the
 * hit tests beside it take the same record, so the definition moved here from that unit's source;
 * the box builders themselves (`fn_8028F44C`/`fn_8028F4B4`) stay in the owner.
 */
#ifndef MHTRI_PL_FN_80288CEC_H
#define MHTRI_PL_FN_80288CEC_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus

/* The 0x24-byte record the box builders fill: three 0xC-byte vectors.  `fn_80041E40` copies one
 * 0xC-byte record and `fn_80050CA0` writes `vec_0x0C - vec_0x00` into the third, so +0x00/+0x0C are
 * the two endpoints and +0x18 their difference.  size: 0x24 */
struct PlBox {
    /* +0x00 */ nw4r::math::VEC3 vec_0x00;
    /* +0x0C */ nw4r::math::VEC3 vec_0x0C;
    /* +0x18 */ nw4r::math::VEC3 vec_0x18;
};

#endif /* __cplusplus */


/* Added by `enemy/em_action.cpp` (rule 2: the declaration belongs with the owner TU, which
 * had not declared it yet). */
void fn_8028F558(void* a, void* b);

#endif /* MHTRI_PL_FN_80288CEC_H */
