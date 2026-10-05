/*
 * IOS declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The four IOS entry points the `NWC24/nwc24_io.c` device layer wraps - `IOS_Open` (0x804BBE80),
 * `IOS_Close` (0x804BC190), `IOS_IoctlAsync` (0x804BC7F0) and `IOS_Ioctl` (0x804BC930) - are in an
 * address band no registered unit covers, so stylelint's rule 2 cannot resolve them to an owner's
 * header (`resolve` returns `unsplit` with no module: the nearest registered range below is
 * `EXI/ProbeBarnacle.c` and the nearest above `OS/FindContainHeap_.c`, different modules).  The
 * library is `IOS` - the SDK's internal-OS layer, carried by the symbols' own `IOS_` prefix - and no
 * `IOS/` owner exists, so this file is their home.
 *
 * Added with the networking conformance pass (the `NWC24/nwc24_io.c` registration declared all four
 * locally).  `EXI/ProbeBarnacle.c` declares three of them locally too; it can `#include` this header
 * next time that unit is touched (its `IOS_Ioctl` parameter widths differ - recorded, not changed here).
 */
#ifndef MHTRI_UNSPLIT_IOS_H
#define MHTRI_UNSPLIT_IOS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804BBE80 (0x124) - open a device path, returning a file descriptor or a negative error. */
s32 IOS_Open(const char* path, u32 mode);

/* 0x804BC190 (0xA8) - close a descriptor. */
s32 IOS_Close(s32 fd);

/* 0x804BC930 (0x130) - the synchronous ioctl. */
s32 IOS_Ioctl(s32 fd, u32 command, void* in, u32 inLen, void* out, u32 outLen);

/* 0x804BC7F0 (0x138) - the asynchronous ioctl; `callback(result, userData)` runs at completion. */
s32 IOS_IoctlAsync(s32 fd, u32 command, void* in, u32 inLen, void* out, u32 outLen,
                   void (*callback)(u32, u32*), void* userData);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_IOS_H */
