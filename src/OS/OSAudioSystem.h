/*
 * OS/OSAudioSystem.h - declarations of the symbols owned by `OS/OSAudioSystem.c` that other units call or read.
 */
#ifndef OS_OSAUDIOSYSTEM_H
#define OS_OSAUDIOSYSTEM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804CC130 - programs the audio clock from clock source `source`. */
void __AIClockInit(int source);

/* 0x804CC350 - brings the DSP audio system up. */
void __OSInitAudioSystem(void);

/* 0x804CC520 - stops the DSP audio system. */
void __OSStopAudioSystem(void);

#ifdef __cplusplus
}
#endif

#endif
