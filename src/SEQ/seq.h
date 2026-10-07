/*
 * SEQ/seq.h - declarations of the symbols owned by `SEQ/seq.c` that other units call or read.
 */
#ifndef SEQ_SEQ_H
#define SEQ_SEQ_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DD430 - marks the sequencer initialised with an empty sequence list (a no-op when already initialised). */
void SEQInit(void);

/* 0x804DD450 - clears the initialised flag and the sequence list. */
void SEQQuit(void);

/* 0x804DD460 - advances every running sequence by one audio frame. */
void SEQRunAudioFrame(void);

#ifdef __cplusplus
}
#endif

#endif
