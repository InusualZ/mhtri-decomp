/*
 * OS/OSAudioSystem.h - declarations of the symbols owned by `OS/OSAudioSystem.c` that other units call or read.
 */
#ifndef OS_OSAUDIOSYSTEM_H
#define OS_OSAUDIOSYSTEM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CC520 - stops the DSP audio system. */
void __OSStopAudioSystem(void);

#ifdef __cplusplus
}
#endif

#endif
