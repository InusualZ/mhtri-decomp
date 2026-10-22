/*
 * Declarations for the symbols `src/ef/eft026_fx.cpp` owns that other units use (docs/plan.md 6.5, rule 2).
 */
#ifndef MHTRI_EF_EFT026_FX_H
#define MHTRI_EF_EFT026_FX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80119970 */
void eft028_set(u8 kind, u8 variant, u8 id);
/* 0x80119AA8 */
void eft028_set_capsule(u8 area);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT026_FX_H */
