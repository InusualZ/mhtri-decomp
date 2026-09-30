/*
 * Network/NetworkWiiMediator.cpp - the `NetworkWiiMediator` class band
 * (`.text` 0x80413C64..0x804155D4, 79 functions / 6490 B).
 *
 * BOUNDARY.  Left seam 0x80413C64 is **strong** (two signals): `tudiscover at
 * reflectInit__18NetworkWiiMediatorFPFllllPvPv_vPv` flags the `.data` jumptable run jump
 * `jumptable_806024B8 -> jumptable_8060277C` at 0x80413C64 *and* 0x80413E18 (share 0.090/0.086),
 * so the function above it (`getLanguage`, 0x80413C40..0x80413C64) is a different TU.  Right seam
 * 0x804155D4 is **weak** (`tudiscover at agreeReflect__18NetworkWiiMediatorFv`, share 0.247): the
 * class band may continue past it, but 0x804155D4 is the largest single jump the evidence offers and
 * the next function (`0x804155D4`, 0x9D8) opens a new shape.  Sections: `.text`
 * 0x80413C64..0x804155D4, `extab` 0x8001CA4C..0x8001CBF4, `extabindex` 0x8003D2F0..0x8003D524.
 *
 * RECONCILIATION.  This unit folds in the retired `Network/NetworkWiiMediator.c`, which owned the
 * 20-byte `getReflectPageBuffer` at **0x80413F3C inside this span** as a `Matching` unit.  That address
 * is this original TU's, so the class band owns it now; `getReflectPageBuffer`'s body is carried across
 * verbatim (5 instructions, 100 %).
 *
 * WHAT IT IS.  The mediator's account/opening/reflect query surface (the reflected class methods at
 * 0x8041416C..0x80414B88) plus the helpers around them; the jumptable-referenced 0x80414BF0..0x804155D4
 * block is the reflect sub-machine.  C++ (the mangled member names), so the class is declared in
 * `include/Network/NetworkWiiMediator.h` and every unmangled helper is `extern "C"`.
 *
 * FLAGS.  `cflags_network` + `-O3 -inline noauto` (the `Object(...)` line in `configure.py`), measured
 * over the whole unit: `-O4,p` leaves getAccountBan/Warning/WaitQueue at 42.86 and getReflectName3C at
 * 51.28 where `-O3` puts both at 100.00, and `-inline auto` scores validateReflectName 0.00 where
 * `-inline noauto` scores it 88.28; every other function is byte-identical under either `-inline`
 * setting (a 79-row report diff).  The `-O4,p` epilogue-swap / hoisted-`li` signature this band showed
 * (updatePatInterface180, updateTermVersion, resetMediatorFlags) is playbook 27's -O3 evidence, and the
 * two sibling units in the same library are already `-O3`.  `-Cpp_exceptions on` now comes from
 * `cflags_network` (flags-audit 2026-09-28), not a per-file pragma: the target object carries `extab`
 * 0x1a8 + `extabindex` 0x234 (one unwind record per frame-bearing function, 47 of them).
 *
 * NAMES.  The `NetworkWiiMediator::*` spellings are the runtime dump's and already in `symbols.txt`.
 * The 65 helpers answer only `zz_XXXXXXXX_` in the dump, so the registration batch named them from their
 * own code: the accessor/updater pairs after the field they touch (`getReflectField30` /
 * `setReflectField30`, `getReflectName3C` / `setReflectName3C`, `getReflectPageRange` /
 * `setReflectPageRange`), the singleton forwarders after the 0x803FE helper they call
 * (`updatePatField854` -> 0x803FE854, `queryOpeningFlag208` -> 0x803FE208), and the packet/reflect
 * helpers after what they do (`parseReflectPacket`, `buildReflectPacket`, `validateReflectName`,
 * `getReflectModeFromLanguage`, `isShiftJisLeadByte`).
 *
 * NAMING GUESSES (rule 6.5: a guess is stated, not hidden).  The 26 callees the bodies call that no
 * registered unit owns answered only `fn_XXXXXXXX` in the map, so they were renamed through `symedit.py`
 * in this pass and every one of those names is a GUESS: `fn_803FE388` -> `setPatRange`, `fn_803FE744` ->
 * `setPatReflectField30`, `fn_8041241C` -> `getNetworkWiiMediator`, `fn_8041A1C4` ->
 * the reflect service's constructor, `fn_80413BF8` -> `getReflectEventId` and so on - each named for the
 * singleton (`PatInterface`) or the service (`NetworkReflectService`) it belongs to and the field or slot
 * it works on.  Within this file, `getReflectField30/34/38` and `getAccountQuery1..5` are positional (the
 * dump has no name and no caller reveals the field's meaning); `queryOpeningFlagNNN` and
 * The six 0x803FE helpers carry the names `Network/network_state.cpp` gave them
 * (`isSubState_8254_3` / `isSubState_894F_2..6`), which main's committed source already
 * called - those are semantic (the field and value each tests), not address-keyed.
 * `pat_*` / `line_table_*` / `buffer_*` field names come from the functions that use them.
 *
 * BODIES.  70 of 79 functions are at 100 %, and the unit's `.data` (the three jump tables) is
 * byte-complete.  The 9 residuals, largest first:
 *   - `isShiftJisLeadByte` 71.69 - retail evaluates the two byte ranges as four separate unsigned
 *     compares; MWCC folds each range into one `addi`/`clrlwi`/`cmplwi` and the `>= 224` test into
 *     branchless code, the same on -O4,p and for every spelling tried (`||`, nested `if`, a result
 *     variable, an `s32` parameter), so the fold is not reachable from the source side.
 *   - `setMediatorBufferA`/`setMediatorBufferB` 80.00 - retail schedules the `addi r3,r3,0x7C` before
 *     the `li r5,0x106` of the same `memcpy` call; MWCC emits them the other way round for the store
 *     direction only (the load direction, `getMediatorBuffer*`, is byte-identical).  Tried as a typed
 *     array, `&self->buffer[0]` and an explicit `(u8*)self + 0x7C`.
 *   - `parseReflectPacket` 91.27 - the lead-byte guard lands in `cr1` where retail uses `cr0` (retail's
 *     `&&` chain reuses one condition register, ours needs a second because the `>= 0xA0` test is
 *     consumed by a later `bne`), and retail re-reads `in[1]` with a `lbz`+`extsb` before the
 *     `out[1] = in[1] - 32` store where ours keeps the byte live.  The switch and the if-chain spelling
 *     of the nine symbol-map cases emit the same object.
 *   - `parseReflectLines` 96.31 - one instruction (retail keeps `mr r0,r28` before the `*table = text`
 *     store; MWCC stores the pointer directly).  A named temporary and a `#pragma peephole off`
 *     around the function both leave it at 96.31.
 *   - `buildReflectPacket` 99.94 - byte-identical instructions, same 344 B; the last 0.06 is a
 *     relocation/address artefact of the split (no `.rela.text` record in this function).
 *   - `updateServerTime` 98.46 - the 64-bit tail `return 0` emits `li r3,0` where retail emits
 *     `li r4,0` + `mr r3,r4`.
 *   - `getReflectPage` 96.25 - `mr r4,r31` where retail masks the `u8` page (`clrlwi r4,r31,24`) before
 *     the call; an explicit `(u32)` widening cast does not reproduce it.
 *   - `isNameSymbolChar` 96.43 - the `u8` local's mask lands in `r3` where retail masks into `r0` and
 *     keeps the raw parameter in `r4`; declaring the parameter `u8` fixes the mask but costs
 *     `validateReflectName` 3.45 (the caller's `char` -> `u8` conversion), so the local spelling stays.
 *
 * SOLVED THIS PASS (measured before -> after).  `initializeNetworkMediator` 1.89 -> 100, `reflectInit`
 * 88.34 -> 100, `setMediatorState68A` 91.25 -> 100, `isNameSymbolChar` 91.67 -> 96.43, `buildReflectPacket`
 * 95.93 -> 99.94; unit 95.09 -> 98.89, 67 -> 70 functions at 100 %.
 *
 * THE SHAPE THAT CLOSED `initializeNetworkMediator` AND `reflectInit` (playbook-worthy).  Both tail
 * residuals are the same one instruction - a `mr r31,r3` between `bl __nw__FUl` and the null check -
 * and it is **not** reachable from `T* p = (T*)operator new(n); if (p != NULL) ctor(p);`: MWCC coalesces
 * that copy away.  It appears when the allocation is a real **`new` expression whose constructor is
 * called** - with `-Cpp_exceptions on` (the lib's setting) the new-expression's value must survive
 * the constructor for the unwind path, so MWCC keeps it in a callee-saved register and emits the copy.
 * The three classes therefore each carry a declared (out-of-line) ctor plus a padding member that makes
 * `sizeof` the size the allocation passes to `operator new` (0xD640 / 0x816C / 0x44A0), and the three
 * allocation sites are `new PatInterface();` / `new NetworkReflectService();` /
 * `new GameSpyInterfaceThread();`.
 *
 * LINK INPUTS (cleared this pass - the unit could not link before it).  `objdiff` scores a `bl` by its
 * instruction whatever name it carries, which is why the report said 100 % for these sites while the
 * linker had nothing to resolve them against: the three `new` sites and the thread accessor referenced
 * four names **no map row and no link input carried** - `__ct__12PatInterfaceFv`,
 * `__ct__21NetworkReflectServiceFv`, `__ct__22GameSpyInterfaceThreadFv` and
 * `getGameSpyInterfaceThread`, all four `*UND*` with an empty provider set - so a flip would have
 * failed with `undefined:`.  The map now carries the compiler's own spellings, each defined by a link
 * input: `fn_803FCC34` -> `__ct__12PatInterfaceFv` (0x803FCC34) and `constructReflectService` ->
 * `__ct__21NetworkReflectServiceFv` (0x8041A1C4), both unowned bands whose symbols the `new` sites
 * already spelled that way; `create__22GameSpyInterfaceThreadFv` -> `__ct__22GameSpyInterfaceThreadFv`
 * (0x8041C66C), whose owner `Network/fn_8041A87C.cpp` turned its `create()` into the class's
 * constructor in the same change; and `fn_803D6A98` -> `GameSpyInterfaceThread_getInstance`
 * (0x803D6A98, `Network/fn_803D3CE8.cpp`), with the referrers in `include/Network/fn_8041A87C.h` and
 * `src/Network/fn_8041A87C.cpp`.  Re-measured: every `.text` row and the whole-project progress are
 * unchanged, and `flipcheck.py` reports exactly the four complaints it did before, no new one.
 *
 * Those four rows were **renamed again** by `worker/fn-803d3ce8-2477`, to the owners' own definition
 * spellings (the map rows at 36030, 36676, 36678 and 57701), and the referrers this file and
 * `src/Network/fn_8041A87C.cpp` carried were left on the old ones - `flipcheck.py` then reported "3
 * referenced symbol(s) are defined by nothing a flip can use - clearPatInterface,
 * getGameSpyInterfaceThread, isPatInterfaceReady" for this unit.  They are spelled
 * `GameSpyInterfaceThread_getInstance`, `PatInterface_clear` and `PatInterface_isReady` here now, and
 * each referrer's **signature** is unchanged: the target passes the pointer in r3 (`bl getInstance_`
 * followed by `bl PatInterface_clear` and `bl PatInterface_isReady`), so the argument stays and only
 * the name moved.
 *
 * .data.  The three jump tables are `jumptable_806024B8` (0x2C4, 177 entries), `jumptable_8060277C`
 * (0x28, 10) and `jumptable_806027A4` (0x80, 32); they tile exactly 0x806024B8..0x80602824 in the same
 * order as the three functions' `.text` addresses, so the range is this TU's `.data` section and is
 * claimed in `splits.txt` (playbook 53).  MWCC emits each table under an anonymous local name while
 * the map calls it `jumptable_<addr>`, so objdiff still reports the tables' relocations as unpaired
 * even though the section sizes and bytes line up - the same shape `tools/elf/objextab.py` fixes for
 * `@etb_`/`@eti_`, wanted here for `.data`.
 */
#include "types.h"
#include "Network/NetworkWiiMediator.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"

/* The reflect sub-service the mediator starts, stops and agrees through.  This band only dispatches
 * into its `+0x08` slot (retail's `lwz r12, 0(r3)` / `lwz r12, 8(r12)`), so the class is declared with
 * the real virtual and never defined here - MWCC emits the table only for a class this TU defines,
 * and this one's vtable lives in the unit that owns the service.  Its constructor is the unit-level
 * builder at 0x8041A1C4 (out of line, so `new NetworkReflectService` lowers to that call and keeps
 * the allocation alive across it - the `mr r31,r3` of `reflectInit`); the 0x816C body is the size
 * `reflectInit` allocates. */
class NetworkReflectService {
public:
    NetworkReflectService();
    /* +0x00 */ virtual void finalize(s32 flags);
    /* +0x04 */ u8 pad_004[0x8168];
    /* only the vtable word and the one slot this band dispatches through are evidenced */
};   /* size: 0x816C (the allocation `reflectInit` makes) */

/* The network singleton's Pat accessor the two buffer loaders forward to.  Its five buffer setters are
 * the mangled member symbols this band calls (`setTermsBuffer__12PatInterfaceFPScUl`); the class is
 * only used here, never constructed, so no vtable and no table bytes enter our object. */
class PatInterface {
public:
    PatInterface();
    /* +0x00 */ virtual void finalize(s32 flags);
    /* +0x04 */ u8 pad_004[0xD63C];
    void setTermsBuffer(s8* buffer, u32 size);
    void setMaintenanceBuffer(s8* buffer, u32 size);
    void setAnnounceBuffer(s8* buffer, u32 size);
    void setNoChargeBuffer(s8* buffer, u32 size);
    void setPatchMessageBuffer(s8* buffer, u32 size);
    /* only the vtable word is evidenced: the band reaches the record through `getInstance_` */
};   /* size: 0xD640 (the allocation `initializeNetworkMediator` makes) */

/* The GameSpy worker thread `initializeNetworkMediator` spawns.  Its full layout and the rest of its
 * methods live in `include/Network/fn_8041A87C.h`, which this band does not include (that header also
 * declares `updatePatInterface` with a different first parameter, and playbook 60: a declaration set
 * is a codegen input), so only the entry point this band calls is named here. */
class GameSpyInterfaceThread {
public:
    GameSpyInterfaceThread();
    /* +0x0000 */ u8 pad_000[0x44A0];
};   /* size: 0x44A0 (the allocation `initializeNetworkMediator` makes) */

/* `operator new` is what the band's allocation lowers to (`__nw__FUl`). */
void* operator new(unsigned long size);   /* untyped: allocation returns a raw byte range */

extern "C" {
/* the retired NetworkWiiMediator.c symbol, carried across verbatim */
void getReflectPageBuffer(char* self, char** subobject, unsigned int* limit);

void setMediatorBufferA(NetworkWiiMediatorFields* self, const u8* src);
void getMediatorBufferA(NetworkWiiMediatorFields* self, u8* dst);
void setMediatorBufferB(NetworkWiiMediatorFields* self, const u8* src);
void getMediatorBufferB(NetworkWiiMediatorFields* self, u8* dst);
void getMediatorNameBuffer(NetworkWiiMediatorFields* self, u32* out1, u8* out2);
void setMediatorFlag78B(NetworkWiiMediatorFields* self, u8 value);
void getMediatorField288(NetworkWiiMediatorFields* self, u32* out);
void setMediatorFlag78C(NetworkWiiMediatorFields* self, u8 value);
void getMediatorFlag78C(NetworkWiiMediatorFields* self, u8* out);
void initializeNetworkMediator(NetworkWiiMediatorFields* self, u32 value);

/* The thread accessor the opening's init reaches.  Its body sits in the `Network/fn_803D3CE8.cpp` band
 * and returns that band's own thread object, so it stays `void*` in the owner's declaration and this
 * file declares the typed call; the map row is the owner's spelling of the dump's placeholder
 * `getInstance`, qualified by the class. */
GameSpyInterfaceThread* GameSpyInterfaceThread_getInstance(void);
u32   getMediatorField24(NetworkWiiMediatorFields* self);
u8    getMediatorFlag6B(NetworkWiiMediatorFields* self);
u8    getMediatorFlag60D1(NetworkWiiMediatorFields* self);
void  setMediatorTimestamp(NetworkWiiMediatorFields* self, u64 value);
void  getMediatorState68A(NetworkWiiMediatorFields* self, u8* out);
u64   getMediatorTimestamp(NetworkWiiMediatorFields* self);
s32   getAccountQuery1(NetworkWiiMediatorFields* self);
s32   getAccountQuery2(NetworkWiiMediatorFields* self);
s32   getAccountQuery3(NetworkWiiMediatorFields* self);
s32   getAccountQuery4(NetworkWiiMediatorFields* self);
s32   getAccountQuery5(NetworkWiiMediatorFields* self);
void  getReflectPageRange(NetworkWiiMediatorFields* self, u32* out1, u32* out2);
void  getReflectField30(NetworkWiiMediatorFields* self, u32* out);
void  getReflectField34(NetworkWiiMediatorFields* self, u32* out);
void  getReflectField38(NetworkWiiMediatorFields* self, u32* out);

s32  getReflectModeFromLanguage();
u64  updateServerTime(NetworkWiiMediatorFields* self);
u64  setServerTimeResult(NetworkWiiMediatorFields* self);
void setMediatorState68A(NetworkWiiMediatorFields* self, u8 value);
s32  queryOpeningFlag208(NetworkWiiMediatorFields* self);
s32  queryOpeningFlag250(NetworkWiiMediatorFields* self);
s32  queryOpeningFlag290(NetworkWiiMediatorFields* self);
char* getReflectPageText(char* self, char* out, u32 size);
void getReflectName3C(NetworkWiiMediatorFields* self, char* out, u32 size);
void getMediaVersionString(NetworkWiiMediatorFields* self, char* out, u32 size);
void getStr1String(NetworkWiiMediatorFields* self, char* out, u32 size);
void getReflectName5C(NetworkWiiMediatorFields* self, char* out, u32 size);
void updatePatField854(NetworkWiiMediatorFields* self, u32 value);
void updatePatField860(NetworkWiiMediatorFields* self, u32 value);
s32  isNameSymbolChar(NetworkWiiMediatorFields* self, char value);
s32  isShiftJisLeadByte(NetworkWiiMediatorFields* self, u8 value);
void parseReflectLines(NetworkWiiMediatorFields* self, s32 source);
s32  parseReflectPacket(NetworkWiiMediatorFields* self, char* out, const char* in, u32 unused4,
                        u32 length, s32 allowSymbolMap);
s32  validateReflectName(NetworkWiiMediatorFields* self, const char* name);
s32  buildReflectPacket(NetworkWiiMediatorFields* self, const char* text, s32* skipCount, s32* flags);

/* the network singleton and the reflect service the band forwards to (0x803D5xxx / 0x8041Axxx bands,
 * owned by no registered unit) */
PatInterface*  getInstance_(void);
NetworkReflectService* getReflectService(void);
NetworkWiiMediatorFields* getNetworkWiiMediator(void);

/* the singleton's Pat accessor surface, reached through `getInstance_` */
void  setTermVersion(PatInterface* self, u32 value);
s32   getTermsVersion(PatInterface* self);
u32   getWarningUInt2(PatInterface* self);
s32   isOpeningMaintenanceTerms(PatInterface* self);
s32   isOpeningMaintenanceServer(PatInterface* self);
s32   isOpeningAnnounce(PatInterface* self);
void  updatePatInterface(PatInterface* self, u32 a, u32 b, u32 c);
void  PatInterface_clear(PatInterface* self);
s32   PatInterface_isReady(PatInterface* self);
void  setPatBuffer(PatInterface* self, u32 index, char* buffer, u32 size);
void  setPatRange(PatInterface* self, u32 index, u32 address, u32 size);
u32   getPatServerTime(PatInterface* self);
s32   isSubState_8254_3(PatInterface* self);
s32   isSubState_894F_4or6(PatInterface* self);
s32   isSubState_894F_6(PatInterface* self);
s32   isSubState_894F_5(PatInterface* self);
s32   isSubState_894F_2(PatInterface* self);
s32   isSubState_894F_3(PatInterface* self);
char* getPatAccountName(PatInterface* self);
void  setPatReflectPageRange(PatInterface* self, u32 address, u32 size);
void  setPatReflectField30(PatInterface* self, u32 value);
void  setPatReflectField34(PatInterface* self, u32 value);
void  setPatReflectField38(PatInterface* self, u32 value);
void  setPatReflectName3C(PatInterface* self, char* name);
void  setPatReflectName5C(PatInterface* self, char* name);
void  setPatField854(PatInterface* self, u32 value);
void  setPatField860(PatInterface* self, u32 value);

/* the reflect service's own entry points */
void initReflectService(NetworkReflectService* service, NetworkWiiMediatorReflectFn callback,
                        void* arg);   /* untyped: the callback's user payload, caller-owned */
void finalizeReflectService(NetworkReflectService* service);
void setReflectServicePage(NetworkReflectService* service, u32 page);

/* the singleton's remaining query the band forwards */
char* getMediaVersion(PatInterface* self);
char* getStr1(PatInterface* self);
u32   getServerTime(PatInterface* self);
s32   getLanguage(void);
s32   getReflectEventId(void);
void  reflectServiceStart(NetworkReflectService* service);
void  reflectServiceStop(NetworkReflectService* service);
void  reflectServiceAgree(NetworkReflectService* service);

} /* extern "C" */

/* retired Network/NetworkWiiMediator.c, carried across verbatim (100 %) */
void getReflectPageBuffer(char *self, char **subobject, unsigned int *limit)
{
    *subobject = self + 0x78D;
    *limit = 0x400;
}

void NetworkWiiMediator::reflectInit(NetworkWiiMediatorReflectFn callback, void* arg)
{
    if (getReflectService() == NULL) {
        new NetworkReflectService();
    }
    initReflectService(getReflectService(), callback, arg);
}
void NetworkWiiMediator::reflectStart()
{
    if (getReflectService() != NULL) {
        reflectServiceStart(getReflectService());
    }
}
void NetworkWiiMediator::reflectStop()
{
    if (getReflectService() != NULL) {
        reflectServiceStop(getReflectService());
    }
}
void NetworkWiiMediator::reflectFinal()
{
    if (getReflectService() == NULL) {
        return;
    }
    finalizeReflectService(getReflectService());
    NetworkReflectService* service = getReflectService();
    if (service != NULL) {
        service->finalize(1);
    }
}
#pragma peephole off
s32  NetworkWiiMediator::getOpeningProgress()
{
    NetworkWiiMediatorFields* self = (NetworkWiiMediatorFields*)this;
    if (self->flag_1C != 0) {
        self->flag_1E = self->flag_1C;
    }
    if (getInstance_() != NULL) {
        u8 state = getMediatorFlag60D1((NetworkWiiMediatorFields*)getInstance_());
        if (state != 0 && state != 90) {
            self->flag_1F = state;
        }
    }
    if (self->flag_1E == 6) {
        if (queryOpeningFlag208(self) == 0) {
            return 100;
        }
        u8 progress = 0;
        if (getNetworkWiiMediator() != NULL) {
            progress = getMediatorFlag6B(getNetworkWiiMediator());
        }
        return progress + 90;
    }
    return self->flag_1F + self->flag_1E * 10;
}
#pragma peephole on
s32  NetworkWiiMediator::getOpeningTermsVersion()
{
    if (getInstance_() != NULL) {
        return getTermsVersion(getInstance_());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningMaintenanceTerms()
{
    if (getInstance_() != NULL) {
        return ::isOpeningMaintenanceTerms(getInstance_());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningMaintenanceServer()
{
    if (getInstance_() != NULL) {
        return ::isOpeningMaintenanceServer(getInstance_());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningAnnounce()
{
    if (getInstance_() != NULL) {
        return ::isOpeningAnnounce(getInstance_());
    }
    return 0;
}
char* NetworkWiiMediator::getAccountBan(char* out, u32 size)
{
    NetworkWiiMediatorFields* self = (NetworkWiiMediatorFields*)this;
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self->reflect_page, length);
        out[length] = 0;
    }
    return (char*)self->reflect_page;
}
char* NetworkWiiMediator::getAccountWarning(char* out, u32 size)
{
    NetworkWiiMediatorFields* self = (NetworkWiiMediatorFields*)this;
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self->reflect_page, length);
        out[length] = 0;
    }
    return (char*)self->reflect_page;
}
char* NetworkWiiMediator::getAccountWaitQueue(char* out, u32 size)
{
    NetworkWiiMediatorFields* self = (NetworkWiiMediatorFields*)this;
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self->reflect_page, length);
        out[length] = 0;
    }
    return (char*)self->reflect_page;
}
void NetworkWiiMediator::getReflectPage(u8 page)
{
    if (getReflectService() != NULL) {
        setReflectServicePage(getReflectService(), (u32)page);
    }
}
void NetworkWiiMediator::agreeReflect()
{
    if (getReflectService() != NULL) {
        reflectServiceAgree(getReflectService());
    }
}

/* the unmangled helpers */
/* Returns the reflect event id's packet kind: the ids the sub-machine knows, each mapped to the
 * kind the reflect reply carries, and 48 for everything else. */
s32 dispatchReflectEvent()
{
    switch (getReflectEventId()) {
    case 65:
        return 1;
    case 66:
        return 2;
    case 176:
        return 3;
    case 67:
        return 4;
    case 70:
        return 5;
    case 18:
        return 6;
    case 71:
        return 7;
    case 73:
        return 8;
    case 74:
        return 9;
    case 76:
        return 10;
    case 77:
        return 11;
    case 78:
        return 12;
    case 110:
        return 13;
    case 79:
        return 14;
    case 144:
        return 15;
    case 80:
        return 16;
    case 81:
        return 17;
    case 169:
        return 18;
    case 82:
        return 19;
    case 83:
        return 21;
    case 1:
        return 22;
    case 136:
        return 23;
    case 173:
        return 24;
    case 88:
        return 26;
    case 94:
        return 27;
    case 95:
        return 28;
    case 96:
        return 29;
    case 171:
        return 30;
    case 97:
        return 31;
    case 98:
        return 32;
    case 172:
        return 33;
    case 99:
        return 34;
    case 100:
        return 35;
    case 174:
        return 36;
    case 153:
        return 37;
    case 103:
        return 39;
    case 104:
        return 40;
    case 105:
        return 41;
    case 107:
        return 42;
    case 108:
        return 43;
    case 128:
        return 44;
    case 109:
        return 45;
    case 168:
        return 46;
    case 49:
        return 47;
    case 152:
        return 49;
    case 154:
        return 50;
    default:
        return 48;
    }
}
/* Maps the console language to the reflect mode byte the service is set to. */
s32 getReflectModeFromLanguage()
{
    switch (getLanguage()) {
    case 0:
        return 5;
    case 1:
        return 1;
    case 2:
        return 3;
    case 3:
        return 2;
    case 4:
        return 10;
    case 5:
        return 4;
    case 6:
        return 9;
    case 7:
        return 11;
    case 8:
        return 6;
    case 9:
        break;
    default:
        return 12;
    }
}
void setMediatorBufferA(NetworkWiiMediatorFields* self, const u8* src)
{
    if (src == NULL) {
        return;
    }
    memcpy(self->buffer_A, src, 262);
}
void getMediatorBufferA(NetworkWiiMediatorFields* self, u8* dst)
{
    if (dst == NULL) {
        return;
    }
    memcpy(dst, self->buffer_A, 262);
}
void setMediatorBufferB(NetworkWiiMediatorFields* self, const u8* src)
{
    if (src == NULL) {
        return;
    }
    memcpy(self->buffer_B, src, 262);
}
void getMediatorBufferB(NetworkWiiMediatorFields* self, u8* dst)
{
    if (dst == NULL) {
        return;
    }
    memcpy(dst, self->buffer_B, 262);
}
void getMediatorNameBuffer(NetworkWiiMediatorFields* self, u32* out1, u8* out2)
{
    *out1 = (u32)&self->name_buffer_68B;
    *out2 = self->flag_78B;
}
void setMediatorFlag78B(NetworkWiiMediatorFields* self, u8 value)
{
    self->flag_78B = value;
}
void getMediatorField288(NetworkWiiMediatorFields* self, u32* out)
{
    *out = (u32)&self->field_288;
}
void setMediatorFlag78C(NetworkWiiMediatorFields* self, u8 value)
{
    self->flag_78C = value;
}
void getMediatorFlag78C(NetworkWiiMediatorFields* self, u8* out)
{
    *out = self->flag_78C;
}
void resetMediatorState(NetworkWiiMediatorFields* self)
{
    initializeNetworkMediator(self, 0);
}
/* Builds the Pat interface the opening reads through, spawns the GameSpy worker thread if it is not
 * running, then points the two Pat buffer slots at this mediator's term/maintenance blocks. */
void initializeNetworkMediator(NetworkWiiMediatorFields* self, u32 value)
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        new GameSpyInterfaceThread();
    }
    setTermVersion(getInstance_(), 0);
    setPatBuffer(getInstance_(), 1, (char*)self->single_line_buffer, 1);
    setPatRange(getInstance_(), 1, self->pat_terms_ptr, self->pat_terms_size);
    setPatRange(getInstance_(), 2, self->pat_maintenance_ptr, self->pat_maintenance_size);
    self->field_24 = value;
}
void resetMediatorFlags(NetworkWiiMediatorFields* self)
{
    self->flag_1C = 1;
    self->flag_1E = 0;
    self->flag_1F = 0;
    self->flag_68A = 0;
    self->flag_21 = 0;
}
void resetMediatorFlag1D(NetworkWiiMediatorFields* self)
{
    self->flag_1D = 1;
    self->flag_1C = 0;
}
void loadPatInterfaceBuffers()
{
    if (getInstance_() == NULL) {
        return;
    }
    setTermVersion(getInstance_(), 0);
    getInstance_()->setTermsBuffer(NULL, 0);
    getInstance_()->setMaintenanceBuffer(NULL, 0);
    getInstance_()->setAnnounceBuffer(NULL, 0);
    getInstance_()->setNoChargeBuffer(NULL, 0);
    getInstance_()->setPatchMessageBuffer(NULL, 0);
    setPatBuffer(getInstance_(), 1, NULL, 0);
    setPatRange(getInstance_(), 1, 0, 0);
    setPatRange(getInstance_(), 2, 0, 0);
    PatInterface_clear(getInstance_());
    if (PatInterface_isReady(getInstance_()) != 0) {
        return;
    }
    PatInterface* instance = getInstance_();
    if (instance != NULL) {
        instance->finalize(1);
    }
}
u32 getMediatorField24(NetworkWiiMediatorFields* self) { return self->field_24; }
u8 getMediatorFlag6B(NetworkWiiMediatorFields* self) { return self->field_6B; }
u8 getMediatorFlag60D1(NetworkWiiMediatorFields* self) { return self->flag_60D1; }
void updatePatInterface180(NetworkWiiMediatorFields* self, u32 a, u32 b, u32 c)
{
    (void)self;
    if (getInstance_() != NULL) {
        updatePatInterface(getInstance_(), a, b, c);
    }
}
void updateTermVersion(NetworkWiiMediatorFields* self, u32 value)
{
    (void)self;
    if (getInstance_() != NULL) {
        setTermVersion(getInstance_(), value);
    }
}
void updateOpeningState(NetworkWiiMediatorFields* self, u32 slot, u32 address, u32 size)
{
    if (slot == 2) {
        if (getInstance_() != NULL) {
            setPatRange(getInstance_(), 2, address, size);
        }
        self->pat_maintenance_ptr = address;
        self->pat_maintenance_size = size;
        return;
    }
    if (getInstance_() != NULL) {
        setPatRange(getInstance_(), 1, address, size);
    }
    self->pat_terms_ptr = address;
    self->pat_terms_size = size;
}
u64 updateServerTime(NetworkWiiMediatorFields* self)
{
    if (getInstance_() != NULL) {
        u32 seconds = getServerTime(getInstance_());
        u64 stamp = setServerTimeResult(self) + seconds;
        stamp = stamp * 1000000;
        return stamp + 0xDCDCC1A9170000ull;
    }
    return 0;
}
/* The timestamp the account is stamped with: the mediator's own, or the Pat singleton's when the
 * mediator has none yet.  The fallback lands in the *high* half of the 64-bit value, the way the
 * retail code builds it (`li r3,0` + `mr r4,<result>`). */
#pragma peephole off
u64 setServerTimeResult(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        u64 stamp = 0;
        if (getNetworkWiiMediator() != NULL) {
            stamp = getMediatorTimestamp(getNetworkWiiMediator());
        }
        if (stamp != 0) {
            return stamp;
        }
        return (u64)getPatServerTime(getInstance_());
    }
    return 0;
}
u64 getMediatorTimestamp(NetworkWiiMediatorFields* self)
{
    return self->stamp_3A10;
}
void setMediatorState68A(NetworkWiiMediatorFields* self, u8 value)
{
    if ((u32)value == 0 && getNetworkWiiMediator() != NULL) {
        setMediatorTimestamp(getNetworkWiiMediator(), 0);
    }
    self->flag_68A = value;
}
#pragma peephole on
void setMediatorTimestamp(NetworkWiiMediatorFields* self, u64 value)
{
    self->stamp_3A10 = value;
}
void getMediatorState68A(NetworkWiiMediatorFields* self, u8* out)
{
    *out = self->flag_68A;
}
s32 queryOpeningFlag208(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return isSubState_8254_3(getInstance_());
    }
    return 0;
}
s32 queryOpeningFlag250(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return isSubState_894F_4or6(getInstance_());
    }
    return 0;
}
s32 queryOpeningFlag278(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return isSubState_894F_6(getInstance_());
    }
    return 0;
}
s32 queryOpeningFlag290(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return isSubState_894F_5(getInstance_());
    }
    return 0;
}
s32 queryOpeningFlag2A8(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return isSubState_894F_2(getInstance_());
    }
    return 0;
}
s32 queryOpeningFlag2C0(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return isSubState_894F_3(getInstance_());
    }
    return 0;
}
s32 getAccountQuery1(NetworkWiiMediatorFields* self) { (void)self; return 0; }
s32 getAccountQuery2(NetworkWiiMediatorFields* self) { (void)self; return 0; }
s32 getAccountQuery3(NetworkWiiMediatorFields* self) { (void)self; return 0; }
s32 getAccountQuery4(NetworkWiiMediatorFields* self) { (void)self; return 0; }
s32 getAccountQuery5(NetworkWiiMediatorFields* self) { (void)self; return 0; }
char* getReflectPageText(char* self, char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self + 0x78D, length);
        out[length] = 0;
    }
    return self + 0x78D;
}
char* getAccountName(NetworkWiiMediatorFields* self, char* out, u32 size)
{
    (void)self;
    if (getInstance_() != NULL) {
        char* name = getPatAccountName(getInstance_());
        if (name != 0) {
            if (size != 0) {
                u32 length = (strlen(name) < size - 1) ? strlen(name) : size - 1;
                memcpy(out, name, length);
                out[length] = 0;
            }
            return name;
        }
    }
    return 0;
}
s32 getWarningUInt(NetworkWiiMediatorFields* self)
{
    (void)self;
    if (getInstance_() != NULL) {
        return getWarningUInt2(getInstance_());
    }
    return 0;
}
void getReflectPageRange(NetworkWiiMediatorFields* self, u32* out1, u32* out2)
{
    *out1 = self->field_28;
    *out2 = self->field_2C;
}
void getReflectField30(NetworkWiiMediatorFields* self, u32* out) { *out = self->field_30; }
void getReflectField34(NetworkWiiMediatorFields* self, u32* out) { *out = self->field_34; }
void getReflectField38(NetworkWiiMediatorFields* self, u32* out) { *out = self->field_38; }
void getReflectName3C(NetworkWiiMediatorFields* self, char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 32) ? size - 1 : 31;
        memcpy(out, (char*)&self->reflect_name_3C, length);
        out[length] = 0;
    }
}
void getMediaVersionString(NetworkWiiMediatorFields* self, char* out, u32 size)
{
    (void)self;
    u32 length = 0;
    if (size != 0) {
        if (getInstance_() != NULL) {
            char* text = getMediaVersion(getInstance_());
            length = (strlen(text) < size) ? strlen(text) : size - 1;
            memcpy(out, text, length);
        }
        out[length] = 0;
    }
}
void getStr1String(NetworkWiiMediatorFields* self, char* out, u32 size)
{
    (void)self;
    u32 length = 0;
    if (size != 0) {
        if (getInstance_() != NULL) {
            char* text = getStr1(getInstance_());
            length = (strlen(text) < size) ? strlen(text) : size - 1;
            memcpy(out, text, length);
        }
        out[length] = 0;
    }
}
void getReflectName5C(NetworkWiiMediatorFields* self, char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 32) ? size - 1 : 31;
        memcpy(out, (char*)&self->reflect_name_5C, length);
        out[length] = 0;
    }
}
void setReflectPageRange(NetworkWiiMediatorFields* self, u32 address, u32 size)
{
    if (getInstance_() != NULL) {
        setPatReflectPageRange(getInstance_(), address, size);
    }
    self->field_28 = address;
    self->field_2C = size;
}
void setReflectField30(NetworkWiiMediatorFields* self, u32 value)
{
    if (getInstance_() != NULL) {
        setPatReflectField30(getInstance_(), value);
    }
    self->field_30 = value;
}
void setReflectField34(NetworkWiiMediatorFields* self, u32 value)
{
    if (getInstance_() != NULL) {
        setPatReflectField34(getInstance_(), value);
    }
    self->field_34 = value;
}
void setReflectField38(NetworkWiiMediatorFields* self, u32 value)
{
    if (getInstance_() != NULL) {
        setPatReflectField38(getInstance_(), value);
    }
    self->field_38 = value;
}
void setReflectName3C(NetworkWiiMediatorFields* self, char* name)
{
    if (getInstance_() != NULL) {
        setPatReflectName3C(getInstance_(), name);
    }
    u32 length = (strlen(name) < 31) ? strlen(name) : 31;
    memcpy((char*)&self->reflect_name_3C, name, length);
    ((char*)&self->reflect_name_3C)[length] = 0;
}
void setReflectName5C(NetworkWiiMediatorFields* self, char* name)
{
    if (getInstance_() != NULL) {
        setPatReflectName5C(getInstance_(), name);
    }
    u32 length = (strlen(name) < 31) ? strlen(name) : 31;
    memcpy((char*)&self->reflect_name_5C, name, length);
    ((char*)&self->reflect_name_5C)[length] = 0;
}
void updatePatField854(NetworkWiiMediatorFields* self, u32 value)
{
    (void)self;
    if (getInstance_() != NULL) {
        setPatField854(getInstance_(), value);
    }
}
void updatePatField860(NetworkWiiMediatorFields* self, u32 value)
{
    (void)self;
    if (getInstance_() != NULL) {
        setPatField860(getInstance_(), value);
    }
}
s32 isShiftJisLeadByte(NetworkWiiMediatorFields* self, u8 value)
{
    (void)self;
    if ((value >= 129 && value <= 159) || (value >= 224 && value <= 239)) {
        return 1;
    }
    return 0;
}
#pragma peephole off
s32 isNameSymbolChar(NetworkWiiMediatorFields* self, char value)
{
    (void)self;
    u8 symbol = (u8)value;
    if (symbol == 0 || symbol >= 144 ||
        (symbol >= 0x30 && symbol <= 0x39) ||
        (symbol >= 0x41 && symbol <= 0x5A) ||
        (symbol >= 0x61 && symbol <= 0x7A)) {
        return 0;
    }
    return 1;
}
#pragma peephole on
void parseReflectLines(NetworkWiiMediatorFields* self, s32 source)
{
    char** table;
    s32 count;
    u32 length;
    char* text;

    switch (source) {
    default:
        return;
    case 0:
        text = (char*)self->pat_terms_ptr;
        table = self->line_table_a;
        count = 1024;
        length = self->pat_terms_size;
        break;
    case 1:
        text = (char*)self->single_line_buffer;
        table = self->line_table_c;
        count = 1;
        length = 1;
        break;
    case 2:
        text = (char*)self->pat_maintenance_ptr;
        table = self->line_table_b;
        count = 1024;
        length = self->pat_maintenance_size;
        break;
    }

    memset(table, 0, count * 4);
    text[length - 1] = 0;
    while (*text == '\r' || *text == '\n') {
        *text++ = 0;
    }
    if (*text == 0) {
        return;
    }
    for (s32 i = count; i > 0; i--) {
        *table = text++;
        while (*text != '\r' && *text != '\n' && *text != 0) {
            text++;
        }
        while (*text == '\r' || *text == '\n') {
            *text++ = 0;
        }
        if (*text == 0) {
            break;
        }
        table++;
    }
}
/* Decodes one reflect reply packet at `in` into `out`: the 0xC3 lead-byte escapes, the
 * lower-case -> upper-case fold, and the symbol map the reflect service keys on.  Returns the number
 * of bytes written (0 or 1 has no meaning here: 1 for a decoded byte, 2 for a two-byte escape). */
s32 parseReflectPacket(NetworkWiiMediatorFields* self, char* out, const char* in, u32 unused4,
                       u32 length, s32 allowSymbolMap)
{
    (void)self;
    (void)unused4;
    if (length > 1 && (u8)in[0] == 0xC3 && (u8)in[1] >= 0xA0 && (u8)in[1] <= 0xBF) {
        if ((u8)in[1] == 0xA0) {
            out[0] = (char)0xC5;
            out[1] = (char)0xB8;
            return 2;
        }
        if ((u8)in[1] != 0xB7) {
            out[0] = in[0];
            out[1] = (char)(in[1] - 32);
            return 2;
        }
    }
    char c = in[0];
    if ((u8)(c - 97) <= 25) {
        out[0] = (char)(c - 32);
    } else if (allowSymbolMap != 0) {
        switch (c - 33) {
        case 31:
            out[0] = 'A';
            break;
        case 3:
            out[0] = 'S';
            break;
        case 20:
            out[0] = 'S';
            break;
        case 7:
            out[0] = 'C';
            break;
        case 27:
            out[0] = 'C';
            break;
        case 0:
            out[0] = 'I';
            break;
        case 16:
            out[0] = 'I';
            break;
        case 17:
            out[0] = 'Z';
            break;
        case 15:
            out[0] = 'O';
            break;
        default:
            out[0] = c;
            break;
        }
    } else {
        out[0] = c;
    }
    return 1;
}

#pragma peephole off
s32 validateReflectName(NetworkWiiMediatorFields* self, const char* name)
{
    const char* cursor;
    s32 count;
    count = 0;
    for (cursor = name; *cursor != 0; cursor++) {
        if (isNameSymbolChar(self, *cursor) != 0) {
            break;
        }
        count++;
    }
    return count;
}
/* Scans one reflected name and reports the shape the reflect service has to be told about: the
 * characters a `*`-escape covers go into `skipCount` and the matched escape into `flags`, so the
 * caller knows which of the four name forms the text is.  Returns the character count left over. */
s32 buildReflectPacket(NetworkWiiMediatorFields* self, const char* text, s32* skipCount, s32* flags)
{
    (void)self;
    s32 count = 0;
    *skipCount = 0;
    *flags = 0;
    for (s32 i = 0; text[i] != 0; i++) {
        char c = text[i];
        if (c == '*') {
            if (i == 1) {
                if (text[i - 1] == '.') {
                    *flags |= 1;
                    *skipCount = *skipCount + 2;
                    count--;
                    continue;
                }
            } else if (i > 1) {
                if (text[i - 1] == '.' && text[i + 1] == 0) {
                    *flags |= 2;
                    count--;
                    break;
                }
            }
        } else if (c == '^' && i == 0) {
            *flags |= 4;
            *skipCount = *skipCount + 1;
            continue;
        } else if (c == '$' && text[i + 1] == 0) {
            *flags |= 8;
            break;
        }
        count++;
    }
    switch (*flags) {
    case 9:
    case 1:
        *flags = 8;
        break;
    case 6:
    case 2:
        *flags = 4;
        break;
    case 12:
        *flags = 0;
        break;
    }
    return count;
}
#pragma peephole on
