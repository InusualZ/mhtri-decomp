/*
 * Declarations for the symbols `src/BTE/bd.c` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_BTE_BD_H
#define MHTRI_BTE_BD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8047ABC4 - copies the device address `b` into `a`. */
void bdcpy(u8* a, const u8* b);

/* 0x8047ABF8 */
s32 bdcmp(const u8* a, const u8* b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_BD_H */
