/* Declarations owned by `src/Network/sNetworkLibraryWii.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_SNETWORKLIBRARYWII_H
#define MHTRI_NETWORK_SNETWORKLIBRARYWII_H

#include "types.h"
#include "Network/network_transport.h"
#include "Network/sNetworkLibrary.h"   /* sNetworkLibrary, the base class */

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct NetworkPat;
class NetworkSessionManagerPat;   /* include/Network/NetworkSessionManager.h */

/* The OS thread record the library runs its SO/DWC calls on (the SDK's `OSThread`). size: 0x318 */
typedef struct sNetworkLibraryThread {
    /* +0x000 */ u8 pad_0x000[0x318];
} sNetworkLibraryThread;

/* The Wii implementation of `sNetworkLibrary` (log strings `sNetworkLibraryWii::init/final/start/stop`):
 * SO and DWC bring-up on a worker thread, a wall clock and the host address tables.  Its table is
 * 0x80603080, emitted by this unit.  size: 0x1408 (the allocation `constructNetworkWiiMediator` makes) */
class sNetworkLibraryWii : public sNetworkLibrary {
public:
    sNetworkLibraryWii();
    /* +0x08 */ virtual ~sNetworkLibraryWii();
    /* +0x0C */ virtual s32 logLevel(s32 level, const char* format, ...);
    /* +0x10 */ virtual s32 logError(const char* format, ...);
    /* +0x14 */ virtual s32 logWarning(const char* format, ...);
    /* +0x1C */ virtual void update();
    /* +0x20 */ virtual s32 isLinkUp();
    /* +0x24 */ virtual s32 isOnline();
    /* +0x28 */ virtual s32 isBusy();
    /* +0x2C */ virtual void print(const char* format, ...);
    /* +0x30 */ virtual s32 init(sNetworkLibraryInitParam* param);
    /* +0x34 */ virtual void final();
    /* +0x38 */ virtual s32 start(s32 reserved, sNetworkLibraryError* error);
    /* +0x3C */ virtual s32 stop();
    /* +0x40 */ virtual void updateTime();
    /* +0x44 */ virtual void suspend();
    /* +0x60 */ virtual f32 getElapsedTime();
    /* +0x64 */ virtual s64 getTime(s32 zone);
    /* +0x74 */ virtual s32 getHostIdCount();
    /* +0x78 */ virtual void getHostId(s32 index, u8* out);
    /* +0x7C */ virtual s32 getDnsServerCount();
    /* +0x80 */ virtual void getDnsServer(s32 index, u8* out);
    /* +0x84 */ virtual void resume();
    /* +0x88 */ virtual NetworkResolverWii* createResolver();
    /* +0x8C */ virtual NetworkSocketHandle* createSocket();
    /* +0x90 */ virtual NetworkFileFetcher* createFetcher(u32 kind);

    /* The worker-thread bodies: each stores the SDK call's result in `threadResult`. */
    void runSOStartup();
    void runSOCleanup();
    void runDwcInit();
    /* Adds the microseconds since `*lastTick` to `*elapsed` once more than a millisecond has passed. */
    void accumulateElapsed(f32* elapsed, u32* lastTick);

    /* +0x09A */ u8 initialized;
    /* +0x09B */ u8 initFailed;
    /* +0x09C */ u8 startState;
    /* +0x09D */ u8 stopState;
    /* +0x09E */ u8 pad_0x09E[0x02];
    /* +0x0A0 */ f32 elapsedTime;
    /* +0x0A4 */ u32 lastTick;
    /* +0x0A8 */ s32 startCount;
    /* +0x0AC */ s32 startRetries;
    /* +0x0B0 */ u8 hostIds[4][4];
    /* +0x0C0 */ s32 hostIdCount;
    /* +0x0C4 */ u8 dnsServers[4][8];
    /* +0x0E4 */ s32 dnsServerCount;
    /* +0x0E8 */ sNetworkLibraryThread thread;
    /* +0x400 */ u8 threadStack[0x1000];
    /* +0x1400 */ s32 threadResult;
    /* +0x1404 */ u8 pad_0x1404[0x04];
};

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794CD0 - the Pat holder the constructor publishes (`getPatsObject` reads it). */
extern struct NetworkPat* sNetworkPatInstance;

/* 0x8041891C / 0x80418940 / 0x80418964 - the three worker-thread entry points `sNetworkLibraryWii`
 * starts: SO start-up, SO clean-up and the DWC initialisation; each is handed the library. */
/* untyped: caller-owned payload - the thread argument */
void* networkLibrarySOStartupThread(void* library);
/* untyped: caller-owned payload - the thread argument */
void* networkLibrarySOCleanupThread(void* library);
/* untyped: caller-owned payload - the thread argument */
void* networkLibraryDwcInitThread(void* library);

/* 0x80418988 - creates the Wii network library and publishes it as `sNetworkLibrary::mpInstance`. */
void constructNetworkWiiMediator(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_SNETWORKLIBRARYWII_H */
