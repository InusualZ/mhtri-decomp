/*
 * include/NWC24/nwc24_io.h - the NWC24 device/utility half (`src/NWC24/nwc24_io.c`, `.text`
 * 0x8051E068..0x8051E864).
 *
 * Rule 2: this unit owns the `/dev/net/kd/*` fd + ioctl wrappers, the user-id CRC/unscramble pair,
 * the RTC pair and the shutdown pair, so `NWC24/nwc24_msg.c` includes this header instead of
 * declaring them.  The device-path literals it opens have no owner and live in
 * `include/unsplit/NWC24.h`, which this file includes.
 */
#ifndef MHTRI_NWC24_NWC24_IO_H
#define MHTRI_NWC24_NWC24_IO_H

#include "types.h"
#include "unsplit/NWC24.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the descriptor wrappers: `handle` is the caller's own name string (the error report) */
int NWC24iOpenFd(u32 handle, const char* path, s32* fd, u32 mode);
int NWC24iCloseFd(u32 handle, s32 fd);
int NWC24iIoctl(u32 handle, s32 fd, u32 command, u32* in, u32 inLen, u32* out, u32 outLen);
int NWC24iIoctlAsync(u32 handle, s32 fd, u32 command, u32* in, u32 inLen, u32* out,
                     u32 outLen, u32* userData);
u32 NWC24iIsAsyncIoctlBusy(void);
int NWC24iAsyncIoctlCallback(u32 value, u32* out);

/* the user-id pair and the RTC pair */
int NWC24iCheckUserIdCRC(void);
u64 getUnScrambleId(void);
int NWC24iSetRtcCounter(u32 value, u32 flag);
int NWC24iSynchronizeRtcCounter(void);

/* the shutdown pair */
int NWC24iPrepareShutdown(void);
int NWC24iRequestShutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NWC24_NWC24_IO_H */
