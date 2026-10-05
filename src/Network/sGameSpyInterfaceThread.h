/*
 * Network/sGameSpyInterfaceThread.h - the live worker-thread pointer `src/Network/GameSpyInterfaceThread.cpp`
 * defines (`.sbss` 0x80794CE4, claimed by that unit).  A leaf header (docs/plan.md 6.5 rule 2): a consumer
 * that cannot include the owner's full header - `Network/NetworkSessionManager.h` carries its own partial
 * view of `GameSpyInterfaceThread` - includes this one for the pointer.
 */
#ifndef MHTRI_NETWORK_SGAMESPYINTERFACETHREAD_H
#define MHTRI_NETWORK_SGAMESPYINTERFACETHREAD_H

/* the class is named through the elaborated specifier so the header declares exactly the one symbol */
extern "C" class GameSpyInterfaceThread* sGameSpyInterfaceThread;   /* 0x80794CE4 (.sbss) - the live GameSpyInterfaceThread */

#endif /* MHTRI_NETWORK_SGAMESPYINTERFACETHREAD_H */
