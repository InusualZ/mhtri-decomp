/*
 * EXI2_Reserve.h - the declaration of `EXI2_Reserve`, owned by `VF/vf.cpp`.
 */
#ifndef VF_EXI2_RESERVE_H
#define VF_EXI2_RESERVE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80522660 - 4-byte stub called by `gdev_cc_post_stop`. */
void EXI2_Reserve(void);

#ifdef __cplusplus
}
#endif

#endif
