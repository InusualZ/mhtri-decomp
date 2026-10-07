/*
 * MTX/mtxvec.h - the entry `MTX/mtxvec.c` owns.
 */
#ifndef MHTRI_MTX_MTXVEC_H
#define MHTRI_MTX_MTXVEC_H

#include "types.h"
#include "MTX/mtx.h"

#ifdef __cplusplus
extern "C" {
#endif

void PSMTXMultVec(const Mtx m, const Vec* src, Vec* dst);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MTX_MTXVEC_H */
