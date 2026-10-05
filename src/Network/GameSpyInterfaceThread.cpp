/*
 * GameSpyInterfaceThread.cpp - the GameSpy interface / peer band, `.text` 0x8041B194..0x8041DF10.
 *
 * FILE NAME.  Registered under the map stem `fn_8041A87C.cpp`; renamed for the worker-thread class whose methods
 * fill the range (`__ct__22GameSpyInterfaceThreadFv`, `tGameSpyInterface`, `step`, `publishRequest`).  The class's
 * type-only header, read by the mediator band, is `Network/gamespy_interface_types.h`.
 *
 * WHAT IT IS.  The GT2 socket callbacks and three object types share the range: the worker thread
 * `GameSpyInterfaceThread`, `NetworkPeerGameSpy` and `NetworkTimedHandler`.  Each owns its bodies as members;
 * every call through a foreign object's vtable goes through a declared `virtual` on that class (the only
 * shape MWCC emits as `lwz r12, 0x0(r3)` / `lwz r12, <slot>(r12)`).
 *
 * SEAMS.  (1) The left edge 0x8041B194 is the recut of request net2-l4-cff5#1: `updateCallbackStep`..`applyEvent`
 * (0x8041A87C..0x8041B194) are `NetworkReflectService` members and moved to `Network/NetworkReflectService.cpp`
 * with their string 0x80603154 and the class table 0x80603190.  (2) The thread's vtable 0x806036A0 is followed by the peer's strings 0x806036B0: a
 * vtable-to-string seam at 0x8041D750 (`NetworkPeerGameSpy`, vtable 0x80603714).  (3) The timed handler's vtable
 * 0x80603740 rises above the peer's: a third TU at 0x8041DE20.
 *
 * FLAGS.  Per object in `configure.py`: `-O3` + `-inline noauto`, the lib's `-Cpp_exceptions on`, and `-pool off`
 * (playbook 43; measured there).  The file is `#pragma peephole off` throughout: retail keeps the unfused
 * `clrlwi`/`extsb` + `cmpwi` forms, so narrow stores are compound assignments and shift-and-mask bytes are
 * `(v >> 16) & 0xFF` (playbook 95).
 *
 * LOAD-BEARING SHAPES.  The vtable pointer sits where the first virtual is declared, so `~GameSpyInterfaceThread`
 * comes first in the class (playbook 98); being the key function it makes this unit emit `__vt__` and `__dt__`.
 * The error record is passed by value through `PatInterface`'s inline `postError` overload (playbook 97).
 * `unregisterReceiver` keeps an element pointer across its call (playbook 96); `executeError` copies the two
 * values it logs so the switch re-reads the stack; `isQueued` is the plain three-way return.  `profile_4485` is
 * five `u32` at the odd offset 0x4485 (the class's `#pragma pack(1)`); the connect call's out parameter is one
 * `GT2Connection`.  A `switch` whose cases all leave 0 is `break` + one trailing `return 0;` (playbook 34).
 *
 * DATA (INTERIM).  Claimed and emitted: `.data` 0x806031A0..0x80603714 (the callback set first, then the log
 * strings as literals in retail's order, then the thread vtable), `.bss` 0x806D3650..0x806D3670 and `.sbss`
 * 0x80794CE0..0x80794CE8.  The `.data` claim is interim: its left edge is the Reflect TU's end, but the right
 * edge 0x80603714 is probably false - the thread TU should end at 0x806036B0 (peer strings and vtable belong to
 * the peer TU).  Deferred by the data tools: `lbl_80603740` (span-blocked, it belongs to the timed-handler TU)
 * and the `.sdata` entries `sRejectMessageNG`, `sEmptyString`, `sPortFormat` (isolated run).  The `lbl_` name
 * stays: renaming an extern of unclaimed data adds a rule-12 finding.  The callback set is defined ahead of the first
 * literal because MWCC emits `.data` in definition order.
 *
 * NAMES.  The six `gt2*Callback` bodies and `DWC_GetLastErrorEx` come from the pool strings; every other name
 * for an unsplit callee (the `gt2*` API, `DWC_*`, `DWCi_natProbe*`, `sendReq*`, `hasMultipleRefs60d4`,
 * `errorRecordCode613c`, `getErrorInfo654c`, `getGameInfo2d1c`) and `natNegProgressCallback`,
 * `natNegCompletedCallback`, `copyGameSpyAddress`, `NetworkReflectService::notify` and the peer's
 * `armDrop`/`init` (the base's +0x20/+0x24 slots) is a GUESS from call shape or opcode.
 *
 * RESIDUALS.  `send`/`receive`: retail materialises the error-source constant as a relocation
 * (`@eti_80030018+9/+10`), ours is the immediate; `receive` also keeps a `clrlwi` ours drops.
 * `tGameSpyInterface`: the hoisted high word of `ticks * 17` is a fresh `li r0, 0` where retail reuses r24.
 * `NetworkTimedHandler::create`: retail leaves the `lis` half of the vtable address in r4 as `init`'s unused
 * first argument.  `init`, `publishRequest`: register colouring.  Sections: `.data` 1392 of 1396 B until the
 * cut, `extab` 360 of 380 B (a 20-byte cleanup record against `dtor_803CA338`, the peer's member-mutex
 * destructor, which the hand-modelled `NetworkPeerGameSpy::destroy` does not produce; retry a real
 * `NetworkPeerBase` derivation with a member mutex in the peer TU after the cut).
 */

#include "types.h"
#include "Network/GameSpyInterfaceThread.h"
#include "Runtime.PPCEABI.H/memcpy.h"    /* memcpy  - owner Runtime.PPCEABI.H/memcpy.c */
#include "Runtime.PPCEABI.H/memset.h"    /* memset  - owner Runtime.PPCEABI.H/memset.c */
#include "unsplit/Runtime.PPCEABI.H.h"   /* memmove / memcmp / snprintf - no registered owner */
#include "Network/NetworkWiiMediator.h"     /* getGameInfo2d1c - owner Network/NetworkWiiMediator.cpp */

/* retail keeps the unfused peephole forms across the whole band (see the header): `clrlwi`/`extsb` + `cmpwi`
 * in place of the recording forms, `clrlwi`+`slwi` in place of `clrlslwi`. */
#pragma peephole off

/* The debug manager's virtual slots: the target re-runs `bl getNetworkLogger` at *every* logging site
 * (never once per function), so each site expands to its own block that fetches the singleton and
 * dispatches through its vtable - a real virtual call, which is the only shape MWCC emits as
 * `lwz r12, 0x0(r3)` / `lwz r12, 0xC(r12)`. */
#define SIGNAL_LOG(...) do { NetworkLogger* lm = getNetworkLogger(); lm->signal_0C(__VA_ARGS__); } while (0)
#define WARN_LOG(...)   do { NetworkLogger* lm = getNetworkLogger(); lm->warn_10(__VA_ARGS__); } while (0)
#define INFO_LOG(...)   do { NetworkLogger* lm = getNetworkLogger(); lm->log_14(__VA_ARGS__); } while (0)

extern "C" {

/* ---- this unit's free (callback and entry-point) bodies, in address order --------------------- -
 * Every other body in the range is a member of one of the four classes, so those are declared in
 * include/Network/GameSpyInterfaceThread.h and not here.  The DWC callbacks are installed through
 * `(NetworkCallback)`, so they keep the C spelling and the flat parameter lists retail shows. */
void gt2SocketErrorCallback(void);
void natNegProgressCallback(void);
/* untyped: byte range - the received datagram */
s32 gt2UnrecognizedMessageCallback(NetworkInstance* self, u32 peer, u16 value, const void* data, u32 size);
void natNegCompletedCallback(s32 result, s32 unused, const GameSpyAddress* src, GameSpyResultInfo* info);
void copyGameSpyAddress(GameSpyAddress* out, const GameSpyAddress* in);
/* untyped: byte range - the peer profile */
void gt2ConnectAttemptCallback(s32 unused0, s32 socket, s32 unused1, s32 unused2, s32 unused3, const void* profile,
                 u32 size);
void gt2ConnectedCallback(s32 socket, s32 result, s32 unused, s32 timeout);
void gt2ReceivedCallback(u32 socket, s32 address, s32 size);
void gt2ClosedCallback(u32 socket, s32 result);
void gt2PingCallback(void);
s32 runThread(GameSpyInterfaceThread* self);

}

/* The callbacks every GT2 connection this unit accepts or opens gets. */
GT2ConnectionCallbacks sGameSpyConnectionCallbacks = {
    gt2ConnectedCallback, gt2ReceivedCallback, gt2ClosedCallback, gt2PingCallback
};

/* Drops every socket of the three-slot table, one request record at a time. */
extern "C" void gt2SocketErrorCallback(void)
{
    s32 i;

    SIGNAL_LOG(3, "SocketErrorCallback is called\n");
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        SIGNAL_LOG(3, "IGSInterface is NULL\n");
        return;
    }
    for (i = 0; i < 3; i++) {
        if (sGameSpyConnections[i] != 0) {
            GameSpyInterfaceThread::getInstance()->failRequest(-0x2DB0, 1, 0, (u8)i, -1);
            sGameSpyConnections[i] = 0;
        }
    }
}

/* Empty body: the socket table's accept callback placeholder, installed but never used. */
extern "C" void natNegProgressCallback(void)
{
}

/* Sends a framed GameSpy header through the DWC socket layer. */
/* untyped: byte range - the received datagram */
extern "C" s32 gt2UnrecognizedMessageCallback(NetworkInstance* self, u32 peer, u16 value, const void* data, u32 size)
{
    GameSpyHeader header;

    if (size == 0 || data == NULL) {
        return 0;
    }
    memset(&header, 0, sizeof(header));
    header.code_1 = 2;
    header.value_4 = peer;
    header.peer_2 = SOHtoNs(value);
    if (memcmp(data, natNegMessageMagic, 6) == 0) {
        DWCi_NatNegSendPacket((void*)data, size, &header);
        return 1;
    }
    return 0;
}

/* Maps a GameSpy connect result onto the interface's request state and error record. */
extern "C" void natNegCompletedCallback(s32 result, s32 unused, const GameSpyAddress* src, GameSpyResultInfo* info)
{
    s32 error;

    info->result_04 = 1;
    info->connected_00 = 0;
    error = 0;
    switch (result) {
    case 0:
        copyGameSpyAddress(&info->address_08, src);
        info->connected_00 = 1;
        SIGNAL_LOG(3, "NAT negotiation succeeded.\n");
        break;
    case 1:
        SIGNAL_LOG(3, "dearbeatpartner\n");
        error = -0x2DA6;
        break;
    case 2:
        SIGNAL_LOG(3, "inittimeout\n");
        error = -0x2DA7;
        break;
    case 3:
        SIGNAL_LOG(3, "pingtimeout\n");
        error = -0x2DA8;
        break;
    case 4:
        SIGNAL_LOG(3, "unknownerror\n");
        error = -0x2DA9;
        break;
    default:
        SIGNAL_LOG(3, "unknown\n");
        error = -0x2DA9;
        break;
    }
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        SIGNAL_LOG(3, "IGSInterface is NULL\n");
        return;
    }
    if (info->connected_00 == 0) {
        GameSpyInterfaceThread::getInstance()->setError(0x80000007, 0x5F, -error);
    }
}

/* Copies the 8-byte GameSpy header out of a received frame. */
extern "C" void copyGameSpyAddress(GameSpyAddress* out, const GameSpyAddress* in)
{
    out->first_00 = in->first_00;
    out->second_01 = in->second_01;
    out->port_02 = in->port_02;
    out->value_04 = in->value_04;
}

/* Handles a connect-attempt callback: validates the reply and publishes the socket. */
/* untyped: byte range - the peer profile */
extern "C" void gt2ConnectAttemptCallback(s32 unused0, s32 socket, s32 unused1, s32 unused2, s32 unused3,
                            const void* profile, u32 size)
{
    u32 peerId;
    s32 i;

    SIGNAL_LOG(3, "connectAttemptCallback is called.\n");
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        SIGNAL_LOG(3, "IGSInterface is NULL\n");
        return;
    }
    peerId = GameSpyInterfaceThread::getInstance()->getPeerId();
    if (GameSpyInterfaceThread::getInstance()->checkPeerProfile(profile, size) == 0) {
        gt2Reject(socket, sRejectMessageNG, 2);
        GameSpyInterfaceThread::getInstance()->publishRequest(-0x2DA0, 0xFF, peerId);
        return;
    }
    if (gt2Accept(socket, &sGameSpyConnectionCallbacks) != 0) {
        SIGNAL_LOG(3, "gt2Accept is succeeded.\n");
        for (i = 0; i < 3; i++) {
            if (sGameSpyConnections[i] == 0) {
                sGameSpyConnections[i] = socket;
                GameSpyInterfaceThread::getInstance()->publishRequest(0, (u8)i, peerId);
                break;
            }
        }
        if (i >= 3 && getInstance_() != NULL) {
            NetworkPostedError error;

            error.code_00 = 0x80000007;
            error.param1_04 = 0x5F;
            error.param2_08 = 0x2D6A;
            ((PatInterface*)getInstance_())->postError(error);
        }
    } else {
        GameSpyInterfaceThread::getInstance()->publishRequest(-0x2DAE, 0xFF, peerId);
    }
}

/* Handles a socket accept callback: registers the socket or fails the request. */
extern "C" void gt2ConnectedCallback(s32 socket, s32 result, s32 unused, s32 timeout)
{
    s32 i;
    s32 error;

    SIGNAL_LOG(3, "connectedCallback is called. result:%d\n", result);
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        SIGNAL_LOG(3, "IGSInterface is NULL\n");
        return;
    }
    if (result == 0) {
        for (i = 0; i < 3; i++) {
            if (sGameSpyConnections[i] == 0) {
                sGameSpyConnections[i] = socket;
                GameSpyInterfaceThread::getInstance()->publishRequest(0, (u8)i, 0);
                break;
            }
        }
        if (i >= 3 && getInstance_() != NULL) {
            NetworkPostedError error;

            error.code_00 = 0x80000007;
            error.param1_04 = 0x5F;
            error.param2_08 = 0x2D6A;
            ((PatInterface*)getInstance_())->postError(error);
        }
    } else {
        error = timeout > 0 ? -0x2DA0 : -0x2DAD;
        GameSpyInterfaceThread::getInstance()->publishRequest(error, 0xFF, 0);
    }
}

/* Forwards receive data to the socket's receiver slot. */
extern "C" void gt2ReceivedCallback(u32 socket, s32 address, s32 size)
{
    s32 i;

    if (size <= 0) {
        SIGNAL_LOG(3, "receivedCallback len is zero\n");
        return;
    }
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        SIGNAL_LOG(3, "IGSInterface is NULL\n");
        return;
    }
    for (i = 0; i < 3; i++) {
        if (socket == sGameSpyConnections[i]) {
            GameSpyInterfaceThread::getInstance()->dispatchReceiver((u8)i, address, size);
            return;
        }
    }
}

/* Handles a socket close callback: unregisters the socket and fails its record. */
extern "C" void gt2ClosedCallback(u32 socket, s32 result)
{
    s32 error;
    s32 i;

    SIGNAL_LOG(3, "closedCallback is called. reason:%d\n", result);
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        SIGNAL_LOG(3, "IGSInterface is NULL\n");
        return;
    }
    switch (result) {
    case 2:
        error = -0x2DAF;
        break;
    case 3:
        error = -0x2DB0;
        break;
    case 4:
        error = -0x2DB1;
        break;
    default:
        error = 0;
        break;
    }
    for (i = 0; i < 3; i++) {
        if (socket == sGameSpyConnections[i]) {
            GameSpyInterfaceThread::getInstance()->failRequest(error, 0, 0, (u8)i, -1);
            sGameSpyConnections[i] = 0;
            return;
        }
    }
}

/* Logs a ping callback. */
extern "C" void gt2PingCallback(void)
{
    SIGNAL_LOG(3, "pingCallback is called\n");
}

/* Stores the id the timed handler waits for. */
void GameSpyInterfaceThread::setWaitHandle(DWCSvlResult* result)
{
    stage_98 = 0;
    svlResult_94 = result;
}

/* Runs the NAS-login handshake the timed handler drives, one step per frame. */
s32 GameSpyInterfaceThread::runNasLogin()
{
    s32 result;
    s32 error;

    switch (stage_98) {
    case 0:
        if (DWC_NASLoginAsync() == 0) {
            stage_98 = 4;
            break;
        }
        stage_98 = stage_98 + 1;
        break;
    case 1:
        result = DWC_NASLoginProcess();
        switch (result) {
        case 3:
            SIGNAL_LOG(3, "NASLogin succeeded\n");
            stage_98 = stage_98 + 1;
            break;
        case 4:
            SIGNAL_LOG(3, "NASLogin failed\n");
            stage_98 = 4;
            break;
        case 5:
            SIGNAL_LOG(3, "NASLogin canceled\n");
            setError(0x80000000, 0, 0);
            stage_98 = 4;
            break;
        default:
            break;
        }
        break;
    case 2:
        if (DWC_SVLBegin() == 0) {
            DWC_SVLEnd();
            stage_98 = 4;
        } else if (DWC_SVLGetTokenAsync(sEmptyString, svlResult_94) == 0) {
            DWC_SVLEnd();
            stage_98 = 4;
        } else {
            stage_98 = stage_98 + 1;
        }
        break;
    case 3:
        result = DWC_SVLProcess();
        switch (result) {
        case 3:
            SIGNAL_LOG(3, "SVL succeeded\n");
            DWC_SVLEnd();
            return 1;
        case 4:
            SIGNAL_LOG(3, "SVL error\n");
            DWC_SVLEnd();
            stage_98 = 4;
            break;
        case 5:
            SIGNAL_LOG(3, "SVL canceled\n");
            DWC_SVLEnd();
            stage_98 = 4;
            break;
        default:
            break;
        }
        break;
    case 4:
        executeError();
        return -1;
    default:
        break;
    }
    return 0;
}

/* Hands the index's receiver slot to the object stored for it (a tail call). */
void GameSpyInterfaceThread::dispatchReceiver(u8 index, s32 a, s32 b)
{
    GameSpyReceiver* receiver;

    receiver = (GameSpyReceiver*)receivers_14[receiverState_24[index]];
    if (receiver == NULL) {
        return;
    }
    receiver->handle_18(a, b, 0, 0, 0);
}

/* Fails the index's request record when `error` is set, then closes the slot. */
void GameSpyInterfaceThread::failRequest(s32 error, s32 a, s32 b, u8 index, s32 e)
{
    if (error != 0) {
        setError(0x80000007, 0x5F, -error);
    }
    slotState_54[index] = -1;
}

/* Records the outcome of the DWC request the interface is waiting on. */
void GameSpyInterfaceThread::setRequestResult(s32 error, s32 value)
{
    running_6C = 2;
    if (error == 0) {
        value_30 = value;
        if (cancelPending_122 == 1) {
            state_2C = 2;
            return;
        }
        state_2C = 1;
        return;
    }
    setError(0x80000007, 0x5F, -error);
    armCancel();
    state_2C = -2;
}

/* Publishes a completed request: rewrites the slot tables and the negotiation result. */
void GameSpyInterfaceThread::publishRequest(s32 error, u8 index, u32 value)
{
    u8 i;

    if (error == 0) {
        slotIds_44[receiverCount_28 - 1] = value_30;
        slotState_54[receiverCount_28 - 1] = 1;
        if (value == 0) {
            value = peerId_4470;
        }
        for (i = 0; i < receiverCount_28; i++) {
            if (slotIds_44[i] == value) {
                if (i != index) {
                    slotIds_44[i] = 0;
                    if (sGameSpyConnections[i] != 0) {
                        gt2CloseConnection(sGameSpyConnections[i]);
                    }
                }
                break;
            }
        }
        slotIds_44[index] = value;
        slotState_54[index] = 1;
        receiverState_24[index] = 3;
        for (i = 0; i < receiverCount_28; i++) {
            if (receiverIds_34[i] == slotIds_44[index]) {
                receiverState_24[index] = i;
                break;
            }
        }
        negotiationDone_446E = 1;
        negotiationResult_446D = 1;
        return;
    }
    setError(0x80000007, 0x5F, -error);
    negotiationDone_446E = -1;
    negotiationResult_446D = 1;
}

/* Opens the GameSpy socket and installs the callback set, then applies the pending requests. */
void GameSpyInterfaceThread::ConnectToAnybody(s32 arg)
{
    char address[7];
    s32 phase;
    s32 state;

    state = 1;
    phase = state_2C;
    if (phase <= 0) {
        INFO_LOG("NetworkGameSpyInterface::ConnectToAnybody not login GameSpy:%d\n", phase);
        phase_74 = -1;
        return;
    }
    snprintf(address, 7, sPortFormat, (u16)bufferSize_4482);
    address[6] = 0;
    if (gt2CreateSocket(&sGameSpySocket, address, 0x2000, 0x2000, (NetworkCallback)gt2SocketErrorCallback) == 0) {
        gt2SetUnrecognizedMessageCallback(sGameSpySocket, (NetworkCallback)gt2UnrecognizedMessageCallback);
        gt2Listen(sGameSpySocket, (NetworkCallback)gt2ConnectAttemptCallback);
    } else {
        setError(0x80000007, 0x5F, 0x2DA2);
        state = 0;
    }
    if (state != 0 && closePending_123 != 0) {
        closePending_123 = 0;
        closeSession();
        state = 0;
    }
    if (cancelPending_122 != 0) {
        cancelPending_122 = 0;
        clearPendingClose();
        return;
    }
    if (state != 0) {
        phase_74 = 1;
        return;
    }
    phase_74 = -1;
}

/* Starts a GameSpy match for `count` players and stores the peer id it was given. */
s32 GameSpyInterfaceThread::startMatch(s32 count, u32 value, s32 a, u16 b,
                           s32 c, s32 d, s32 e)
{
    s32 phase;

    phase = phase_74;
    if (phase > 0) {
        INFO_LOG("NetworkGameSpyInterface::startMatch already call startMatch:%d\n", phase);
        return -1;
    }
    if (count <= 1) {
        return -1;
    }
    if (state_2C <= 0) {
        return -1;
    }
    paramA_7C = a;
    paramB_80 = b;
    paramC_84 = c;
    paramD_88 = d;
    paramE_8C = e;
    snprintf(name_9C, 0x80, "ik1='%d' and ik2='%d' and ik3='%d' and ik4='%d' and ik5='%d'", a, b, c, d, e);
    if (count > 4) {
        WARN_LOG("NetworkGameSpyInterface::startMatch match_num(%d) is greater than MAX_NUM_MATCH(%d)\n", count, 4);
        receiverCount_28 = 4;
    } else {
        receiverCount_28 = (u8)count;
    }
    value_30 = value;
    idByte_11C = value >> 24;
    idByte_11D = (value >> 16) & 0xFF;
    idByte_11E = (value >> 8) & 0xFF;
    idByte_11F = value & 0xFF;
    phase_74 = 2;
    openRequested_125 = 1;
    return 1;
}

/* Ticks the GameSpy clock and counts one more frame. */
s32 GameSpyInterfaceThread::updateClock()
{
    u32 time[9];

    getGameInfo2d1c(::getInstance(), time);
    DWCi_natProbeStart((const char*)time[3]);
    frame_70 = frame_70 + 1;
    return 1;
}

/* Moves the interface into its running state, or reports the shutdown. */
s32 GameSpyInterfaceThread::initialize()
{
    if (field_68 < 0) {
        state_2C = -1;
        return -1;
    }
    if (running_6C != 0) {
        return 0;
    }
    state_2C = 0;
    value_30 = 0;
    frame_70 = 0;
    phase_74 = 0;
    initRequested_124 = 1;
    return 0;
}

/* Reports whether a close is already pending, arming the step flag if so. */
bool GameSpyInterfaceThread::requestClose()
{
    if (closePending_123 != 0 || cancelPending_122 != 0) {
        stepRequested_126 = 1;
        return 1;
    }
    if (started_120 != 0) {
        stopRequested_121 = 1;
    }
    return started_120;
}

/* Returns the interface phase. */
s32 GameSpyInterfaceThread::getPhase()
{
    return phase_74;
}

/* Returns the running state, or -1 while the interface is not running. */
s32 GameSpyInterfaceThread::getState()
{
    if (running_6C != 0) {
        return state_2C;
    }
    return -1;
}

/* Clears the running state and the pending close. */
void GameSpyInterfaceThread::clearPendingClose()
{
    running_6C = 0;
    state_2C = 0;
    phase_74 = 0;
}

/* Arms the cancellation when a close arrives while the request is still in flight. */
void GameSpyInterfaceThread::armCancel()
{
    if (field_68 < 0) {
        return;
    }
    if (running_6C == 0) {
        return;
    }
    if (started_120 == 0) {
        return;
    }
    cancelPending_122 = 1;
    if (state_2C == 1) {
        state_2C = 2;
    }
}

/* Tears the socket, the session and the request pool down under the interface mutex. */
s32 GameSpyInterfaceThread::closeSession()
{
    OSLockMutex(mutex_4450);
    if (sessionOpen_4484 != 0) {
        DWCi_NatNegEndSession(session_447C);
        DWCi_NatNegCleanup();
        sessionOpen_4484 = 0;
    }
    if (sGameSpySocket != 0) {
        gt2CloseSocket(sGameSpySocket);
        memset(sGameSpyConnections, 0, 0x10);
        sGameSpySocket = 0;
    }
    OSUnlockMutex(mutex_4450);
    phase_74 = 0;
    return 0;
}

/* Reports whether the interface can be closed right now. */
s32 GameSpyInterfaceThread::canClose()
{
    if (field_68 < 0) {
        return -1;
    }
    if (state_2C != 1) {
        return -1;
    }
    if (phase_74 <= 0) {
        return -1;
    }
    if (started_120 != 0) {
        closePending_123 = 1;
        return -2;
    }
    return 0;
}

/* Replies to a pending request by filling the first free return handle. */
s32 GameSpyInterfaceThread::replyRequest(s32 handle)
{
    s32 i;

    if (field_68 < 0) {
        return -1;
    }
    if (state_2C != 1) {
        return -1;
    }
    if (phase_74 <= 0) {
        return -1;
    }
    if (started_120 != 0) {
        for (i = 0; i < receiverCount_28; i++) {
            if (slotHandles_58[i] == 0) {
                slotHandles_58[i] = handle;
                break;
            }
        }
        return -2;
    }
    return 0;
}

/* Resets the slot tables and the negotiation state. */
void GameSpyInterfaceThread::resetSlots()
{
    flag_78 = 0;
    memset(receivers_14, 0, 0x10);
    memset(receiverIds_34, 0, 0x10);
    memset(slotIds_44, 0, 0x10);
    memset(slotState_54, 0, 4);
    memset(slotHandles_58, 0, 0x10);
    negotiation_446C = 0;
    negotiationResult_446D = 0;
    negotiationDone_446E = 0;
    bufferSize_4482 = 0x2AF8;
    memset(receiverState_24, 3, 4);
    if (errorParam1_08 != 0x4B) {
        clearError();
    }
}

/* Stores the thread's vtable, publishes the singleton, resets the tables and starts its worker
 * thread.  The map row at 0x8041C66C is the mangled constructor name, and callers only reach it as
 * `new GameSpyInterfaceThread()`. */
GameSpyInterfaceThread::GameSpyInterfaceThread()
{
    sGameSpyInterfaceThread = this;
    field_68 = 0;
    running_6C = 0;
    frame_70 = 0;
    value_30 = 0;
    resetState();
    clearError();
    phase_74 = 0;
    started_120 = 0;
    initRequested_124 = 0;
    openRequested_125 = 0;
    stepRequested_126 = 0;
    result_90 = 0;
    stopRequested_121 = 0;
    mutexReady_444C = 0;
    memset(mutex_4450, 0, 0x18);
    GameSpyInterfaceThreadInit();
    memset(sGameSpyConnections, 0, 0x10);
    sGameSpySocket = 0;
    sessionOpen_4484 = 0;
}

/* Deleting destructor: restores the base vtable, empties the singleton and frees on request. */
GameSpyInterfaceThread::~GameSpyInterfaceThread()
{
    onDestroy();
    sGameSpyInterfaceThread = NULL;
}

/* Resets the per-request tables and the pending-request flags. */
void GameSpyInterfaceThread::resetState()
{
    flag_78 = 0;
    memset(receivers_14, 0, 0x10);
    memset(receiverIds_34, 0, 0x10);
    memset(slotIds_44, 0, 0x10);
    memset(slotState_54, 0, 4);
    memset(slotHandles_58, 0, 0x10);
    receiverCount_28 = 0;
    state_2C = 0;
    cancelPending_122 = 0;
    closePending_123 = 0;
    memset(&busy_128, 0, 4);
    field_4468 = 0;
    negotiation_446C = 0;
    negotiationResult_446D = 0;
    negotiationDone_446E = 0;
}

/* Empty body: the interface thread's vtable cleanup hook. */
void GameSpyInterfaceThread::onDestroy()
{
}

/* Registers a receiver for `id` in the first free slot. */
s32 GameSpyInterfaceThread::registerReceiver(NetworkPeerGameSpy* receiver, u32 id)
{
    u8 slot;
    u8 i;

    if (receiver == NULL) {
        return -1;
    }
    for (slot = 0; slot < receiverCount_28; slot++) {
        if (receivers_14[slot] == 0) {
            break;
        }
    }
    if (slot >= receiverCount_28) {
        return -2;
    }
    receivers_14[slot] = (u32)receiver;
    receiverIds_34[slot] = id;
    for (i = 0; i < receiverCount_28; i++) {
        if (id == slotHandles_58[i]) {
            break;
        }
    }
    if (i >= receiverCount_28) {
        for (i = 0; i < receiverCount_28; i++) {
            if (id == slotIds_44[i]) {
                receiverState_24[i] = slot;
                break;
            }
        }
    }
    return slot;
}

/* Releases the receiver slot at `index` and clears its id. */
void GameSpyInterfaceThread::unregisterReceiver(s32 index)
{
    u8 i;

    u32* slot = &receiverIds_34[index];
    replyRequest((s32)*slot);
    for (i = 0; i < receiverCount_28; i++) {
        if (index == receiverState_24[i]) {
            receiverState_24[i] = 3;
            break;
        }
    }
    receivers_14[index] = 0;
    *slot = 0;
}

/* Returns the state of the slot `id` maps to, or the pending negotiation result. */
u8 GameSpyInterfaceThread::getSlotState(u32 id)
{
    u8 i;

    for (i = 0; i < receiverCount_28; i++) {
        if (slotIds_44[i] == id) {
            return slotState_54[i];
        }
    }
    if (negotiationResult_446D != 0 && id != value_30 &&
        (id == peerId_4470 || id == selfPeerId_4474)) {
        return (u8)negotiationDone_446E;
    }
    return 0;
}

/* Returns the slot index `id` maps to, or -1. */
s8 GameSpyInterfaceThread::findSlot(u32 id)
{
    s8 i;

    for (i = 0; i < receiverCount_28; i++) {
        if (slotIds_44[i] == id) {
            return i;
        }
    }
    return -1;
}

/* Runs the interface's per-frame step: the negotiation sub-machine, the request sweep and the
   close/cancel bookkeeping. */
void GameSpyInterfaceThread::step()
{
    u32 connection;
    s32 r;
    s32 error;
    s32 i;
    s8 slot;

    if (running_6C != 0 && state_2C >= 0) {
        OSLockMutex(mutex_4450);
        if (frame_70 == 1 && state_2C == 0) {
            r = DWCi_natProbePoll();
            if (r != 0) {
                if (r == 1) {
                    setRequestResult(0, 0);
                } else {
                    setRequestResult(-0x2DA1, 0);
                }
            }
        }
        if (field_68 == 0 && state_2C == 1 && phase_74 == 1 && sGameSpySocket != 0) {
            if (negotiation_446C != 0) {
                switch (negotiationStep_4480) {
                case 0:
                    peerMatch_4478 = (value_30 != peerId_4470);
                    if (sNatNegState.result_04 == 0) {
                        session_447C = peerId_4470 ^ selfPeerId_4474;
                        r = DWCi_NatNegStartSession(gt2GetSocketSOCKET(sGameSpySocket), session_447C, peerMatch_4478,
                                        (NetworkCallback)natNegProgressCallback, (NetworkCallback)natNegCompletedCallback,
                                        &sNatNegState);
                        if (r != 0) {
                            error = 0;
                            switch (r) {
                            case 1:
                                error = -0x2DA3;
                                break;
                            case 2:
                                error = -0x2DA4;
                                break;
                            case 3:
                                error = -0x2DA5;
                                break;
                            default:
                                break;
                            }
                            sNatNegState.result_04 = 1;
                            sNatNegState.active_00 = 0;
                            setError(0x80000007, 0x5F, -error);
                        }
                        sessionOpen_4484 = 1;
                        negotiationStep_4480 = 1;
                    } else {
                        negotiationStep_4480 = 2;
                    }
                    break;
                case 1:
                    if (sNatNegState.result_04 != 0) {
                        DWCi_NatNegCleanup();
                        sessionOpen_4484 = 0;
                        negotiationStep_4480 = 2;
                    }
                    break;
                case 2:
                    if (sNatNegState.active_00 != 0) {
                        if (peerMatch_4478 == 1) {
                            if (gt2Connect(sGameSpySocket, &connection,
                                            DWCi_formatAddress(sNatNegState.session_0C,
                                                        DWCi_htons(sNatNegState.encoded_0A), NULL),
                                            profile_4485, 0x14, 0x2710, &sGameSpyConnectionCallbacks, 0) == 0) {
                                negotiationStep_4480 = 3;
                                break;
                            }
                            setError(0x80000007, 0x5F, 0x2DAD);
                        }
                        negotiationDone_446E = -1;
                        negotiationResult_446D = 1;
                    } else {
                        negotiationDone_446E = -1;
                        negotiationResult_446D = 1;
                    }
                    negotiation_446C = 0;
                    break;
                case 3:
                    if (negotiationDone_446E != 0) {
                        negotiation_446C = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            DWCi_NatNegProcess();
            gt2Think(sGameSpySocket);
        }
        OSUnlockMutex(mutex_4450);
    }
    if (closePending_123 != 0) {
        closePending_123 = 0;
        if (field_68 == 0 && state_2C >= 1 && phase_74 > 0) {
            closeSession();
        }
    }
    for (i = 0; i < receiverCount_28; i++) {
        if (slotHandles_58[i] != 0) {
            slot = findSlot(slotHandles_58[i]);
            if (slot < 0) {
                slotHandles_58[i] = 0;
            } else {
                if (sGameSpyConnections[slot] != 0) {
                    gt2CloseConnection(sGameSpyConnections[slot]);
                }
                slotIds_44[slot] = 0;
                slotState_54[slot] = 0;
                slotHandles_58[i] = 0;
            }
        }
    }
    if (cancelPending_122 != 0) {
        if (field_68 == 0) {
            if (running_6C != 0) {
                if (frame_70 == 1 && (state_2C == 2 || state_2C == -2)) {
                    cancelPending_122 = 0;
                    clearPendingClose();
                }
            } else {
                cancelPending_122 = 0;
            }
        } else {
            cancelPending_122 = 0;
        }
    }
}

/* Returns the request result, arming the step flag first. */
s32 GameSpyInterfaceThread::getResult()
{
    if (field_68 < 0) {
        return -1;
    }
    stepRequested_126 = 1;
    return result_90;
}

/* Drains the DWC error queue and folds the reported type into the interface state. */
s32 GameSpyInterfaceThread::executeError()
{
    s32 code;
    s32 type;

    if (DWC_GetLastErrorEx(&code, &type) != 0 && code < 0 && type != 0) {
        {
            s32 loggedCode = code;
            s32 loggedType = type;

            INFO_LOG("NetworkGameSpyInterface::executeError DWC_GetLastErrorEx ErrorCode:%d ErrorType:%d\n",
                     loggedCode, loggedType);
        }
        switch (type) {
        case 1:
            setError(0x80000007, 0x4A, -code);
            DWC_ClearError();
            break;
        case 2:
            setError(0x80000007, 0x49, -code);
            DWC_ClearError();
            break;
        case 3:
            setError(0x80000007, 0x49, -code);
            errorReported_10 = 0;
            state_2C = code;
            if (running_6C != 0) {
                if (sessionOpen_4484 != 0) {
                    DWCi_NatNegEndSession(session_447C);
                    DWCi_NatNegCleanup();
                    sessionOpen_4484 = 0;
                }
                if (sGameSpySocket != 0) {
                    gt2CloseSocket(sGameSpySocket);
                    memset(sGameSpyConnections, 0, 0x10);
                    sGameSpySocket = 0;
                }
                running_6C = 0;
            }
            phase_74 = 0;
            DWC_ClearError();
            break;
        case 6:
            setError(0x80000007, 0x49, -code);
            errorReported_10 = 0;
            field_68 = -1;
            state_2C = code;
            if (running_6C != 0) {
                if (sessionOpen_4484 != 0) {
                    DWCi_NatNegEndSession(session_447C);
                    DWCi_NatNegCleanup();
                    sessionOpen_4484 = 0;
                }
                if (sGameSpySocket != 0) {
                    gt2CloseSocket(sGameSpySocket);
                    memset(sGameSpyConnections, 0, 0x10);
                    sGameSpySocket = 0;
                }
                running_6C = 0;
            }
            phase_74 = 0;
            DWC_ClearError();
            break;
        case 7:
            errorReported_10 = 1;
            setError(0x80000007, 0x4B, -code);
            errorReported_10 = 0;
            field_68 = -1;
            state_2C = code;
            break;
        default:
            break;
        }
        return type;
    }
    return 0;
}

/* Copies the interface's error record out for the game to read. */
void GameSpyInterfaceThread::getErrorStruct(NetworkErrorInfo* out)
{
    if (out != NULL) {
        out->code_00 = errorCode_04;
        out->param1_04 = errorParam1_08;
        out->param2_08 = errorParam2_0C;
    }
}

/* Clears the error record and marks it reported. */
void GameSpyInterfaceThread::clearError()
{
    memset(&errorCode_04, 0, 0xC);
    errorReported_10 = 1;
}

/* Stores the first error the interface sees, unless it has already been reported. */
void GameSpyInterfaceThread::setError(s32 code, s32 a, s32 b)
{
    if (errorCode_04 == 0 || errorReported_10 != 0) {
        errorCode_04 = code;
        errorParam1_08 = a;
        errorParam2_0C = b;
    }
}

/* Sends a buffer out over the socket the index maps to. */
/* untyped: byte range - the datagram */
s32 GameSpyInterfaceThread::sendUnreliable(u8 index, const void* data, s32 size)
{
    s32 sent;

    sent = 0;
    if (field_68 < 0 || state_2C != 1 || phase_74 <= 0) {
        INFO_LOG("NetworkGameSpyInterface::sendUnreliable DWC_SendUnreliable Error over time size %d\n", size);
    } else {
        if (mutexReady_444C != 0) {
            OSLockMutex(mutex_4450);
            if (sGameSpyConnections[index] != 0) {
                sent = size;
                gt2Send(sGameSpyConnections[index], data, size, 0);
            }
            OSUnlockMutex(mutex_4450);
        } else {
            if (sGameSpyConnections[index] != 0) {
                sent = size;
                gt2Send(sGameSpyConnections[index], data, size, 0);
            }
        }
    }
    return sent;
}

/* The worker thread's entry point. */
extern "C" s32 runThread(GameSpyInterfaceThread* self)
{
    self->tGameSpyInterface((s32)self);
    return 0;
}

/* The worker thread's body: drains the request flags until the stop flag is set. */
void GameSpyInterfaceThread::tGameSpyInterface(s32 arg)
{
    u64 ticks;

    OSInitMutex(mutex_4450);
    mutexReady_444C = 1;
    for (;;) {
        if ((s8)initRequested_124 != 0) {
            running_6C = 1;
            updateClock();
            initRequested_124 = 0;
        } else if ((s8)openRequested_125 != 0) {
            ConnectToAnybody(arg);
            openRequested_125 = 0;
        } else if ((s8)stepRequested_126 != 0) {
            step();
            stepRequested_126 = 0;
        }
        if (stopRequested_121 != 0) {
            stopRequested_121 = 0;
            started_120 = 0;
            break;
        }
        ticks = (u64)(*(volatile u32*)0x800000F8 / 4 / 1000);
        OSSleepTicks(ticks * 17);
    }
    mutexReady_444C = 0;
    INFO_LOG("NetworkGameSpyInterface::tGameSpyInterface GameSpyInterfaceThread End\n");
}

/* Spawns the interface's worker thread. */
s32 GameSpyInterfaceThread::GameSpyInterfaceThreadInit()
{
    s32 thread;

    thread = OSCreateThread(thread_130, runThread, this, &threadParam_4448, 0x4000, 0x0E, 1);
    if (thread != 0) {
        started_120 = 1;
        threadParam_4448 = 0;
        OSResumeThread(thread_130);
        INFO_LOG("NetworkGameSpyInterface::GameSpyInterfaceThreadInit GameSpyInterfaceThread Start\n");
    } else {
        INFO_LOG("NetworkGameSpyInterface::GameSpyInterfaceThreadInit GameSpyInterfaceThread Fail\n");
    }
    return thread;
}

/* Sets the socket address buffer size. */
void GameSpyInterfaceThread::setBufferSize(s16 size)
{
    bufferSize_4482 = size;
}

/* Starts a NAT negotiation between the two peer ids. */
void GameSpyInterfaceThread::startNegotiation(const GameSpyPeerId* a, const GameSpyPeerId* b)
{
    if (a != NULL && b != NULL && negotiation_446C == 0) {
        negotiationResult_446D = 0;
        negotiationDone_446E = 0;
        peerId_4470 = a->peerId_00;
        selfPeerId_4474 = b->peerId_00;
        sNatNegState.result_04 = 0;
        if (a->mode_04 == b->mode_04) {
            sNatNegState.result_04 = 1;
            sNatNegState.active_00 = 1;
            sNatNegState.session_0C = a->session_08;
            {
                NetworkLogger* lm = getNetworkLogger();
                sNatNegState.encoded_0A = lm->encode_4C(a->port_0C);
            }
        }
        profile_4485[0] = selfPeerId_4474;
        profile_4485[1] = paramA_7C;
        profile_4485[2] = paramB_80;
        profile_4485[3] = paramC_84;
        profile_4485[4] = paramD_88;
        negotiationStep_4480 = 0;
        negotiation_446C = 1;
    }
}

/* Reports whether a negotiation is running. */
u8 GameSpyInterfaceThread::isNegotiating()
{
    return negotiation_446C;
}

/* Reports the negotiation's outcome. */
u8 GameSpyInterfaceThread::getNegotiationResult()
{
    return (u8)negotiationDone_446E;
}

/* Compares a received peer profile against the one this interface published. */
/* untyped: byte range - the received peer profile */
s32 GameSpyInterfaceThread::checkPeerProfile(const void* profile, u32 size)
{
    if (size == 0x14) {
        if (memcmp(profile, profile_4485, 0x14) == 0) {
            return 1;
        }
        if (memcmp(profile, profile_4485, 4) != 0) {
            SIGNAL_LOG(3, "message does not fit profile_id.\n");
        }
        if (memcmp((const u8*)profile + 4, (const u8*)profile_4485 + 4, 0x10) != 0) {
            SIGNAL_LOG(3, "message does not fit position.\n");
        }
    } else {
        SIGNAL_LOG(3, "message does not fit len[%d].\n", size);
    }
    return 0;
}

/* Returns this interface's own peer id. */
u32 GameSpyInterfaceThread::getPeerId()
{
    return selfPeerId_4474;
}

/* Binds the peer to the interface and resets both buffers. */
void NetworkPeerGameSpy::bind(const u32* id)
{
    interface_6634 = (GameSpyInterfaceThread*)id[0];
    peer_6638 = id[1];
    field_6630 = interface_6634->registerReceiver(this, peer_6638);
    memset(sendBuffer_14, 0, 0x600);
    memset(recvBuffer_614, 0, 0x6000);
    received_10 = 0;
}

/* Builds and sends a framed peer message from the two optional payloads. */
s32 NetworkPeerGameSpy::send(const u16* a, s32 aLen, const u16* b, s32 bLen,
                           s8 flag)
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
/* untyped: byte range - the two caller buffers the framed payloads are copied into */
s32 NetworkPeerGameSpy::receive(void* a, s32* aLen, void* b, s32* bLen,
                           u8* flag)
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
    LockMutex(mutex_6614);
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
        UnlockMutex(mutex_6614);
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
    UnlockMutex(mutex_6614);
    return consumed;
}

/* Appends a buffer to the peer's receive queue. */
/* untyped: byte range - the datagram appended to the receive queue */
s32 NetworkPeerGameSpy::put(const void* data, u32 size)
{
    LockMutex(mutex_6614);
    if (received_10 + size > 0x6000) {
        INFO_LOG("NetworkPeerGameSpy::put: buf_recv_peer over. please check NetworkPeerGameSpy::MAX_SIZE_BUF_PEER\n");
        UnlockMutex(mutex_6614);
        return -1;
    }
    memcpy(recvBuffer_614 + received_10, data, size);
    received_10 = received_10 + size;
    UnlockMutex(mutex_6614);
    return 0;
}

/* Reports whether the peer has a message queued. */
s32 NetworkPeerGameSpy::isQueued()
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
    ((NetworkPeerCallback*)this)->tick_28();
    return 1;
}

/* Releases the peer's interface slot and drops its receive queue. */
void NetworkPeerGameSpy::release()
{
    interface_6634->unregisterReceiver(field_6630);
    LockMutex(mutex_6614);
    memset(recvBuffer_614, 0, 0x6000);
    received_10 = 0;
    UnlockMutex(mutex_6614);
}

/* Deleting destructor: destroys the queue mutex and the base, then frees on request. */
NetworkPeerGameSpy* NetworkPeerGameSpy::destroy(s16 flags)
{
    if (this != NULL) {
        dtor_803CA338(mutex_6614, -1);
        /* C cast: this class still hand-models `void* vtable_00`, unrelated to NetworkPeerBase; the measured alternatives failed. */
        ((NetworkPeerBase*)this)->NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Constructs the timed handler. */
NetworkTimedHandler* NetworkTimedHandler::create()
{
    vtable_00 = lbl_80603740;
    init((s32)lbl_80603740, 0, 0);
    return this;
}

/* Deleting destructor for the timed handler. */
NetworkTimedHandler* NetworkTimedHandler::destroy(s16 flags)
{
    if (this != NULL && flags > 0) {
        operator delete(this);
    }
    return this;
}

/* Initialises the timed handler's interval, limit and timeout. */
void NetworkTimedHandler::init(s32 a, s32 b, s32 c)
{
    clear();
    timeout_14 = 1000;
    interval_08 = b;
    limit_0C = c;
}

/* Clears the timed handler's state, ready flag and expiry flag. */
void NetworkTimedHandler::clear()
{
    state_04 = 0;
    ready_10 = 0;
    expired_18 = 0;
}

/* ---- the zero-initialised data this unit owns (declared in include/Network/GameSpyInterfaceThread.h; the
 * callback set is defined above, ahead of the first log string, because MWCC emits `.data` in
 * definition order and retail's set precedes the whole string run) ------------------------------- */

extern "C" {

u32 sGameSpyConnections[4];
GameSpyNegotiation sNatNegState;
u32 sGameSpySocket;
GameSpyInterfaceThread* sGameSpyInterfaceThread;

}
