/*
 * Network/NetworkConnection.h - the declarations of `Network/NetworkConnection.cpp`: the connection base class a
 *   session slot's connection is built on, and the member mutex pair the network records embed.
 */
#ifndef MHTRI_NETWORK_NETWORKCONNECTION_H
#define MHTRI_NETWORK_NETWORKCONNECTION_H

#include "types.h"

class NetworkSessionStable;               /* Network/NetworkSessionStable.h */
class NetworkPeerBase;                    /* Network/NetworkPeerBase.h */
struct NetworkConnectionOpenInfo;         /* Network/NetworkSessionStable.h */

/* The callback a connection reports through: the event, the slot index, an argument, a size, the payload
   and the session that opened it. */
typedef void (*NetworkConnectionCallback)(s32 event, s8 index, u32 arg, s32 size, const u8* data, NetworkSessionStable* owner);

/* The connection base (table 0x805F9190): the constructor creates the slot's peer by kind and the destructor
   destroys it; the three timing setters keep the defaults every new connection starts from.  The other slots are
   pure: `NetworkConnectionStable` fills them.  Slot names are GUESSes read off the calls that reach them. */
class NetworkConnection {
public:
    /* 0x803CA1D4 - kind 1 a buffer peer, 2 a Udp peer, 4/5 an Mcs peer (armed / not), 6 a GameSpy peer */
    NetworkConnection(s32 kind);
    /* +0x08 */ virtual ~NetworkConnection();
    /* +0x0C */ virtual void open(NetworkConnectionCallback callback, NetworkSessionStable* owner, s8 index, const NetworkConnectionOpenInfo* info) = 0;
    /* +0x10 */ virtual void reset() = 0;
    /* +0x14 */ virtual void clear() = 0;
    /* +0x18 */ virtual void update() = 0;
    /* +0x1C */ virtual void start(u32 a, u32 b) = 0;
    /* +0x20 */ virtual void begin(f32 roundTrip) = 0;
    /* +0x24 */ virtual void end() = 0;
    /* +0x28 */ virtual void stop() = 0;
    /* +0x2C */ virtual void clearPending() = 0;
    /* +0x30 */ virtual s32 getIndex() = 0;
    /* +0x34 */ virtual void setRelay(u8 enabled) = 0;
    /* +0x38 */ virtual void flush() = 0;
    /* +0x3C 0x803CA484 - the keep-alive interval new connections start with */ virtual void setInterval(f32 seconds);
    /* +0x40 0x803CA48C - the connect timeout new connections start with */ virtual void setTimeout(f32 seconds);
    /* +0x44 0x803CA494 - the receive timeout new connections start with */ virtual void setLimit(f32 seconds);
    /* +0x48 */ virtual void setValue(f32 value) = 0;
    /* +0x4C */ virtual f32 getValue() = 0;

    /* +0x04 */ NetworkConnectionCallback callback_04;   /* the owner's event callback (`open`) */
    /* +0x08 */ NetworkSessionStable* owner_08;
    /* +0x0C */ NetworkPeerBase* peer_0C;               /* the peer the constructor created */
};   /* size: 0x10 */

extern "C" {

/* 0x803CA338 / 0x803CA37C - destroy (flags -1 at every call site) and construct the member mutex the network
 * records embed (the constructor references the 0x10-byte .data 0x805F91E0).  The callers hand a byte block
 * whose layout only this unit's class owns. */
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_destroyMutex(void* self, s32 flags);
/* untyped: opaque handle passed through - only the writer band owns the layout */
void networkInstance_initMutex(void* self);

}

#endif /* MHTRI_NETWORK_NETWORKCONNECTION_H */
