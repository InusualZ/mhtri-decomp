/* BTE/hcisu_h2.h - the H2 (USB) HCI services transport `BTE/hcisu_h2.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_HCISU_H2_H
#define MHTRI_BTE_HCISU_H2_H

#include "types.h"
#include "BTE/bte_hcisu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .rodata 0x80573530 - the H2 transport's entry points, the table BTA_Init installs. */
extern const tHCISU_IF hcisu_h2_interface;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_HCISU_H2_H */
