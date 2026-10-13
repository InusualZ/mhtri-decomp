/*
 * RVLGX/GXFifo.h - declarations of the symbols owned by RVLGX/GXFifo.c that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXFIFO_H
#define MHTRI_RVLGX_GXFIFO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B4BC0 (0x8): returns whether a GP FIFO is attached. */
u8 __GXIsGPFifoReady(void);
/* 0x804B4CA0 (0x1A0): resets the attached GP FIFO pointers to its base. */
void __GXCleanGPFifo(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXFIFO_H */
