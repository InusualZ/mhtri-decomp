/*
 * nw4r/ut_LockedCache.cpp - nw4r::ut::LC, the locked (scratch) data cache wrappers and their mutex.
 * RANGE. .text 0x80502678-0x80502828 (9 functions with the static initialiser); .ctors 0x8056F3D8-0x8056F3DC;
 *   .bss 0x80760C78-0x80760C98 (sLCImpl and the section's padding).
 * RANGE. The edges with the neighbouring nw4r files (math_types .. ut_LockedCache, 0x805012C4-0x80502828) are the
 *   namespace/class runs in link order; each file pools its own `.sdata2` constants (0.0f once per file).
 * FLAGS. the `nw4r` lib block: GC/3.0a5.2 with `cflags_nw4r` (`cflags_os` + `-fp_contract off`); evidence in docs/nw4r.md.
 * NAMES. File from NW4R's ut_LockedCache.cpp. GUESS from the nw4r source, read off the bodies: LC::Enable ..
 *   LC::StoreData (the tail calls in the SDK's LCLoadBlocks/LCLoadData/LCStoreBlocks/LCStoreData order).
 * SHAPES. Lock tests `isEnabled` true-first.
 */

#include "nw4r/ut_LockedCache.h"
#include "OS/LCEnable.h"
#include "OS/OSThread.h"
#include "unsplit/OS.h"

namespace nw4r {
namespace ut {

namespace LC {

/* The locked cache's state: whether it is enabled and the mutex that guards it. size: 0x1C */
struct LCImpl {
    LCImpl() : isEnabled(false) {
        OSInitMutex(&mutex);
    }

    void Lock() {
        OSLockMutex(&mutex);
    }

    void Unlock() {
        OSUnlockMutex(&mutex);
    }

    /* +0x00 */ bool isEnabled;
    /* +0x01 */ u8 pad_0x01[0x3];
    /* +0x04 */ OSMutex mutex;
};

/* Holds the locked cache's mutex for the scope it lives in. size: 0x4 */
class AutoLock {
public:
    AutoLock(LCImpl& impl) : mMutex(&impl.mutex) {
        OSLockMutex(mMutex);
    }

    ~AutoLock() {
        OSUnlockMutex(mMutex);
    }

    /* +0x0 */ OSMutex* mMutex;
};

static LCImpl sLCImpl;

/* Waits until the cache's DMA queue is empty. */
static inline void QueueWait() {
    while (LCQueueLength() != 0) {
        OSYieldThread();
    }
}

/* 0x80502678 (0x60): enables the locked cache once. */
void Enable() {
    AutoLock lock(sLCImpl);

    if (!sLCImpl.isEnabled) {
        LCEnable();
        sLCImpl.isEnabled = true;
    }
}

/* 0x805026D8 (0x78): drains the DMA queue and disables the locked cache. */
void Disable() {
    AutoLock lock(sLCImpl);

    if (sLCImpl.isEnabled) {
        QueueWait();
        LCDisable();
        sLCImpl.isEnabled = false;
    }
}

/* 0x80502750 (0x70): takes the cache for the caller; false (and released) when it is not enabled. */
bool Lock() {
    sLCImpl.Lock();
    if (sLCImpl.isEnabled) {
        QueueWait();
        return true;
    } else {
        sLCImpl.Unlock();
        return false;
    }
}

/* 0x805027C0 (0x40): drains the DMA queue and releases the cache. */
void Unlock() {
    QueueWait();
    sLCImpl.Unlock();
}

/* 0x80502800 (0x4): DMAs `blocks` 32-byte blocks into the locked cache. */
/* untyped: byte range */
void LoadBlocks(void* dst, void* src, u32 blocks) {
    LCLoadBlocks(dst, src, blocks);
}

/* 0x80502804 (0x4): DMAs `size` bytes into the locked cache. */
/* untyped: byte range */
void LoadData(void* dst, void* src, u32 size) {
    LCLoadData(dst, src, size);
}

/* 0x80502808 (0x4): DMAs `blocks` 32-byte blocks out of the locked cache. */
/* untyped: byte range */
void StoreBlocks(void* dst, void* src, u32 blocks) {
    LCStoreBlocks(dst, src, blocks);
}

/* 0x8050280C (0x4): DMAs `size` bytes out of the locked cache. */
/* untyped: byte range */
void StoreData(void* dst, void* src, u32 size) {
    LCStoreData(dst, src, size);
}

}  // namespace LC

}  // namespace ut
}  // namespace nw4r
