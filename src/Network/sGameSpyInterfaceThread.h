/*
 * Network/sGameSpyInterfaceThread.h - the live worker-thread pointer (`.sbss` 0x80794CE4)
 *   `Network/GameSpyInterfaceThread.cpp` defines: a leaf header for consumers that cannot include the owner's full
 *   header (`Network/NetworkSessionManager.h`).
 */
#ifndef MHTRI_NETWORK_SGAMESPYINTERFACETHREAD_H
#define MHTRI_NETWORK_SGAMESPYINTERFACETHREAD_H

/* the class is named through the elaborated specifier so the header declares exactly the one symbol */
extern "C" class GameSpyInterfaceThread* sGameSpyInterfaceThread;   /* 0x80794CE4 (.sbss) - the live GameSpyInterfaceThread */

#endif /* MHTRI_NETWORK_SGAMESPYINTERFACETHREAD_H */
