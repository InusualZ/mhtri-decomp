/*
 * include/Network/PatInterface.h - the declarations `src/Network/PatInterface.cpp` owns
 * (`.text` 0x803FCC34..0x803FE8E4, the `PatInterface` singleton's C-linkage surface).
 *
 * The unit has no bodies yet, so every parameter list is the one its callers' calls demonstrate; the
 * first parameter is the singleton (`this` in r3 - the target bodies read it), spelled the way each
 * caller holds it: `PatInterface` (the mediator band), `NetworkInstance` (what `getInstance_` returns)
 * or `NetworkStateMachine` (the state machine's view).  All three name the same object; settling one
 * spelling is the bodies' pass.  Moved here from `include/unsplit/Network.h`,
 * `include/Network/network_state.h` and `src/Network/NetworkWiiMediator.cpp` (docs/plan.md 6.5 rule 2:
 * the owner declares).
 *
 * One signature disagreement is left at its consumer: `include/Network/session_mediator_views.h` keeps
 * the no-argument `PatInterface_clear`/`PatInterface_isReady` the Pat session units call (the target
 * passes the singleton there too; spelling it changes those units' code - see that header).
 */
#ifndef MHTRI_NETWORK_PATINTERFACE_H
#define MHTRI_NETWORK_PATINTERFACE_H

#include "types.h"
#include "Network/sNetworkLibrary.h"   /* sNetworkLibraryError - the error triple `reportPatError` takes */

typedef struct NetworkInstance NetworkInstance;           /* include/unsplit/Network.h */
typedef struct NetworkStateMachine NetworkStateMachine;   /* include/Network/network_state.h */

/* The network singleton (`getInstance_` returns it).  Moved here from the mediator band
 * (`Network/NetworkWiiMediator.cpp`), which allocates it with `new` - 0xD640 bytes, the allocation
 * `initializeNetworkMediator` makes - and calls its five buffer setters by their mangled member names
 * (`setTermsBuffer__12PatInterfaceFPScUl`).  The vtable word and the opening state byte are the fields
 * evidenced so far: the bands reach the record through the C accessors below.  The constructor is out of line
 * (`__ct__12PatInterfaceFv`, 0x803FCC34), the virtual is declared and not defined here, so no table is
 * emitted by a consumer.
 *
 * NOT YET THE ONLY VIEW: `include/Network/session_mediator_views.h` carries the Pat session units'
 * interim view (no fields, so `sizeof` is 4) - unifying it changes those units' code, recorded there. */
class PatInterface {
public:
    PatInterface();
    /* +0x00 - the vtable pointer; slot +0x08 is the deleting finalizer */
    virtual void finalize(s32 flags);
    /* +0x0004 */ u8 pad_0004[0x60CD];
    /* +0x60D1 */ u8 opening_state;     /* the opening sub-step the mediator's progress reads (0 and 90 ignored) */
    /* +0x60D2 */ u8 pad_60D2[0x756E];
    void setTermsBuffer(s8* buffer, u32 size);
    void setMaintenanceBuffer(s8* buffer, u32 size);
    void setAnnounceBuffer(s8* buffer, u32 size);
    void setNoChargeBuffer(s8* buffer, u32 size);
    void setPatchMessageBuffer(s8* buffer, u32 size);
};   /* size: 0xD640 (the allocation `initializeNetworkMediator` makes) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803FD2D8 / 0x803FD644 - resets the singleton's callbacks unless it is still busy; whether it is
 * idle (both bodies read the singleton in r3). */
void  PatInterface_clear(PatInterface* self);
s32   PatInterface_isReady(PatInterface* self);

/* 0x803FD324 - the singleton's per-frame step the Pat holder's drive calls (GUESS from the caller). */
void stepPatInterface(NetworkInstance* self);

/* the singleton's callback slots */
s32 isCallback(NetworkInstance* self, s32 index);
void resetCallback(NetworkInstance* self, s32 index);

void decrement60d4(NetworkInstance* self);
/* 0x803FD258 - takes a reference on the singleton like `increment60d4`, and the first one also clears the
 * receive buffer's head before opening it (GUESS name). */
void openPatInterface(NetworkInstance* self);
/* 0x803FD90C - keeps the first error triple (+0x654C) and dispatches it to the session handlers with
 * code `kind` (GUESS name). */
void reportPatError(NetworkInstance* self, u32 kind, sNetworkLibraryError error);
/* 0x803FD944 - dispatches event `code` with a payload to the session handlers (GUESS name). */
void notifyPatEvent(NetworkInstance* self, u32 code, u32 count, const u8* data);
/* 0x803FD298 - takes a reference on the singleton (+0x60D4); the first one opens it (its +0x0C slot). */
void increment60d4(NetworkInstance* self);
/* 0x803FD9CC - installs callback `index` (0..7) with its argument; 1, or -1 for a bad index. */
/* untyped: caller-owned payload - handed back to the callback */
s32 setCallback(NetworkInstance* self, void (*callback)(), void* arg, u32 index);
/* 0x803FDCB0 - selects the connect server type (masked to 0..3). */
void setConnectServerType(NetworkInstance* self, s32 type);
s32 hasMultipleRefs60d4(NetworkInstance* self);
/* untyped: caller-owned payload - the 0x208-byte error record copied in when non-null */
s32 errorRecordCode613c(NetworkInstance* self, const void* record);
void getErrorInfo654c(NetworkInstance* self, u32* info);

/* 0x803768F8-band clock: the network singleton's game time (GUESS: the caller passes the singleton). */
u32 getGameTime(NetworkInstance* self);

/* 0x803FE854 / 0x803FE860 / 0x803FE404 / 0x803FE73C - the network singleton's Pat setters.  0x860 is a
 * name: the body `strlen`s its argument and copies at most 31 bytes of it. */
void setPatField854(PatInterface* self, u32 value);
void setPatField860(PatInterface* self, const char* name);
void setPatByteD400(NetworkInstance* self, u8 value);
void setPatByte6138On(NetworkInstance* self);

/* the term/opening queries and buffers the mediator band forwards */
void  setTermVersion(PatInterface* self, u32 value);
s32   getTermsVersion(PatInterface* self);
u32   getWarningUInt2(PatInterface* self);
s32   isOpeningAnnounce(PatInterface* self);
/* 0x803FE1E0 / 0x803FE1C8 - the status byte at +0x8254 reads 1 or 2 / reads 1 */
s32   isOpeningMaintenanceServer(PatInterface* self);
s32   isOpeningMaintenanceTerms(PatInterface* self);
void  updatePatInterface(PatInterface* self, u32 a, u32 b, u32 c);
void  setPatBuffer(PatInterface* self, u32 index, char* buffer, u32 size);
void  setPatRange(PatInterface* self, u32 index, u32 address, u32 size);
u32   getPatServerTime(PatInterface* self);
char* getPatAccountName(PatInterface* self);
char* getMediaVersion(PatInterface* self);
char* getStr1(PatInterface* self);
u32   getServerTime(PatInterface* self);

/* the connection sub-machine's state predicates (named for the field and value each tests) */
s32   isSubState_8254_3(PatInterface* self);     /* +0x8254 == 3 */
s32   isSubState_894F_2(PatInterface* self);     /* +0x894F == 2 */
s32   isSubState_894F_3(PatInterface* self);     /* +0x894F == 3 */
s32   isSubState_894F_4or6(PatInterface* self);  /* +0x894F == 4 || == 6 */
s32   isSubState_894F_5(PatInterface* self);     /* +0x894F == 5 */
s32   isSubState_894F_6(PatInterface* self);     /* +0x894F == 6 */

/* the reflect page fields the mediator mirrors */
void  setPatReflectPageRange(PatInterface* self, u32 address, u32 size);
void  setPatReflectField30(PatInterface* self, u32 value);
void  setPatReflectField34(PatInterface* self, u32 value);
void  setPatReflectField38(PatInterface* self, u32 value);
void  setPatReflectName3C(PatInterface* self, char* name);
void  setPatReflectName5C(PatInterface* self, char* name);

/* the login data getters, the FMP slot lookup and the call-stack helpers the state machine uses */
u32 getSomething3(NetworkStateMachine* self);       /* +0x655C */
u32 getSomething6(NetworkStateMachine* self);       /* +0x6560 */
u32 getSomething9(NetworkStateMachine* self);       /* +0x6564 */
u32 getFmpSlotIndex(NetworkStateMachine* self, u32 value);
u8*  createStack(NetworkStateMachine* self, u32 size, u32* outSize);
void growStackSize(NetworkStateMachine* self, u32 size);
void chooseServerAddress(NetworkStateMachine* self, u32 a, u32 b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_PATINTERFACE_H */
