/*
 * Network/NetworkPeerMcs.cpp - the Mcs peer: constructor, the table slots, the connect machine and the shared Mcs
 *   scratch packet and retry time.  Its log string names the class (`NetworkPeerMcs::put`).
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CD764..0x803CE060, `.data` 0x805F9570..0x805F9610, extab 0x80019968..0x800199B0, extabindex
 * 0x8003A170..0x8003A1DC, `.bss` 0x806D3240..0x806D3650, `.sbss` 0x80794C98..0x80794CA0.
 *
 * NAMES.  The slot names other than the log-named `put` are GUESSes.  Every name here is the map's or a derived
 * one; the derived ones are marked GUESS in `Network/network_transport_types.h`.
 *
 * TABLE.  Its table (0x805F95E0, 0x30 B) is emitted from `NetworkPeerMcs::destroy`, the key function (rule 10).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  `send`/`receive`/`put`/`move` 99.7-99.8 % (the same `NETWORK_ERROR_*` immediates, playbook 58's
 * class); `.text` differs from the target by 36 B, all relocations; `.sbss` holds the 4-byte `networkMcsRetryTime`
 * against the claimed 8 B.
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

/* The scratch packet the Mcs peer frames its traffic in, and the time it last tried to connect. */
u8 networkMcsPacketBuffer[0x410];
f32 networkMcsRetryTime;

#pragma dont_inline on

/* Deleting destructor: chains the base destructor, then frees on request. */
NetworkPeerBase* NetworkPeerMcs::destroy(s16 flags)
{
    if (this != NULL) {
        NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Builds the Mcs peer: the error base, the peer's table, its flags, the shared Mcs scratch packet and
   its own work area emptied, and no connection yet. */
NetworkPeerMcs::NetworkPeerMcs(u8 armed)
{
    this->armed_10 = armed;
    this->state_14 = 0;
    this->dropped_18 = 0;
    memset(networkMcsPacketBuffer, 0, 0x410);
    memset(this->work_19, 0, 0x2400);
    this->workUsed_241C = 0;
    memset(&this->peerAddress_2420, 0, 6);
    this->connection_2428 = NULL;
}

/* Publishes the peer's record: the six-byte address, the connection it registers with, and up to 0x40
   bytes of label with its used length. */
/* untyped: caller-owned payload - the record the Mcs peer publishes itself from */
void NetworkPeerMcs::setContext(const void* context)
{
    const NetworkPeerInfo* info = (const NetworkPeerInfo*)context;
    s32 available;
    s32 labelSize;

    memcpy(&this->peerAddress_2420, &info->address_00, 6);
    this->connection_2428 = info->connection_08;
    memset(this->label_242C, 0, 0x40);
    available = info->labelSize_4C;
    if (available < 0x40) {
        labelSize = available;
    } else {
        labelSize = 0x40;
    }
    this->labelSize_246C = labelSize;
    memcpy(this->label_242C, info->label_0C, labelSize);
}

/* Frames the payload behind a length prefix, sends it on the connection (a listening peer does not
   need the socket to be idle first) and keeps a copy in the work area; the framed byte count, or -1. */
s32 NetworkPeerMcs::send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind)
{
    u16 prefix;
    s32 error;

    if (this->connection_2428 == NULL) {
        return 0;
    }
    if (data == NULL) {
        return 0;
    }
    if (size <= 0) {
        return 0;
    }
    if ((u32)(size + 2) > 0x410) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0x410, 0x80000000);
        return -1;
    }
    if ((u32)(size + this->workUsed_241C + 2) > 0x2400) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0x2400, 0x80000000);
        return -1;
    }
    prefix = getNetworkLogger()->encode_4C((u16)size);
    memcpy(networkMcsPacketBuffer, &prefix, 2);
    memcpy(networkMcsPacketBuffer + 2, data, size);
    if ((this->armed_10 == 0 || this->connection_2428->clearReceive() != 0)
        && this->connection_2428->send(networkMcsPacketBuffer, size + 2) < 0) {
        error = this->connection_2428->getError();
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, this->connection_2428->getAvailableToRead(), error);
        return -1;
    }
    memcpy(this->work_19 + this->workUsed_241C, networkMcsPacketBuffer, size + 2);
    this->workUsed_241C += size + 2;
    return size + 2;
}

/* Takes the leading length-prefixed packet out of the work area into the caller's buffer; 0 when none
   is complete or it does not fit, -1 on failure, else the bytes the packet took. */
s32 NetworkPeerMcs::receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind)
{
    u16 length;
    s32 error;
    s32 taken;
    u8* work;
    s32 capacity;

    if (this->connection_2428 == NULL) {
        return 0;
    }
    if (this->armed_10 == 0 && this->connection_2428->getAvailableToRead() != 0) {
        error = this->connection_2428->getError();
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, this->connection_2428->getAvailableToRead(), error);
        return -1;
    }
    capacity = *size;
    *size = 0;
    if (size2 != NULL) {
        *size2 = 0;
    }
    if (kind != NULL) {
        *kind = 0;
    }
    if ((s32)this->workUsed_241C < 2) {
        return 0;
    }
    work = this->work_19;
    memcpy(&length, work, 2);
    length = getNetworkLogger()->flag_48(length);
    if (length == 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, 0, 0x80000000);
        return -1;
    }
    if ((s32)this->workUsed_241C < (s32)(length + 2)) {
        return 0;
    }
    if (capacity < length) {
        return 0;
    }
    memcpy(out, work + 2, length);
    *size = length;
    taken = length + 2;
    this->workUsed_241C -= taken;
    if ((s32)this->workUsed_241C > 0) {
        memmove(work, this->work_19 + taken, this->workUsed_241C);
    }
    return length + 2;
}

/* Appends received bytes to the work area; -1 (logged, record filled) when its 0x2400 bytes would
   overflow. */
s32 NetworkPeerMcs::put(const u8* data, s32 length, u32 a, u32 b, u32 c)
{
    u32 used;

    if (length <= 0) {
        return 0;
    }
    used = this->workUsed_241C;
    if (used + length > 0x2400) {
        getNetworkLogger()->log_14("NetworkPeerMcs::put: mPacketRecvBuf over. please check sizeof(NetworkPeerMcs::mPacketRecvBuf)=0x%x<(0x%x+0x%x)\n",
                                   0x2400, used, length);
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PUT_OVERFLOW, 0x2400, 0x80000000);
        return -1;
    }
    memcpy(this->work_19 + used, data, length);
    this->workUsed_241C += length;
    return length;
}

/* Walks the peer's connect machine one step: a listening peer re-registers with its connection once
   the retry interval has passed; a connecting peer opens the socket (state 0), waits for it to finish
   and publishes its label (state 1).  1 when it made progress, 0 when idle, -1 on failure. */
s32 NetworkPeerMcs::move()
{
    s32 result;
    s32 error;

    if (this->armed_10 == 0) {
        if (this->connection_2428 == NULL) {
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, 0, 0x80000000);
            return -1;
        }
        if (this->connection_2428->getAvailableToRead() != 0) {
            error = this->connection_2428->getError();
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, this->connection_2428->getAvailableToRead(), error);
            return -1;
        }
        if (this->connection_2428->clearReceive() == 0) {
            return 0;
        }
        if (getNetworkLogger()->getTime_60() < networkMcsRetryInterval + networkMcsRetryTime) {
            return 0;
        }
        networkMcsRetryTime = getNetworkLogger()->getTime_60();
        memset(this->work_19, 0, 0x2400);
        this->workUsed_241C = 0;
        this->connection_2428->add(this);
        return 1;
    }
    if (this->connection_2428 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, 0, 0x80000000);
        this->state_14 = 0;
        return -1;
    }
    if (this->dropped_18 != 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_STATE, 0, 0x80000000);
        this->state_14 = 0;
        return -1;
    }
    switch (this->state_14) {
    case 0:
        this->connection_2428->clearReceiveBuffer();
        if (this->connection_2428->open(&this->peerAddress_2420) < 0) {
            error = this->connection_2428->getError();
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, this->connection_2428->getAvailableToRead(), error);
            this->state_14 = 0;
            return -1;
        }
        memset(this->work_19, 0, 0x2400);
        this->workUsed_241C = 0;
        this->state_14++;
        break;
    case 1:
        result = this->connection_2428->close();
        if (result < 0) {
            error = this->connection_2428->getError();
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, this->connection_2428->getAvailableToRead(), error);
            this->state_14 = 0;
            return -1;
        }
        if (result > 0) {
            networkMcsRetryTime = getNetworkLogger()->getTime_60();
            this->connection_2428->send((u8*)this->label_242C, this->labelSize_246C);
            this->state_14 = 0;
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

#pragma dont_inline off

/* Raises the deferred-drop flag of a peer that has something to drop. */
void NetworkPeerMcs::armDrop()
{
    if (this->armed_10 != 0) {
        this->dropped_18 = 1;
    }
}

#pragma dont_inline on

/* The peer class's init slot: close whatever the peer was holding and report it usable. */
s32 NetworkPeerMcs::init()
{
    this->reset();
    return 1;
}

/* Closes the peer: drops it from its connection's registry and, when it was armed, releases the
   socket and empties both work areas. */
void NetworkPeerMcs::reset()
{
    if (this->connection_2428 != NULL) {
        this->connection_2428->remove(this);
        if (this->armed_10 != 0) {
            this->connection_2428->release();
            this->connection_2428->clearReceiveBuffer();
        }
    }
    memset(this->work_19, 0, 0x2400);
    this->workUsed_241C = 0;
}

#pragma dont_inline off

}
