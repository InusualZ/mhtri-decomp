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
int NWC24iCheckUserIdCRC(u64 userId);
u64 getUnScrambleId(u64 id);
int NWC24iSetRtcCounter(u32 value, u32 flag);

/* `flag` is passed straight on to `NWC24iSetRtcCounter`'s second argument; the band's only caller
 * (`__OSInitNet`, 0x804D67C4) passes 0, so what the device does with a non-zero value is not visible
 * from this image - the parameter name is a GUESS. */
int NWC24iSynchronizeRtcCounter(u32 flag);

/* the shutdown pair: `NWC24iPrepareShutdown` is called once at OS bring-up, and
 * `NWC24iRequestShutdown` is the OS shutdown handler registered against it (`final`/`event` are the
 * arguments the OS hands every registered handler). */
int NWC24iPrepareShutdown(void);
BOOL NWC24iRequestShutdown(BOOL final, u32 event);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `include/unsplit/NWC24.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

extern const char Nwc24TimePath[];         /* 0x806311C0 "/dev/net/kd/time" */

extern const char Nwc24SetRtcName[];       /* 0x806311D4 "NWC24iSetRtcCounter" */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NWC24_NWC24_IO_H */
