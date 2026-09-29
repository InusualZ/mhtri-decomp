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

/* The band got its first *claimed* range (0x803CCDF8..0x803D3CE8, `Network/network_transport.cpp`),
   so the declarations that range now owns moved into that unit's own header and are reached
   through this include - a band header that still declared them would collide with the owner's
   definitions (docs/plan.md 6.5 rule 2). */
#include "Network/network_transport.h"

typedef struct NetworkErrorInfo NetworkErrorInfo;
/* the record's layout lives in `include/Network/fn_803D3CE8.h`, beside the GameSpy handshake that
 * fills it: `NetworkInstance::postError` below only takes a pointer to it. */

/* the DWC callbacks are installed as unprototyped pointers and the callee casts them back */
typedef void (*NetworkCallback)();

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
    /* +0x50 (GUESS: the transport stream readers' value decode) */ virtual u32 decode_50(u32 value);
    /* +0x54 (GUESS: the transport stream writers' value encode) */ virtual u32 encode_54(u32 value);
    /* +0x58 */ virtual void pad_58();
    /* +0x5C */ virtual void pad_5C();
    /* +0x60 */ virtual f32  getTime_60();
};   /* size: 0x04 (the object's leading vtable word) */

/* ---- the network singleton `getInstance_` returns -------------------------------------------- */

/* The singleton `getInstance_` returns, read as a *data* slot.  `network_state.cpp` reaches
 * `postError` through this view because its record is passed **by value** there - the by-value copy's
 * address is what retail hands the callee - and a struct of typed function pointers is the shape that
 * keeps that call and its table read separate. */
typedef struct NetworkInstanceVtable {
    /* +0x000 */ u8 pad_00[0x288];
    /* +0x288 */ void (*postError_288)(void* self, NetworkErrorInfo* info);
} NetworkInstanceVtable;   /* size: 0x28C */

typedef struct NetworkInstance {
    /* +0x00 */ NetworkInstanceVtable* vtable;
} NetworkInstance;   /* size: 0x04 */

/* The same object dispatched as a real virtual: the four `postError` sites of
 * `fn_8041A87C.cpp` pass a **pointer**, and that is retail's `lwz r12, 0x0(r3)` /
 * `lwz r12, 0x288(r12)` shape - the struct view above stages the table through a scratch register
 * instead.  The slot is 161 declared virtuals in, so the unnamed ones consume the table; none is
 * defined and MWCC emits no table of its own (rule 10). */
class NetworkInstanceDispatch {
public:
    /* +0x008 */ virtual void pad_008();
    /* +0x00C */ virtual void pad_00C();
    /* +0x010 */ virtual void pad_010();
    /* +0x014 */ virtual void pad_014();
    /* +0x018 */ virtual void pad_018();
    /* +0x01C */ virtual void pad_01C();
    /* +0x020 */ virtual void pad_020();
    /* +0x024 */ virtual void pad_024();
    /* +0x028 */ virtual void pad_028();
    /* +0x02C */ virtual void pad_02C();
    /* +0x030 */ virtual void pad_030();
    /* +0x034 */ virtual void pad_034();
    /* +0x038 */ virtual void pad_038();
    /* +0x03C */ virtual void pad_03C();
    /* +0x040 */ virtual void pad_040();
    /* +0x044 */ virtual void pad_044();
    /* +0x048 */ virtual void pad_048();
    /* +0x04C */ virtual void pad_04C();
    /* +0x050 */ virtual void pad_050();
    /* +0x054 */ virtual void pad_054();
    /* +0x058 */ virtual void pad_058();
    /* +0x05C */ virtual void pad_05C();
    /* +0x060 */ virtual void pad_060();
    /* +0x064 */ virtual void pad_064();
    /* +0x068 */ virtual void pad_068();
    /* +0x06C */ virtual void pad_06C();
    /* +0x070 */ virtual void pad_070();
    /* +0x074 */ virtual void pad_074();
    /* +0x078 */ virtual void pad_078();
    /* +0x07C */ virtual void pad_07C();
    /* +0x080 */ virtual void pad_080();
    /* +0x084 */ virtual void pad_084();
    /* +0x088 */ virtual void pad_088();
    /* +0x08C */ virtual void pad_08C();
    /* +0x090 */ virtual void pad_090();
    /* +0x094 */ virtual void pad_094();
    /* +0x098 */ virtual void pad_098();
    /* +0x09C */ virtual void pad_09C();
    /* +0x0A0 */ virtual void pad_0A0();
    /* +0x0A4 */ virtual void pad_0A4();
    /* +0x0A8 */ virtual void pad_0A8();
    /* +0x0AC */ virtual void pad_0AC();
    /* +0x0B0 */ virtual void pad_0B0();
    /* +0x0B4 */ virtual void pad_0B4();
    /* +0x0B8 */ virtual void pad_0B8();
    /* +0x0BC */ virtual void pad_0BC();
    /* +0x0C0 */ virtual void pad_0C0();
    /* +0x0C4 */ virtual void pad_0C4();
    /* +0x0C8 */ virtual void pad_0C8();
    /* +0x0CC */ virtual void pad_0CC();
    /* +0x0D0 */ virtual void pad_0D0();
    /* +0x0D4 */ virtual void pad_0D4();
    /* +0x0D8 */ virtual void pad_0D8();
    /* +0x0DC */ virtual void pad_0DC();
    /* +0x0E0 */ virtual void pad_0E0();
    /* +0x0E4 */ virtual void pad_0E4();
    /* +0x0E8 */ virtual void pad_0E8();
    /* +0x0EC */ virtual void pad_0EC();
    /* +0x0F0 */ virtual void pad_0F0();
    /* +0x0F4 */ virtual void pad_0F4();
    /* +0x0F8 */ virtual void pad_0F8();
    /* +0x0FC */ virtual void pad_0FC();
    /* +0x100 */ virtual void pad_100();
    /* +0x104 */ virtual void pad_104();
    /* +0x108 */ virtual void pad_108();
    /* +0x10C */ virtual void pad_10C();
    /* +0x110 */ virtual void pad_110();
    /* +0x114 */ virtual void pad_114();
    /* +0x118 */ virtual void pad_118();
    /* +0x11C */ virtual void pad_11C();
    /* +0x120 */ virtual void pad_120();
    /* +0x124 */ virtual void pad_124();
    /* +0x128 */ virtual void pad_128();
    /* +0x12C */ virtual void pad_12C();
    /* +0x130 */ virtual void pad_130();
    /* +0x134 */ virtual void pad_134();
    /* +0x138 */ virtual void pad_138();
    /* +0x13C */ virtual void pad_13C();
    /* +0x140 */ virtual void pad_140();
    /* +0x144 */ virtual void pad_144();
    /* +0x148 */ virtual void pad_148();
    /* +0x14C */ virtual void pad_14C();
    /* +0x150 */ virtual void pad_150();
    /* +0x154 */ virtual void pad_154();
    /* +0x158 */ virtual void pad_158();
    /* +0x15C */ virtual void pad_15C();
    /* +0x160 */ virtual void pad_160();
    /* +0x164 */ virtual void pad_164();
    /* +0x168 */ virtual void pad_168();
    /* +0x16C */ virtual void pad_16C();
    /* +0x170 */ virtual void pad_170();
    /* +0x174 */ virtual void pad_174();
    /* +0x178 */ virtual void pad_178();
    /* +0x17C */ virtual void pad_17C();
    /* +0x180 */ virtual void pad_180();
    /* +0x184 */ virtual void pad_184();
    /* +0x188 */ virtual void pad_188();
    /* +0x18C */ virtual void pad_18C();
    /* +0x190 */ virtual void pad_190();
    /* +0x194 */ virtual void pad_194();
    /* +0x198 */ virtual void pad_198();
    /* +0x19C */ virtual void pad_19C();
    /* +0x1A0 */ virtual void pad_1A0();
    /* +0x1A4 */ virtual void pad_1A4();
    /* +0x1A8 */ virtual void pad_1A8();
    /* +0x1AC */ virtual void pad_1AC();
    /* +0x1B0 */ virtual void pad_1B0();
    /* +0x1B4 */ virtual void pad_1B4();
    /* +0x1B8 */ virtual void pad_1B8();
    /* +0x1BC */ virtual void pad_1BC();
    /* +0x1C0 */ virtual void pad_1C0();
    /* +0x1C4 */ virtual void pad_1C4();
    /* +0x1C8 */ virtual void pad_1C8();
    /* +0x1CC */ virtual void pad_1CC();
    /* +0x1D0 */ virtual void pad_1D0();
    /* +0x1D4 */ virtual void pad_1D4();
    /* +0x1D8 */ virtual void pad_1D8();
    /* +0x1DC */ virtual void pad_1DC();
    /* +0x1E0 */ virtual void pad_1E0();
    /* +0x1E4 */ virtual void pad_1E4();
    /* +0x1E8 */ virtual void pad_1E8();
    /* +0x1EC */ virtual void pad_1EC();
    /* +0x1F0 */ virtual void pad_1F0();
    /* +0x1F4 */ virtual void pad_1F4();
    /* +0x1F8 */ virtual void pad_1F8();
    /* +0x1FC */ virtual void pad_1FC();
    /* +0x200 */ virtual void pad_200();
    /* +0x204 */ virtual void pad_204();
    /* +0x208 */ virtual void pad_208();
    /* +0x20C */ virtual void pad_20C();
    /* +0x210 */ virtual void pad_210();
    /* +0x214 */ virtual void pad_214();
    /* +0x218 */ virtual void pad_218();
    /* +0x21C */ virtual void pad_21C();
    /* +0x220 */ virtual void pad_220();
    /* +0x224 */ virtual void pad_224();
    /* +0x228 */ virtual void pad_228();
    /* +0x22C */ virtual void pad_22C();
    /* +0x230 */ virtual void pad_230();
    /* +0x234 */ virtual void pad_234();
    /* +0x238 */ virtual void pad_238();
    /* +0x23C */ virtual void pad_23C();
    /* +0x240 */ virtual void pad_240();
    /* +0x244 */ virtual void pad_244();
    /* +0x248 */ virtual void pad_248();
    /* +0x24C */ virtual void pad_24C();
    /* +0x250 */ virtual void pad_250();
    /* +0x254 */ virtual void pad_254();
    /* +0x258 */ virtual void pad_258();
    /* +0x25C */ virtual void pad_25C();
    /* +0x260 */ virtual void pad_260();
    /* +0x264 */ virtual void pad_264();
    /* +0x268 */ virtual void pad_268();
    /* +0x26C */ virtual void pad_26C();
    /* +0x270 */ virtual void pad_270();
    /* +0x274 */ virtual void pad_274();
    /* +0x278 */ virtual void pad_278();
    /* +0x27C */ virtual void pad_27C();
    /* +0x280 */ virtual void pad_280();
    /* +0x284 */ virtual void pad_284();
    /* +0x288 */ virtual void postError(NetworkErrorInfo* info);
};   /* size: 0x04 (the object's leading vtable word) */

extern "C" {

/* debug manager */
NetworkLogger* getNetworkLogger(void);

/* The socket pool the transport peers register their socket with (0x804187F0 / 0x80418864 - no
 * registered range covers either address).  Both bodies walk a four-entry table at the manager's
 * +0x04 and take the socket from the manager's own +0x8C slot, so the argument is the manager the
 * transport band reaches through `getNetworkLogger` (GUESS on both names: they carry the acquire /
 * release roles the two call sites give them, nothing in the range spells them). */
NetworkSocketHandle* networkSocketPool_acquire(NetworkLogger* pool);
s32 networkSocketPool_release(NetworkLogger* pool, NetworkSocketHandle* socket);

/* network singleton and its callbacks.  `getInstance_` (0x803768F0) is owned by
 * `enemy/em020_ai.cpp` now that its range is registered - rule 2: the declaration moved to the
owner's header and is included here. */
#include "enemy/em020_ai.h"
s32 isCallback(NetworkInstance* self, s32 index);
void resetCallback(NetworkInstance* self, s32 index);

/* DWC/GameSpy session layer */
void constructNetworkLibrary(void);   /* 0x804189C8, the sNetworkLibrary constructor body the opener calls */
void decrement60d4(NetworkInstance* self);
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
void dtor_803CA338(void* self, s32 flags);
void OSLockMutex(void* mutex);
void OSUnlockMutex(void* mutex);
void OSInitMutex(void* mutex);
s32 OSCreateThread(void* thread, void* entry, void* param, void* stack, u32 stackSize, s32 priority, u32 flags);
s32 OSResumeThread(void* thread);
void OSSleepTicks(u64 ticks);

/* another TU's vtables (rule 10: reference, never rebuild) */
extern u32 lbl_806036A0[];
extern u32 lbl_80603740[];

/* the three-slot socket table and the peer thread's reply marker, lookup key and address format */
extern u32 lbl_806D3650[3];
extern const char lbl_80793990[3];
extern u32 lbl_80793994;
extern const char lbl_80793998[4];
/* 0x80794380 `natNegMessageMagic` - the NATNEG message signature this unit compares the head of a
 * received datagram against - is deliberately *not* declared here: the bytes belong to the NATNEG
 * unit, so rule 2 puts the declaration in `include/DWCi/DWCi_NatNeg.h` (included above), and that
 * is the *unsized* spelling this unit needs.  It addresses the symbol with `lis`/`addi`
 * (ADDR16_HA/LO, the target's relocation kind), while the owner's own source re-declares it sized
 * for the SDA21 form its ten sites use (playbook row 12 - the reloc kind is a codegen input). */

/* Three declarations that used to stand here are owned now, so each lives in its OWNER's header and is
 * reached through this band by including it (section 6.5 rule 2, 2026-09-28): `lbl_80794CE0` and
 * `sGameSpyInterfaceThread` by `Network/fn_8041A87C.cpp`, whose `splits.txt` claims
 * `.sbss:0x80794CE0..0x80794CE8` - they are declared in `include/Network/fn_8041A87C.h`, the header of
 * the unit that defines them - and `natNegMessageMagic` by `DWCi/DWCi_NatNeg.c`, whose `.sdata` run
 * 0x80794368..0x807943A0 covers it (declared in `include/DWCi/DWCi_NatNeg.h`, included at the top of
 * this band).  A band header that still declared them would collide with the owners' definitions.
 */

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

/* ---- the layer request state machine's callees (0x803DECF0..0x803EF4C8, 0x8040144C..0x80401B68) ---- */

/* The request record `NetworkRequest_getError` copies out: the three words `NetworkRequest_setError`
 * stores under the request's mutex (+0x54/+0x58/+0x5C of `NetworkRequest`). */
typedef struct NetworkRequestError {
    u32 code_00;
    u32 arg_04;
    u32 arg_08;
} NetworkRequestError;   /* size: 0x0C */

class NetworkLayerPat;             /* include/Network/NetworkLayerPat.h */
class NetworkSessionManagerPat;    /* include/Network/fn_803D3CE8.h */
typedef struct NetworkRequest NetworkRequest;   /* include/Network/fn_803D3CE8.h */

/* 0x803E3598 - copies the request's error record out under its mutex; false while none is set. */
s32 NetworkRequest_getError(NetworkRequest* request, NetworkRequestError* out);
/* GUESS on both names: 0x803EF3C0 and 0x803EF4C8 are the siblings of `setCollectionLog` for the two
 * fixed codes the state machine's flag tests report (0x80060033 and 0x80060012); the names follow the
 * flag bits that select them (bit 0 = the session dropped, bit 1 = the request was cancelled). */
void setCollectionLogSessionLost(NetworkLayerPat* self, NetworkRequest* request);
void setCollectionLogAborted(NetworkLayerPat* self, NetworkRequest* request);
/* 0x803EF568 - records `code` (+ two arguments) as the request's error and reports it to the server. */
void setCollectionLog(NetworkLayerPat* self, NetworkRequest* request, u32 code, u32 arg_a, u32 arg_b);
/* GUESS: 0x803EBAF0 hands one layer event (kind 3 = error, kind 4 = done) to the callback object the
 * layer holds, after posting `info` to the network singleton. */
void notifyLayerEvent(NetworkLayerPat* self, u32 kind, s32 code, u32 has_info, NetworkRequestError* info,
                      u32 context);
/* GUESS: 0x803EB9D4 walks the layer's 100 child slots and releases each finished one. */
void pollLayerSlots(NetworkLayerPat* self);
/* GUESS: 0x803EBA5C reports the layer's slot counts as one kind-0x14 event. */
void notifyLayerSlotSummary(NetworkLayerPat* self);
/* GUESS: 0x803DECF0 clears the session manager's busy byte and releases its buffers. */
void closeNetworkSessionManagerPat(NetworkSessionManagerPat* self);
/* The layer requests: each writes its op-code and returns the request id (the callee narrows it to 16
 * bits, but its caller stores the full register, so the declared type is the wide one - playbook 66). */
u32 sendReqLayerUp(NetworkInstance* self);
u32 sendReqLayerChildInfo(NetworkInstance* self, s16 layer_id, u32 unused_arg);
u32 sendReqLayerUserList(NetworkInstance* self);

}

/* `__dl__FPv`'s real spelling (the caller's `operator delete`); see the ef units' convention.
 * It is a C++ operator, so it is declared outside the `extern "C"` block. */
void operator delete(void* ptr) throw();

#endif
