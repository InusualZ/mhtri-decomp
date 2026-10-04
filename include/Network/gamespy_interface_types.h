/*
 * include/Network/gamespy_interface_types.h - the GameSpy worker thread class and its error record.
 *
 * Owner: `src/Network/GameSpyInterfaceThread.cpp`, which defines the constructor (`__ct__22GameSpyInterfaceThreadFv`,
 * 0x8041C66C), the destructor (the class's key function, so that unit emits `__vt__22GameSpyInterfaceThread`)
 * and every member below except `getInstance` (defined by `Network/NetworkSessionManager.cpp`).  A
 * type-only header (no free symbol is declared here) so the mediator band can name the class without
 * the GameSpy band's whole header (`Network/GameSpyInterfaceThread.h`).
 *
 * The size is the allocation both `new GameSpyInterfaceThread()` sites make (0x44A0: the mediator's
 * `initializeNetworkMediator` and the Pat session manager's constructor); the field run ends at +0x4499
 * under `#pragma pack(1)`, so the tail is padding.
 *
 * The one view: the Pat session units' former interim view of this class and its record is folded in - the error
 * record is 12 bytes (a 0x10 record lowers 10 Pat rows, measured), `requestClose` returns `bool` (a `u8` makes both
 * callers re-extend it) and `started_120` is the `bool` it returns.
 */
#ifndef MHTRI_NETWORK_GAMESPY_INTERFACE_TYPES_H
#define MHTRI_NETWORK_GAMESPY_INTERFACE_TYPES_H

#include "types.h"

/* the records the members take pointers to (defined in `Network/GameSpyInterfaceThread.h`) */
struct GameSpyPeerId;
class NetworkPeerGameSpy;

/* --------------------------------------------------------------------------------------------- */
/* the GameSpy worker thread object (the string pool spells its methods `NetworkGameSpyInterface::<method>`) */
/* --------------------------------------------------------------------------------------------- */

/* The DWC service-locator result `DWC_SVLGetTokenAsync` fills (the SDK's `DWCSvlResult`: a status, the host and
 * the token): `GameSpyInterfaceThread::runNasLogin` hands the one the mediator keeps to the SDK.  size: 0x174 */
typedef struct DWCSvlResult {
    /* +0x000 */ s32  status;
    /* +0x004 */ char svlhost[65];
    /* +0x045 */ char svltoken[301];
    /* +0x172 */ u8   pad_172[0x02];
} DWCSvlResult;

/* The error triple `GameSpyInterfaceThread::getErrorStruct` fills (three words) and the Pat band copies and
 * forwards: +0x04 is compared with 75 signed.  size: 0xC - `NetworkSessionManagerPat::move`'s frame gives the record
 * 12 bytes (0x14..0x20). */
typedef struct NetworkErrorInfo {
    /* +0x00 */ s32 code_00;
    /* +0x04 */ s32 param1_04;
    /* +0x08 */ s32 param2_08;
} NetworkErrorInfo;

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
    /* +0x0094 */ DWCSvlResult* svlResult_94;
    /* +0x0098 */ s32 stage_98;
    /* +0x009C */ char name_9C[0x80];
    /* +0x011C */ u8  idByte_11C;
    /* +0x011D */ u8  idByte_11D;
    /* +0x011E */ u8  idByte_11E;
    /* +0x011F */ u8  idByte_11F;
    /* +0x0120 */ bool started_120;      /* `requestClose` returns it as its bool result */
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
    /* +0x4499 */ u8  pad_4499[0x07];

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
    bool  requestClose();
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
    /* sets the service-locator result the NAS login fills */
    void  setWaitHandle(DWCSvlResult* result);
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
};   /* size: 0x44A0 (the allocation `new GameSpyInterfaceThread` makes at both sites) */
#pragma pack()

#endif /* MHTRI_NETWORK_GAMESPY_INTERFACE_TYPES_H */
