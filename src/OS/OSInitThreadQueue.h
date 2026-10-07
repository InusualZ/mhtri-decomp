/* OS/OSInitThreadQueue.h - the declaration of `OSInitThreadQueue`, which `OS/OSThread.c` owns (docs/plan.md 6.5 rule 2, leaf header). */
#ifndef MHTRI_OS_OSINITTHREADQUEUE_H
#define MHTRI_OS_OSINITTHREADQUEUE_H

#include "OS/OSThread.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804D3960 - empties a thread queue. */
void OSInitThreadQueue(OSThreadQueue* queue);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_OS_OSINITTHREADQUEUE_H */
