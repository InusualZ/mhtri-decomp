/* NAND/QUATMtx.h - the quaternion helpers `NAND/nand.c` owns (0x804C6D70 and 0x804C6F40; docs/plan.md 6.5 rule 2,
 *   leaf header).  QUATMtx and QUATSlerp are GUESSES (the SDK's quaternion-from-matrix and spherical interpolation). */
#ifndef MHTRI_NAND_QUATMTX_H
#define MHTRI_NAND_QUATMTX_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804C6D70 - the quaternion of the matrix; `pMtx` is the address `mtx34_const_ptr` hands back. */
void QUATMtx(nw4r::math::QUAT* r, u32 pMtx);

/* 0x804C6F40 - the spherical interpolation of `p` towards `q` by `t` into `r`. */
void QUATSlerp(const nw4r::math::QUAT* p, const nw4r::math::QUAT* q, nw4r::math::QUAT* r, f32 t);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NAND_QUATMTX_H */
