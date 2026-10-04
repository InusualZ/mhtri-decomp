/*
 * Network/NetworkPeerUdp.cpp - the Udp peer: its table slots and deleting destructor (its constructor is outside
 *   the range - an unowned function stores the table) and the shared Udp scratch packet in `.bss`.
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CD248..0x803CD764, `.data` 0x805F9540..0x805F9570, extab 0x80019940..0x80019968, extabindex
 * 0x8003A134..0x8003A170, `.bss` 0x806D2C60..0x806D3240.
 *
 * NAMES.  `NetworkPeerUdp` slot names are GUESSes (the dump names
 * `setPeerAndSocket`/`sendPackets`/`receivePackets`).  Every name here is the map's or a derived one; the derived
 * ones are marked GUESS in `Network/network_transport_types.h`.
 *
 * TABLE.  Its table (0x805F9540, 0x30 B) is emitted from `NetworkPeerUdp::destroy`, the key function (rule 10);
 * the constructor lives outside the range (an unowned function at 0x803CA3BC stores the table).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * SHAPES.  `send`/`receive` declare the error code at function scope (a block-scope `error` takes r28, retail
 * r31) and `receive` declares `cursor, length, length2` in that order (the two length slots at 0xA/0x8).
 *
 * RESIDUALS.  Code only in relocations: the `NETWORK_ERROR_*` immediates (0x8003xxxx) are plain `lis`/`addi`
 * pairs in ours where dtk's split relocates them against `@eti_` extabindex rows; `.text` differs from the
 * target by 18 B, all such relocation fields.  As `Matching` the DOL hash holds (measured 2026-10-03).
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/NetworkSessionManager.h"
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

/* The scratch packet the Udp peer frames its traffic in (the unit's `.bss`). */
u8 networkUdpPacketBuffer[0x5E0];

/* Binds the Udp peer to its slot on the shared Udp socket. */
/* untyped: caller-owned payload - the binding record the Udp peer reads */
void NetworkPeerUdp::setContext(const void* context)
{
    this->peerIndex_10 = ((const NetworkPeerUdpBinding*)context)->peerIndex;
    this->udp_14 = ((const NetworkPeerUdpBinding*)context)->udp;
}

#pragma dont_inline on

/* Frames up to two payloads (each a length prefix, the second with a kind byte) into the shared
   scratch packet and sends it to the peer's slot on the Udp socket; the byte count, or -1. */
s32 NetworkPeerUdp::send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind)
{
    s32 error;
    u16 prefix;
    u16 prefix2;
    s8 kindByte;
    u8* cursor;
    s32 total;
    s32 result;

    kindByte = kind;
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return -1;
    }
    cursor = networkUdpPacketBuffer;
    if (data == NULL || size <= 0) {
        prefix = 0;
        memcpy(cursor, &prefix, 2);
        cursor += 2;
        total = 2;
    } else {
        prefix = getNetworkLogger()->encode_4C((u16)size);
        memcpy(cursor, &prefix, 2);
        cursor += 2;
        memcpy(cursor, data, size);
        cursor += size;
        total = size + 2;
    }
    if (data2 == NULL || size2 <= 0) {
        prefix2 = 0;
        memcpy(cursor, &prefix2, 2);
        total += 2;
    } else {
        prefix2 = getNetworkLogger()->encode_4C((u16)(size2 + 1));
        memcpy(cursor, &prefix2, 2);
        memcpy(cursor + 2, &kindByte, 1);
        total += 3;
        memcpy(cursor + 3, data2, size2);
        total += size2;
    }
    if (getNetworkLogger()->flag_48(prefix) == 0 && getNetworkLogger()->flag_48(prefix2) == 0) {
        return 0;
    }
    result = this->udp_14->send(this->peerIndex_10, networkUdpPacketBuffer, total);
    if (result < 0) {
        error = this->udp_14->getError();
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, this->udp_14->getAvailableToRead(), error);
        result = -1;
    }
    return result;
}

/* Reads one framed packet off the peer's slot into the caller's two buffers (the second one led by a
   kind byte); the raw byte count, 0 when nothing is queued, -1 on failure. */
s32 NetworkPeerUdp::receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind)
{
    s32 error;
    u8* cursor;
    u16 length;
    u16 length2;
    s32 received;
    s32 capacity;
    s32 capacity2;

    capacity = *size;
    capacity2 = *size2;
    *size = 0;
    *size2 = 0;
    *kind = 0;
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return -1;
    }
    received = this->udp_14->receive(this->peerIndex_10, networkUdpPacketBuffer, 0x5DC);
    if (received < 0) {
        error = this->udp_14->getError();
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, this->udp_14->getAvailableToRead(), error);
        return -1;
    }
    if (received == 0) {
        return received;
    }
    cursor = networkUdpPacketBuffer;
    memcpy(&length, cursor, 2);
    cursor += 2;
    length = getNetworkLogger()->flag_48(length);
    if (length != 0 && length <= capacity) {
        memcpy(out, cursor, length);
        cursor += length;
        *size = length;
    } else {
        *size = 0;
    }
    memcpy(&length2, cursor, 2);
    length2 = getNetworkLogger()->flag_48(length2) - 1;
    if (length2 != 0 && length2 <= capacity2) {
        memcpy(kind, cursor + 2, 1);
        memcpy(out2, cursor + 3, length2);
        *size2 = length2;
    } else {
        *kind = 0;
        *size2 = 0;
    }
    return received;
}

s32 NetworkPeerUdp::put(const u8* packet, s32 length, u32 a, u32 b, u32 c)
{
    return 0;
}

/* Clears the peer's queued bytes on the shared Udp socket; -1 (with the record filled) when the peer
   has no socket, else 1. */
s32 NetworkPeerUdp::move()
{
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return -1;
    }
    this->udp_14->reset(this->peerIndex_10);
    return 1;
}

void NetworkPeerUdp::armDrop()
{
}

/* The Udp peer's init slot: reset the peer's queue through its own reset slot and report it usable. */
s32 NetworkPeerUdp::init()
{
    this->reset();
    return 1;
}

/* The tail-called twin of the clear: nothing is reported. */
void NetworkPeerUdp::reset()
{
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return;
    }
    this->udp_14->reset(this->peerIndex_10);
}

/* Deleting destructor: chains the base destructor, then frees on request. */
NetworkPeerBase* NetworkPeerUdp::destroy(s16 flags)
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
