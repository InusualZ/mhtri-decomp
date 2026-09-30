/*
 * Network/network_socket_streams.cpp - the transport's socket users (`NetworkSingleTcp`, `NetworkMultipleUdp`),
 *   the byte-stream helpers and `NetworkResolverBase`.  The log strings name `NetworkSingleTcp::*` and
 *   `NetworkMultipleUdp::*`.
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CE060..0x803CF14C, `.data` 0x805F9610..0x805F9958, extab 0x800199B0..0x80019A68, extabindex
 * 0x8003A1DC..0x8003A2F0.
 *
 * NAMES.  The file name is a GUESS (the range mixes the socket users, the byte stream and the resolver base, and
 * no `__FILE__` string names it); a further cut at `NetworkResolverBase`'s constructor (0x803CF0A8) is possible
 * but has no `.data` evidence.  Every name here is the map's or a derived one; the derived ones are marked GUESS
 * in `Network/network_transport_types.h`.
 *
 * EDGE UNPROVEN: the left edge is the 4-byte `networkPeer_disconnect` stub; its single caller is in the unsplit
 * Network band, so nothing contradicts placing it here, and no evidence pins it to the previous unit either.
 *
 * TABLE.  `NetworkResolverBase`'s table (0x805F9938, 0x20 B) is emitted from its destructor, the key function
 * (rule 10).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  `networkPeerStream_takeRecord` 81.39 % (retail branches forward to the shared zero-store where ours
 * falls through, plus one `lhz` reload of the address-taken length local; both spellings were tried);
 * `NetworkMultipleUdp_receive` 94.06 % (the peer-index/length register pair is swapped: r31 in retail, r29 ours);
 * the object's `.text` is 4 B short of the claim.
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/fn_803D3CE8.h"
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

/* Releases the peer's socket through the peer's own helper. */
void networkPeer_disconnect(NetworkSingleTcp* self)
{
    networkPeer_release(self);
}

#pragma dont_inline off

/* Reads what the socket holds into the receive area, cuts it into length-prefixed packets and hands
   the complete ones to every registered peer; an invalid length releases the socket. */
void receivePatInterfaces(PatReceiver* receiver)
{
    NetworkSingleTcp* self = (NetworkSingleTcp*)receiver;
    s32 received;
    s32 offset;
    s32 index;
    u16 length;

    if (self->handle_04 != NULL && self->handle_04->clearReceive() != 0) {
        do {
            received = self->handle_04->receive(self->recv_20 + self->recvUsed_2420, 0x2400 - self->recvUsed_2420, NULL);
            if (received < 1) {
                break;
            }
            self->recvUsed_2420 += received;
            offset = 0;
            while (offset + 2 <= (s32)self->recvUsed_2420) {
                memcpy(&length, self->recv_20 + offset, 2);
                length = getNetworkLogger()->flag_48(length);
                if (length == 0 || length > 0x400) {
                    getNetworkLogger()->warn_10("NetworkSingleTcp::move: invalid packet. %d\n", length);
                    networkPeer_release(self);
                    return;
                }
                if ((s32)self->recvUsed_2420 < offset + length + 2) {
                    break;
                }
                offset += length + 2;
            }
            for (index = 0; index < 4; index++) {
                if (self->peers_10[index] != NULL && self->peers_10[index]->put(self->recv_20, offset, 0, 0, 0) < 0) {
                    getNetworkLogger()->log_14("NetworkSingleTcp::move: [%d] put failed. 0x%x(0x%x)/0x%x\n",
                                               index, self->recvUsed_2420, received, offset);
                }
            }
            self->recvUsed_2420 -= offset;
            if ((s32)self->recvUsed_2420 > 0) {
                memmove(self->recv_20, self->recv_20 + offset, self->recvUsed_2420);
            }
        } while (received > 0);
    }
}

#pragma dont_inline on

/* Opens the connection's socket and registers the address it was opened on: the socket comes from the
   band's pool, `open`/`setPeer` are the socket's own two slots, and the six address bytes are kept on
   the connection.  Returns 0, or the negative step that failed. */
s32 networkPeer_openSocket(NetworkSingleTcp* self, const NetworkPeerAddress* address)
{
    if (self->handle_04 != NULL) {
        return -1;
    }
    self->handle_04 = networkSocketPool_acquire(getNetworkLogger());
    if (self->handle_04 == NULL) {
        return -2;
    }
    if (self->handle_04->open(1) < 0) {
        networkPeer_release(self);
        return -3;
    }
    if (self->handle_04->setPeer(address) < 0) {
        networkPeer_release(self);
        return -4;
    }
    memcpy(&self->address_08, address, 6);
    return 0;
}

#pragma dont_inline off

/* Closes the peer's socket through the socket's own vtable; -1 when there is no socket. */
s32 networkPeer_closeSocket(NetworkSocketUser* self)
{
    if (self->handle_04 == NULL) {
        return -1;
    }
    return self->handle_04->closeSocket();
}

/* Clears the peer's socket receive buffer through the socket's own vtable; 0 when there is none. */
s32 networkPeer_clearReceiveSocket(NetworkSocketUser* self)
{
    if (self->handle_04 == NULL) {
        return 0;
    }
    return self->handle_04->clearReceive();
}

#pragma dont_inline on

/* Releases the peer's socket: shuts the socket down, hands it back to the pool the peer band
   registers with, and clears the peer's own slot. */
void networkPeer_release(NetworkSingleTcp* self)
{
    if (self->handle_04 != NULL) {
        self->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), self->handle_04);
        self->handle_04 = NULL;
    }
}

#pragma dont_inline off

/* Registers a peer in the connection's first free slot of four; the slot, or -1 (logged) when full. */
s32 NetworkSingleTcp_add(NetworkSingleTcp* self, NetworkPeerMcs* peer)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (self->peers_10[index] == NULL) {
            self->peers_10[index] = peer;
            getNetworkLogger()->signal_0C(3, "NetworkSingleTcp::add: %d\n", index);
            return index;
        }
    }
    getNetworkLogger()->warn_10("NetworkSingleTcp::add: cannot add peer.\n");
    return -1;
}

/* Unregisters a peer from the connection (logged when it is not registered). */
void NetworkSingleTcp_remove(NetworkSingleTcp* self, NetworkPeerMcs* peer)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (self->peers_10[index] == peer) {
            self->peers_10[index] = NULL;
            getNetworkLogger()->signal_0C(3, "NetworkSingleTcp::remove: %d\n", index);
            return;
        }
    }
    getNetworkLogger()->warn_10("NetworkSingleTcp::remove: cannot remove peer.\n");
}

/* Empties the peer's own 0x2400-byte receive area. */
void networkPeer_clearReceiveBuffer(NetworkSingleTcp* self)
{
    memset(self->recv_20, 0, 0x2400);
    self->recvUsed_2420 = 0;
}

/* Sends bytes on the connection's socket; -1 (logged) when it has none. */
s32 NetworkSingleTcp_send(NetworkSingleTcp* self, const u8* data, s32 size)
{
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkSingleTcp::send: socket is NULL\n");
        return -1;
    }
    return self->handle_04->send(data, size, NULL);
}

/* Same question for the peer class that keeps its socket in the same slot (GUESS: identical body,
   the two classes' slot tables differ). */
s32 networkPeer_getAvailableToRead(NetworkSocketUser* self)
{
    if (self->handle_04 != NULL) {
        return getBytesAvailableToRead(self->handle_04);
    }
    return 0;
}

/* The socket's last error code; 0 when the connection has no socket. */
s32 NetworkSingleTcp_getError(NetworkSingleTcp* self)
{
    if (self->handle_04 != NULL) {
        return networkSocketHandle_getLastError(self->handle_04);
    }
    return 0;
}

#pragma dont_inline on

/* The same teardown for the second peer class in the band (GUESS: identical tail). */
void networkPeer_disconnectSocket(NetworkSingleTcp* self)
{
    networkPeer_releaseSocket(self);
}

#pragma dont_inline off

/* Reads datagrams off the shared socket and queues each one behind the peer whose address sent it
   (logged when the peer's 0x1770-byte queue would overflow). */
void flushPatRequests(PatRequestQueue* queue)
{
    NetworkMultipleUdp* self = (NetworkMultipleUdp*)queue;
    NetworkPeerAddress sender;
    s32 received;
    u16 length;
    s32 index;

    if (self->handle_04 != NULL) {
        while (1) {
            received = self->handle_04->receive(self->datagram_26, 0x5DC, &sender);
            if (received < 1) {
                break;
            }
            length = received;
            for (index = 0; index < 4; index++) {
                if (memcmp(&self->addresses_0E[index], &sender, 6) == 0) {
                    if ((s32)(length + self->used_63C4[index]) > 0x1770) {
                        getNetworkLogger()->log_14("NetworkMultipleUdp::move: buf_recv_peer over. please check NetworkMultipleUdp::MAX_SIZE_BUF_PEER\n");
                    } else {
                        memcpy(self->received_602[index] + self->used_63C4[index], self->datagram_26, length);
                        self->used_63C4[index] += length;
                    }
                    break;
                }
            }
        }
    }
}

#pragma dont_inline on

/* The same teardown for the second peer class in the band (GUESS: identical body, the two classes'
   tables differ). */
void networkPeer_releaseSocket(NetworkSingleTcp* self)
{
    if (self->handle_04 != NULL) {
        self->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), self->handle_04);
        self->handle_04 = NULL;
    }
}

#pragma dont_inline off

/* Forgets the peer that owns an address: its address and its queued bytes (logged). */
void NetworkMultipleUdp_remove(NetworkMultipleUdp* self, const NetworkPeerAddress* address)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (memcmp(&self->addresses_0E[index], address, 6) == 0) {
            memset(&self->addresses_0E[index], 0, 6);
            self->used_63C4[index] = 0;
            getNetworkLogger()->signal_0C(3, "NetworkMultipleUdp::remove: %d.%d.%d.%d:%d\n",
                                          address->ip_00[0], address->ip_00[1], address->ip_00[2], address->ip_00[3],
                                          address->port_04);
            return;
        }
    }
}

/* Drops the bytes queued for one peer slot (logged when the slot or the socket is invalid). */
void NetworkMultipleUdp_reset(NetworkMultipleUdp* self, s32 peerIndex)
{
    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::reset: peer_id is invalid -> %d\n", peerIndex);
        return;
    }
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::reset: socket is NULL\n");
        return;
    }
    self->used_63C4[peerIndex] = 0;
}

/* Sends bytes to one peer slot's address; -1 (logged) when the slot or the socket is invalid. */
s32 NetworkMultipleUdp_send(NetworkMultipleUdp* self, s32 peerIndex, const u8* data, s32 size)
{
    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::send: peer_id is invalid -> %d\n", peerIndex);
        return -1;
    }
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::send: socket is NULL\n");
        return -1;
    }
    return self->handle_04->send(data, size, &self->addresses_0E[peerIndex]);
}

/* Takes the two length-prefixed payloads a peer slot has queued into the caller's buffer; 0 when they
   are incomplete or do not fit, -1 (logged) on an invalid slot or socket, else the bytes taken. */
s32 NetworkMultipleUdp_receive(NetworkMultipleUdp* self, s32 peerIndex, u8* out, s32 capacity)
{
    u16 length;
    u16 length2;
    s32 taken;
    s32 total;
    u8* queue;
    s32* used;

    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: peer_id is invalid -> %d\n", peerIndex);
        return -1;
    }
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: socket is NULL\n");
        return -1;
    }
    used = &self->used_63C4[peerIndex];
    if (*used < 2) {
        return 0;
    }
    queue = self->received_602[peerIndex];
    memcpy(&length, queue, 2);
    length = getNetworkLogger()->flag_48(length);
    if (length == 0 || length > 0x400) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: invalid packet %d from %d.%d.%d.%d:%d\n", length,
                                    self->addresses_0E[peerIndex].ip_00[0], self->addresses_0E[peerIndex].ip_00[1],
                                    self->addresses_0E[peerIndex].ip_00[2], self->addresses_0E[peerIndex].ip_00[3],
                                    self->addresses_0E[peerIndex].port_04);
        *used = 0;
        return 0;
    }
    taken = length + 2;
    if (*used < taken) {
        return 0;
    }
    if (*used < taken + 2) {
        return 0;
    }
    memcpy(&length2, queue + taken, 2);
    length2 = getNetworkLogger()->flag_48(length2);
    taken += 2;
    if (*used < taken + length2) {
        return 0;
    }
    total = taken + length2;
    if (capacity < total) {
        return 0;
    }
    memcpy(out, queue, total);
    *used -= total;
    if (*used > 0) {
        memmove(queue, queue + total, *used);
    }
    return total;
}

/* Asks the peer's socket how many bytes are readable; 0 when it holds no socket. */
s32 getAvailableToRead(NetworkSocketUser* self)
{
    if (self->handle_04 != NULL) {
        return getBytesAvailableToRead(self->handle_04);
    }
    return 0;
}

/* The socket's last error code; 0 when the Udp socket has none. */
s32 NetworkMultipleUdp_getError(NetworkMultipleUdp* self)
{
    if (self->handle_04 != NULL) {
        return networkSocketHandle_getLastError(self->handle_04);
    }
    return 0;
}

/* Returns the stream's byte block. */
u8* networkPeer_getSocket(NetworkByteStream* self)
{
    return self->data_04;
}

/* Returns how many bytes the stream holds. */
u32 networkPeer_getPeerId(NetworkByteStream* self)
{
    return self->cursor_0C;
}

#pragma dont_inline on

/* Fills the stream's leading 0xE-byte record from a sink and advances the cursor over it. */
void networkPeerStream_pullRecord(NetworkByteStream* self, NetworkStreamSink* sink)
{
    if (self->cursor_0C + 0xE <= self->size_08) {
        if (sink->fill(&self->data_04[self->cursor_0C], 0xE) > 0) {
            self->cursor_0C += 0xE;
        }
    }
}

/* Writes the record's u16 length prefix and then its bytes, when both fit. */
void networkPeerStream_putRecord(NetworkByteStream* self, const NetworkPeerRecord* record)
{
    if (self->cursor_0C + record->size_04 + 2 <= self->size_08) {
        networkPeerStream_putU16(self, record->size_04);
        if (record->data_00 != NULL && record->size_04 != 0) {
            memcpy(&self->data_04[self->cursor_0C], record->data_00, record->size_04);
        }
        self->cursor_0C += record->size_04;
    }
}

/* Writes one 4-byte value, encoded through the log manager's own value slot. */
void networkPeerStream_putU32(NetworkByteStream* self, u32 value)
{
    u32 encoded;

    if (self->cursor_0C + 4 > self->size_08) {
        return;
    }
    encoded = getNetworkLogger()->encode_54(value);
    memcpy(&self->data_04[self->cursor_0C], &encoded, 4);
    self->cursor_0C += 4;
}

/* Writes one 2-byte value, encoded through the log manager's own value slot. */
void networkPeerStream_putU16(NetworkByteStream* self, u16 value){
    u16 encoded;

    if (self->cursor_0C + 2 > self->size_08) {
        return;
    }
    encoded = getNetworkLogger()->encode_4C(value);
    memcpy(&self->data_04[self->cursor_0C], &encoded, 2);
    self->cursor_0C += 2;
}

#pragma dont_inline off

/* Appends one byte when the stream's cursor has room for it. */
void networkPeerStream_putByte(NetworkByteStream* self, u8 value)
{
    if (self->cursor_0C + 1 <= self->size_08) {
        self->data_04[self->cursor_0C] = value;
        self->cursor_0C++;
    }
}

/* Hands the stream's leading 0xE-byte record to a sink and drops it from the front. */
void networkPeerStream_forwardRecord(NetworkByteStream* self, NetworkStreamSink* sink)
{
    u32 remaining;

    if (self->cursor_0C < 0xE) {
        return;
    }
    if (sink->put(self->data_04, 0xE) > 0) {
        remaining = self->cursor_0C - 0xE;
        self->cursor_0C = remaining;
        if (remaining != 0) {
            memmove(self->data_04, self->data_04 + 0xE, remaining);
        }
    }
}

/* Copies the stream's leading length-prefixed record into the caller's record when both the stream
   and the caller have room, then drops it from the front. */
void networkPeerStream_takeRecord(NetworkByteStream* self, NetworkPeerRecord* record)
{
    u16 length;

    if (self->cursor_0C < 2) {
        return;
    }
    networkPeerStream_readLength(self, &length);
    if (length > self->cursor_0C || length > record->size_04) {
        record->size_04 = 0;
        return;
    }
    record->size_04 = length;
    if (length == 0) {
        return;
    }
    if (record->data_00 != NULL) {
        memcpy(record->data_00, self->data_04, length);
    }
    self->cursor_0C -= length;
    if (self->cursor_0C != 0) {
        memmove(self->data_04, self->data_04 + length, self->cursor_0C);
    }
}

#pragma dont_inline on

/* Takes the stream's leading 4-byte value, decoded through the log manager's own value slot, and
   drops it from the front. */
void networkPeerStream_takeU32(NetworkByteStream* self, u32* out)
{
    u32 value;
    u32 remaining;

    if (self->cursor_0C < 4) {
        return;
    }
    memcpy(&value, self->data_04, 4);
    *out = getNetworkLogger()->decode_50(value);
    remaining = self->cursor_0C - 4;
    self->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(self->data_04, self->data_04 + 4, remaining);
    }
}

/* Records the stream's leading length prefix into the caller's u16, decoded through the log
   manager's own value slot, then drops the two bytes from the front. */
void networkPeerStream_readLength(NetworkByteStream* self, u16* out)
{
    u16 encoded;
    u32 remaining;

    if (self->cursor_0C < 2) {
        return;
    }
    memcpy(&encoded, self->data_04, 2);
    *out = getNetworkLogger()->flag_48(encoded);
    remaining = self->cursor_0C - 2;
    self->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(self->data_04, self->data_04 + 2, remaining);
    }
}

#pragma dont_inline off

/* Takes the stream's leading byte into the caller's byte and drops it from the front. */
void networkPeerStream_takeByte(NetworkByteStream* self, u8* out)
{
    u32 remaining;
    u8* base;

    if (self->cursor_0C < 1) {
        return;
    }
    base = self->data_04;
    *out = base[0];
    remaining = self->cursor_0C - 1;
    self->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(self->data_04, self->data_04 + 1, remaining);
    }
}

#pragma dont_inline on

/* Builds the resolver base: its table, an empty name and an empty address table. */
NetworkResolverBase::NetworkResolverBase()
{
    memset(this->name_04, 0, 0x200);
    memset(this->records_204, 0, 0x10);
    this->count_214 = 0;
}

/* Deleting destructor of the peer state object: frees on request, nothing else to chain. */
NetworkResolverBase::~NetworkResolverBase()
{
}

#pragma dont_inline off

}
