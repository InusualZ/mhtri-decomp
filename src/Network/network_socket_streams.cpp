/*
 * Network/network_socket_streams.cpp - the transport's socket users (`NetworkSingleTcp`, `NetworkMultipleUdp`, named by
 *   their log strings), the byte-stream helpers and `NetworkResolverBase`.
 * RANGE. .text 0x803CE060-0x803CF14C (35 functions); .data 0x805F9610-0x805F9958, extab, extabindex.  One TU of the
 *   transport band: docs/network.md (a further cut at `NetworkResolverBase`'s constructor, 0x803CF0A8,
 *   has no `.data` evidence; the left edge, the 4-byte `NetworkSingleTcp::disconnect` stub, is unproven).
 * FLAGS. `-O3 -pool off` (configure.py; measured in docs/network.md); file-scope `#pragma peephole off`
 *   (playbook 39).
 * NAMES. The file name is a GUESS (no `__FILE__` string).  GUESSes: `NetworkByteStream` and its `getData`/`getSize`
 *   (they return `data_04`/`cursor_0C`), `readLength`, and the Tcp members `open`/`close`/`clearReceive`/`release`/
 *   `disconnect`; `NetworkStreamSink` is the offset-derived interface of the two record helpers (no `NetworkPeer*` class
 *   has `fill`/`put` at +0x20/+0x24).  The rest are marked in `Network/network_transport_types.h`.
 * RESIDUALS. `NetworkMultipleUdp::receive`: register colouring only - retail gives the peer index/`taken` r31, the
 *   used-count pointer r30 and the length/total r29, ours r29/r31/r30 (all 720 declaration orders measured).
 * SHAPES. Every function is a member (rule 13).  Tcp and Udp each own a copy of `release`/`disconnect`/
 *   `getAvailableToRead`/`getError` (identical bodies, Tcp run from 0x803CE060, Udp from 0x803CE5EC), so
 *   `NetworkSocketUser` stays a data base; `close`/`clearReceive`/`open`/`clearReceiveBuffer` exist once, in the Tcp
 *   run. `NetworkResolverBase`'s table (0x805F9938) is emitted from its destructor, the key function (rule 10).
 *   `takeRecord` copies the address-taken length into a register local before testing it and keeps the zero-store in an
 *   `else` (retail's shared tail); `receive` computes the running total as `taken + 2` before testing it.
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/NetworkSessionManager.h"
#include "Network/NetworkSocketWii.h"
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
void NetworkSingleTcp::disconnect()
{
    this->release();
}

#pragma dont_inline off

/* Reads what the socket holds into the receive area, cuts it into length-prefixed packets and hands
   the complete ones to every registered peer; an invalid length releases the socket. */
void NetworkSingleTcp::move()
{
    s32 received;
    s32 offset;
    s32 index;
    u16 length;

    if (this->handle_04 != NULL && this->handle_04->clearReceive() != 0) {
        do {
            received = this->handle_04->receive(this->recv_20 + this->recvUsed_2420, 0x2400 - this->recvUsed_2420, NULL);
            if (received < 1) {
                break;
            }
            this->recvUsed_2420 += received;
            offset = 0;
            while (offset + 2 <= (s32)this->recvUsed_2420) {
                memcpy(&length, this->recv_20 + offset, 2);
                length = getNetworkLogger()->flag_48(length);
                if (length == 0 || length > 0x400) {
                    getNetworkLogger()->warn_10("NetworkSingleTcp::move: invalid packet. %d\n", length);
                    this->release();
                    return;
                }
                if ((s32)this->recvUsed_2420 < offset + length + 2) {
                    break;
                }
                offset += length + 2;
            }
            for (index = 0; index < 4; index++) {
                if (this->peers_10[index] != NULL && this->peers_10[index]->put(this->recv_20, offset, 0, 0, 0) < 0) {
                    getNetworkLogger()->log_14("NetworkSingleTcp::move: [%d] put failed. 0x%x(0x%x)/0x%x\n",
                                               index, this->recvUsed_2420, received, offset);
                }
            }
            this->recvUsed_2420 -= offset;
            if ((s32)this->recvUsed_2420 > 0) {
                memmove(this->recv_20, this->recv_20 + offset, this->recvUsed_2420);
            }
        } while (received > 0);
    }
}

#pragma dont_inline on

/* Opens the connection's socket from the band's pool through its `open`/`setPeer` slots and keeps the six address
   bytes; 0, or the negative step that failed. */
s32 NetworkSingleTcp::open(const NetworkPeerAddress* address)
{
    if (this->handle_04 != NULL) {
        return -1;
    }
    this->handle_04 = networkSocketPool_acquire(getNetworkLogger());
    if (this->handle_04 == NULL) {
        return -2;
    }
    if (this->handle_04->open(1) < 0) {
        this->release();
        return -3;
    }
    if (this->handle_04->setPeer(address) < 0) {
        this->release();
        return -4;
    }
    memcpy(&this->address_08, address, 6);
    return 0;
}

#pragma dont_inline off

/* Closes the peer's socket through the socket's own vtable; -1 when there is no socket. */
s32 NetworkSingleTcp::close()
{
    if (this->handle_04 == NULL) {
        return -1;
    }
    return this->handle_04->closeSocket();
}

/* Clears the peer's socket receive buffer through the socket's own vtable; 0 when there is none. */
s32 NetworkSingleTcp::clearReceive()
{
    if (this->handle_04 == NULL) {
        return 0;
    }
    return this->handle_04->clearReceive();
}

#pragma dont_inline on

/* Releases the peer's socket: shuts the socket down, hands it back to the pool the peer band
   registers with, and clears the peer's own slot. */
void NetworkSingleTcp::release()
{
    if (this->handle_04 != NULL) {
        this->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), this->handle_04);
        this->handle_04 = NULL;
    }
}

#pragma dont_inline off

/* Registers a peer in the connection's first free slot of four; the slot, or -1 (logged) when full. */
s32 NetworkSingleTcp::add(NetworkPeerMcs* peer)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (this->peers_10[index] == NULL) {
            this->peers_10[index] = peer;
            getNetworkLogger()->signal_0C(3, "NetworkSingleTcp::add: %d\n", index);
            return index;
        }
    }
    getNetworkLogger()->warn_10("NetworkSingleTcp::add: cannot add peer.\n");
    return -1;
}

/* Unregisters a peer from the connection (logged when it is not registered). */
void NetworkSingleTcp::remove(NetworkPeerMcs* peer)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (this->peers_10[index] == peer) {
            this->peers_10[index] = NULL;
            getNetworkLogger()->signal_0C(3, "NetworkSingleTcp::remove: %d\n", index);
            return;
        }
    }
    getNetworkLogger()->warn_10("NetworkSingleTcp::remove: cannot remove peer.\n");
}

/* Empties the peer's own 0x2400-byte receive area. */
void NetworkSingleTcp::clearReceiveBuffer()
{
    memset(this->recv_20, 0, 0x2400);
    this->recvUsed_2420 = 0;
}

/* Sends bytes on the connection's socket; -1 (logged) when it has none. */
s32 NetworkSingleTcp::send(const u8* data, s32 size)
{
    if (this->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkSingleTcp::send: socket is NULL\n");
        return -1;
    }
    return this->handle_04->send(data, size, NULL);
}

/* Asks the connection's socket how many bytes are readable; 0 when it holds no socket. */
s32 NetworkSingleTcp::getAvailableToRead()
{
    if (this->handle_04 != NULL) {
        return getBytesAvailableToRead(this->handle_04);
    }
    return 0;
}

/* The socket's last error code; 0 when the connection has no socket. */
s32 NetworkSingleTcp::getError()
{
    if (this->handle_04 != NULL) {
        return networkSocketHandle_getLastError(this->handle_04);
    }
    return 0;
}

#pragma dont_inline on

/* Disconnects the Udp socket by releasing it. */
void NetworkMultipleUdp::disconnect()
{
    this->release();
}

#pragma dont_inline off

/* Reads datagrams off the shared socket and queues each one behind the peer whose address sent it
   (logged when the peer's 0x1770-byte queue would overflow). */
void NetworkMultipleUdp::move()
{
    NetworkPeerAddress sender;
    s32 received;
    u16 length;
    s32 index;

    if (this->handle_04 != NULL) {
        while (1) {
            received = this->handle_04->receive(this->datagram_26, 0x5DC, &sender);
            if (received < 1) {
                break;
            }
            length = received;
            for (index = 0; index < 4; index++) {
                if (memcmp(&this->addresses_0E[index], &sender, 6) == 0) {
                    if ((s32)(length + this->used_63C4[index]) > 0x1770) {
                        getNetworkLogger()->log_14("NetworkMultipleUdp::move: buf_recv_peer over. please check NetworkMultipleUdp::MAX_SIZE_BUF_PEER\n");
                    } else {
                        memcpy(this->received_602[index] + this->used_63C4[index], this->datagram_26, length);
                        this->used_63C4[index] += length;
                    }
                    break;
                }
            }
        }
    }
}

#pragma dont_inline on

/* Releases the Udp socket: shuts it down, hands it back to the pool and clears the slot. */
void NetworkMultipleUdp::release()
{
    if (this->handle_04 != NULL) {
        this->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), this->handle_04);
        this->handle_04 = NULL;
    }
}

#pragma dont_inline off

/* Forgets the peer that owns an address: its address and its queued bytes (logged). */
void NetworkMultipleUdp::remove(const NetworkPeerAddress* address)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (memcmp(&this->addresses_0E[index], address, 6) == 0) {
            memset(&this->addresses_0E[index], 0, 6);
            this->used_63C4[index] = 0;
            getNetworkLogger()->signal_0C(3, "NetworkMultipleUdp::remove: %d.%d.%d.%d:%d\n",
                                          address->ip_00[0], address->ip_00[1], address->ip_00[2], address->ip_00[3],
                                          address->port_04);
            return;
        }
    }
}

/* Drops the bytes queued for one peer slot (logged when the slot or the socket is invalid). */
void NetworkMultipleUdp::reset(s32 peerIndex)
{
    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::reset: peer_id is invalid -> %d\n", peerIndex);
        return;
    }
    if (this->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::reset: socket is NULL\n");
        return;
    }
    this->used_63C4[peerIndex] = 0;
}

/* Sends bytes to one peer slot's address; -1 (logged) when the slot or the socket is invalid. */
s32 NetworkMultipleUdp::send(s32 peerIndex, const u8* data, s32 size)
{
    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::send: peer_id is invalid -> %d\n", peerIndex);
        return -1;
    }
    if (this->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::send: socket is NULL\n");
        return -1;
    }
    return this->handle_04->send(data, size, &this->addresses_0E[peerIndex]);
}

/* Takes the two length-prefixed payloads a peer slot has queued into the caller's buffer; 0 when they
   are incomplete or do not fit, -1 (logged) on an invalid slot or socket, else the bytes taken. */
s32 NetworkMultipleUdp::receive(s32 peerIndex, u8* out, s32 capacity)
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
    if (this->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: socket is NULL\n");
        return -1;
    }
    used = &this->used_63C4[peerIndex];
    if (*used < 2) {
        return 0;
    }
    queue = this->received_602[peerIndex];
    memcpy(&length, queue, 2);
    length = getNetworkLogger()->flag_48(length);
    if (length == 0 || length > 0x400) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: invalid packet %d from %d.%d.%d.%d:%d\n", length,
                                    this->addresses_0E[peerIndex].ip_00[0], this->addresses_0E[peerIndex].ip_00[1],
                                    this->addresses_0E[peerIndex].ip_00[2], this->addresses_0E[peerIndex].ip_00[3],
                                    this->addresses_0E[peerIndex].port_04);
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
    total = taken + 2;
    if (*used < total + length2) {
        return 0;
    }
    total += length2;
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

/* Asks the Udp socket how many bytes are readable; 0 when it holds no socket. */
s32 NetworkMultipleUdp::getAvailableToRead()
{
    if (this->handle_04 != NULL) {
        return getBytesAvailableToRead(this->handle_04);
    }
    return 0;
}

/* The socket's last error code; 0 when the Udp socket has none. */
s32 NetworkMultipleUdp::getError()
{
    if (this->handle_04 != NULL) {
        return networkSocketHandle_getLastError(this->handle_04);
    }
    return 0;
}

/* Returns the stream's byte block. */
u8* NetworkByteStream::getData()
{
    return this->data_04;
}

/* Returns how many bytes the stream holds. */
u32 NetworkByteStream::getSize()
{
    return this->cursor_0C;
}

#pragma dont_inline on

/* Fills the stream's leading 0xE-byte record from a sink and advances the cursor over it. */
void NetworkByteStream::pullRecord(NetworkStreamSink* sink)
{
    if (this->cursor_0C + 0xE <= this->size_08) {
        if (sink->fill(&this->data_04[this->cursor_0C], 0xE) > 0) {
            this->cursor_0C += 0xE;
        }
    }
}

/* Writes the record's u16 length prefix and then its bytes, when both fit. */
void NetworkByteStream::putRecord(const NetworkPeerRecord* record)
{
    if (this->cursor_0C + record->size_04 + 2 <= this->size_08) {
        this->putU16(record->size_04);
        if (record->data_00 != NULL && record->size_04 != 0) {
            memcpy(&this->data_04[this->cursor_0C], record->data_00, record->size_04);
        }
        this->cursor_0C += record->size_04;
    }
}

/* Writes one 4-byte value, encoded through the log manager's own value slot. */
void NetworkByteStream::putU32(u32 value)
{
    u32 encoded;

    if (this->cursor_0C + 4 > this->size_08) {
        return;
    }
    encoded = getNetworkLogger()->encode_54(value);
    memcpy(&this->data_04[this->cursor_0C], &encoded, 4);
    this->cursor_0C += 4;
}

/* Writes one 2-byte value, encoded through the log manager's own value slot. */
void NetworkByteStream::putU16(u16 value)
{
    u16 encoded;

    if (this->cursor_0C + 2 > this->size_08) {
        return;
    }
    encoded = getNetworkLogger()->encode_4C(value);
    memcpy(&this->data_04[this->cursor_0C], &encoded, 2);
    this->cursor_0C += 2;
}

#pragma dont_inline off

/* Appends one byte when the stream's cursor has room for it. */
void NetworkByteStream::putByte(u8 value)
{
    if (this->cursor_0C + 1 <= this->size_08) {
        this->data_04[this->cursor_0C] = value;
        this->cursor_0C++;
    }
}

/* Hands the stream's leading 0xE-byte record to a sink and drops it from the front. */
void NetworkByteStream::forwardRecord(NetworkStreamSink* sink)
{
    u32 remaining;

    if (this->cursor_0C < 0xE) {
        return;
    }
    if (sink->put(this->data_04, 0xE) > 0) {
        remaining = this->cursor_0C - 0xE;
        this->cursor_0C = remaining;
        if (remaining != 0) {
            memmove(this->data_04, this->data_04 + 0xE, remaining);
        }
    }
}

/* Copies the stream's leading length-prefixed record into the caller's record when both the stream
   and the caller have room, then drops it from the front. */
void NetworkByteStream::takeRecord(NetworkPeerRecord* record)
{
    u16 length;
    u16 size;

    if (this->cursor_0C < 2) {
        return;
    }
    this->readLength(&length);
    size = length;
    if (size <= this->cursor_0C && size <= record->size_04) {
        record->size_04 = size;
        if (size != 0) {
            if (record->data_00 != NULL) {
                memcpy(record->data_00, this->data_04, size);
            }
            this->cursor_0C -= record->size_04;
            if (this->cursor_0C != 0) {
                memmove(this->data_04, this->data_04 + record->size_04, this->cursor_0C);
            }
        }
    } else {
        record->size_04 = 0;
    }
}

#pragma dont_inline on

/* Takes the stream's leading 4-byte value, decoded through the log manager's own value slot, and
   drops it from the front. */
void NetworkByteStream::takeU32(u32* out)
{
    u32 value;
    u32 remaining;

    if (this->cursor_0C < 4) {
        return;
    }
    memcpy(&value, this->data_04, 4);
    *out = getNetworkLogger()->decode_50(value);
    remaining = this->cursor_0C - 4;
    this->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(this->data_04, this->data_04 + 4, remaining);
    }
}

/* Records the stream's leading length prefix into the caller's u16, decoded through the log
   manager's own value slot, then drops the two bytes from the front. */
void NetworkByteStream::readLength(u16* out)
{
    u16 encoded;
    u32 remaining;

    if (this->cursor_0C < 2) {
        return;
    }
    memcpy(&encoded, this->data_04, 2);
    *out = getNetworkLogger()->flag_48(encoded);
    remaining = this->cursor_0C - 2;
    this->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(this->data_04, this->data_04 + 2, remaining);
    }
}

#pragma dont_inline off

/* Takes the stream's leading byte into the caller's byte and drops it from the front. */
void NetworkByteStream::takeByte(u8* out)
{
    u32 remaining;
    u8* base;

    if (this->cursor_0C < 1) {
        return;
    }
    base = this->data_04;
    *out = base[0];
    remaining = this->cursor_0C - 1;
    this->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(this->data_04, this->data_04 + 1, remaining);
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
