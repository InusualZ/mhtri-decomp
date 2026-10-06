/*
 * Network/NetworkConnection.h - the declarations of `Network/NetworkConnection.cpp`: the connection base class a
 *   session slot's connection is built on, and the member mutex pair the network records embed.
 */
#ifndef MHTRI_NETWORK_NETWORKCONNECTION_H
#define MHTRI_NETWORK_NETWORKCONNECTION_H

#include "types.h"
#include "unsplit/OS.h"                  /* OSMutex */

class NetworkSessionStable;               /* Network/NetworkSessionStable.h */
class NetworkPeerBase;                    /* Network/NetworkPeerBase.h */
struct NetworkConnectionOpenInfo;         /* Network/NetworkSessionStable.h */

/* The callback a connection reports through: the event, the slot index, an argument, a size, the payload
   and the session that opened it. */
typedef void (*NetworkConnectionCallback)(s32 event, s8 index, u32 arg, s32 size, const u8* data, NetworkSessionStable* owner);

/* The mutex the network records embed as a member (GUESS on the name): the constructor stores the table 0x805F91E0
   and initialises the OS mutex after it; the destructor only frees on request.  The records' `LockMutex`/`UnlockMutex`
   (`Network/NetworkSessionBase.cpp`) take its address. */
class NetworkMutex {
public:
    NetworkMutex();
    /* +0x08 */ virtual ~NetworkMutex();

    /* +0x04 */ OSMutex mutex_04;
};   /* size: 0x1C (the records' 0x1C-byte block at +0x78) */

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

#endif /* MHTRI_NETWORK_NETWORKCONNECTION_H */
