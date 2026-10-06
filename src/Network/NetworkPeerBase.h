/*
 * Network/NetworkPeerBase.h - the abstract peer `Network/NetworkPeerBase.cpp` defines, its error record and the
 *   error-record accessors.
 */

#ifndef NETWORK_NetworkPeerBase_H
#define NETWORK_NetworkPeerBase_H

#include "Network/NetworkUniqueId.h"

/* The neighbouring units' classes, named but not defined here (a forward declaration is all a pointer needs). */
class NetworkSessionStable;
class NetworkStreamWriter;
struct NetworkPeerInfo;

/* ---------------- the peer's error record --------------------------------------------------- */

/* The `source` word of an error record when the failing operation stores a constant: which transport
   operation failed (the record's field is a pointer because other peers store the table they were
   given, so the constants cross as `(const void*)`).  The values are the constants the range stores as immediates (the target relocates them against `@eti_` extabindex
   rows only because dtk derives a group-relative name for the `lis`/`addi` pair - they are not
   addresses).  GUESS on every name: each one is read off the operation that stores it. */
enum NetworkPeerErrorSource {
    NETWORK_ERROR_NONE = 0,
    NETWORK_ERROR_UDP_UNATTACHED = 0x80030002,   /* the udp peer has no udp object to send/receive on */
    NETWORK_ERROR_SESSION_CONTROL = 0x80030003,  /* `NetworkSessionStable::execControlOne` refused a control message */
    NETWORK_ERROR_PUT_TOO_BIG = 0x80030004,      /* `NetworkSessionStable::put: data too big` */
    NETWORK_ERROR_MCS_SOCKET = 0x80030011,       /* the Mcs peer's socket open/close/idle failed */
    NETWORK_ERROR_MCS_STATE = 0x80030012,        /* the Mcs peer's state machine met an armed flag */
    NETWORK_ERROR_PEER_SEND = 0x80030021,        /* a peer's send overflowed or the socket refused it */
    NETWORK_ERROR_PEER_RECEIVE = 0x80030022,     /* a peer's receive failed or read a zero length */
    NETWORK_ERROR_PUT_OVERFLOW = 0x80030032,     /* `NetworkSessionStable::put: data overflow` */
    NETWORK_ERROR_SLEEP_TIMEOUT = 0x80030036,    /* a peer slept longer than it announced (`move`) */
    NETWORK_ERROR_SESSION_DROPPED = 0x80030037,  /* a peer said it dropped (`execControlOne` case 5) */
    NETWORK_ERROR_SESSION_TIMEOUT = 0x80030039,  /* too many peers timed out (`move`) */
    NETWORK_ERROR_SESSION_KICKED = 0x8003003A,   /* `leave` / `kick` */
    NETWORK_ERROR_CONNECT_TIMEOUT = 0x8003003B,  /* a peer could not be established within the subhost timeout (`move`) */
    NETWORK_ERROR_HOST_TIMEOUT = 0x8003003F,     /* no host seen within the timeout (`move`) */
    NETWORK_ERROR_CONNECT_FAILED = 0x80030041,   /* the slot could not be established (`move`) */
    NETWORK_ERROR_PEER_CLOSED = 0x80030042,      /* a peer was closed: too few peers left, or it cannot be established (`move`) */
    NETWORK_ERROR_PEER_LEFT = 0x80030044         /* the peer left (`setNetworkConnectionEvent`, `execControlOne`) */
};

/* The abstract peer the transport peers derive from (GUESS on the name: the class is the error-record
   holder every peer constructor chains, and its table carries the eight slots the three peers below
   fill).  Its vtable (0x805F94E0, 0x2C B: the deleting `destroy` and eight pure slots; the object's 0x30 claim ends in alignment fill) is emitted here,
   from the `destroy` this unit defines.  The pure slots carry the *union* of the derived peers'
   signatures - a derived slot with a different parameter list would be a new virtual and would move
   every later slot - so the peers that ignore an argument simply leave it unused (GUESS on every
   parameter list: the buffer/Mcs `send` and the buffer/Udp `put` take fewer arguments than the widest
   spelling).  The record is the three words a failing operation fills in **once**
   (`networkPeerError_set` refuses to overwrite a filled record). */
class NetworkPeerBase {
public:
    NetworkPeerBase();
    /* +0x08 the deleting destructor as a plain virtual: the peers chain it with flags 0 by hand (the
       hand-written `destroy` shape the band's other peers use), so no destructor vptr store is emitted */
    virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C (GUESS: the Udp peer binds its slot here, the Mcs peer its record, the buffer ignores it) */
    virtual void setContext(const void* context) = 0; /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 (GUESS) */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind) = 0;
    /* +0x14 (GUESS) */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind) = 0;
    /* +0x18 (GUESS: the Tcp pump hands it a packet, its length and three zeros) */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c) = 0;
    /* +0x1C (GUESS: the Mcs peer's connect machine; the others clear their queue and report usable) */ virtual s32 move() = 0;
    /* +0x20 (GUESS: only the Mcs peer does anything) */ virtual void armDrop() = 0;
    /* +0x24 (GUESS: the peers clear through the +0x28 slot and report usable) */ virtual s32 init() = 0;
    /* +0x28 (GUESS: empties the peer's payload/queue; the Mcs peer's close) */ virtual void reset() = 0;

    const void* source_04;  /* +0x04 - what failed: a `NetworkPeerErrorSource` value or the table the failing operation was given */
    u32 argument_08;        /* +0x08 - its size or argument */
    u32 code_0C;            /* +0x0C - the error code */
};   /* size: 0x10 */

/* The same three words as a caller-owned record: the range copies them out intact. */
typedef struct NetworkPeerErrorRecord {
    const void* source;  /* +0x00 */
    u32 argument;  /* +0x04 */
    u32 code;      /* +0x08 */
} NetworkPeerErrorRecord;   /* size: 0x0C */

/* The socket reader `getBytesAvailableToRead` is owned by `Network/NetworkCommunityPat.cpp` and declared
   in `Network/NetworkCommunityPat.h` (rule 2). */

/* `__dl__FPv`'s real spelling - every deleting destructor calls it.  It is a C++ operator, so it is declared
   outside the `extern "C"` block. */
/* untyped: opaque handle passed through - the freed allocation */
void operator delete(void* ptr) throw();


extern "C" {

void networkPeerError_get(NetworkPeerBase* self, NetworkPeerErrorRecord* out);
void networkPeerError_clear(NetworkPeerBase* self);
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code);
}

#endif /* NETWORK_NetworkPeerBase_H */
