/*
 * DVD/dvd_broadway.h - declarations of the symbols owned by `DVD/dvd_broadway.c` that other units call: the DI
 * (disc interface) command layer. A command is queued and `callback` runs with its result.
 */
#ifndef DVD_DVD_BROADWAY_H
#define DVD_DVD_BROADWAY_H

#include "types.h"
#include "DVD/dvd.h"
#include "ESP/esp.h"
#include "OS/OSAlarm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Called with the result of a queued DI command (1 when it succeeded). */
typedef void (*DVDLowCallback)(u32 result);

/* 0x804AC350 - closes the DI device; FALSE when the close failed. */
BOOL DVDLowFinish(void);

/* 0x804AC3A0 - opens the DI device and starts its callbacks; FALSE when the device could not be set up. */
BOOL DVDLowInit(void);

/* 0x804AC610 - reads the disc id into `diskId`. */
BOOL DVDLowReadDiskID(DVDDiskID* diskId, DVDLowCallback callback);

/* 0x804AC7A0 - opens the partition at `offset` (in 4-byte units); ticket and certificates are optional, `tmd` receives the TMD. */
/* untyped: byte range the caller owns (ticket and certificates) */
BOOL DVDLowOpenPartition(u32 offset, void* ticket, u32 certsLength, void* certs, void* tmd, DVDLowCallback callback);

/* 0x804ACA10 - opens the partition at `offset` with an explicit TMD and ticket view. */
/* untyped: byte range the caller owns (TMD and certificates) */
BOOL DVDLowOpenPartitionWithTmdAndTicketView(u32 offset, ESTicketView* view, u32 tmdSize, void* tmd, u32 certsLength, void* certs,
                                             DVDLowCallback callback);

/* 0x804ACC80 (0x210): Asks the drive for the TMD and certificate sizes of the partition at `offset`. */
BOOL DVDLowGetNoDiscBufferSizes(u32 offset, u32* numTmdBytes, u32* numCertBytes, DVDLowCallback callback);

/* 0x804ACE90 (0x298): Reads the ticket, TMD, certificates, data offset and H3 table of the partition at `offset`. */
BOOL DVDLowGetNoDiscOpenPartitionParams(u32 offset, ESTicket* ticket, u32* numTmdBytes, u8* tmd, u32* numCertBytes,
                                        u8* certs, u32* dataWordOffset, u8* h3Table, DVDLowCallback callback);

/* 0x804AD130 - closes the open partition. */
BOOL DVDLowClosePartition(DVDLowCallback callback);

/* 0x804AD2A0 - reads `length` bytes at `offset` (in 4-byte units) without decryption. */
/* untyped: byte range the read fills */
BOOL DVDLowUnencryptedRead(void* buffer, u32 length, u32 offset, DVDLowCallback callback);

/* 0x804AD440 - stops the drive motor; `eject` and `saving` select the stop mode. */
BOOL DVDLowStopMotor(u32 eject, u32 saving, DVDLowCallback callback);

/* 0x804AD5E0 (0x16C): Reads the drive's inquiry record into `info`. */
BOOL DVDLowInquiry(DVDDriveInfo* info, DVDLowCallback callback);

/* 0x804AD750 - asks the drive for its error code. */
BOOL DVDLowRequestError(DVDLowCallback callback);

/* 0x804AD8C0 - records whether the next reset spins the disc up. */
BOOL DVDLowSetSpinupFlag(u32 spinUp);

/* 0x804AD8D0 - resets the drive, spinning up when the spin-up flag is set. */
BOOL DVDLowReset(DVDLowCallback callback);

/* 0x804ADA50 (0x198): Configures the drive's audio streaming buffer. */
BOOL DVDLowAudioBufferConfig(u32 enable, u32 size, DVDLowCallback callback);

/* 0x804ADBF0 - reads the 0x20-byte key report of `format` at sector `lsn` into `reportKey`. */
/* untyped: byte range the report fills */
BOOL DVDLowReportKey(void* reportKey, u32 format, u32 lsn, DVDLowCallback callback);

/* 0x804ADD90 - limits the drive's rotation speed to `subcmd`. */
BOOL DVDLowSetMaximumRotation(u32 subcmd, DVDLowCallback callback);

/* 0x804ADF20 (0x1B0): Reads `length` bytes at `offset` (in 4-byte units) into the 32-byte aligned `buffer`. */
/* untyped: byte range the read fills */
BOOL DVDLowRead(void* buffer, u32 length, u32 offset, DVDLowCallback callback);

/* 0x804AE0D0 (0x180): Moves the drive head to `offset` (in 4-byte units). */
BOOL DVDLowSeek(u32 offset, DVDLowCallback callback);

/* 0x804AE250 - the cover register value the last cover-register read stored. */
u32 DVDLowGetCoverRegister(void);

/* 0x804AE260 (0xC): Returns the status register value the last status-register read stored. */
u32 DVDLowGetStatusRegister(void);

/* 0x804AE270 (0xC): Returns the control register value the last control-register read stored. */
u32 DVDLowGetControlRegister(void);

/* 0x804AE280 - reads the cover register into the driver's copy. */
BOOL DVDLowPrepareCoverRegister(DVDLowCallback callback);

/* 0x804AE3F0 (0x16C): Reads the status register into the driver's copy. */
BOOL DVDLowPrepareStatusRegister(DVDLowCallback callback);

/* 0x804AE560 (0x16C): Reads the control register into the driver's copy. */
BOOL DVDLowPrepareControlRegister(DVDLowCallback callback);

/* 0x804AE6D0 - the immediate buffer register value the last command stored. */
u32 DVDLowGetImmBufferReg(void);

/* 0x804AE6E0 - unmasks the status interrupts; always TRUE in this driver. */
BOOL DVDLowUnmaskStatusInterrupts(void);

/* 0x804AE6F0 - masks the cover interrupt; always TRUE in this driver. */
BOOL DVDLowMaskCoverInterrupt(void);

/* 0x804AE700 - acknowledges a cover interrupt. */
BOOL DVDLowClearCoverInterrupt(DVDLowCallback callback);

/* 0x804AE870 - whether `alarm` belongs to the DI driver; always FALSE in this driver. */
BOOL DVDLowTestAlarm(const OSAlarm* alarm);

#ifdef __cplusplus
}
#endif

#endif
