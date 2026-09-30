/*
 * Network/NetworkPeerBuffer.cpp - the payload-buffer peer: constructor, the eight table slots and the deleting
 *   destructor.
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CCF30..0x803CD248, `.data` 0x805F9510..0x805F9540, extab 0x800198E0..0x80019940, extabindex
 * 0x8003A0E0..0x8003A134.
 *
 * NAMES.  `NetworkPeerBuffer` and the slot names that are only an offset are GUESSes.  Every name here is the
 * map's or a derived one; the derived ones are marked GUESS in `Network/network_transport_types.h`.
 *
 * TABLE.  Its table (0x805F9510, 0x30 B) is emitted from `NetworkPeerBuffer::destroy`, the key function (rule 10).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  `send` 99.73 % (the `NETWORK_ERROR_*` constant is an immediate the target relocates against an
 * `@eti_` extabindex row, playbook 58's class); the object's `extab` is 0x38 of the claimed 0x60 B and `.text`
 * differs from the target by 3 B (relocations).
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/NetworkSessionManager.h"
#include "unsplit/NetworkData.h"
#include "unsplit/NetworkStream.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers).  It
   cannot be included beside `unsplit/OS.h`: the two band headers declare `OSCreateThread`/`OSResumeThread` with
   different signatures and a TU that sees both fails with `(10197) illegal function overloading`. */
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the memset argument setup
   in source order (`addi` before the two `li`s) - the peephole pass folds both.  Scoped off for the file. */
#pragma peephole off

extern "C" {

#pragma dont_inline on

/* Builds the payload buffer: the error base, the buffer's own table, then an empty payload. */
NetworkPeerBuffer::NetworkPeerBuffer()
{
    memset(this->payload_10, 0, 0x2000);
    this->used_2010 = 0;
}

/* Slots 0x0C/0x18/0x20 of the payload-buffer table and 0x18/0x20 of the Udp peer table are empty or
   constant; nothing in a body pins a role, so the names carry the slot offset the table gives them. */
/* untyped: caller-owned payload - the buffer ignores it */
void NetworkPeerBuffer::setContext(const void* context)
{
}

/* Queues bytes behind the ones already in the payload; -1 (with the record filled) when they do not fit. */
s32 NetworkPeerBuffer::send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind)
{
    if (data == NULL || size <= 0) {
        return 0;
    }
    if (this->used_2010 + size > 0x2000) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0x2000, 0x80000000);
        return -1;
    }
    memcpy(this->payload_10 + this->used_2010, data, size);
    this->used_2010 += size;
    return size;
}

/* Takes the leading packet out of the payload into the caller's buffer; 0 when none is complete or it
   does not fit, else its length. */
s32 NetworkPeerBuffer::receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind)
{
    NetworkStreamWriterDefault stream;
    s32 capacity;
    s32 length;

    networkStreamWriter_constructDefault(&stream);
    capacity = *size;
    *size = 0;
    *size2 = 0;
    *kind = 0;
    networkStreamReader_attach(&stream, this->payload_10, this->used_2010);
    if (make_sure_enough_space(&stream) == 0) {
        networkStreamWriterDefault_dtor(&stream, -1);
        return 0;
    }
    if (capacity < read_size_from_buffer(&stream)) {
        networkStreamWriterDefault_dtor(&stream, -1);
        return 0;
    }
    length = copy_from_buffer(&stream, out, capacity);
    if (length < 0) {
        networkStreamWriterDefault_dtor(&stream, -1);
        return 0;
    }
    this->used_2010 -= networkStreamReader_consumePacket(&stream);
    *size = length;
    networkStreamWriterDefault_dtor(&stream, -1);
    return length;
}

s32 NetworkPeerBuffer::put(const u8* packet, s32 length, u32 a, u32 b, u32 c)
{
    return 0;
}

#pragma dont_inline off

/* Empties the payload and its use count; reports that the buffer is usable. */
s32 NetworkPeerBuffer::move()
{
    memset(this->payload_10, 0, 0x2000);
    this->used_2010 = 0;
    return 1;
}

#pragma dont_inline on

void NetworkPeerBuffer::armDrop()
{
}

#pragma dont_inline off

/* Empties the payload buffer through the class's own clear slot. */
s32 NetworkPeerBuffer::init()
{
    this->reset();
    return 1;
}

/* Empties the payload and its use count. */
void NetworkPeerBuffer::reset()
{
    memset(this->payload_10, 0, 0x2000);
    this->used_2010 = 0;
}

/* Retail keeps the `bl` into the base destructor (measured: folding it drops the whole call, since
   the inlined body's `flags > 0` test is constant-folded away). */
#pragma dont_inline on

/* Deleting destructor of the payload buffer: chains the error-record base, then frees on request. */
NetworkPeerBase* NetworkPeerBuffer::destroy(s16 flags)
{
    if (this != NULL) {
        NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

#pragma dont_inline off

}
