/*
 * Network/NetworkConnectionStable.cpp - `NetworkConnectionStable`, the connection a `NetworkSessionStable` slot owns
 *   (its log strings: `NetworkConnectionStable[%d] ...`), the slot queues it embeds (`NetworkSlotQueues`), the send pool
 *   and the frame writer's entry points.
 * RANGE. .text 0x803CA49C-0x803CCDF8 (50 functions), extab 0x80019618-0x800198C8, extabindex 0x80039EF4-0x8003A0BC,
 *   .data 0x805F91F0-0x805F94E0 (the log strings and the class's table 0x805F9490).  Left edge: the V->S seam at
 *   0x805F91F0 and the constructor's extabindex record (`Network/NetworkConnection.cpp`).  Right edge:
 *   `Network/NetworkPeerBase.cpp` at 0x803CCDF8.
 * FLAGS. the library's flags plus `#pragma peephole off` (kept `clrlwi`/`extsb` forms) and `#pragma fp_contract off`
 *   (`updateRoundTrip` keeps `fmuls`+`fadds`).
 * NAMES. The file and class names are the log strings'.  The method names are GUESSes read off what each one does:
 *   GUESS: open, reset, clear, update, start, begin, end, stop, clearPending, sendChannel, clearSendPending,
 *   GUESS: getSendRoom, networkStreamWriter_attach, networkStreamWriter_reserve, receivePackets, getIndex,
 *   GUESS: getCongestion, setRelay, flush, setInterval, setTimeout, setLimit, getRoundTripTime, setValue, getValue,
 *   GUESS: getMaxPacketSize, setState, getState, setError, checkChannel, sendHello, sendHelloDone, sendPing, sendPong,
 *   GUESS: sendClose, sendMtuProbe, stepMtu, sendMtuReply, dispatchControl, updateRoundTrip, updateRate.
 *   The runtime dump's names at 0x803CBA2C, 0x803CBF34, 0x803CBF3C and 0x803CBFFC..
 *   0x803CC00C (`GoalOverlay::SceneCreated`, `J3DColorBlockLightOff::setColorChanNum`, `nw4hbm::ut::TextWriterBase`'s
 *   char-space accessors) are folded one-store bodies, not evidence.
 * RESIDUALS. The constructor calls `NetworkConnection(s32)`, which is unwritten in `Network/NetworkConnection.cpp`.
 *   `sendChannel`: the frame's sequence copy is kept 32-bit where retail keeps it (`mr`, the `u16` return unextended).
 *   `open`: `rand() + base` adds in the other operand
 *   order (`(s32)`/`(u16)` spellings tried).  `sendClose`: the 0x84/0x85
 *   select computes in r4 directly (a `u8` local tried).
 *   `.sdata2` (flip blocker): our object emits the `u32`->`f32` conversion double (8 B) that retail pools at
 *   0x8079C6B8 (`dispatchControl`'s pong time); the range is not this unit's to claim.
 * SHAPES. The destructor, the `NetworkSlotQueues` constructor and destructor and the four writer constructors and
 *   destructors are complete with empty bodies (the compiler emits the member and base calls and the table stores);
 *   all of them match.  The writers' tables are `Network/NetworkUnitPacket.cpp`'s: each class declares its flush
 *   override first, so that override, not the destructor defined here, is the key function.
 */

#include "Network/NetworkConnectionStable.h"
#include "Network/NetworkSessionStable.h"   /* NetworkConnectionStable, NetworkSlotQueues */
#include "Network/NetworkUnitPacket.h"      /* the packet, writer, reader and queue entry points */
#include "Network/network_shared_data.h"    /* the connection's pooled constants */
#include "MSL_C/alloc.h"                    /* rand */
#include "unsplit/Network.h"                /* NetworkLogger */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off

/* Retail keeps `fmuls`+`fadds` where `-fp_contract on` would fuse a `fmadds` (`updateRoundTrip`). */
#pragma fp_contract off

/* Builds the connection on the base's peer of `kind`, with its queues and the idle state. */
NetworkConnectionStable::NetworkConnectionStable(s32 kind) : NetworkConnection(kind)
{
    state_21B0 = 0;
}

/* ==== the slot queues ========================================================================================= */

/* Destroys the two receive queues and the two send queues. */
NetworkSlotQueues::~NetworkSlotQueues()
{
}

/* Builds the two send queues and the two receive queues. */
NetworkSlotQueues::NetworkSlotQueues()
{
}

/* ==== the connection ========================================================================================== */

/* Destroys the queues and the base. */
NetworkConnectionStable::~NetworkConnectionStable()
{
}

/* Binds the queues to their blocks with random send sequence numbers, copies this side's address record, starts the
 * clocks from the defaults and clears the rest. */
void NetworkConnectionStable::open(NetworkConnectionCallback callback, NetworkSessionStable* owner, s8 index,
                                   const NetworkConnectionOpenInfo* info)
{
    u32 base;

    (&queues_1024.send_00[0])->attach(sendBlocks_10A4[0], 0x400);
    (&queues_1024.send_00[1])->attach(sendBlocks_10A4[1], 0x400);
    base = (u32)(networkConnectionMilliseconds * getNetworkLogger()->getTime_60());
    networkStreamQueue_setSequence(&queues_1024.send_00[0], base + rand());
    base = (u32)(networkConnectionMilliseconds * getNetworkLogger()->getTime_60());
    networkStreamQueue_setSequence(&queues_1024.send_00[1], base + rand());
    (&queues_1024.receive_30[0])->attach(receiveBlocks_18A4[0], 0x400);
    (&queues_1024.receive_30[1])->attach(receiveBlocks_18A4[1], 0x400);
    queues_1024.receiveStarted_60 = 0;
    queues_1024.expiry_64 = networkConnectionZero;
    index_20A4 = index;
    addressSize_2128 = (s32)info->size_04 < 0x80 ? (s32)info->size_04 : 0x80;
    memcpy(address_20A5, info->data_00, addressSize_2128);
    now_21B4 = getNetworkLogger()->getTime_60();
    openTime_21B8 = getNetworkLogger()->getTime_60();
    pollInterval_21C0 = networkConnectionDefaultInterval;
    connectTimeout_21C8 = networkConnectionDefaultTimeout;
    pingsLost_21F8 = 0;
    pingsAnswered_21FC = 0;
    lastPing_2200 = networkConnectionZero;
    lastStat_2204 = networkConnectionZero;
    totalBytesSent_222C = 0;
    totalBytesReceived_2230 = 0;
    lastReceive_2234 = networkConnectionZero;
    receiveTimeout_2238 = networkConnectionDefaultLimit;
    receivedThisFrame_223C = 0;
    callback_04 = callback;
    owner_08 = owner;
    clear();
}

/* Resets the peer and returns to the idle state. */
void NetworkConnectionStable::reset()
{
    if (peer_0C != NULL) {
        peer_0C->reset();
        setState(0);
    }
}

/* Clears every buffer, queue, clock and counter. */
void NetworkConnectionStable::clear()
{
    memset(&error_0010, 0, sizeof(error_0010));
    memset(frame_001C, 0, sizeof(frame_001C));
    memset(receive_041C, 0, sizeof(receive_041C));
    memset(record_081C, 0, sizeof(record_081C));
    memset(raw_0C1C, 0, sizeof(raw_0C1C));
    rawSize_101C = 0;
    sendPending_1020 = 0;
    networkStreamQueue_clear(&queues_1024.send_00[0]);
    networkStreamQueue_clear(&queues_1024.send_00[1]);
    networkStreamQueue_clear(&queues_1024.receive_30[0]);
    networkStreamQueue_clear(&queues_1024.receive_30[1]);
    queues_1024.resend_68 = 0;
    memset(queues_1024.lastSend_6C, 0, sizeof(queues_1024.lastSend_6C));
    queues_1024.rate_74 = networkConnectionTwo * networkSessionDefaultDelay;
    queues_1024.rateTarget_78 = networkConnectionZero;
    queues_1024.byteLimit_7C = 0x400;
    peerAddressSize_21AC = 0;
    memset(peerAddress_212C, 0, sizeof(peerAddress_212C));
    setState(0);
    lastPoll_21BC = networkConnectionZero;
    connectStart_21C4 = networkConnectionZero;
    lastHello_21CC = networkConnectionZero;
    pingIndex_21D0 = 0;
    pingTimes_21D4[0] = networkSessionDefaultDelay;
    pingTimes_21D4[1] = networkSessionDefaultDelay;
    pingTimes_21D4[2] = networkSessionDefaultDelay;
    pingTimes_21D4[3] = networkSessionDefaultDelay;
    pingTimes_21D4[4] = networkSessionDefaultDelay;
    pingTimes_21D4[5] = networkSessionDefaultDelay;
    pingTimes_21D4[6] = networkSessionDefaultDelay;
    pingTimes_21D4[7] = networkSessionDefaultDelay;
    roundTrip_21F4 = networkSessionDefaultDelay;
    bytesSent_2208 = 0;
    bytesReceived_220C = 0;
    lastBytesSent_2210 = 0;
    lastBytesReceived_2214 = 0;
    minuteBytesSent_2218 = 0;
    minuteBytesReceived_221C = 0;
    packetsSent_2220 = 0;
    lastPacketsSent_2224 = 0;
    minutePacketsSent_2228 = 0;
    receiveTimeoutOn_223D = 0;
    mtuProbe_223E = 0;
    mtuReply_223F = 0;
    mtuSeen_2240 = 0x200;
    mtu_2244 = 0x400;
    mtuFixed_2248 = 0;
    mtuTries_224C = 8;
}

/* One frame of the machine: connect (1), handshake (2, 3), linked (4) with its pings, statistics and receive
 * timeout, and the close (5, 6); every live state pumps both channels, the peer and the control messages. */
void NetworkConnectionStable::update()
{
    f32 elapsed;
    s32 result;
    NetworkPeerErrorRecord record;

    elapsed = getNetworkLogger()->getTime_60() - now_21B4;
    now_21B4 = getNetworkLogger()->getTime_60();
    switch (getState()) {
    case 1:
        if (peer_0C == NULL) {
            setError(0x80030001, 0xFF, 0x80000000);
            callback_04(2, index_20A4, error_0010.code_00, 1, (const u8*)&error_0010, owner_08);
            clear();
            break;
        }
        if (lastPoll_21BC + pollInterval_21C0 < now_21B4) {
            lastPoll_21BC = now_21B4;
            peer_0C->armDrop();
        }
        result = peer_0C->move();
        if (result < 0) {
            networkPeerError_get(peer_0C, &record);
            if (record.source != NULL) {
                setError((s32)record.source, record.argument, record.code);
                networkPeerError_clear(peer_0C);
            } else {
                setError(0x80030011, 0, result);
            }
            callback_04(2, index_20A4, error_0010.code_00, 1, (const u8*)&error_0010, owner_08);
            clear();
            break;
        }
        if (result > 0) {
            setState(2);
            connectStart_21C4 = now_21B4;
            lastHello_21CC = networkConnectionZero;
        }
        break;
    case 2:
    case 3:
        if (peer_0C == NULL) {
            setError(0x80030001, 0xFF, 0x80000000);
        } else {
            networkPeerError_get(peer_0C, &record);
            if (record.source != NULL) {
                setError((s32)record.source, record.argument, record.code);
                networkPeerError_clear(peer_0C);
            }
        }
        if (error_0010.code_00 != 0) {
            callback_04(2, index_20A4, error_0010.code_00, 1, (const u8*)&error_0010, owner_08);
            clear();
            break;
        }
        if (connectStart_21C4 + connectTimeout_21C8 < now_21B4) {
            getNetworkLogger()->log_14("NetworkConnectionStable[%d] connect auth timeout (%dsec)\n", index_20A4,
                                       (s32)connectTimeout_21C8);
            setError(0x80030033, (s32)connectTimeout_21C8, 0x80000000);
            break;
        }
        if (networkConnectionPingInterval + lastHello_21CC <= now_21B4) {
            lastHello_21CC = now_21B4;
            sendHello(0);
        }
        sendChannel(0);
        sendChannel(1);
        receivePackets();
        dispatchControl();
        break;
    case 4:
        if (peer_0C == NULL) {
            setError(0x80030001, 0xFF, 0x80000000);
        } else {
            networkPeerError_get(peer_0C, &record);
            if (record.source != NULL) {
                setError((s32)record.source, record.argument, record.code);
                networkPeerError_clear(peer_0C);
            }
        }
        if (error_0010.code_00 != 0) {
            callback_04(2, index_20A4, error_0010.code_00, 1, (const u8*)&error_0010, owner_08);
            clear();
            break;
        }
        if (networkConnectionPingInterval + lastPing_2200 <= now_21B4) {
            lastPing_2200 = now_21B4;
            updateRoundTrip(pingIndex_21D0);
            sendPing(pingIndex_21D0);
            pingIndex_21D0 = (pingIndex_21D0 + 1) % 8;
            stepMtu();
            lastBytesSent_2210 = bytesSent_2208;
            lastBytesReceived_2214 = bytesReceived_220C;
            minuteBytesSent_2218 += bytesSent_2208;
            minuteBytesReceived_221C += bytesReceived_220C;
            bytesSent_2208 = 0;
            bytesReceived_220C = 0;
            lastPacketsSent_2224 = packetsSent_2220;
            minutePacketsSent_2228 += packetsSent_2220;
            packetsSent_2220 = 0;
        }
        if (networkConnectionStatInterval + lastStat_2204 <= now_21B4) {
            lastStat_2204 = now_21B4;
            getNetworkLogger()->signal_0C(1, "NetworkConnectionStable[%d] 1min send %d/%d recv %d\n", index_20A4,
                                          minuteBytesSent_2218, minutePacketsSent_2228, minuteBytesReceived_221C);
            minuteBytesSent_2218 = 0;
            minuteBytesReceived_221C = 0;
            minutePacketsSent_2228 = 0;
        }
        if (receiveTimeoutOn_223D != 0) {
            if (receivedThisFrame_223C != 0) {
                receivedThisFrame_223C = 0;
                lastReceive_2234 += elapsed;
            }
            if (lastReceive_2234 + receiveTimeout_2238 <= now_21B4) {
                getNetworkLogger()->log_14("NetworkConnectionStable[%d] norecv timeout (%dsec)\n", index_20A4,
                                           (s32)receiveTimeout_2238);
                setError(0x80030035, (s32)receiveTimeout_2238, 0);
                break;
            }
        }
        sendChannel(0);
        sendChannel(1);
        receivePackets();
        if (mtuReply_223F != 0) {
            mtuReply_223F = 0;
            sendMtuReply(mtuSeen_2240);
            getNetworkLogger()->signal_0C(3, "NetworkConnectionStable[%d] mtu check packet recv -> %d\n", index_20A4,
                                          mtuSeen_2240);
        }
        dispatchControl();
        break;
    case 5:
        if (peer_0C != NULL && peer_0C->init() == 0) {
            break;
        }
        setState(6);
    case 6:
        callback_04(6, index_20A4, 0, 0, NULL, owner_08);
        clear();
        getNetworkLogger()->signal_0C(3, "NetworkConnectionStable[%d] shutdown end\n", index_20A4);
        break;
    }
}

/* Binds the peer to its record; `b` == 0 keeps the MTU probing on. */
void NetworkConnectionStable::start(u32 a, u32 b)
{
    if (peer_0C != NULL) {
        peer_0C->setContext((const void*)a);
        mtuFixed_2248 = (b == 0);
    }
}

/* Starts the connect from the idle state with the owner's round-trip estimate. */
void NetworkConnectionStable::begin(f32 roundTrip)
{
    if (getState() == 0) {
        setState(1);
        roundTrip_21F4 = roundTrip;
        updateRate();
        lastPoll_21BC = now_21B4;
    }
}

/* Closes towards a relay: tells a connected peer, then resets the peer and clears. */
void NetworkConnectionStable::end()
{
    if (peer_0C != NULL) {
        getNetworkLogger()->signal_0C(3, "NetworkConnectionStable[%d] close-relay\n", index_20A4);
        if (getState() != 0 && getState() != 1) {
            getNetworkLogger()->signal_0C(3, "NetworkConnectionStable[%d] send control close-relay\n", index_20A4);
            sendClose(1);
            sendChannel(1);
        }
        peer_0C->reset();
        clear();
    }
}

/* Closes and leaves: tells a connected peer, then resets the peer and clears. */
void NetworkConnectionStable::stop()
{
    if (peer_0C != NULL) {
        getNetworkLogger()->signal_0C(3, "NetworkConnectionStable[%d] close-leave\n", index_20A4);
        if (getState() != 0 && getState() != 1) {
            getNetworkLogger()->signal_0C(3, "NetworkConnectionStable[%d] send control close-leave\n", index_20A4);
            sendClose(0);
            sendChannel(1);
        }
        peer_0C->reset();
        clear();
    }
}

/* Flushes both channels of a live connection and enters the shutdown (5), or the end (6) from idle. */
void NetworkConnectionStable::clearPending()
{
    if (getState() != 0 && getState() != 1) {
        sendChannel(0);
        sendChannel(1);
    }
    if (getState() != 0) {
        setState(5);
        return;
    }
    setState(6);
}

/* Queues `packet` on the send queue of `channel`, unstamped; an overflow is the connection's error. */
void NetworkConnectionStable::putOnSendPool(NetworkStreamWriter* packet, s32 channel)
{
    s32 result;

    if (checkChannel(channel) == 0) {
        networkPacket_setTimestamp(packet, networkConnectionZero);
        result = networkStreamQueue_append(&queues_1024.send_00[channel], packet, 0xFF);
        if (result < 0) {
            getNetworkLogger()->log_14("NetworkConnectionStable[%d] put send pool over (0x%x)\n", index_20A4, result);
            setError(0x80030032, result, 0);
        }
    }
}

/* Builds one frame of `channel` from the messages that are due (skipping the ones sent within the rate), stamps them
 * and hands the frame to the peer; channel 1 drops what it sent. */
void NetworkConnectionStable::sendChannel(s32 channel)
{
    NetworkStreamWriterDefault frame;
    NetworkStreamWriter packet;
    NetworkSlotQueues* queues;
    s32 sent;
    u32 sequence;
    u16 size;
    u16 overhead;
    s32 result;

    if (checkChannel(channel) != 0) {
        return;
    }
    if (channel == 1) {
        sendMtuProbe();
    }
    queues = &queues_1024;
    if (now_21B4 <= queues_1024.rateTarget_78 + queues_1024.lastSend_6C[channel]) {
        return;
    }
    networkStreamWriter_putBytes(&frame, frame_001C, getMaxPacketSize());
    networkStreamWriter_flush(&frame);
    networkStreamQueue_fillPacket(&queues->send_00[channel], &packet);
    sent = 0;
    sequence = networkStreamWriter_size(&queues->send_00[channel]);
    while (networkPacket_hasMessage(&packet) != 0) {
        if (networkPacket_getTimestamp(&packet) < networkConnectionZero
            && now_21B4 < networkConnectionNoTimestamp * networkPacket_getTimestamp(&packet)) {
            break;
        }
        if (queues->rate_74 + networkPacket_getTimestamp(&packet) <= now_21B4) {
            break;
        }
        sequence += networkPacket_getMessageSize(&packet, 0x80);
        networkPacket_nextMessage(&packet);
    }
    while (networkPacket_hasMessage(&packet) != 0) {
        if (networkPacket_getTimestamp(&packet) < networkConnectionZero
            && now_21B4 < networkConnectionNoTimestamp * networkPacket_getTimestamp(&packet)) {
            break;
        }
        size = networkPacket_getMessageSize(&packet, 0x80);
        if ((u16)networkPacket_getFrameOverhead() + size > 0x200) {
            size = networkPacket_getMessageSize(&packet, 0xFF);
            overhead = networkPacket_getFrameOverhead();
            getNetworkLogger()->warn_10("NetworkConnectionStable:send:[%d] unit size over -> %d\n", index_20A4,
                                        overhead + size);
        }
        result = networkStreamWriter_putPacket(&frame, &packet, 0x80);
        if (result < 0) {
            break;
        }
        sent += result;
        networkPacket_setTimestamp(&packet, now_21B4);
        networkPacket_nextMessage(&packet);
    }
    if (channel == 0 && queues->resend_68 != 0) {
        queues->resend_68 = 0;
    } else if (sent == 0) {
        return;
    }
    networkStreamWriter_setMode(&frame, sent);
    networkStreamWriter_putU16(&frame, sequence);
    networkStreamWriter_putU16b(&frame, networkStreamWriter_size(&queues->receive_30[channel]));
    networkStreamWriter_putU32(&frame, index_20A4);
    networkStreamWriter_putU32b(&frame, index_20A4);
    networkStreamWriter_enable1(&frame, 0);
    networkStreamWriter_enable2(&frame, (u8)channel);
    networkStreamWriter_commit(&frame);
    networkStreamWriter_bytes(&frame);
    result = peer_0C->send(frame_001C, getPosition(&frame), NULL, 0, 0);
    if (result < 0) {
        return;
    }
    bytesSent_2208 += result;
    totalBytesSent_222C += result;
    packetsSent_2220++;
    if (channel == 1 && sent > 0) {
        networkStreamQueue_discard(&queues->send_00[channel], sent, 0x80);
        networkStreamQueue_setSequence(&queues->send_00[channel], sequence + sent);
    }
    queues->lastSend_6C[channel] = now_21B4;
}

/* ==== the writer band's stream objects ======================================================================= */

/* Destroys the frame writer's sink. */
NetworkStreamWriterDefault::~NetworkStreamWriterDefault()
{
}

/* Destroys the packet's sink. */
NetworkStreamWriter::~NetworkStreamWriter()
{
}

/* Builds the packet on an empty sink. */
NetworkStreamWriter::NetworkStreamWriter()
{
}

/* Builds the frame writer on an empty sink. */
NetworkStreamWriterDefault::NetworkStreamWriterDefault()
{
}

/* ==== the collected frames =================================================================================== */

/* Clears the flag `networkStreamWriter_attach` raises when it flushed for room. */
void NetworkConnectionStable::clearSendPending()
{
    sendPending_1020 = 0;
}

/* The bytes the collected frames can still take: none after a flush for room, else the smaller of the block's and
 * the packet size's room. */
s32 NetworkConnectionStable::getSendRoom()
{
    s32 blockRoom;
    s32 room;

    if (sendPending_1020 != 0) {
        return 0;
    }
    blockRoom = 0x400 - rawSize_101C;
    room = getMaxPacketSize() - rawSize_101C;
    if (blockRoom < room) {
        room = blockRoom;
    }
    return room;
}

/* Appends the frame `stream` holds to the collected frames, flushing them first when it would not fit. */
void networkStreamWriter_attach(NetworkConnectionStable* connection, NetworkStreamWriterDefault* stream)
{
    s32 size;

    size = read_size_from_buffer(stream);
    if ((u32)(connection->rawSize_101C + size) > 0x400
        || connection->getMaxPacketSize() < (s32)(connection->rawSize_101C + size)) {
        networkStreamWriter_reserve(connection, NULL, 0, 0);
        connection->sendPending_1020 = 1;
    }
    if ((u32)(connection->rawSize_101C + size) > 0x400
        || connection->getMaxPacketSize() < (s32)(connection->rawSize_101C + size)) {
        connection->setError(0x80030032, 9, 0x80000000);
        return;
    }
    memcpy(connection->raw_0C1C + connection->rawSize_101C, networkStreamReader_getFrame(stream), size);
    connection->rawSize_101C += size;
}

/* Hands the collected frames, and `length` bytes of channel `kind`, to the peer and empties the collection. */
void networkStreamWriter_reserve(NetworkConnectionStable* connection, const u8* bytes, u32 length, s8 kind)
{
    s32 result;

    if (connection->rawSize_101C > 0 || (s32)length > 0) {
        result = connection->peer_0C->send(connection->raw_0C1C, connection->rawSize_101C, bytes, length, kind);
        connection->rawSize_101C = 0;
        if (result >= 0) {
            connection->bytesSent_2208 += result;
            connection->totalBytesSent_222C += result;
            connection->packetsSent_2220++;
        }
    }
}

/* Drains the peer: every whole frame is unscrambled, checked and filed by channel (or reported as an MTU probe or an
 * out-of-band record); a bad frame or a refused one is the connection's error. */
void NetworkConnectionStable::receivePackets()
{
    NetworkConnectionFrame frame;
    NetworkStreamWriterDefault reader;
    s32 received;
    s32 valid;
    s32 filed;
    u8 outOfBand;
    u8 channel;

    do {
        frame.size_00 = 0x400;
        frame.block_04 = receive_041C;
        frame.record_08.length_00 = 0x400;
        frame.record_08.bytes_04 = record_081C;
        received = peer_0C->receive(frame.block_04, (s32*)&frame.size_00, frame.record_08.bytes_04,
                                    (s32*)&frame.record_08.length_00, &frame.record_08.kind_08);
        if (received == 0) {
            break;
        }
        if (received < 0) {
            return;
        }
        bytesReceived_220C += received;
        totalBytesReceived_2230 += received;
        networkStreamReader_attach(&reader, frame.block_04, frame.size_00);
        while (make_sure_enough_space(&reader) != 0) {
            networkStreamReader_decryptFrame(&reader);
            valid = networkStreamReader_test(&reader);
            if (valid < 0) {
                setError(valid, 0, 0x80000000);
                return;
            }
            if (valid != 0) {
                outOfBand = networkPacket_getFlag10(&reader);
                channel = networkPacket_getChannel(&reader);
                filed = 0;
                if (outOfBand == 0) {
                    if (networkPacket_getFlag04(&reader) != 0) {
                        mtuReply_223F = 1;
                        if (mtuSeen_2240 < read_size_from_buffer(&reader)) {
                            mtuSeen_2240 = read_size_from_buffer(&reader);
                        }
                    } else if (channel == 0) {
                        filed = putTopPacket(&queues_1024.receive_30[channel], &reader);
                        networkStreamQueue_acknowledge(&queues_1024.send_00[channel], &reader);
                    } else if (channel == 1) {
                        filed = putAllPacket(&queues_1024.receive_30[channel], &reader);
                        if (filed < 0) {
                            filed = 0;
                        }
                    }
                } else if (outOfBand == 1) {
                    callback_04(3, index_20A4, 0, 1, (const u8*)&frame, owner_08);
                }
                if (filed < 4 && (u32)filed > 2) {
                    if (filed != 3) {
                        setError(filed, 0, 0x80000000);
                        return;
                    }
                    queues_1024.resend_68 = 1;
                }
            }
            networkStreamReader_consumePacket(&reader);
            lastReceive_2234 = now_21B4;
        }
    } while (received > 0);
}

/* The slot index. */
s32 NetworkConnectionStable::getIndex()
{
    return (s8)(u8)index_20A4;
}

/* Two for every open ping among the oldest ones (a ping not answered yet reads 0). */
s32 NetworkConnectionStable::getCongestion()
{
    s32 count;
    s32 i;
    s32 n;

    n = 7 - (s32)networkSessionDefaultDelay;
    count = 0;
    for (i = 0; i < n; i++) {
        if (networkConnectionZero == pingTimes_21D4[(pingIndex_21D0 + i) % 8]) {
            count += 2;
        }
    }
    return count;
}

/* Arms or disarms the receive timeout. */
void NetworkConnectionStable::setRelay(u8 enabled)
{
    receiveTimeoutOn_223D = enabled;
}

/* Holds the receive timeout for this frame. */
void NetworkConnectionStable::flush()
{
    receivedThisFrame_223C = 1;
}

/* Sets the default and this connection's keep-alive interval. */
void NetworkConnectionStable::setInterval(f32 seconds)
{
    NetworkConnection::setInterval(seconds);
    pollInterval_21C0 = seconds;
}

/* Sets the default and this connection's connect timeout. */
void NetworkConnectionStable::setTimeout(f32 seconds)
{
    NetworkConnection::setTimeout(seconds);
    connectTimeout_21C8 = seconds;
}

/* Sets the default and this connection's receive timeout. */
void NetworkConnectionStable::setLimit(f32 seconds)
{
    NetworkConnection::setLimit(seconds);
    receiveTimeout_2238 = seconds;
}

/* The averaged round trip. */
f32 NetworkConnectionStable::getRoundTripTime()
{
    return roundTrip_21F4;
}

/* Sets the queues' expiry. */
void NetworkConnectionStable::setValue(f32 value)
{
    queues_1024.expiry_64 = value;
}

/* The queues' expiry. */
f32 NetworkConnectionStable::getValue()
{
    return queues_1024.expiry_64;
}

/* The probed MTU once it is settled, else the safe 0x200. */
s32 NetworkConnectionStable::getMaxPacketSize()
{
    if (mtuFixed_2248 != 0) {
        return mtu_2244;
    }
    return 0x200;
}

/* Sets the machine's state. */
void NetworkConnectionStable::setState(s32 state)
{
    state_21B0 = state;
}

/* The machine's state. */
s32 NetworkConnectionStable::getState()
{
    return state_21B0;
}

/* Records the first error; a later one is dropped. */
void NetworkConnectionStable::setError(s32 code, s32 param1, s32 param2)
{
    if (error_0010.code_00 == 0) {
        error_0010.code_00 = code;
        error_0010.param1_04 = param1;
        error_0010.param2_08 = param2;
    }
}

/* 1, with an error recorded, for a channel other than 0 or 1. */
s32 NetworkConnectionStable::checkChannel(s32 channel)
{
    if (channel < 0 || 2 <= channel) {
        setError(0x80030001, channel, 0);
        return 1;
    }
    return 0;
}

/* Sends the hello (0x81, or 0x82 answering one): both send sequence numbers and this side's address record. */
void NetworkConnectionStable::sendHello(s32 reply)
{
    NetworkStreamWriter packet;
    u32 n;

    networkPacket_attach(&packet, frame_001C, 0x400);
    networkPacket_begin(&packet, 0);
    if (reply != 0) {
        n = (u16)writeByte(&packet, 0x82);
    } else {
        n = (u16)writeByte(&packet, 0x81);
    }
    n += (u16)writeUShort(&packet, networkStreamWriter_size(&queues_1024.send_00[0]));
    n += (u16)writeUShort(&packet, networkStreamWriter_size(&queues_1024.send_00[1]));
    n += (u16)writeUInt(&packet, addressSize_2128);
    n += (u16)writeBytes(&packet, address_20A5, addressSize_2128);
    writeSize(&packet, (u16)n);
    putOnSendPool(&packet, 1);
}

/* Sends the handshake's end (0x83) on channel 0. */
void NetworkConnectionStable::sendHelloDone()
{
    NetworkStreamWriter packet;

    networkPacket_attach(&packet, frame_001C, 0x400);
    networkPacket_begin(&packet, 0);
    writeSize(&packet, writeByte(&packet, 0x83));
    putOnSendPool(&packet, 0);
}

/* Opens ping `index` (0x91) with the time in milliseconds. */
void NetworkConnectionStable::sendPing(u8 index)
{
    NetworkStreamWriter packet;
    u32 time;
    u32 n;

    index = index % 8;
    time = (u32)(networkConnectionMilliseconds * now_21B4);
    pingTimes_21D4[index] = networkConnectionZero;
    networkPacket_attach(&packet, frame_001C, 0x400);
    networkPacket_begin(&packet, 0);
    n = (u16)writeByte(&packet, 0x91);
    n += (u16)writeByte(&packet, index);
    n += (u16)writeUInt(&packet, time);
    writeSize(&packet, (u16)n);
    putOnSendPool(&packet, 1);
}

/* Answers ping `index` (0x92) with its time. */
void NetworkConnectionStable::sendPong(u8 index, u32 time)
{
    NetworkStreamWriter packet;
    u32 n;

    index = index % 8;
    networkPacket_attach(&packet, frame_001C, 0x400);
    networkPacket_begin(&packet, 0);
    n = (u16)writeByte(&packet, 0x92);
    n += (u16)writeByte(&packet, index);
    n += (u16)writeUInt(&packet, time);
    writeSize(&packet, (u16)n);
    putOnSendPool(&packet, 1);
}

/* Sends the close (0x84 towards a relay, else 0x85). */
void NetworkConnectionStable::sendClose(s32 relay)
{
    NetworkStreamWriter packet;

    networkPacket_attach(&packet, frame_001C, 0x400);
    networkPacket_begin(&packet, 0);
    writeSize(&packet, writeByte(&packet, relay != 0 ? 0x84 : 0x85));
    putOnSendPool(&packet, 1);
}

/* Sends one padded probe frame of the MTU being tried, when a probe is due. */
void NetworkConnectionStable::sendMtuProbe()
{
    if (mtuProbe_223E != 0) {
        mtuProbe_223E = 0;
        NetworkStreamWriterDefault frame;
        networkStreamWriter_putBytes(&frame, frame_001C, mtu_2244);
        networkStreamWriter_flush(&frame);
        networkStreamWriter_skip(&frame, mtu_2244 - (u16)networkPacket_getFrameOverhead());
        networkStreamWriter_setMode(&frame, mtu_2244 - (u16)networkPacket_getFrameOverhead());
        networkStreamWriter_putU16(&frame, networkStreamWriter_size(&queues_1024.send_00[1]));
        networkStreamWriter_putU16b(&frame, networkStreamWriter_size(&queues_1024.receive_30[1]));
        networkStreamWriter_putU32(&frame, 0);
        networkStreamWriter_putU32b(&frame, 0);
        networkStreamWriter_enable1(&frame, 0);
        networkStreamWriter_enable2(&frame, 1);
        networkStreamWriter_setFlag04(&frame, 1);
        networkStreamWriter_commit(&frame);
        networkStreamWriter_bytes(&frame);
        if (peer_0C->send(frame_001C, getPosition(&frame), NULL, 0, 0) < 0) {
            return;
        }
    }
}

/* Counts down the probes of the current MTU; when they run out, steps it down by 0x80 and settles at 0x200. */
void NetworkConnectionStable::stepMtu()
{
    if (mtuFixed_2248 == 0) {
        if (mtuTries_224C <= 0) {
            if (mtu_2244 > 0x200) {
                mtu_2244 -= 0x80;
                mtuTries_224C = 8;
                getNetworkLogger()->signal_0C(1, "NetworkConnectionStable[%d] MTU down -> %d\n", index_20A4, mtu_2244);
            }
            if (mtu_2244 <= 0x200) {
                mtu_2244 = 0x200;
                mtuFixed_2248 = 1;
                getNetworkLogger()->signal_0C(1, "NetworkConnectionStable[%d] MTU is %d\n", index_20A4, mtu_2244);
                return;
            }
        }
        mtuTries_224C--;
        mtuProbe_223E = 1;
    }
}

/* Reports the largest probe received (0x95). */
void NetworkConnectionStable::sendMtuReply(u32 size)
{
    NetworkStreamWriter packet;
    u32 n;

    networkPacket_attach(&packet, frame_001C, 0x400);
    networkPacket_begin(&packet, 0);
    n = (u16)writeByte(&packet, 0x95);
    n += (u16)writeUInt(&packet, size);
    writeSize(&packet, (u16)n);
    putOnSendPool(&packet, 1);
}

/* Reads the control messages of channel 1 (hello, hello reply, ping, pong, MTU, close) and then channel 0's (the
 * handshake's end), and empties both queues. */
void NetworkConnectionStable::dispatchControl()
{
    NetworkStreamWriter packet;
    u8 type;

    type = 0;
    networkStreamQueue_fillPacket(&queues_1024.receive_30[1], &packet);
    while (networkPacket_hasMessage(&packet) != 0) {
        networkPacket_takeByte(&packet, &type);
        switch (type) {
        case 0x81: {
            u16 sequence;
            if (getState() == 2) {
                networkPacket_takeU16(&packet, &sequence);
                networkStreamQueue_setSequence(&queues_1024.receive_30[0], sequence);
                networkPacket_takeU16(&packet, &sequence);
                networkStreamQueue_setSequence(&queues_1024.receive_30[1], sequence);
                networkPacket_takeU32(&packet, (u32*)&peerAddressSize_21AC);
                peerAddressSize_21AC = peerAddressSize_21AC < 0x80 ? peerAddressSize_21AC : 0x80;
                networkPacket_takeBytes(&packet, peerAddress_212C, peerAddressSize_21AC);
                setState(3);
            }
            if (getState() == 3) {
                sendHello(1);
            }
            break;
        }
        case 0x82: {
            u16 sequence;
            if (getState() == 2) {
                networkPacket_takeU16(&packet, &sequence);
                networkStreamQueue_setSequence(&queues_1024.receive_30[0], sequence);
                networkPacket_takeU16(&packet, &sequence);
                networkStreamQueue_setSequence(&queues_1024.receive_30[1], sequence);
                networkPacket_takeU32(&packet, (u32*)&peerAddressSize_21AC);
                peerAddressSize_21AC = peerAddressSize_21AC < 0x80 ? peerAddressSize_21AC : 0x80;
                networkPacket_takeBytes(&packet, peerAddress_212C, peerAddressSize_21AC);
                setState(3);
            }
            if (getState() == 3) {
                sendHelloDone();
                setState(4);
                callback_04(1, index_20A4, 0, peerAddressSize_21AC, peerAddress_212C, owner_08);
            }
            break;
        }
        case 0x91:
            if (getState() == 4 || getState() == 3) {
                u8 index = 0;
                u32 time = 0;
                networkPacket_takeByte(&packet, &index);
                networkPacket_takeU32(&packet, &time);
                sendPong(index, time);
            }
            break;
        case 0x92:
            if (getState() == 4) {
                u8 index = 0;
                u32 time = 0;
                f32 sent;
                networkPacket_takeByte(&packet, &index);
                networkPacket_takeU32(&packet, &time);
                index = index % 8;
                sent = time / networkConnectionMilliseconds;
                if (index < 8 && sent <= now_21B4) {
                    pingTimes_21D4[index] = now_21B4 - sent;
                    if (networkConnectionZero == pingTimes_21D4[index]) {
                        pingTimes_21D4[index] = networkConnectionMinRoundTrip;
                    }
                }
            }
            break;
        case 0x95:
            if (getState() == 4 && mtuFixed_2248 == 0) {
                u32 size = 0;
                networkPacket_takeU32(&packet, &size);
                mtu_2244 = size;
                mtuFixed_2248 = 1;
                getNetworkLogger()->signal_0C(1, "NetworkConnectionStable[%d] MTU is %d\n", index_20A4, mtu_2244);
            }
            break;
        case 0x84:
            setState(0);
            callback_04(4, index_20A4, 0, 0, NULL, owner_08);
            return;
        case 0x85:
            setState(0);
            callback_04(5, index_20A4, 0, 0, NULL, owner_08);
            return;
        }
        networkPacket_nextMessage(&packet);
    }
    networkStreamQueue_clear(&queues_1024.receive_30[1]);
    networkStreamQueue_fillPacket(&queues_1024.receive_30[0], &packet);
    while (networkPacket_hasMessage(&packet) != 0) {
        networkPacket_takeByte(&packet, &type);
        if ((s32)type == 0x83 && getState() == 3) {
            setState(4);
            callback_04(1, index_20A4, 0, peerAddressSize_21AC, peerAddress_212C, owner_08);
        }
        networkPacket_nextMessage(&packet);
    }
    networkStreamQueue_clear(&queues_1024.receive_30[0]);
}

/* Folds ping `index` into the round-trip average (a lost or slow ping counts as the default delay) and counts it as
 * lost or answered. */
void NetworkConnectionStable::updateRoundTrip(u8 index)
{
    f32* ping;
    f32 time;

    ping = &pingTimes_21D4[(u8)(index % 8)];
    time = *ping;
    if (time <= networkConnectionZero || networkSessionDefaultDelay < time) {
        time = networkSessionDefaultDelay;
    }
    roundTrip_21F4 = networkConnectionRoundTripKeep * roundTrip_21F4 + networkConnectionRoundTripWeight * time;
    updateRate();
    if (networkConnectionZero == *ping) {
        pingsLost_21F8++;
        return;
    }
    pingsAnswered_21FC++;
}

/* Sets the send rate to twice the round trip (or the target), clamped to 0.1 .. 1 seconds. */
void NetworkConnectionStable::updateRate()
{
    f32 a;
    f32 rate;

    a = networkConnectionTwo * roundTrip_21F4;
    rate = networkConnectionTwo * queues_1024.rateTarget_78;
    rate = a < rate ? rate : a;
    rate = networkConnectionRoundTripWeight < rate ? rate : networkConnectionRoundTripWeight;
    rate = rate < networkConnectionPingInterval ? rate : networkConnectionPingInterval;
    queues_1024.rate_74 = rate;
}
