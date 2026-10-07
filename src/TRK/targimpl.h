/*
 * TRK/targimpl.h - the MetroTRK target (CPU) layer entry points, owned by `TRK/targimpl.c`.
 */
#ifndef TRK_TARGIMPL_H
#define TRK_TARGIMPL_H

#include "types.h"
#include "TRK/msgbuf.h"
#include "TRK/nubevent.h"

/* The target program's saved CPU state (approximate: only the extended register block is named). */
typedef struct TRKCPUState {
    /* +0x000 */ u8 pad_0x000[0x1A8];
    /* +0x1A8 */ u32 extended1_block[97];  /* segment, time base, HID, BAT and SPR images (0x184 B) */
    /* +0x32C */ u8 pad_0x32C[0x104];
} TRKCPUState; /* size: 0x430 */

/* Flags telling TRKRestoreExtended1Block which registers to write back. */
typedef struct TRKRestoreFlags {
    /* +0x0 */ u8 restore_time_base;  /* set when the debugger changed the time base */
    /* +0x1 */ u8 restore_flag_1;     /* cleared on restore */
    /* +0x2 */ u8 pad_0x2[7];
} TRKRestoreFlags; /* size: 0x9 */

#ifdef __cplusplus
extern "C" {
#endif

extern TRKCPUState gTRKCPUState;
extern TRKRestoreFlags gTRKRestoreFlags;

/* 0x8046CF44 (0x10): records whether the target is stopped. */
void TRKTargetSetStopped(s32 stopped);

/* 0x8046C700 (0xC4): swaps the nub and target contexts and runs the target until the next interrupt. */
void TRKSwapAndGo(void);

/* 0x8046C818 (0x5C): handles a breakpoint or exception event. */
void TRKTargetInterrupt(TRKEvent* event);

/* 0x8046C874 (0x200): appends the stop description to the notification in `message`. */
s32 TRKTargetAddStopInfo(TRKBuffer* message);

/* 0x8046CA74 (0x9C): appends the exception description to the notification in `message`. */
s32 TRKTargetAddExceptionInfo(TRKBuffer* message);

/* 0x8046D368 (0x10): records the nub's input-pending flag pointer. */
void TRKTargetSetInputPendingPtr(u8* inputPendingPtr);

/* 0x8046CD40 (0x1F4): services a file or console request raised by the program. */
void TRKTargetSupportRequest(void);

/* 0x8046D418 (0x8): returns whether console I/O goes over the serial link. */
u8 GetUseSerialIO(void);

/* 0x8046CF34 (0x10): returns whether the target is stopped. */
s32 TRKTargetStopped(void);

#ifdef __cplusplus
}
#endif

#endif
