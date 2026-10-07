/*
 * TRK/nubinit.h - the MetroTRK nub bring-up and teardown, owned by `TRK/nubinit.c`.
 */
#ifndef TRK_NUBINIT_H
#define TRK_NUBINIT_H

#include "types.h"

/* The byte order probe result. */
typedef struct TRKByteOrder {
    /* +0x00 */ s32 big_endian;   /* 1 when the target is big-endian, 0 when little-endian */
    /* +0x04 */ s32 unused_0x04;  /* never read or written */
} TRKByteOrder; /* size: 0x8 */

#ifdef __cplusplus
extern "C" {
#endif

/* The communication driver's "input is waiting" flag; set from the UART interrupt path. */
extern u8* gTRKInputPendingPtr;

extern TRKByteOrder gTRKByteOrder;

/* 0x80469714 (0x74): probes the byte order into gTRKByteOrder; returns 0 or 1 when the order is unrecognised. */
s32 TRKInitializeEndian(void);

/* 0x80469638 (0xAC): brings the nub up (buffers, event queue, serial handler, target); returns 0 or an error code. */
s32 TRKInitializeNub(void);

/* 0x804696E4 (0x24): shuts the nub down; returns the shutdown result. */
s32 TRKTerminateNub(void);

/* 0x80469708 (0xC): prints the nub banner. */
void TRKNubWelcome(void);

#ifdef __cplusplus
}
#endif

#endif
