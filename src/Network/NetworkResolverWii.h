/*
 * Network/NetworkResolverWii.h - the Wii resolver class, the lookup thread's entry and the lookup it runs
 *   (`Network/NetworkResolverWii.cpp`).
 */

#ifndef NETWORK_NETWORK_RESOLVER_WII_H
#define NETWORK_NETWORK_RESOLVER_WII_H

#include "unsplit/SO.h"                    /* SOAddrInfo - the lookup hints and result list */
#include "Network/network_socket_streams.h"  /* NetworkResolverBase */

/* `NetworkResolverWii` (table 0x805F9980; 0x1568 B, the size `createResolver` allocates): the base record plus the state of the lookup - the one-byte
   state at +0x218 (idle until the first failure, then 0xFF; 0x0A while the lookup thread runs, 0x0F when it
   ended, 0x14 once the addresses are copied, 0x5A on failure), the thread the lookup runs on with its
   stack, the lookup's result and inputs and the SDK's result list. */
class NetworkResolverWii : public NetworkResolverBase {
public:
    NetworkResolverWii();
    virtual ~NetworkResolverWii();
    /* +0x0C */ virtual s32 setName(const char* name);
    /* +0x10 */ virtual void resetCode();
    /* +0x14 */ virtual s32 check();
    /* +0x18 */ virtual void recordGet(s32 index, u32* out);

    u8    code_218;                    /* +0x218 - the lookup's state */
    u8    pad_219[0x07];               /* +0x219..+0x21F */
    u8    thread_220[0x318] __attribute__((aligned(8))); /* +0x220..+0x537 - the OS thread the lookup runs on (8-aligned: the OS thread record holds doubles) */
    u8    stack_538[0x1000];           /* +0x538..+0x1537 - its stack */
    s32   result_1538;                 /* +0x1538 - what the lookup returned */
    const char* lookupName_153C;       /* +0x153C - the name the thread resolves */
    SOAddrInfo hints_1540;             /* +0x1540..+0x155F - the lookup hints: only the family is set */
    SOAddrInfo* addrInfo_1560;         /* +0x1560 - the SDK's result list, freed when consumed */
};   /* size: 0x1568 */


extern "C" {

/* untyped: opaque handle passed through - the OS thread entry receives and returns a plain pointer */
void* networkResolver_threadEntry(void* self);
void networkResolver_lookup(NetworkResolverWii* self);
}

#endif /* NETWORK_NETWORK_RESOLVER_WII_H */
