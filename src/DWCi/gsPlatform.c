/*
 * DWCi/gsPlatform.c - the GameSpy platform layer of this port: the socket API over SO (create, close, bind,
 *   receive, send, local name, address parsing, last error, select, the cached host resolver, readiness checks,
 *   the local host record, the private-address test), the socket start-up / shut-down hooks, the millisecond clock
 *   and sleep, and the memory hooks.
 *
 * RANGE. `.text` 0x8050B880..0x8050C5F0 (22 functions); `.data` 0x80630848..0x80630858 ("localhost"); `.bss`
 *   0x807613B8..0x807614D8 (memory hooks, local host record); `.sdata` 0x80794338..0x80794340 ("clear"); `.sbss`
 *   0x80795808..0x80795818 (host cache, alias list, last error). One TU, not three: the memory hooks' `.bss` word
 *   (0x807613B8) precedes the socket statics while their `.text` follows the socket code, and
 *   `DWCi_socketLookupHost` calls through the hooks in place (retail inlines `gsimalloc`/`gsifree` there). The
 *   left edge follows the hash table (`DWCi/dwc_nasfunc.c`, `svlResult`'s padded `.sbss` word); the right edge is
 *   the availability probe (`DWCi/gsAvailable.c`; "localhost" ends 0x80630852, the next object starts 8-aligned).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names where known (`SocketStartUp`, `SocketShutDown`, `current_time`, `msleep`,
 *   `gsiMemoryCallbacksSet`, `gsimalloc`, `gsirealloc`, `gsifree`, `gsiSocketError`); the `DWCi_socket*` wrappers are
 *   GUESSes from their bodies (the SDK's socket / closesocket / bind / recvfrom / sendto / getsockname / inet_addr /
 *   GOAGetLastError / select / gethostbyname / CanReceiveOnSocket / CanSendOnSocket / getlocalhost / IsPrivateIP),
 *   GUESS `DWCi_hostAddressIsPrivate`, `DWCi_hostCache`, `DWCi_localHostAliases`, `DWCi_localHostEntry`,
 *   `DWCi_localAddressList`, `DWCi_localAddresses`, `gsiMemoryCallbacks`. The file name `gsPlatform.c` is a GUESS (the
 *   GameSpy common platform layer; the SDK spreads these over gsPlatformSocket / gsPlatformUtil / gsMemory, but the
 *   retail bytes link them as one object; no `__FILE__` string).
 * RESIDUALS. `DWCi_socketLookupHost` keeps the name hash in r0 where retail colours it r28 (the unrolled hash loop
 *   shifts every temporary by one register). `DWCi_socketGetLocalHostEntry` places its two SO option words 4 bytes
 *   lower on the stack (no declaration order moves them). The resolver cache and the local host record are function
 *   statics, so the object names them `@LOCAL@<function>@<name>` where the map has `DWCi_hostCache`,
 *   `DWCi_localHostAliases`, `DWCi_localHostEntry`, `DWCi_localAddressList`, `DWCi_localAddresses` (same bytes, same
 *   offsets; flipcheck reports the names as missing).
 * SHAPES. Socket results go through the static inline `DWCi_socketResult`; the resolver calls the memory hooks
 *   through static inlines (retail expands `gsimalloc`/`gsifree` there). The resolver cache is a static of
 *   `DWCi_socketLookupHost` and the local host record, its alias list and address tables are statics of
 *   `DWCi_socketGetLocalHostEntry` (retail addresses each by its own symbol, and lays them out in that order); the
 *   memory hooks carry a zero initialiser and the last socket error is a tentative definition, which places them
 *   first in `.bss` and last in `.sbss` as retail has them (playbook idea 158).
 */
#include "DWCi/gsPlatform.h"
#include "DWCi/dwc_nasfunc.h"
#include "unsplit/OS.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "MSL_C/alloc.h"
#include "MSL_C/strstr.h"
#include "DWCi/natneg.h"

/* The GameSpy allocator hooks (`gsiMemoryCallbacksSet` stores, `gsimalloc`/`gsifree`/`gsirealloc` call). */
typedef struct GSIMemoryCallbacks {
    /* +0x00 */ GSIMallocFn mallocFn;
    /* +0x04 */ GSIFreeFn freeFn;
    /* +0x08 */ GSIReallocFn reallocFn;
    /* +0x0C */ GSIMemalignFn memalignFn;
} GSIMemoryCallbacks; /* size: 0x10 */

GSIMemoryCallbacks gsiMemoryCallbacks = {0};

/* The last socket error (a tentative definition: MWCC lays it out after the function statics, as retail does). */
s32 gsiSocketError;


/* The SO result as the socket API answers it: the value, or -1 with the error recorded. */
static inline int DWCi_socketResult(int result) {
    if (result >= 0) {
        return result;
    }
    gsiSocketError = result;
    return -1;
}

/* 0x8050B880 (0x38): a new socket; -1 with the error recorded on failure. */
int DWCi_socketCreate(int domain, int type, int protocol) {
    return DWCi_socketResult(SOSocket(domain, type, 0));
}

/* 0x8050B8C0 (0x34): closes a socket. */
int DWCi_socketClose(int sock) {
    int result = SOClose(sock);
    return DWCi_socketResult(result);
}

/* 0x8050B900 (0x78): binds a socket to `addr` (nothing to do for port 0). */
int DWCi_socketBind(int sock, const SOSockAddrIn* addr, int len) {
    SOSockAddrIn local;
    int result;

    if (addr->port == 0) {
        return 0;
    }
    memcpy(&local, addr, sizeof(SOSockAddrIn));
    local.len = len;
    result = SOBind(sock, &local);
    return DWCi_socketResult(result);
}

/* 0x8050B980 (0x60): receives one datagram without blocking. */
int DWCi_socketRecvFrom(int sock, u8* buf, int len, u32 flags, SOSockAddrIn* from, int* fromLen) {
    int result;

    from->len = *fromLen;
    result = SORecvFrom(sock, buf, len, 4, from);
    *fromLen = from->len;
    return DWCi_socketResult(result);
}

/* 0x8050B9E0 (0x80): sends one datagram to `to`. */
/* untyped: byte range - the datagram */
int DWCi_socketSendTo(int sock, const void* buf, int len, u32 flags, const SOSockAddrIn* to, int toLen) {
    SOSockAddrIn local;
    int result;

    memcpy(&local, to, sizeof(SOSockAddrIn));
    local.len = toLen;
    result = SOSendTo(sock, (const u8*)buf, len, flags, &local);
    return DWCi_socketResult(result);
}

/* 0x8050BA60 (0x5C): the address a socket is bound to. */
int DWCi_socketGetLocalName(int sock, SOSockAddrIn* addr, int* len) {
    int result;

    addr->len = *len;
    result = SOGetSockName(sock, addr);
    *len = addr->len;
    return DWCi_socketResult(result);
}

/* 0x8050BAC0 (0x38): a dotted address as a network-order word, or -1. */
u32 DWCi_socketResolveAddress(char* host) {
    u32 addr;
    if (SOInetAtoN(host, (u8*)&addr) == 0) {
        return (u32)-1;
    }
    return addr;
}

/* 0x8050BB00 (0x8): the last socket error. */
int DWCi_socketGetLastError(int sock) {
    return gsiSocketError;
}

/* 0x8050BB10 (0x128): polls one socket for reading, writing and errors (any of the flag pointers may be NULL). */
int DWCi_socketSelect(int sock, int* readFlag, int* writeFlag, int* exceptFlag) {
    SOPollFD fd;
    int result;

    fd.fd = sock;
    fd.events = 0;
    if (readFlag != NULL) {
        fd.events |= 1;
    }
    if (writeFlag != NULL) {
        fd.events |= 8;
    }
    fd.revents = 0;
    result = SOPoll(&fd, 1, 0);
    if (result < 0) {
        return -1;
    }
    if (readFlag != NULL) {
        if (result > 0 && (fd.revents & 0x41)) {
            *readFlag = 1;
        } else {
            *readFlag = 0;
        }
    }
    if (writeFlag != NULL) {
        if (result > 0 && (fd.revents & 8)) {
            *writeFlag = 1;
        } else {
            *writeFlag = 0;
        }
    }
    if (exceptFlag != NULL) {
        if (result > 0 && (fd.revents & 0x20)) {
            *exceptFlag = 1;
        } else {
            *exceptFlag = 0;
        }
    }
    return result;
}

/* The GameSpy allocator hooks called in place (the retail socket TU expands `gsimalloc`/`gsifree` inline). */
/* untyped: caller-owned payload - a raw block */
static inline void* gsiMallocInline(u32 size) {
    return gsiMemoryCallbacks.mallocFn(size);
}

/* untyped: caller-owned payload - a raw block */
static inline void gsiFreeInline(void* block) {
    if (block != NULL) {
        gsiMemoryCallbacks.freeFn(block);
    }
}

/* The cache record of one resolved host: the resolver's `hostent` fields, then the host name it was asked for. */
typedef struct DWCiHostCacheEntry {
    /* +0x00 */ char* name;
    /* +0x04 */ char** aliases;
    /* +0x08 */ s16 addrType;
    /* +0x0A */ s16 length;
    /* +0x0C */ char** addrList;
    /* +0x10 */ char* hostName;
} DWCiHostCacheEntry; /* size: 0x14 */

#define DWCI_HOST_CACHE_SIZE 31

/* 0x8050BC40 (0x584): resolves `host` through a 31-slot cache of resolver answers (quadratic probing); the
 * name "clear" frees the cache instead. */
DWCiHostEntry* DWCi_socketLookupHost(char* host) {
    /* The resolver cache: 31 slots of `DWCiHostCacheEntry`. */
    static struct DWCiHostCacheEntry** DWCi_hostCache = NULL;
    u32 i;
    int j;
    int hash;
    u32 probe;
    int slot;
    int count;
    int length;
    DWCiHostCacheEntry* entry;
    DWCiHostCacheEntry** cache;
    DWCiHostEntry* result;

    if (strncmp("clear", host, strlen("clear")) == 0) {
        if (DWCi_hostCache != NULL) {
            for (i = 0; i < DWCI_HOST_CACHE_SIZE; i++) {
                if (DWCi_hostCache[i] != NULL) {
                    for (j = 0; DWCi_hostCache[i]->addrList[j] != NULL; j++) {
                        gsiFreeInline(DWCi_hostCache[i]->addrList[j]);
                    }
                    gsiFreeInline(DWCi_hostCache[i]->addrList[j]);
                    gsiFreeInline(DWCi_hostCache[i]->addrList);
                    gsiFreeInline(DWCi_hostCache[i]->hostName);
                }
                gsiFreeInline(DWCi_hostCache[i]);
            }
            gsiFreeInline(DWCi_hostCache);
            DWCi_hostCache = NULL;
        }
        return NULL;
    }
    if (DWCi_hostCache == NULL) {
        DWCi_hostCache = (DWCiHostCacheEntry**)gsiMallocInline(DWCI_HOST_CACHE_SIZE * sizeof(DWCiHostCacheEntry*));
        memset(DWCi_hostCache, 0, DWCI_HOST_CACHE_SIZE * sizeof(DWCiHostCacheEntry*));
    }
    length = strlen(host);
    hash = 0;
    for (i = 0; i < (u32)length; i++) {
        hash += host[i] << ((i % 8) * 4);
    }
    hash = (u32)hash % DWCI_HOST_CACHE_SIZE;
    cache = DWCi_hostCache;
    for (probe = 0; probe < 15; probe++) {
        entry = cache[(u32)(hash + probe * probe) % DWCI_HOST_CACHE_SIZE];
        if (entry != NULL && strcmp(entry->hostName, host) == 0) {
            return (DWCiHostEntry*)entry;
        }
    }
    if (cache[hash] != NULL) {
        for (probe = 1; probe < 15; probe++) {
            slot = (u32)(hash + probe * probe) % DWCI_HOST_CACHE_SIZE;
            if (cache[slot] == NULL) {
                hash = slot;
                break;
            }
        }
        if (probe == 15) {
            return SOGetHostByName(host);
        }
    }
    result = SOGetHostByName(host);
    if (result == NULL) {
        return NULL;
    }
    DWCi_hostCache[hash] = (DWCiHostCacheEntry*)gsiMallocInline(sizeof(DWCiHostCacheEntry));
    count = 0;
    while (result->hosts[count] != NULL) {
        count++;
    }
    DWCi_hostCache[hash]->addrType = 2;
    DWCi_hostCache[hash]->length = result->length;
    DWCi_hostCache[hash]->name = NULL;
    DWCi_hostCache[hash]->aliases = NULL;
    DWCi_hostCache[hash]->addrList = (char**)gsiMallocInline((count + 1) * sizeof(char*));
    for (j = 0; result->hosts[j] != NULL; j++) {
        DWCi_hostCache[hash]->addrList[j] = (char*)gsiMallocInline(result->length);
        memcpy(DWCi_hostCache[hash]->addrList[j], result->hosts[j], result->length);
    }
    DWCi_hostCache[hash]->addrList[j] = NULL;
    DWCi_hostCache[hash]->hostName = (char*)gsiMallocInline(strlen(host) + 1);
    strcpy(DWCi_hostCache[hash]->hostName, host);
    return result;
}

/* 0x8050C1D0 (0x48): non-zero when the socket has a datagram waiting. */
int DWCi_socketHasData(int sock) {
    int readFlag = 0;
    if (DWCi_socketSelect(sock, &readFlag, NULL, NULL) == 1) {
        return readFlag;
    }
    return 0;
}

/* 0x8050C220 (0x48): non-zero when the socket can be written to. */
int DWCi_socketIsUsable(int sock) {
    int writeFlag = 0;
    if (DWCi_socketSelect(sock, NULL, &writeFlag, NULL) == 1) {
        return writeFlag;
    }
    return 0;
}

/* 0x8050C270 (0x1E0): the resolver record of this machine ("localhost" with every interface address), or NULL
 * when the interface count cannot be read. */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void) {
    static DWCiHostEntry DWCi_localHostEntry;
    static char* DWCi_localHostAliases = NULL;
    static DWCiHostAddr* DWCi_localAddressList[16];
    static u8 DWCi_localAddresses[16][12];
    s32 count;
    s32 length;
    s32 i;

    length = 4;
    SOGetInterfaceOpt(NULL, 0xFFFE, 0x4002, &count, &length);
    if ((u32)(count - 1) > 15) {
        return NULL;
    }
    memset(DWCi_localAddresses, 0xBE, count * 12);
    length = count * 12;
    SOGetInterfaceOpt(NULL, 0xFFFE, 0x4003, DWCi_localAddresses, &length);
    DWCi_localHostEntry.name = "localhost";
    DWCi_localHostEntry.aliases = &DWCi_localHostAliases;
    DWCi_localHostEntry.addrType = 2;
    DWCi_localHostEntry.length = count;
    for (i = 0; i < count; i++) {
        DWCi_localAddressList[i] = (DWCiHostAddr*)DWCi_localAddresses[i];
    }
    DWCi_localAddressList[i] = NULL;
    DWCi_localHostEntry.hosts = DWCi_localAddressList;
    return &DWCi_localHostEntry;
}

/* 0x8050C450 (0x74): non-zero when the address is a private one (10/8, 172.16/12, 192.168/16). */
int DWCi_hostAddressIsPrivate(DWCiHostAddr* host) {
    u32 addr = DWCi_socketTokenFromPeer(host->addr);
    int first = addr >> 24;
    int second = (addr >> 16) & 0xFF;

    if (first == 10) {
        return 1;
    }
    if (first == 172 && second >= 16 && second <= 31) {
        return 1;
    }
    if (first == 192 && second == 168) {
        return 1;
    }
    return 0;
}

/* ---- gsPlatform -------------------------------------------------------------------------------------- */

/* 0x8050C4D0 (0x4): the socket layer start-up (nothing to do on this platform). */
void SocketStartUp(void) {
}

/* 0x8050C4E0 (0x4): the socket layer shut-down (nothing to do on this platform). */
void SocketShutDown(void) {
}

/* 0x8050C4F0 (0x48): the time base in milliseconds. */
u32 current_time(void) {
    return (u32)(OSGetTime() / (OS_BUS_CLOCK / 4 / 1000));
}

/* 0x8050C540 (0x3C): sleeps the calling thread for `msec` milliseconds. */
void msleep(u32 msec) {
    OSSleepTicks((s64)msec * (s64)(OS_BUS_CLOCK / 4 / 1000));
}


/* ---- gsMemory ---------------------------------------------------------------------------------------- */

/* 0x8050C580 (0x1C): installs the allocator callbacks. */
void gsiMemoryCallbacksSet(GSIMallocFn mallocFn, GSIFreeFn freeFn, GSIReallocFn reallocFn, GSIMemalignFn memalignFn) {
    gsiMemoryCallbacks.mallocFn = mallocFn;
    gsiMemoryCallbacks.freeFn = freeFn;
    gsiMemoryCallbacks.reallocFn = reallocFn;
    gsiMemoryCallbacks.memalignFn = memalignFn;
}

/* 0x8050C5A0 (0x10): allocates through the installed callback. */
/* untyped: caller-owned payload - a raw block */
void* gsimalloc(u32 size) {
    return gsiMemoryCallbacks.mallocFn(size);
}

/* 0x8050C5B0 (0x14): reallocates through the installed callback. */
/* untyped: caller-owned payload - a raw block */
void* gsirealloc(void* block, u32 size) {
    return gsiMemoryCallbacks.reallocFn(block, size);
}

/* 0x8050C5D0 (0x20): frees through the installed callback (NULL is ignored). */
/* untyped: caller-owned payload - a raw block */
void gsifree(void* block) {
    if (block == NULL) {
        return;
    }
    gsiMemoryCallbacks.freeFn(block);
}
