/* BTE/gap_api.h - the GAP control block and the entry point `BTE/gap_api.c` owns (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_BTE_GAP_API_H
#define MHTRI_BTE_GAP_API_H

#include "BTE/gap_int.h"

#ifdef __cplusplus
extern "C" {
#endif

/* .bss 0x8073BE60 - the GAP control block (GUESS name gap_cb, the BTE source's). */
extern tGAP_CB gap_cb;

/* 0x8048C294 - clears the control block, installs the two BTM completion callbacks and starts the connection layer. */
void GAP_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_BTE_GAP_API_H */
