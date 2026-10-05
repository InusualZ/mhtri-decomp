/*
 * include/Network/NetworkUnitPacket.h - the free functions of `src/Network/NetworkUnitPacket.cpp` (`.text` 0x803F89CC..0x803FAE9C): the packet, bit-stream writer, stream reader and stream queue entry points.
 * Moved here from `Network/NetworkCommunityPat.h` when the network pilot round 3 recut gave the range its own unit
 * (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORKUNITPACKET_H
#define MHTRI_NETWORK_NETWORKUNITPACKET_H

#include "types.h"

class NetworkStreamWriterDefault;   /* include/Network/network_writer_types.h */
class NetworkStreamWriter;          /* include/Network/network_writer_types.h */
class NetworkUniqueId;             /* include/Network/NetworkUniqueId.h */
class NetworkStreamQueue;

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the bit-stream writer API (moved here from `Network/NetworkSessionManager.h`).  The two writer
   classes are reconstructions from the frame each constructor is given: `NetworkStreamWriter` is the
   0x20-byte local every op-code sender reserves, `NetworkStreamWriterDefault` the one
   `NetworkSessionStable::move` reserves. */
u32 writeByte(NetworkStreamWriter* self, u8 value);
u32 writeUInt(NetworkStreamWriter* self, u32 value);
u32 writeUShort(NetworkStreamWriter* self, u16 value);
void writeSize(NetworkStreamWriter* self, u16 size);
/* untyped: byte range - the bytes appended to the stream */
s32 writeBytes(NetworkStreamWriter* self, const void* data, u32 len);

/* the writer's remaining entry points the session tail drives (the second writer class); 0x803F9880 is its
   flush override (empty) */
void networkStreamWriter_flushFrame(NetworkStreamWriterDefault* self);
/* untyped: byte range - the bytes appended to the stream */
void networkStreamWriter_putBytes(NetworkStreamWriterDefault* self, const void* data, u32 size);
s32 networkStreamWriter_flush(NetworkStreamWriterDefault* self);
void networkStreamWriter_setMode(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU16(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU16b(NetworkStreamWriterDefault* self, u16 value);
void networkStreamWriter_putU32(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_putU32b(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable1(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable2(NetworkStreamWriterDefault* self, u32 value);
void networkStreamWriter_enable3(NetworkStreamWriterDefault* self, u32 value);
/* 0x803F9CAC - sets or clears the frame flag 0x04 (GUESS: offset-derived name). */
void networkStreamWriter_setFlag04(NetworkStreamWriterDefault* self, s32 on);
/* 0x803F9A04 / 0x803F9A10 - the bytes the framed writer holds, and a reservation of `size` more. */
u16 getPosition(NetworkStreamWriterDefault* self);
s32 networkStreamWriter_skip(NetworkStreamWriterDefault* self, u16 size);
/* 0x803F9D88 / 0x803F9DEC / 0x803F9DF8 - the frame's CRC (+0x12) and key bytes (+0x14, +0x15). */
void networkStreamWriter_setCrc(NetworkStreamWriterDefault* self, u16 crc);
void networkStreamWriter_setKeyA(NetworkStreamWriterDefault* self, u8 key);
void networkStreamWriter_setKeyB(NetworkStreamWriterDefault* self, u8 key);
/* 0x803F9D0C - seals the frame with the sink's checksum; 0x803FA390 - scrambles the payload and hides the key. */
void networkStreamWriter_commit(NetworkStreamWriterDefault* self);
void networkStreamWriter_bytes(NetworkStreamWriterDefault* self);
/* 0x803FA438 - unscrambles the payload; 0x803FA4B0 - checks the CRC and the version (retail `NetworkUnitPacket::test`),
 * 1 when the frame is good. */
void networkStreamReader_decryptFrame(NetworkStreamWriterDefault* self);
s32 networkStreamReader_test(NetworkStreamWriterDefault* self);
/* 0x803FAE7C / 0x803FAE8C - the framed writer's scramble overrides (table 0x805FCDD4 +0x30/+0x34). */
void networkStreamWriter_encrypt(NetworkStreamWriterDefault* self, u8 key, u16 offset, u16 size);
void networkStreamWriter_decrypt(NetworkStreamWriterDefault* self, u8 key, u16 offset, u16 size);
u32 networkStreamWriter_size(const void* sub);

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

/* 0x803FA018 / 0x803FA098 / 0x803FA0A0 / 0x803FA110 - the current frame's payload (NULL when incomplete), the
 * frame, its version and its payload length; 0x803FA32C / 0x803FA378 / 0x803FA384 - its CRC and key bytes. */
u8* networkStreamReader_getPayload(NetworkStreamWriterDefault* self);
u8* networkStreamReader_getFrame(NetworkStreamWriterDefault* self);
u16 networkStreamReader_getVersion(NetworkStreamWriterDefault* self);
u16 networkStreamReader_getPayloadSize(NetworkStreamWriterDefault* self);
u16 networkStreamReader_getCrc(NetworkStreamWriterDefault* self);
u8 networkStreamReader_getKeyA(NetworkStreamWriterDefault* self);
u8 networkStreamReader_getKeyB(NetworkStreamWriterDefault* self);

/* 0x803FA0E8 - the length field of the leading packet (its payload size plus the 22-byte header). */
u16 read_size_from_buffer(NetworkStreamWriterDefault* self);

/* 0x803F89CC - the packet's flush override (empty). */
void networkPacket_flush(NetworkStreamWriter* self);

/* 0x803F89D0 - binds the packet to `size` bytes at `buffer`. */
/* untyped: byte range (the block the packet reads or writes) */
void networkPacket_attach(NetworkStreamWriter* self, const void* buffer, u32 size);

/* 0x803F8A14 - starts a message in `mode`. */
void networkPacket_begin(NetworkStreamWriter* self, s32 mode);

/* 0x803F8A28 - starts a message: the flags, the timestamp slot and the optional value block; the header size,
 * or -1 when the packet has no room. */
s32 networkPacket_beginMessage(NetworkStreamWriter* self, s32 userData, s32 timestamp, u32 base, u8 count,
                               const u32* values);

/* 0x803F8BDC - appends the 14-byte address record; returns the bytes written. */
u32 networkPacket_writeRecord(NetworkStreamWriter* self, const NetworkUniqueId* value);

/* 0x803F8E88 - stamps the message with `seconds`. */
void networkPacket_setTimestamp(NetworkStreamWriter* self, f32 seconds);

/* 0x803F8F20 / 0x803F8F6C - binds the packet to received bytes (the packet's `bind`), and moves the cursor
 * to the current message's payload. */
void networkPacket_bind(NetworkStreamWriter* self, u8* block, u32 size);
void networkPacket_rewind(NetworkStreamWriter* self);

/* 0x803F8FAC - true when a whole message sits at the packet's front. */
s32 networkPacket_hasMessage(NetworkStreamWriter* self);

/* 0x803F9038 - steps to the next message. */
void networkPacket_nextMessage(NetworkStreamWriter* self);

/* 0x803F90D8 - copies the current message into `out` under `mask`; the bytes, 0, or -1 when `capacity` is
 * too small.  0x803F92EC - the current message's payload (NULL when incomplete). */
s32 networkPacket_copyMessage(NetworkStreamWriter* self, u8* out, u32 capacity, u8 mask);
u8* networkPacket_getPayload(NetworkStreamWriter* self);

/* 0x803F9388 - takes a 14-byte address record into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeRecord(NetworkStreamWriter* self, NetworkUniqueId* out);

/* 0x803F9418 - takes `length` bytes into `out`; returns the length or 0. */
u16 networkPacket_takeBytes(NetworkStreamWriter* self, u8* out, u32 length);

/* 0x803F948C - takes a word into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeU32(NetworkStreamWriter* self, u32* out);

/* 0x803F952C - takes a half-word into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeU16(NetworkStreamWriter* self, u16* out);

/* 0x803F95CC - takes a byte into `out`; returns the bytes taken or 0. */
s32 networkPacket_takeByte(NetworkStreamWriter* self, u8* out);

/* 0x803F9608 - the size of the front message under the field mask `mask`. */
u32 networkPacket_getMessageSize(NetworkStreamWriter* self, u32 mask);

/* 0x803F96C8 - the front message's payload length. */
u16 networkPacket_getPayloadSize(NetworkStreamWriter* self);

/* 0x803F9710 - the front message's header size (3, the timestamp, the value block). */
u16 networkPacket_getHeaderSize(NetworkStreamWriter* self);

/* 0x803F9798 - true when the front message carries user data. */
s32 networkPacket_isUserData(NetworkStreamWriter* self);

/* 0x803F97B4 - true when the front message carries a timestamp; 0x803F985C - its value count. */
s32 networkPacket_hasTimestamp(NetworkStreamWriter* self);
u8 networkPacket_getValueCount(NetworkStreamWriter* self);

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
/* 0x803FA294 / 0x803FA2F4 - the frame flags `networkStreamWriter_enable1` and `_setFlag04` set (GUESS names). */
s32 networkPacket_getFlag10(NetworkStreamWriter* self);
s32 networkPacket_getFlag04(NetworkStreamWriter* self);

s32 networkPacket_isHandshake(NetworkStreamWriter* self);

/* 0x803F9970 - appends the front message under `mask` to the writer; returns the bytes or -1. */
s32 networkStreamWriter_putPacket(NetworkStreamWriterDefault* self, NetworkStreamWriter* packet, u32 mask);

/* 0x803FA698 - appends the packet to the queue under `mask`; returns the bytes or -1. */
s32 networkStreamQueue_append(NetworkStreamQueue* self, NetworkStreamWriter* packet, u32 mask);

/* 0x803FA6FC - appends every whole message left in the packet; the bytes, or the first negative result. */
s32 networkStreamQueue_appendAll(NetworkStreamQueue* self, NetworkStreamWriter* packet);

/* 0x803FAAFC - appends the frame's whole payload; 1, or an error source. */
u32 putAllPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet);

/* 0x803FAC38 - lifts whichever of two 16-bit sequence numbers wrapped so they compare the short way round. */
void networkStreamQueue_unwrapSequence(NetworkStreamQueue* self, u16 own, u16 other, s32* ownOut, s32* otherOut);

/* 0x803FAE70 / 0x803FAE74 / 0x803FAE78 - the writers' flush-hook and fill overrides (forward to the sink's). */
void networkPacket_onFlush(NetworkStreamWriter* self, u8* data, u32 size);
s32 networkPacket_fill(NetworkStreamWriter* self, u8* out, u32 size);
void networkStreamWriter_onFlush(NetworkStreamWriterDefault* self, u8* data, u32 size);

/* 0x803FA77C - binds `packet` to the bytes the queue holds. */
void networkStreamQueue_fillPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet);

/* 0x803FA79C - drops whole messages totalling up to `bytes`, then closes the gap. */
void networkStreamQueue_discard(NetworkStreamQueue* self, s32 bytes, u8 mask);

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

#endif /* MHTRI_NETWORK_NETWORKUNITPACKET_H */
