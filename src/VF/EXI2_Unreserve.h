/*
 * EXI2_Unreserve.h - the declaration of `EXI2_Unreserve`, owned by `VF/vf.cpp`.
 */
#ifndef VF_EXI2_UNRESERVE_H
#define VF_EXI2_UNRESERVE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80522664 - 4-byte stub called by `gdev_cc_pre_continue`. */
void EXI2_Unreserve(void);

#ifdef __cplusplus
}
#endif

#endif
