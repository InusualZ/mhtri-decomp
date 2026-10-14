/*
 * RVLGX/GXFifo.h - declarations of the symbols owned by RVLGX/GXFifo.c that other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_RVLGX_GXFIFO_H
#define MHTRI_RVLGX_GXFIFO_H

#include "types.h"

/* The SDK's opaque FIFO descriptor (`GXInitFifoBase`, `GXSetCPUFifo`, `GXSetGPFifo`); GXFifo.c reads it through its
 * own view. size: 0x80 */
typedef struct _GXFifoObj {
    /* +0x00 */ u8 pad_0x00[0x80];
} GXFifoObj;

/* The callback the CP breakpoint interrupt calls (`GXSetBreakPtCallback`). */
typedef void (*GXBreakPtCallback)(void);

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804B45C0 (0x8C): fills a FIFO descriptor for the buffer `base`..`base + size` and resets its pointers. */
void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size); /* untyped: byte range */
/* 0x804B4750 (0x1E0): makes `fifo` the CPU write FIFO (NULL detaches it). */
void GXSetCPUFifo(const GXFifoObj* fifo);
/* 0x804B4930 (0x290): makes `fifo` the GP read FIFO (NULL detaches it). */
void GXSetGPFifo(const GXFifoObj* fifo);
/* 0x804B4BD0 (0x44): installs the CP breakpoint callback and returns the previous one. */
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback cb);
/* 0x804B4C20 (0x7C): installs the CP interrupt handler and clears both FIFO records. */
void __GXFifoInit(void);

/* 0x804B4BC0 (0x8): returns whether a GP FIFO is attached. */
u8 __GXIsGPFifoReady(void);
/* 0x804B4CA0 (0x1A0): resets the attached GP FIFO pointers to its base. */
void __GXCleanGPFifo(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_RVLGX_GXFIFO_H */
