/*
 * TRK/targimpl.h - the MetroTRK target (CPU) layer entry points, owned by `TRK/targimpl.c`.
 */
#ifndef TRK_TARGIMPL_H
#define TRK_TARGIMPL_H

#include "types.h"
#include "TRK/msgbuf.h"
#include "TRK/nubevent.h"

/* The target program's saved CPU state (approximate: only the words the target layer reads are named). */
typedef struct TRKCPUState {
    /* +0x000 */ u32 gpr[32];             /* general registers */
    /* +0x080 */ u32 pc;                  /* program counter (SRR0 at the stop) */
    /* +0x084 */ u32 lr;
    /* +0x088 */ u32 cr;
    /* +0x08C */ u32 ctr;
    /* +0x090 */ u32 xer;
    /* +0x094 */ u8 pad_0x094[0x114];
    /* +0x1A8 */ u32 pad_0x1A8[16];       /* segment registers and the first SPR images */
    /* +0x1E8 */ u32 time_base_image[2];  /* time base words; a debugger write sets restore_time_base */
    /* +0x1F0 */ u32 pad_0x1F0[2];
    /* +0x1F8 */ u32 srr1;                /* MSR image the target resumes with */
    /* +0x1FC */ u32 pad_0x1FC[15];
    /* +0x238 */ u32 dbat3_upper;         /* DBAT3U image: bits 0-1 tell whether the locked cache is mapped */
    /* +0x23C */ u32 pad_0x23C[15];
    /* +0x278 */ u32 flagged_reg_image;   /* register image a debugger write flags in restore_flag_1 */
    /* +0x27C */ u32 pad_0x27C[31];
    /* +0x2F8 */ u32 exception_id;        /* low 16 bits: the exception vector that stopped the target */
    /* +0x2FC */ u32 pad_0x2FC[12];       /* rest of the extended block */
    /* +0x32C */ u8 pad_0x32C[0x104];
} TRKCPUState; /* size: 0x430 */

/* The nub's own register image, saved while the target runs. */
typedef struct TRKState {
    /* +0x00 */ u32 gpr[32];              /* nub general registers */
    /* +0x80 */ u32 lr;
    /* +0x84 */ u32 ctr;
    /* +0x88 */ u32 xer;
    /* +0x8C */ u32 msr;                  /* MSR captured by TRKInitializeTarget and TRKSwapAndGo */
    /* +0x90 */ u32 dar;
    /* +0x94 */ u32 dsisr;
    /* +0x98 */ s32 stopped;              /* nonzero while the target is stopped */
    /* +0x9C */ s32 input_activated;      /* set when a debugger byte arrived while the target ran */
    /* +0xA0 */ u8* input_pending_ptr;    /* the nub's input-pending flag, published by the UART driver */
} TRKState; /* size: 0xA4 */

/* The register block the exception vector stubs save before they enter the nub. */
typedef struct TRKSaveState {
    /* +0x00 */ u32 gpr[32];
    /* +0x80 */ u32 pc;
    /* +0x84 */ u32 lr;
    /* +0x88 */ u32 cr;
    /* +0x8C */ u32 ctr;
    /* +0x90 */ u32 xer;
} TRKSaveState; /* size: 0x94 */

/* The pending single-step or step-out-of-range request. */
typedef struct TRKStepState {
    /* +0x00 */ s32 active;          /* nonzero while a step is armed */
    /* +0x04 */ s32 kind;            /* 0 single step, 1 step out of range */
    /* +0x08 */ s32 count;           /* instructions left to step */
    /* +0x0C */ u32 range_start;
    /* +0x10 */ u32 range_end;
    /* +0x14 */ u8 pad_0x14[4];
} TRKStepState; /* size: 0x18 */

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
extern TRKState gTRKState;
extern TRKRestoreFlags gTRKRestoreFlags;

/* 0x8046BBEC (0x8): reads the machine state register. */
u32 __TRK_get_MSR(void);

/* 0x8046BBF4 (0x8): writes the machine state register. */
void __TRK_set_MSR(u32 msr);

/* 0x8046D300 (0x68): patches a blr into the ten-word instruction template, flushes it and calls it with `data`. */
s32 TRKPPCAccessSpecialReg(u32* data, u32* instructions, s32 is_read);

/* 0x8046D378 (0x8): maps an address into the cached view. */
u32* ConvertAddress(u32 address);

/* 0x8046D380 (0x90): counts the OS active threads and reports the index of the running one. */
void GetThreadInfo(u32* count, u32* currentIndex);

/* 0x8046C418 (0x194): the exception entry the vector stubs jump to; saves the target and posts an event. */
void TRKInterruptHandler(void);

/* 0x8046C5AC (0x9C): handles an exception raised while the nub itself runs; skips trapping instructions and flags it. */
void TRKExceptionHandler(void);

/* 0x8046C648 (0xB8): posts the event that matches the exception that stopped the target. */
void TRKPostInterruptEvent(void);

/* 0x8046C7C4 (0x54): restores the nub's register image and continues in TRKPostInterruptEvent. */
void TRKInterruptHandlerEnableInterrupts(void);

/* 0x8046CB10 (0x124): advances an armed step request after a trace exception; returns whether one is still armed. */
s32 TRKTargetCheckStep(void);

/* 0x8046CF44 (0x10): records whether the target is stopped. */
void TRKTargetSetStopped(s32 stopped);

/* 0x8046C700 (0xC4): swaps the nub and target contexts and runs the target until the next interrupt. */
void TRKSwapAndGo(void);

/* 0x8046C818 (0x5C): handles a breakpoint or exception event. */
s32 TRKTargetInterrupt(TRKEvent* event);

/* 0x8046C874 (0x200): appends the stop description to the notification in `message`. */
s32 TRKTargetAddStopInfo(TRKBuffer* message);

/* 0x8046CA74 (0x9C): appends the exception description to the notification in `message`. */
s32 TRKTargetAddExceptionInfo(TRKBuffer* message);

/* 0x8046D368 (0x10): records the nub's input-pending flag pointer. */
void TRKTargetSetInputPendingPtr(u8* inputPendingPtr);

/* 0x8046CD40 (0x1F4): services a file or console request raised by the program. */
s32 TRKTargetSupportRequest(void);

/* 0x8046BDC0 (0x150): reads (`is_read`) or writes `*length` bytes of target memory at `address` through `data`; returns 0 or an error code. */
/* untyped: byte range */
s32 TRKTargetAccessMemory(void* data, u32 address, u32* length, s32 options, s32 is_read);

/* 0x8046BF10 (0xF8): reads or writes the general registers `first`..`last` through `message`; returns 0 or an error code. */
s32 TRKTargetAccessDefault(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read);

/* 0x8046C008 (0x13C): reads or writes the floating-point registers `first`..`last`. */
s32 TRKTargetAccessFP(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read);

/* 0x8046C144 (0x164): reads or writes the first extended register block `first`..`last`. */
s32 TRKTargetAccessExtended1(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read);

/* 0x8046C2A8 (0x170): reads or writes the second extended register block `first`..`last`. */
s32 TRKTargetAccessExtended2(u32 first, u32 last, TRKBuffer* message, u32* count, s32 is_read);

/* 0x8046CC34 (0x88): steps the target `count` instructions; `step_over` skips calls. */
s32 TRKTargetSingleStep(u32 count, s32 step_over);

/* 0x8046CCBC (0x74): runs the target until the PC leaves `start`..`end`; `step_over` skips calls. */
s32 TRKTargetStepOutOfRange(u32 start, u32 end, s32 step_over);

/* 0x8046CD30 (0x10): returns the target's program counter. */
u32 TRKTargetGetPC(void);

/* 0x8046CF54 (0x18): stops the target; returns 0 or an error code. */
s32 TRKTargetStop(void);

/* 0x8046D410 (0x8): selects whether console I/O goes over the serial link. */
void SetUseSerialIO(u8 useSerial);

/* 0x8046D418 (0x8): returns whether console I/O goes over the serial link. */
u8 GetUseSerialIO(void);

/* 0x8046CF34 (0x10): returns whether the target is stopped. */
s32 TRKTargetStopped(void);

#ifdef __cplusplus
}
#endif

#endif
