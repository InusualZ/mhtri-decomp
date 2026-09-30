/*
 * Network stream and socket helpers with no registered owner (docs/plan.md 6.5 rule 2) - the packet
 * reader/writer band at 0x803F7540..0x803FAE00 and the small socket-handle accessors the transport
 * band (the Network transport units, `Network/network_transport.h`) calls.
 *
 * Every address is outside a registered range, so no owner header exists and the declarations live here,
 * with the band.  The stream object is the `NetworkStreamWriterDefault` local the session band reserves
 * (`Network/NetworkSessionManager.h`); only a forward declaration is needed here.  Names come from what each body
 * does and what its callers pass (GUESS marks are on the ones carrying no string or dump evidence).
 *
 * Added with the transport body pass.
 */
#ifndef MHTRI_UNSPLIT_NETWORKSTREAM_H
#define MHTRI_UNSPLIT_NETWORKSTREAM_H

#include "types.h"

struct NetworkStreamWriterDefault;
struct NetworkStreamWriter;
struct NetworkSmallObject;
class NetworkStreamQueue;
class NetworkSocketHandle;

extern "C" {

/* 0x8045DFA0 - the C library's `rand` (the 0x41C64E6D linear congruential step, top 15 bits). */
s32 rand(void);

/* 0x803F7540 - the last error code the socket handle recorded (its +0x08 word). */
s32 networkSocketHandle_getLastError(NetworkSocketHandle* handle);

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

/* ---- the packet (the 0x20-byte `NetworkStreamWriter` local) and the message queue it is read from ----
 * The session unit (`Network/NetworkSessionStable.cpp`) builds a packet on its stack, binds it to a block
 * and walks the framed messages in it; each queue stores such messages.  GUESS on every name: each is read
 * off the body and the call that reaches it. */

/* 0x803CB9B4 - builds an empty packet (the `NetworkBuffer` base, then its own table). */
void networkPacket_construct(NetworkStreamWriter* self);
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

}

#endif /* MHTRI_UNSPLIT_NETWORKSTREAM_H */
