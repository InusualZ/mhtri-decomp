/*
 * include/unsplit/Network.h - declarations for the Network band's callees that no registered unit owns.
 *
 * Filled in by the `8041A87C_fn_8041A87C` lane: the helpers the 0x8041A87C..0x8041DF10 range calls
 * (the debug manager, the DWC/GameSpy socket layer, the OS thread API and the neighbouring `fn_`
 * helpers) all live in address bands that are still unsplit, so per brief section 6.5 rule 2 this is
 * their legitimate home.  Nothing owned by a registered unit is declared here.
 */

#ifndef UNSPLIT_NETWORK_H
#define UNSPLIT_NETWORK_H

#include "types.h"

typedef struct NetworkErrorInfo NetworkErrorInfo;

/* the DWC callbacks are installed as unprototyped pointers and the callee casts them back */
typedef void (*NetworkCallback)();

/* ---- the game's debug/log manager (`fn_803C9974` returns the singleton) ---------------------- */

typedef struct NetworkLoggerVtable {
    /* +0x00 */ void* rtti_00;
    /* +0x04 */ void* rtti_04;
    /* +0x08 */ void (*destroy_08)(void* self, u32 flags);
    /* +0x0C */ void (*signal_0C)(void* self, u32 level, const char* fmt, ...);
    /* +0x10 */ void (*warn_10)(void* self, const char* fmt, ...);
    /* +0x14 */ void (*log_14)(void* self, const char* fmt, ...);
    /* +0x18 */ u8 pad_18[0x24];
    /* +0x3C */ s32 (*isVerbose_3C)(void* self);
    /* +0x40 */ u8 pad_40[0x08];
    /* +0x48 */ s32 (*flag_48)(void* self, u16 value);
    /* +0x4C */ u16 (*encode_4C)(void* self, u32 value);
} NetworkLoggerVtable;   /* size: 0x50 */

typedef struct NetworkLogger {
    /* +0x00 */ NetworkLoggerVtable* vtable;
} NetworkLogger;   /* size: 0x04 */

/* ---- the network singleton `getInstance_` returns -------------------------------------------- */

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

/* network singleton and its callbacks */
NetworkInstance* getInstance_(void);
s32 isCallback(NetworkInstance* self, s32 index);
void resetCallback(NetworkInstance* self, s32 index);

/* DWC/GameSpy session layer */
void sendReqShut(NetworkInstance* self, s32 mode);
void resetNetworkState3(NetworkInstance* self);
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
void fn_80512C50(void);
s32 fn_805132C0(u32 session, s32 flag, NetworkCallback empty, NetworkCallback callback, void* result);
void fn_805135E0(u32 session);
void fn_80513BB0(void);
void fn_80514400(void* data, u32 size, void* header);

/* OS / runtime helpers */
u16 SOHtoNs(u16 value);
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
extern const char lbl_80793990[];
extern u32 lbl_80793994;
extern const char lbl_80793998[];
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

}

/* `__dl__FPv`'s real spelling (the caller's `operator delete`); see the ef units' convention.
 * It is a C++ operator, so it is declared outside the `extern "C"` block. */
void operator delete(void* ptr) throw();

#endif
