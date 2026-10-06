/*
 * unsplit/Network.h - declarations for the Network band's callees that no registered unit owns.
 *
 * Filled in by the `8041A87C_fn_8041A87C` lane: the helpers the 0x8041A87C..0x8041DF10 range calls
 * (the debug manager, the DWC/GameSpy socket layer, the OS thread API and the neighbouring `fn_`
 * helpers) all live in address bands that are still unsplit, so per brief section 6.5 rule 2 this is
 * their legitimate home.  A symbol a registered unit owns is declared in that unit's header and
 * included from here instead (`DWCi/DWCi_NatNeg.h` is the first such case).
 */

#ifndef UNSPLIT_NETWORK_H
#define UNSPLIT_NETWORK_H

#include "types.h"

/* The band got its first *claimed* range (0x803CCDF8..0x803D3CE8, the Network transport units),
   so the declarations that range now owns moved into those units' own headers and are reached
   through this include - a band header that still declared them would collide with the owner's
   definitions (docs/plan.md 6.5 rule 2). */
#include "Network/NetworkPeerBase.h"
#include "Network/NetworkSessionBase.h"
#include "Network/NetworkStreamSink.h"
#include "Network/NetworkResolverWii.h"
#include "Network/network_socket_streams.h"
#include "Network/NetworkSessionStable.h"
#include "Network/sNetworkLibraryWii.h"
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/dwc_error.h"
#include "Network/net_session_close.h"
#include "Network/sNetworkLibrary.h"
#include "menu/menu_plsearch.h"

typedef struct NetworkErrorInfo NetworkErrorInfo;
typedef struct PatMatchOptions PatMatchOptions;   /* Network/NetworkSessionManager.h */
typedef struct NetId NetId;                 /* Network/NetworkLayerPat.h */
typedef struct NetLayerRequest NetLayerRequest; /* Network/NetworkLayerPat.h */
/* the record's layout lives in `Network/NetworkSessionManager.h`, beside the GameSpy handshake that
 * fills it: `NetworkInstance::postError` below only takes a pointer to it. */

/* ---- the game's debug/log manager (`getNetworkLogger` returns the singleton) ---------------------- */

/* The log manager is dispatched through, never constructed here, so it is a class with the real
 * virtuals and no vtable in our object: retail's `lwz r12, 0x0(r3)` / `lwz r12, 0xC(r12)` shape is
 * MWCC's virtual-call form, and a struct of function pointers loads through a scratch register
 * instead.  The slots are the target's own offsets (0x08 is the deleting destructor, then the log
 * entry points, `isVerbose`, `flag` and `encode`), so the unnamed ones between them are padding. */
class NetworkLogger {
public:
    /* +0x08 */ virtual void destroy_08(u32 flags);
    /* +0x0C */ virtual void signal_0C(u32 level, const char* fmt, ...);
    /* +0x10 */ virtual void warn_10(const char* fmt, ...);
    /* +0x14 */ virtual void log_14(const char* fmt, ...);
    /* +0x18 (GUESS: takes the log level 3 the pat control sets) */ virtual void setLevel_18(s32 level);
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual void pad_20();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual void pad_28();
    /* +0x2C */ virtual void pad_2C();
    /* +0x30 */ virtual void pad_30();
    /* +0x34 */ virtual void pad_34();
    /* +0x38 (the login step's account check: negative with the error filled, positive when done) */ virtual s32 checkAccount_38(s32 kind, NetworkErrorInfo* error);
    /* +0x3C */ virtual s32  isVerbose_3C();
    /* +0x40 */ virtual void pad_40();
    /* +0x44 */ virtual void pad_44();
    /* +0x48 */ virtual u16  flag_48(u16 value);
    /* +0x4C */ virtual u16  encode_4C(u32 value);
    /* +0x50 (GUESS: the transport stream readers' value decode) */ virtual u32 decode_50(u32 value);
    /* +0x54 (GUESS: the transport stream writers' value encode) */ virtual u32 encode_54(u32 value);
    /* +0x58 */ virtual void pad_58();
    /* +0x5C */ virtual void pad_5C();
    /* +0x60 */ virtual f32  getTime_60();
    /* +0x64 */ virtual void pad_64();
    /* +0x68 */ virtual void pad_68();
    /* +0x6C */ virtual void pad_6C();
    /* +0x70 */ virtual void pad_70();
    /* +0x74 */ virtual void pad_74();
    /* +0x78 (reads this console's match options into out) */ virtual void readMatchOptions_78(s32 kind, PatMatchOptions* out);
};   /* size: 0x04 (the object's leading vtable word) */

/* ---- the network singleton `getInstance_` returns -------------------------------------------- */

/* The singleton `getInstance_` returns is the `PatInterface` (`getInstance_` loads `mpInstance__12PatInterface`,
 * which `__ct__12PatInterfaceFv` stores; request net3-d-03c2#1): the band's spelling is an alias of the class,
 * whose layout and 161 virtual slots live in `Network/PatInterface.h`. */
class PatInterface;
typedef PatInterface NetworkInstance;

/* The DWC error record `postError` takes **by value** (a by-value parameter makes MWCC build the argument copy
 * and re-materialise the constants, which is retail's shape at every caller). */
typedef struct NetworkPostedError {
    /* +0x00 */ s32 code_00;
    /* +0x04 */ s32 param1_04;
    /* +0x08 */ s32 param2_08;
} NetworkPostedError;   /* size: 0x0C */

/* The session units' dispatch spelling of the same object (the `postError` slot +0x288 and its by-value
 * inline are `PatInterface`'s). */
typedef PatInterface NetworkInstanceDispatch;

extern "C" {

/* debug manager: `getNetworkLogger` is `Network/NetworkStreamSink.cpp`'s, declared in its header (included at the
 * top of this band). */

/* The socket pool (`networkSocketPool_acquire`/`_release`, owner `Network/sNetworkLibrary.cpp`) is
 * declared in `Network/sNetworkLibrary.h`, included at the top of this band. */

/* network singleton and its callbacks.  `getInstance_` (0x803768F0) is owned by
 * `enemy/em020_ai.cpp` now that its range is registered - rule 2: the declaration moved to the
owner's header and is included here.  The singleton's callback/reference/error accessors are owned by
`Network/PatInterface.cpp` and declared in `Network/PatInterface.h`. */
#include "enemy/em020_ai.h"

/* The GameSpy GT2 transport API and its types are `DWCi/dwc_nasfunc.cpp`'s, declared in
 * `DWCi/dwc_nasfunc.h` (included at the top of this band). */

/* The DWCi NATNEG / transport-tail unit (`.text` 0x80512490..0x805145B8) is registered as
 * `src/DWCi/DWCi_NatNeg.c`, so its five entry points now live in its owner header; including
 * it keeps the callers below compiling without redeclaring an owned symbol (rule 2). */
#include "DWCi/DWCi_NatNeg.h"

/* OS / runtime helpers.  `SOHtoNs` is an SO-library symbol whose one home is the SO band header,
 * which is C-linkage-safe and so reachable from the DWCi `.c` units as well; the OS mutex and thread
 * calls are `NAND/nand.c`'s (the NAND/OS SDK block) and are declared in `NAND/nand.h`. */
#include "unsplit/SO.h"
#include "NAND/nand.h"

/* 0x80794380 `natNegMessageMagic` - the NATNEG message signature this unit compares the head of a
 * received datagram against - is deliberately *not* declared here: the bytes belong to the NATNEG
 * unit, so rule 2 puts the declaration in `DWCi/DWCi_NatNeg.h` (included above), and that
 * is the *unsized* spelling this unit needs.  It addresses the symbol with `lis`/`addi`
 * (ADDR16_HA/LO, the target's relocation kind), while the owner's own source re-declares it sized
 * for the SDA21 form its ten sites use (playbook row 12 - the reloc kind is a codegen input). */

/* Three declarations that used to stand here are owned now, so each lives in its OWNER's header and is
 * reached through this band by including it (section 6.5 rule 2): `sGameSpySocket` and
 * `sGameSpyInterfaceThread` by `Network/GameSpyInterfaceThread.cpp`, whose `splits.txt` claims
 * `.sbss:0x80794CE0..0x80794CE8` - they are declared in `Network/GameSpyInterfaceThread.h`, the header of
 * the unit that defines them - and `natNegMessageMagic` by `DWCi/DWCi_NatNeg.c`, whose `.sdata` run
 * 0x80794368..0x807943A0 covers it (declared in `DWCi/DWCi_NatNeg.h`, included at the top of
 * this band).  A band header that still declared them would collide with the owners' definitions.
 */

/* ---- the layer request state machine's callees (0x803DECF0..0x803EF4C8, 0x8040144C..0x80401B68) ---- */

/* The request record `NetworkRequest_getError` copies out: the three words `NetworkRequest_setError`
 * stores under the request's mutex (+0x54/+0x58/+0x5C of `NetworkRequest`). */
typedef struct NetworkRequestError {
    u32 code_00;
    u32 arg_04;
    u32 arg_08;
} NetworkRequestError;   /* size: 0x0C */

class NetworkLayerPat;             /* Network/NetworkLayerPat.h */
class NetworkSessionManagerPat;    /* Network/NetworkSessionManager.h */
typedef struct NetworkRequest NetworkRequest;   /* Network/NetworkSessionManager.h */

/* The request/layer state machine's owned callees live in their owners' headers (rule 2):
 * `NetworkRequest_getError` in `Network/NetworkSessionManagerPat.h`, `notifyLayerEvent` in
 * `Network/NetworkCommunityPat.h`, the layer requests and `isMessageRestricted` in
 * `Network/PatInterface.h`, the Pat setters and `getGameTime` in `Network/PatInterface.h`, and the
 * terms entry points in `Network/NetworkWiiMediator.h`. */
struct PatTerms;
struct NetRosterSync;
struct NetworkPat;
}

/* `__dl__FPv`'s real spelling (the caller's `operator delete`); see the ef units' convention.
 * It is a C++ operator, so it is declared outside the `extern "C"` block. */
void operator delete(void* ptr) throw();

#endif
