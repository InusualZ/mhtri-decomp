/*
 * include/unsplit/Network.h - declarations for the Network band's callees that no registered unit owns.
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

typedef struct NetworkErrorInfo NetworkErrorInfo;

/* the DWC callbacks are installed as unprototyped pointers and the callee casts them back */
typedef void (*NetworkCallback)();

/* ---- the game's debug/log manager (`fn_803C9974` returns the singleton) ---------------------- */

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
    /* +0x18 */ virtual void pad_18();
    /* +0x1C */ virtual void pad_1C();
    /* +0x20 */ virtual void pad_20();
    /* +0x24 */ virtual void pad_24();
    /* +0x28 */ virtual void pad_28();
    /* +0x2C */ virtual void pad_2C();
    /* +0x30 */ virtual void pad_30();
    /* +0x34 */ virtual void pad_34();
    /* +0x38 */ virtual void pad_38();
    /* +0x3C */ virtual s32  isVerbose_3C();
    /* +0x40 */ virtual void pad_40();
    /* +0x44 */ virtual void pad_44();
    /* +0x48 */ virtual u16  flag_48(u16 value);
    /* +0x4C */ virtual u16  encode_4C(u32 value);
};   /* size: 0x04 (the object's leading vtable word) */

/* ---- the network singleton `getInstance_` returns -------------------------------------------- */

/* `postError` sits at vtable slot +0x288, which is 161 declared virtuals away: reaching it through a
 * real virtual would mean inventing that many slots, so the singleton stays a documented table view
 * here (a class we only construct in another unit).  Its two dispatch sites in fn_8041A87C.cpp read
 * the same two loads retail does, off a scratch register. */
typedef struct NetworkInstanceVtable {
    /* +0x000 */ u8 pad_00[0x288];
    /* +0x288 */ void (*postError_288)(void* self, NetworkErrorInfo* info);
} NetworkInstanceVtable;   /* size: 0x28C */

typedef struct NetworkInstance {
    /* +0x00 */ NetworkInstanceVtable* vtable;
} NetworkInstance;   /* size: 0x04 */

extern "C" {

/* debug manager */
NetworkLogger* fn_803C9974(void);

/* network singleton and its callbacks.  `getInstance_` (0x803768F0) is owned by
 * `enemy/em020_ai.cpp` now that its range is registered - rule 2: the declaration moved to the
owner's header and is included here. */
#include "enemy/em020_ai.h"
s32 isCallback(NetworkInstance* self, s32 index);
void resetCallback(NetworkInstance* self, s32 index);

/* DWC/GameSpy session layer */
void constructNetworkLibrary(void);   /* 0x804189C8, the sNetworkLibrary constructor body the opener calls */
void decrement60d4(NetworkInstance* self);
u32 getSomething5(NetworkInstance* self);
s32 fn_803FD658(NetworkInstance* self);
s32 fn_803FD694(NetworkInstance* self, s32 index);
void fn_803FD794(NetworkInstance* self, void* info);

/* the work-record emitter the previous band owns */
void fn_8041A5A8(void* self, u32 code, s32 a, s32 b, s32 c, void* data);

/* GameSpy socket layer (DWC / GameSpyInterface) */
s32 fn_8050A8A0(void);
s32 fn_8050A8D0(void);
s32 fn_8050A990(void);
void fn_8050A9A0(void);
s32 fn_8050A9B0(const void* key, s32 value);
s32 fn_8050A9D0(void);
void fn_8050C5F0(const void* self);
s32 fn_8050C770(void);
s32 fn_8050DEC0(void* socket, const void* address, u32 a, u32 b, NetworkCallback callback);
void fn_8050DED0(u32 socket);
void fn_8050DF20(u32 socket);
void fn_8050DF70(u32 socket, NetworkCallback callback);
s32 fn_8050DF80(u32 socket, const char* string);
void fn_8050DF90(u32 socket, const void* data, s32 len);
s32 fn_8050DFA0(u32 socket, void* out, void* address, void* buffer, u32 size, u32 timeout, const char* fmt, s32 flags);
s32 fn_8050E150(u32 socket, const void* data, u32 size, s32 flags);
void fn_8050E250(u32 socket);
void fn_8050E290(u32 socket);
void fn_8050E2A0(u32 data, NetworkCallback callback);

/* NHTTP / network utility layer */
s32 fn_805073C0(s32* code, s32* type);
void fn_80507470(void);

/* The DWCi NATNEG / transport-tail unit (`.text` 0x80512490..0x805145B8) is registered as
 * `src/DWCi/DWCi_NatNeg.c`, so its five entry points now live in its owner header; including
 * it keeps the callers below compiling without redeclaring an owned symbol (rule 2). */
#include "DWCi/DWCi_NatNeg.h"

/* OS / runtime helpers.  `SOHtoNs` is an SO-library symbol whose one home is the SO band header,
 * which is C-linkage-safe and so reachable from the DWCi `.c` units as well. */
#include "unsplit/SO.h"
void fn_804167B4(void* out);
void fn_80403F60(NetworkInstance* self, u32 handle);
void fn_80403FE4(NetworkInstance* self, u32 handle, u32 offset, u32 size);
void fn_80404070(NetworkInstance* self);
void fn_803CCF14(void* self, const void* table, s32 a, s32 err);
void dtor_803CA338(void* self, s32 flags);
void dtor_803CCE9C(void* self, s32 flags);
void OSLockMutex(void* mutex);
void LockMutex(void* mutex);
void UnlockMutex(void* mutex);
void OSUnlockMutex(void* mutex);
void OSInitMutex(void* mutex);
s32 OSCreateThread(void* thread, void* entry, void* param, void* stack, u32 stackSize, s32 priority, u32 flags);
s32 OSResumeThread(void* thread);
void OSSleepTicks(u64 ticks);

/* another TU's vtables (rule 10: reference, never rebuild) */
extern u32 lbl_806036A0[];
extern u32 lbl_80603740[];

/* the three-slot socket table, the peer thread's result record and the global socket */
extern u32 lbl_806D3650[3];
extern u32 lbl_80794CE0;
extern void* lbl_80794CE4;
extern const char lbl_80793990[3];
extern u32 lbl_80793994;
extern const char lbl_80793998[4];
extern const u8 lbl_80794380[];

/* the range's own string pool */
extern const char lbl_80603154[];
extern const char lbl_806031A0[];
extern const char lbl_806031B0[];
extern const char lbl_806031D0[];
extern const char lbl_806031E8[];
extern const char lbl_80603204[];
extern const char lbl_80603218[];
extern const char lbl_80603228[];
extern const char lbl_80603238[];
extern const char lbl_80603248[];
extern const char lbl_80603254[];
extern const char lbl_80603278[];
extern const char lbl_80603298[];
extern const char lbl_806032C0[];
extern const char lbl_806032E0[];
extern const char lbl_80603308[];
extern const char lbl_80603320[];
extern const char lbl_80603334[];
extern const char lbl_80603348[];
extern const char lbl_8060335C[];
extern const char lbl_8060336C[];
extern const char lbl_80603378[];
extern const char lbl_80603388[];
extern const char lbl_806033C8[];
extern const char lbl_80603408[];
extern const char lbl_80603448[];
extern const char lbl_806034A0[];
extern const char lbl_806034F4[];
extern const char lbl_80603548[];
extern const char lbl_80603590[];
extern const char lbl_806035E4[];
extern const char lbl_80603638[];
extern const char lbl_80603660[];
extern const char lbl_80603680[];
extern const char lbl_806036B0[];

/* The Network band's constants live in the data-only sibling header (see its comment for why a
 * unit that also includes `Network/fn_803D3CE8.h` cannot take them from here). */
#include "unsplit/NetworkData.h"

}

/* `__dl__FPv`'s real spelling (the caller's `operator delete`); see the ef units' convention.
 * It is a C++ operator, so it is declared outside the `extern "C"` block. */
void operator delete(void* ptr) throw();

#endif
