/*
 * Network/NetworkFileFetcher.h - the fetcher and socket classes of four neighbouring units (`.text`
 * 0x803F6458..0x803F7538): the abstract file fetcher with its error record (`Network/NetworkFetcherBase.cpp`), the
 * Pat-server fetcher `NetworkFileFetcher` (`Network/NetworkFileFetcher.cpp`), the kind-2 fetcher whose every
 * operation succeeds at once (`Network/NetworkNullFetcher.cpp`), and the abstract socket base `NetworkSocketWii`
 * derives from (`Network/NetworkSocketBase.cpp`).
 */
#ifndef MHTRI_NETWORK_NETWORKFILEFETCHER_H
#define MHTRI_NETWORK_NETWORKFILEFETCHER_H

#include "types.h"
#include "Network/network_transport_types.h"   /* NetworkPeerAddress */

struct NetFetchError;              /* Network/network_pat_control.h */

/* The fetcher interface `sNetworkLibraryWii::createFetcher` hands out (GUESS on the name: the table 0x805FC820 is a
   destructor and six empty - pure - slots, the constructor clears the error record): the first failure's error
   triple at +0x04, which `copyError` reports. */
class NetworkFetcherBase {
public:
    NetworkFetcherBase();
    /* +0x08 */ virtual ~NetworkFetcherBase();
    /* +0x0C */ virtual s32 poll(u32* status) = 0;
    /* +0x10 */ virtual s32 open(s32 flags, const char* path) = 0;
    /* +0x14 */ virtual s32 read(void* buffer, s32 size) = 0;   /* untyped: byte range (the caller's file buffer) */
    /* +0x18 */ virtual s32 write() = 0;
    /* +0x1C */ virtual s32 list(u8* out) = 0;
    /* +0x20 */ virtual s32 remove() = 0;
    /* +0x24 */ virtual s32 close() = 0;

    void copyError(NetFetchError* error);
    void setError(u32 code, u32 detail, u32 reason);

    /* +0x04 */ u32 errorCode_04;
    /* +0x08 */ u32 errorDetail_08;
    /* +0x0C */ u32 errorReason_0C;
};   /* size: 0x10 (the kind-2 fetcher `createFetcher` allocates is the base alone) */

/* The remote-file client the fetch state machines drive (table 0x805FC880): `open`/`read`/`list`/`close` and
   `queryChecksum` each start a command (`command_18`) that `poll` steps through its own sub-steps (`step_1C`),
   sending the Pat binary requests and waiting for the reply bits the Pat callback sets in `replies_10`.  Names of
   the slots +0x18/+0x20 (`write`, `remove`: they only report "unsupported") and of every field are GUESSes from
   the bodies. */
class NetworkFileFetcher : public NetworkFetcherBase {
public:
    NetworkFileFetcher();
    /* +0x08 */ virtual ~NetworkFileFetcher();
    /* +0x0C */ virtual s32 poll(u32* status);
    /* +0x10 */ virtual s32 open(s32 flags, const char* path);
    /* +0x14 */ virtual s32 read(void* buffer, s32 size);   /* untyped: byte range (the caller's file buffer) */
    /* +0x18 */ virtual s32 write();
    /* +0x1C */ virtual s32 list(u8* out);
    /* +0x20 */ virtual s32 remove();
    /* +0x24 */ virtual s32 close();

    s32 queryChecksum(s32 flags, const char* path);
    s32 step();
    s32 stepOpen();
    s32 stepRead();
    s32 stepWrite();
    s32 stepList();
    s32 stepRemove();
    s32 stepClose();
    s32 stepChecksum();
    void onReply(u32 code, s32 requestId, s32 b, s32 c, const u32* message);

    /* +0x10 */ u32 replies_10;        /* reply bits: 1 failed, 2 refused, 0x40 checksum, 0x80 opened, 0x100 data,
                                          0x200 closed */
    /* +0x14 */ s32 requestId_14;      /* the id of the request in flight (-1 none) */
    /* +0x18 */ s32 command_18;        /* 0 idle, 1 open, 2 read, 3 write, 4 remove, 5 list, 6 close, 7 checksum */
    /* +0x1C */ u8 step_1C;            /* the command's sub-step (9 = failed) */
    /* +0x1D */ u8 pad_1D[3];
    /* +0x20 */ u32 transferred_20;    /* bytes read so far, or the checksum (`poll`'s first status word) */
    /* +0x24 */ u32 progress_24;       /* percent done (`poll`'s second status word) */
    /* +0x28 */ u8 fileId_28;          /* the numbered file (1..128) the path names */
    /* +0x29 */ u8 pad_29[3];
    /* +0x2C */ u32 fileSize_2C;
    /* +0x30 */ u32 handle_30;         /* the server's handle (or the checksum) */
    /* +0x34 */ u8* buffer_34;         /* `read`'s destination */
    /* +0x38 */ u32 bufferSize_38;
    /* +0x3C */ u32 offset_3C;         /* the file offset `read` starts at */
    /* +0x40 */ u8* list_40;           /* `list`'s destination */
    /* +0x44 */ u8 unused_44;
    /* +0x45 */ u8 pad_45[3];
};   /* size: 0x48 (the `__nw` size `createFetcher` and the fetch steps allocate) */

/* The kind-2 fetcher (table 0x805FC8A8, GUESS on the name): every operation succeeds at once and records no
   error; `createFetcher` builds it for kind 2. */
class NetworkNullFetcher : public NetworkFetcherBase {
public:
    NetworkNullFetcher();
    /* +0x08 */ virtual ~NetworkNullFetcher();
    /* +0x0C */ virtual s32 poll(u32* status);
    /* +0x10 */ virtual s32 open(s32 flags, const char* path);
    /* +0x14 */ virtual s32 read(void* buffer, s32 size);   /* untyped: byte range (the caller's file buffer) */
    /* +0x18 */ virtual s32 write();
    /* +0x1C */ virtual s32 list(u8* out);
    /* +0x20 */ virtual s32 remove();
    /* +0x24 */ virtual s32 close();
};   /* size: 0x10 */

/* The abstract socket (table 0x805FC8D0: a destructor and sixteen empty - pure - slots; GUESS on the name) that
   `NetworkSocketWii` implements: the source and the code of its last error at +0x04/+0x08.  The slot names come
   from `NetworkSocketWii`'s bodies (GUESSes); `NetworkSocketHandle` (`Network/network_transport_types.h`) is the
   transport users' view of the same object. */
class NetworkSocketBase {
public:
    NetworkSocketBase();
    /* +0x08 */ virtual ~NetworkSocketBase();
    /* +0x0C */ virtual s32 open(s32 mode) = 0;
    /* +0x10 */ virtual s32 openSecure(const char* host, const u8* rootCA, s32 rootCASize) = 0;
    /* +0x14 */ virtual s32 shutdown() = 0;
    /* +0x18 */ virtual s32 connect(const NetworkPeerAddress* address) = 0;
    /* +0x1C */ virtual s32 pollConnect() = 0;
    /* +0x20 */ virtual s32 listen(const NetworkPeerAddress* address) = 0;
    /* +0x24 */ virtual s32 accept() = 0;
    /* +0x28 */ virtual s32 isConnected() = 0;
    /* +0x2C */ virtual s32 getLocalAddress(NetworkPeerAddress* out) = 0;
    /* +0x30 */ virtual s32 getPeerAddress(NetworkPeerAddress* out) = 0;
    /* +0x34 */ virtual s32 close() = 0;
    /* +0x38 */ virtual s32 send(const u8* data, s32 size, const NetworkPeerAddress* address) = 0;
    /* +0x3C */ virtual s32 receive(u8* out, s32 size, NetworkPeerAddress* address) = 0;
    /* +0x40 */ virtual void getAnyAddress(NetworkPeerAddress* out) = 0;
    /* +0x44 */ virtual void getBroadcastAddress(NetworkPeerAddress* out) = 0;
    /* +0x48 */ virtual s32 getMode() = 0;

    /* +0x04 */ u32 errorSource_04;   /* the step that failed (0x8001xxxx), 0 when none */
    /* +0x08 */ s32 errorCode_08;     /* the SO/SSL result it failed with */
};   /* size: 0x0C */

#endif /* MHTRI_NETWORK_NETWORKFILEFETCHER_H */
