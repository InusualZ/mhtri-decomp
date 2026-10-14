/*
 * Declarations for the symbols `src/BTE/btu_init.c` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_BTE_BTU_INIT_H
#define MHTRI_BTE_BTU_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8048C13C */
void btu_init_core(void);
/* 0x8048C168 */
void BTE_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BTU_INIT_H */
