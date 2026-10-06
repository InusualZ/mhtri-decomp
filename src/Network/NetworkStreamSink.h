/*
 * Network/NetworkStreamSink.h - the classes and entry points `Network/NetworkStreamSink.cpp` owns that other units
 *   use: the stream sink and its writers, the stream buffer, the logger view, the mutex pair and the frame writer's
 *   entry points.
 */
#ifndef MHTRI_NETWORK_NETWORKSTREAMSINK_H
#define MHTRI_NETWORK_NETWORKSTREAMSINK_H

#include "types.h"

class NetworkLogger;                /* unsplit/Network.h: the log manager the accessor returns */
class NetworkConnectionStable;      /* Network/NetworkSessionStable.h */

/* ---------------- the peer that owns a byte stream -------------------------------------------- */

/* The sink the stream helpers hand a record to: only its +0x20 and +0x24 slots are called here, the first
   fills the buffer it is given and the second takes a record, each reporting how many bytes it moved
   (GUESS: offset-derived - the range never names the class, no `NetworkPeer*` class carries these two
   slots at +0x20/+0x24, and the helper stays valid through a pointer). */
class NetworkStreamSink {
public:
    NetworkStreamSink();
    /* +0x08 */ virtual ~NetworkStreamSink();
    /* +0x0C (GUESS: the flush hook, empty in this class) */ virtual void onFlush(u8* data, u32 size);
    /* +0x10 (GUESS: hands the stored bytes to `onFlush`) */ virtual void flush();
    /* +0x14 (GUESS: binds the stream to an empty block) */ virtual void attach(u8* block, u32 capacity);
    /* +0x18 (GUESS: zeroes the block) */ virtual void clear();
    /* +0x1C (GUESS: binds the stream to a block that is full) */ virtual void bind(u8* block, u32 size);
    /* +0x20 (GUESS) */ virtual s32 fill(u8* out, u32 size);
    /* +0x24 (GUESS) */ virtual s32 put(const u8* data, u32 size);
    /* +0x28 ("duplicate" in its log strings: copies another buffer's stored bytes in - the roster, peer and session
       address copies dispatch it with the source record; their count, or -1) */ virtual s32 copyFrom(const u8* src);
    /* +0x2C ("NetworkBuffer::equals" in its log strings: whether another buffer stores the same bytes; retail takes the
       other buffer and returns the verdict, which this declaration cannot carry while `NetworkUniqueId` overrides it
       as declared) */ virtual void slot_2C();
    /* +0x30 (GUESS: the framed writer scrambles a frame's payload with a random key byte - `size` bytes from `offset`,
       22 past the frame header) */ virtual void encrypt(u8 key, u16 offset, u16 size);
    /* +0x34 (GUESS: the reader's inverse, with the key the frame header carries) */
    virtual void decrypt(u8 key, u16 offset, u16 size);
    /* +0x38 (GUESS: the frame CRC over the first `size` bytes, stored at +0x12 and compared by the reader's test) */
    virtual u16 checksum(u16 size);

    /* +0x04 */ u8* data_04;         /* the block the stream reads or writes */
    /* +0x08 */ u32 capacity_08;     /* its size */
    /* +0x0C */ u32 used_0C;         /* the bytes it holds */
};   /* size: 0x10 (evidence: the constructor stores the table and three words) */


/* ---------------- the bit-stream writer's frame objects (the writer band's classes) ------------- */

/* Two stream-sink classes of the writer band (this header's unit defines their constructors and
   destructors).  Each constructor chains `NetworkStreamSink`'s and stores its own table - 0x805FCE10 for
   `NetworkStreamWriter` (0x803CB9B4), 0x805FCDD4 for `NetworkStreamWriterDefault` (0x803CB9F0) - and each
   table's +0x08 slot is the matching destructor (0x803CB958, 0x803CB8FC); both destructors chain
   `~NetworkStreamSink` directly, which is also what an inlined `~NetworkStreamWriter` reduces to, so the bodies
   allow either hierarchy - `NetworkStreamWriterDefault : NetworkStreamWriter` is the one the call sites need (the
   writer band's `networkPacket_*` readers take a `NetworkStreamWriter*` and are handed the default writer).
   Only the sizes are evidenced beyond that:
   `NetworkStreamWriter` is the 0x18-byte packet every op-code sender in this range reserves
   (`NetworkSessionStable::send` places it at +0x08 and the 0x1C-byte writer at +0x20), and
   `NetworkStreamWriterDefault` the one `moveOutOfBand` reserves.  Their remaining members belong to the
   writer's own band; only the constructor and destructor are declared, so no table is emitted here. */
class NetworkStreamWriter : public NetworkStreamSink {
public:
    NetworkStreamWriter();
    virtual ~NetworkStreamWriter();

    /* +0x10 */ u8* message_10;   /* the message (or, in the framed writer, the frame) the packet stands at */
    /* +0x14 */ u8* cursor_14;    /* the read cursor in the current message's payload */
};   /* size: 0x18 */

class NetworkStreamWriterDefault : public NetworkStreamWriter {
public:
    NetworkStreamWriterDefault();
    virtual ~NetworkStreamWriterDefault();

    u8 unused_18[0x04];   /* +0x18..+0x1B */
};   /* size: 0x1C */

/* ---------------- the stream buffer (its table 0x805F9150 is this unit's .data) ------------- */

/* The stream buffer every op-code writer puts its packet on.  A polymorphic class (rule 10): its
   table lives at 0x805F9150 in `Network/NetworkStreamSink.cpp`'s .data, whose bodies are unwritten, so the class
   only *declares* its virtuals - none is defined anywhere, so MWCC emits no table - and the
   slots are the offsets the target's calls address (a declared virtual at index i is at +8+4*i).
   The three never-called groups are the dispatch holes between the called slots: the entry points
   this unit reaches are named from what the call passes, the eight `slot_NN` slots from their offset
   alone (GUESS - the buffer class's own names are unknown). */
class NetworkBuffer {
public:
    /* +0x08 */ virtual u32 destroy(u32 flags);
    /* +0x0C */ virtual u32 signal(u32 a, const char* fmt, ...);
    /* +0x10 */ virtual u32 begin();
    /* +0x14 */ virtual u32 end();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual u32 available();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual void pad_28();
    /* +0x2C */ virtual void pad_2C();
    /* +0x30 (GUESS: offset-derived) */ virtual void slot_30();
    /* +0x34 (GUESS: offset-derived) */ virtual void slot_34();
    /* +0x38 */ virtual u16 put(u32 a, u32 b, u32 c, u32 d, const void* data, u8 e);  /* untyped: byte range (the packet's data bytes) */
    /* +0x3C */ virtual void pad_3C();
    /* +0x40 */ virtual void pad_40();
    /* +0x44 */ virtual void flush();
    /* +0x48 */ virtual void pad_48();
    /* +0x4C */ virtual void pad_4C();
    /* +0x50 (GUESS: offset-derived) */ virtual void slot_50();
    /* +0x54 (GUESS: offset-derived) */ virtual void slot_54();
    /* +0x58 */ virtual void pad_58();
    /* +0x5C (GUESS: offset-derived) */ virtual void slot_5C();
    /* +0x60 (GUESS: offset-derived) */ virtual void slot_60();
    /* +0x64 (GUESS: offset-derived) */ virtual void slot_64();
    /* +0x68 (GUESS: offset-derived) */ virtual void slot_68();
    /* +0x6C */ virtual void pad_6C();
    /* +0x70 */ virtual void pad_70();
    /* +0x74 */ virtual void pad_74();
    /* +0x78 */ virtual void pad_78();
    /* +0x7C */ virtual void pad_7C();
    /* +0x80 */ virtual void pad_80();
    /* +0x84 */ virtual void pad_84();
    /* +0x88 */ virtual void pad_88();
    /* +0x8C */ virtual f32 getFloat(s32 idx);
    /* +0x90 */ virtual s32 getInt(s32 idx);
};   /* size: 0x04 - only ever reached through a pointer in this unit */

/* ---------------- the manager's logger view (`getNetworkLogger` below returns the object) -- */

typedef struct NetworkManagerLoggerVtable {
    u8 pad00[0x0C];
    void (*verbose_0C)(void* self, u32 level, const char* fmt, ...);   /* +0x0C */
    void (*warn_10)(void* self, const char* fmt, ...);                 /* +0x10 */
    void (*log_14)(void* self, const char* fmt, ...);                  /* +0x14 */
    u8 pad18[0x48];
    f32 (*getTime_60)(void* self);                                     /* +0x60 */
    u8 pad64[0x08];              /* the real vtable is longer; only the called slots are named */
} NetworkManagerLoggerVtable;   /* size: 0x6C (approximation) */

typedef struct NetworkSessionManagerLogger {
    NetworkManagerLoggerVtable* vtable;   /* +0x00 */
} NetworkSessionManagerLogger;   /* size: 0x04 */


extern "C" {

/* 0x803C9974 - the game's debug/log manager (the network library singleton, read through `mpInstance`). */
NetworkLogger* getNetworkLogger(void);

/* 0x803CA338 / 0x803CA37C - destroy (flags -1 at every call site) and construct the member mutex the network
 * records embed (the constructor references the 0x10-byte .data 0x805F91E0).  The callers hand a byte block
 * whose layout only this unit's class owns. */
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_destroyMutex(void* self, s32 flags);
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_initMutex(void* self);

/* 0x803CBA9C / 0x803CBB98 - the frame writer's entry points on a connection that `NetworkSessionStable` drives
 * (attach a default writer; hand it `length` bytes of channel `kind`, or none). */
void networkStreamWriter_attach(NetworkConnectionStable* connection, NetworkStreamWriterDefault* stream);
void networkStreamWriter_reserve(NetworkConnectionStable* connection, const u8* bytes, u32 length, s8 kind);

}

#endif /* MHTRI_NETWORK_NETWORKSTREAMSINK_H */
