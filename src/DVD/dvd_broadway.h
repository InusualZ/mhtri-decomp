/*
 * DVD/dvd_broadway.h - declarations of the symbols owned by `DVD/dvd_broadway.c` that other units call: the DI
 * (disc interface) command layer. A command is queued and `callback` runs with its result.
 */
#ifndef DVD_DVD_BROADWAY_H
#define DVD_DVD_BROADWAY_H

#include "types.h"
#include "DVD/dvd.h"
#include "ESP/esp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Called with the result of a queued DI command (1 when it succeeded). */
typedef void (*DVDLowCallback)(u32 result);

/* 0x804AC350 - closes the DI device; FALSE when the close failed. */
BOOL DVDLowClose(void);

/* 0x804AC3A0 - opens the DI device and starts its callbacks. */
void DVDLowInit(void);

/* 0x804AC610 - reads the disc id into `diskId`. */
BOOL DVDLowReadDiskID(DVDDiskID* diskId, DVDLowCallback callback);

/* 0x804AC7A0 - opens the partition at `offset` (in 4-byte units); ticket and certificates are optional, `tmd` receives the TMD. */
/* untyped: byte range the caller owns (ticket and certificates) */
BOOL DVDLowOpenPartition(u32 offset, void* ticket, u32 certsLength, void* certs, void* tmd, DVDLowCallback callback);

/* 0x804ACA10 - opens the partition at `offset` with an explicit TMD and ticket view. */
/* untyped: byte range the caller owns (TMD and certificates) */
BOOL DVDLowOpenPartitionWithTmdAndTicketView(u32 offset, ESTicketView* view, u32 tmdSize, void* tmd, u32 certsLength, void* certs,
                                             DVDLowCallback callback);

/* 0x804AD130 - closes the open partition. */
void DVDLowClosePartition(DVDLowCallback callback);

/* 0x804AD2A0 - reads `length` bytes at `offset` (in 4-byte units) without decryption. */
/* untyped: byte range the read fills */
BOOL DVDLowUnencryptedRead(void* buffer, u32 length, u32 offset, DVDLowCallback callback);

#ifdef __cplusplus
}
#endif

#endif
