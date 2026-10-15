/*
 * Network/NetworkPeerGameSpy.cpp - the GameSpy peer `NetworkPeerGameSpy` (a `NetworkPeerBase` peer with a 0x600-byte send
 *   and a 0x6000-byte receive buffer, bound to a `GameSpyInterfaceThread` slot) and the small `NetworkTimedHandler`.
 * RANGE. .text 0x8041D750-0x8041DF10 (14 functions, `NetworkPeerGameSpy::setContext` .. `NetworkTimedHandler::clear`);
 *   extab 0x8001D100-0x8001D16C; extabindex 0x8003DB54-0x8003DBD8 (its first record names 0x8041D750); .data
 *   0x806036B0-0x80603750 (the put-overflow string, then `__vt__18NetworkPeerGameSpy` and `__vt__19NetworkTimedHandler`).
 *   The left edge is a vtable-to-string seam: `Network/GameSpyInterfaceThread.cpp`'s table 0x806036A0 (padded to
 *   0x806036B0) is followed by this unit's string, an order MWCC cannot emit inside one object (vtables come last).
 * FLAGS. `-O3 -inline noauto -pool off` and file-scope `#pragma peephole off`, as `Network/GameSpyInterfaceThread.cpp`
 *   (configure.py).
 * NAMES. GUESS: the file name, from the class whose members fill the range.  The peer's slot names are
 *   `NetworkPeerBase`'s - GUESS: setContext, send, receive, put, move, reset (the base slot each overrides); `armDrop`/`init`
 *   are GUESSes from the base's +0x20/+0x24 slots.
 * RESIDUALS. `NetworkPeerGameSpy::receive`: retail keeps `payload` in the zero constant's register (r22) and a `clrlwi`
 *   before `cursor += aLen16` that ours drops.  `.data`: the bytes match; the map row `lbl_806036B0` labels the
 *   anonymous put-overflow literal our object names `@N`, and the target's last 4 bytes are the link's alignment pad.
 * SHAPES. The include set is `Network/GameSpyInterfaceThread.cpp`'s (the interface's class and the peer base), so the
 *   declarations reach the compiler in the same order as before the cut.  The logger call re-runs `getNetworkLogger`
 *   at the site (a real virtual call, `lwz r12,0x0(r3)` / `lwz r12,0x14(r12)`).
 */

#include "types.h"
#include "Network/GameSpyInterfaceThread.h"
#include "Network/NetworkPeerGameSpy.h"
#include "Runtime.PPCEABI.H/memcpy.h"    /* memcpy  - owner Runtime.PPCEABI.H/memcpy.c */
#include "Runtime.PPCEABI.H/memset.h"    /* memset  - owner Runtime.PPCEABI.H/memset.c */
#include "unsplit/Runtime.PPCEABI.H.h"   /* memmove - no registered owner */

/* retail keeps the unfused peephole forms here as in the interface's unit */
#pragma peephole off

/* one logging site: fetch the singleton and dispatch through its vtable slot +0x14 */
#define INFO_LOG(...)   do { NetworkLogger* lm = getNetworkLogger(); lm->log_14(__VA_ARGS__); } while (0)

/* Binds the peer to the interface and resets both buffers. */
/* untyped: caller-owned payload - the interface and peer id pair */
void NetworkPeerGameSpy::setContext(const void* context)
{
    const u32* id = (const u32*)context;

    interface_6634 = (GameSpyInterfaceThread*)id[0];
    peer_6638 = id[1];
    field_6630 = interface_6634->registerReceiver(this, peer_6638);
    memset(sendBuffer_14, 0, 0x600);
    memset(recvBuffer_614, 0, 0x6000);
    received_10 = 0;
}

/* Builds and sends a framed peer message from the two optional payloads. */
s32 NetworkPeerGameSpy::send(const u8* a, s32 aLen, const u8* b, s32 bLen, s8 flag)
{
    s8 flagByte;
    u16 aLen16;
    u16 bLen16;
    u8* cursor;
    s32 total;
    s8 slot;

    flagByte = flag;
    slot = interface_6634->getSlotState(peer_6638);
    if (slot < 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0, -1);
        return -1;
    }
    cursor = sendBuffer_14;
    if (a == NULL || aLen <= 0) {
        aLen16 = 0;
        memcpy(cursor, &aLen16, 2);
        cursor += 2;
        total = 2;
    } else {
        {
            NetworkLogger* lm = getNetworkLogger();
            aLen16 = lm->encode_4C((u16)aLen);
        }
        memcpy(cursor, &aLen16, 2);
        cursor += 2;
        memcpy(cursor, a, (u32)(u16)aLen);
        cursor += aLen;
        total = aLen + 2;
    }
    if (b == NULL || bLen <= 0) {
        bLen16 = 0;
        memcpy(cursor, &bLen16, 2);
        total = total + 2;
    } else {
        {
            NetworkLogger* lm = getNetworkLogger();
            bLen16 = lm->encode_4C((u16)(bLen + 1));
        }
        memcpy(cursor, &bLen16, 2);
        memcpy(cursor + 2, &flagByte, 1);
        total = total + 3;
        memcpy(cursor + 3, b, (u32)(u16)bLen);
        total = total + bLen;
    }
    {
        NetworkLogger* lm = getNetworkLogger();
        if (lm->flag_48(aLen16) == 0) {
            NetworkLogger* lm2 = getNetworkLogger();
            if (lm2->flag_48(bLen16) == 0) {
                return 0;
            }
        }
    }
    slot = interface_6634->findSlot(peer_6638);
    if (slot < 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0, -2);
        return -1;
    }
    if (interface_6634->sendUnreliable((u8)slot, sendBuffer_14, total) == 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0, -3);
        return -1;
    }
    return total;
}

/* Pulls one framed message out of the peer's receive buffer. */
s32 NetworkPeerGameSpy::receive(u8* a, s32* aLen, u8* b, s32* bLen, u8* flag)
{
    u16 aLen16;
    u16 bLen16;
    u16 payload;
    u8* cursor;
    s32 aMax;
    s32 bMax;
    s32 consumed;
    s8 slot;

    aMax = *aLen;
    bMax = *bLen;
    *aLen = 0;
    *bLen = 0;
    *flag = 0;
    slot = interface_6634->getSlotState(peer_6638);
    if (slot < 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, 0, 0);
        return -1;
    }
    if (received_10 < 4) {
        return 0;
    }
    LockMutex(&mutex_6614);
    cursor = recvBuffer_614;
    memcpy(&aLen16, cursor, 2);
    {
        NetworkLogger* lm = getNetworkLogger();
        aLen16 = lm->flag_48(aLen16);
    }
    payload = 0;
    bLen16 = 0;
    if (received_10 >= (u32)aLen16 + 4) {
        cursor += aLen16 + 2;
        memcpy(&bLen16, cursor, 2);
        {
            NetworkLogger* lm = getNetworkLogger();
            payload = lm->flag_48(bLen16);
        }
        bLen16 = payload;
        cursor = recvBuffer_614;
    }
    if (received_10 < (u32)(aLen16 + payload + 4)) {
        UnlockMutex(&mutex_6614);
        return 0;
    }
    cursor += 2;
    if (aLen16 != 0 && aLen16 <= aMax) {
        memcpy(a, cursor, aLen16);
        *aLen = aLen16;
    }
    cursor += aLen16;
    cursor += 2;
    if (bLen16 > 1) {
        payload = bLen16 - 1;
        if (payload <= bMax) {
            memcpy(flag, cursor, 1);
            cursor += 1;
            memcpy(b, cursor, payload);
            cursor += payload;
            *bLen = payload;
        }
    }
    consumed = aLen16 + bLen16 + 4;
    received_10 = received_10 - consumed;
    if (received_10 != 0) {
        memmove(recvBuffer_614, cursor, received_10);
    }
    UnlockMutex(&mutex_6614);
    return consumed;
}

/* Appends a buffer to the peer's receive queue. */
s32 NetworkPeerGameSpy::put(const u8* data, s32 size, u32 a, u32 b, u32 c)
{
    LockMutex(&mutex_6614);
    if (received_10 + size > 0x6000) {
        INFO_LOG("NetworkPeerGameSpy::put: buf_recv_peer over. please check NetworkPeerGameSpy::MAX_SIZE_BUF_PEER\n");
        UnlockMutex(&mutex_6614);
        return -1;
    }
    memcpy(recvBuffer_614 + received_10, data, size);
    received_10 = received_10 + size;
    UnlockMutex(&mutex_6614);
    return 0;
}

/* Reports whether the peer has a message queued. */
s32 NetworkPeerGameSpy::move()
{
    s8 state;

    state = (s8)interface_6634->getSlotState(peer_6638);
    if (state > 0) {
        return 1;
    }
    if (state < 0) {
        return state;
    }
    return 0;
}

/* Empty body: the peer's vtable slot +0x20, which only the Mcs peer implements. */
void NetworkPeerGameSpy::armDrop()
{
}

/* Clears the peer through its own virtual slot +0x28 and reports it usable. */
s32 NetworkPeerGameSpy::init()
{
    reset();
    return 1;
}

/* Releases the peer's interface slot and drops its receive queue. */
void NetworkPeerGameSpy::reset()
{
    interface_6634->unregisterReceiver(field_6630);
    LockMutex(&mutex_6614);
    memset(recvBuffer_614, 0, 0x6000);
    received_10 = 0;
    UnlockMutex(&mutex_6614);
}

/* Destroys the peer: MWCC destroys the queue mutex and the base after the empty body. */
NetworkPeerGameSpy::~NetworkPeerGameSpy()
{
}

/* Constructs the timed handler. */
NetworkTimedHandler::NetworkTimedHandler()
{
    init(0);
}

/* The timed handler owns nothing. */
NetworkTimedHandler::~NetworkTimedHandler()
{
}

/* Sets the period and the timeout, clearing the state. */
void NetworkTimedHandler::init(s64 period)
{
    clear();
    timeout_14 = 1000;
    period_08 = period;
}

/* Clears the timed handler's state, ready flag and expiry flag. */
void NetworkTimedHandler::clear()
{
    state_04 = 0;
    ready_10 = 0;
    expired_18 = 0;
}
