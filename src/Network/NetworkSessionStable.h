/*
 * Network/NetworkSessionStable.h - the session classes `Network/NetworkSessionStable.cpp` owns: the `NetworkSessionStable` session
 * (derived from `NetworkSessionBase`), the four-slot table it keeps, the connection object each slot owns
 * and the unit packet class.
 *
 * The layouts are reconstructed from the unit's own instructions: every field carries the offset the
 * target addresses, and a field the unit never touches is `pad_`/`unused_` with its offset kept.  The class
 * names follow the log strings (`NetworkSessionStable::`, `NetworkConnectionStable[%d]`) and the map's
 * manglings; every other name is a GUESS read off the field's use.  The types `Network/network_writer_types.h`
 * declares (the small object, the stream writer) are the writer band's.
 */

#ifndef NETWORK_NETWORK_SESSION_STABLE_H
#define NETWORK_NETWORK_SESSION_STABLE_H

#include "Network/network_transport_types.h"
#include "Network/network_writer_types.h"

class NetworkSessionStable;

/* The four queues a slot owns (two send, two receive), built and destroyed by the writer band, and the
   sequence/flush state and send rate that travel with them.  `resetSlot` addresses the trailing words off
   the queues' own base (`stb r30,0x60(r29)` .. `stw r0,0x7C(r29)`), so they are members of this object; the
   writer band's constructor (0x803CA570) builds only the four queues. */
class NetworkSlotQueues {
public:
    NetworkSlotQueues();
    ~NetworkSlotQueues();

    /* +0x00 */ NetworkStreamQueue send_00[2];
    /* +0x30 */ NetworkStreamQueue receive_30[2];
    /* +0x60 */ u8 receiveStarted_60;       /* the receive sequence numbers were taken from the peer */
    /* +0x61 */ u8 pad_61[0x03];
    /* +0x64 */ f32 expiry_64;
    /* +0x68 */ u8 resend_68;
    /* +0x69 */ u8 pad_69[0x03];
    /* +0x6C */ f32 lastSend_6C[2];         /* when each send queue last flushed */
    /* +0x74 */ f32 rate_74;                /* the governor's send rate */
    /* +0x78 */ f32 rateTarget_78;
    /* +0x7C */ s32 byteLimit_7C;
};   /* size: 0x80 (the slot's next member starts at +0xD4 = 0x54 + 0x80) */

/* The slot's rate governor: when it last moved the rate, the floors it falls back to and the raise/lower
   timestamps (GUESS on the name and every field name).  `resetSlot` addresses it off its own base
   (`stfs f2,0x0(r28)` .. `stfs f2,0x14(r28)`). */
class NetworkRateGovernor {
public:
    /* +0x00 */ f32 lastAdjust_00;
    /* +0x04 */ f32 rateFloor_04;
    /* +0x08 */ s32 byteFloor_08;
    /* +0x0C */ u8 adjusting_0C;
    /* +0x0D */ u8 pad_0D[0x03];
    /* +0x10 */ f32 lastRaise_10;
    /* +0x14 */ f32 lastLower_14;
};   /* size: 0x18 (the slot's next member starts at +0xEC = 0xD4 + 0x18) */

/* ---------------- the connection a slot owns ---------------------------------------------------- */

/* What a connection reports to its owner: the record an event carries (its length, the bytes and a
   kind byte), and the frame event 3 wraps it in (the block the frame arrived in first). */
struct NetworkConnectionEventRecord {
    /* +0x00 */ u32 length_00;
    /* +0x04 */ u8* bytes_04;
    /* +0x08 */ u8 kind_08;
    /* +0x09 */ u8 pad_09[0x03];
};   /* size: 0x0C */

struct NetworkConnectionFrame {
    /* +0x00 */ u32 size_00;      /* bytes in the block */
    /* +0x04 */ u8* block_04;
    /* +0x08 */ NetworkConnectionEventRecord record_08;
};   /* size: 0x14 */

/* The block of bytes `set` hands the connection to open with: the address record the unit packet holds. */
struct NetworkConnectionOpenInfo {
    /* +0x00 */ u8* data_00;
    /* +0x04 */ u32 size_04;
};   /* size: 0x08 */

/* The callback a connection reports through: the event, the slot index, an argument, a size, the payload
   and the session that opened it. */
typedef void (*NetworkConnectionCallback)(s32 event, s8 index, u32 arg, s32 size, const u8* data, NetworkSessionStable* owner);

/* The connection object of a slot (the log strings call it `NetworkConnectionStable[%d]`), allocated by
   `NetworkSessionStable::set`; the class is the connection band's, so it is declared and never defined.
   Every virtual name is a GUESS read off the call that reaches it. */
class NetworkConnectionStable {
public:
    NetworkConnectionStable(s32 isSelf);
    /* +0x08 */ virtual ~NetworkConnectionStable();
    /* +0x0C (GUESS: opens the connection; takes the event callback, its owner, the slot index and the address record) */
    virtual void open(NetworkConnectionCallback callback, NetworkSessionStable* owner, s32 index, const NetworkConnectionOpenInfo* info);
    /* +0x10 (GUESS) */ virtual void reset();
    /* +0x14 */ virtual void slot_14();
    /* +0x18 (GUESS) */ virtual void update();
    /* +0x1C (GUESS: starts the connect to the given address) */ virtual void start(u32 a, u32 b);
    /* +0x20 (GUESS) */ virtual void begin();
    /* +0x24 (GUESS) */ virtual void end();
    /* +0x28 (GUESS) */ virtual void stop();
    /* +0x2C (GUESS) */ virtual void clearPending();
    /* +0x30 */ virtual void slot_30();
    /* +0x34 (GUESS) */ virtual void setRelay(s32 enabled);
    /* +0x38 (GUESS) */ virtual void flush();
    /* +0x3C (GUESS) */ virtual void setInterval(f32 seconds);
    /* +0x40 (GUESS) */ virtual void setTimeout(f32 seconds);
    /* +0x44 (GUESS) */ virtual void setLimit(f32 seconds);
    /* +0x48 (GUESS) */ virtual void setValue(f32 value);
    /* +0x4C (GUESS) */ virtual f32 getValue();

    void clearSendPending();       /* 0x803CBA2C (GUESS) */
    s32 getSendRoom();             /* 0x803CBA38 (GUESS) */
    s32 getCongestion();           /* 0x803CBEBC (GUESS) */
    f32 getRoundTripTime();        /* 0x803CBFFC (GUESS) */
    s32 getMaxPacketSize();        /* 0x803CC014 (GUESS) */

    /* +0x04 */ u8 pad_04[0x224C];
};   /* size: 0x2250 (evidence: the `__nw` size `set` passes) */

/* ---------------- the slot ------------------------------------------------------------------------ */

/* The record a slot keeps its peer address in: a `NetworkUniqueId` with a constructor and a destructor of its own
   in this unit (the wrapper owns the object at +0x00). */
class NetworkSlotSmallObject {
public:
    NetworkSlotSmallObject();
    ~NetworkSlotSmallObject();

    /* +0x00 */ NetworkUniqueId object_00;
};   /* size: 0x20 */

/* One of the session's four slots: a peer's link state, error record, connection, address, queues and the
   rate governor (GUESS on every name).  The offsets are relative to the slot. */
class NetworkSessionSlot {
public:
    NetworkSessionSlot();
    ~NetworkSessionSlot();

    /* +0x000 */ s32 linkState_00;           /* the link machine's state (1..5) */
    /* +0x004 */ s32 sessionState_04;        /* the connect machine's state (1..6) */
    /* +0x008 */ s32 closeState_08;          /* 0 open, else the close kind / phase (1..4) */
    /* +0x00C */ s32 sleepSeconds_0C;         /* the sleep the peer announced, in seconds */
    /* +0x010 */ s8 relayIndex_10;           /* the slot this peer is reached through, -1 direct */
    /* +0x011 */ u8 pad_11[0x03];
    /* +0x014 */ f32 relayDelay_14;          /* the delay of that route */
    /* +0x018 */ u8 established_18;          /* the connection was reported up */
    /* +0x019 */ u8 authenticated_19;        /* the peer's authentication arrived */
    /* +0x01A */ u8 left_1A;                 /* the peer left */
    /* +0x01B */ u8 relayAck_1B;             /* the peer acknowledged the relay route */
    /* +0x01C */ u8 shutdown_1C;             /* the connection shut down */
    /* +0x01D */ u8 pad_1D[0x03];
    /* +0x020 */ NetworkPeerErrorRecord error_20;   /* the first error the slot recorded */
    /* +0x02C */ NetworkConnectionStable* connection_2C;
    /* +0x030 */ NetworkSlotSmallObject address_30;
    /* +0x050 */ u32 nonce_50;               /* the nonce the peer announced */
    /* +0x054 */ NetworkSlotQueues queues_54;
    /* +0x0D4 */ NetworkRateGovernor governor_D4;
    /* +0x0EC */ f32 waitStart_EC;
    /* +0x0F0 */ f32 coolStart_F0;
    /* +0x0F4 */ s32 sequence_F4;
    /* +0x0F8 */ f32 retryTime_F8;
    /* +0x0FC */ s32 retryCount_FC;
    /* +0x100 */ u8 sleeping_100;
    /* +0x101 */ u8 pad_101[0x03];
    /* +0x104 */ f32 sleepStart_104;
    /* +0x108 */ f32 measureTime_108;
    /* +0x10C */ f32 congestion_10C;
    /* +0x110 */ s32 congestionCount_110;
    /* +0x114 */ f32 measureSend_114;
    /* +0x118 */ u8 queue_118[0x400];        /* the record the next flush appends */
    /* +0x518 */ s32 queueUsed_518[2];       /* bytes queued per channel; the second word is also what the send path reads as its channel */
    /* +0x520 */ u8 queue2_520[0x400];
    /* +0x920 */ s32 queue2Used_920;
};   /* size: 0x924 */

/* ---------------- the session -------------------------------------------------------------------- */

/* The stable session: the four slots, the send and receive blocks, the intervals and the nonce.  Its table
   (0x805FA6E8, 0xA0 B) is emitted from `~NetworkSessionStable`, the key function.  `initNetworkSessionStable`
   allocates it.  Every virtual replaces the pure slot of the same position in `NetworkSessionBase`. */
class NetworkSessionStable : public NetworkSessionBase {
public:
    NetworkSessionStable();
    virtual ~NetworkSessionStable();
    /* +0x0C */ virtual void init(s8 isHost, NetworkSessionCallback callback, void* user, /* untyped: caller-owned payload - the callback's user pointer */
                                  const u8* address, s32 param);
    /* +0x10 */ virtual void resetAllSlots();
    /* +0x14 */ virtual void move();
    /* +0x18 */ virtual s32 set(s32 isSelf, const u8* address);
    /* +0x1C */ virtual void resetSlot(s8 index);
    /* +0x20 */ virtual void connect(s8 index, u32 a, u32 b);
    /* +0x24 */ virtual void disconnectAll();
    /* +0x28 */ virtual void leave();
    /* +0x2C */ virtual void kick(s8 index);
    /* +0x30 */ virtual void sendOp1(u32 value);
    /* +0x34 */ virtual void sendOp2();
    /* +0x38 */ virtual void put(const u8* data, s32 size, s32 a, s32 b, const s8* targets, u8 limit);
    /* +0x3C */ virtual void post(const u8* data, s32 size, s8 channel, s8 index);
    /* +0x40 */ virtual void sendOp4(const void* data, u32 length, s8 index); /* untyped: byte range (the payload bytes) */
    /* +0x44 */ virtual void receiveAll();
    /* +0x48 */ virtual void discardAll();
    /* +0x4C */ virtual s32 isConnected(s8 index);
    /* +0x50 */ virtual void setLimits(u32 maxHosts, u32 maxSubhosts);
    /* +0x54 */ virtual void setHostTimeout(f32 seconds);
    /* +0x58 */ virtual void setSubhostTimeout(f32 seconds);
    /* +0x5C */ virtual void setRate(s32 count, s32 divisor);
    /* +0x60 */ virtual void setConnectionInterval(f32 seconds);
    /* +0x64 */ virtual void setConnectionTimeout(f32 seconds);
    /* +0x68 */ virtual void setConnectionLimit(f32 seconds);
    /* +0x6C */ virtual s32 getOwnIndex();
    /* +0x70 */ virtual void setHostIndex(s8 index);
    /* +0x74 */ virtual s8 getRelayIndex(s8 index);
    /* +0x78 */ virtual void setUserFlagA(u8 value);
    /* +0x7C */ virtual u8 getUserFlagA();
    /* +0x80 */ virtual void setUserFlagB(u8 value);
    /* +0x84 */ virtual u8 getUserFlagB();
    /* +0x88 */ virtual void markJoined();
    /* +0x8C */ virtual f32 getRoundTrip(s8 index);
    /* +0x90 */ virtual s32 getBandwidth(s8 index);
    /* +0x94 */ virtual void setCongestion(s8 index, f32 value);
    /* +0x98 */ virtual f32 getCongestion(s8 index);
    /* +0x9C */ virtual s32 getFreeSpace(s8 index);

    /* the connection's event callback: `event` as in `NetworkSessionCallback`, the session as the user pointer */
    static void onConnectionEvent(s32 event, s8 index, u32 arg, s32 size, const u8* data, NetworkSessionStable* self);
    void setNetworkConnectionEvent(s32 event, s8 index, u32 arg, s32 size, const u8* data);
    void setSubhostIndex(s8 index);
    void clearSubhostIndex();
    void markLeft(s8 index);
    void setError(s8 index, NetworkPeerErrorSource source, u32 argument, u32 code, s32 kind);
    s32 send(s8 index, s32 channel, s32 force);
    void execControl(s8 index, s32 user);
    void discardControlMessages(s8 index);
    s32 execControlOne(s8 index, NetworkStreamWriter* packet);
    void sendStream(NetworkStreamWriter* stream, s32 channel, s32 count, const s8* targets, u8 limit);
    void writeOp1(u32 value);
    void writeOp2();
    void writeOp6(s8 value);
    void writeOp10(s8 index);
    void writeOp11(s8 index, const NetworkUniqueId* address, u32 nonce, s8 kind, f32 delay);
    void writeOp8or9(u32 hasExtra, s8 index);
    void writeOp3(const NetworkSessionSlot* slot);
    void writeOp5(s8 index);
    void writeOp7();
    void updateRate(s8 index);
    void downPerformance(s8 index);
    void upPerformance(s8 index);
    void moveOutOfBand(s8 index);
    s8 getUsableSlot(s8 index);

    /* +0x0D */ char sendBuffer_0D[0x400];
    /* +0x40D */ u8 pad_40D[0x03];
    /* +0x410 */ u8 receiveBuffer_410[0x400];
    /* +0x810 */ u8 sendBlock_810[4][0x2000];     /* each slot's send queue block */
    /* +0x8810 */ u8 sendAux_8810[4][0x800];
    /* +0xA810 */ u8 receiveBlock_A810[4][0x2000];
    /* +0x12810 */ u8 receiveAux_12810[4][0x800];
    /* +0x14810 */ u8 userFlagA_14810;
    /* +0x14811 */ u8 userFlagB_14811;
    /* +0x14812 */ u8 pad_14812[0x02];
    /* +0x14814 */ u32 nonceDrop_14814[4];         /* the nonces of peers that dropped */
    /* +0x14824 */ s8 hostIndex_14824;
    /* +0x14825 */ s8 subhostIndex_14825;
    /* +0x14826 */ u8 ownIndex_14826;              /* the slot this session owns */
    /* +0x14827 */ u8 pad_14827;
    /* +0x14828 */ NetworkSessionSlot slots_14828[4];
    /* +0x16CB8 */ NetworkSlotSmallObject address_16CB8;   /* this session's own address record */
    /* +0x16CD8 */ u32 nonce_16CD8;
    /* +0x16CDC */ f32 time_16CDC;
    /* +0x16CE0 */ f32 hostSeen_16CE0;
    /* +0x16CE4 */ f32 subhostSeen_16CE4;
    /* +0x16CE8 */ s32 maxHosts_16CE8;
    /* +0x16CEC */ s32 maxSubhosts_16CEC;
    /* +0x16CF0 */ f32 hostTimeout_16CF0;
    /* +0x16CF4 */ f32 subhostTimeout_16CF4;
    /* +0x16CF8 */ f32 rateStep_16CF8;
    /* +0x16CFC */ s32 rateWindow_16CFC;
    /* +0x16D00 */ u8 joined_16D00;
    /* +0x16D01 */ u8 pad_16D01[0x07];
};   /* size: 0x16D08 */

/* ---------------- the unit packet ------------------------------------------------------------------ */

/* The packet `NetworkSessionStable::set` and `setNetworkConnectionEvent` keep on the stack: the writer band's
   stream sink with a destructor of its own.  Its table (0x805FA6A8, 0x40 B) is emitted from that destructor. */
class NetworkUnitPacket : public NetworkStreamSink {
public:
    NetworkUnitPacket();
    virtual ~NetworkUnitPacket();
};   /* size: 0x10 */

#endif /* NETWORK_NETWORK_SESSION_STABLE_H */
