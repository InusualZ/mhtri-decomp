/*
 * DVD/dvderror.h - declarations of the symbols owned by `DVD/dvderror.c` that other units use: the error record the
 * driver fills and the call that stores an error code in it.
 */
#ifndef DVD_DVDERROR_H
#define DVD_DVDERROR_H

#include "types.h"
#include "DVD/dvd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One entry of the ring of recent drive commands. */
typedef struct DVDCommandInfo {
    /* +0x00 */ u32 command; /* the drive command issued */
    /* +0x04 */ u32 param1;  /* its first argument */
    /* +0x08 */ u32 param2;  /* its second argument */
    /* +0x0C */ u32 intType; /* the interrupt type it ended with */
    /* +0x10 */ u32 tick;    /* the bus tick it was issued at */
} DVDCommandInfo; /* size: 0x14 */

/* The error record kept in NAND: the disc, the error and the last five drive commands. */
typedef struct DVDErrorInfo {
    /* +0x00 */ char gameName[4];               /* copied from the disc id at 0x80000000 */
    /* +0x04 */ u8 diskNumber;
    /* +0x05 */ u8 gameVersion;
    /* +0x06 */ u8 pad_0x06[2];
    /* +0x08 */ u32 error;                      /* the stored error code */
    /* +0x0C */ u32 dateTime;                   /* the time of the error in seconds */
    /* +0x10 */ u32 status;                     /* the status register at the error */
    /* +0x14 */ u32 control;                    /* the control register at the error */
    /* +0x18 */ u32 unused_0x18;
    /* +0x1C */ DVDCommandInfo lastCommand[5];  /* ring of the most recent commands */
} DVDErrorInfo; /* size: 0x80 */

/* 0x80746880 - the error record the driver fills before storing it. */
extern DVDErrorInfo __ErrorInfo;

/* 0x804ABC70 - stores `error` in the error record and calls `callback` when the record is written. */
void __DVDStoreErrorCode(u32 error, DVDCBCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* DVD_DVDERROR_H */
