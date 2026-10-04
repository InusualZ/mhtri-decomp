/*
 * include/Network/NetworkCommunityPat.h - the `NetworkCommunityPat` view the pat-control band drives.
 *
 * `NetworkCommunityPat` is the +0x08 element of the `NetworkPat` holder (include/Network/NetworkPat.h):
 * the class the constructor at 0x803F02C4 builds (allocation 0x25EC).  The band reaches it only through the friend
 * requests below - direct (non-virtual) calls into the class's own code at 0x803F10E4..0x803F1424 - so
 * only those methods are declared and the layout is left to its owner (a class without declared data is
 * never instantiated or read here).  GUESS on every method name: they are derived from what the
 * pat-control band passes (a network id, and for the invite a second word) and from the friend flow the
 * band drives.
 */
#ifndef MHTRI_NETWORK_NETWORKCOMMUNITYPAT_H
#define MHTRI_NETWORK_NETWORKCOMMUNITYPAT_H

#include "types.h"

/* The network id the friend requests take (defined in `Network/NetworkLayerPat.h`; only its address
 * crosses the calls below, so the name is enough and this header stays free of the session types). */
typedef struct NetId NetId;

/* The roster block `NetCtrlWk::roster_sync_0x61CC` holds (defined by the work record header). */
struct NetRosterSync;

class NetworkCommunityPat {
public:
    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void pad_14();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void openCommunity_1C();
    /* +0x20 */ virtual void shutdown_20();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual void pad_28();
    /* +0x2C */ virtual void pad_2C();
    /* +0x30 */ virtual void pad_30();
    /* +0x34 */ virtual void pad_34();
    /* +0x38 */ virtual void requestNews_38();

    /* 0x803F10E4 - sends the friend roster held at `roster` (`NetCtrlWk::roster_sync_0x61CC`). */
    void syncFriends(NetRosterSync* roster);
    /* 0x803F1204 - sends an invite to `id`; `kind` is the invite type. */
    void inviteFriend(const NetId* id, s32 kind);
    /* 0x803F129C - removes `id` from the friend list. */
    void removeFriend(const NetId* id);
    /* 0x803F139C - sends a friend request to `id`. */
    void sendFriendRequest(const NetId* id);
    /* 0x803F1424 - accepts the friend request from `id`. */
    void acceptFriendRequest(const NetId* id);
    /* 0x803F1324 - requests the block list (once). */
    void requestBlockList(void);
};

/* Declarations moved here from `include/unsplit/Network.h, NetworkStream.h` (docs/plan.md 6.5 rule 2: the owner declares). */
class NetworkLayerPat;             /* include/Network/NetworkLayerPat.h */
typedef struct NetworkRequest NetworkRequest;   /* include/Network/NetworkSessionManager.h */
class NetworkSocketHandle;
class NetworkFileFetcher;          /* include/Network/network_pat_control.h */
struct NetworkStreamWriterDefault;
struct NetworkStreamWriter;
struct NetworkSmallObject;
class NetworkStreamQueue;
typedef struct NetworkRequestError NetworkRequestError;   /* include/unsplit/Network.h */
typedef struct NetworkStateMachine NetworkStateMachine;   /* include/Network/network_state.h */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the bit-stream writer API (moved here from `Network/NetworkSessionManager.h`).  The two writer
   classes are reconstructions from the frame each constructor is given: `NetworkStreamWriter` is the
   0x20-byte local every op-code sender reserves, `NetworkStreamWriterDefault` the one
   `NetworkSessionStable::move` reserves. */
u32 writeByte(NetworkStreamWriter* self, u32 value);
u32 writeUInt(NetworkStreamWriter* self, u32 value);
u16 writeSize(NetworkStreamWriter* self, u16 size);
/* untyped: byte range - the bytes appended to the stream */
s32 writeBytes(NetworkStreamWriter* self, const void* data, u32 len);

/* the writer's remaining entry points the session tail drives (the second writer class) */
/* untyped: byte range - the bytes appended to the stream */
void networkStreamWriter_putBytes(NetworkStreamWriterDefault* self, const void* data, u32 size);
void networkStreamWriter_flush(NetworkStreamWriterDefault* self);
void networkStreamWriter_setMode(NetworkStreamWriterDefault* self, u32 mode);
void networkStreamWriter_putU16(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU16b(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU32(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_putU32b(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable1(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable2(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable3(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_commit(NetworkStreamWriterDefault* self);
void networkStreamWriter_bytes(NetworkStreamWriterDefault* self);
u32 networkStreamWriter_size(const void* sub);

/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkSmallObject_construct(void* self);

/* ---- the session band's request writers (moved here from `Network/network_state.h`; the state machine
   passes its own view of the session object) */
u32  flushBuffer(NetworkStateMachine* self, u32 opcode, u32 flags);
void encryptBuffer(NetworkStateMachine* self);
u32  writeUInt8(NetworkStateMachine* self, u8 value);
void writeUInt16(NetworkStateMachine* self, u16 value);
void writeUInt32(NetworkStateMachine* self, u32 value);
void writeUInt32Shared(NetworkStateMachine* self, u32 value);
void writeUInt8Array(NetworkStateMachine* self, const u8* data, u16 count);
void writeBool(NetworkStateMachine* self, s8 value);

/* The socket reader `network_socket_streams.cpp` polls (moved here from `Network/network_transport_types.h`). */
/* untyped: opaque handle passed through - the socket object belongs to another band */
s32 getBytesAvailableToRead(void* handle);

/* GUESS: 0x803EBAF0 hands one layer event (kind 3 = error, kind 4 = done) to the callback object the
 * layer holds, after posting `info` to the network singleton. */
void notifyLayerEvent(NetworkLayerPat* self, u32 kind, s32 code, u32 has_info, NetworkRequestError* info,
                      u32 context);

/* GUESS on both names: 0x803EF3C0 and 0x803EF4C8 are the siblings of `setCollectionLog` for the two
 * fixed codes the state machine's flag tests report (0x80060033 and 0x80060012); the names follow the
 * flag bits that select them (bit 0 = the session dropped, bit 1 = the request was cancelled). */
void setCollectionLogSessionLost(NetworkLayerPat* self, NetworkRequest* request);

void setCollectionLogAborted(NetworkLayerPat* self, NetworkRequest* request);

/* 0x803EF568 - records `code` (+ two arguments) as the request's error and reports it to the server. */
void setCollectionLog(NetworkLayerPat* self, NetworkRequest* request, u32 code, u32 arg_a, u32 arg_b);

/* GUESS: 0x803EB9D4 walks the layer's 100 child slots and releases each finished one. */
void pollLayerSlots(NetworkLayerPat* self);

/* GUESS: 0x803EBA5C reports the layer's slot counts as one kind-0x14 event. */
void notifyLayerSlotSummary(NetworkLayerPat* self);

/* 0x803F7540 - the last error code the socket handle recorded (its +0x08 word). */
s32 networkSocketHandle_getLastError(NetworkSocketHandle* handle);

/* 0x803F7548 - construct the 0x24-byte Wii socket object `sNetworkLibraryWii::createSocket` allocates:
 * the base constructor, its own table (0x805FC984), descriptor -1 at +0x0C and the rest cleared.  GUESS
 * on the name (the retail symbol is the class's constructor; the class is not reconstructed yet). */
NetworkSocketHandle* constructNetworkSocket(NetworkSocketHandle* socket);

/* 0x803F73C0 - construct the 0x10-byte fetcher `sNetworkLibraryWii::createFetcher` builds for kind 2:
 * the base constructor, then its own table (0x805FC8A8).  GUESS on the name, from that kind. */
NetworkFileFetcher* constructNetworkFetcherKind2(NetworkFileFetcher* fetcher);

/* 0x803F9E04 - binds a stream to the byte block it reads packets from (GUESS: the parent's init, then
 * the block pointer stored at +0x10). */
void networkStreamReader_attach(NetworkStreamWriterDefault* self, u8* block, u32 size);

/* 0x803F9E40 - true when a whole packet sits at the front of the block. */
s32 make_sure_enough_space(NetworkStreamWriterDefault* self);

/* 0x803F9EBC - copies the leading packet out; 0 when it is incomplete, -1 when `capacity` is too small,
 * else the packet's length. */
s32 copy_from_buffer(NetworkStreamWriterDefault* self, u8* out, u32 capacity);

/* 0x803F9F74 - drops the leading packet from the block and returns its length (0 when incomplete). */
s32 networkStreamReader_consumePacket(NetworkStreamWriterDefault* self);

/* 0x803FA0E8 - the length field of the leading packet (its payload size plus the 22-byte header). */
u16 read_size_from_buffer(NetworkStreamWriterDefault* self);

/* 0x803F89D0 - binds the packet to `size` bytes at `buffer`. */
/* untyped: byte range (the block the packet reads or writes) */
void networkPacket_attach(NetworkStreamWriter* self, const void* buffer, u32 size);

/* 0x803F8A14 - starts a message in `mode`. */
void networkPacket_begin(NetworkStreamWriter* self, s32 mode);

/* 0x803F8BDC - appends the 14-byte address record; returns the bytes written. */
u32 networkPacket_writeRecord(NetworkStreamWriter* self, const NetworkSmallObject* value);

/* 0x803F8904 - true when both address records are set and equal. */
s32 networkSmallObject_isEqual(const NetworkSmallObject* a, const NetworkSmallObject* b);

/* 0x803F8E88 - stamps the message with `seconds`. */
void networkPacket_setTimestamp(NetworkStreamWriter* self, f32 seconds);

/* 0x803F8FAC - true when a whole message sits at the packet's front. */
s32 networkPacket_hasMessage(NetworkStreamWriter* self);

/* 0x803F9038 - steps to the next message. */
void networkPacket_nextMessage(NetworkStreamWriter* self);

/* 0x803F9388 - takes a 14-byte address record into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeRecord(NetworkStreamWriter* self, NetworkSmallObject* out);

/* 0x803F9418 - takes `length` bytes into `out`; returns the length or 0. */
u16 networkPacket_takeBytes(NetworkStreamWriter* self, u8* out, u32 length);

/* 0x803F948C - takes a word into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeU32(NetworkStreamWriter* self, u32* out);

/* 0x803F95CC - takes a byte into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeByte(NetworkStreamWriter* self, u8* out);

/* 0x803F9608 - the size of the front message under the field mask `mask`. */
u32 networkPacket_getMessageSize(NetworkStreamWriter* self, u32 mask);

/* 0x803F96C8 - the front message's payload length. */
u16 networkPacket_getPayloadSize(NetworkStreamWriter* self);

/* 0x803F9798 - true when the front message carries user data. */
s32 networkPacket_isUserData(NetworkStreamWriter* self);

/* 0x803F97D0 - the front message's timestamp in seconds. */
f32 networkPacket_getTimestamp(NetworkStreamWriter* self);

/* 0x803F986C - the bytes the packet holds. */
u32 networkPacket_getLength(NetworkStreamWriter* self);

/* 0x803F9878 / 0x803FA15C - the fixed overhead of a message (0x17) and of a frame (0x16). */
u32 networkPacket_getMessageOverhead(void);

u32 networkPacket_getFrameOverhead(void);

/* 0x803FA164 / 0x803FA1B0 - the frame's two sequence numbers; 0x803FA1FC / 0x803FA248 - its two nonces. */
u16 networkPacket_getSequenceA(NetworkStreamWriter* self);

u16 networkPacket_getSequenceB(NetworkStreamWriter* self);

u32 networkPacket_getSourceNonce(NetworkStreamWriter* self);

u32 networkPacket_getSessionNonce(NetworkStreamWriter* self);

/* 0x803FA2C4 - the frame's channel (0 or 1); 0x803FA310 - true for a handshake frame. */
u8 networkPacket_getChannel(NetworkStreamWriter* self);

s32 networkPacket_isHandshake(NetworkStreamWriter* self);

/* 0x803F9970 - appends the front message under `mask` to the writer; returns the bytes or -1. */
s32 networkStreamWriter_putPacket(NetworkStreamWriterDefault* self, NetworkStreamWriter* packet, u32 mask);

/* 0x803FA698 - appends the packet to the queue under `mask`; returns the bytes or -1. */
s32 networkStreamQueue_append(NetworkStreamQueue* self, NetworkStreamWriter* packet, u32 mask);

/* 0x803FA77C - binds `packet` to the bytes the queue holds. */
void networkStreamQueue_fillPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet);

/* 0x803FA79C - drops whole messages totalling up to `bytes`, then closes the gap. */
void networkStreamQueue_discard(NetworkStreamQueue* self, s32 bytes, u32 mask);

/* 0x803FA87C - empties the queue; 0x803FA888 - sets its sequence number. */
void networkStreamQueue_clear(NetworkStreamQueue* self);

void networkStreamQueue_setSequence(NetworkStreamQueue* self, u16 sequence);

/* 0x803FA898 / 0x803FA9D0 - files `packet` by its first / next sequence number; 0..3 is the outcome,
 * else an error source. */
u32 putTopPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet);

u32 networkStreamQueue_putNextPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet);

/* 0x803FABC0 - drops what the packet's second sequence number acknowledges. */
void networkStreamQueue_acknowledge(NetworkStreamQueue* self, NetworkStreamWriter* packet);

/* 0x803FAC80 - moves the read cursor to the front; 0x803FAC8C - true when a whole message is under it;
 * 0x803FACF4 - steps over it; 0x803FAD88 - binds `packet` to the bytes from the cursor; 0x803FADB8 - removes it. */
void networkStreamQueue_rewind(NetworkStreamQueue* self);

s32 networkStreamQueue_hasMessage(NetworkStreamQueue* self);

void networkStreamQueue_skipMessage(NetworkStreamQueue* self);

void networkStreamQueue_peekPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet);

void networkStreamQueue_removeMessage(NetworkStreamQueue* self);

#ifdef __cplusplus
}
#endif

#endif
