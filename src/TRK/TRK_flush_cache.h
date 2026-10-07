/*
 * TRK/TRK_flush_cache.h - declarations of the symbols owned by `TRK/TRK_flush_cache.cpp` that other units call or read.
 */
#ifndef TRK_TRK_FLUSH_CACHE_H
#define TRK_TRK_FLUSH_CACHE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern f64 pow(f64 x, f64 y);
/* untyped: cache-line range start */
void TRK_flush_cache(void* addr, u32 len);

#ifdef __cplusplus
}
#endif

#endif
