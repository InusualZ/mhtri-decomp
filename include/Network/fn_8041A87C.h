/*
 * include/Network/fn_8041A87C.h - the classes and data of the 0x8041A87C Network band
 * (`src/Network/fn_8041A87C.cpp`; its header carries the seam evidence, the flags and the residuals).
 *
 * Every offset is one the target instructions address.  `NetworkReflectService` (five methods here), the
 * worker thread `GameSpyInterfaceThread`, `NetworkPeerGameSpy` and `NetworkTimedHandler` own member
 * functions; the foreign objects the target dispatches through (`GameSpyReceiver`, `NetworkPeerCallback`,
 * and `NetworkLogger`/`NetworkInstanceDispatch` in `include/unsplit/Network.h`) are classes with real
 * virtuals - the only shape MWCC emits as `lwz r12, 0x0(r3)` / `lwz r12, <slot>(r12)`; none is
 * constructed here, so no vtable is emitted for them.  `GameSpyInterfaceThread` is the exception: its
 * destructor is the class's one virtual, so this unit emits its vtable.  The peer's and the timed
 * handler's tables (0x80603714, 0x80603740) belong to other TUs and stay hand-wired.
 */

#ifndef FN_8041A87C_H
#define FN_8041A87C_H

#include "types.h"
#include "unsplit/Network.h"
#include "Network/network_state.h"
#include "Network/sGameSpyInterfaceThread.h"  /* the live thread pointer this unit defines */
#include "DWCi/fn_805113B0.h"                  /* DWCi_htons / DWCi_formatAddress - owner DWCi/fn_805113B0.c */
#include "sound/fn_800E46E8.h"                 /* getInstance - owner sound/fn_800E46E8.cpp */

/* the records the two interface classes take pointers to, defined further down */
struct GameSpyPeerId;
class NetworkPeerGameSpy;
union GameSpyEventMsg;
/* --------------------------------------------------------------------------------------------- */
/* GameSpy interface state machine - the connect/NAT sub-machines                                 */
/* --------------------------------------------------------------------------------------------- */

typedef struct GameSpyChannel {
    /* +0x00 */ u8  ownerId_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 peerId_04;
    /* +0x08 */ u8  address_08[0x1F];
    /* +0x27 */ u8  tail_27;
} GameSpyChannel;   /* size: 0x28 */

class NetworkReflectService {
public:
    /* +0x0000 */ u8  pad_00[0x0C];
    /* +0x000C */ u32 flags_0C;
    /* +0x0010 */ s32 task_10;
    /* +0x0014 */ u8  searchStep_14;
    /* +0x0015 */ u8  connectStep_15;
    /* +0x0016 */ u8  pad_16;
    /* +0x0017 */ u8  callbackStep_17;
    /* +0x0018 */ u8  channel_18;
    /* +0x0019 */ u8  pad_19[0x03];
    /* +0x001C */ u32 peerId_1C;
    /* +0x0020 */ u8  recvArea_20[0x8000];
    /* +0x8020 */ s32 channelCount_8020;
    /* +0x8024 */ GameSpyChannel channels_8024[8];
    /* +0x8164 */ u32 limit_8164;
    /* +0x8168 */ u32 writePos_8168;

    /* forwards a work record (code, three words, payload) to the callback pair at +0x04 / +0x08;
       defined by the neighbouring band, the only body here that is not this unit's */
    /* untyped: caller-owned payload - each work code carries its own record */
    void notify(u32 code, s32 a, s32 b, s32 c, void* data);
    /* one step of the connect-attempt callback sub-machine */
    void updateCallbackStep();
    /* dispatches to the search (task 1) or the connect (task 2) sub-machine */
    s32 dispatchTask();
    /* one step of the GameSpy search sub-machine */
    s32 runSearch();
    /* one step of the GameSpy NAT/connect sub-machine */
    s32 runConnect();
    /* applies a DWC event, then folds the event bits into the state machine's flags */
    void applyEvent(u32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg);
};   /* size: 0x816C (approximation: the range addresses up to +0x8168) */

/* --------------------------------------------------------------------------------------------- */
/* the GameSpy worker thread object (the string pool spells its methods `NetworkGameSpyInterface::<method>`) */
/* --------------------------------------------------------------------------------------------- */

typedef struct NetworkErrorInfo {
    /* +0x00 */ s32 code_00;
    /* +0x04 */ s32 param1_04;
    /* +0x08 */ s32 param2_08;
    /* +0x0C */ s32 reported_0C;
} NetworkErrorInfo;   /* size: 0x10 */

/* The worker thread object.  The pool's own logs spell its methods `NetworkGameSpyInterface::<method>`, but
 * the map's names (`__ct__22GameSpyInterfaceThreadFv`, `GameSpyInterfaceThreadInit`) say `GameSpyInterfaceThread`
 * and every consumer already spells it so: the class keeps the map's name.
 *
 * `thread_130` is sized 0x4318 (dolphin's 0x318-byte `OSThread` plus the 0x4000-byte stack the
 * constructor hands `OSCreateThread`), which is what puts the annotations from `threadParam_4448`
 * on at `+0x4448` in the object as well as in this comment.  Sized 0x1E50 (the pre-fix value) the
 * whole tail of the object compiled 0x24C8 low - retail reads `sessionOpen` at +0x4484 where ours
 * read +0x1FBC - and the four tail accessors (`getPeerId`, `isNegotiating`,
 * `getNegotiationResult`, `setBufferSize`) each lost the displacement while 19 more rows moved up
 * with the fix. */
#pragma pack(1)   /* `profile_4485` is five 32-bit values at the odd offset 0x4485 */
class GameSpyInterfaceThread {
public:
    /* +0x0000 - the vtable pointer the compiler stores.  The deleting destructor is the class's one virtual
     * and is declared before any field: MWCC places the pointer where the first virtual is declared.  It is
     * the key function (map row `__dt__22GameSpyInterfaceThreadFv`), so this unit emits the vtable
     * (`__vt__22GameSpyInterfaceThread`) and empties the singleton in it. */
    virtual ~GameSpyInterfaceThread();

    /* the live worker thread (`sGameSpyInterfaceThread`); defined in `Network/NetworkSessionManager.cpp` */
    static GameSpyInterfaceThread* getInstance();
    /* +0x0004 */ s32 errorCode_04;
    /* +0x0008 */ s32 errorParam1_08;
    /* +0x000C */ s32 errorParam2_0C;
    /* +0x0010 */ u8  errorReported_10;
    /* +0x0011 */ u8  pad_11[0x03];
    /* +0x0014 */ u32 receivers_14[4];
    /* +0x0024 */ u8  receiverState_24[4];
    /* +0x0028 */ u8  receiverCount_28;
    /* +0x0029 */ u8  pad_29[0x03];
    /* +0x002C */ s32 state_2C;
    /* +0x0030 */ u32 value_30;
    /* +0x0034 */ u32 receiverIds_34[4];
    /* +0x0044 */ u32 slotIds_44[4];
    /* +0x0054 */ s8  slotState_54[4];
    /* +0x0058 */ u32 slotHandles_58[4];
    /* +0x0068 */ s32 field_68;
    /* +0x006C */ s32 running_6C;
    /* +0x0070 */ s32 frame_70;
    /* +0x0074 */ s32 phase_74;
    /* +0x0078 */ u8  flag_78;
    /* +0x0079 */ u8  pad_79[0x03];
    /* +0x007C */ s32 paramA_7C;
    /* +0x0080 */ u32 paramB_80;
    /* +0x0084 */ s32 paramC_84;
    /* +0x0088 */ s32 paramD_88;
    /* +0x008C */ s32 paramE_8C;
    /* +0x0090 */ s32 result_90;
    /* +0x0094 */ s32 handle_94;
    /* +0x0098 */ s32 stage_98;
    /* +0x009C */ char name_9C[0x80];
    /* +0x011C */ u8  idByte_11C;
    /* +0x011D */ u8  idByte_11D;
    /* +0x011E */ u8  idByte_11E;
    /* +0x011F */ u8  idByte_11F;
    /* +0x0120 */ u8  started_120;
    /* +0x0121 */ u8  stopRequested_121;
    /* +0x0122 */ u8  cancelPending_122;
    /* +0x0123 */ u8  closePending_123;
    /* +0x0124 */ u8  initRequested_124;
    /* +0x0125 */ u8  openRequested_125;
    /* +0x0126 */ u8  stepRequested_126;
    /* +0x0127 */ u8  pad_127;
    /* +0x0128 */ u32 busy_128;
    /* +0x012C */ u8  pad_12C[0x04];
    /* +0x0130 */ u8  thread_130[0x4318];
    /* +0x4448 */ u32 threadParam_4448;
    /* +0x444C */ u8  mutexReady_444C;
    /* +0x444D */ u8  pad_444D[0x03];
    /* +0x4450 */ u8  mutex_4450[0x18];
    /* +0x4468 */ s32 field_4468;
    /* +0x446C */ u8  negotiation_446C;
    /* +0x446D */ u8  negotiationResult_446D;
    /* +0x446E */ s8  negotiationDone_446E;
    /* +0x446F */ u8  pad_446F;
    /* +0x4470 */ u32 peerId_4470;
    /* +0x4474 */ u32 selfPeerId_4474;
    /* +0x4478 */ s32 peerMatch_4478;
    /* +0x447C */ u32 session_447C;
    /* +0x4480 */ s8  negotiationStep_4480;
    /* +0x4481 */ u8  pad_4481;
    /* +0x4482 */ s16 bufferSize_4482;
    /* +0x4484 */ s8  sessionOpen_4484;
    /* +0x4485 */ u32 profile_4485[5];

    /* the constructor: stores the vtable, publishes the singleton, resets the tables and calls the
     * thread init.  Callers reach it only as `new GameSpyInterfaceThread()`, so the map row at
     * 0x8041C66C carries the mangled spelling (`__ct__22GameSpyInterfaceThreadFv`) */
    GameSpyInterfaceThread();
    /* resets the per-request tables and the pending-request flags */
    void  resetState();
    /* resets the slot tables and the negotiation state */
    void  resetSlots();
    /* the empty vtable cleanup hook */
    void  onDestroy();
    /* replies to a pending request by filling the first free return handle */
    s32   replyRequest(s32 handle);
    /* clears the error record and marks it reported */
    void  clearError();
    /* stores the first error the thread sees, unless it has already been reported */
    void  setError(s32 code, s32 a, s32 b);
    /* copies the thread's error record out for the game to read */
    void  getErrorStruct(NetworkErrorInfo* out);
    /* moves the interface into its running state, or reports the shutdown */
    s32   initialize();
    /* ticks the GameSpy clock and counts one more frame */
    s32   updateClock();
    /* reports whether a close is already pending, arming the step flag if so */
    u8    requestClose();
    /* returns the interface phase */
    s32   getPhase();
    /* returns the running state, or -1 while the interface is not running */
    s32   getState();
    /* clears the running state and the pending close */
    void  clearPendingClose();
    /* arms the cancellation when a close arrives while a request is still in flight */
    void  armCancel();
    /* tears the socket, the session and the request pool down under the interface mutex */
    s32   closeSession();
    /* reports whether the interface can be closed right now */
    s32   canClose();
    /* reports whether a negotiation is running */
    u8    isNegotiating();
    /* reports the negotiation's outcome */
    u8    getNegotiationResult();
    /* returns this interface's own peer id */
    u32   getPeerId();
    /* returns the request result, arming the step flag first */
    s32   getResult();
    /* drains the DWC error queue and folds the reported type into the interface state */
    s32   executeError();
    /* the worker thread's body: drains the request flags until the stop flag is set */
    void  tGameSpyInterface(s32 arg);
    /* spawns the worker thread */
    s32   GameSpyInterfaceThreadInit();
    /* sets the socket address buffer size */
    void  setBufferSize(s16 size);
    /* starts a NAT negotiation between the two peer ids */
    void  startNegotiation(const GameSpyPeerId* a, const GameSpyPeerId* b);
    /* sets the handle the NAS login waits for */
    void  setWaitHandle(s32 limit);
    /* runs the NAS-login handshake the timed handler drives, one step per frame */
    s32   runNasLogin();
    /* hands the index's receiver slot to the object stored for it (a tail call) */
    void  dispatchReceiver(u8 index, s32 a, s32 b);
    /* fails the index's request record when `error` is set, then closes the slot */
    void  failRequest(s32 error, s32 a, s32 b, u8 index, s32 e);
    /* records the outcome of the DWC request the interface is waiting on */
    void  setRequestResult(s32 error, s32 value);
    /* publishes a completed request: rewrites the slot tables and the negotiation result */
    void  publishRequest(s32 error, u8 index, u32 value);
    /* opens the GameSpy socket and installs the callback set, then applies the pending requests;
     * the caller's thread argument is unused here but retail's caller passes it (see the unit header) */
    void  ConnectToAnybody(s32 arg);
    /* starts a GameSpy match for `count` players and stores the peer id it was given */
    s32   startMatch(s32 count, u32 value, s32 a, u16 b, s32 c, s32 d, s32 e);
    /* registers a receiver for `id` in the first free slot */
    s32   registerReceiver(NetworkPeerGameSpy* receiver, u32 id);
    /* releases the receiver slot at `index` and clears its id */
    void  unregisterReceiver(s32 index);
    /* returns the state of the slot `id` maps to, or the pending negotiation result; the callers
       narrow it to `s8`, so a negative value means the peer's interface slot is gone */
    u8    getSlotState(u32 id);
    /* returns the slot index `id` maps to, or -1 */
    s8    findSlot(u32 id);
    /* the per-frame step: the negotiation sub-machine, the request sweep and the close/cancel
       bookkeeping */
    void  step();
    /* sends a buffer out over the socket the index maps to */
    /* untyped: byte range - the datagram */
    s32   sendUnreliable(u8 index, const void* data, s32 size);
    /* compares a received peer profile against the one this interface published */
    /* untyped: byte range - the received peer profile */
    s32   checkPeerProfile(const void* profile, u32 size);
};   /* size: 0x8450 (approximation: the 0x4000-byte stack at +0x4448 is the upper bound) */
#pragma pack()

/* --------------------------------------------------------------------------------------------- */
/* the small timed handler whose ctor sits at the end of the range                                */
/* --------------------------------------------------------------------------------------------- */

class NetworkTimedHandler {
public:
    /* +0x00 */ void* vtable_00;
    /* +0x04 */ s32 state_04;
    /* +0x08 */ s32 interval_08;
    /* +0x0C */ s32 limit_0C;
    /* +0x10 */ u8  ready_10;
    /* +0x11 */ u8  pad_11[0x03];
    /* +0x14 */ s32 timeout_14;
    /* +0x18 */ u8  expired_18;
    /* +0x19 */ u8  pad_19[0x03];

    /* constructs the timed handler */
    NetworkTimedHandler* create();
    /* deleting destructor for the timed handler */
    NetworkTimedHandler* destroy(s16 flags);
    /* initialises the timed handler's interval, limit and timeout */
    void  init(s32 a, s32 b, s32 c);
    /* clears the timed handler's state, ready flag and expiry flag */
    void  clear();
};   /* size: 0x1C */

/* --------------------------------------------------------------------------------------------- */
/* `NetworkPeerGameSpy` - the 0x600-byte send / 0x6000-byte receive buffer pair                   */
/* --------------------------------------------------------------------------------------------- */

class NetworkPeerGameSpy {
public:
    /* +0x0000 */ void* vtable_00;
    /* +0x0004 */ u8  pad_04[0x0C];
    /* +0x0010 */ u32 received_10;
    /* +0x0014 */ u8  sendBuffer_14[0x600];
    /* +0x0614 */ u8  recvBuffer_614[0x6000];
    /* +0x6614 */ u8  mutex_6614[0x1C];
    /* +0x6630 */ s32 field_6630;
    /* +0x6634 */ GameSpyInterfaceThread* interface_6634;
    /* +0x6638 */ u32 peer_6638;

    /* binds the peer to the interface and resets both buffers */
    void  bind(const u32* id);
    /* builds and sends a framed peer message from the two optional payloads */
    s32   send(const u16* a, s32 aLen, const u16* b, s32 bLen, s8 flag);
    /* pulls one framed message out of the peer's receive buffer */
    /* untyped: byte range - the two caller buffers the framed payloads are copied into */
    s32   receive(void* a, s32* aLen, void* b, s32* bLen, u8* flag);
    /* appends a buffer to the peer's receive queue (the pool's `NetworkPeerGameSpy::put`) */
    /* untyped: byte range - the datagram appended to the receive queue */
    s32   put(const void* data, u32 size);
    /* reports whether the peer has a message queued */
    s32   isQueued();
    /* releases the peer's interface slot and drops its receive queue */
    void  release();
    /* vtable slot +0x20: empty, only the Mcs peer implements it */
    void  armDrop();
    /* vtable slot +0x24: clears the peer through slot +0x28 and reports it usable */
    s32   init();
    /* deleting destructor: destroys the queue mutex and the base, then frees on request */
    NetworkPeerGameSpy* destroy(s16 flags);
};   /* size: 0x663C (approximation: the range addresses up to +0x6638) */

/* The receiver object `GameSpyInterfaceThread::dispatchReceiver` tail-calls.  Its slot +0x18 takes the five arguments the
 * tail call passes; the class is only ever dispatched through, so no vtable is emitted for it. */
class GameSpyReceiver {
public:
    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void pad_14();
    /* +0x18 */ virtual void handle_18(s32 a, s32 b, s32 c, s32 d, s32 e);
};   /* size: 0x04 (the object's leading vtable word) */

/* The peer object `NetworkPeerGameSpy::init` ticks through slot +0x28. */
class NetworkPeerCallback {
public:
    /* +0x08 */ virtual void pad_08();
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void pad_14();
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual void pad_20();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual s32  tick_28();
};   /* size: 0x04 (the object's leading vtable word) */

/* the 8-byte GameSpy header `gt2UnrecognizedMessageCallback` builds on the stack */
typedef struct GameSpyHeader {
    /* +0x00 */ u8  type_0;
    /* +0x01 */ u8  code_1;
    /* +0x02 */ u16 peer_2;
    /* +0x04 */ u32 value_4;
} GameSpyHeader;   /* size: 0x08 */

/* an 8-byte GameSpy peer address */
typedef struct GameSpyAddress {
    /* +0x00 */ u8  first_00;
    /* +0x01 */ u8  second_01;
    /* +0x02 */ u16 port_02;
    /* +0x04 */ u32 value_04;
} GameSpyAddress;   /* size: 0x08 */

/* the result record `natNegCompletedCallback` fills in for the DWC callback */
typedef struct GameSpyResultInfo {
    /* +0x00 */ u32 connected_00;
    /* +0x04 */ u32 result_04;
    /* +0x08 */ GameSpyAddress address_08;
} GameSpyResultInfo;   /* size: 0x10 */

/* the id pair `fn_8041D530` starts a NAT negotiation from */
typedef struct GameSpyPeerId {
    /* +0x00 */ u32 peerId_00;
    /* +0x04 */ u32 mode_04;
    /* +0x08 */ u32 session_08;
    /* +0x0C */ u16 port_0C;
    /* +0x0E */ u16 pad_0E;
} GameSpyPeerId;   /* size: 0x10 */

/* the peer thread's NAT-negotiation record (`sNatNegState`) */
typedef struct GameSpyNegotiation {
    /* +0x00 */ u32 active_00;
    /* +0x04 */ u32 result_04;
    /* +0x08 */ u16 value_08;
    /* +0x0A */ u16 encoded_0A;
    /* +0x0C */ u32 session_0C;
} GameSpyNegotiation;   /* size: 0x10 */


/* the two overlapping views of a DWC event message */
typedef struct GameSpyChannelMsg {
    /* +0x00 */ u8  channel_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 peerId_04;
    /* +0x08 */ u32 limit_08;
    /* +0x0C */ u8  count_0C;
    /* +0x0D */ u8  pad_0D[0x03];
    /* +0x10 */ const GameSpyChannel* channels_10;
} GameSpyChannelMsg;   /* size: 0x14 (approximation: only the leading words are addressed) */

typedef struct GameSpyDataMsg {
    /* +0x00 */ u32 channel_00;
    /* +0x04 */ u32 writePos_04;
    /* +0x08 */ u32 size_08;
    /* +0x0C */ void* data_0C;
} GameSpyDataMsg;   /* size: 0x10 (approximation: only the leading words are addressed) */

/* DWC hands the same payload to both event codes, so the record is read through either view */
typedef union GameSpyEventMsg {
    /* +0x00 */ GameSpyChannelMsg channelView;
    /* +0x00 */ GameSpyDataMsg dataView;
} GameSpyEventMsg;   /* size: 0x14 */

/* --------------------------------------------------------------------------------------------- */
/* The data this unit owns, defined at the foot of `fn_8041A87C.cpp`.  `splits.txt` claims `.data`
 * 0x806031A0.. (the callback set and the string run), `.sbss` 0x80794CE0..0x80794CE8 and `.bss`
 * 0x806D3650..0x806D3670; the labels were declared in `include/unsplit/Network.h` while the ranges
 * were unowned and sit in the owner's own header now (rule 2).  They stay under `extern "C"` so the
 * symbol names the object reports are the map's. */
extern "C" {
extern GT2ConnectionCallbacks sGameSpyConnectionCallbacks;   /* 0x806031A0 (.data) - the callbacks every connection gets */
extern u32   sGameSpyConnections[4];      /* 0x806D3650 (.bss) - the GT2 connection behind each receiver slot */
extern GameSpyNegotiation sNatNegState;   /* 0x806D3660 (.bss) - the NAT-negotiation record */
extern u32   sGameSpySocket;              /* 0x80794CE0 (.sbss) - the GT2 socket */
}

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

/* the GT2 reject message, the empty service-locator string and the listen-address format (shared
   `.sdata` pool entries no registered unit claims) */
extern const char sRejectMessageNG[3];

extern const char sEmptyString[4];

extern const char sPortFormat[4];

#ifdef __cplusplus
}
#endif

#endif
