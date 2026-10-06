/*
 * Network/sNetworkLibraryWii.cpp - `sNetworkLibraryWii`, the Wii implementation of the network library singleton: the
 *   three worker-thread entry points, `constructNetworkWiiMediator`, ctor, dtor, the table's own slots, the worker
 *   bodies, `start`, `stop`, the clock and address slots, the three factories.
 * RANGE. .text 0x8041891C-0x80419AD4 (34 functions); .data 0x80602AF8-0x80603118 (the log strings, then the class table
 *   0x80603080), .sbss 0x80794CD0-0x80794CD8 (`sNetworkPatInstance`), .sdata2 0x8079C878-0x8079C888 (0.0f, 1e-6f, the
 *   u32->float bias), extab, extabindex.  Left seam: the base table 0x80602A60 followed by this unit's strings puts the
 *   start in 0x80417C44..0x80418A60, and the entry points 0x8041891C/0x80418940/0x80418964 and 0x80418988 call only
 *   into this class.  Right seam: the class's last slot (`resume`, 0x80419AD0); the Pat holder above is
 *   `Network/NetworkPat.cpp`'s.  `sNetworkPatInstance` is written only by that family and read by `getPatsObject`, but
 *   stays claimed here: the `Matching` NetworkPat object would emit 4 B of the 8-byte claim.
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md).  File scope: `#pragma peephole
 *   off` (retail keeps `extsh`+`cmpwi` on the `s16` delete flags and reloads the table word through r3 before each
 *   virtual call: the dtor, `updateNetworkPat`) and `#pragma pool_data off` (every string is its own `lis`/`addi`:
 *   `init`, `start`); `#pragma fp_contract off` around `accumulateElapsed` (retail keeps `fmuls`+`fadds`).
 * NAMES. The log strings name the class (`sNetworkLibraryWii::init/final/start/stop`, `sNetworkLibrary::start/stop` for
 *   the shared messages); every method and field name is a GUESS from the body and strings; the SDK calls are named
 *   from the strings beside them (`SOInit`, `SOFinish`, `SOStartup`, `SOCleanup`, `DWC_Init`).  GUESSes in the owners'
 *   headers: `DWC_Shutdown`, `NETGetStartupErrorCode`.
 * RESIDUALS. none in `.text`.  `.data` 0x61C of the claimed 0x620 and `.sbss` 4 of 8 (trailing alignment words); `extab`
 *   carries one `__dt__15sNetworkLibraryFv` cleanup record more than retail.
 * SHAPES. The table 0x80603080 inherits the base's non-pure slots (`setLogLevel`, the byte-order and calendar helpers)
 *   from the base table 0x80602A60 (`Network/sNetworkLibrary.cpp`).  `mpMediator__15sNetworkLibrary` (.sbss 0x80794CC4)
 *   is a static member of the base, defined by `Network/sNetworkLibrary.cpp` like `mpInstance` 0x80794CC0 and `mpRandom`
 *   0x80794CC8; this unit's `init`/`final` are its only writers, and `getInstance` in `sound/snd_stream_reloc.cpp`
 *   (0x800E89D8) is the header-inline accessor.  `createSocket`/`createFetcher` are real `new`s (the six `__dl__FPv`
 *   cleanup records); `constructNetworkWiiMediator`'s `new` gives its extab record.  `getTime` accumulates into a local
 *   seeded with the epoch, and `accumulateElapsed` tests `!*elapsed` (operand order of the `fcmpu`).
 */
#include "types.h"
#include "Network/sNetworkLibraryWii.h"
#include "Network/NetworkWiiMediator.h"
#include "Network/network_pat_control.h"   /* NetworkFileFetcher */
#include "Network/network_socket_streams.h"
#include "Network/NetworkResolverWii.h"
#include "Network/NetworkPat.h"
#include "unsplit/Network.h"
#include "sound/fn_800E46E8.h"           /* getInstance - the mediator accessor */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "sys_mem.h"
#include "Network/PatInterface.h"
#include "Network/NetworkSocketWii.h"     /* NetworkSocketWii */
#include "Network/NetworkFileFetcher.h"   /* NetworkNullFetcher */
#include "SO/soi.h"                        /* SOInit, SOFinish, SOStartup, SOCleanup, SOGetHostID */
#include "DWCi/dwc_error.h"                /* DWC_Init, DWC_Shutdown */
#include "NCD/ncdsystem.h"                 /* NETGetStartupErrorCode */
#include "MSL_C/alloc.h"                   /* srand */
#include "NAND/nand.h"                     /* OSGetTick, OSGetTime */

/* The OS bus clock, read out of the low-memory arena (the SDK's `OS_BUS_CLOCK`). */
#define NETWORK_BUS_CLOCK (*(u32*)0x800000F8)
#define NETWORK_TICKS_TO_USEC(ticks) (((ticks) * 8) / ((NETWORK_BUS_CLOCK / 4) / 125000))

#pragma peephole off
#pragma pool_data off

/* The Pat holder `constructNetworkPat` (`Network/NetworkPat.cpp`) publishes; see BOUNDARY. */
NetworkPat* sNetworkPatInstance;

/* untyped: caller-owned payload - the thread argument */
void* networkLibrarySOStartupThread(void* library)
{
    ((sNetworkLibraryWii*)library)->runSOStartup();
    return NULL;
}

/* untyped: caller-owned payload - the thread argument */
void* networkLibrarySOCleanupThread(void* library)
{
    ((sNetworkLibraryWii*)library)->runSOCleanup();
    return NULL;
}

/* untyped: caller-owned payload - the thread argument */
void* networkLibraryDwcInitThread(void* library)
{
    ((sNetworkLibraryWii*)library)->runDwcInit();
    return NULL;
}

/* Creates the Wii network library and publishes it as the singleton. */
void constructNetworkWiiMediator(void)
{
    sNetworkLibrary::mpInstance = new sNetworkLibraryWii();
}

/* Clears the run state, the clock and both address tables. */
sNetworkLibraryWii::sNetworkLibraryWii()
{
    initialized = 0;
    initFailed = 1;
    startState = 0xFF;
    stopState = 0xFF;
    elapsedTime = 0.0f;
    lastTick = 0;
    startCount = 0;
    memset(hostIds, 0, sizeof(hostIds));
    hostIdCount = 0;
    memset(dnsServers, 0, sizeof(dnsServers));
    dnsServerCount = 0;
}

/* Shuts the library down before the base destructor runs. */
sNetworkLibraryWii::~sNetworkLibraryWii()
{
    final();
}

/* The release build's logging slots discard their message. */
s32 sNetworkLibraryWii::logLevel(s32 level, const char* format, ...)
{
    return 0;
}

s32 sNetworkLibraryWii::logError(const char* format, ...)
{
    return 0;
}

s32 sNetworkLibraryWii::logWarning(const char* format, ...)
{
    return 0;
}

void sNetworkLibraryWii::update()
{
}

s32 sNetworkLibraryWii::isLinkUp()
{
    return 0;
}

s32 sNetworkLibraryWii::isOnline()
{
    return 0;
}

s32 sNetworkLibraryWii::isBusy()
{
    return 0;
}

void sNetworkLibraryWii::print(const char* format, ...)
{
}

/* Creates the mediator, hands it the DWC game info and brings the SO library up. */
s32 sNetworkLibraryWii::init(sNetworkLibraryInitParam* param)
{
    u32 gameInfo[9];
    s32 result;

    if (initialized) {
        logWarning("sNetworkLibraryWii::init: reinitialized\n");
        return 0;
    }
    if (param == NULL) {
        logError("sNetworkLibraryWii::init: argument is NULL.\n");
        setLastError(0x80000003, 0, 0);
        return -1;
    }
    result = sNetworkLibrary::init(NULL);
    if (result < 0) {
        return result;
    }
    if (mpMediator == NULL) {
        mpMediator = new NetworkWiiMediator();
    }
    memset(gameInfo, 0, sizeof(gameInfo));
    if (param->mode != -1) {
        if (param->gameInfo[0] == 0 || param->gameInfo[1] == 0 || param->gameInfo[2] == 0 ||
            param->gameInfo[3] == 0 || param->gameInfo[4] == 0 || param->gameInfo[5] == 0 ||
            param->gameInfo[6] == 0 || param->gameInfo[7] == 0) {
            logError("sNetworkLibraryWii::init: DwcInitParam is NULL.\n");
            setLastError(0x80000003, 0, 0);
            return -1;
        }
        gameInfo[0] = param->mode != 1;
        gameInfo[1] = param->gameInfo[0];
        gameInfo[2] = param->gameInfo[1];
        gameInfo[3] = param->gameInfo[2];
        gameInfo[4] = param->gameInfo[3];
        gameInfo[5] = param->gameInfo[4];
        gameInfo[6] = param->gameInfo[5];
        gameInfo[7] = param->gameInfo[6];
        gameInfo[8] = param->gameInfo[7];
    }
    setGameInfo2d1c(getInstance(), gameInfo);
    if (param->soConfig.alloc == NULL || param->soConfig.free == NULL) {
        logError("sNetworkLibraryWii::init: SOLibraryConfig is NULL.\n");
        setLastError(0x80000003, 0, 0);
        return -1;
    }
    initFailed = 0;
    result = SOInit(&param->soConfig);
    if (result == -7) {
        logLevel(3, "sNetworkLibraryWii::init: SOInit() have been initialized.\n");
        initFailed = 1;
    } else if (result < 0) {
        logError("sNetworkLibraryWii::init: SOInit() failed.(%d)\n", result);
        setLastError(0x80000007, 0x4C, result);
        initFailed = 1;
        return result;
    }
    startState = 0;
    stopState = 0;
    elapsedTime = 0.0f;
    lastTick = 0;
    startCount = 0;
    initialized = 1;
    return 0;
}

/* Cleans SO up if a start is still outstanding, finishes the SO library and deletes the mediator. */
void sNetworkLibraryWii::final()
{
    s32 result;

    if (startCount != 0) {
        logError("sNetworkLibraryWii::final: SOCleanup() have not been called.\n");
        DWC_Shutdown();
        SOCleanup();
    }
    result = SOFinish();
    if (result == -7) {
        logLevel(3, "sNetworkLibraryWii::final: SOFinish() have been finished.\n");
    } else if (result < 0) {
        logError("sNetworkLibraryWii::final: SOFinish() failed.(%d)\n", result);
        setLastError(0x80000007, 0x4D, result);
    }
    if (mpMediator != NULL) {
        delete mpMediator;
        mpMediator = NULL;
    }
    sNetworkLibrary::final();
    initialized = 0;
}

void sNetworkLibraryWii::runSOStartup()
{
    threadResult = SOStartup();
}

void sNetworkLibraryWii::runSOCleanup()
{
    threadResult = SOCleanup();
}

void sNetworkLibraryWii::runDwcInit()
{
    u32 gameInfo[9];

    getGameInfo2d1c(getInstance(), gameInfo);
    threadResult = DWC_Init(gameInfo[0], gameInfo[3], gameInfo[4], gameInfo[1], gameInfo[2]);
}

/* Steps the start state machine: SO start-up on the worker thread, then DWC_Init, then the host id.
 * 1 when the library is up, 0 while in progress, negative on failure. */
s32 sNetworkLibraryWii::start(s32 reserved, sNetworkLibraryError* error)
{
    s32 result;
    u32 gameInfo[9];

    if (!initialized) {
        error->facility = 0x80000007;
        error->step = 0x4C;
        error->code = 0;
        logError("sNetworkLibrary::start: init() have not done.");
        return -1;
    }
    if (initFailed) {
        error->facility = 0x80000007;
        error->step = 0x4C;
        error->code = 0;
        logError("sNetworkLibraryWii::start: SOInit() failed.\n");
        setLastError(error->facility, error->step, error->code);
        return -1;
    }
    switch (startState) {
    case 0:
        if (startCount != 0) {
            logLevel(3, "sNetworkLibrary::start(): already done %d times.\n", startCount++);
            return 1;
        }
        if (stopState == 0) {
            startRetries = 2;
            startState += 10;
        }
        break;
    case 10:
        result = OSCreateThread(&thread, (void*)networkLibrarySOStartupThread, this, threadStack + sizeof(threadStack),
                                sizeof(threadStack), 14, 1);
        if (result == 0) {
            error->facility = 0x80000007;
            error->step = 0x50;
            error->code = result;
            logError("sNetworkLibraryWii::start: OSCreateThread failed.(%d)\n", result);
            setLastError(error->facility, error->step, error->code);
            startState = 0;
            return -1;
        }
        threadResult = 0;
        OSResumeThread(&thread);
        startState += 5;
        break;
    case 15:
        if (OSIsThreadTerminated(&thread)) {
            result = threadResult;
            if (result < 0) {
                if (result == -7) {
                    logError("sNetworkLibraryWii::start: SOStartup() already done. (%d)\n", result);
                } else {
                    if (result == -10) {
                        logError("sNetworkLibraryWii::start: SOStartup() failed, Link state is in transit. (%d)\n", result);
                    } else {
                        logError("sNetworkLibraryWii::start: SOStartup() failed. (%d)\n", result);
                    }
                    if (startRetries-- > 0) {
                        startState -= 5;
                        break;
                    }
                }
                error->facility = 0x80000007;
                error->step = 0x32;
                error->code = -NETGetStartupErrorCode(result);
                logError("sNetworkLibraryWii::start: Network Error Code is %d\n", error->code);
                setLastError(error->facility, error->step, error->code);
                startState = 0;
                return -1;
            }
            startCount++;
            startState += 5;
        }
        break;
    case 20:
        getGameInfo2d1c(getInstance(), gameInfo);
        if (gameInfo[1] != 0) {
            result = OSCreateThread(&thread, (void*)networkLibraryDwcInitThread, this, threadStack + sizeof(threadStack),
                                    sizeof(threadStack), 14, 1);
            if (result == 0) {
                logError("sNetworkLibraryWii::init: OSCreateThread failed.(%d)\n", result);
                setLastError(0x80000007, 0x50, result);
                return -1;
            }
            threadResult = 0;
            OSResumeThread(&thread);
            startState += 5;
        } else {
            startState += 10;
        }
        break;
    case 25:
        if (OSIsThreadTerminated(&thread)) {
            result = threadResult;
            if (result < 0) {
                logError("sNetworkLibraryWii::init: DWC_Init failed.(%d)\n");
            }
            startState += 5;
        }
        break;
    case 30:
        startState = 0;
        result = SOGetHostID();
        if (result != 0) {
            memcpy(hostIds, &result, sizeof(hostIds[0]));
            hostIdCount = 1;
        }
        return 1;
    }
    return 0;
}

/* Steps the stop state machine: SO clean-up on the worker thread.  1 when stopped, 0 while in progress. */
s32 sNetworkLibraryWii::stop()
{
    s32 result;

    if (!initialized) {
        logError("sNetworkLibrary::stop: init() have not done.");
        return -1;
    }
    switch (stopState) {
    case 0:
        if (startCount > 1) {
            logLevel(3, "sNetworkLibrary::stop() yet %d times.\n", --startCount);
            return 1;
        }
        if (startCount < 1) {
            logLevel(3, "sNetworkLibrary::stop() already done.\n");
            return 1;
        }
        if (startState != 0) {
            break;
        }
        DWC_Shutdown();
        result = OSCreateThread(&thread, (void*)networkLibrarySOCleanupThread, this, threadStack + sizeof(threadStack),
                                sizeof(threadStack), 14, 1);
        if (result == 0) {
            logError("sNetworkLibraryWii::stop: OSCreateThread failed.(%d)\n", result);
            setLastError(0x80000007, 0x50, result);
            return -1;
        }
        threadResult = 0;
        OSResumeThread(&thread);
        stopState = 10;
        break;
    case 10:
        if (OSIsThreadTerminated(&thread)) {
            stopState = 0;
            result = threadResult;
            if (result < 0) {
                if (result == -7) {
                    logError("sNetworkLibraryWii::stop: SOCleanup() already done. (%d)\n", result);
                } else if (result == -10) {
                    logError("sNetworkLibraryWii::stop: SOCleanup() failed, Link state is in transit. (%d)\n", result);
                } else {
                    logError("sNetworkLibraryWii::stop: SOCleanup() failed.(%d)\n", result);
                }
                setLastError(0x80000007, 0x4E, result);
                return -1;
            }
            startCount--;
            return 1;
        }
        break;
    }
    return 0;
}

void sNetworkLibraryWii::updateTime()
{
    accumulateElapsed(&elapsedTime, &lastTick);
}

void sNetworkLibraryWii::suspend()
{
}

f32 sNetworkLibraryWii::getElapsedTime()
{
    return elapsedTime;
}

/* The console clock in microseconds, offset to the library's epoch. */
s64 sNetworkLibraryWii::getTime(s32 zone)
{
    s64 time = 0x08C2419CEB14C000LL;

    time += NETWORK_TICKS_TO_USEC(OSGetTime());
    return time;
}

s32 sNetworkLibraryWii::getHostIdCount()
{
    return hostIdCount;
}

void sNetworkLibraryWii::getHostId(s32 index, u8* out)
{
    if (hostIdCount > index) {
        memcpy(out, hostIds[index], sizeof(hostIds[0]));
    }
}

s32 sNetworkLibraryWii::getDnsServerCount()
{
    return dnsServerCount;
}

void sNetworkLibraryWii::getDnsServer(s32 index, u8* out)
{
    if (dnsServerCount > index) {
        memcpy(out, dnsServers[index], sizeof(dnsServers[0]));
    }
}

#pragma fp_contract off
void sNetworkLibraryWii::accumulateElapsed(f32* elapsed, u32* lastTick)
{
    u32 now;
    u32 usec;

    if (!*elapsed && !*lastTick) {
        *lastTick = OSGetTick();
    } else {
        now = OSGetTick();
        usec = NETWORK_TICKS_TO_USEC(now - *lastTick);
        if (usec > 1000) {
            *elapsed += 1.0e-6f * usec;
            *lastTick = now;
        }
    }
}
#pragma fp_contract reset

NetworkResolverWii* sNetworkLibraryWii::createResolver()
{
    return new NetworkResolverWii();
}

NetworkSocketHandle* sNetworkLibraryWii::createSocket()
{
    return (NetworkSocketHandle*)new NetworkSocketWii();
}

NetworkFileFetcher* sNetworkLibraryWii::createFetcher(u32 kind)
{
    if (kind != 2) {
        return new NetworkFileFetcher();
    }
    return (NetworkFileFetcher*)new NetworkNullFetcher();
}

void sNetworkLibraryWii::resume()
{
}
