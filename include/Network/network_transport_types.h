/*
 * include/Network/network_transport_types.h - the types the Network transport band's units share.
 *
 * The band `.text` 0x803CCDF8..0x803D3CE8 was one unit (`Network/network_transport.cpp`) and is now eight
 * (docs/network-transport-split.md).  The classes are used by more than one of them - the peers by the socket
 * users and the session, the resolver base by the Wii resolver - so they live here once (rule 1) and each unit's
 * own function declarations live in that unit's header (`Network/<stem>.h`, rule 2); `Network/network_transport.h`
 * includes them all for the band's consumers.
 *
 * The types are reconstructed from the range's own instructions: the abstract peer whose error record every peer
 * constructor fills, the payload buffer, Udp and Mcs peers derived from it, the two resolver classes and the
 * session base - the classes whose tables the units' objects emit - plus the socket users and the byte stream.
 * Every field carries the offset the target addresses and a name from what the range stores in it; a field the
 * range never touches is `unused_`/`pad_` with its offset kept (rules 3-5).
 */

#ifndef NETWORK_NETWORK_TRANSPORT_TYPES_H
#define NETWORK_NETWORK_TRANSPORT_TYPES_H

#include "types.h"
#include "unsplit/SO.h"

/* The neighbouring units' classes, named but not defined here: this header is included BY
   `Network/fn_803D3CE8.h`, so it cannot include it back.  A forward declaration of the class name is
   all a pointer parameter needs. */
struct NetworkSessionStable;
struct NetworkStreamWriter;
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
    NETWORK_ERROR_PUT_TOO_BIG = 0x80030004,      /* `NetworkSessionStable::put: data too big` */
    NETWORK_ERROR_MCS_SOCKET = 0x80030011,       /* the Mcs peer's socket open/close/idle failed */
    NETWORK_ERROR_MCS_STATE = 0x80030012,        /* the Mcs peer's state machine met an armed flag */
    NETWORK_ERROR_PEER_SEND = 0x80030021,        /* a peer's send overflowed or the socket refused it */
    NETWORK_ERROR_PEER_RECEIVE = 0x80030022,     /* a peer's receive failed or read a zero length */
    NETWORK_ERROR_PUT_OVERFLOW = 0x80030032      /* `NetworkSessionStable::put: data overflow` */
};

/* The abstract peer the transport peers derive from (GUESS on the name: the class is the error-record
   holder every peer constructor chains, and its table carries the eight slots the three peers below
   fill).  Its vtable (0x805F94E0, 0x30 B: the deleting `destroy` and eight pure slots plus `slot_2C`, size padding: the +0x2C word is the size's alignment tail) is emitted here,
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
    /* +0x2C - not a code slot (retail relocates 9 per table): a pure slot only so the table is 0x30 B, the size's alignment tail */ virtual void slot_2C() = 0;

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

/* ---------------- the peer payload buffer --------------------------------------------------- */

/* The 0x2014-byte peer buffer (table 0x805F9510): the 0x2000-byte payload at +0x10 and the used-byte
   count at +0x2010.  Its `init` slot dispatches the clear through +0x28, which is MWCC's own virtual-call
   shape. */
class NetworkPeerBuffer : public NetworkPeerBase {
public:
    NetworkPeerBuffer();
    /* +0x08 */ virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C */ virtual void setContext(const void* context); /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C (docs/memory-dump.md names this one `clearBuffer`) */ virtual s32 move();
    /* +0x20 */ virtual void armDrop();
    /* +0x24 */ virtual s32 init();
    /* +0x28 */ virtual void reset();

    u8 payload_10[0x2000];           /* +0x0010 - the bytes queued for the socket */
    u32 used_2010;                   /* +0x2010 - bytes in use */
};   /* size: 0x2014 */

/* ---------------- the peer's socket handle ---------------------------------------------------- */

/* The six-byte peer address the transport keeps and logs: four address bytes and a port. */
struct NetworkPeerAddress {
    u8  ip_00[4];   /* +0x00 - the IPv4 address */
    u16 port_04;    /* +0x04 */
};   /* size: 0x06 */

/* The object the transport users keep at +0x04 and dispatch through: the +0x1C slot is a tail-jump
   (`bctr`), which is the shape MWCC gives a virtual whose result is returned directly, +0x28 is the
   receive-buffer clear, +0x34 the shutdown the release path drives and +0x38/+0x3C the datagram send and
   receive the Tcp and Udp users call.  Only the called slots are named; the gaps are the dispatch holes
   between them (GUESS: the names are offset-derived - the range calls them and nothing else does). */
class NetworkSocketHandle {
public:
    /* +0x08 */ virtual void slot_08();
    /* +0x0C (GUESS: name from the open path's mode store) */ virtual s32 open(u32 mode);
    /* +0x10 */ virtual void slot_10();
    /* +0x14 */ virtual void slot_14();
    /* +0x18 (GUESS: name from the open path's address store) */ virtual s32 setPeer(const NetworkPeerAddress* address);
    /* +0x1C (GUESS) */ virtual s32 closeSocket();
    /* +0x20 */ virtual void slot_20();
    /* +0x24 */ virtual void slot_24();
    /* +0x28 (GUESS) */ virtual s32 clearReceive();
    /* +0x2C */ virtual void slot_2C();
    /* +0x30 */ virtual void slot_30();
    /* +0x34 (GUESS: name from the release path's shutdown) */ virtual void shutdownSocket();
    /* +0x38 (GUESS: the send helpers pass bytes, a length and the peer address) */ virtual s32 send(const u8* data, s32 size, const NetworkPeerAddress* address);
    /* +0x3C (GUESS: the receive twin - a buffer, its capacity and the sender's address out) */ virtual s32 receive(u8* out, s32 size, NetworkPeerAddress* address);
};   /* size: 0x04 - only ever reached through a pointer in this unit */

/* What the Tcp and Udp users share: a vptr word and the socket they were opened on at +0x04.  Size
   evidence: `NetworkSingleTcp::open` copies the connection's address to +0x08 (`memcpy(&address_08, ..)`),
   so the base ends there; nothing reads the +0x00 word, which is why it stays `unused_`. */
struct NetworkSocketUser {
    void* unused_00;                 /* +0x00 */
    NetworkSocketHandle* handle_04;  /* +0x04 - the socket the user owns */
};   /* size: 0x08 */

/* ---------------- the Tcp connection and the Udp socket ---------------------------------------- */

/* The peer class the connection pumps: only its +0x18 slot is called from here. */
class NetworkPeerMcs;

/* `NetworkSingleTcp`: its socket at +0x04, the address it was opened on at +0x08, the four peers
   registered on it at +0x10, its own 0x2400-byte receive area at +0x20 and the byte count behind it. */
struct NetworkSingleTcp : public NetworkSocketUser {
    NetworkPeerAddress address_08;   /* +0x08..+0x0D - the address it was opened on */
    u8  pad_0E[0x02];                /* +0x0E..+0x0F */
    NetworkPeerMcs* peers_10[4];  /* +0x10..+0x1F - the peers registered on it */
    u8  recv_20[0x2400];             /* +0x20..+0x241F */
    u32 recvUsed_2420;               /* +0x2420 - bytes received */

    void disconnect();
    void move();
    s32 open(const NetworkPeerAddress* address);
    s32 close();
    s32 clearReceive();
    void release();
    s32 add(NetworkPeerMcs* peer);
    void remove(NetworkPeerMcs* peer);
    void clearReceiveBuffer();
    s32 send(const u8* data, s32 size);
    s32 getAvailableToRead();
    s32 getError();
};   /* size: 0x2424 */

/* `NetworkMultipleUdp`: one socket shared by up to four peers, each with its own address at +0x0E and
   its own 0x1770-byte reassembly buffer behind the one datagram buffer at +0x26.  The datagram is 0x5DC
   bytes because that is the capacity `NetworkMultipleUdp::move` reads with and `receivePackets` frames into; the
   map's 0x5E0 for the `.bss` scratch packet is dtk's gap to the next symbol (a 32-byte boundary), i.e. the
   same 0x5DC plus alignment - the source declares that scratch as [0x5E0] because [0x5DC] measures the
   `.bss` row at 70 %.  Size: (approximation - the last word the range touches, `used_63C4[3]`, ends at
   +0x63D4). */
struct NetworkMultipleUdp : public NetworkSocketUser {
    u8  pad_08[0x06];                /* +0x08..+0x0D */
    NetworkPeerAddress addresses_0E[4]; /* +0x0E..+0x25 - the four peers' addresses */
    u8  datagram_26[0x5DC];          /* +0x26..+0x601 - the datagram just received */
    u8  received_602[4][0x1770];     /* +0x602..+0x63C1 - each peer's queued bytes */
    u8  pad_63C2[0x02];              /* +0x63C2..+0x63C3 */
    s32 used_63C4[4];                /* +0x63C4..+0x63D3 - bytes queued per peer */

    void disconnect();
    void move();
    void release();
    void remove(const NetworkPeerAddress* address);
    void reset(s32 peerIndex);
    s32 send(s32 peerIndex, const u8* data, s32 size);
    s32 receive(s32 peerIndex, u8* out, s32 capacity);
    s32 getAvailableToRead();
    s32 getError();
};   /* size: 0x63D4 (approximation - see above) */

/* ---------------- the peers ------------------------------------------------------------------- */

/* The Udp peer (table 0x805F9540): the abstract peer, the peer index it owns on the socket at +0x10 and
   the socket at +0x14.  Its constructor lives outside this unit (the table is stored by an unowned
   function); the `destroy` defined here is the key function that makes MWCC emit the table. */
class NetworkPeerUdp : public NetworkPeerBase {
public:
    /* +0x08 */ virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C (docs/memory-dump.md names this one `setPeerAndSocket`) */ virtual void setContext(const void* context); /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 (`sendPackets`) */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 (`receivePackets`) */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C */ virtual s32 move();
    /* +0x20 */ virtual void armDrop();
    /* +0x24 */ virtual s32 init();
    /* +0x28 (GUESS: the reset helper's tail twin) */ virtual void reset();

    s32 peerIndex_10;                   /* +0x10 - the slot the peer owns on the Udp socket */
    NetworkMultipleUdp* udp_14;         /* +0x14 - the socket */
};   /* size: 0x18 (approximation - only the two fields the range touches are evidenced) */

/* The pair `setPeerAndSocket` copies in. */
struct NetworkPeerUdpBinding {
    s32 peerIndex;                      /* +0x00 */
    NetworkMultipleUdp* udp;            /* +0x04 */
};   /* size: 0x08 */

/* The Mcs peer (table 0x805F95E0, the `NetworkPeerMcs` the log strings name): the abstract peer, the
   one-byte flags, the state the connect machine walks, its own 0x2400-byte work area and the connection
   it registers with. */
class NetworkPeerMcs : public NetworkPeerBase {
public:
    NetworkPeerMcs(u8 armed);
    /* +0x08 */ virtual NetworkPeerBase* destroy(s16 flags);
    /* +0x0C (GUESS: the record it publishes itself from) */ virtual void setContext(const void* context); /* untyped: caller-owned payload - each derived peer reads its own record layout through it */
    /* +0x10 */ virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 */ virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 */ virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C (GUESS: the connect machine) */ virtual s32 move();
    /* +0x20 */ virtual void armDrop();
    /* +0x24 */ virtual s32 init();
    /* +0x28 (GUESS: closes the peer's connection) */ virtual void reset();

    u8  armed_10;                    /* +0x10 - set while the peer has something to drop */
    u8  pad_11[0x03];                /* +0x11..+0x13 */
    s32 state_14;                    /* +0x14 - the connect machine's state */
    u8  dropped_18;                  /* +0x18 - raised when the drop lands */
    u8  work_19[0x2400];             /* +0x19..+0x2418 - the peer's own work area */
    u32 workUsed_241C;               /* +0x241C - bytes in use */
    NetworkPeerAddress peerAddress_2420; /* +0x2420..+0x2425 - the address the peer publishes */
    u8  pad_2426[0x02];              /* +0x2426..+0x2427 */
    NetworkSingleTcp* connection_2428; /* +0x2428 - the connection the peer registers with */
    char label_242C[0x40];           /* +0x242C..+0x246B - the label the peer publishes */
    s32  labelSize_246C;             /* +0x246C - bytes of it in use */
};   /* size: 0x2470 */

/* The record a peer publishes itself from: the six-byte address at +0x00, its connection at +0x08 and
   the label it answers to with its used length (GUESS on the name - the setInfo slot and the stream
   writer are the only readers). */
typedef struct NetworkPeerInfo {
    NetworkPeerAddress address_00;      /* +0x00..+0x05 */
    u8  pad_06[0x02];                   /* +0x06..+0x07 */
    NetworkSingleTcp* connection_08;    /* +0x08 - the connection the peer registers with */
    char label_0C[0x40];                /* +0x0C..+0x4B - the label it publishes */
    s32  labelSize_4C;                  /* +0x4C - bytes of it in use */
} NetworkPeerInfo;   /* size: 0x50 */

/* The session base class (table 0x805F99A0, 0xA0 B: the deleting destructor, thirty-three pure slots
   and the four setters this unit defines - GUESS on the class name, evidenced only by `networkPeer_resetSlots`
   driving its +0x1C slot once per slot index and by the setters' slot positions).  The destructor is the
   key function that makes MWCC emit the table. */
class NetworkSessionBase {
public:
    virtual ~NetworkSessionBase();
    /* +0x0C */ virtual void slot_0C() = 0;
    /* +0x10 */ virtual void slot_10() = 0;
    /* +0x14 */ virtual void slot_14() = 0;
    /* +0x18 */ virtual void slot_18() = 0;
    /* +0x1C (GUESS) */ virtual void resetSlot(s8 index) = 0;
    /* +0x20 */ virtual void slot_20() = 0;
    /* +0x24 */ virtual void slot_24() = 0;
    /* +0x28 */ virtual void slot_28() = 0;
    /* +0x2C */ virtual void slot_2C() = 0;
    /* +0x30 */ virtual void slot_30() = 0;
    /* +0x34 */ virtual void slot_34() = 0;
    /* +0x38 */ virtual void slot_38() = 0;
    /* +0x3C */ virtual void slot_3C() = 0;
    /* +0x40 */ virtual void slot_40() = 0;
    /* +0x44 */ virtual void slot_44() = 0;
    /* +0x48 */ virtual void slot_48() = 0;
    /* +0x4C */ virtual void slot_4C() = 0;
    /* +0x50 (GUESS: the name follows the words it writes) */ virtual void setLimits(u32 maxHosts, u32 maxSubhosts);
    /* +0x54 (GUESS) */ virtual void setHostTimeout(f32 seconds);
    /* +0x58 (GUESS) */ virtual void setSubhostTimeout(f32 seconds);
    /* +0x5C (GUESS) */ virtual void setRate(s32 count, s32 divisor);
    /* +0x60 */ virtual void slot_60() = 0;
    /* +0x64 */ virtual void slot_64() = 0;
    /* +0x68 */ virtual void slot_68() = 0;
    /* +0x6C */ virtual void slot_6C() = 0;
    /* +0x70 */ virtual void slot_70() = 0;
    /* +0x74 */ virtual void slot_74() = 0;
    /* +0x78 */ virtual void slot_78() = 0;
    /* +0x7C */ virtual void slot_7C() = 0;
    /* +0x80 */ virtual void slot_80() = 0;
    /* +0x84 */ virtual void slot_84() = 0;
    /* +0x88 */ virtual void slot_88() = 0;
    /* +0x8C */ virtual void slot_8C() = 0;
    /* +0x90 */ virtual void slot_90() = 0;
    /* +0x94 */ virtual void slot_94() = 0;
    /* +0x98 */ virtual void slot_98() = 0;
    /* +0x9C */ virtual void slot_9C() = 0;
};   /* size: 0x04 - the state lives in unit-level globals */

/* ---------------- the peer that owns a byte stream -------------------------------------------- */

/* The sink the stream helpers hand a record to: only its +0x20 and +0x24 slots are called here, the first
   fills the buffer it is given and the second takes a record, each reporting how many bytes it moved
   (GUESS: offset-derived - the range never names the class, no `NetworkPeer*` class carries these two
   slots at +0x20/+0x24, and the helper stays valid through a pointer). */
class NetworkStreamSink {
public:
    /* +0x08 */ virtual void slot_08();
    /* +0x0C */ virtual void slot_0C();
    /* +0x10 */ virtual void slot_10();
    /* +0x14 */ virtual void slot_14();
    /* +0x18 */ virtual void slot_18();
    /* +0x1C */ virtual void slot_1C();
    /* +0x20 (GUESS) */ virtual s32 fill(u8* out, u32 size);
    /* +0x24 (GUESS) */ virtual s32 put(const u8* data, u32 size);
};   /* size: 0x04 - only ever reached through a pointer in this unit */

/* What a caller hands several of the stream's helpers: somewhere to put the bytes and how much room there
   is (a `u16`, because the length prefix it is compared against is one). */
typedef struct NetworkPeerRecord {
    /* untyped: caller-owned payload - the caller's own buffer */
    void* data_00;   /* +0x00 - the caller's buffer */
    u16   size_04;   /* +0x04 - its capacity, and the length actually copied */
} NetworkPeerRecord;   /* size: 0x08 (approximation: the two fields the range reads) */

/* The stream cursor the append and take helpers drive (GUESS on the class name: the map's helpers are
   `networkPeerStream_*` and the record is a byte stream; its callers reach it as an embedded object): the
   block at +0x04, its capacity at +0x08 and the cursor at +0x0C, which counts the bytes the block holds.
   +0x00 is never read here, so it is not evidenced as a vptr and the methods are not virtual. */
struct NetworkByteStream {
    void* unused_00;       /* +0x00 */
    u8* data_04;           /* +0x04 - the bytes the cursor writes into */
    u32 size_08;           /* +0x08 - capacity */
    u32 cursor_0C;         /* +0x0C - bytes held so far */

    u8* getData();
    u32 getSize();
    void pullRecord(NetworkStreamSink* sink);
    void putRecord(const NetworkPeerRecord* record);
    void putU32(u32 value);
    void putU16(u16 value);
    void putByte(u8 value);
    void forwardRecord(NetworkStreamSink* sink);
    void takeRecord(NetworkPeerRecord* record);
    void takeU32(u32* out);
    void readLength(u16* out);
    void takeByte(u8* out);
};   /* size: 0x10 */

/* The object whose destructor destroys two sub-objects: the first is an embedded small object at
   +0x30, the second an object of another class at +0x54.  Only those two offsets are evidenced. */
typedef struct NetworkPeerOwner {
    void* unused_00;                 /* +0x00 */
    u8    pad_04[0x2C];              /* +0x04..+0x2F */
    u8    small_30[0x10];            /* +0x30..+0x3F - the embedded small object's own storage */
    u8    pad_40[0x14];              /* +0x40..+0x53 */
    u8    sub_54[0x0C];              /* +0x54..+0x5F - the second owned sub-object */
} NetworkPeerOwner;   /* size: 0x60 (approximation - only the two sub-object offsets are evidenced) */

/* The lock a peer keeps at its own +0x04: the two mutex helpers wrap the OS mutex that starts four
   bytes into the object they are handed. */
typedef struct NetworkPeerLock {
    void* unused_00;         /* +0x00 */
    u8    mutex_04[0x1C];    /* +0x04..+0x1F - the OS mutex's own storage */
} NetworkPeerLock;   /* size: 0x20 (approximation - only the +0x04 offset is evidenced) */

/* ---------------- the name resolver ------------------------------------------------------------ */

/* The abstract resolver (table 0x805F9938, 0x20 B: the deleting destructor and five pure slots - the
   sixth entry of the derived table stays pure too): the name it resolves (at most 0x1FF bytes), the
   four-word address table and how many of them are live.  The destructor is the key function that makes
   MWCC emit the table. */
class NetworkResolverBase {
public:
    NetworkResolverBase();
    virtual ~NetworkResolverBase();
    /* +0x0C (GUESS) */ virtual s32 setName(const char* name) = 0;
    /* +0x10 (GUESS) */ virtual void resetCode() = 0;
    /* +0x14 (GUESS: the state machine `check()` in the log strings) */ virtual s32 check() = 0;
    /* +0x18 (GUESS) */ virtual void recordGet(s32 index, u32* out) = 0;
    /* +0x1C */ virtual void slot_1C() = 0;

    char  name_04[0x200];   /* +0x04..+0x203 */
    u32   records_204[4];   /* +0x204..+0x213 - the resolved addresses */
    u32   count_214;        /* +0x214 - how many of them are live */
};   /* size: 0x218 (evidence: the derived constructor stores its first own field, `code_218`, at +0x218
        and the base constructor's last store is the word at +0x214) */

/* `NetworkResolverWii` (table 0x805F9980): the base record plus the state of the lookup - the one-byte
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
    u8    thread_220[0x318];           /* +0x220..+0x537 - the OS thread the lookup runs on */
    u8    stack_538[0x1000];           /* +0x538..+0x1537 - its stack */
    s32   result_1538;                 /* +0x1538 - what the lookup returned */
    const char* lookupName_153C;       /* +0x153C - the name the thread resolves */
    SOAddrInfo hints_1540;             /* +0x1540..+0x155F - the lookup hints: only the family is set */
    SOAddrInfo* addrInfo_1560;         /* +0x1560 - the SDK's result list, freed when consumed */
};   /* size: 0x1564 */

/* ---------------- symbols no registered unit owns ---------------------------------------------- */

extern "C" {

/* The two unsplit helpers the destructors chain and the socket reader: symbols no registered unit owns, declared
   in the header the band's consumers already include.  They used to be forced here by a return-type clash
   between `include/unsplit/Network.h` and `Network/fn_803D3CE8.h` over the logger accessor `fn_803C9974` - now the
   map's `getNetworkLogger`, whose declaration the Pat landing left in the band header alone, so the clash is
   gone. */
/* untyped: opaque handle passed through - the caller hands an object of another band's layout */
void dtor_803C989C(void* self, s32 flags);
/* untyped: opaque handle passed through - the caller hands an object of another band's layout */
void dtor_803CA4E8(void* self, s32 flags);
/* untyped: opaque handle passed through - the socket object belongs to another band */
s32 getBytesAvailableToRead(void* handle);

}

/* `__dl__FPv`'s real spelling - every deleting destructor calls it.  It is a C++ operator, so it is declared
   outside the `extern "C"` block. */
/* untyped: opaque handle passed through - the freed allocation */
void operator delete(void* ptr) throw();

#endif /* NETWORK_NETWORK_TRANSPORT_TYPES_H */
