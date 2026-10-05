/*
 * Network/NetworkUnitPacket.cpp - the unit packet band: the message packet (`networkPacket_*`, the 0x18-byte
 *   `NetworkStreamWriter`), the 22-byte framed writer/reader (`networkStreamWriter_*` / `networkStreamReader_*`, the
 *   0x1C-byte `NetworkStreamWriterDefault`) and the queue of framed messages (`networkStreamQueue_*`).
 *
 * SECTIONS. extab 0x8001B96C..0x8001BBCC; extabindex 0x8003BB20..0x8003BDF0; .text 0x803F89CC..0x803FAE9C;
 *   .data 0x805FCC98..0x805FCE50; .sdata 0x80793958..0x80793960; .sdata2 0x8079C7C0..0x8079C7D0.
 *
 * WHAT IT IS. The log strings name the classes `NetworkUnitPacket` ("::test: CRC error", "Invalid version") and
 *   `NetworkUnitPacketPool` (`putTopPacket`/`putLowPacket`/`putAllPacket`); the project still spells them by the
 *   older names `NetworkStreamWriter`/`NetworkStreamWriterDefault`/`NetworkStreamQueue` that every caller uses.  A
 *   message is a big-endian u16 payload size, a flag byte (0x80 user data, 0x40 a timestamp follows, 0x3F the
 *   value count), the optional timestamp (milliseconds), the optional value block and the payload; a frame is a
 *   22-byte header (version, payload length, the two sequence numbers, the two nonces, the flag byte at +0x11, the
 *   CRC at +0x12 and the two key bytes at +0x14/+0x15) and its payload.  Every value goes through the network
 *   library's byte-order slots (`getNetworkLogger()`'s +0x48..+0x54).
 *
 * CLASS MODEL (residual). The queue is the class `NetworkStreamQueue : NetworkStreamSink`
 *   (`Network/network_writer_types.h`): its constructor and destructor (0x803FA5F4/0x803FA63C) are defined here, so
 *   this unit emits its table 0x805FCD98 (`__vt__18NetworkStreamQueue`).  The functions the two writer tables
 *   0x805FCDD4 / 0x805FCE10 point at (the writers' `attach`/`bind`/`fill`/`flush` overrides) are still written as
 *   the free functions their callers in other units already name (`networkPacket_attach`,
 *   `networkStreamReader_attach`, ...); those two tables stay unemitted until the overrides become members (their
 *   destructors live in `lobby/lb_server_sel_trans.cpp`, which would also need its weak copies kept out of ours).
 *
 * NAMES. The sink's +0x30/+0x34/+0x38 slots are typed from the calls here (`encrypt`/`decrypt`/`checksum`, GUESS
 *   names): the forwarding overrides re-narrow the offset and size to u16, so the slots take u16.
 *   `networkStreamReader_decryptFrame`, `networkStreamReader_test` and the two overrides
 *   `networkStreamWriter_encrypt`/`_decrypt` are GUESS names; `NetworkStreamQueue` is the project's name for the
 *   log strings' `NetworkUnitPacketPool`.
 *
 * RESIDUALS (measured). 91 of 98 rows at 100 %.  `networkPacket_getMessageSize` 98.75: retail returns a u16 (its own
 *   callers re-mask it), but `Network/NetworkSessionStable.cpp`'s `send` loses 2.3 points with a u16 declaration, so
 *   the header keeps u32 and the callers here cast; `networkPacket_copyMessage` 98.05 (the same u16 return, and
 *   operand order in two address sums); `beginMessage` 99.36, `getHeaderSize` 99.71, `takeByte` 99.20,
 *   `putTopPacket` 99.74, `acknowledge` 99.00: operand order / one register.  `.data`: the two writer tables stay
 *   unemitted (above; `dataorder` also reads a zigzag seam at 0x805FCE10 because those tables' referrers sit in
 *   `lobby/lb_server_sel_trans.cpp`).  `extab` 608 B, 2 bytes differ: `networkStreamQueue_discard`'s cleanup range
 *   ends at the `hasMessage` call in retail (0x11 words) and at `memmove` in ours (0x1B) - retail's `memmove` is
 *   treated as non-throwing; a `throw()` on the declaration does not move it.  `.sdata` 2 of 8 B (the frame version;
 *   the rest is the alignment tail before the next unit).
 *
 * FLAGS. `cflags_network` with `-O3` like the transport siblings; file-scope `#pragma peephole off` and
 *   `#pragma dont_inline on` (retail calls every helper).
 */

#include "Network/NetworkUnitPacket.h"
#include "Network/network_writer_types.h"        /* NetworkStreamWriter, NetworkStreamWriterDefault, NetworkStreamQueue */
#include "Network/NetworkUniqueId.h"
#include "unsplit/Network.h"                     /* getNetworkLogger - no registered owner */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"                         /* memmove - owner MSL_C/alloc.cpp */

#pragma peephole off
#pragma dont_inline on

/* The frame version every frame header carries and `NetworkUnitPacket::test` checks (1.1). */
u16 networkFrameVersion = 0x0101;

extern "C" {

/* The packet's flush override: nothing to hand on. */
void networkPacket_flush(NetworkStreamWriter* self)
{
}

/* Binds the packet to `size` empty bytes at `buffer`; the first message starts there. */
/* untyped: byte range (the block the packet reads or writes) */
void networkPacket_attach(NetworkStreamWriter* self, const void* buffer, u32 size)
{
    self->NetworkStreamSink::attach((u8*)buffer, size);
    self->message_10 = (u8*)buffer;
    self->cursor_14 = NULL;
}

/* Starts a plain message (a timestamp, no values) with user-data flag `mode`. */
void networkPacket_begin(NetworkStreamWriter* self, s32 mode)
{
    networkPacket_beginMessage(self, mode, 1, 0, 0, NULL);
}

/* Starts a message at the end of the packet: the flags, the timestamp slot and the optional value block (`base`
   and `count` values); returns the header size, or -1 when the packet has no room for it. */
s32 networkPacket_beginMessage(NetworkStreamWriter* self, s32 userData, s32 timestamp, u32 base, u8 count,
                               const u32* values)
{
    u32 word;
    s32 offset;
    s32 i;
    s32 flags;
    s32 stamp;

    if (self->capacity_08 - self->used_0C < 3) {
        return -1;
    }
    writeSize(self, 0);
    stamp = timestamp ? 0x40 : 0;
    self->message_10[2] = (flags = count & 0x3F) | (userData ? 0x80 : 0) | stamp;
    if (self->capacity_08 - self->used_0C < networkPacket_getHeaderSize(self)) {
        return -1;
    }
    networkPacket_setTimestamp(self, 0.0f);
    if (count != 0) {
        word = getNetworkLogger()->encode_54(base);
        memcpy(self->message_10 + (timestamp ? 4 : 0) + 3, &word, 4);
        for (offset = 0, i = 0; i < flags; i++, offset += 4) {
            word = getNetworkLogger()->encode_54(*values);
            memcpy((timestamp ? 4 : 0) + offset + self->message_10 + 7, &word, 4);
            values++;
        }
    }
    self->used_0C += networkPacket_getHeaderSize(self);
    return networkPacket_getHeaderSize(self);
}

/* Appends the 14-byte address record of `value`; the bytes written, or -1. */
u32 networkPacket_writeRecord(NetworkStreamWriter* self, const NetworkUniqueId* value)
{
    s32 n;

    if (self->capacity_08 - self->used_0C < 14) {
        return -1;
    }
    n = ((NetworkUniqueId*)value)->fill(self->data_04 + self->used_0C, 14);
    if (n < 0) {
        return -1;
    }
    self->used_0C += n;
    return n;
}

/* Appends `len` raw bytes; the bytes written, or -1. */
/* untyped: byte range - the bytes appended to the stream */
s32 writeBytes(NetworkStreamWriter* self, const void* data, u32 len)
{
    if (self->capacity_08 - self->used_0C < len) {
        return -1;
    }
    memcpy(self->data_04 + self->used_0C, data, len);
    self->used_0C += len;
    return len;
}

/* Appends a big-endian word; 4, or -1. */
u32 writeUInt(NetworkStreamWriter* self, u32 value)
{
    u32 word;

    if (self->capacity_08 - self->used_0C < 4) {
        return -1;
    }
    word = getNetworkLogger()->encode_54(value);
    memcpy(self->data_04 + self->used_0C, &word, 4);
    self->used_0C += 4;
    return 4;
}

/* Appends a big-endian half-word; 2, or -1. */
u32 writeUShort(NetworkStreamWriter* self, u16 value)
{
    u16 half;

    if (self->capacity_08 - self->used_0C < 2) {
        return -1;
    }
    half = getNetworkLogger()->encode_4C(value);
    memcpy(self->data_04 + self->used_0C, &half, 2);
    self->used_0C += 2;
    return 2;
}

/* Appends a byte; 1, or -1. */
u32 writeByte(NetworkStreamWriter* self, u8 value)
{
    if (self->capacity_08 - self->used_0C < 1) {
        return -1;
    }
    self->data_04[self->used_0C] = value;
    self->used_0C++;
    return 1;
}

/* Stores the current message's payload size. */
void writeSize(NetworkStreamWriter* self, u16 size)
{
    u16 half;

    half = getNetworkLogger()->encode_4C(size);
    memcpy(self->message_10, &half, 2);
}

/* Stamps the current message with `seconds` (in milliseconds) when it carries a timestamp. */
void networkPacket_setTimestamp(NetworkStreamWriter* self, f32 seconds)
{
    u32 word;
    s32 ms;

    if (networkPacket_hasTimestamp(self) != 0) {
        ms = 1000.0f * seconds;
        word = getNetworkLogger()->encode_54(ms);
        memcpy(self->message_10 + 3, &word, 4);
    }
}

/* Binds the packet to `size` received bytes at `block` and moves the cursor to the first message's payload. */
void networkPacket_bind(NetworkStreamWriter* self, u8* block, u32 size)
{
    self->NetworkStreamSink::bind(block, size);
    self->message_10 = block;
    self->cursor_14 = NULL;
    networkPacket_rewind(self);
}

/* Moves the cursor to the current message's payload when a whole message is there. */
void networkPacket_rewind(NetworkStreamWriter* self)
{
    if (networkPacket_hasMessage(self) != 0) {
        self->cursor_14 = networkPacket_getPayload(self);
    }
}

/* True when a whole message sits at the current position. */
s32 networkPacket_hasMessage(NetworkStreamWriter* self)
{
    if (self->used_0C - (self->message_10 - self->data_04) < networkPacket_getHeaderSize(self)) {
        return 0;
    }
    return self->used_0C - (self->message_10 - self->data_04) >= (u16)networkPacket_getMessageSize(self, 0xFF);
}

/* Steps to the next message when a whole one is at the current position, and moves the cursor to its payload. */
void networkPacket_nextMessage(NetworkStreamWriter* self)
{
    if (self->used_0C - (self->message_10 - self->data_04) >= networkPacket_getHeaderSize(self) &&
        self->used_0C - (self->message_10 - self->data_04) >= (u16)networkPacket_getMessageSize(self, 0xFF)) {
        self->message_10 += (u16)networkPacket_getMessageSize(self, 0xFF);
        self->cursor_14 = networkPacket_getPayload(self);
    }
}

/* Copies the current message into `out` keeping only the parts `mask` selects (0x80 the user-data flag, 0x40 the
   timestamp, 0x3F the value block); the bytes copied, 0 when no whole message is there, -1 when `capacity` is
   too small. */
s32 networkPacket_copyMessage(NetworkStreamWriter* self, u8* out, u32 capacity, u8 mask)
{
    u16 size;
    u16 n;
    s32 count;
    u8* payload;

    if (self->used_0C - (self->message_10 - self->data_04) < 3) {
        return 0;
    }
    size = (u16)networkPacket_getMessageSize(self, mask);
    if (self->used_0C - (self->message_10 - self->data_04) < size) {
        return 0;
    }
    if (capacity < size) {
        return -1;
    }
    memcpy(out, self->message_10, 3);
    n = 3;
    if (!(mask & 0x80)) {
        out[2] &= 0x7F;
    }
    if (!(mask & 0x40)) {
        out[2] &= 0xBF;
    } else if (networkPacket_hasTimestamp(self) != 0) {
        memcpy(out + 3, self->message_10 + 3, 4);
        n = 7;
    }
    if (!(mask & 0x3F)) {
        out[2] &= 0xC0;
    } else {
        count = networkPacket_getValueCount(self);
        if (count > 0) {
            memcpy(out + n, self->message_10 + (networkPacket_hasTimestamp(self) ? 4 : 0) + 3, 4);
            n += 4;
            memcpy(out + n, self->message_10 + (networkPacket_hasTimestamp(self) ? 4 : 0) + 7, count * 4);
            n += (u16)(count * 4);
        }
    }
    payload = networkPacket_getPayload(self);
    if (payload == NULL) {
        return 0;
    }
    memcpy(out + n, payload, networkPacket_getPayloadSize(self));
    return (u16)(n + networkPacket_getPayloadSize(self));
}

/* The current message's payload, or NULL when no whole message is there. */
u8* networkPacket_getPayload(NetworkStreamWriter* self)
{
    if (self->used_0C - (self->message_10 - self->data_04) < networkPacket_getHeaderSize(self)) {
        return NULL;
    }
    if (self->used_0C - (self->message_10 - self->data_04) < (u16)networkPacket_getMessageSize(self, 0xFF)) {
        return NULL;
    }
    return self->message_10 + networkPacket_getHeaderSize(self);
}

/* Takes a 14-byte address record into `out`; 14, or 0 when the payload holds fewer bytes. */
s32 networkPacket_takeRecord(NetworkStreamWriter* self, NetworkUniqueId* out)
{
    u8 record[14];

    if (self->used_0C - (self->cursor_14 - self->data_04) < 14) {
        return 0;
    }
    memcpy(record, self->cursor_14, 14);
    out->put(record, 14);
    self->cursor_14 += 14;
    return 14;
}

/* Takes `length` raw bytes into `out`; the length, or 0. */
u16 networkPacket_takeBytes(NetworkStreamWriter* self, u8* out, u32 length)
{
    if (self->used_0C - (self->cursor_14 - self->data_04) < length) {
        return 0;
    }
    memcpy(out, self->cursor_14, length);
    self->cursor_14 += length;
    return length;
}

/* Takes a big-endian word into `out`; 4, or 0. */
s32 networkPacket_takeU32(NetworkStreamWriter* self, u32* out)
{
    u32 raw;
    u32 word;

    if (self->used_0C - (self->cursor_14 - self->data_04) < 4) {
        return 0;
    }
    memcpy(&raw, self->cursor_14, 4);
    word = getNetworkLogger()->decode_50(raw);
    memcpy(out, &word, 4);
    self->cursor_14 += 4;
    return 4;
}

/* Takes a big-endian half-word into `out`; 2, or 0. */
s32 networkPacket_takeU16(NetworkStreamWriter* self, u16* out)
{
    u16 raw;
    u16 half;

    if (self->used_0C - (self->cursor_14 - self->data_04) < 2) {
        return 0;
    }
    memcpy(&raw, self->cursor_14, 2);
    half = getNetworkLogger()->flag_48(raw);
    memcpy(out, &half, 2);
    self->cursor_14 += 2;
    return 2;
}

/* Takes a byte into `out`; 1, or 0. */
s32 networkPacket_takeByte(NetworkStreamWriter* self, u8* out)
{
    u8* data = self->data_04;
    u8* cursor = self->cursor_14;

    if (self->used_0C - (cursor - data) < 1) {
        return 0;
    }
    *out = *cursor;
    self->cursor_14 = cursor + 1;
    return 1;
}

/* The size of the current message counting only the parts `mask` selects. */
u32 networkPacket_getMessageSize(NetworkStreamWriter* self, u32 mask)
{
    u16 size;

    size = networkPacket_getPayloadSize(self) + 3;
    if ((u8)mask & 0x40) {
        size += networkPacket_hasTimestamp(self) ? 4 : 0;
    }
    if ((u8)mask & 0x3F) {
        size += (u16)((networkPacket_getValueCount(self) ? 4 : 0) + networkPacket_getValueCount(self) * 4);
    }
    return size;
}

/* The current message's payload size. */
u16 networkPacket_getPayloadSize(NetworkStreamWriter* self)
{
    u16 raw;

    memcpy(&raw, self->message_10, 2);
    return getNetworkLogger()->flag_48(raw);
}

/* The current message's header size: 3, the timestamp and the value block. */
u16 networkPacket_getHeaderSize(NetworkStreamWriter* self)
{
    s32 block = networkPacket_getValueCount(self) ? 4 : 0;
    s32 stamp = networkPacket_hasTimestamp(self) ? 4 : 0;

    return stamp + block + networkPacket_getValueCount(self) * 4 + 3;
}

/* True when the current message carries user data. */
s32 networkPacket_isUserData(NetworkStreamWriter* self)
{
    return (self->message_10[2] & 0x80) != 0;
}

/* True when the current message carries a timestamp. */
s32 networkPacket_hasTimestamp(NetworkStreamWriter* self)
{
    return (self->message_10[2] & 0x40) != 0;
}

/* The current message's timestamp in seconds (0 when it carries none). */
f32 networkPacket_getTimestamp(NetworkStreamWriter* self)
{
    u32 raw;

    if (networkPacket_hasTimestamp(self) == 0) {
        return 0.0f;
    }
    memcpy(&raw, self->message_10 + 3, 4);
    return (s32)getNetworkLogger()->decode_50(raw) / 1000.0f;
}

/* The number of values the current message carries. */
u8 networkPacket_getValueCount(NetworkStreamWriter* self)
{
    return self->message_10[2] & 0x3F;
}

/* The bytes the packet holds. */
u32 networkPacket_getLength(NetworkStreamWriter* self)
{
    return (u16)self->used_0C;
}

/* The largest message header: 3, a timestamp, the value word and four values. */
u32 networkPacket_getMessageOverhead(void)
{
    return 0x17;
}

/* The framed writer's flush override: nothing to hand on. */
void networkStreamWriter_flushFrame(NetworkStreamWriterDefault* self)
{
}

/* Binds the framed writer to `size` empty bytes at `data`; the first frame starts there (the writer's `attach`). */
/* untyped: byte range - the bytes appended to the stream */
void networkStreamWriter_putBytes(NetworkStreamWriterDefault* self, const void* data, u32 size)
{
    self->NetworkStreamSink::attach((u8*)data, size);
    self->message_10 = (u8*)data;
}

/* Appends an empty frame header (the version and the frame flag 4); 22, or -1. */
s32 networkStreamWriter_flush(NetworkStreamWriterDefault* self)
{
    u16 half;

    if (self->capacity_08 - self->used_0C < 22) {
        return -1;
    }
    memset(self->data_04 + self->used_0C, 0, 22);
    half = getNetworkLogger()->encode_4C(networkFrameVersion);
    memcpy(self->data_04 + self->used_0C, &half, 2);
    (self->data_04 + self->used_0C)[0x10] = 4;
    self->used_0C += 22;
    return 22;
}

/* Appends the packet's current message under `mask`; the bytes, or -1. */
s32 networkStreamWriter_putPacket(NetworkStreamWriterDefault* self, NetworkStreamWriter* packet, u32 mask)
{
    s32 n;

    if (networkPacket_hasMessage(packet) == 0) {
        return -1;
    }
    n = networkPacket_copyMessage(packet, self->data_04 + self->used_0C, self->capacity_08 - self->used_0C,
                                       mask);
    if (n < 0) {
        return -1;
    }
    self->used_0C += n;
    return n;
}

/* The bytes the framed writer holds. */
u16 getPosition(NetworkStreamWriterDefault* self)
{
    return self->used_0C;
}

/* Reserves `size` bytes at the end of the framed writer; the size, or -1. */
s32 networkStreamWriter_skip(NetworkStreamWriterDefault* self, u16 size)
{
    if (self->capacity_08 - self->used_0C < size) {
        return -1;
    }
    self->used_0C += size;
    return size;
}

/* Stores the frame's payload length (its +0x02 half-word). */
void networkStreamWriter_setMode(NetworkStreamWriterDefault* self, u16 value)
{
    u16 half;

    half = getNetworkLogger()->encode_4C(value);
    memcpy(self->message_10 + 2, &half, 2);
}

/* Stores the frame's first sequence number (+0x04). */
void networkStreamWriter_putU16(NetworkStreamWriterDefault* self, u16 value)
{
    u16 half;

    half = getNetworkLogger()->encode_4C(value);
    memcpy(self->message_10 + 4, &half, 2);
}

/* Stores the frame's second sequence number (+0x06). */
void networkStreamWriter_putU16b(NetworkStreamWriterDefault* self, u16 value)
{
    u16 half;

    half = getNetworkLogger()->encode_4C(value);
    memcpy(self->message_10 + 6, &half, 2);
}

/* Stores the frame's source nonce (+0x08). */
void networkStreamWriter_putU32(NetworkStreamWriterDefault* self, u32 value)
{
    u32 word;

    word = getNetworkLogger()->encode_54(value);
    memcpy(self->message_10 + 8, &word, 4);
}

/* Stores the frame's session nonce (+0x0C). */
void networkStreamWriter_putU32b(NetworkStreamWriterDefault* self, u32 value)
{
    u32 word;

    word = getNetworkLogger()->encode_54(value);
    memcpy(self->message_10 + 0xC, &word, 4);
}

/* Sets the frame flag 0x10 for `value` 0 or 0x20 for 1. */
void networkStreamWriter_enable1(NetworkStreamWriterDefault* self, u32 value)
{
    if ((u8)value == 0) {
        self->message_10[0x11] |= 0x10;
    } else if ((u8)value == 1) {
        self->message_10[0x11] |= 0x20;
    }
}

/* Sets the frame's channel flag: 1 for channel 0, 2 for channel 1. */
void networkStreamWriter_enable2(NetworkStreamWriterDefault* self, u32 value)
{
    if ((u8)value == 0) {
        self->message_10[0x11] |= 1;
    } else if ((u8)value == 1) {
        self->message_10[0x11] |= 2;
    }
}

/* Sets or clears the frame flag 0x04. */
void networkStreamWriter_setFlag04(NetworkStreamWriterDefault* self, s32 on)
{
    if (on) {
        self->message_10[0x11] |= 4;
    } else {
        self->message_10[0x11] &= ~4;
    }
}

/* Sets or clears the frame's handshake flag (0x08). */
void networkStreamWriter_enable3(NetworkStreamWriterDefault* self, u32 value)
{
    if (value) {
        self->message_10[0x11] |= 8;
    } else {
        self->message_10[0x11] &= ~8;
    }
}

/* Seals the frame: clears the CRC and key bytes, then stores the sink's checksum over the whole frame. */
void networkStreamWriter_commit(NetworkStreamWriterDefault* self)
{
    networkStreamWriter_setCrc(self, 0);
    networkStreamWriter_setKeyA(self, 0);
    networkStreamWriter_setKeyB(self, 0);
    networkStreamWriter_setCrc(self, self->checksum(read_size_from_buffer(self)));
}

/* Stores the frame's CRC (+0x12). */
void networkStreamWriter_setCrc(NetworkStreamWriterDefault* self, u16 crc)
{
    u16 half;

    half = getNetworkLogger()->encode_4C(crc);
    memcpy(self->message_10 + 0x12, &half, 2);
}

/* Stores the frame's first key byte (+0x14). */
void networkStreamWriter_setKeyA(NetworkStreamWriterDefault* self, u8 key)
{
    self->message_10[0x14] = key;
}

/* Stores the frame's second key byte (+0x15). */
void networkStreamWriter_setKeyB(NetworkStreamWriterDefault* self, u8 key)
{
    self->message_10[0x15] = key;
}

/* Binds the reader to `size` received bytes at `block`; the first frame starts there (the writer's `bind`). */
void networkStreamReader_attach(NetworkStreamWriterDefault* self, u8* block, u32 size)
{
    self->NetworkStreamSink::bind(block, size);
    self->message_10 = block;
}

/* True when a whole frame sits at the current position. */
s32 make_sure_enough_space(NetworkStreamWriterDefault* self)
{
    if (self->used_0C - (self->message_10 - self->data_04) < 22) {
        return 0;
    }
    return self->used_0C - (self->message_10 - self->data_04) >= read_size_from_buffer(self);
}

/* Copies the current frame into `out`; its size, 0 when no whole frame is there, -1 when `capacity` is too
   small. */
s32 copy_from_buffer(NetworkStreamWriterDefault* self, u8* out, u32 capacity)
{
    u16 size;

    if (self->used_0C - (self->message_10 - self->data_04) < 22) {
        return 0;
    }
    size = read_size_from_buffer(self);
    if (self->used_0C - (self->message_10 - self->data_04) < size) {
        return 0;
    }
    if (capacity < size) {
        return -1;
    }
    memcpy(out, self->message_10, size);
    return size;
}

/* Drops the current frame from the block, closing the gap; its size, or 0 when no whole frame is there. */
s32 networkStreamReader_consumePacket(NetworkStreamWriterDefault* self)
{
    u16 size;

    if (self->used_0C - (self->message_10 - self->data_04) < 22) {
        return 0;
    }
    size = read_size_from_buffer(self);
    if (self->used_0C - (self->message_10 - self->data_04) < size) {
        return 0;
    }
    self->used_0C -= size;
    if (self->used_0C != 0) {
        memmove(self->message_10, self->message_10 + size, self->used_0C);
    }
    return size;
}

/* The current frame's payload, or NULL when no whole frame is there. */
u8* networkStreamReader_getPayload(NetworkStreamWriterDefault* self)
{
    u8* frame = self->message_10;

    if (self->used_0C - (frame - self->data_04) < 22) {
        return NULL;
    }
    if (self->used_0C - (frame - self->data_04) < read_size_from_buffer(self)) {
        return NULL;
    }
    return frame + 22;
}

/* The current frame. */
u8* networkStreamReader_getFrame(NetworkStreamWriterDefault* self)
{
    return self->message_10;
}

/* The current frame's version (+0x00). */
u16 networkStreamReader_getVersion(NetworkStreamWriterDefault* self)
{
    u16 raw;

    memcpy(&raw, self->message_10, 2);
    return getNetworkLogger()->flag_48(raw);
}

/* The current frame's size: its payload length plus the 22-byte header. */
u16 read_size_from_buffer(NetworkStreamWriterDefault* self)
{
    return networkStreamReader_getPayloadSize(self) + 22;
}

/* The current frame's payload length (+0x02). */
u16 networkStreamReader_getPayloadSize(NetworkStreamWriterDefault* self)
{
    u16 raw;

    memcpy(&raw, self->message_10 + 2, 2);
    return getNetworkLogger()->flag_48(raw);
}

/* The frame header's size. */
u32 networkPacket_getFrameOverhead(void)
{
    return 22;
}

/* The frame's first sequence number (+0x04). */
u16 networkPacket_getSequenceA(NetworkStreamWriter* self)
{
    u16 raw;

    memcpy(&raw, self->message_10 + 4, 2);
    return getNetworkLogger()->flag_48(raw);
}

/* The frame's second sequence number (+0x06). */
u16 networkPacket_getSequenceB(NetworkStreamWriter* self)
{
    u16 raw;

    memcpy(&raw, self->message_10 + 6, 2);
    return getNetworkLogger()->flag_48(raw);
}

/* The frame's source nonce (+0x08). */
u32 networkPacket_getSourceNonce(NetworkStreamWriter* self)
{
    u32 raw;

    memcpy(&raw, self->message_10 + 8, 4);
    return getNetworkLogger()->decode_50(raw);
}

/* The frame's session nonce (+0x0C). */
u32 networkPacket_getSessionNonce(NetworkStreamWriter* self)
{
    u32 raw;

    memcpy(&raw, self->message_10 + 0xC, 4);
    return getNetworkLogger()->decode_50(raw);
}

/* The frame flag `networkStreamWriter_enable1` sets: 0 for 0x10, 1 for 0x20. */
s32 networkPacket_getFlag10(NetworkStreamWriter* self)
{
    u8 flags = self->message_10[0x11];

    if (flags & 0x10) {
        return 0;
    }
    return (flags & 0x20) != 0;
}

/* The frame's channel: 0 for flag 1, 1 for flag 2. */
u8 networkPacket_getChannel(NetworkStreamWriter* self)
{
    u8 flags = self->message_10[0x11];

    if (flags & 1) {
        return 0;
    }
    return (flags & 2) != 0;
}

/* True when the frame flag 0x04 is set. */
s32 networkPacket_getFlag04(NetworkStreamWriter* self)
{
    return (self->message_10[0x11] & 4) != 0;
}

/* True for a handshake frame (flag 0x08). */
s32 networkPacket_isHandshake(NetworkStreamWriter* self)
{
    return (self->message_10[0x11] & 8) != 0;
}

/* The frame's CRC (+0x12). */
u16 networkStreamReader_getCrc(NetworkStreamWriterDefault* self)
{
    u16 raw;

    memcpy(&raw, self->message_10 + 0x12, 2);
    return getNetworkLogger()->flag_48(raw);
}

/* The frame's first key byte (+0x14). */
u8 networkStreamReader_getKeyA(NetworkStreamWriterDefault* self)
{
    return self->message_10[0x14];
}

/* The frame's second key byte (+0x15). */
u8 networkStreamReader_getKeyB(NetworkStreamWriterDefault* self)
{
    return self->message_10[0x15];
}

/* Scrambles the payload with a random key byte and hides the key in the header: an odd key goes to +0x15
   inverted (with an even random byte at +0x14), an even one to +0x14 inverted. */
void networkStreamWriter_bytes(NetworkStreamWriterDefault* self)
{
    u8 key = rand();

    self->NetworkStreamSink::encrypt(key, 22, networkStreamReader_getPayloadSize(self));
    if (key & 1) {
        networkStreamWriter_setKeyA(self, rand() & 0xFE);
        networkStreamWriter_setKeyB(self, ~key);
    } else {
        networkStreamWriter_setKeyA(self, ~key);
        networkStreamWriter_setKeyB(self, rand());
    }
}

/* Unscrambles the payload with the key the header hides (the inverse of `networkStreamWriter_bytes`). */
void networkStreamReader_decryptFrame(NetworkStreamWriterDefault* self)
{
    u8 key = ~networkStreamReader_getKeyA(self);

    if (key & 1) {
        key = ~networkStreamReader_getKeyB(self);
    }
    self->NetworkStreamSink::decrypt(key, 22, networkStreamReader_getPayloadSize(self));
}

/* Checks a received frame: its CRC against the sink's checksum, then its version; a bad frame is logged and
   flushed. */
s32 networkStreamReader_test(NetworkStreamWriterDefault* self)
{
    u16 crc;
    u16 computed;
    u16 version;

    crc = networkStreamReader_getCrc(self);
    networkStreamWriter_setCrc(self, 0);
    networkStreamWriter_setKeyA(self, 0);
    networkStreamWriter_setKeyB(self, 0);
    computed = self->checksum(read_size_from_buffer(self));
    if (crc != computed) {
        getNetworkLogger()->warn_10("NetworkUnitPacket::test: CRC error %04x:%04x\n", crc, computed);
        self->flush();
        return 0;
    }
    version = networkStreamReader_getVersion(self);
    if (version != networkFrameVersion) {
        getNetworkLogger()->warn_10("NetworkUnitPacket::test: Invalid version %04x:%04x\n", version,
                                    networkFrameVersion);
        self->flush();
        return 0;
    }
    return 1;
}

}

/* Builds an empty queue: the sink's block, sequence 0 and no read cursor. */
NetworkStreamQueue::NetworkStreamQueue()
{
    sequence_10 = 0;
    cursor_14 = NULL;
}

/* Releases the queue; the block belongs to the caller. */
NetworkStreamQueue::~NetworkStreamQueue()
{
}

extern "C" {

/* Appends the packet's current message under `mask`; the bytes, or a negative result. */
s32 networkStreamQueue_append(NetworkStreamQueue* self, NetworkStreamWriter* packet, u32 mask)
{
    s32 n;

    n = networkPacket_copyMessage(packet, self->data_04 + self->used_0C, self->capacity_08 - self->used_0C, mask);
    if (n < 0) {
        return n;
    }
    self->used_0C += n;
    return n;
}

/* Appends every whole message left in the packet; the bytes, or the first negative result. */
s32 networkStreamQueue_appendAll(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    s32 total;
    s32 n;

    total = 0;
    while (networkPacket_hasMessage(packet) != 0) {
        n = networkStreamQueue_append(self, packet, 0xFF);
        if (n < 0) {
            return n;
        }
        total += n;
        networkPacket_nextMessage(packet);
    }
    return total;
}

/* Binds `packet` to the bytes the queue holds. */
void networkStreamQueue_fillPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    packet->bind(self->data_04, self->used_0C);
}

/* Drops whole messages from the front until `bytes` (counted under `mask`) are gone, then closes the gap. */
void networkStreamQueue_discard(NetworkStreamQueue* self, s32 bytes, u8 mask)
{
    NetworkStreamWriter packet;
    s32 removed;
    s32 left;

    removed = 0;
    networkPacket_bind(&packet, self->data_04, self->used_0C);
    while (networkPacket_hasMessage(&packet) != 0) {
        bytes -= (u16)networkPacket_getMessageSize(&packet, mask);
        removed += (u16)networkPacket_getMessageSize(&packet, 0xFF);
        if (bytes <= 0) {
            break;
        }
        networkPacket_nextMessage(&packet);
    }
    left = self->used_0C - removed;
    if (left > 0) {
        memmove(self->data_04, self->data_04 + removed, left);
    }
    self->used_0C = left;
}

/* Empties the queue. */
void networkStreamQueue_clear(NetworkStreamQueue* self)
{
    self->used_0C = 0;
}

/* Sets the sequence number the queue expects next. */
void networkStreamQueue_setSequence(NetworkStreamQueue* self, u16 sequence)
{
    self->sequence_10 = sequence;
}

/* The sequence number the queue expects next. */
/* untyped: opaque handle passed through - the session hands the queue it holds */
u32 networkStreamWriter_size(const void* sub)
{
    return ((const NetworkStreamQueue*)sub)->sequence_10;
}

/* Files the payload of the received frame by its first sequence number when it starts at or before the
   expected one: 0 when it is old, 2 when it starts beyond, 3 when the new part was appended, or an error
   source when the queue is full. */
u32 putTopPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    NetworkStreamWriter messages;
    NetworkStreamWriterDefault* frame = (NetworkStreamWriterDefault*)packet;
    s32 first;
    s32 expected;
    s32 size;
    s32 skip;
    u32 result;

    size = networkStreamReader_getPayloadSize(frame);
    networkStreamQueue_unwrapSequence(self, self->sequence_10, networkPacket_getSequenceA(packet), &expected, &first);
    if (first + size <= expected) {
        result = 0;
    } else if (expected < first) {
        result = 2;
    } else {
        skip = expected - first;
        networkPacket_bind(&messages, networkStreamReader_getPayload(frame) + skip, size - skip);
        if (networkStreamQueue_appendAll(self, &messages) < 0) {
            getNetworkLogger()->log_14("NetworkUnitPacketPool::putTopPacket: recv buf over\n");
            return 0x80030032;
        }
        self->sequence_10 = first + size;
        result = 3;
    }
    return result;
}

/* Files the payload of the received frame when its first sequence number is not beyond the expected one:
   0 when it is old or beyond, 1 when it was appended, or an error source when the queue is full. */
u32 networkStreamQueue_putNextPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    NetworkStreamWriter messages;
    NetworkStreamWriterDefault* frame = (NetworkStreamWriterDefault*)packet;
    s32 first;
    s32 expected;
    s32 size;
    u32 result;

    size = networkStreamReader_getPayloadSize(frame);
    result = 0;
    networkStreamQueue_unwrapSequence(self, self->sequence_10, networkPacket_getSequenceA(packet), &expected, &first);
    if (first + size <= expected) {
        result = 0;
    } else if (expected <= first) {
        networkPacket_bind(&messages, networkStreamReader_getPayload(frame), size);
        if (networkStreamQueue_appendAll(self, &messages) < 0) {
            getNetworkLogger()->log_14("NetworkUnitPacketPool::putLowPacket: recv buf over\n");
            return 0x80030032;
        }
        self->sequence_10 = first + size;
        result = 1;
    }
    return result;
}

/* Appends the whole payload of the received frame: 1, or an error source when the queue is full. */
u32 putAllPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    NetworkStreamWriter messages;
    NetworkStreamWriterDefault* frame = (NetworkStreamWriterDefault*)packet;
    u32 size;

    size = networkStreamReader_getPayloadSize(frame);
    networkPacket_bind(&messages, networkStreamReader_getPayload(frame), size);
    if (networkStreamQueue_appendAll(self, &messages) < 0) {
        getNetworkLogger()->log_14("NetworkUnitPacketPool::putAllPacket: recv buf over\n");
        return 0x80030032;
    }
    return 1;
}

/* Drops what the frame's second sequence number acknowledges and moves the expected number up to it. */
void networkStreamQueue_acknowledge(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    s32 acknowledged;
    s32 expected;

    networkStreamQueue_unwrapSequence(self, self->sequence_10, networkPacket_getSequenceB(packet), &expected,
                                      &acknowledged);
    if (expected < acknowledged) {
        networkStreamQueue_discard(self, acknowledged - expected, 0xBF);
        self->sequence_10 = acknowledged;
    }
}

/* Lifts whichever of two 16-bit sequence numbers wrapped by 0x10000 so the two compare the short way round. */
void networkStreamQueue_unwrapSequence(NetworkStreamQueue* self, u16 own, u16 other, s32* ownOut, s32* otherOut)
{
    s32 a = own;
    s32 b = other;

    if ((b + 0x10000) - a < a - b) {
        b += 0x10000;
    } else if ((a + 0x10000) - b < b - a) {
        a += 0x10000;
    }
    *ownOut = a;
    *otherOut = b;
}

/* Moves the read cursor to the front of the queue. */
void networkStreamQueue_rewind(NetworkStreamQueue* self)
{
    self->cursor_14 = self->data_04;
}

/* True when a whole message sits under the read cursor. */
s32 networkStreamQueue_hasMessage(NetworkStreamQueue* self)
{
    NetworkStreamWriter packet;

    networkPacket_bind(&packet, self->cursor_14, self->used_0C - (self->cursor_14 - self->data_04));
    return networkPacket_hasMessage(&packet);
}

/* Steps the read cursor over the message under it. */
void networkStreamQueue_skipMessage(NetworkStreamQueue* self)
{
    NetworkStreamWriter packet;

    networkPacket_bind(&packet, self->cursor_14, self->used_0C - (self->cursor_14 - self->data_04));
    if (networkPacket_hasMessage(&packet) == 0) {
        return;
    }
    self->cursor_14 += (u16)networkPacket_getMessageSize(&packet, 0xFF);
}

/* Binds `packet` to the bytes from the read cursor on. */
void networkStreamQueue_peekPacket(NetworkStreamQueue* self, NetworkStreamWriter* packet)
{
    packet->bind(self->cursor_14, self->used_0C - (self->cursor_14 - self->data_04));
}

/* Removes the message under the read cursor, closing the gap. */
void networkStreamQueue_removeMessage(NetworkStreamQueue* self)
{
    NetworkStreamWriter packet;
    u32 left;
    u16 size;

    left = self->used_0C - (self->cursor_14 - self->data_04);
    networkPacket_bind(&packet, self->cursor_14, left);
    if (networkPacket_hasMessage(&packet) == 0) {
        return;
    }
    size = networkPacket_getMessageSize(&packet, 0xFF);
    self->used_0C -= size;
    if (left - size != 0) {
        memmove(self->cursor_14, self->cursor_14 + size, left - size);
    }
}

/* The packet's flush hook override: forwards to the sink's. */
void networkPacket_onFlush(NetworkStreamWriter* self, u8* data, u32 size)
{
    self->NetworkStreamSink::onFlush(data, size);
}

/* The packet's fill override: forwards to the sink's. */
s32 networkPacket_fill(NetworkStreamWriter* self, u8* out, u32 size)
{
    return self->NetworkStreamSink::fill(out, size);
}

/* The framed writer's flush hook override: forwards to the sink's. */
void networkStreamWriter_onFlush(NetworkStreamWriterDefault* self, u8* data, u32 size)
{
    self->NetworkStreamSink::onFlush(data, size);
}

/* The framed writer's scramble override: forwards to the sink's. */
void networkStreamWriter_encrypt(NetworkStreamWriterDefault* self, u8 key, u16 offset, u16 size)
{
    self->NetworkStreamSink::encrypt(key, offset, size);
}

/* The framed writer's unscramble override: forwards to the sink's. */
void networkStreamWriter_decrypt(NetworkStreamWriterDefault* self, u8 key, u16 offset, u16 size)
{
    self->NetworkStreamSink::decrypt(key, offset, size);
}

}
