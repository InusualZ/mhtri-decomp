/*
 * include/Network/network_transport.h - the types and declarations `Network/network_transport.cpp`
 * owns and the rest of the Network band reaches through.
 *
 * The types are reconstructed from the range's own instructions: the error-record base every peer
 * constructor in the band fills, the 0x2000-byte payload buffer with its head peer/socket pair, the
 * peer that owns a byte stream, and the peer that owns a four-entry record table.  Every field
 * carries the offset the target addresses and a name from what the range stores in it; a field the
 * range never touches is `unused_`/`pad_` with its offset kept (rules 3-5).
 *
 * The declarations below are the symbols this unit owns that other units call.  They used to be
 * declared in `include/unsplit/Network.h` and `include/Network/fn_803D3CE8.h`; after this unit's
 * range was claimed they became rule-2 findings in those files (a declaration belongs with the TU
 * that defines the symbol), so they moved here and both headers include this one.
 */

#ifndef NETWORK_NETWORK_TRANSPORT_H
#define NETWORK_NETWORK_TRANSPORT_H

#include "types.h"

/* The neighbouring units' classes, named but not defined here: this header is included BY
   `Network/fn_803D3CE8.h`, so it cannot include it back.  A forward declaration of the class name is
   all a pointer parameter needs. */
struct NetworkSessionStable;
struct NetworkStreamWriter;
/* the Pat band's opaque record types, which only ever cross as pointers (`receivePatInterfaces` /
   `flushPatRequests` below are the pumps `NetworkSessionManagerPat` drives) */
class PatReceiver;
class PatRequestQueue;

/* ---------------- the peer's error record --------------------------------------------------- */

/* The 0x10-byte base the transport peers derive from: the vptr their constructors store, then the
   three-word record a failing operation fills in **once** (the range's `networkPeerError_set`
   refuses to overwrite a filled record). */
typedef struct NetworkPeerError {
    void* unused_00;        /* +0x00 - the vptr every constructor in the band stores */
    const void* source_04;  /* +0x04 - the table or string the failing operation was given */
    u32 argument_08;        /* +0x08 - its size or argument */
    u32 code_0C;            /* +0x0C - the error code */
} NetworkPeerError;   /* size: 0x10 */

/* The same three words as a caller-owned record: the range copies them out intact. */
typedef struct NetworkPeerErrorRecord {
    const void* source;  /* +0x00 */
    u32 argument;        /* +0x04 */
    u32 code;            /* +0x08 */
} NetworkPeerErrorRecord;   /* size: 0x0C */

/* ---------------- the peer payload buffer --------------------------------------------------- */

/* The peer/socket pair `setPeerAndSocket` stores in the payload buffer's first two words, then the
   0x1FF8 bytes of payload behind it. */
typedef struct NetworkPeerPayload {
    u32 peer_00;           /* +0x00 */
    u32 socket_04;         /* +0x04 */
    u8  data_08[0x1FF8];   /* +0x08..+0x1FFF */
} NetworkPeerPayload;   /* size: 0x2000 */

/* The 0x2014-byte peer buffer: the 0x2000-byte payload at +0x10 (whose head is the peer/socket
   pair) and the used-byte count at +0x2010.  It is a class with virtuals - the range's `init` slots
   dispatch the clear through +0x28, which is MWCC's own virtual-call shape - and no virtual is
   defined here, so MWCC emits no table for it (the band's tables belong to the peer classes). */
class NetworkPeerBuffer {
public:
    /* +0x08 */ virtual void slot_08();
    /* +0x0C */ virtual void slot_0C();
    /* +0x10 */ virtual void slot_10();
    /* +0x14 */ virtual void slot_14();
    /* +0x18 */ virtual void slot_18();
    /* +0x1C */ virtual void slot_1C();
    /* +0x20 */ virtual void slot_20();
    /* +0x24 (GUESS: name from the call the stream helpers make) */ virtual s32 put(const void* data, u32 size); /* untyped: byte range (the record's bytes) */
    /* +0x28 */ virtual void clearPayload();

    NetworkPeerErrorRecord error_04; /* +0x0004 - the record a failing operation fills */
    NetworkPeerPayload payload_10;   /* +0x0010 - the payload, head word pair first */
    u32 used_2010;                   /* +0x2010 - bytes in use */
};   /* size: 0x2014 */

/* ---------------- the peer's socket handle ---------------------------------------------------- */

/* The object the peer keeps at +0x04 and dispatches through: the +0x1C slot is a tail-jump
   (`bctr`), which is the shape MWCC gives a virtual whose result is returned directly, and +0x28 is
   the receive-buffer clear.  Only the two called slots are named; the gaps are the dispatch holes
   between them (GUESS: both names are offset-derived - the range calls them and nothing else). */
class NetworkSocketHandle {
public:
    /* +0x08 */ virtual void slot_08();
    /* +0x0C */ virtual void slot_0C();
    /* +0x10 */ virtual void slot_10();
    /* +0x14 */ virtual void slot_14();
    /* +0x18 */ virtual void slot_18();
    /* +0x1C (GUESS) */ virtual s32 closeSocket();
    /* +0x20 */ virtual void slot_20();
    /* +0x24 */ virtual void slot_24();
    /* +0x28 (GUESS) */ virtual s32 clearReceive();
};   /* size: 0x04 - only ever reached through a pointer in this unit */

/* ---------------- the peer that owns a socket and a byte stream ------------------------------- */

/* The peer the socket helpers work on: the socket handle at +0x04 (which answers how much is
   readable and takes the close/clear slots), the peer id at +0x0C and the two one-byte state flags
   at +0x10/+0x18. */
typedef struct NetworkPeerSocket {
    void* unused_00;                 /* +0x00 */
    NetworkSocketHandle* handle_04;  /* +0x04 - the socket object */
    u32 unused_08;                   /* +0x08 */
    u32 peerId_0C;                   /* +0x0C - the identifier the socket was registered with */
    u8  armed_10;                    /* +0x10 - set while the peer has something to drop */
    u8  pad_11[0x07];                /* +0x11..+0x17 */
    u8  dropped_18;                  /* +0x18 - raised when the drop lands */
} NetworkPeerSocket;   /* size: 0x1C (approximation - only the fields the range reads are evidenced) */

/* The peer that keeps its 0x2400-byte receive area inline. */
typedef struct NetworkPeerReceive {
    void* unused_00;        /* +0x00 */
    u8  pad_04[0x1C];       /* +0x04..+0x1F */
    u8  recv_20[0x2400];    /* +0x20..+0x241F */
    u32 recvUsed_2420;      /* +0x2420 - bytes received */
} NetworkPeerReceive;   /* size: 0x2424 */

/* ---------------- the peer that owns a byte stream -------------------------------------------- */

/* The stream cursor the append helpers drive: the block at +0x04, its size at +0x08 and the write
   cursor at +0x0C. */
typedef struct NetworkByteStream {
    void* unused_00;       /* +0x00 */
    u8* data_04;           /* +0x04 - the bytes the cursor writes into */
    u32 size_08;           /* +0x08 - capacity */
    u32 cursor_0C;         /* +0x0C - bytes written so far */
} NetworkByteStream;   /* size: 0x10 */

/* ---------------- the record the byte stream copies out --------------------------------------- */

/* What a caller hands several of these helpers: somewhere to put the bytes and how much room there
   is (a `u16`, because the length prefix it is compared against is one). */
typedef struct NetworkPeerRecord {
    /* untyped: caller-owned payload - the caller's own buffer */
    void* data_00;   /* +0x00 - the caller's buffer */
    u16   size_04;   /* +0x04 - its capacity, and the length actually copied */
} NetworkPeerRecord;   /* size: 0x08 (approximation: the two fields the range reads) */

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

/* ---------------- the peer name and its per-peer flags ---------------------------------------- */

/* The record a peer publishes under: its name at +0x04 (at most 0x1FF bytes), then the same
   four-word record table and one-byte error code the block below has, and the flag at +0x1560 that
   makes a rename fail. */
typedef struct NetworkPeerConfig {
    void* unused_00;        /* +0x00 */
    char  name_04[0x200];   /* +0x04..+0x203 */
    u8    pad_204[0x10];    /* +0x204..+0x213 */
    u32   count_214;        /* +0x214 */
    u8    code_218;         /* +0x218 */
    u8    pad_219[0x1347];  /* +0x219..+0x155F */
    u32   flag_1560;        /* +0x1560 - set while the peer must not be renamed */
} NetworkPeerConfig;   /* size: 0x1564 */

/* ---------------- the peer that owns a four-entry record table -------------------------------- */

/* The per-peer record table: four words at +0x204, how many of them are live at +0x214 and the
   peer's one-byte error code at +0x218. */
typedef struct NetworkPeerRecordTable {
    void* unused_00;       /* +0x00 */
    u8  pad_04[0x200];     /* +0x04..+0x203 */
    u32 records_204[4];    /* +0x204..+0x213 */
    u32 count_214;         /* +0x214 */
    u8  code_218;          /* +0x218 - idle until the first failure, then 0xFF */
} NetworkPeerRecordTable;   /* size: 0x21C */

/* ---------------- this unit's own definitions -------------------------------------------------- */

extern "C" {

/* the error record */
void networkPeerError_get(NetworkPeerError* self, NetworkPeerErrorRecord* out);
void networkPeerError_clear(NetworkPeerError* self);
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code);

/* the small transport object (its construct lives in the writer band, its dtor here) */
/* untyped: opaque handle passed through - the object's layout belongs to the writer band */
void* networkSmallObject_dtor(void* self, s32 flags);
/* untyped: opaque handle passed through - the object's layout belongs to the writer band */
void* networkSmallObject_destroy(void* self, s32 flags);
/* untyped: opaque handle passed through - the object's layout belongs to the writer band */
void* networkSmallObject_init(void* self);

/* the payload buffer */
s32 clearBuffer(NetworkPeerBuffer* self);
void clearBuffer2(NetworkPeerBuffer* self);
s32 networkPeerBuffer_init(NetworkPeerBuffer* self);
NetworkPeerBuffer* networkPeerBuffer_destroy(NetworkPeerBuffer* self, s32 flags);
void setPeerAndSocket(NetworkPeerBuffer* self, const u32* peerAndSocket);

/* the socket peer */
s32 getAvailableToRead(NetworkPeerSocket* self);
s32 networkPeer_getAvailableToRead(NetworkPeerSocket* self);
NetworkSocketHandle* networkPeer_getSocket(NetworkPeerSocket* self);
u32 networkPeer_getPeerId(NetworkPeerSocket* self);
s32 networkPeer_closeSocket(NetworkPeerSocket* self);
s32 networkPeer_clearReceiveSocket(NetworkPeerSocket* self);
void networkPeer_armDrop(NetworkPeerSocket* self);
void networkPeer_clearReceiveBuffer(NetworkPeerReceive* self);

/* the byte stream */
void networkPeerStream_putByte(NetworkByteStream* self, u8 value);
void networkPeerStream_forwardRecord(NetworkByteStream* self, NetworkPeerBuffer* sink);
void networkPeerStream_takeByte(NetworkByteStream* self, u8* out);
void networkPeerStream_takeRecord(NetworkByteStream* self, NetworkPeerRecord* record);
void networkPeerStream_readLength(NetworkByteStream* self, u16* out);

/* the peer name */
s32 networkPeer_setName(NetworkPeerConfig* self, const char* name);

/* the two socket-teardown tails and the two helpers they tail into.  The two `release*` bodies are
   NOT written yet (they load a logger singleton and a log string, both of which the `.data` blocker
   above covers), so this unit references two symbols it does not define - a link-time residual for
   whoever flips the unit, recorded in the unit header. */
void networkPeer_disconnect(NetworkPeerBuffer* self);
void networkPeer_disconnectSocket(NetworkPeerBuffer* self);
void networkPeer_release(NetworkPeerBuffer* self);
void networkPeer_releaseSocket(NetworkPeerBuffer* self);

/* the record table */
void networkPeer_resetCode(NetworkPeerRecordTable* self);
void networkPeer_recordGet(NetworkPeerRecordTable* self, s32 index, u32* out);

/* the peer classes' deleting destructors (the map's own `dtor_` names, kept: they chain the base
   destructor and nothing in the range pins their class down) */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void* dtor_803CCE9C(void* self, s32 flags);
NetworkPeerError* dtor_803CD708(NetworkPeerError* self, s32 flags);
NetworkPeerError* dtor_803CD764(NetworkPeerError* self, s32 flags);
NetworkPeerError* dtor_803CF108(NetworkPeerError* self, s32 flags);
NetworkPeerError* dtor_803CF694(NetworkPeerError* self, s32 flags);
NetworkPeerOwner* dtor_803CF8F4(NetworkPeerOwner* self, s32 flags);
NetworkPeerError* dtor_803D14C0(NetworkPeerError* self, s32 flags);

/* the session accessors this unit owns */
s32 NetworkSessionStable_getOwnIndex(NetworkSessionStable* self);

/* the four symbols below are owned by this unit but consumed by the bands around it: the neighbouring
   `Network/fn_803D3CE8.cpp` drives `NetworkSessionStable_sendStream` (its seven op-code writers end
   with it) and `NetworkSessionStable_setNotifyValue`, and the manager's mutex helpers wrap the OS
   ones with the peer's own +0x04 offset. */
void NetworkSessionStable_sendStream(NetworkSessionStable* self, NetworkStreamWriter* stream,
                                     u32 a, u32 b, const void* term, u32 c); /* untyped: byte range (the terminator) */
void NetworkSessionStable_setNotifyValue(u32 value);
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void LockMutex(void* mutex);
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void UnlockMutex(void* mutex);
/* untyped: opaque handle passed through - the peer the caller already holds */
u32 getSomething5(void* self);

/* The Pat manager's two pumps: their addresses (0x803CE064, 0x803CE5F0) are inside this unit's range,
   so rule 2 gives them to its owner's header.  `Network/fn_803D3CE8.h` - the Pat band's own header,
   which includes this one - and its neighbour `Network/NetworkSessionManagerPat.cpp` take them from
   here. */
void receivePatInterfaces(PatReceiver* receiver);
void flushPatRequests(PatRequestQueue* queue);

/* The two unsplit helpers the destructors above chain and the socket reader: symbols no registered
   unit owns, declared in the header the band's consumers already include.  They used to be forced
   here by a return-type clash between `include/unsplit/Network.h` and `Network/fn_803D3CE8.h` over
   the logger accessor `fn_803C9974` - now the map's `getNetworkLogger`, whose declaration the Pat
   landing left in the band header alone, so the clash is gone. */
/* untyped: opaque handle passed through - the caller hands an object of another band's layout */
void dtor_803C989C(void* self, s32 flags);
/* untyped: opaque handle passed through - the caller hands an object of another band's layout */
void dtor_803CA4E8(void* self, s32 flags);
/* untyped: opaque handle passed through - the socket object belongs to another band */
s32 getBytesAvailableToRead(void* handle);

}

/* `__dl__FPv`'s real spelling - every deleting destructor above calls it.  It is a C++ operator, so
   it is declared outside the `extern "C"` block. */
/* untyped: opaque handle passed through - the freed allocation */
void operator delete(void* ptr) throw();

#endif /* NETWORK_NETWORK_TRANSPORT_H */