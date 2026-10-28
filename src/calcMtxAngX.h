/* calcMtxAngX.h - leaf header (docs/plan.md 6.5 rule 2) for `calcMtxAngX`, which `fn_8004CAD8.cpp` owns, with the
 * owner's signature. */
#ifndef MHTRI_CALCMTXANGX_H
#define MHTRI_CALCMTXANGX_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x80050DE4 (0x7C): the 16-bit pitch angle of the matrix's Z axis. */
u16 calcMtxAngX(nw4r::math::MTX34* mtx);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_CALCMTXANGX_H */
