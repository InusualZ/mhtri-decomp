/*
 * include/Network/network_writer_types.h - the bit-stream writer band's classes the session units share.
 *
 * The stream frame objects, the `NetworkBuffer` class and the logger view were
 * declared in `Network/NetworkSessionManager.h`; `Network/NetworkSessionStable.h` needs them below the
 * session class, so they live here once (rule 1) and both headers include this one.  Every type keeps
 * the size and layout evidence it carried there.
 */

#ifndef NETWORK_NETWORK_WRITER_TYPES_H
#define NETWORK_NETWORK_WRITER_TYPES_H

#include "types.h"
#include "Network/network_transport_types.h"   /* NetworkStreamSink */
#include "Network/NetworkUniqueId.h"           /* NetworkUniqueId - the address object the session records embed */

/* ---------------- the bit-stream writer's frame objects (the writer band's classes) ------------- */

/* Two stream-sink classes of the writer band (`lobby/lb_server_sel_trans.cpp` defines their constructors and
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

/* ---------------- bit-stream writer (owned by the network-serialization band) -------------- */

/* The stream buffer every op-code writer puts its packet on.  A polymorphic class (rule 10): its
   table lives at 0x805F9150 in the serialization band, which this unit does not own, so the class
   only *declares* its virtuals - none is defined here, so MWCC emits no table of ours - and the
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

/* ---------------- the manager's logger (its accessor is another band's) ------------------- */

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

#endif /* NETWORK_NETWORK_WRITER_TYPES_H */
