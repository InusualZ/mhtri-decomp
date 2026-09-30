/*
 * Network/NetworkSessionStable.cpp - `NetworkSessionStable`, the stable session the Network band opens
 *   (`initNetworkSessionStable`): its four slots, connection hand-off, control/event handling, rate governor
 *   and op-code packet writers.
 *
 * One translation unit of the retail Network transport band.  `.text` 0x803CF6D8..0x803D4904, extab
 * 0x80019AB4..0x80019F3C, extabindex 0x8003A344..0x8003A5A8, `.data` 0x805F9A40..0x805FA788.  The seam at
 * 0x803D4904 is proven by the `.data` order (`dataorder.py at 0x805FA6E8`, `tudiscover`: the two tables are
 * followed by `NetworkSessionManager`'s first string, and the down/up performance and out-of-band strings sit
 * before them), so the twelve op-code writers and rate functions that opened `Network/NetworkSessionManager.cpp`
 * (0x803D3CE8..0x803D4904) belong here; that unit now starts at 0x803D4904.
 *
 * CLASSES.  `NetworkSessionStable : NetworkSessionBase` (every one of the base's 38 slots is overridden here),
 * `NetworkSessionSlot` (0x924 B, four of them), `NetworkSlotSmallObject`, the connection/queue classes the writer
 * band owns (declared only) and `NetworkUnitPacket : NetworkStreamSink`.  The tables are compiler output
 * (rule 10): 0x805FA6E8 from `~NetworkSessionStable`, 0x805FA6A8 from `~NetworkUnitPacket`.  Slot, field and
 * method names are the log strings' (`init`, `set`, `put`, `send`, `move`, `execControl`, `execControlOne`,
 * `setNetworkConnectionEvent`, `downPerformance`, `upPerformance`, `moveOutOfBand`) or GUESSes read off the body,
 * each marked in `Network/NetworkSessionStable.h`; the callees in the unsplit writer band are named from their
 * bodies in `unsplit/NetworkStream.h` (GUESS on all of them).
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off`
 * (`configure.py`); file-scope `#pragma peephole off` (playbook 39) and `#pragma fp_contract off` (retail keeps
 * `fmuls`+`fadds` in `move` and `upPerformance`); `dont_inline` around the nonce pair and the destructors, whose
 * `bl`s retail keeps.  Source shapes that are levers here, each measured: bodies in address order (a callee
 * defined after its caller is not inlined, as in retail); `4 <= index` for the slot bounds test (`index >= 4`
 * merges both compares into one unsigned one); `slots_14828[index].field` rather than a local slot pointer
 * where retail recomputes the base; the packet getters return `u32` and the source casts `(u16)` where retail
 * masks; the `.sdata2` floats are `const` so loops hoist them; a ternary for the clamp in `updateRate`; the
 * return value of `execControlOne` is computed before the small object's destructor runs.
 *
 * DATA.  The claim holds the strings, the jump table and both tables, and `flipcheck.py` reports the `.data`
 * section byte-identical.  The object also emits 16 B of `.sdata2` int-to-float magic constants that retail
 * loads from `Network/network_shared_data.cpp`'s pool (playbook 58: this unit is not their sole referencer, so
 * they cannot be claimed).
 *
 * RESIDUALS (`flipcheck.py`, `relocdiff.py --by-owner`).  NOT READY: `.text` 0x51F0 against the claim's 0x522C
 * (-60); `extab` 0x2A0 against 0x488 (-488; 621 of 1160 bytes differ, 17 `@etb_` records only in the target and
 * 2 only in ours); `extabindex` 12 of 612 bytes differ; `.sdata2` 16 B unclaimable as above.  Rows:
 *  - register allocation and frame size in the long functions (`move`, `init`, `send`, `set`, `execControlOne`)
 *    and in `connect` (`bge` to the epilogue where retail has `blt` over a `b`, an extra `stw r29`, frame -0x20
 *    against -0x10, the two `stw 0x4828` stores in the other order);
 *  - `setError`, `markLeft` and `kick` end the bounds test with `bgelr` where retail branches over a `blr`;
 *  - `post` addresses `queueUsed_518[channel]` as base+index where retail folds the offset into the displacement;
 *  - the `writeOp*` and `writeSize` arguments are masked one instruction earlier than retail;
 *  - `move`: retail destroys the inlined `writeOp7` stream after the `setError` loop that follows it, ours before
 *    it (the `bl` at +0x360 lands eight bytes away and the `@eti_` immediate of the next `setError` is shifted);
 *  - `upPerformance`: retail reloads `networkRateFloor` (the `.sdata` word) for the clamp, ours reuses the
 *    register (the `lfs` at +0x70), and the `byteFloor`/`byteLimit` compare is `ble` against retail's `bge`;
 *  - the string/jump-table labels (`@NNNN` against `lbl_...`), dtk's `@eti_` immediates and `_savegpr_`/`_restgpr_`
 *    entry points that follow the frame differences.
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

/* Retail keeps `fmuls`+`fadds` where `-fp_contract on` would fuse a `fmadds` (`move`, `upPerformance`). */
#pragma fp_contract off

/* Sets how many host and subhost slots the session allows (GUESS on both meanings). */
void NetworkSessionBase::setLimits(u32 maxHosts, u32 maxSubhosts)
{
    networkSessionMaxHosts = maxHosts;
    networkSessionMaxSubhosts = maxSubhosts;
}

/* Sets the host timeout in seconds (GUESS: the name follows the word it writes). */
void NetworkSessionBase::setHostTimeout(f32 seconds)
{
    networkSessionHostTimeout = seconds;
}

/* Sets the subhost timeout in seconds (GUESS: the name follows the word it writes). */
void NetworkSessionBase::setSubhostTimeout(f32 seconds)
{
    networkSessionSubhostTimeout = seconds;
}

/* Stores a window (the quotient of two counts) and the per-unit step, the reciprocal of the divisor. */
void NetworkSessionBase::setRate(s32 count, s32 divisor)
{
    networkSessionRateWindow = count / divisor;
    networkSessionRateStep = networkSessionUnit / (f32)divisor;
}

/* Retail calls the two nonce helpers out of line from the session's methods, so the inline pass skips them. */
#pragma dont_inline on
extern "C" {

/* Draws a session nonce from the clock and `rand`, redrawing until it is neither 0 nor -1. */
u32 networkSessionNonce_generate(void)
{
    f32 scale = networkNonceScale;
    u32 nonce;

    do {
        u32 stamp = (u32)(scale * getNetworkLogger()->getTime_60());
        nonce = rand() + stamp;
    } while (nonce == 0 || nonce == 0xFFFFFFFF);
    return nonce;
}

/* True when a nonce is neither 0 nor -1. */
s32 networkSessionNonce_isValid(u32 nonce)
{
    s32 valid = 0;

    if (nonce != 0 && nonce != 0xFFFFFFFF) {
        valid = 1;
    }
    return valid;
}

}
#pragma dont_inline off

/* The connection's event callback: forwards the event to the session that opened the connection. */
void NetworkSessionStable::onConnectionEvent(s32 event, s8 index, u32 arg, s32 size, const u8* data,
                                             NetworkSessionStable* self)
{
    self->setNetworkConnectionEvent(event, index, arg, size, data);
}

/* Builds the session: the base, the four slots and its own address object. */
NetworkSessionStable::NetworkSessionStable()
{
}

#pragma dont_inline on

/* Destroys the address object's small object. */
NetworkSlotSmallObject::~NetworkSlotSmallObject()
{
    ((NetworkSmallObjectSink*)&object_00)->NetworkSmallObjectSink::~NetworkSmallObjectSink();
}

/* Retail keeps the `bl` into the object's own constructor/destructor rather than folding it (the two
   live in different original objects), so the inline pass is off for these. */

/* Builds the address object around the writer band's small object. */
NetworkSlotSmallObject::NetworkSlotSmallObject()
{
    networkSmallObject_construct(&object_00);
}

/* Destroys the slot's queues and its address object. */
NetworkSessionSlot::~NetworkSessionSlot()
{
}

#pragma dont_inline off

/* Builds the slot: its address object and its four queues. */
NetworkSessionSlot::NetworkSessionSlot()
{
}

#pragma dont_inline on

/* Resets every slot, then destroys the address object, the slots and the base. */
NetworkSessionStable::~NetworkSessionStable()
{
    resetAllSlots();
}

#pragma dont_inline off

/* Opens the session: clears the buffers, readies the four slots' queues, draws the nonce and takes the
   own slot. */
/* untyped: caller-owned payload - the callback's user pointer */
void NetworkSessionStable::init(s8 isHost, NetworkSessionCallback callback, void* user, const u8* address, s32 param)
{
    s32 i;

    memset(sendBuffer_0D, 0, sizeof(sendBuffer_0D));
    memset(receiveBuffer_410, 0, sizeof(receiveBuffer_410));
    isHost_0C = isHost;
    callback_04 = callback;
    user_08 = user;
    userFlagA_14810 = 0;
    userFlagB_14811 = 0;
    nonceDrop_14814[0] = 0;
    nonceDrop_14814[1] = 0;
    nonceDrop_14814[2] = 0;
    nonceDrop_14814[3] = 0;
    hostIndex_14824 = -1;
    subhostIndex_14825 = -1;
    ownIndex_14826 = -1;
    for (i = 0; i < 4; i++) {
        NetworkSessionSlot* slot = &slots_14828[i];

        slot->linkState_00 = 0;
        slot->sessionState_04 = 0;
        slot->closeState_08 = 0;
        slot->sleepSeconds_0C = 0;
        slot->relayIndex_10 = -1;
        slot->relayDelay_14 = networkSessionDefaultDelay;
        slot->established_18 = 0;
        slot->authenticated_19 = 0;
        slot->left_1A = 0;
        slot->relayAck_1B = 0;
        slot->shutdown_1C = 0;
        memset(&slot->error_20, 0, sizeof(slot->error_20));
        slot->connection_2C = NULL;
        slot->address_30.object_00.vtable->slot_18(&slot->address_30.object_00);
        slot->nonce_50 = 0;
        slot->waitStart_EC = networkSessionZero;
        slot->coolStart_F0 = networkSessionZero;
        slot->sequence_F4 = 0xFFFF;
        slot->retryTime_F8 = networkSessionZero;
        slot->retryCount_FC = 0;
        slot->sleeping_100 = 0;
        slot->sleepStart_104 = networkSessionZero;
        slot->measureTime_108 = networkSessionZero;
        slot->congestion_10C = networkSessionZero;
        slot->congestionCount_110 = 0;
        slot->measureSend_114 = networkSessionZero;
        memset(slot->queue_118, 0, sizeof(slot->queue_118));
        memset(&slot->queueUsed_518[0], 0, sizeof(slot->queueUsed_518[0]));
        slot->queueUsed_518[1] = 0;
        memset(slot->queue2_520, 0, sizeof(slot->queue2_520));
        memset(&slot->queue2Used_920, 0, sizeof(slot->queue2Used_920));
        slot->queues_54.send_00[0].attach(sendBlock_810[i], sizeof(sendBlock_810[i]));
        slot->queues_54.send_00[1].attach(sendAux_8810[i], sizeof(sendAux_8810[i]));
        networkStreamQueue_setSequence(&slot->queues_54.send_00[0],
                                       (u16)(rand() + (u32)(networkMillisecondsPerSecond * getNetworkLogger()->getTime_60())));
        networkStreamQueue_setSequence(&slot->queues_54.send_00[1],
                                       (u16)(rand() + (u32)(networkMillisecondsPerSecond * getNetworkLogger()->getTime_60())));
        slot->queues_54.receive_30[0].attach(receiveBlock_A810[i], sizeof(receiveBlock_A810[i]));
        slot->queues_54.receive_30[1].attach(receiveAux_12810[i], sizeof(receiveAux_12810[i]));
        slot->receiveStarted_B4 = 0;
        slot->expiry_B8 = networkSessionZero;
        slot->resend_BC = 0;
        memset(slot->lastSend_C0, 0, sizeof(slot->lastSend_C0));
        slot->rate_C8 = networkRateScale * networkSessionDefaultDelay;
        slot->rateTarget_CC = networkRateFloor;
        slot->byteLimit_D0 = 0x400;
        slot->lastAdjust_D4 = networkSessionZero;
        slot->adjusting_E0 = 0;
        slot->rateFloor_D8 = networkRateFloor;
        slot->byteFloor_DC = 0x400;
        slot->lastRaise_E4 = networkSessionZero;
        slot->lastLower_E8 = networkSessionZero;
    }
    address_16CB8.object_00.vtable->slot_18(&address_16CB8.object_00);
    nonce_16CD8 = 0;
    time_16CDC = getNetworkLogger()->getTime_60();
    hostSeen_16CE0 = networkSessionZero;
    subhostSeen_16CE4 = networkSessionZero;
    maxHosts_16CE8 = networkSessionMaxHosts;
    maxSubhosts_16CEC = networkSessionMaxSubhosts;
    hostTimeout_16CF0 = networkSessionHostTimeout;
    subhostTimeout_16CF4 = networkSessionSubhostTimeout;
    rateStep_16CF8 = networkSessionRateStep;
    rateWindow_16CFC = networkSessionRateWindow;
    joined_16D00 = isHost;
    address_16CB8.object_00.vtable->slot_28(&address_16CB8.object_00, address);
    nonce_16CD8 = networkSessionNonce_generate();
    getNetworkLogger()->signal_0C(3, "NetworkSessionStable::init: my nonce is 0x%08x\n", nonce_16CD8);
    ownIndex_14826 = set(1, address);
    connect(ownIndex_14826, param, 0);
}

/* Resets every slot through the +0x1C slot of the session. */
void NetworkSessionStable::resetAllSlots()
{
    s32 index;

    for (index = 0; index < 4; index++) {
        resetSlot((s8)index);
    }
}

/* Frames op code 5 and sends it to the slot `index` on the reliable channel. */
inline void NetworkSessionStable::writeOp5(s8 index)
{
    NetworkStreamWriter stream;
    s8 term;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    writeSize(&stream, (u16)writeByte(&stream, 5));
    term = index;
    sendStream(&stream, 1, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 3 with the slot's nonce and sends it to every slot but the own one. */
inline void NetworkSessionStable::writeOp3(const NetworkSessionSlot* slot)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 n;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    n = (u16)writeByte(&stream, 3);
    n += (u16)writeUInt(&stream, slot->nonce_50);
    writeSize(&stream, (u16)n);
    term = -2;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 7 and sends it to every slot but the own one on the reliable channel. */
inline void NetworkSessionStable::writeOp7()
{
    NetworkStreamWriter stream;
    s8 term;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    writeSize(&stream, (u16)writeByte(&stream, 7));
    term = -2;
    sendStream(&stream, 1, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* One tick of the session: steps every slot's close, link and session machines, flushes what is queued
   for it, then measures the congestion of the usable slots, tunes the rate governor and times out a
   missing host or subhost. */
void NetworkSessionStable::move()
{
    NetworkSessionSlot* slot;
    NetworkConnectionStable* connection;
    s8 index;
    s8 j;
    s32 k;
    s32 channel;
    s32 queued;
    s32 room;
    s32 consumed;
    s32 sendResult;
    s8 usable;
    s32 connectedCount;
    s32 hostsOver;
    s32 hostsWarn;
    s32 subhostsOver;
    s32 subhostsWarn;
    s32 sizes[4];
    s32 peers;
    u16 length;
    u16 recordLength;
    u16 encoded;
    NetworkStreamWriter packet;
    f32 timeout;

    time_16CDC = getNetworkLogger()->getTime_60();
    slot = &slots_14828[0];
    for (index = 0; index < 4; index++, slot++) {
        connection = slot->connection_2C;
        if (connection != NULL) {
            connection->update();
            if (networkSessionNonce_isValid(slot->nonce_50) != 0) {
                for (k = 0; k < 4; k++) {
                    if (slot->nonce_50 == nonceDrop_14814[k]) {
                        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d]'s nonce was found in mNonceDrop. sync_close now -> 0x%08x\n",
                                                      index, nonceDrop_14814[k]);
                        setError(index, NETWORK_ERROR_SESSION_DROPPED, 0, 0x80000000, 3);
                        break;
                    }
                }
            }
            switch (slot->closeState_08) {
            case 1:
                writeOp5(index);
                slot->closeState_08 = 2;
            case 2:
                if (networkSessionNonce_isValid(slot->nonce_50) != 0) {
                    writeOp3(slot);
                }
                slot->closeState_08 = 3;
            case 3:
                if ((s8)ownIndex_14826 == index) {
                    writeOp7();
                    for (j = 0; j < 4; j++) {
                        if ((s8)ownIndex_14826 != j && slots_14828[j].connection_2C != 0) {
                            setError(j, NETWORK_ERROR_CONNECT_FAILED, 0, 0x80000000, 3);
                        }
                    }
                }
                slot->shutdown_1C = 0;
                connection->clearPending();
                slot->closeState_08 = 4;
            case 4:
                if (slot->shutdown_1C != 0) {
                    callback_04(3, index, (u32)slot->error_20.source, 1, &slot->error_20, user_08);
                    resetSlot(index);
                }
                continue;
            default:
                break;
            }
        switch (slot->linkState_00) {
        case 1:
            slot->left_1A = 0;
            slot->authenticated_19 = 0;
            getRoundTrip(index);
            connection->begin();
            slot->linkState_00 = 2;
            break;
        case 2:
            if (slot->left_1A != 0) {
                slot->linkState_00 = 4;
            } else if (slot->authenticated_19 != 0) {
                connection->setRelay(1);
                slot->linkState_00 = 3;
            }
            break;
        case 3:
            if (slot->left_1A != 0) {
                slot->linkState_00 = 4;
            }
            break;
        case 4:
            connection->end();
            if (index == (s8)ownIndex_14826) {
                slot->linkState_00 = 1;
            } else {
                peers = 0;
                for (j = 0; j < 4; j++) {
                    if (slots_14828[j].connection_2C != 0) {
                        peers++;
                    }
                }
                if (peers <= 2) {
                    setError(index, NETWORK_ERROR_PEER_CLOSED, 1, 0x80000000, 1);
                } else {
                    slot->waitStart_EC = time_16CDC;
                    slot->linkState_00 = 5;
                    if (slot->sessionState_04 == 0) {
                        slot->sessionState_04 = 1;
                    }
                }
            }
            break;
        case 5:
            if (networkSessionRelayCooldown + slot->waitStart_EC <= time_16CDC) {
                slot->linkState_00 = 1;
            }
            break;
        }
        if (slot->authenticated_19 != 0) {
            slot->sessionState_04 = 0;
            slot->relayIndex_10 = -1;
            slot->relayDelay_14 = networkSessionDefaultDelay;
            slot->sequence_F4 = 0xFFFF;
            slot->relayAck_1B = 0;
        }
        switch (slot->sessionState_04) {
        case 1:
            if (index == (s8)ownIndex_14826) {
                slot->sessionState_04 = 0;
            } else {
                getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d] relay connect init.\n", index);
                connection->setRelay(0);
                slot->relayIndex_10 = -2;
                slot->relayDelay_14 = networkSessionDefaultDelay;
                slot->sequence_F4 = 0xFFFF;
                slot->relayAck_1B = 0;
                slot->retryTime_F8 = networkSessionZero;
                slot->retryCount_FC = 0;
                slot->sessionState_04 = 2;
            }
            break;
        case 2:
            if (slot->relayIndex_10 < 0) {
                if (networkRateMax + slot->retryTime_F8 <= time_16CDC) {
                    slot->retryTime_F8 = time_16CDC;
                    slot->retryCount_FC = slot->retryCount_FC + 1;
                    if (networkSessionRelayWarnRatio * subhostTimeout_16CF4 < (f32)slot->retryCount_FC) {
                        getNetworkLogger()->log_14("NetworkSessionStable::move: [%d] no relay. -> %d/%d\n", index,
                                                   slot->retryCount_FC, (s32)subhostTimeout_16CF4);
                    }
                    if (subhostTimeout_16CF4 < (f32)slot->retryCount_FC) {
                        if (joined_16D00 == 0) {
                            getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d] cannot establish, but drop sync disable. -> 0x%08x\n",
                                                          index, slot->nonce_50);
                            setError((s8)ownIndex_14826, NETWORK_ERROR_CONNECT_TIMEOUT, slot->retryCount_FC, 0x80000000, 3);
                        } else if (networkSessionNonce_isValid(slot->nonce_50) != 0) {
                            getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d] cannot establish. kick now. -> 0x%08x\n",
                                                          index, slot->nonce_50);
                            setError(index, NETWORK_ERROR_PEER_CLOSED, 0, 0x80000000, 1);
                        } else if (isHost_0C == 0 && (s8)ownIndex_14826 == hostIndex_14824) {
                            getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d] cannot establish, first connect failed.\n", index);
                            setError((s8)ownIndex_14826, NETWORK_ERROR_CONNECT_TIMEOUT, slot->retryCount_FC, 0x80000000, 3);
                        }
                    } else {
                        writeOp10(index);
                    }
                }
            } else {
                slot->retryTime_F8 = networkSessionZero;
                slot->retryCount_FC = 0;
                slot->sessionState_04 = 3;
            }
            break;
        case 3:
            if (getUsableSlot(index) < 0) {
                getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: relay closed. [%d] -> [%d]\n", index, slot->relayIndex_10);
                slot->sessionState_04 = 1;
            } else if (slot->relayAck_1B != 0) {
                getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: relay establish. [%d] -> [%d] -> [%d]\n",
                                              (s8)ownIndex_14826, slot->relayIndex_10, index);
                slot->sessionState_04 = 4;
            } else if (networkRateMax + slot->retryTime_F8 <= time_16CDC) {
                slot->retryTime_F8 = time_16CDC;
                moveOutOfBand(index);
                if (slot->receiveStarted_B4 != 0) {
                    writeOp8or9(0, index);
                }
            }
            break;
        case 4:
            if (getUsableSlot(index) < 0) {
                getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: relay closed. [%d] -> [%d]\n", index, slot->relayIndex_10);
                slot->sessionState_04 = 1;
            } else if (networkSessionEstablishInterval + slot->retryTime_F8 <= time_16CDC) {
                slot->sequence_F4 = 0xFFFF;
                slot->retryTime_F8 = time_16CDC;
                writeOp10(index);
            }
            break;
        case 5:
            slot->coolStart_F0 = time_16CDC;
            slot->sessionState_04 = 6;
            break;
        case 6:
            if (slot->coolStart_F0 + networkSessionDefaultDelay <= time_16CDC) {
                slot->sessionState_04 = 1;
            }
            break;
        }
        execControl(index, 0);
        usable = getUsableSlot(index);
        if (usable >= 0) {
            channel = slot->queueUsed_518[1];
            slots_14828[usable].connection_2C->clearSendPending();
            if (send(index, 0, slots_14828[index].resend_BC) != 0) {
                slots_14828[index].resend_BC = 0;
            }
            queued = slot->queueUsed_518[channel];
            sendResult = send(index, 1, queued > 0);
            room = slots_14828[usable].connection_2C->getSendRoom();
            if (slot->queueUsed_518[channel] > 0 && room > 0 && sendResult != 0) {
                length = 0;
                consumed = 0;
                if (slot->queueUsed_518[channel] > 0 && room > 0) {
                    memcpy(&recordLength, slot->queue_118 + (channel << 10), 2);
                    encoded = getNetworkLogger()->flag_48(recordLength);
                    if (encoded == 0 || room < encoded) {
                        break;
                    }
                    memcpy(sendBuffer_0D, slot->queue_118 + (channel << 10) + 2, encoded);
                    length = encoded;
                    consumed = length + 2;
                }
                networkStreamWriter_reserve(slots_14828[usable].connection_2C, (const u8*)sendBuffer_0D, length, (s8)channel);
                if (consumed > 0) {
                    slot->queueUsed_518[channel] = slot->queueUsed_518[channel] - consumed;
                    if (slot->queueUsed_518[channel] > 0) {
                        memmove(slot->queue_118 + (channel << 10), slot->queue_118 + (channel << 10) + consumed,
                                slot->queueUsed_518[channel]);
                    }
                }
            } else {
                networkStreamWriter_reserve(slots_14828[usable].connection_2C, NULL, 0, 0);
            }
            slot->queueUsed_518[1] = 0;
        }
        updateRate(index);
        if (slot->sleeping_100 != 0) {
            connection->flush();
            if (getUsableSlot(index) < 0) {
                slot->sleepStart_104 = time_16CDC;
            } else if (slot->sleepStart_104 + (f32)slot->sleepSeconds_0C <= time_16CDC) {
                setError(index, NETWORK_ERROR_SLEEP_TIMEOUT, slot->sleepSeconds_0C, 0x80000000, 1);
            }
        }
        }
    }
    connectedCount = 0;
    hostsOver = 0;
    hostsWarn = 0;
    subhostsOver = 0;
    subhostsWarn = 0;
    memset(sizes, 0, sizeof(sizes));
    for (index = 0, slot = &slots_14828[0]; index < 4; index++, slot++) {
        if (isConnected(index) != 0) {
            usable = getUsableSlot(index);
            if (usable >= 0) {
                if (networkRateMax + slot->measureTime_108 <= time_16CDC) {
                    slot->measureTime_108 = time_16CDC;
                    networkPacket_construct(&packet);
                    networkStreamQueue_fillPacket(&slot->queues_54.send_00[0], &packet);
                    length = networkPacket_getLength(&packet);
                    encoded = networkPacket_getMessageOverhead();
                    if ((s32)length < (s32)(networkPacket_getFrameOverhead() + encoded)) {
                        slot->congestionCount_110 = 0;
                        slot->congestion_10C = networkSessionCongestionKeep * slot->congestion_10C + networkRateMin * (f32)length;
                    } else if ((f32)length <= slot->congestion_10C) {
                        if (slot->congestionCount_110 > 0) {
                            slot->congestionCount_110 = slot->congestionCount_110 - 1;
                        }
                        slot->congestion_10C = networkSessionCongestionKeep * slot->congestion_10C + networkRateMin * (f32)length;
                    } else {
                        if (slot->congestionCount_110 < 0xFFFF) {
                            slot->congestionCount_110 = slot->congestionCount_110 + 1;
                        }
                        slot->congestion_10C = (f32)length;
                    }
                    if (slot->congestionCount_110 > 7) {
                        getNetworkLogger()->log_14("NetworkSessionStable::move: [%d] %d <= %f congestion worse %d\n", index, length,
                                                   slot->congestion_10C, slot->congestionCount_110);
                    }
                    if (networkRateMax + slot->measureSend_114 <= time_16CDC) {
                        writeOp6(index);
                    }
                    networkStreamWriter_dtor(&packet, -1);
                }
                connectedCount++;
                sizes[index] = slot->congestionCount_110 + slots_14828[usable].connection_2C->getCongestion();
                if (maxHosts_16CE8 <= sizes[index]) {
                    hostsOver++;
                } else if (maxHosts_16CE8 / 2 <= sizes[index]) {
                    hostsWarn++;
                }
                if (maxSubhosts_16CEC <= sizes[index]) {
                    subhostsOver++;
                } else if (maxSubhosts_16CEC / 2 <= sizes[index]) {
                    subhostsWarn++;
                }
            }
        }
    }
    if (connectedCount > 2) {
        if (connectedCount - 1 <= hostsOver) {
            setError((s8)ownIndex_14826, NETWORK_ERROR_SESSION_TIMEOUT, 0, 0x80000000, 3);
        } else if (connectedCount - 1 <= hostsWarn) {
            for (j = 0; j < 4; j++) {
                if (slots_14828[j].connection_2C != NULL) {
                    slots_14828[j].connection_2C->flush();
                }
            }
        }
    }
    if (connectedCount > 1) {
        if (subhostsOver >= 1) {
            if (connectedCount / 2 <= subhostsWarn) {
                for (j = 0; j < 4; j++) {
                    if (isConnected(j) != 0 && j != (s8)ownIndex_14826) {
                        downPerformance(j);
                    }
                }
            } else {
                for (j = 0; j < 4; j++) {
                    if (isConnected(j) != 0 && j != (s8)ownIndex_14826 && sizes[j] >= maxSubhosts_16CEC) {
                        downPerformance(j);
                    }
                }
            }
        } else if (subhostsWarn == 0) {
            for (j = 0; j < 4; j++) {
                if (isConnected(j) != 0 && j != (s8)ownIndex_14826) {
                    upPerformance(j);
                }
            }
        }
    }
    if (getUsableSlot(hostIndex_14824) >= 0) {
        if (networkSessionZero == hostSeen_16CE0) {
            getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d] host[%d] exist.\n", (s8)ownIndex_14826, hostIndex_14824);
        }
        hostSeen_16CE0 = time_16CDC;
    }
    if (networkSessionZero < hostSeen_16CE0) {
        if (isHost_0C != 0) {
            timeout = hostTimeout_16CF0;
        } else {
            timeout = (f32)(maxHosts_16CE8 + 6);
        }
        if (timeout <= time_16CDC - hostSeen_16CE0) {
            setError((s8)ownIndex_14826, NETWORK_ERROR_HOST_TIMEOUT, (s32)timeout, 0x80000000, 3);
        }
    }
    if (getUsableSlot(subhostIndex_14825) >= 0) {
        if (networkSessionZero == subhostSeen_16CE4) {
            getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: [%d] subhost[%d] exist.\n", (s8)ownIndex_14826, subhostIndex_14825);
        }
        subhostSeen_16CE4 = time_16CDC;
    }
    if (networkSessionZero < subhostSeen_16CE4) {
        if (isHost_0C != 0) {
            timeout = hostTimeout_16CF0;
        } else {
            timeout = (f32)(maxHosts_16CE8 + 6);
        }
        if (timeout <= time_16CDC - subhostSeen_16CE4) {
            setError((s8)ownIndex_14826, NETWORK_ERROR_HOST_TIMEOUT, (s32)timeout, 0x80000000, 3);
        }
    }
}

/* Takes the free slot, builds its connection and hands it the session's callback and the address record. */
s32 NetworkSessionStable::set(s32 isSelf, const u8* address)
{
    NetworkConnectionStable* connection;
    u8 block[0x80];
    NetworkConnectionOpenInfo info;
    s32 index;
    NetworkSessionSlot* slot;

    connection = new NetworkConnectionStable(isSelf);
    NetworkUnitPacket packet;
    if (connection == NULL) {
        getNetworkLogger()->warn_10("NetworkSessionStable::set: connection is NULL.\n");
        return -1;
    }
    for (index = 0; index < 4; index++) {
        if (slots_14828[index].connection_2C == NULL) {
            break;
        }
    }
    if (index == 4) {
        getNetworkLogger()->warn_10("NetworkSessionStable::set: cannot get connection work.\n");
        if (connection != NULL) {
            delete connection;
        }
        return -1;
    }
    packet.attach(block, sizeof(block));
    ((NetworkByteStream*)&packet)->pullRecord((NetworkStreamSink*)&address_16CB8.object_00);
    ((NetworkByteStream*)&packet)->putU32(nonce_16CD8);
    ((NetworkByteStream*)&packet)->putU16((u16)networkStreamWriter_size(&slots_14828[index].queues_54.send_00[0]));
    ((NetworkByteStream*)&packet)->putU16((u16)networkStreamWriter_size(&slots_14828[index].queues_54.send_00[1]));
    memset(&info, 0, sizeof(info));
    info.data_00 = ((NetworkByteStream*)&packet)->getData();
    info.size_04 = ((NetworkByteStream*)&packet)->getSize();
    connection->open(onConnectionEvent, this, index, &info);
    slot = &slots_14828[index];
    slot->connection_2C = connection;
    slot->address_30.object_00.vtable->slot_28(&slot->address_30.object_00, address);
    slot->nonce_50 = 0;
    return index;
}

#pragma dont_inline on

/* Destroys the unit packet (the base destructor does the freeing). */
NetworkUnitPacket::~NetworkUnitPacket()
{
}

/* Builds the unit packet. */
NetworkUnitPacket::NetworkUnitPacket()
{
}

#pragma dont_inline off

/* Releases a slot's connection and puts every field of the slot back to its initial state. */
void NetworkSessionStable::resetSlot(s8 index)
{
    NetworkSessionSlot* slot;

    if (index >= 0) {
        if (index >= 4) {
            return;
        }
        slot = &slots_14828[index];
        if (slot->connection_2C != NULL) {
            slot->connection_2C->reset();
            if (slot->connection_2C != NULL) {
                delete slot->connection_2C;
                slot->connection_2C = NULL;
            }
            slot->linkState_00 = 0;
            slot->sessionState_04 = 0;
            slot->closeState_08 = 0;
            slot->sleepSeconds_0C = 0;
            slot->relayIndex_10 = -1;
            slot->relayDelay_14 = networkSessionDefaultDelay;
            slot->established_18 = 0;
            slot->authenticated_19 = 0;
            slot->left_1A = 0;
            slot->relayAck_1B = 0;
            slot->shutdown_1C = 0;
            memset(&slot->error_20, 0, sizeof(slot->error_20));
            slot->connection_2C = NULL;
            slot->address_30.object_00.vtable->slot_18(&slot->address_30.object_00);
            slot->nonce_50 = 0;
            slot->waitStart_EC = networkSessionZero;
            slot->coolStart_F0 = networkSessionZero;
            slot->sequence_F4 = 0xFFFF;
            slot->retryTime_F8 = networkSessionZero;
            slot->retryCount_FC = 0;
            slot->sleeping_100 = 0;
            slot->sleepStart_104 = networkSessionZero;
            slot->measureTime_108 = networkSessionZero;
            slot->congestion_10C = networkSessionZero;
            slot->congestionCount_110 = 0;
            slot->measureSend_114 = networkSessionZero;
            networkStreamQueue_clear(&slot->queues_54.send_00[0]);
            networkStreamQueue_clear(&slot->queues_54.send_00[1]);
            networkStreamQueue_clear(&slot->queues_54.receive_30[0]);
            networkStreamQueue_clear(&slot->queues_54.receive_30[1]);
            networkStreamQueue_setSequence(&slot->queues_54.send_00[0],
                                           (u16)(rand() + (u32)(networkMillisecondsPerSecond * getNetworkLogger()->getTime_60())));
            networkStreamQueue_setSequence(&slot->queues_54.send_00[1],
                                           (u16)(rand() + (u32)(networkMillisecondsPerSecond * getNetworkLogger()->getTime_60())));
            slot->receiveStarted_B4 = 0;
            slot->resend_BC = 0;
            slot->expiry_B8 = networkSessionZero;
            slot->lastSend_C0[0] = networkSessionZero;
            slot->lastSend_C0[1] = networkSessionZero;
            slot->rate_C8 = networkRateScale * networkSessionDefaultDelay;
            slot->rateTarget_CC = networkRateFloor;
            slot->byteLimit_D0 = 0x400;
            slot->lastAdjust_D4 = networkSessionZero;
            slot->adjusting_E0 = 0;
            slot->rateFloor_D8 = networkRateFloor;
            slot->byteFloor_DC = 0x400;
            slot->lastRaise_E4 = networkSessionZero;
            slot->lastLower_E8 = networkSessionZero;
        }
    }
}

/* Starts the slot's connection once, and arms its connect machine. */
void NetworkSessionStable::connect(s8 index, u32 a, u32 b)
{
    if (index >= 0) {
        if (4 <= index) {
            return;
        }
        if (slots_14828[index].connection_2C != NULL && slots_14828[index].linkState_00 == 0) {
            slots_14828[index].connection_2C->start(a, b);
            slots_14828[index].linkState_00 = 1;
            if (slots_14828[index].sessionState_04 == 0) {
                slots_14828[index].sessionState_04 = 5;
            }
        }
    }
}

/* Stops every slot's connection. */
void NetworkSessionStable::disconnectAll()
{
    s32 index;

    for (index = 0; index < 4; index++) {
        NetworkConnectionStable* connection = slots_14828[index].connection_2C;

        if (connection != NULL) {
            connection->stop();
        }
    }
}

/* Records the own slot as kicked. */
void NetworkSessionStable::leave()
{
    setError((s8)ownIndex_14826, NETWORK_ERROR_SESSION_KICKED, 0, 0x80000000, 3);
}

/* Records a slot as kicked. */
void NetworkSessionStable::kick(s8 index)
{
    if (index >= 0) {
        if (4 <= index) {
            return;
        }
        if (slots_14828[index].connection_2C != NULL) {
            setError(index, NETWORK_ERROR_SESSION_KICKED, 0, 0x80000000, 1);
        }
    }
}

/* Forwards to the op-code 1 writer. */
void NetworkSessionStable::sendOp1(u32 value)
{
    writeOp1(value);
}

/* Forwards to the op-code 2 writer. */
void NetworkSessionStable::sendOp2()
{
    writeOp2();
}

/* Frames `data` as a user message and sends it to the given targets, refusing one that does not fit. */
void NetworkSessionStable::put(const u8* data, s32 size, s32 channel, s32 count, const s8* targets, u8 limit)
{
    NetworkStreamWriter packet;
    s32 room;

    networkPacket_construct(&packet);
    room = 0x200 - (u16)networkPacket_getFrameOverhead();
    if (room - (u16)networkPacket_getMessageOverhead() < size) {
        setError((s8)ownIndex_14826, NETWORK_ERROR_PUT_TOO_BIG, 0, 0x80000000, 3);
        getNetworkLogger()->warn_10("NetworkSessionStable::put: data too big -> %d\n", size);
        networkStreamWriter_dtor(&packet, -1);
        return;
    }
    networkPacket_attach(&packet, sendBuffer_0D, 0x200);
    networkPacket_begin(&packet, 1);
    if (writeBytes(&packet, data, size) < 0) {
        setError((s8)ownIndex_14826, NETWORK_ERROR_PUT_OVERFLOW, 0, 0x80000000, 3);
        getNetworkLogger()->warn_10("NetworkSessionStable::put: data overflow -> %d\n", size);
        networkStreamWriter_dtor(&packet, -1);
        return;
    }
    writeSize(&packet, (u16)size);
    sendStream(&packet, channel, count, targets, limit);
    networkStreamWriter_dtor(&packet, -1);
}

/* Queues a length-prefixed record for the slot's next flush. */
void NetworkSessionStable::post(const u8* data, s32 size, s8 channel, s8 index)
{
    NetworkSessionSlot* slot;
    u16 length;

    if (size > 0 && data != NULL && isConnected(index) != 0 && channel >= 0 && 1 > channel && size + 2 <= 0x400
        && (networkSessionNotifyValue != 0 || index == getUsableSlot(index))) {
        slot = &slots_14828[index];
        if (size + slot->queueUsed_518[channel] + 2 > 0x400) {
            slot->queueUsed_518[channel] = 0;
        }
        length = getNetworkLogger()->encode_4C((u16)size);
        memcpy(slot->queue_118 + (channel << 10) + slot->queueUsed_518[channel], &length, 2);
        slot->queueUsed_518[channel] = slot->queueUsed_518[channel] + 2;
        memcpy(slot->queue_118 + (channel << 10) + slot->queueUsed_518[channel], data, size);
        slot->queueUsed_518[channel] = slot->queueUsed_518[channel] + size;
    }
}

/* Delivers the user messages every slot received. */
void NetworkSessionStable::receiveAll()
{
    s8 index;

    for (index = 0; index < 4; index++) {
        execControl(index, 1);
    }
}

/* Drops the control messages every slot received. */
void NetworkSessionStable::discardAll()
{
    s8 index;

    for (index = 0; index < 4; index++) {
        discardControlMessages(index);
    }
}

/* True when the slot has a connection that was reported up. */
s32 NetworkSessionStable::isConnected(s8 index)
{
    if (index < 0 || 4 <= index) {
        return 0;
    }
    if (slots_14828[index].connection_2C == NULL) {
        return 0;
    }
    return slots_14828[index].established_18;
}

/* Connection events and control messages                                                     */

/* Handles an event a slot's connection reports: an error, a relay or a leave, a shutdown, the establish
   handshake, or a frame that is queued on the slot it belongs to (or forwarded through the slot that
   reaches its target). */
void NetworkSessionStable::setNetworkConnectionEvent(s32 event, s8 index, u32 arg, s32 size, const u8* data)
{
    NetworkUnitPacket packet;
    NetworkSmallObject address;
    NetworkStreamWriterDefault reader;
    NetworkSessionSlot* slot;
    u16 sequenceTop;
    u16 sequenceLow;
    u8 channel;
    s8 i;
    s8 usable;
    u32 result;
    NetworkConnectionEventRecord record;

    networkSmallObject_construct(&address);
    networkStreamWriter_constructDefault(&reader);
    switch (event) {
    case 2:
        markLeft(index);
        getNetworkLogger()->log_14("NetworkSessionStable::setNetworkConnectionEvent: [%d] error. 0x%08x 0x%08x 0x%08x\n", index,
                                   ((const NetworkPeerErrorRecord*)data)->source, ((const NetworkPeerErrorRecord*)data)->argument,
                                   ((const NetworkPeerErrorRecord*)data)->code);
        break;
    case 4:
        markLeft(index);
        getNetworkLogger()->log_14("NetworkSessionStable::setNetworkConnectionEvent: [%d] is relay.\n", index);
        break;
    case 5:
        setError(index, NETWORK_ERROR_PEER_LEFT, 0, 0x80000000, 2);
        getNetworkLogger()->log_14("NetworkSessionStable::setNetworkConnectionEvent: [%d] is leave.\n", index);
        break;
    case 6:
        if (slots_14828[index].connection_2C == 0) {
            getNetworkLogger()->warn_10("NetworkSessionStable::setNetworkConnectionEvent: EVENT_SHUTDOWN [%d].mpObject is NULL.\n", index);
        } else {
            slots_14828[index].shutdown_1C = 1;
        }
        break;
    case 1:
        slot = &slots_14828[index];
        if (slot->connection_2C == 0) {
            getNetworkLogger()->warn_10("NetworkSessionStable::setNetworkConnectionEvent: EVENT_ESTABLISH [%d].mpObject is NULL.\n", index);
        } else {
            packet.bind((u8*)data, size);
            ((NetworkByteStream*)&packet)->forwardRecord((NetworkStreamSink*)&address);
            if (networkSmallObject_isEqual(&address, &slot->address_30.object_00) == 0) {
                markLeft(index);
                getNetworkLogger()->log_14("NetworkSessionStable::setNetworkConnectionEvent: [%d] auth error.\n", index);
            } else {
                ((NetworkByteStream*)&packet)->takeU32(&slot->nonce_50);
                ((NetworkByteStream*)&packet)->readLength(&sequenceTop);
                ((NetworkByteStream*)&packet)->readLength(&sequenceLow);
                if (slot->receiveStarted_B4 == 0) {
                    slot->receiveStarted_B4 = 1;
                    networkStreamQueue_setSequence(&slot->queues_54.receive_30[0], sequenceTop);
                    networkStreamQueue_setSequence(&slot->queues_54.receive_30[1], sequenceLow);
                }
                getNetworkLogger()->signal_0C(3, "NetworkSessionStable::setNetworkConnectionEvent: [%d] nonce:0x%08x sqntop:0x%04x sqnlow:0x%04x\n",
                                              index, slot->nonce_50,
                                              (u16)networkStreamWriter_size(&slot->queues_54.receive_30[0]),
                                              (u16)networkStreamWriter_size(&slot->queues_54.receive_30[1]));
                slot->authenticated_19 = 1;
                if (slot->established_18 == 0) {
                    slot->established_18 = 1;
                    callback_04(1, index, 0, 0, NULL, user_08);
                }
            }
        }
        break;
    case 3:
        networkStreamReader_attach(&reader, ((const NetworkConnectionFrame*)data)->block_04, ((const NetworkConnectionFrame*)data)->size_00);
        channel = networkPacket_getChannel(&reader);
        if (nonce_16CD8 == networkPacket_getSessionNonce(&reader)) {
            for (i = 0; i < 4; i++) {
                if (slots_14828[i].nonce_50 == networkPacket_getSourceNonce(&reader)) {
                    slot = &slots_14828[i];
                    result = 0;
                    if (networkPacket_isHandshake(&reader) != 0) {
                        if (slot->receiveStarted_B4 == 0) {
                            slot->receiveStarted_B4 = 1;
                            networkStreamQueue_setSequence(&slot->queues_54.receive_30[0], networkPacket_getSequenceA(&reader));
                            networkStreamQueue_setSequence(&slot->queues_54.receive_30[1], networkPacket_getSequenceB(&reader));
                        }
                        if (slot->authenticated_19 != 0) {
                            markLeft(i);
                        }
                        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::setNetworkConnectionEvent: [%d] oob sqn received. sqntop:0x%04x sqnlow:0x%04x\n",
                                                      i, (u16)networkStreamWriter_size(&slot->queues_54.receive_30[0]),
                                                      (u16)networkStreamWriter_size(&slot->queues_54.receive_30[1]));
                    } else if (slot->receiveStarted_B4 != 0) {
                        if (channel == 0) {
                            result = putTopPacket(&slot->queues_54.receive_30[channel], &reader);
                            networkStreamQueue_acknowledge(&slot->queues_54.send_00[channel], &reader);
                        } else if (channel == 1) {
                            result = networkStreamQueue_putNextPacket(&slot->queues_54.receive_30[channel], &reader);
                            if ((s32)result < 0) {
                                result = 0;
                            }
                            if (isConnected(i) != 0) {
                                record.length_00 = ((const NetworkConnectionFrame*)data)->record_08.length_00;
                                record.bytes_04 = ((const NetworkConnectionFrame*)data)->record_08.bytes_04;
                                record.kind_08 = ((const NetworkConnectionFrame*)data)->record_08.kind_08;
                                callback_04(5, i, 0, 1, &record, user_08);
                            }
                        }
                    }
                    if ((s32)result < 4 && result > 2) {
                        if (result != 3) {
                            setError(i, (NetworkPeerErrorSource)result, 0, 0x80000000, 1);
                        } else {
                            slot->resend_BC = 1;
                        }
                    }
                    break;
                }
            }
        } else if (networkPacket_getSessionNonce(&reader) != 0) {
            for (i = 0; i < 4; i++) {
                if (slots_14828[i].nonce_50 == networkPacket_getSessionNonce(&reader)) {
                    usable = getUsableSlot(i);
                    if (usable >= 0) {
                        networkStreamWriter_commit(&reader);
                        networkStreamWriter_bytes(&reader);
                        slot = &slots_14828[usable];
                        networkStreamWriter_attach(slot->connection_2C, &reader);
                        networkStreamWriter_reserve(slot->connection_2C, ((const NetworkConnectionFrame*)data)->record_08.bytes_04,
                                                    ((const NetworkConnectionFrame*)data)->record_08.length_00,
                                                    (s8)((const NetworkConnectionFrame*)data)->record_08.kind_08);
                    }
                    break;
                }
            }
        }
        break;
    }
    networkStreamWriterDefault_dtor(&reader, -1);
    NetworkSmallObjectSink::destroy(&address);
}

#pragma dont_inline on

/* Stores the limits in the session as well as in the globals. */
void NetworkSessionStable::setLimits(u32 maxHosts, u32 maxSubhosts)
{
    NetworkSessionBase::setLimits(maxHosts, maxSubhosts);
    maxHosts_16CE8 = networkSessionMaxHosts;
    maxSubhosts_16CEC = networkSessionMaxSubhosts;
}

/* Stores the host timeout in the session and restarts its clock. */
void NetworkSessionStable::setHostTimeout(f32 seconds)
{
    NetworkSessionBase::setHostTimeout(seconds);
    hostTimeout_16CF0 = networkSessionHostTimeout;
    hostSeen_16CE0 = getNetworkLogger()->getTime_60();
}

/* Stores the subhost timeout in the session. */
void NetworkSessionStable::setSubhostTimeout(f32 seconds)
{
    NetworkSessionBase::setSubhostTimeout(seconds);
    subhostTimeout_16CF4 = networkSessionSubhostTimeout;
}

/* Stores the rate window and step in the session. */
void NetworkSessionStable::setRate(s32 count, s32 divisor)
{
    NetworkSessionBase::setRate(count, divisor);
    rateWindow_16CFC = networkSessionRateWindow;
    rateStep_16CF8 = networkSessionRateStep;
}

#pragma dont_inline off

/* Hands the interval to every connection. */
void NetworkSessionStable::setConnectionInterval(f32 seconds)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        NetworkConnectionStable* connection = slots_14828[index].connection_2C;

        if (connection != NULL) {
            connection->setInterval(seconds);
        }
    }
}

/* Hands the timeout to every connection. */
void NetworkSessionStable::setConnectionTimeout(f32 seconds)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        NetworkConnectionStable* connection = slots_14828[index].connection_2C;

        if (connection != NULL) {
            connection->setTimeout(seconds);
        }
    }
}

/* Hands the limit to every connection. */
void NetworkSessionStable::setConnectionLimit(f32 seconds)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        NetworkConnectionStable* connection = slots_14828[index].connection_2C;

        if (connection != NULL) {
            connection->setLimit(seconds);
        }
    }
}

/* Returns the slot index the session owns.  Declared `s32` on purpose: the caller narrows the
   result with `extsb`, which MWCC only emits for a callee declared wider than its own `s8`. */
s32 NetworkSessionStable::getOwnIndex()
{
    return (s8)ownIndex_14826;
}

/* Stores the host slot index. */
void NetworkSessionStable::setHostIndex(s8 index)
{
    hostIndex_14824 = index;
}

/* Stores the subhost slot index. */
void NetworkSessionStable::setSubhostIndex(s8 index)
{
    subhostIndex_14825 = index;
}

/* Forgets the subhost and its clock. */
void NetworkSessionStable::clearSubhostIndex()
{
    subhostIndex_14825 = -1;
    subhostSeen_16CE4 = networkSessionZero;
}

/* The slot a connected peer is reached through, -1 when it is direct or not connected. */
s8 NetworkSessionStable::getRelayIndex(s8 index)
{
    if (index < 0 || 4 <= index) {
        return -1;
    }
    if (slots_14828[index].connection_2C == NULL) {
        return -1;
    }
    if (isConnected(index) != 0) {
        return slots_14828[index].relayIndex_10;
    }
    return -1;
}

/* Stores the first user flag. */
void NetworkSessionStable::setUserFlagA(u8 value)
{
    userFlagA_14810 = value;
}

/* Returns the first user flag. */
u8 NetworkSessionStable::getUserFlagA()
{
    return userFlagA_14810;
}

/* Stores the second user flag. */
void NetworkSessionStable::setUserFlagB(u8 value)
{
    userFlagB_14811 = value;
}

/* Returns the second user flag. */
u8 NetworkSessionStable::getUserFlagB()
{
    return userFlagB_14811;
}

/* Marks the session as joined. */
void NetworkSessionStable::markJoined()
{
    joined_16D00 = 1;
}

/* The round trip to a slot: its connection's, plus the delay of the relay it is reached through. */
f32 NetworkSessionStable::getRoundTrip(s8 index)
{
    s8 usable;

    if (isConnected(index) == 0) {
        return networkSessionDefaultDelay;
    }
    usable = getUsableSlot(index);
    if (usable < 0) {
        return networkSessionDefaultDelay;
    }
    if (index == usable) {
        return slots_14828[index].connection_2C->getRoundTripTime();
    }
    return slots_14828[index].relayDelay_14 + slots_14828[usable].connection_2C->getRoundTripTime();
}

/* The bytes a slot's rate governor lets through per window. */
s32 NetworkSessionStable::getBandwidth(s8 index)
{
    f32 step;

    if (isConnected(index) == 0) {
        return 0;
    }
    if (getUsableSlot(index) < 0) {
        return 0x400;
    }
    step = slots_14828[index].rateTarget_CC;
    if (step <= networkRateUpStep) {
        step = networkRateUpStep;
    }
    return (s32)((networkRateMax / step) * (f32)slots_14828[index].byteLimit_D0);
}

/* Hands a value to a connected slot's connection. */
void NetworkSessionStable::setCongestion(s8 index, f32 value)
{
    if (isConnected(index) != 0) {
        NetworkConnectionStable* connection = slots_14828[index].connection_2C;

        connection->setValue(value);
    }
}

/* Reads the value back from a connected slot's connection. */
f32 NetworkSessionStable::getCongestion(s8 index)
{
    if (isConnected(index) != 0) {
        NetworkConnectionStable* connection = slots_14828[index].connection_2C;

        return connection->getValue();
    }
    return networkSessionZero;
}

/* The room left in a connected slot's send queue. */
s32 NetworkSessionStable::getFreeSpace(s8 index)
{
    NetworkStreamWriter packet;
    s32 room;

    if (isConnected(index) == 0) {
        return 0;
    }
    networkPacket_construct(&packet);
    networkStreamQueue_fillPacket(&slots_14828[index].queues_54.send_00[0], &packet);
    room = 0x2000 - (u16)networkPacket_getLength(&packet);
    networkStreamWriter_dtor(&packet, -1);
    return room;
}

#pragma dont_inline on

/* Marks a connected slot as having left. */
void NetworkSessionStable::markLeft(s8 index)
{
    if (index >= 0) {
        if (4 <= index) {
            return;
        }
        if (slots_14828[index].connection_2C != NULL) {
            slots_14828[index].left_1A = 1;
            slots_14828[index].authenticated_19 = 0;
        }
    }
}

#pragma dont_inline off

/* Records the first error of a connected slot and the kind of close it asks for. */
/* untyped: caller-owned payload - the error source is a constant or a table address */
void NetworkSessionStable::setError(s8 index, NetworkPeerErrorSource source, u32 argument, u32 code, s32 kind)
{
    NetworkSessionSlot* slot;

    if (index >= 0) {
        if (index >= 4) {
            return;
        }
        slot = &slots_14828[index];
        if (slot->connection_2C != NULL) {
            if (slot->closeState_08 == 0) {
                if ((u32)(kind - 1) <= 1) {
                    slot->closeState_08 = kind;
                } else {
                    slot->closeState_08 = 3;
                }
            }
            slot = &slots_14828[index];
            if (slot->error_20.source == NULL) {
                slot->error_20.source = (const void*)source;
                slot->error_20.argument = argument;
                slot->error_20.code = code;
            }
        }
    }
}

/* Flushes the channel's send queue of a slot into frames and hands them to the connection; returns 1 when
   a frame went out. */
s32 NetworkSessionStable::send(s8 index, s32 channel, s32 force)
{
    NetworkStreamWriterDefault writer;
    NetworkStreamWriter packet;
    NetworkConnectionStable* connection;
    NetworkStreamQueue* queue;
    s32 result;
    s32 sentTotal;
    s32 sent;
    s32 total;
    s32 congestion;
    s8 usable;
    f32 rate;
    f32 interval;
    f32 limit;

    networkStreamWriter_constructDefault(&writer);
    networkPacket_construct(&packet);
    result = 0;
    interval = networkSessionZero;
    rate = interval;
    limit = interval;
    usable = getUsableSlot(index);
    if (usable < 0) {
        networkStreamWriter_dtor(&packet, -1);
        networkStreamWriterDefault_dtor(&writer, -1);
        return 0;
    }
    connection = slots_14828[usable].connection_2C;
    if (channel == 0) {
        interval = slots_14828[usable].rateTarget_CC;
        rate = slots_14828[usable].rate_C8;
        limit = (f32)slots_14828[usable].byteLimit_D0;
    }
    if (time_16CDC <= interval + slots_14828[index].lastSend_C0[channel]) {
        networkStreamWriter_dtor(&packet, -1);
        networkStreamWriterDefault_dtor(&writer, -1);
        return 0;
    }
    sentTotal = 0;
    queue = &slots_14828[index].queues_54.send_00[channel];
    do {
        networkStreamWriter_putBytes(&writer, sendBuffer_0D, connection->getMaxPacketSize());
        networkStreamWriter_flush(&writer);
        networkStreamQueue_fillPacket(queue, &packet);
        sent = 0;
        total = networkStreamWriter_size(queue);
        while (networkPacket_hasMessage(&packet) != 0) {
            if (networkPacket_getTimestamp(&packet) < networkSessionZero
                && time_16CDC < networkSessionNever * networkPacket_getTimestamp(&packet)) {
                break;
            }
            if (rate + networkPacket_getTimestamp(&packet) <= time_16CDC) {
                break;
            }
            total += networkPacket_getMessageSize(&packet, 0xBF);
            networkPacket_nextMessage(&packet);
        }
        while (networkPacket_hasMessage(&packet) != 0) {
            if (networkPacket_getTimestamp(&packet) < networkSessionZero
                && time_16CDC < networkSessionNever * networkPacket_getTimestamp(&packet)) {
                break;
            }
            {
                u16 size = networkPacket_getMessageSize(&packet, 0xBF);
                s32 put;

                if ((u16)networkPacket_getFrameOverhead() + size > 0x200) {
                    u16 fullSize = networkPacket_getMessageSize(&packet, 0xFF);

                    getNetworkLogger()->warn_10("NetworkSessionStable::send:[%d] unit size over -> %d\n", index,
                                                (u16)networkPacket_getFrameOverhead() + fullSize);
                }
                put = networkStreamWriter_putPacket(&writer, &packet, 0xBF);
                if (put < 0) {
                    break;
                }
                sent += put;
            }
            networkPacket_setTimestamp(&packet, time_16CDC);
            networkPacket_nextMessage(&packet);
        }
        if (sent == 0 && force == 0) {
            networkStreamWriter_dtor(&packet, -1);
            networkStreamWriterDefault_dtor(&writer, -1);
            return result;
        }
        force = 0;
        networkStreamWriter_setMode(&writer, (u16)sent);
        networkStreamWriter_putU16(&writer, (u16)total);
        networkStreamWriter_putU16b(&writer, (u16)networkStreamWriter_size(&slots_14828[index].queues_54.receive_30[channel]));
        networkStreamWriter_putU32(&writer, nonce_16CD8);
        networkStreamWriter_putU32b(&writer, slots_14828[index].nonce_50);
        networkStreamWriter_enable1(&writer, 1);
        networkStreamWriter_enable2(&writer, (u8)channel);
        networkStreamWriter_commit(&writer);
        networkStreamWriter_bytes(&writer);
        congestion = 0;
        if (channel == 1) {
            congestion = slots_14828[index].congestionCount_110 + connection->getCongestion();
        }
        if (congestion < 1) {
            congestion = 1;
        }
        if ((u8)rand() < (u32)(0x200 / congestion)) {
            networkStreamWriter_attach(connection, &writer);
            result = 1;
        }
        sentTotal += sent;
        slots_14828[index].lastSend_C0[channel] = time_16CDC;
        if (channel == 1 && sent > 0) {
            networkStreamQueue_discard(queue, sent, 0xBF);
            networkStreamQueue_setSequence(queue, (u16)(total + sent));
        }
    } while (channel == 0 && (f32)sentTotal < limit);
    networkStreamWriter_dtor(&packet, -1);
    networkStreamWriterDefault_dtor(&writer, -1);
    return result;
}

/* Delivers (or executes) the messages queued in a slot's two receive queues: user data goes to the
   callback, control messages to `execControlOne`. */
void NetworkSessionStable::execControl(s8 index, s32 user)
{
    NetworkStreamWriter packet;
    NetworkStreamQueue* queue;
    s32 i;
    s32 result;

    networkPacket_construct(&packet);
    if (user != 0 && isConnected(index) == 0) {
        networkStreamWriter_dtor(&packet, -1);
        return;
    }
    i = 0;
    queue = &slots_14828[index].queues_54.receive_30[0];
    do {
        networkStreamQueue_rewind(queue);
        while (networkStreamQueue_hasMessage(queue) != 0) {
            networkStreamQueue_peekPacket(queue, &packet);
            if (user != 0 && networkPacket_isUserData(&packet) != 0) {
                networkPacket_takeBytes(&packet, receiveBuffer_410, networkPacket_getPayloadSize(&packet));
                callback_04(2, index, 0, networkPacket_getPayloadSize(&packet), receiveBuffer_410, user_08);
                networkStreamQueue_removeMessage(queue);
                continue;
            }
            if (user == 0 && networkPacket_isUserData(&packet) == 0) {
                result = execControlOne(index, &packet);
                if (result < 0) {
                    setError(index, NETWORK_ERROR_SESSION_CONTROL, -result, 0x80000000, 1);
                    networkStreamWriter_dtor(&packet, -1);
                    return;
                }
                networkStreamQueue_removeMessage(queue);
                continue;
            }
            networkStreamQueue_skipMessage(queue);
        }
        queue++;
        i++;
    } while (i < 2);
    networkStreamWriter_dtor(&packet, -1);
}

/* Drops the control messages waiting in a slot's two receive queues and keeps the user ones. */
void NetworkSessionStable::discardControlMessages(s8 index)
{
    NetworkStreamWriter packet;
    NetworkStreamQueue* queue;
    s32 i;

    networkPacket_construct(&packet);
    i = 0;
    queue = &slots_14828[index].queues_54.receive_30[0];
    do {
        networkStreamQueue_rewind(queue);
        while (networkStreamQueue_hasMessage(queue) != 0) {
            networkStreamQueue_peekPacket(queue, &packet);
            if (networkPacket_isUserData(&packet) != 0) {
                networkStreamQueue_removeMessage(queue);
            } else {
                networkStreamQueue_skipMessage(queue);
            }
        }
        queue++;
        i++;
    } while (i < 2);
    networkStreamWriter_dtor(&packet, -1);
}

/* Executes one control message of a slot: sleep and wake notices, the sync-drop nonce list, relayed user
   data, drop and leave notices, the relay authentication handshake and the relay route search.  Returns 0,
   or the negated message type when the message is refused. */
s32 NetworkSessionStable::execControlOne(s8 index, NetworkStreamWriter* packet)
{
    NetworkSmallObject address;
    NetworkSessionSlot* slot;
    u8 type;
    u32 value;
    u32 other;
    s32 found;
    s32 result;
    f32 delay;

    networkSmallObject_construct(&address);
    slot = &slots_14828[index];
    networkPacket_takeByte(packet, &type);
    switch (type) {
    case 1:
        networkPacket_takeU32(packet, &value);
        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne:[%d] status changed -> SLEEP(%dsec)\n", index, value);
        slot->sleeping_100 = 1;
        slot->sleepStart_104 = time_16CDC;
        slot->sleepSeconds_0C = value;
        break;
    case 2:
        slot->sleeping_100 = 0;
        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne:[%d] status changed -> ESTABLISH(AWAKE)\n", index);
        break;
    case 3:
        networkPacket_takeU32(packet, &other);
        if (networkSessionNonce_isValid(other) == 0) {
            result = -(s32)type;
            NetworkSmallObjectSink::destroy(&address);
            return result;
        }
        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne:[%d] sync drop control -> 0x%08x\n", index, other);
        found = 0;
        if (other != nonceDrop_14814[0]) {
            found = 1;
            if (other != nonceDrop_14814[1]) {
                found = 2;
                if (other != nonceDrop_14814[2]) {
                    found = 3;
                    if (other != nonceDrop_14814[3]) {
                        found = 4;
                    }
                }
            }
        }
        if (found == 4) {
            memmove(&nonceDrop_14814[1], &nonceDrop_14814[0], 0xC);
            nonceDrop_14814[0] = other;
        }
        break;
    case 4:
        networkPacket_takeBytes(packet, receiveBuffer_410, networkPacket_getPayloadSize(packet) - 1);
        callback_04(4, index, 0, networkPacket_getPayloadSize(packet) - 1, receiveBuffer_410, user_08);
        break;
    case 5:
        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne:[%d] said i'm drop. sync_close me.\n", index);
        setError((s8)ownIndex_14826, NETWORK_ERROR_SESSION_DROPPED, 0, 0x80000000, 3);
        break;
    case 6:
        break;
    case 7:
        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne:[%d] is leave.\n", index);
        setError(index, NETWORK_ERROR_PEER_LEFT, 0, 0x80000000, 2);
        break;
    case 8:
    case 9:
        networkPacket_takeU32(packet, &other);
        if (networkSessionNonce_isValid(other) == 0) {
            getNetworkLogger()->warn_10("NetworkSessionStable::execControlOne:[%d] relay auth error -> he think:0x%08x\n", index, other);
            result = -(s32)type;
            NetworkSmallObjectSink::destroy(&address);
            return result;
        }
        if (nonce_16CD8 != other) {
            getNetworkLogger()->warn_10("NetworkSessionStable::execControlOne:[%d] relay auth error -> mine:0x%08x <=> he think:0x%08x\n",
                                        index, nonce_16CD8, other);
            result = -(s32)type;
            NetworkSmallObjectSink::destroy(&address);
            return result;
        }
        networkPacket_takeU32(packet, &other);
        if (networkSessionNonce_isValid(other) == 0) {
            getNetworkLogger()->warn_10("NetworkSessionStable::execControlOne:[%d] relay auth error -> his:0x%08x\n", index, other);
            result = -(s32)type;
            NetworkSmallObjectSink::destroy(&address);
            return result;
        }
        if (networkSessionNonce_isValid(slot->nonce_50) != 0) {
            if (slot->nonce_50 != other) {
                getNetworkLogger()->warn_10("NetworkSessionStable::execControlOne:[%d] relay auth error -> i think:0x%08x <=> his:0x%08x\n",
                                            index, slot->nonce_50, other);
                result = -(s32)type;
                NetworkSmallObjectSink::destroy(&address);
                return result;
            }
            if (type == 8 && slot->relayIndex_10 < 0) {
                if (slot->authenticated_19 != 0) {
                    markLeft(index);
                }
            } else {
                if (type == 8) {
                    writeOp8or9(1, index);
                }
                slot->relayAck_1B = 1;
                if (slot->established_18 == 0) {
                    slot->established_18 = 1;
                    callback_04(1, index, 0, 0, NULL, user_08);
                }
            }
        }
        break;
    case 10: {
        networkPacket_takeRecord(packet, &address);
        for (found = 0; found < 4; found++) {
            if (networkSmallObject_isEqual(&slots_14828[found].address_30.object_00, &address) != 0) {
                break;
            }
        }
        if (found == 4) {
            writeOp11(index, &address, 0, 0, networkSessionZero);
        } else {
            delay = networkSessionDefaultDelay;
            if (found == (s8)ownIndex_14826) {
                type = 0;
            } else if (slots_14828[found].authenticated_19 != 0) {
                type = 2;
                if (slots_14828[found].connection_2C != 0) {
                    delay = slots_14828[found].connection_2C->getRoundTripTime();
                }
            } else {
                type = 0;
            }
            writeOp11(index, &address, slots_14828[found].nonce_50, (s8)type, delay);
        }
        break;
    }
    case 11: {
        u32 delayMs;

        networkPacket_takeRecord(packet, &address);
        networkPacket_takeU32(packet, &other);
        networkPacket_takeU32(packet, &delayMs);
        networkPacket_takeByte(packet, &type);
        if (slot->authenticated_19 != 0) {
            for (found = 0; found < 4; found++) {
                if (networkSmallObject_isEqual(&slots_14828[found].address_30.object_00, &address) != 0) {
                    break;
                }
            }
            if (found < 4) {
                if (networkSessionNonce_isValid(other) == 0) {
                    result = -(s32)type;
                    NetworkSmallObjectSink::destroy(&address);
                    return result;
                }
                if (networkSessionNonce_isValid(slots_14828[found].nonce_50) == 0) {
                    slots_14828[found].nonce_50 = other;
                    getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne: relay route [%d] nonce learning [%d] 0x%08x:0x%08x\n",
                                                  index, found, slots_14828[found].nonce_50, other);
                } else if (other != slots_14828[found].nonce_50) {
                    getNetworkLogger()->log_14("NetworkSessionStable::execControlOne: relay route [%d] nonce is different 0x%08x:0x%08x\n",
                                               index, slots_14828[found].nonce_50, other);
                }
                if ((s32)type == 2) {
                    if (slot->connection_2C->getCongestion() < slots_14828[found].sequence_F4) {
                        slots_14828[found].sequence_F4 = slot->connection_2C->getCongestion();
                        slots_14828[found].relayIndex_10 = index;
                        slots_14828[found].relayDelay_14 = (f32)delayMs / networkMillisecondsPerSecond;
                        getNetworkLogger()->signal_0C(3, "NetworkSessionStable::execControlOne: relay route (SELF)[%d](0x%08x) -> (RELAY)[%d](0x%08x) -> (FORWARD)[%d](0x%08x)\n",
                                                      (s8)ownIndex_14826, nonce_16CD8, index, slot->nonce_50, found,
                                                      slots_14828[found].nonce_50);
                    }
                }
            }
        }
        break;
    }
    default:
        result = -(s32)type;
        NetworkSmallObjectSink::destroy(&address);
        return result;
    }
    NetworkSmallObjectSink::destroy(&address);
    return 0;
}

/* Sending                                                                                    */

/* Queues the packet on the channel's send queue of every slot the target list names. */
void NetworkSessionStable::sendStream(NetworkStreamWriter* stream, s32 channel, s32 count, const s8* targets, u8 limit)
{
    u8 selected[4];
    s8 index;
    s32 n;
    const s8* target;
    s32 appended;

    if (channel >= 0) {
        if (channel >= 2) {
            return;
        }
        for (index = 0; index < 4; index++) {
            selected[index] = 0;
            if (slots_14828[index].connection_2C != 0 && slots_14828[index].receiveStarted_B4 != 0
                && (networkPacket_isUserData(stream) == 0 || slots_14828[index].established_18 != 0)) {
                target = targets;
                for (n = count; n > 0; n--) {
                    s8 value = *target;

                    if (value == -1) {
                        selected[index] = 1;
                        break;
                    }
                    if (value == -2) {
                        if (index != (s8)ownIndex_14826) {
                            selected[index] = 1;
                            break;
                        }
                    } else if (index == value) {
                        selected[index] = 1;
                        break;
                    }
                    target++;
                }
            }
        }
        for (index = 0; index < 4; index++) {
            if (selected[index] != 0
                && (channel != 1 || limit >= 0xFE
                    || !((f32)limit < (networkSessionPriorityScale
                                       * ((slots_14828[index].congestion_10C - (f32)(u16)networkPacket_getFrameOverhead())
                                          - (f32)(u16)networkPacket_getMessageOverhead()))
                                          / networkSessionPriorityRange))) {
                f32 expiry = slots_14828[index].expiry_B8;

                if (networkSessionZero < expiry) {
                    networkPacket_setTimestamp(stream, networkSessionNever * (time_16CDC + expiry));
                } else {
                    networkPacket_setTimestamp(stream, networkSessionZero);
                }
                appended = networkStreamQueue_append(&slots_14828[index].queues_54.send_00[channel], stream, 0xFF);
                if (channel == 0) {
                    if (appended < 0) {
                        getNetworkLogger()->log_14("NetworkSessionStable[%d] put send pool over (0x%x)\n", index, appended);
                        setError(index, NETWORK_ERROR_PUT_OVERFLOW, appended, 0x80000000, 1);
                    } else {
                        slots_14828[index].measureSend_114 = time_16CDC;
                    }
                }
            }
        }
    }
}

/* Op-code packet writers                                                                     */

/* Frames op code 1 with `value` and sends it to every slot. */
void NetworkSessionStable::writeOp1(u32 value)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 n;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    n = (u16)writeByte(&stream, 1);
    n += (u16)writeUInt(&stream, value);
    writeSize(&stream, (u16)n);
    term = -1;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 2 and sends it to every slot. */
void NetworkSessionStable::writeOp2()
{
    NetworkStreamWriter stream;
    s8 term;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    writeSize(&stream, writeByte(&stream, 2));
    term = -1;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 6 and sends it to the slot `value`. */
void NetworkSessionStable::writeOp6(s8 value)
{
    NetworkStreamWriter stream;
    s8 term;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    writeSize(&stream, writeByte(&stream, 6));
    term = value;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 10 with the slot's address record and sends it to every slot but the own one. */
void NetworkSessionStable::writeOp10(s8 index)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 n;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    n = (u16)writeByte(&stream, 10);
    n += (u16)networkPacket_writeRecord(&stream, &slots_14828[index].address_30.object_00);
    writeSize(&stream, (u16)n);
    term = -2;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 11 with an address record, the nonce, the delay and a kind, and sends it to the slot `index`. */
void NetworkSessionStable::writeOp11(s8 index, const NetworkSmallObject* address, u32 nonce, s8 kind, f32 delay)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 scaled;
    u32 n;

    scaled = 0;
    networkPacket_construct(&stream);
    scaled = (u32)(networkMillisecondsPerSecond * delay);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    n = (u16)writeByte(&stream, 11);
    n += (u16)networkPacket_writeRecord(&stream, address);
    n += (u16)writeUInt(&stream, nonce);
    n += (u16)writeUInt(&stream, scaled);
    n += (u16)writeByte(&stream, (u8)kind);
    writeSize(&stream, (u16)n);
    term = index;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 8 (or 9 with `hasExtra`) with the slot's nonce and the session's, sends it to the slot. */
void NetworkSessionStable::writeOp8or9(u32 hasExtra, s8 index)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 n;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    if (hasExtra != 0) {
        n = (u16)writeByte(&stream, 9);
    } else {
        n = (u16)writeByte(&stream, 8);
    }
    n += (u16)writeUInt(&stream, slots_14828[index].nonce_50);
    n += (u16)writeUInt(&stream, nonce_16CD8);
    writeSize(&stream, (u16)n);
    term = index;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Frames op code 4 around `data` and sends it to the slot `index`. */
/* untyped: byte range (the payload bytes) */
void NetworkSessionStable::sendOp4(const void* data, u32 length, s8 index)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 n;

    networkPacket_construct(&stream);
    networkPacket_attach(&stream, sendBuffer_0D, 0x400);
    networkPacket_begin(&stream, 0);
    n = (u16)writeByte(&stream, 4);
    n += (u16)writeBytes(&stream, data, length);
    writeSize(&stream, (u16)n);
    term = index;
    sendStream(&stream, 0, 1, &term, 0xFF);
    networkStreamWriter_dtor(&stream, -1);
}

/* Rate governor                                                                              */

/* Sets the slot's send rate to the larger of the two round-trip based estimates, clamped. */
void NetworkSessionStable::updateRate(s8 index)
{
    f32 a;
    f32 b;
    f32 r;

    if (getUsableSlot(index) < 0) {
        return;
    }
    a = networkRateScale * getRoundTrip(index);
    b = networkRateScale * slots_14828[index].rateTarget_CC;
    r = (a < b) ? b : a;
    r = (networkRateMin < r) ? r : networkRateMin;
    r = (r < networkRateMax) ? r : networkRateMax;
    slots_14828[index].rate_C8 = r;
}

/* Runs once every tick: decays the slot's pack interval towards the floor and reports it. */
void NetworkSessionStable::downPerformance(s8 index)
{
    NetworkSessionSlot* slot;
    s32 step;
    f32 t;
    f32 delta;

    slot = &slots_14828[index];
    t = time_16CDC;
    if (t < networkRateMax + slot->lastLower_E8) {
        return;
    }
    slot->lastLower_E8 = t;
    if (slot->adjusting_E0 == 0) {
        slot->adjusting_E0 = 1;
        slot->rateFloor_D8 = slot->rateTarget_CC;
        slot->byteFloor_DC = slot->byteLimit_D0;
    }
    slot->lastAdjust_D4 = time_16CDC;
    if (slot->rateTarget_CC < rateStep_16CF8) {
        delta = rateStep_16CF8 - slot->rateTarget_CC;
        delta = delta * networkRateUpLerp;
        if (delta < networkRateDownStep) {
            slot->rateTarget_CC = rateStep_16CF8;
        } else {
            slot->rateTarget_CC = slot->rateTarget_CC + delta;
        }
        getNetworkLogger()->signal_0C(1, "NetworkSessionStable::downPerformance:[%d] down -> pack: %fs\n", (u32)index,
                                      slot->rateTarget_CC);
    }
    if (slot->byteLimit_D0 > rateWindow_16CFC) {
        step = (slot->byteLimit_D0 - rateWindow_16CFC) / 2;
        if (step < 16) {
            slot->byteLimit_D0 = rateWindow_16CFC;
        } else {
            slot->byteLimit_D0 = slot->byteLimit_D0 - step;
        }
        getNetworkLogger()->signal_0C(1, "NetworkSessionStable::downPerformance:[%d] down -> byte: %dbyte\n", (u32)index,
                                      slot->byteLimit_D0);
    }
}

/* Runs once every tick: grows the slot's pack interval towards the ceiling and reports it. */
void NetworkSessionStable::upPerformance(s8 index)
{
    NetworkSessionSlot* slot;
    f32 t;

    slot = &slots_14828[index];
    t = time_16CDC;
    if (t < networkRateMax + slot->lastRaise_E4) {
        return;
    }
    slot->lastRaise_E4 = t;
    if (networkRateMax + slot->lastAdjust_D4 < time_16CDC) {
        slot->lastAdjust_D4 = time_16CDC;
        slot->rateFloor_D8 = slot->rateFloor_D8 - networkRateDecay;
        if (slot->rateFloor_D8 < networkRateFloor) {
            slot->rateFloor_D8 = networkRateFloor;
        }
        slot->byteFloor_DC = slot->byteFloor_DC + 4;
        if (slot->byteFloor_DC > 1024) {
            slot->byteFloor_DC = 1024;
        }
    }
    if (slot->adjusting_E0 != 0) {
        slot->adjusting_E0 = 0;
        slot->rateFloor_D8 = slot->rateFloor_D8 + (slot->rateTarget_CC - slot->rateFloor_D8) * networkRateDownLerp;
        slot->byteFloor_DC = slot->byteFloor_DC - (slot->byteFloor_DC - slot->byteLimit_D0) / 4;
    }
    if (slot->rateFloor_D8 < slot->rateTarget_CC) {
        if (slot->rateTarget_CC < networkRateUpStep + slot->rateFloor_D8) {
            slot->rateTarget_CC = slot->rateFloor_D8;
        } else {
            slot->rateTarget_CC = slot->rateTarget_CC - (slot->rateTarget_CC - slot->rateFloor_D8) * networkRateUpLerp;
        }
        getNetworkLogger()->signal_0C(1, "NetworkSessionStable::upPerformance[%d] up -> pack: %fs\n", (u32)index,
                                      slot->rateTarget_CC);
    }
    if (slot->byteLimit_D0 > slot->byteFloor_DC) {
        if (slot->byteFloor_DC - 32 < slot->byteLimit_D0) {
            slot->byteLimit_D0 = slot->byteFloor_DC;
        } else {
            slot->byteLimit_D0 = slot->byteLimit_D0 + (slot->byteFloor_DC - slot->byteLimit_D0) / 2;
        }
        getNetworkLogger()->signal_0C(1, "NetworkSessionStable::upPerformance[%d] up -> byte: %dbyte\n", (u32)index,
                                      slot->byteLimit_D0);
    }
}

/* Packs the slot's current record into the session's stream buffer and hands it to the socket. */
void NetworkSessionStable::moveOutOfBand(s8 index)
{
    NetworkStreamWriterDefault stream;
    NetworkConnectionStable* connection;
    s8 slotIndex;
    s32 size44;
    s32 size5C;

    networkStreamWriter_constructDefault(&stream);
    slotIndex = getUsableSlot(index);
    if (slotIndex < 0) {
        networkStreamWriterDefault_dtor(&stream, -1);
        return;
    }
    connection = slots_14828[slotIndex].connection_2C;
    networkStreamWriter_putBytes(&stream, sendBuffer_0D, 0x400);
    networkStreamWriter_flush(&stream);
    networkStreamWriter_setMode(&stream, 0);
    networkStreamWriter_putU16(&stream, (u16)networkStreamWriter_size(&slots_14828[index].queues_54.send_00[0]));
    networkStreamWriter_putU16b(&stream, (u16)networkStreamWriter_size(&slots_14828[index].queues_54.send_00[1]));
    networkStreamWriter_putU32(&stream, nonce_16CD8);
    networkStreamWriter_putU32b(&stream, slots_14828[index].nonce_50);
    networkStreamWriter_enable1(&stream, 1);
    networkStreamWriter_enable2(&stream, 1);
    networkStreamWriter_enable3(&stream, 1);
    networkStreamWriter_commit(&stream);
    networkStreamWriter_bytes(&stream);
    networkStreamWriter_attach(connection, &stream);
    networkStreamWriter_reserve(connection, 0, 0, 0);
    size5C = networkStreamWriter_size(&slots_14828[index].queues_54.send_00[1]) & 0xFFFF;
    size44 = networkStreamWriter_size(&slots_14828[index].queues_54.send_00[0]) & 0xFFFF;
    getNetworkLogger()->signal_0C(3, "NetworkSessionStable::move: oob sqn send. [%d] -> [%d] -> [%d] sqntop:0x%04x sqnlow:0x%04x\n",
                                  (s8)ownIndex_14826, (u32)slotIndex, (u32)index, (u32)size44, (u32)size5C);
    networkStreamWriterDefault_dtor(&stream, -1);
}

/* Resolves the slot the session should transmit on: the slot itself, else the slot it is reached through. */
s8 NetworkSessionStable::getUsableSlot(s8 index)
{
    NetworkSessionSlot* slot;
    NetworkSessionSlot* via;
    s8 viaIndex;

    if (index < 0 || 4 <= index) {
        return -1;
    }
    slot = &slots_14828[index];
    if (slot->connection_2C == 0) {
        return -1;
    }
    if (slot->authenticated_19 != 0) {
        return index;
    }
    viaIndex = slot->relayIndex_10;
    if (viaIndex < 0) {
        return -1;
    }
    via = &slots_14828[viaIndex];
    if (via->connection_2C == 0) {
        return -1;
    }
    if (via->authenticated_19 != 0) {
        return viaIndex;
    }
    if (via->relayAck_1B != 0) {
        return viaIndex;
    }
    return -1;
}
