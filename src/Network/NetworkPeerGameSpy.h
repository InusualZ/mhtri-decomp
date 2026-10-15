/*
 * Network/NetworkPeerGameSpy.h - the classes of `Network/NetworkPeerGameSpy.cpp`: the GameSpy peer `NetworkPeerGameSpy`
 *   (a `NetworkPeerBase` peer; its table 0x80603714 is emitted from its destructor) and the small timed handler
 *   `NetworkTimedHandler` (table 0x80603740).  `Network/GameSpyInterfaceThread.h` includes this header where the two
 *   classes used to be defined, so its includers see the same declarations in the same order.
 */

#ifndef MHTRI_NETWORK_NETWORKPEERGAMESPY_H
#define MHTRI_NETWORK_NETWORKPEERGAMESPY_H

#include "types.h"
#include "Network/NetworkConnection.h"      /* NetworkMutex - the member mutex */
#include "Network/NetworkPeerBase.h"        /* NetworkPeerBase - the peer base class */

/* the interface the peer is bound to (`Network/gamespy_interface_types.h`) */
class GameSpyInterfaceThread;

/* --------------------------------------------------------------------------------------------- */
/* the small timed handler whose ctor sits at the end of the range                                */
/* --------------------------------------------------------------------------------------------- */

/* A polymorphic class: the constructor 0x8041DE20 stores its table 0x80603740 (the deleting destructor 0x8041DE64
 * alone), `NetworkPool::init` allocates one with a new-expression (0x20 bytes) and `NetworkPool::destroySession`
 * deletes it through the table. */
class NetworkTimedHandler {
public:
    /* constructs the timed handler (period 0) */
    NetworkTimedHandler();
    virtual ~NetworkTimedHandler();
    /* sets the period and the 1000 ms timeout, clearing the state */
    void  init(s64 period);
    /* clears the timed handler's state, ready flag and expiry flag */
    void  clear();

    /* +0x04 */ s32 state_04;
    /* +0x08 */ s64 period_08;
    /* +0x10 */ u8  ready_10;
    /* +0x11 */ u8  pad_11[0x03];
    /* +0x14 */ s32 timeout_14;
    /* +0x18 */ u8  expired_18;
    /* +0x19 */ u8  pad_19[0x07];
};   /* size: 0x20 (the allocation `NetworkPool::init` makes) */

/* --------------------------------------------------------------------------------------------- */
/* `NetworkPeerGameSpy` - the 0x600-byte send / 0x6000-byte receive buffer pair                   */
/* --------------------------------------------------------------------------------------------- */

class NetworkPeerGameSpy : public NetworkPeerBase {
public:
    /* 0x803CA2F4 (`Network/NetworkConnection.cpp`) */ NetworkPeerGameSpy();
    /* +0x08 destroys the queue mutex and the base */
    virtual ~NetworkPeerGameSpy();
    /* +0x0C binds the peer to the interface and resets both buffers (the context is the interface and peer id pair) */
    virtual void setContext(const void* context); /* untyped: caller-owned payload - the interface and peer id pair */
    /* +0x10 builds and sends a framed peer message from the two optional payloads */
    virtual s32 send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind);
    /* +0x14 pulls one framed message out of the peer's receive buffer */
    virtual s32 receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind);
    /* +0x18 appends a datagram to the peer's receive queue */
    virtual s32 put(const u8* packet, s32 length, u32 a, u32 b, u32 c);
    /* +0x1C reports whether the peer has a message queued */
    virtual s32 move();
    /* +0x20 empty, only the Mcs peer implements it */
    virtual void armDrop();
    /* +0x24 clears the peer through slot +0x28 and reports it usable */
    virtual s32 init();
    /* +0x28 releases the peer's interface slot and drops its receive queue */
    virtual void reset();

    /* +0x0000..+0x000F - `NetworkPeerBase` */
    /* +0x0010 */ u32 received_10;
    /* +0x0014 */ u8  sendBuffer_14[0x600];
    /* +0x0614 */ u8  recvBuffer_614[0x6000];
    /* +0x6614 */ NetworkMutex mutex_6614;
    /* +0x6630 */ s32 field_6630;
    /* +0x6634 */ GameSpyInterfaceThread* interface_6634;
    /* +0x6638 */ u32 peer_6638;
};   /* size: 0x663C (the allocation `NetworkConnection`'s constructor makes) */

#endif
