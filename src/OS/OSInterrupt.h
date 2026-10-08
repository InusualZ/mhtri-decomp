/*
 * OS/OSInterrupt.h - declarations of the symbols owned by `OS/OSInterrupt.c` that other units call or read.
 */
#ifndef OS_OSINTERRUPT_H
#define OS_OSINTERRUPT_H

#include "types.h"
#include "OS/OSContext.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL OSDisableInterrupts(void);

BOOL OSRestoreInterrupts(BOOL level);

BOOL OSEnableInterrupts(void);

/* The handler of one interrupt source; `interrupt` is the source index, `context` the interrupted context. */
typedef void (*__OSInterruptHandler)(s16 interrupt, OSContext* context);

/* 0x804D0CE0 - installs a handler and returns the previous one. */
__OSInterruptHandler __OSSetInterruptHandler(s16 interrupt, __OSInterruptHandler handler);
/* 0x804D0D00 - the handler installed for an interrupt source. */
__OSInterruptHandler __OSGetInterruptHandler(s16 interrupt);
/* 0x80795360 / 0x8079535C / 0x80795358 - the time, source and interrupted address of the last external interrupt. */
extern s64 __OSLastInterruptTime;
extern s16 __OSLastInterrupt;
extern u32 __OSLastInterruptSrr0;

/* 0x804D1040 / 0x804D10C0 - mask / unmask a set of interrupt sources; return the previous mask. */
u32 __OSMaskInterrupts(u32 mask);
u32 __OSUnmaskInterrupts(u32 mask);

/* 0x804D0D10 - installs the external interrupt vector handler and masks every interrupt source. */
void __OSInterruptInit(void);

#ifdef __cplusplus
}
#endif

#endif
