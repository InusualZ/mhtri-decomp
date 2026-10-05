/*
 * Network/sNetworkLibrary.h - the base network library class `sNetworkLibrary`, its records and the free helpers
 *   `Network/sNetworkLibrary.cpp` defines.
 */
#ifndef MHTRI_NETWORK_SNETWORKLIBRARY_H
#define MHTRI_NETWORK_SNETWORKLIBRARY_H

#include "types.h"
#include "Network/network_transport.h"
#include "SO/soi.h"                     /* SOLibraryConfig - the init block's first member */

class NetworkLogger;          /* unsplit/Network.h */
class NetworkWiiMediator;     /* Network/NetworkWiiMediator.h */
class NetworkResolverWii;     /* Network/network_transport_types.h */
class NetworkFileFetcher;     /* Network/network_pat_control.h */
class NetworkRandom;          /* Network/NetworkPool.h */

/* The block `sNetworkLibrary::init` is handed: the SO allocator pair `SOInit` takes by address, then the
 * DWC game-info words the mediator keeps (`mode` -1 skips them; the eight values are all required).
 * size: 0x2C */
typedef struct sNetworkLibraryInitParam {
    /* +0x00 */ SOLibraryConfig soConfig;
    /* +0x08 */ s32 mode;
    /* +0x0C */ u32 gameInfo[8];
} sNetworkLibraryInitParam;

/* The error record `start`/`stop` fill on failure: a facility code, the step that failed and the code
 * the SDK returned (the same triple `setLastError` keeps).  size: 0x0C */
typedef struct sNetworkLibraryError {
    /* +0x00 */ s32 facility;
    /* +0x04 */ s32 step;
    /* +0x08 */ s32 code;
} sNetworkLibraryError;

/* The calendar record `convertTime` fills and `dateToTime` turns back into microseconds (the day count
 * starts at 1 January of year 0, proleptic Gregorian).  size: 0x14 */
typedef struct NetworkDateTime {
    /* +0x00 */ s16 year;
    /* +0x02 */ s16 month;
    /* +0x04 */ s16 day;
    /* +0x06 */ s16 hour;
    /* +0x08 */ s16 minute;
    /* +0x0A */ s16 second;
    /* +0x0C */ s16 weekday;
    /* +0x0E */ u8 pad_0x0E[0x02];
    /* +0x10 */ s32 weekIndex;      /* (days - 2) / 7 - GUESS: a running week count, the weekday's quotient */
} NetworkDateTime;

/* The platform-independent network library: the singleton the whole Network band reaches through
 * `getNetworkLogger` (lobby/lb_server_sel_trans.cpp reads `mpInstance`).  Its table (0x80602A60) and the
 * non-pure virtuals are this unit's; the Wii implementation `sNetworkLibraryWii` lives in
 * `Network/sNetworkLibraryWii.cpp`.  Slot names are GUESSES from the bodies and the log strings
 * (`sNetworkLibrary::start`, `sNetworkLibrary::stop`).  size: 0x9C (sizeof; MWCC packs the derived
 * class's first byte into the tail at +0x9A) */
class sNetworkLibrary {
public:
    sNetworkLibrary();
    /* +0x08 */ virtual ~sNetworkLibrary();
    /* +0x0C */ virtual s32 logLevel(s32 level, const char* format, ...) = 0;
    /* +0x10 */ virtual s32 logError(const char* format, ...) = 0;
    /* +0x14 */ virtual s32 logWarning(const char* format, ...) = 0;
    /* +0x18 */ virtual void setLogLevel(s32 level);
    /* +0x1C */ virtual void update() = 0;
    /* +0x20 */ virtual s32 isLinkUp() = 0;
    /* +0x24 */ virtual s32 isOnline() = 0;
    /* +0x28 */ virtual s32 isBusy() = 0;
    /* +0x2C */ virtual void print(const char* format, ...) = 0;
    /* +0x30 */ virtual s32 init(sNetworkLibraryInitParam* param);
    /* +0x34 */ virtual void final();
    /* +0x38 */ virtual s32 start(s32 reserved, sNetworkLibraryError* error) = 0;
    /* +0x3C */ virtual s32 stop() = 0;
    /* +0x40 */ virtual void updateTime() = 0;
    /* +0x44 */ virtual void suspend() = 0;
    /* +0x48 */ virtual u32 hostToNet16(u32 value);
    /* +0x4C */ virtual u32 netToHost16(u32 value);
    /* +0x50 */ virtual u32 hostToNet32(u32 value);
    /* +0x54 */ virtual u32 netToHost32(u32 value);
    /* +0x58 */ virtual u64 hostToNet64(u64 value);
    /* +0x5C */ virtual u64 netToHost64(u64 value);
    /* +0x60 */ virtual f32 getElapsedTime() = 0;
    /* +0x64 */ virtual s64 getTime(s32 zone) = 0;
    /* +0x68 */ virtual void setTimeZone(s32 minutes);
    /* +0x6C */ virtual void convertTime(s64 time, NetworkDateTime* out);
    /* +0x70 */ virtual s64 dateToTime(NetworkDateTime* date);
    /* +0x74 */ virtual s32 getHostIdCount() = 0;
    /* +0x78 */ virtual void getHostId(s32 index, u8* out) = 0;
    /* +0x7C */ virtual s32 getDnsServerCount() = 0;
    /* +0x80 */ virtual void getDnsServer(s32 index, u8* out) = 0;
    /* +0x84 */ virtual void resume() = 0;
    /* +0x88 */ virtual NetworkResolverWii* createResolver() = 0;
    /* +0x8C */ virtual NetworkSocketHandle* createSocket() = 0;
    /* +0x90 */ virtual NetworkFileFetcher* createFetcher(u32 kind) = 0;

    /* Keeps the first failure (facility, step, SDK code); later ones are dropped until it is cleared. */
    void setLastError(s32 facility, s32 step, s32 code);
    /* Sets one of the three listen ports. */
    void setPort(s32 index, u16 port);
    /* Takes a free resolver slot and fills it from `createResolver`; NULL when all four are in use. */
    NetworkResolverWii* acquireResolver();

    /* 0x80794CC0 - the library singleton (the Wii implementation `constructNetworkWiiMediator` publishes). */
    static sNetworkLibrary* mpInstance;
    /* 0x80794CC4 - the network mediator `sNetworkLibraryWii::init` creates and `final` deletes. */
    static NetworkWiiMediator* mpMediator;
    /* 0x80794CC8 - the library's random generator, created by `init` and deleted by `final`. */
    static NetworkRandom* mpRandom;

    /* +0x04 */ NetworkSocketHandle* sockets[4];
    /* +0x14 */ NetworkResolverWii* resolvers[4];
    /* +0x24 */ NetworkFileFetcher* fetchers[4];
    /* +0x34 */ s32 logLevelValue;
    /* +0x38 */ sNetworkLibraryError lastError;
    /* +0x44 */ u32 field_0x44;
    /* +0x48 */ u32 field_0x48;
    /* +0x4C */ u8 flag_0x4C;
    /* +0x4D */ u8 flag_0x4D;
    /* +0x4E */ u8 pad_0x4E[0x02];
    /* +0x50 */ u8 mutex[0x3C];
    /* +0x8C */ s32 timeZoneMinutes;
    /* +0x90 */ u32 field_0x90;
    /* +0x94 */ u16 ports[3];
};

#ifdef __cplusplus
extern "C" {
#endif

/* The socket pool the transport peers register their socket with (0x804187F0 / 0x80418864).  Both
 * bodies walk a four-entry table at the manager's +0x04 and take the socket from the manager's own +0x8C
 * slot, so the argument is the manager the transport band reaches through `getNetworkLogger` (GUESS on
 * both names: they carry the acquire / release roles the two call sites give them, nothing in the range
 * spells them). */
NetworkSocketHandle* networkSocketPool_acquire(NetworkLogger* pool);
s32 networkSocketPool_release(NetworkLogger* pool, NetworkSocketHandle* socket);

/* 0x80418738 - releases and deletes a resolver the library holds (the Pat session's shutdown passes the
 * one it acquired): 0, or -1 when the library does not hold it.  Really `sNetworkLibrary`'s release half of
 * `acquireResolver`; the name and the parameter types are the callers' (integrator request filed). */
/* untyped: opaque handle passed through - the resolver the caller acquired */
s32 networkLog_destroyContext(NetworkSessionManagerLogger* log, void* context);

/* 0x80417C40 - an empty hook the library and the Pat holder constructors call with themselves and 1
 * (GUESS: a debug object registration compiled out of this build). */
/* untyped: opaque handle - any constructed object */
void registerNetworkObject(void* object, s32 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_SNETWORKLIBRARY_H */
