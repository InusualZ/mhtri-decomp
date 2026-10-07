/*
 * OS/OSMemory.h - declarations of the symbols owned by `OS/OSMemory.c` that other units call or read.
 */
#ifndef OS_OSMEMORY_H
#define OS_OSMEMORY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D1740 / 0x804D1750 / 0x804D1760 - the sizes of MEM2 and of the simulated MEM1 / MEM2, read from the low-memory arena. */
u32 OSGetPhysicalMem2Size(void);
u32 OSGetConsoleSimulatedMem1Size(void);
u32 OSGetConsoleSimulatedMem2Size(void);

/* 0x804D1DC0 - restores the BAT mapping that executes from MEM1; `magic` is the value the caller must pass for the call to return. */
void __OSRestoreCodeExecOnMEM1(u32 magic);

/* 0x804D1E10 - installs the memory-protection interrupt handlers and sets up the BATs; runs once. */
void __OSInitMemoryProtection(void);

#ifdef __cplusplus
}
#endif

#endif
