/*
 * TRK/dolphin_trk.h - the MetroTRK Dolphin/Revolution start-up and board layer, owned by `TRK/dolphin_trk.c`.
 */
#ifndef TRK_DOLPHIN_TRK_H
#define TRK_DOLPHIN_TRK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804693B8 (0x54): patches the program-end trap into the halt routine. */
void InitializeProgramEndTrap(void);

/* 0x80469018 (0x48): initialises the target layer; returns 0 or an error code. */
s32 TRKInitializeTarget(void);

/* 0x8046926C (0x54): sets up the interrupt-driven UART; the driver publishes its input-pending flag through the reference. */
s32 TRKInitializeIntDrivenUART(u32 mode, u32 option, u8** inputPendingPtrRef);

/* 0x804692EC (0x14): returns the number of UART bytes waiting. */
s32 TRKPollUART(void);

/* 0x80469300 (0x3C): reads exactly `length` bytes from the UART; returns 0, or -1 when the read fails. */
/* untyped: byte range */
s32 TRKReadUARTN(void* bytes, u32 length);

/* 0x80469378 (0x14): reserves the EXI channel 2 port used by the debugger link. */
void ReserveEXI2Port(void);

/* 0x8046938C (0x14): releases the EXI channel 2 port. */
void UnreserveEXI2Port(void);

/* 0x804693A0 (0x18): shows `message` on the board display (reports it). */
void TRK_board_display(const char* message);

#ifdef __cplusplus
}
#endif

#endif
