/*
 * Network/NetworkSessionStable.h - the classes of `Network/NetworkSessionStable.cpp`: the `NetworkSessionStable`
 *   session, its four-slot table, the connection object each slot owns and the unit packet class.  The names follow the
 *   log strings (`NetworkSessionStable::`, `NetworkConnectionStable[%d]`) and the map; every other name is a GUESS.
 */

#ifndef NETWORK_NETWORK_SESSION_STABLE_H
#define NETWORK_NETWORK_SESSION_STABLE_H

#include "Network/NetworkPeerBase.h"
#include "Network/NetworkSessionBase.h"
#include "Network/NetworkStreamSink.h"
#include "Network/NetworkUnitPacket.h"     /* NetworkStreamQueue */

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

/* The callback a connection reports through (`NetworkConnectionCallback`) is declared with the connection base in
   `Network/NetworkConnection.h`. */

/* The error a connection keeps for its owner: the first failure's code and its two arguments (`setError` refuses to
   overwrite a filled record). */
struct NetworkConnectionError {
    /* +0x00 */ s32 code_00;
    /* +0x04 */ s32 param1_04;
    /* +0x08 */ s32 param2_08;
};   /* size: 0x0C */

/* The connection object of a slot (the log strings call it `NetworkConnectionStable[%d]`), allocated by
   `NetworkSessionStable::set`: a `NetworkConnection` whose peer carries the frames, with the slot's queues, the
   handshake that trades sequence numbers and addresses, a ping table and round-trip average, the MTU probe and the
   traffic counters.  Its methods are `Network/NetworkConnectionStable.cpp`'s; the slot names are GUESSes read off the
   call that reaches each one. */
class NetworkConnectionStable : public NetworkConnection {
public:
    NetworkConnectionStable(s32 kind);
    /* +0x08 */ virtual ~NetworkConnectionStable();
    /* +0x0C (opens the connection: the event callback, its owner, the slot index and the address record) */
    virtual void open(NetworkConnectionCallback callback, NetworkSessionStable* owner, s8 index, const NetworkConnectionOpenInfo* info);
    /* +0x10 (closes the peer and returns to the idle state) */ virtual void reset();
    /* +0x14 (clears every buffer, queue, clock and counter) */ virtual void clear();
    /* +0x18 (one frame of the connect / link / close machine) */ virtual void update();
    /* +0x1C (binds the peer to its record; `b` == 0 keeps the MTU probing on) */ virtual void start(u32 a, u32 b);
    /* +0x20 (starts the connect with the owner's round-trip estimate) */ virtual void begin(f32 roundTrip);
    /* +0x24 (closes towards a relay) */ virtual void end();
    /* +0x28 (closes and leaves) */ virtual void stop();
    /* +0x2C (flushes both channels and shuts down) */ virtual void clearPending();
    /* +0x30 (the slot index) */ virtual s32 getIndex();
    /* +0x34 (arms the receive timeout) */ virtual void setRelay(u8 enabled);
    /* +0x38 (marks a receive in this frame) */ virtual void flush();
    /* +0x3C */ virtual void setInterval(f32 seconds);
    /* +0x40 */ virtual void setTimeout(f32 seconds);
    /* +0x44 */ virtual void setLimit(f32 seconds);
    /* +0x48 (the queues' expiry) */ virtual void setValue(f32 value);
    /* +0x4C */ virtual f32 getValue();

    void putOnSendPool(NetworkStreamWriter* packet, s32 channel);   /* 0x803CB41C */
    void sendChannel(s32 channel);       /* 0x803CB4E0 (GUESS) */
    void clearSendPending();             /* 0x803CBA2C (GUESS) */
    s32 getSendRoom();                   /* 0x803CBA38 (GUESS) */
    void receivePackets();               /* 0x803CBC38 (GUESS) */
    s32 getCongestion();                 /* 0x803CBEBC (GUESS) */
    f32 getRoundTripTime();              /* 0x803CBFFC (GUESS) */
    s32 getMaxPacketSize();              /* 0x803CC014 (GUESS) */
    void setState(s32 state);            /* 0x803CC030 (GUESS) */
    s32 getState();                      /* 0x803CC038 (GUESS) */
    void setError(s32 code, s32 param1, s32 param2);   /* 0x803CC040 (GUESS) */
    s32 checkChannel(s32 channel);       /* 0x803CC05C (GUESS: 1, and an error, for a channel other than 0/1) */
    void sendHello(s32 reply);           /* 0x803CC0A8 (GUESS: control 0x81, or 0x82 answering one) */
    void sendHelloDone();                /* 0x803CC1C0 (GUESS: control 0x83) */
    void sendPing(u8 index);             /* 0x803CC240 (GUESS: control 0x91) */
    void sendPong(u8 index, u32 time);   /* 0x803CC348 (GUESS: control 0x92) */
    void sendClose(s32 relay);           /* 0x803CC430 (GUESS: control 0x84 towards a relay, else 0x85) */
    void sendMtuProbe();                 /* 0x803CC4CC (GUESS) */
    void stepMtu();                      /* 0x803CC630 (GUESS) */
    void sendMtuReply(u32 size);         /* 0x803CC720 (GUESS: control 0x95) */
    void dispatchControl();              /* 0x803CC7CC (GUESS) */
    void updateRoundTrip(u8 index);      /* 0x803CCCDC (GUESS) */
    void updateRate();                   /* 0x803CCDA4 (GUESS) */

    /* +0x0000..+0x000F - `NetworkConnection` */
    /* +0x0010 */ NetworkConnectionError error_0010;
    /* +0x001C */ u8 frame_001C[0x400];        /* the frame being built or sent */
    /* +0x041C */ u8 receive_041C[0x400];      /* the peer's received frames */
    /* +0x081C */ u8 record_081C[0x400];       /* the peer's received record */
    /* +0x0C1C */ u8 raw_0C1C[0x400];          /* the frames `networkStreamWriter_attach` collects */
    /* +0x101C */ s32 rawSize_101C;
    /* +0x1020 */ u8 sendPending_1020;         /* set when the collected frames were flushed for room */
    /* +0x1021 */ u8 pad_1021[0x03];
    /* +0x1024 */ NetworkSlotQueues queues_1024;
    /* +0x10A4 */ u8 sendBlocks_10A4[2][0x400];
    /* +0x18A4 */ u8 receiveBlocks_18A4[2][0x400];
    /* +0x20A4 */ s8 index_20A4;               /* the session slot */
    /* +0x20A5 */ u8 address_20A5[0x80];       /* this side's address record (`open`) */
    /* +0x2125 */ u8 pad_2125[0x03];
    /* +0x2128 */ s32 addressSize_2128;
    /* +0x212C */ u8 peerAddress_212C[0x80];   /* the other side's (the handshake) */
    /* +0x21AC */ s32 peerAddressSize_21AC;
    /* +0x21B0 */ s32 state_21B0;              /* 0 idle, 1 connecting, 2 handshake, 3 established, 4 linked, 5/6 closing */
    /* +0x21B4 */ f32 now_21B4;                /* the clock this frame */
    /* +0x21B8 */ f32 openTime_21B8;
    /* +0x21BC */ f32 lastPoll_21BC;
    /* +0x21C0 */ f32 pollInterval_21C0;
    /* +0x21C4 */ f32 connectStart_21C4;
    /* +0x21C8 */ f32 connectTimeout_21C8;
    /* +0x21CC */ f32 lastHello_21CC;
    /* +0x21D0 */ u8 pingIndex_21D0;
    /* +0x21D1 */ u8 pad_21D1[0x03];
    /* +0x21D4 */ f32 pingTimes_21D4[8];      /* the open pings' send times, 0 once answered */
    /* +0x21F4 */ f32 roundTrip_21F4;
    /* +0x21F8 */ s32 pingsLost_21F8;
    /* +0x21FC */ s32 pingsAnswered_21FC;
    /* +0x2200 */ f32 lastPing_2200;
    /* +0x2204 */ f32 lastStat_2204;
    /* +0x2208 */ s32 bytesSent_2208;
    /* +0x220C */ s32 bytesReceived_220C;
    /* +0x2210 */ s32 lastBytesSent_2210;
    /* +0x2214 */ s32 lastBytesReceived_2214;
    /* +0x2218 */ s32 minuteBytesSent_2218;
    /* +0x221C */ s32 minuteBytesReceived_221C;
    /* +0x2220 */ s32 packetsSent_2220;
    /* +0x2224 */ s32 lastPacketsSent_2224;
    /* +0x2228 */ s32 minutePacketsSent_2228;
    /* +0x222C */ s32 totalBytesSent_222C;
    /* +0x2230 */ s32 totalBytesReceived_2230;
    /* +0x2234 */ f32 lastReceive_2234;
    /* +0x2238 */ f32 receiveTimeout_2238;
    /* +0x223C */ u8 receivedThisFrame_223C;
    /* +0x223D */ u8 receiveTimeoutOn_223D;
    /* +0x223E */ u8 mtuProbe_223E;
    /* +0x223F */ u8 mtuReply_223F;
    /* +0x2240 */ u32 mtuSeen_2240;
    /* +0x2244 */ u32 mtu_2244;
    /* +0x2248 */ bool mtuFixed_2248;
    /* +0x2249 */ u8 pad_2249[0x03];
    /* +0x224C */ s32 mtuTries_224C;
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
    /* +0x84 */ virtual BOOL getUserFlagB();
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
