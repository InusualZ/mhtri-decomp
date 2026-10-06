/*
 * Network/network_socket_streams.h - `Network/network_socket_streams.cpp` defines members of `NetworkSingleTcp`,
 *   `NetworkMultipleUdp` and `NetworkByteStream`, declared here with the socket handle and the resolver base (rule 1).
 */

#ifndef NETWORK_NETWORK_SOCKET_STREAMS_H
#define NETWORK_NETWORK_SOCKET_STREAMS_H


class NetworkStreamSink;   /* Network/NetworkStreamSink.h - the byte stream hands records to it */

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
   so the base ends there.  The +0x00 word is the vptr: `networkPatReleaseBuffer` (0x803DE948) deletes the Tcp
   connection and the Udp socket through slot +0x08 with r4 = 1 (`delete p`), so the base declares the virtual
   destructor; it is defined nowhere in our source, so no table is emitted (rule 10). */
struct NetworkSocketUser {
    /* +0x08 */ virtual ~NetworkSocketUser();
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

    /* +0x08 */ virtual ~NetworkSingleTcp();
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
   same 0x5DC plus alignment - the source declares that scratch as [0x5E0] because [0x5DC] leaves the
   `.bss` row short of the target.  Size: (approximation - the last word the range touches, `used_63C4[3]`, ends at
   +0x63D4). */
struct NetworkMultipleUdp : public NetworkSocketUser {
    u8  pad_08[0x06];                /* +0x08..+0x0D */
    NetworkPeerAddress addresses_0E[4]; /* +0x0E..+0x25 - the four peers' addresses */
    u8  datagram_26[0x5DC];          /* +0x26..+0x601 - the datagram just received */
    u8  received_602[4][0x1770];     /* +0x602..+0x63C1 - each peer's queued bytes */
    u8  pad_63C2[0x02];              /* +0x63C2..+0x63C3 */
    s32 used_63C4[4];                /* +0x63C4..+0x63D3 - bytes queued per peer */

    /* +0x08 */ virtual ~NetworkMultipleUdp();
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

/* The lock a peer keeps at its own +0x04: the two mutex helpers wrap the OS mutex that starts four
   bytes into the object they are handed. */
typedef struct NetworkPeerLock {
    void* unused_00;         /* +0x00 */
    u8    mutex_04[0x1C];    /* +0x04..+0x1F - the OS mutex's own storage */
} NetworkPeerLock;   /* size: 0x20 (approximation - only the +0x04 offset is evidenced) */

/* ---------------- the name resolver ------------------------------------------------------------ */

/* The abstract resolver (table 0x805F9938: the deleting destructor and four pure slots; the 0x20 B the map
   gives each resolver table is 0x1C of table and the 8-byte `.data` alignment, since `NetworkResolverWii` is
   concrete - `sNetworkLibraryWii::createResolver` news it): the name it resolves (at most 0x1FF bytes), the
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

    char  name_04[0x200];   /* +0x04..+0x203 */
    u32   records_204[4];   /* +0x204..+0x213 - the resolved addresses */
    u32   count_214;        /* +0x214 - how many of them are live */
};   /* size: 0x218 (evidence: the derived constructor stores its first own field, `code_218`, at +0x218
        and the base constructor's last store is the word at +0x214) */


#endif /* NETWORK_NETWORK_SOCKET_STREAMS_H */
