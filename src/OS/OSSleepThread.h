/* OS/OSSleepThread.h - the declaration of `OSSleepThread`, which `OS/OSThread.c` owns (docs/plan.md 6.5 rule 2): puts the calling thread to sleep on a queue. */
#ifndef MHTRI_OS_OSSLEEPTHREAD_H
#define MHTRI_OS_OSSLEEPTHREAD_H

#include "OS/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif
void OSSleepThread(OSThreadQueue* queue);
#ifdef __cplusplus
}
#endif

#endif
