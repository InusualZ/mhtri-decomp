/*
 * nw4r/ut_LockedCache.h - nw4r::ut::LC, the locked-cache wrappers `nw4r/ut_LockedCache.cpp` defines.
 */
#ifndef NW4R_UT_LOCKEDCACHE_H
#define NW4R_UT_LOCKEDCACHE_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace ut {

/* The locked (scratch) data cache, guarded by one mutex. */
namespace LC {

void Enable();
void Disable();
bool Lock();
void Unlock();
void LoadBlocks(void* dst, void* src, u32 blocks); /* untyped: byte range */
void LoadData(void* dst, void* src, u32 size);     /* untyped: byte range */
void StoreBlocks(void* dst, void* src, u32 blocks); /* untyped: byte range */
void StoreData(void* dst, void* src, u32 size);     /* untyped: byte range */

}  // namespace LC

}  // namespace ut
}  // namespace nw4r
#endif

#endif
