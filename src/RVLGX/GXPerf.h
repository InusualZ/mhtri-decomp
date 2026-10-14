/*
 * RVLGX/GXPerf.h - declarations of the symbols owned by RVLGX/GXPerf.c that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXPERF_H
#define MHTRI_RVLGX_GXPERF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804BA9F0 (0x81C): selects the GP performance metrics of counters 0 and 1. */
void GXSetGPMetric(s32 perf0, s32 perf1);
/* 0x804BB210 (0x10): clears the GP performance counters. */
void GXClearGPMetric(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXPERF_H */
