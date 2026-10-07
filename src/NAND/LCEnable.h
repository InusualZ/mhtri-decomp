/* NAND/LCEnable.h - the locked-cache entry points `NAND/nand.c` owns (docs/plan.md 6.5 rule 2, leaf header), in the
 *   SDK's OSCache order. */
#ifndef MHTRI_NAND_LCENABLE_H
#define MHTRI_NAND_LCENABLE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CC870 - enables the locked cache. */
void LCEnable(void);

/* 0x804CC8B0 - disables the locked cache. */
void LCDisable(void);

/* 0x804CC8E0 - queues a DMA of `blocks` 32-byte blocks into the locked cache. */
void LCLoadBlocks(void* dst, void* src, u32 blocks); /* untyped: byte range */

/* 0x804CC910 - queues a DMA of `blocks` 32-byte blocks out of the locked cache. */
void LCStoreBlocks(void* dst, void* src, u32 blocks); /* untyped: byte range */

/* 0x804CC940 - queues DMAs of `size` bytes into the locked cache; returns the transfer count. */
u32 LCLoadData(void* dst, void* src, u32 size); /* untyped: byte range */

/* 0x804CC9E0 - queues DMAs of `size` bytes out of the locked cache; returns the transfer count. */
u32 LCStoreData(void* dst, void* src, u32 size); /* untyped: byte range */

/* 0x804CCA80 - the number of queued locked-cache DMAs. */
u32 LCQueueLength(void);

/* 0x804CCA90 - waits until at most `length` locked-cache DMAs are queued. */
void LCQueueWait(u32 length);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NAND_LCENABLE_H */
