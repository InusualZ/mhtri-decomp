/*
 * Network/GameSpyInterfaceThread.h - the classes and data of `Network/GameSpyInterfaceThread.cpp`.
 * SHAPES. The foreign objects the target dispatches through (`GameSpyReceiver`, `NetworkLogger` in `unsplit/Network.h`,
 *   `PatInterface`) are classes with real virtuals, never constructed here, so no table is emitted for them.
 *   `NetworkPeerGameSpy` and `NetworkTimedHandler` are defined in `Network/NetworkPeerGameSpy.h`, included in place.
 */

#ifndef MHTRI_NETWORK_GAMESPYINTERFACETHREAD_H
#define MHTRI_NETWORK_GAMESPYINTERFACETHREAD_H

#include "types.h"
#include "Network/NetworkConnection.h"      /* NetworkMutex - the member mutex */
#include "Network/NetworkPeerBase.h"            /* NetworkPeerBase - the peer base class */
#include "unsplit/Network.h"
#include "Network/PatInterface.h"             /* isCallback / resetCallback / the error accessors - owner Network/PatInterface.cpp */
#include "Network/NetworkReflectService.h"    /* NetworkReflectService / GameSpyChannel - owner Network/NetworkReflectService.cpp */
#include "Network/gamespy_interface_types.h"   /* the worker thread class and its error record */
#include "DWCi/gt2Utility.h"                   /* DWCi_htons / DWCi_formatAddress - owner DWCi/gt2Utility.c */
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

/* `NetworkTimedHandler` and `NetworkPeerGameSpy` are `Network/NetworkPeerGameSpy.cpp`'s classes. */
#include "Network/NetworkPeerGameSpy.h"

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

#endif
