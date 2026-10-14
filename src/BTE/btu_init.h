/* BTE/btu_init.h - the BTU stack initialisation entry points `BTE/btu_init.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_BTU_INIT_H
#define MHTRI_BTE_BTU_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048C13C - initialises the device manager, L2CAP, SDP and the discovery database. */
void btu_init_core(void);

/* 0x8048C168 - clears the BTU control block and sets its defaults. */
void BTE_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTU_INIT_H */
