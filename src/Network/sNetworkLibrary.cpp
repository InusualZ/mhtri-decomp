/*
 * Network/sNetworkLibrary.cpp - `sNetworkLibrary`, the platform-independent half of the network library singleton
 * (the strings say `SDD NETWORK LIBRARY`); the Wii half is `Network/sNetworkLibraryWii.cpp`.
 *
 * `.text` 0x80417BC0..0x8041891C.  Sections: extab 0x8001CD94..0x8001CE1C; extabindex 0x8003D764..0x8003D7DC;
 * .data 0x80602988..0x80602AF8 (the month-length table, the class's strings, then its table 0x80602A60);
 * .sbss 0x80794CC0..0x80794CD0 (`mpInstance`, `mpMediator`, `mpRandom`).
 *
 * BOUNDARY.  Split out of `Network/network_opening.cpp`: the `.data` order seam (vtable 0x80602978 followed by the
 * month table 0x80602988, `tudiscover dataorder`) pins this TU's start in `.text` 0x80417944..0x80417C44, and
 * the base constructor 0x80417BC0 is the first function there that touches the class (`fn_80417B30` before it is
 * an event-flag callback `network_layer_io` tail-calls).  The right seam 0x8041891C is the Wii half's
 * (its header).  The whole `.data` of the old unit is this class's (`dateToTime` reads the month table, the
 * constructor and `init` the strings); the old unit's `.rodata`/`.sdata2` are read by the opening steps and stayed.
 *
 * NAMES.  The class and every method name are GUESSES from the bodies and strings
 * (include/Network/sNetworkLibrary.h).  `networkLog_destroyContext` (really the release half of
 * `acquireResolver`) and `networkSocketPool_acquire`/`_release` (really members over the socket table) keep
 * the C names and parameter types their callers in other lanes use; integrator requests are filed.
 *
 * FLAGS.  `-O3` in place of the lib's `-O4,p` (configure.py: every row 77-93 % at `-O4,p`, all 100 % at `-O3`
 * except `init`, which `#pragma pool_data off` takes from 94.04 to 100); `#pragma peephole off` like the Wii
 * half (the reload of the table word through r3 before each virtual call).
 *
 * CALENDAR.  `convertTime`/`dateToTime` count days from 1 January of year 0; every unit constant is an unsigned
 * 64-bit macro (retail divides through `__div2u`/`__mod2u` and compares with a 64-bit subtract), the products
 * `n * 366ULL` keep the dead sign extension retail has, and `convertTime` adds each cycle as `(s16)(n * k)`
 * (retail `extsh`es the product, not the sum).  The month table `sMonthDays` (.data 0x80602988) is defined here.
 *
 * RESIDUALS.  `dateToTime` 99.94: the unrolled month loop computes `months - 8` before `months - 1` in retail
 * (two adjacent instructions swapped; loop/declaration/type variants measured, none reorders them).  `extab` is
 * 96 of 136 B: retail's constructor and destructor carry cleanup records for the member mutex
 * (`dtor_803CA338`), which needs the mutex as a class member with a destructor (NetworkSessionManager's type).
 */

#include "Network/sNetworkLibrary.h"
#include "Network/sNetworkLibraryWii.h"   /* sNetworkLibraryWii - the worker-thread bodies */
#include "Network/network_layer_io.h"          /* NetworkRandom */
#include "Network/NetworkSessionManager.h"     /* networkInstance_initMutex / dtor_803CA338 - the member mutex */
#include "unsplit/Network.h"                   /* getNetworkLogger */
#include "Runtime.PPCEABI.H/memset.h"
#include "sys_mem.h"

#pragma peephole off
#pragma pool_data off

/* The protocol of the objects the library owns (sockets, resolvers, fetchers): each is released through
 * its own slot, then deleted through the +0x08 deleting destructor.  Their tables belong to their own
 * bands; declared, never defined, so none is emitted here (rule 10). */
class sNetworkLibraryOwned {
public:
    /* +0x08 */ virtual ~sNetworkLibraryOwned();
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void releaseResolver();
    /* +0x14 */ virtual void releaseSocket();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual void pad_20();
    /* +0x24 */ virtual void releaseFetcher();
};   /* size: 0x04 (the object's leading table word) */

/* The calendar constants (the day count, the leap rule and the microsecond units are all unsigned 64-bit:
 * retail divides through `__div2u`/`__mod2u` and compares with an unsigned 64-bit subtract). */
#define NETWORK_USEC_PER_SECOND     1000000ULL
#define NETWORK_USEC_PER_MINUTE     60000000ULL
#define NETWORK_USEC_PER_HOUR       3600000000ULL
#define NETWORK_USEC_PER_DAY        86400000000ULL
#define NETWORK_DAYS_PER_YEAR       365ULL
#define NETWORK_DAYS_PER_4_YEARS    1461ULL
#define NETWORK_DAYS_PER_100_YEARS  36524ULL
#define NETWORK_DAYS_PER_400_YEARS  146097ULL

/* Days per month, common year then leap year (.data 0x80602988). */
static s32 sMonthDays[2][12] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

/* The base library's statics (.sbss 0x80794CC0..0x80794CCC, in this order). */
sNetworkLibrary* sNetworkLibrary::mpInstance;
NetworkWiiMediator* sNetworkLibrary::mpMediator;
NetworkRandom* sNetworkLibrary::mpRandom;

/* Sets up the member mutex and warns when a second instance is built. */
sNetworkLibrary::sNetworkLibrary()
{
    networkInstance_initMutex(mutex);
    registerNetworkObject(this, 1);
    if (mpInstance != NULL) {
        ((sNetworkLibrary*)getNetworkLogger())->logError("Already instance constructed.\n");
    }
    logLevelValue = 0;
}

/* untyped: opaque handle - any constructed object */
void registerNetworkObject(void* object, s32 kind)
{
}

/* Shuts the library down, drops the singleton and releases the mutex. */
sNetworkLibrary::~sNetworkLibrary()
{
    sNetworkLibrary::final();
    mpInstance = NULL;
    dtor_803CA338(mutex, -1);
}

void sNetworkLibrary::setLogLevel(s32 level)
{
    logLevelValue = level;
}

/* Prints the banner, clears every table and the error record, creates the random generator and sets
 * the two default ports. */
s32 sNetworkLibrary::init(sNetworkLibraryInitParam* param)
{
    print("_/_/_/_/ SDD NETWORK LIBRARY (DEBUG) _/_/_/_/\n");
    print(" Build: %s %s\n", "Jan 30 2010", "10:55:55");
    sockets[0] = NULL;
    sockets[1] = NULL;
    sockets[2] = NULL;
    sockets[3] = NULL;
    resolvers[0] = NULL;
    resolvers[1] = NULL;
    resolvers[2] = NULL;
    resolvers[3] = NULL;
    fetchers[0] = NULL;
    fetchers[1] = NULL;
    fetchers[2] = NULL;
    fetchers[3] = NULL;
    ports[0] = 0;
    ports[1] = 0;
    ports[2] = 0;
    memset(&lastError, 0, sizeof(lastError));
    field_0x44 = 0;
    field_0x48 = 0;
    flag_0x4C = 0;
    flag_0x4D = 0;
    timeZoneMinutes = 0;
    field_0x90 = 0;
    mpRandom = new NetworkRandom();
    setPort(0, 1024);
    setPort(1, 1025);
    return 0;
}

void sNetworkLibrary::setPort(s32 index, u16 port)
{
    ports[index] = port;
}

/* Releases and deletes every socket, resolver and fetcher still held, then the random generator. */
void sNetworkLibrary::final()
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (sockets[i] != NULL) {
            ((sNetworkLibraryOwned*)sockets[i])->releaseSocket();
            if (sockets[i] != NULL) {
                delete (sNetworkLibraryOwned*)sockets[i];
                sockets[i] = NULL;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        if (resolvers[i] != NULL) {
            ((sNetworkLibraryOwned*)resolvers[i])->releaseResolver();
            if (resolvers[i] != NULL) {
                delete (sNetworkLibraryOwned*)resolvers[i];
                resolvers[i] = NULL;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        if (fetchers[i] != NULL) {
            ((sNetworkLibraryOwned*)fetchers[i])->releaseFetcher();
            if (fetchers[i] != NULL) {
                delete (sNetworkLibraryOwned*)fetchers[i];
                fetchers[i] = NULL;
            }
        }
    }
    if (mpRandom != NULL) {
        delete mpRandom;
        mpRandom = NULL;
    }
}

void sNetworkLibrary::setLastError(s32 facility, s32 step, s32 code)
{
    if (lastError.facility == 0) {
        lastError.facility = facility;
        lastError.step = step;
        lastError.code = code;
    }
}

/* The console is big-endian, so the byte-order helpers hand the value back unchanged. */
u32 sNetworkLibrary::hostToNet16(u32 value)
{
    return value;
}

u32 sNetworkLibrary::netToHost16(u32 value)
{
    return value;
}

u32 sNetworkLibrary::hostToNet32(u32 value)
{
    return value;
}

u32 sNetworkLibrary::netToHost32(u32 value)
{
    return value;
}

u64 sNetworkLibrary::hostToNet64(u64 value)
{
    return value;
}

u64 sNetworkLibrary::netToHost64(u64 value)
{
    return value;
}

void sNetworkLibrary::setTimeZone(s32 minutes)
{
    timeZoneMinutes = minutes;
}

/* Splits a microsecond time (shifted by the time zone) into the calendar fields. */
void sNetworkLibrary::convertTime(s64 time, NetworkDateTime* out)
{
    s32 days;
    s32 leap;
    s32 month;
    s32 cycles;
    s32* lengths;
    u64 us;

    if (out == NULL) {
        return;
    }
    time += (s64)timeZoneMinutes * NETWORK_USEC_PER_MINUTE;
    days = time / NETWORK_USEC_PER_DAY;
    out->weekIndex = (days - 2) / 7;
    out->weekday = (days - 2) % 7;
    cycles = days / NETWORK_DAYS_PER_400_YEARS;
    days = days % NETWORK_DAYS_PER_400_YEARS;
    out->year = cycles * 400;
    if (days < 366ULL) {
        leap = 1;
    } else {
        cycles = (days - 1) / NETWORK_DAYS_PER_100_YEARS;
        days = (days - 1) % NETWORK_DAYS_PER_100_YEARS;
        out->year += (s16)(cycles * 100);
        if (days < 365ULL) {
            leap = 0;
        } else {
            cycles = (days + 1) / NETWORK_DAYS_PER_4_YEARS;
            days = (days + 1) % NETWORK_DAYS_PER_4_YEARS;
            out->year += (s16)(cycles * 4);
            if (days < 366ULL) {
                leap = 1;
            } else {
                cycles = (days - 1) / NETWORK_DAYS_PER_YEAR;
                days = (days - 1) % NETWORK_DAYS_PER_YEAR;
                out->year += (s16)cycles;
                leap = 0;
            }
        }
    }
    lengths = sMonthDays[leap];
    for (month = 0; month < 12; month++) {
        if (days < lengths[month]) {
            break;
        }
        days -= lengths[month];
    }
    out->month = month + 1;
    out->day = days + 1;
    us = time % NETWORK_USEC_PER_DAY;
    out->hour = us / NETWORK_USEC_PER_HOUR;
    us %= NETWORK_USEC_PER_HOUR;
    out->minute = us / NETWORK_USEC_PER_MINUTE;
    us %= NETWORK_USEC_PER_MINUTE;
    out->second = us / NETWORK_USEC_PER_SECOND;
}

/* Normalises the month, then turns the calendar fields back into microseconds (minus the time zone). */
s64 sNetworkLibrary::dateToTime(NetworkDateTime* date)
{
    s32 leap;
    s32 year;
    s32 n100;
    s32 n4;
    s32 counted;
    s32 before;
    s32 days;
    s32 month;
    s32 months;
    s64 time;

    leap = 0;
    while (date->month > 12) {
        date->year++;
        date->month -= 12;
    }
    while (date->month <= 0) {
        date->year--;
        date->month += 12;
    }
    year = date->year;
    before = year - 1;
    counted = before / 400;
    days = counted * 366ULL;
    n100 = before / 100 - counted;
    days += n100 * NETWORK_DAYS_PER_YEAR;
    counted += n100;
    n4 = before / 4 - counted;
    days += n4 * 366ULL;
    counted += n4;
    days += (before - counted) * NETWORK_DAYS_PER_YEAR;
    days += 366;
    if (year % 4 == 0) {
        if (year % 100 != 0) {
            leap = 1;
        } else if (year % 400 == 0) {
            leap = 1;
        }
    }
    months = date->month;
    for (month = 1; month < months; month++) {
        days += sMonthDays[leap][month - 1];
    }
    days += date->day - 1;
    time = days * NETWORK_USEC_PER_DAY + date->hour * NETWORK_USEC_PER_HOUR
         + (date->second * NETWORK_USEC_PER_SECOND + date->minute * NETWORK_USEC_PER_MINUTE);
    return time - (s64)timeZoneMinutes * NETWORK_USEC_PER_MINUTE;
}

NetworkResolverWii* sNetworkLibrary::acquireResolver()
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (resolvers[i] == NULL) {
            resolvers[i] = createResolver();
            return resolvers[i];
        }
    }
    return NULL;
}

/* Releases and deletes the resolver `context` when the library holds it: 0, or -1 when it does not. */
/* untyped: opaque handle passed through - the resolver the caller acquired */
s32 networkLog_destroyContext(NetworkSessionManagerLogger* log, void* context)
{
    sNetworkLibrary* library = (sNetworkLibrary*)log;
    s32 i;

    if (context == NULL) {
        return -1;
    }
    for (i = 0; i < 4; i++) {
        if (library->resolvers[i] == context) {
            ((sNetworkLibraryOwned*)context)->releaseResolver();
            if (library->resolvers[i] != NULL) {
                delete (sNetworkLibraryOwned*)library->resolvers[i];
                library->resolvers[i] = NULL;
            }
            return 0;
        }
    }
    return -1;
}

NetworkSocketHandle* networkSocketPool_acquire(NetworkLogger* pool)
{
    sNetworkLibrary* library = (sNetworkLibrary*)pool;
    s32 i;

    for (i = 0; i < 4; i++) {
        if (library->sockets[i] == NULL) {
            library->sockets[i] = library->createSocket();
            return library->sockets[i];
        }
    }
    return NULL;
}

s32 networkSocketPool_release(NetworkLogger* pool, NetworkSocketHandle* socket)
{
    sNetworkLibrary* library = (sNetworkLibrary*)pool;
    s32 i;

    if (socket == NULL) {
        return -1;
    }
    for (i = 0; i < 4; i++) {
        if (library->sockets[i] == socket) {
            ((sNetworkLibraryOwned*)socket)->releaseSocket();
            if (library->sockets[i] != NULL) {
                delete (sNetworkLibraryOwned*)library->sockets[i];
                library->sockets[i] = NULL;
            }
            return 0;
        }
    }
    return -1;
}
