/*
 * Network/GameSpyInterfaceThread.h - the classes and data of `Network/GameSpyInterfaceThread.cpp`.
 * SHAPES. The foreign objects the target dispatches through (`GameSpyReceiver`, `NetworkPeerCallback`, `NetworkLogger`
 *   in `unsplit/Network.h`, `PatInterface`) are classes with real virtuals, never constructed here, so no table is
 *   emitted for them; the peer's and the timed handler's tables (0x80603714, 0x80603740) belong to other TUs.
 */

#ifndef MHTRI_NETWORK_GAMESPYINTERFACETHREAD_H
#define MHTRI_NETWORK_GAMESPYINTERFACETHREAD_H

#include "types.h"
#include "unsplit/Network.h"
#include "Network/PatInterface.h"             /* isCallback / resetCallback / the error accessors - owner Network/PatInterface.cpp */
#include "Network/NetworkReflectService.h"    /* NetworkReflectService / GameSpyChannel - owner Network/NetworkReflectService.cpp */
#include "Network/gamespy_interface_types.h"   /* the worker thread class and its error record */
#include "DWCi/fn_805113B0.h"                  /* DWCi_htons / DWCi_formatAddress - owner DWCi/fn_805113B0.c */
#include "sound/fn_800E46E8.h"                 /* getInstance - owner sound/fn_800E46E8.cpp */

/* 0x80794CE4 (.sbss) - the live GameSpyInterfaceThread this unit defines (named through the elaborated specifier,
   the spelling every includer sees). */
extern "C" class GameSpyInterfaceThread* sGameSpyInterfaceThread;

/* the records the two interface classes take pointers to, defined further down */
struct GameSpyPeerId;
class NetworkPeerGameSpy;
union GameSpyEventMsg;
/* --------------------------------------------------------------------------------------------- */
/* GameSpy interface state machine - the connect/NAT sub-machines                                 */
/* --------------------------------------------------------------------------------------------- */

/* `NetworkReflectService` (five of its members are this unit's) and its channel record `GameSpyChannel`
 * are defined in the reflect service's owner header, `Network/NetworkReflectService.h`, included above. */

/* The GameSpy worker thread `GameSpyInterfaceThread` and its error record `NetworkErrorInfo` are defined in
 * `Network/gamespy_interface_types.h` (included above), so the mediator band can include the class alone. */

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

/* the peer thread's NAT-negotiation record (`sNatNegState`) */
typedef struct GameSpyNegotiation {
    /* +0x00 */ u32 active_00;
    /* +0x04 */ u32 result_04;
    /* +0x08 */ u16 value_08;
    /* +0x0A */ u16 encoded_0A;
    /* +0x0C */ u32 session_0C;
} GameSpyNegotiation;   /* size: 0x10 */


/* --------------------------------------------------------------------------------------------- */
/* The data this unit owns, defined at the foot of `GameSpyInterfaceThread.cpp`.  `splits.txt` claims `.data`
 * 0x806031A0.. (the callback set and the string run), `.sbss` 0x80794CE0..0x80794CE8 and `.bss`
 * 0x806D3650..0x806D3670; the labels were declared in `unsplit/Network.h` while the ranges
 * were unowned and sit in the owner's own header now (rule 2).  They stay under `extern "C"` so the
 * symbol names the object reports are the map's. */
extern "C" {
extern GT2ConnectionCallbacks sGameSpyConnectionCallbacks;   /* 0x806031A0 (.data) - the callbacks every connection gets */
extern u32   sGameSpyConnections[4];      /* 0x806D3650 (.bss) - the GT2 connection behind each receiver slot */
extern GameSpyNegotiation sNatNegState;   /* 0x806D3660 (.bss) - the NAT-negotiation record */
extern u32   sGameSpySocket;              /* 0x80794CE0 (.sbss) - the GT2 socket */
}

/* Declarations moved here from `unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
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
