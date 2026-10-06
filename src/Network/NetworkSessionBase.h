/*
 * Network/NetworkSessionBase.h - the mutex wrappers, the session accessor and the notify setter of
 *   `Network/NetworkSessionBase.cpp`, with the session base class and its event callback.
 */

#ifndef NETWORK_NETWORK_SESSION_BASE_H
#define NETWORK_NETWORK_SESSION_BASE_H


/* The event callback the session reports through (`init` stores it at +0x04, the user pointer at +0x08):
   the event code (1 established, 2 data, 3 error, 4 control data, 5 record), the slot index, an argument,
   a size, the payload and the user pointer. */
/* untyped: caller-owned payload - the payload bytes and the user pointer are forwarded unchanged */
typedef void (*NetworkSessionCallback)(s32 event, s32 index, u32 arg, s32 size, const void* data, void* user);

/* The session base class (table 0x805F99A0, 0xA0 B: the deleting destructor and thirty-eight slots, every
   one of which `NetworkSessionStable` overrides; the four setters at +0x50..+0x5C have a body here, defined in
   the Stable unit - GUESS on the class name, evidenced by `NetworkSessionStable::resetAllSlots` driving the
   +0x1C slot once per slot index).  The destructor is the key function that makes MWCC emit the table.  The slot
   names are the derived class's (GUESS on each, read off its body and callers). */
class NetworkSessionBase {
public:
    NetworkSessionBase();
    virtual ~NetworkSessionBase();
    /* +0x0C */ virtual void init(s8 isHost, NetworkSessionCallback callback, void* user, /* untyped: caller-owned payload - the callback's user pointer */
                                  const u8* address, s32 param) = 0;
    /* +0x10 */ virtual void resetAllSlots() = 0;
    /* +0x14 */ virtual void move() = 0;
    /* +0x18 */ virtual s32 set(s32 isSelf, const u8* address) = 0;
    /* +0x1C */ virtual void resetSlot(s8 index) = 0;
    /* +0x20 */ virtual void connect(s8 index, u32 a, u32 b) = 0;
    /* +0x24 */ virtual void disconnectAll() = 0;
    /* +0x28 */ virtual void leave() = 0;
    /* +0x2C */ virtual void kick(s8 index) = 0;
    /* +0x30 */ virtual void sendOp1(u32 value) = 0;
    /* +0x34 */ virtual void sendOp2() = 0;
    /* +0x38 */ virtual void put(const u8* data, s32 size, s32 a, s32 b, const s8* targets, u8 limit) = 0;
    /* +0x3C */ virtual void post(const u8* data, s32 size, s8 channel, s8 index) = 0;
    /* +0x40 */ virtual void sendOp4(const void* data, u32 length, s8 index) = 0; /* untyped: byte range (the payload bytes) */
    /* +0x44 */ virtual void receiveAll() = 0;
    /* +0x48 */ virtual void discardAll() = 0;
    /* +0x4C */ virtual s32 isConnected(s8 index) = 0;
    /* +0x50 (GUESS: the name follows the words it writes) */ virtual void setLimits(u32 maxHosts, u32 maxSubhosts);
    /* +0x54 (GUESS) */ virtual void setHostTimeout(f32 seconds);
    /* +0x58 (GUESS) */ virtual void setSubhostTimeout(f32 seconds);
    /* +0x5C (GUESS) */ virtual void setRate(s32 count, s32 divisor);
    /* +0x60 */ virtual void setConnectionInterval(f32 seconds) = 0;
    /* +0x64 */ virtual void setConnectionTimeout(f32 seconds) = 0;
    /* +0x68 */ virtual void setConnectionLimit(f32 seconds) = 0;
    /* +0x6C */ virtual s32 getOwnIndex() = 0;
    /* +0x70 */ virtual void setHostIndex(s8 index) = 0;
    /* +0x74 */ virtual s8 getRelayIndex(s8 index) = 0;
    /* +0x78 */ virtual void setUserFlagA(u8 value) = 0;
    /* +0x7C */ virtual u8 getUserFlagA() = 0;
    /* +0x80 */ virtual void setUserFlagB(u8 value) = 0;
    /* +0x84 */ virtual BOOL getUserFlagB() = 0;
    /* +0x88 */ virtual void markJoined() = 0;
    /* +0x8C */ virtual f32 getRoundTrip(s8 index) = 0;
    /* +0x90 */ virtual s32 getBandwidth(s8 index) = 0;
    /* +0x94 */ virtual void setCongestion(s8 index, f32 value) = 0;
    /* +0x98 */ virtual f32 getCongestion(s8 index) = 0;
    /* +0x9C */ virtual s32 getFreeSpace(s8 index) = 0;

    /* +0x04 */ NetworkSessionCallback callback_04;   /* the event callback `init` stores; the constructor clears it */
    /* +0x08 */ void* user_08;                        /* untyped: caller-owned payload - handed back to the callback */
    /* +0x0C */ s8 isHost_0C;                         /* set by `init` for the host session */
};   /* size: 0x10 (evidence: the constructor stores the table and clears the words at +0x04/+0x08 and the byte at +0x0C; the derived fields start at +0x0D) */


extern "C" {

/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void LockMutex(void* mutex);
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void UnlockMutex(void* mutex);
/* untyped: opaque handle passed through - the peer the caller already holds */
u32 getNetworkBinaryState(void* self);
void NetworkSessionStable_setNotifyValue(u32 value);
}

#endif /* NETWORK_NETWORK_SESSION_BASE_H */
