/*
 * TRK/dolphin_trk.h - the MetroTRK Dolphin/Revolution start-up and board layer, owned by `TRK/dolphin_trk.c`.
 */
#ifndef TRK_DOLPHIN_TRK_H
#define TRK_DOLPHIN_TRK_H

#include "types.h"
#include "TRK/targimpl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80468D4C (0x94): MetroTRK entry from the start-up code: saves the register image, builds the comm table and runs the nub. */
void InitMetroTRK(void);

/* 0x80468DE4 (0x94): the BBA variant of the entry. */
void InitMetroTRK_BBA(void);

/* 0x80468E78 (0x4): enables the debugger channel's interrupts. */
void EnableMetroTRKInterrupts(void);

/* 0x80468E7C (0x68): maps a low-memory offset to the address the debugger can reach it at. */
u32 TRKTranslateAddress(u32 address);

/* 0x80468EE4 (0x134): copies the enabled exception vectors over the low-memory vectors. */
void TRKOverrideInterruptVectors(void);

/* 0x80469060 (0x10): resets the system. */
void __TRKreset(void);

/* 0x80469070 (0x88): restores the saved register image from `context` and enters the exception vector `vector`. */
void TRKLoadContext(TRKCPUState* context, u32 vector);

/* 0x804690F8 (0x38): EXI interrupt callback: re-enables the scheduler and restores the interrupted context. */
void TRKEXICallBack(s16 unused, TRKCPUState* context);

/* 0x80469130 (0x138): fills the debugger channel table for the hardware ID the OS passed; returns 0, or 1 for an unknown ID. */
s32 InitMetroTRKCommTable(s32 hardware);

/* 0x80469268 (0x4): UART interrupt handler (empty). */
void TRKUARTInterruptHandler(void);

/* 0x804692C0 (0x2C): enables the debugger channel's interrupts when it is not the BBA channel. */
void EnableEXI2Interrupts(void);

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
