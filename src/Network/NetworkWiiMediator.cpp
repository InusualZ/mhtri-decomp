/*
 * Network/NetworkWiiMediator.cpp - the `NetworkWiiMediator` unit (`.text` 0x80413450..0x80417BC0).
 *
 * ROUND 3 FOLD (network pilot).  One TU: the mediator head 0x80413450..0x80413C64 (`mediatorEventCallback`, the
 * constructors and destructors, `getReflectService`, `update`, `isMaintenanceMode`, `getReflectEventId`,
 * `getLanguage` - formerly the tail of `Network/network_layer_io.cpp`), the class band
 * 0x80413C64..0x804155D4 (the original text below) and the opening part 0x804155D4..0x80417BC0 (the former unit
 * `Network/network_opening.cpp`, folded in byte-identical - its header follows the class band's, below).  Evidence:
 * the band's `.data` (the three jump tables, the opening strings and the two mediator tables 0x80602968/0x80602978)
 * reads as one TU's D/S/V run with no seam until V->D at 0x80602988, and the tables are stored by the constructor
 * 0x80413480 (0x80602968) and by 0x80413714 (0x80602978), inside the head.  The left cut is 0x80413450, not
 * 0x80413384: `NetworkRandom`'s table 0x806024A0 precedes the first jump table 0x806024B8, so `NetworkRandom`
 * (0x80413384..0x80413450) is `Network/NetworkPool.cpp`'s.  Sections: extab 0x8001C9B4..0x8001CD94,
 * extabindex 0x8003D278..0x8003D764, .rodata 0x80570E70..0x80570E98, .data 0x806024B8..0x80602988, .sdata2
 * 0x8079C874..0x8079C878.
 * The opening part keeps its file-scope pragmas (`auto_inline off`, `pool_data off`, peephole) from where it
 * starts, so the class band above it compiles as before.
 *
 * THE HEAD'S CLASSES (round 4).  `NetworkMediator` (table 0x80602978: its destructor 0x804136D0 and a pure `update`)
 * and `NetworkWiiMediator : NetworkMediator` (table 0x80602968: its destructor 0x80413724 and `update` 0x804138EC),
 * with the terms object `PatTerms` (`menu/PatTerms.h`, constructor/destructor in `menu/menu_plsearch.cpp`) as the
 * member at +0x6DD8; this unit emits both tables (the `.data` claim is byte-complete).  `NetworkMediator` is a GUESS
 * from the `sNetworkLibrary`/`sNetworkLibraryWii` pairing; `update`, `deleteNetworkPool` and the `flag_21` setters
 * `enableMediatorLinkError`/`disableMediatorLinkError` are GUESSes from the bodies.
 * The constructors and destructors sit under `#pragma peephole off` (retail stages the table address in r0 and keeps
 * `extsh`+`cmpwi` on the destructor flag).
 * The `.sdata2` claim starts at 0x8079C870 (the constructor's `0.0f`, then `initMediatorTerms`'s `1.0f`; NetworkPool's
 * only pool read is 0x8079C868).  Head residuals: `isMaintenanceMode`, `getReflectEventId`, `getLanguage`, 0x80413980/0x80413A34 (the five 4-byte words copied in
 * and out), 0x80413B18 (the SO link check) are unwritten (their SC/SO callees have no
 * declaration a consumer can include yet).
 *
 * THE CLASS BAND (0x80413C64..0x804155D4) - its notes as written before the fold:
 *
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
 * `setPatReflectField30`, `fn_8041241C` -> `getNetworkPool`, `fn_8041A1C4` ->
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
 * (0x8041C66C), whose owner `Network/GameSpyInterfaceThread.cpp` turned its `create()` into the class's
 * constructor in the same change; and `fn_803D6A98` -> `GameSpyInterfaceThread_getInstance`
 * (0x803D6A98, `Network/NetworkSessionManager.cpp`), with the referrers in `include/Network/GameSpyInterfaceThread.h` and
 * `src/Network/GameSpyInterfaceThread.cpp`.  Re-measured: every `.text` row and the whole-project progress are
 * unchanged, and `flipcheck.py` reports exactly the four complaints it did before, no new one.
 *
 * Those four rows were **renamed again** by `worker/fn-803d3ce8-2477`, to the owners' own definition
 * spellings (the map rows at 36030, 36676, 36678 and 57701), and the referrers this file and
 * `src/Network/GameSpyInterfaceThread.cpp` carried were left on the old ones - `flipcheck.py` then reported "3
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

/*
 * THE OPENING PART (0x804155D4..0x80417BC0) - the notes of the former `Network/network_opening.cpp`:
 *
 * transfer slots (the `NetworkWiiMediator::ECStart()` / `openingStart()` / `openingStop()` strings).
 *
 * `.text` 0x804155D4..0x80417BC0.  Sections: extab 0x8001CBF4..0x8001CD94; extabindex 0x8003D524..0x8003D764;
 * .rodata 0x80570E70..0x80570E98 (read by 0x80416A30); .data 0x80602824..0x80602968; .sdata2 0x8079C874..0x8079C878 (read by `initMediatorTerms`).
 *
 * WHAT IT IS.  Methods of `NetworkWiiMediator` (the strings name the class): the game-info copy pair, the
 * terms entry points, the transfer-slot family (a 0x1000-byte record queue, an activity byte, a timer and a
 * mode record per slot, all in the mediator's fields from +0x2D96) and the opening/EC steps.  The functions
 * other units already call by a C name (`setGameInfo2d1c`, `initMediatorTerms`, `setMediatorTransfer*`, ...)
 * keep it; the ones no source names yet are members (`NetworkWiiMediator::openTransferSlot`, ...).
 *
 * NAMES.  `ECStart`, `openingStart` and `openingStop` are the strings' own; every other member name is a
 * GUESS from the body (the field it reads or writes), and so are the callees this pass named in their
 * owners' headers (`reportPatError`, `notifyPatEvent`, `openPatInterface`, `mediatorEventCallback`,
 * `NetworkPool::start`/`isECStarted`).  The slot family's fields are named in include/Network/NetworkWiiMediator.h.
 * `NetworkPool::isECStarted` (0x8041793C) is the pool's accessor, emitted in this range.
 *
 * BOUNDARY.  The base library class `sNetworkLibrary` that followed (0x80417BC0..0x8041891C) is its own unit,
 * `Network/sNetworkLibrary.cpp`, with the whole `.data` and `.sbss` this unit used to claim (its header has the
 * seam evidence); the worker-thread entry points after it are `Network/sNetworkLibraryWii.cpp`'s.  `.data`
 * 0x80602824..0x80602968 is the five strings only the opening steps read (emitted here: 321 of the claimed 324 B,
 * the rest is the trailing alignment).  The two tables after them (0x80602968, 0x80602978)
 * close this TU's `.data` by MWCC's order but are read by `Network/network_layer_io.cpp` and are code-pointer
 * tables (rule 10), so they stay unclaimed until a class here emits them.
 *
 * FLAGS.  `-O3` in place of the lib's `-O4,p` (configure.py).  `#pragma peephole off` (retail keeps `extsb` +
 * `cmpwi` on the slot argument: getTransferSlotFlag 31.25 -> 93.44 -> 100 with the range spelling below),
 * back on around `setMediatorTransferMode` (its raw `stb` of the u32 mode: 95.96 -> 100); `#pragma
 * auto_inline off` (retail calls `clearTransferQueue` where the lib's `-inline auto` inlines it); `#pragma
 * pool_data off` (each log string its own `lis`/`addi`).  Shapes: the slot range test is written
 * `slot < 0 || 4 <= slot` with an early return (MWCC folds `slot >= 4` into one unsigned compare), the queue
 * is one flat array indexed by `slot << 12`, and `applyEvent` takes the code as `s32` (the `addis` before the
 * unsigned case compares).
 *
 * RESIDUALS.  `popTransferRecord` 97.19 (retail computes `&length` before the queue address for the first
 * `memcpy`, and keeps the activity byte's address in r5); `pushTransferRecord` 98.71 (callee-saved registers
 * r29-r31 permuted; declaration order and a pointer-free spelling measured, neither moves it);
 * `setGameInfo2d1c` 99.85 (the title length lands in r29 where retail reuses r28).  `getNASToken` returns
 * the token inside `DWCSvlResult` (owner `DWCi/dwc_nasfunc.h`).
 *
 * UNWRITTEN.  The word filter 0x804155D4 (2520 B) and `postMediatorRecord`; the forwarding wrappers over the
 * `NetworkPool` singleton 0x80416028..0x80416578 (its methods are `Network/network_layer_io.cpp`'s, unnamed);
 * the sound helpers 0x804170D8..0x804171A4.
 *
 * TERMS WRAPPERS (0x80416800..0x80416C18, written from request net2-l4-cff5#3).  Every name this pass gave -
 * `cancelTermsUpdate`, `getTermsProgressLevel` (a forwarding thunk), `getMediatorTermsProgressLevel`,
 * `getMediatorTermsProgress`, `set/getMediatorTermsFlag`, the leaf accessors `isPatTermsReady` and
 * `set/getPatTermsFlag`, and the `menu/menu_plsearch.cpp` callees `initPatTerms`, `requestPatTermsCheck`,
 * `requestPatTermsUpdate`, `cancelPatTermsUpdate`, `getPatTermsProgress` - is a GUESS from the bodies.  Each leaf
 * accessor sits right after its first caller, the layout MWCC gives an uninlined inline function.
 * `setPatTermsFlag` takes a `u32` under `#pragma peephole on` (the raw `stb`, like `setMediatorTransferMode`);
 * a `u8` parameter puts a `clrlwi` in it (47.50) or in `setMediatorTermsFlag` (96.25).
 */
#include "types.h"
#include "Network/NetworkWiiMediator.h"
#include "Network/PatInterface.h"            /* the singleton's Pat accessors */
#include "Network/NetworkReflectService.h"   /* the reflect service's entry points */
#include "Network/NetworkPool.h"          /* getNetworkPool */
#include "Network/gamespy_interface_types.h"  /* the worker thread `initializeNetworkMediator` spawns */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Network/NetworkSessionBase.h"    /* getNetworkBinaryState */
#include "enemy/em020_ai.h"                /* getInstance_ */
#include "unsplit/Network.h"               /* getNetworkLogger */
#include "Network/network_pat_control.h"     /* struct PatTerms; getPatTerms (through NetworkSessionManagerPat.h) */
#include "menu/menu_plsearch.h"            /* initPatTerms and the terms object's requests */
#include "MSL_C/alloc.h"                   /* memmove, snprintf */
#include "MSL/strlen.h"

/* The console's bus clock in Hz (low memory 0x800000F8), spelled exactly as `unsplit/OS.h` defines `OS_BUS_CLOCK`:
   that header clashes with this unit's includes (its thread prototypes), so the one-line spelling is kept locally. */
#define OS_BUS_CLOCK (*(u32*)0x800000F8)


/* The reflect sub-service the mediator starts, stops and agrees through is `NetworkReflectService`
 * (`Network/NetworkReflectService.h`): this band dispatches into its `+0x08` slot and allocates it with
 * `new` - its out-of-line constructor keeps the allocation alive across the call (the `mr r31,r3` of
 * `reflectInit`). */

/* The network singleton the two buffer loaders forward to is `PatInterface` (`Network/PatInterface.h`):
 * this band allocates it and calls its five buffer setters by their mangled member names. */

/* The GameSpy worker thread `initializeNetworkMediator` spawns is `GameSpyInterfaceThread`
 * (`Network/gamespy_interface_types.h`, the class alone - the GameSpy band's full header is not needed). */

/* `operator new` is what the band's allocation lowers to (`__nw__FUl`). */
void* operator new(unsigned long size);   /* untyped: allocation returns a raw byte range */

extern "C" {
void initializeNetworkMediator(NetworkWiiMediator* self, u32 value);

u32   getMediatorField24(NetworkWiiMediator* self);
u8    getNetworkPoolProgress(NetworkPool* self);
u8    getPatOpeningState(PatInterface* self);
void  setNetworkPoolTimestamp(NetworkPool* self, u64 value);
u64   getNetworkPoolTimestamp(NetworkPool* self);
s32   getAccountQuery1(NetworkWiiMediator* self);
s32   getAccountQuery2(NetworkWiiMediator* self);
s32   getAccountQuery3(NetworkWiiMediator* self);
s32   getAccountQuery4(NetworkWiiMediator* self);
s32   getAccountQuery5(NetworkWiiMediator* self);

s32  getReflectModeFromLanguage();
u64  updateServerTime(NetworkWiiMediator* self);
u64  setServerTimeResult(NetworkWiiMediator* self);
s32  queryOpeningFlag208(NetworkWiiMediator* self);
s32  queryOpeningFlag250(NetworkWiiMediator* self);
s32  queryOpeningFlag290(NetworkWiiMediator* self);
char* getReflectPageText(char* self, char* out, u32 size);
void getMediaVersionString(NetworkWiiMediator* self, char* out, u32 size);
void getStr1String(NetworkWiiMediator* self, char* out, u32 size);
void updatePatField854(NetworkWiiMediator* self, u32 value);
void updatePatField860(NetworkWiiMediator* self, u32 value);
s32  isNameSymbolChar(NetworkWiiMediator* self, char value);
s32  isShiftJisLeadByte(NetworkWiiMediator* self, u8 value);
void parseReflectLines(NetworkWiiMediator* self, s32 source);
s32  parseReflectPacket(NetworkWiiMediator* self, char* out, const char* in, u32 unused4,
                        u32 length, s32 allowSymbolMap);
s32  validateReflectName(NetworkWiiMediator* self, const char* name);
s32  buildReflectPacket(NetworkWiiMediator* self, const char* text, s32* skipCount, s32* flags);

/* The singleton's Pat surface is declared in `Network/PatInterface.h`, the reflect service's in
 * `Network/NetworkReflectService.h` and the layer queries in `Network/PatInterface.h`. */

} /* extern "C" */

/* The network singleton the band forwards to: its owner's header (`enemy/em020_ai.h`) spells it `NetworkInstance`, and
 * this band holds it as the `PatInterface` it is (`Network/PatInterface.h`: the same object).  The opening part calls
 * the owner's spelling directly, so the two views meet here instead of in two declarations. */
static inline PatInterface* getPatInstance(void)
{
    return (PatInterface*)getInstance_();
}

extern "C" {

/* Forwards a Pat interface event to the mediator the callback was installed with. */
void mediatorEventCallback(u32 code, s32 a, s32 b, s32 c, const union GameSpyEventMsg* msg,
                           NetworkWiiMediator* mediator)
{
    mediator->applyEvent(code, a, b, c, msg);
}

}

/* The head keeps retail's unfused forms (the table address staged in r0, the destructors' extsh+cmpwi). */
#pragma peephole off

/* Builds the mediator: the terms object, every buffer and table cleared, the default term/maintenance texts and
   the four transfer slots closed and emptied. */
NetworkWiiMediator::NetworkWiiMediator()
{
    s32 slot;

    memset(cleared_004, 0, 4);
    memset(cleared_008, 0, 4);
    memset(cleared_00C, 0, 4);
    memset(cleared_010, 0, 4);
    memset(cleared_014, 0, 4);
    flag_1C = 0;
    flag_1D = 0;
    flag_1E = 0;
    flag_1F = 0;
    field_28 = 0;
    field_2C = 0;
    field_30 = 0;
    field_34 = 22;
    field_38 = 5;
    memset(reflect_name_3C, 0, 32);
    memset(reflect_name_5C, 0, 32);
    memset(buffer_A, 0, 262);
    memset(buffer_B, 0, 262);
    memset(name_buffer_68B, 0, 256);
    memset(field_288, 0, 1026);
    flag_68A = 0;
    flag_78B = 0;
    flag_78C = 0;
    library_started = 0;
    flag_21 = 0;
    field_24 = 0;
    memset(reflect_page, 0, 1024);
    memset(&terms_default, 0, 1);
    memset(&maintenance_default, 0, 1);
    memset(single_line_buffer, 0, 1);
    memset(line_table_a, 0, 4096);
    memset(line_table_b, 0, 4096);
    memset(line_table_c, 0, 4);
    pat_terms_ptr = (u32)&terms_default;
    pat_maintenance_ptr = (u32)&maintenance_default;
    pat_terms_size = 1;
    pat_maintenance_size = 1;
    memset(game_info, 0, 36);
    memset(game_info_name, 0, 17);
    memset(game_info_secret, 0, 17);
    memset(game_info_title, 0, 52);
    closeTransferSlots();
    for (slot = 0; slot < 4; slot++) {
        clearTransferQueue(slot);
    }
    transfer_mode = 0;
    transfer_flag_6DD1 = 1;
    transfer_flag_6DD2 = 1;
    transfer_level = 0.0f;
}

/* The interface owns nothing. */
NetworkMediator::~NetworkMediator()
{
}

/* Builds the interface (its table alone). */
NetworkMediator::NetworkMediator()
{
}

/* Tears the network down in order: the Pat interface, the reflect service, the GameSpy worker thread (waiting
   while it still runs), the terms check (waiting until it finishes) and the network pool. */
NetworkWiiMediator::~NetworkWiiMediator()
{
    if (getInstance_() != NULL) {
        PatInterface_clear(getInstance_());
        delete getInstance_();
    }
    if (getReflectService() != NULL) {
        finalizeReflectService(getReflectService());
        delete getReflectService();
    }
    if (GameSpyInterfaceThread::getInstance() != NULL) {
        while (GameSpyInterfaceThread::getInstance()->requestClose()) {
            OSSleepTicks(17 * (s64)(OS_BUS_CLOCK / 4 / 1000));
        }
        delete GameSpyInterfaceThread::getInstance();
    }
    if (getPatTerms() != NULL) {
        requestPatTermsCheck(getPatTerms());
        while (updatePatTerms(getPatTerms()) == 0) {
            OSSleepTicks(17 * (s64)(OS_BUS_CLOCK / 4 / 1000));
        }
    }
    deleteNetworkPool();
}

#pragma peephole on

extern "C" {

/* The reflect service singleton (null until `reflectInit` builds it). */
NetworkReflectService* getReflectService(void)
{
    return sNetworkReflectService;
}

}

/* The per-frame step: the network pool and the terms object first, then the opening stop or start the flags
   request, or else the reflect service. */
void NetworkWiiMediator::update()
{
    if (getNetworkPool() != NULL) {
        getNetworkPool()->update();
    }
    if (getPatTerms() != NULL) {
        updatePatTerms(getPatTerms());
    }
    if (flag_1D != 0) {
        openingStop();
    } else if (flag_1C != 0) {
        openingStart();
    } else if (getReflectService() != NULL) {
        updateReflectService(getReflectService());
    }
}

extern "C" {

/* Makes the link check (0x80413B18) report the link as down (error 0x80000008, code 93) without asking SO. */
void enableMediatorLinkError(NetworkWiiMediator* self)
{
    self->flag_21 = 1;
}

/* Lets the link check ask SO again. */
void disableMediatorLinkError(NetworkWiiMediator* self)
{
    self->flag_21 = 0;
}

}

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
    delete getReflectService();
}
#pragma peephole off
s32  NetworkWiiMediator::getOpeningProgress()
{
    NetworkWiiMediator* self = (NetworkWiiMediator*)this;
    if (self->flag_1C != 0) {
        self->flag_1E = self->flag_1C;
    }
    if (getPatInstance() != NULL) {
        u8 state = getPatOpeningState(getPatInstance());
        if (state != 0 && state != 90) {
            self->flag_1F = state;
        }
    }
    if (self->flag_1E == 6) {
        if (queryOpeningFlag208(self) == 0) {
            return 100;
        }
        u8 progress = 0;
        if (getNetworkPool() != NULL) {
            progress = getNetworkPoolProgress(getNetworkPool());
        }
        return progress + 90;
    }
    return self->flag_1F + self->flag_1E * 10;
}
#pragma peephole on
s32  NetworkWiiMediator::getOpeningTermsVersion()
{
    if (getPatInstance() != NULL) {
        return getTermsVersion(getPatInstance());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningMaintenanceTerms()
{
    if (getPatInstance() != NULL) {
        return ::isOpeningMaintenanceTerms(getPatInstance());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningMaintenanceServer()
{
    if (getPatInstance() != NULL) {
        return ::isOpeningMaintenanceServer(getPatInstance());
    }
    return 0;
}
s32  NetworkWiiMediator::isOpeningAnnounce()
{
    if (getPatInstance() != NULL) {
        return ::isOpeningAnnounce(getPatInstance());
    }
    return 0;
}
char* NetworkWiiMediator::getAccountBan(char* out, u32 size)
{
    NetworkWiiMediator* self = (NetworkWiiMediator*)this;
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self->reflect_page, length);
        out[length] = 0;
    }
    return (char*)self->reflect_page;
}
char* NetworkWiiMediator::getAccountWarning(char* out, u32 size)
{
    NetworkWiiMediator* self = (NetworkWiiMediator*)this;
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self->reflect_page, length);
        out[length] = 0;
    }
    return (char*)self->reflect_page;
}
char* NetworkWiiMediator::getAccountWaitQueue(char* out, u32 size)
{
    NetworkWiiMediator* self = (NetworkWiiMediator*)this;
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
void setMediatorBufferA(NetworkWiiMediator* self, const u8* src)
{
    if (src == NULL) {
        return;
    }
    memcpy(self->buffer_A, src, 262);
}
void getMediatorBufferA(NetworkWiiMediator* self, u8* dst)
{
    if (dst == NULL) {
        return;
    }
    memcpy(dst, self->buffer_A, 262);
}
void setMediatorBufferB(NetworkWiiMediator* self, const u8* src)
{
    if (src == NULL) {
        return;
    }
    memcpy(self->buffer_B, src, 262);
}
void getMediatorBufferB(NetworkWiiMediator* self, u8* dst)
{
    if (dst == NULL) {
        return;
    }
    memcpy(dst, self->buffer_B, 262);
}
void getMediatorNameBuffer(NetworkWiiMediator* self, u32* out1, u8* out2)
{
    *out1 = (u32)&self->name_buffer_68B;
    *out2 = self->flag_78B;
}
void setMediatorFlag78B(NetworkWiiMediator* self, u8 value)
{
    self->flag_78B = value;
}
void getMediatorField288(NetworkWiiMediator* self, u32* out)
{
    *out = (u32)&self->field_288;
}
void setMediatorFlag78C(NetworkWiiMediator* self, u8 value)
{
    self->flag_78C = value;
}
void getMediatorFlag78C(NetworkWiiMediator* self, u8* out)
{
    *out = self->flag_78C;
}
void resetMediatorState(NetworkWiiMediator* self)
{
    initializeNetworkMediator(self, 0);
}
/* Builds the Pat interface the opening reads through, spawns the GameSpy worker thread if it is not
 * running, then points the two Pat buffer slots at this mediator's term/maintenance blocks. */
void initializeNetworkMediator(NetworkWiiMediator* self, u32 value)
{
    if (getPatInstance() == NULL) {
        new PatInterface();
    }
    if (GameSpyInterfaceThread::getInstance() == NULL) {
        new GameSpyInterfaceThread();
    }
    setTermVersion(getPatInstance(), 0);
    setPatBuffer(getPatInstance(), 1, (char*)self->single_line_buffer, 1);
    setPatRange(getPatInstance(), 1, self->pat_terms_ptr, self->pat_terms_size);
    setPatRange(getPatInstance(), 2, self->pat_maintenance_ptr, self->pat_maintenance_size);
    self->field_24 = value;
}
void resetMediatorFlags(NetworkWiiMediator* self)
{
    self->flag_1C = 1;
    self->flag_1E = 0;
    self->flag_1F = 0;
    self->flag_68A = 0;
    self->flag_21 = 0;
}
void resetMediatorFlag1D(NetworkWiiMediator* self)
{
    self->flag_1D = 1;
    self->flag_1C = 0;
}
void loadPatInterfaceBuffers()
{
    if (getPatInstance() == NULL) {
        return;
    }
    setTermVersion(getPatInstance(), 0);
    getPatInstance()->setTermsBuffer(NULL, 0);
    getPatInstance()->setMaintenanceBuffer(NULL, 0);
    getPatInstance()->setAnnounceBuffer(NULL, 0);
    getPatInstance()->setNoChargeBuffer(NULL, 0);
    getPatInstance()->setPatchMessageBuffer(NULL, 0);
    setPatBuffer(getPatInstance(), 1, NULL, 0);
    setPatRange(getPatInstance(), 1, 0, 0);
    setPatRange(getPatInstance(), 2, 0, 0);
    PatInterface_clear(getPatInstance());
    if (PatInterface_isReady(getPatInstance()) != 0) {
        return;
    }
    PatInterface* instance = getPatInstance();
    delete instance;
}
u32 getMediatorField24(NetworkWiiMediator* self) { return self->field_24; }
u8 getNetworkPoolProgress(NetworkPool* self) { return self->progress; }
u8 getPatOpeningState(PatInterface* self) { return self->opening_state; }
void updatePatInterface180(NetworkWiiMediator* self, u32 a, u32 b, u32 c)
{
    (void)self;
    if (getPatInstance() != NULL) {
        updatePatInterface(getPatInstance(), a, b, c);
    }
}
void updateTermVersion(NetworkWiiMediator* self, u32 value)
{
    (void)self;
    if (getPatInstance() != NULL) {
        setTermVersion(getPatInstance(), value);
    }
}
void updateOpeningState(NetworkWiiMediator* self, u32 slot, u32 address, u32 size)
{
    if (slot == 2) {
        if (getPatInstance() != NULL) {
            setPatRange(getPatInstance(), 2, address, size);
        }
        self->pat_maintenance_ptr = address;
        self->pat_maintenance_size = size;
        return;
    }
    if (getPatInstance() != NULL) {
        setPatRange(getPatInstance(), 1, address, size);
    }
    self->pat_terms_ptr = address;
    self->pat_terms_size = size;
}
u64 updateServerTime(NetworkWiiMediator* self)
{
    if (getPatInstance() != NULL) {
        u32 seconds = getServerTime(getPatInstance());
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
u64 setServerTimeResult(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        u64 stamp = 0;
        if (getNetworkPool() != NULL) {
            stamp = getNetworkPoolTimestamp(getNetworkPool());
        }
        if (stamp != 0) {
            return stamp;
        }
        return (u64)getPatServerTime(getPatInstance());
    }
    return 0;
}
u64 getNetworkPoolTimestamp(NetworkPool* self)
{
    return self->timestamp;
}
void setMediatorState68A(NetworkWiiMediator* self, u8 value)
{
    if ((u32)value == 0 && getNetworkPool() != NULL) {
        setNetworkPoolTimestamp(getNetworkPool(), 0);
    }
    self->flag_68A = value;
}
#pragma peephole on
void setNetworkPoolTimestamp(NetworkPool* self, u64 value)
{
    self->timestamp = value;
}
void getMediatorState68A(NetworkWiiMediator* self, u8* out)
{
    *out = self->flag_68A;
}
s32 queryOpeningFlag208(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return isSubState_8254_3(getPatInstance());
    }
    return 0;
}
s32 queryOpeningFlag250(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return isSubState_894F_4or6(getPatInstance());
    }
    return 0;
}
s32 queryOpeningFlag278(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return isSubState_894F_6(getPatInstance());
    }
    return 0;
}
s32 queryOpeningFlag290(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return isSubState_894F_5(getPatInstance());
    }
    return 0;
}
s32 queryOpeningFlag2A8(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return isSubState_894F_2(getPatInstance());
    }
    return 0;
}
s32 queryOpeningFlag2C0(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return isSubState_894F_3(getPatInstance());
    }
    return 0;
}
s32 getAccountQuery1(NetworkWiiMediator* self) { (void)self; return 0; }
s32 getAccountQuery2(NetworkWiiMediator* self) { (void)self; return 0; }
s32 getAccountQuery3(NetworkWiiMediator* self) { (void)self; return 0; }
s32 getAccountQuery4(NetworkWiiMediator* self) { (void)self; return 0; }
s32 getAccountQuery5(NetworkWiiMediator* self) { (void)self; return 0; }
char* getReflectPageText(char* self, char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 1024) ? size - 1 : 1023;
        memcpy(out, self + 0x78D, length);
        out[length] = 0;
    }
    return self + 0x78D;
}
char* getAccountName(NetworkWiiMediator* self, char* out, u32 size)
{
    (void)self;
    if (getPatInstance() != NULL) {
        char* name = getPatAccountName(getPatInstance());
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
s32 getWarningUInt(NetworkWiiMediator* self)
{
    (void)self;
    if (getPatInstance() != NULL) {
        return getWarningUInt2(getPatInstance());
    }
    return 0;
}
void getReflectPageRange(NetworkWiiMediator* self, u32* out1, u32* out2)
{
    *out1 = self->field_28;
    *out2 = self->field_2C;
}
void getReflectField30(NetworkWiiMediator* self, u32* out) { *out = self->field_30; }
void getReflectField34(NetworkWiiMediator* self, u32* out) { *out = self->field_34; }
void getReflectField38(NetworkWiiMediator* self, u32* out) { *out = self->field_38; }
void getReflectName3C(NetworkWiiMediator* self, char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 32) ? size - 1 : 31;
        memcpy(out, (char*)&self->reflect_name_3C, length);
        out[length] = 0;
    }
}
void getMediaVersionString(NetworkWiiMediator* self, char* out, u32 size)
{
    (void)self;
    u32 length = 0;
    if (size != 0) {
        if (getPatInstance() != NULL) {
            char* text = getMediaVersion(getPatInstance());
            length = (strlen(text) < size) ? strlen(text) : size - 1;
            memcpy(out, text, length);
        }
        out[length] = 0;
    }
}
void getStr1String(NetworkWiiMediator* self, char* out, u32 size)
{
    (void)self;
    u32 length = 0;
    if (size != 0) {
        if (getPatInstance() != NULL) {
            char* text = getStr1(getPatInstance());
            length = (strlen(text) < size) ? strlen(text) : size - 1;
            memcpy(out, text, length);
        }
        out[length] = 0;
    }
}
void getReflectName5C(NetworkWiiMediator* self, char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 32) ? size - 1 : 31;
        memcpy(out, (char*)&self->reflect_name_5C, length);
        out[length] = 0;
    }
}
void setReflectPageRange(NetworkWiiMediator* self, u32 address, u32 size)
{
    if (getPatInstance() != NULL) {
        setPatReflectPageRange(getPatInstance(), address, size);
    }
    self->field_28 = address;
    self->field_2C = size;
}
void setReflectField30(NetworkWiiMediator* self, u32 value)
{
    if (getPatInstance() != NULL) {
        setPatReflectField30(getPatInstance(), value);
    }
    self->field_30 = value;
}
void setReflectField34(NetworkWiiMediator* self, u32 value)
{
    if (getPatInstance() != NULL) {
        setPatReflectField34(getPatInstance(), value);
    }
    self->field_34 = value;
}
void setReflectField38(NetworkWiiMediator* self, u32 value)
{
    if (getPatInstance() != NULL) {
        setPatReflectField38(getPatInstance(), value);
    }
    self->field_38 = value;
}
void setReflectName3C(NetworkWiiMediator* self, char* name)
{
    if (getPatInstance() != NULL) {
        setPatReflectName3C(getPatInstance(), name);
    }
    u32 length = (strlen(name) < 31) ? strlen(name) : 31;
    memcpy((char*)&self->reflect_name_3C, name, length);
    ((char*)&self->reflect_name_3C)[length] = 0;
}
void setReflectName5C(NetworkWiiMediator* self, char* name)
{
    if (getPatInstance() != NULL) {
        setPatReflectName5C(getPatInstance(), name);
    }
    u32 length = (strlen(name) < 31) ? strlen(name) : 31;
    memcpy((char*)&self->reflect_name_5C, name, length);
    ((char*)&self->reflect_name_5C)[length] = 0;
}
void updatePatField854(NetworkWiiMediator* self, u32 value)
{
    (void)self;
    if (getPatInstance() != NULL) {
        setPatField854(getPatInstance(), value);
    }
}
void updatePatField860(NetworkWiiMediator* self, u32 value)
{
    (void)self;
    if (getPatInstance() != NULL) {
        setPatField860(getPatInstance(), (const char*)value);
    }
}
s32 isShiftJisLeadByte(NetworkWiiMediator* self, u8 value)
{
    (void)self;
    if ((value >= 129 && value <= 159) || (value >= 224 && value <= 239)) {
        return 1;
    }
    return 0;
}
#pragma peephole off
s32 isNameSymbolChar(NetworkWiiMediator* self, char value)
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
void parseReflectLines(NetworkWiiMediator* self, s32 source)
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
s32 parseReflectPacket(NetworkWiiMediator* self, char* out, const char* in, u32 unused4,
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
s32 validateReflectName(NetworkWiiMediator* self, const char* name)
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
s32 buildReflectPacket(NetworkWiiMediator* self, const char* text, s32* skipCount, s32* flags)
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

/* ----------------------------------------------------------------------------------------- */
/* The opening part (0x804155D4..0x80417BC0), folded in from `Network/network_opening.cpp`.   */
/* ----------------------------------------------------------------------------------------- */


#pragma peephole off
#pragma auto_inline off
#pragma pool_data off

/* Finalizes the network pool singleton, then deletes it. */
void NetworkWiiMediator::deleteNetworkPool()
{
    if (getNetworkPool() != NULL) {
        getNetworkPool()->destroySession();
        delete getNetworkPool();
    }
}

/* Starts the EC sequence on the pool singleton, refused while the opening runs. */
void NetworkWiiMediator::ECStart()
{
    if (flag_1C != 0) {
        ((sNetworkLibrary*)getNetworkLogger())
            ->logError("NetworkWiiMediator::ECStart() must not be called, during openingStart()\n");
        return;
    }
    if (getNetworkPool() != NULL) {
        getNetworkPool()->start();
    }
}

/* Copies the reflect name (at most 31 characters) into `out` and returns the mediator's own copy. */
char* NetworkWiiMediator::getReflectName(char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 32) ? size - 1 : 31;

        memcpy(out, reflect_name_3C, length);
        out[length] = 0;
    }
    return (char*)reflect_name_3C;
}

char* getNASToken(NetworkWiiMediator* self)
{
    return self->svl_result.svltoken;
}

/* Keeps the nine DWC game-info words; the two names and the title are copied into the mediator's own
 * buffers and the words point at those copies. */
void setGameInfo2d1c(NetworkWiiMediator* self, u32* info)
{
    const char* text;
    const u16* title;
    u32 length;

    self->game_info[0] = info[0];
    self->game_info[1] = info[1];
    self->game_info[2] = info[2];
    self->game_info_name[0] = 0;
    text = (const char*)info[3];
    if (text != NULL) {
        length = (strlen(text) < 17) ? strlen(text) : 16;
        memcpy(self->game_info_name, text, length);
        self->game_info_name[length] = 0;
    }
    self->game_info[3] = (u32)self->game_info_name;
    self->game_info[4] = info[4];
    self->game_info[5] = info[5];
    self->game_info_secret[0] = 0;
    text = (const char*)info[6];
    if (text != NULL) {
        length = (strlen(text) < 17) ? strlen(text) : 16;
        memcpy(self->game_info_secret, text, length);
        self->game_info_secret[length] = 0;
    }
    self->game_info[6] = (u32)self->game_info_secret;
    self->game_info_title[0] = 0;
    title = (const u16*)info[7];
    if (title != NULL) {
        u32 count = 0;

        while (title[count] != 0) {
            count++;
        }
        count = (count < 26) ? count : 25;
        memcpy(self->game_info_title, title, count * 2);
        self->game_info_title[count] = 0;
    }
    self->game_info[7] = (u32)self->game_info_title;
    self->game_info[8] = info[8];
}

void getGameInfo2d1c(NetworkWiiMediator* self, u32* out)
{
    out[0] = self->game_info[0];
    out[1] = self->game_info[1];
    out[2] = self->game_info[2];
    out[3] = self->game_info[3];
    out[4] = self->game_info[4];
    out[5] = self->game_info[5];
    out[6] = self->game_info[6];
    out[7] = self->game_info[7];
    out[8] = self->game_info[8];
}

/* Hands the terms object its buffer, empties every transfer slot and sets mode 1, both flags clear and the
 * default level. */
/* untyped: byte range - the MEM2 buffer handed to the terms object */
void initMediatorTerms(NetworkWiiMediator* self, void* buffer, u32 size)
{
    s32 i;

    initPatTerms(getPatTerms(), buffer, size);
    self->closeTransferSlots();
    for (i = 0; i < 4; i++) {
        self->clearTransferQueue(i);
    }
    self->transfer_mode = 1;
    self->transfer_flag_6DD1 = 0;
    self->transfer_flag_6DD2 = 0;
    self->transfer_level = 1.0f;
}

/* Starts the terms check, when there is a terms object. */
void startTermsCheck(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        requestPatTermsCheck(getPatTerms());
    }
}

/* The terms object's ready state, 0 when there is none. */
s32 getMediatorTermsStatus(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        return isPatTermsReady(getPatTerms());
    }
    return 0;
}

/* Whether the terms object's ready byte is set. */
u32 isPatTermsReady(PatTerms* terms)
{
    return terms->ready_0x0D != 0;
}

/* Starts the terms update, when there is a terms object. */
void startTermsUpdate(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        requestPatTermsUpdate(getPatTerms());
    }
}

/* Cancels the terms update, when there is a terms object. */
void cancelTermsUpdate(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        cancelPatTermsUpdate(getPatTerms());
    }
}

/* Stores the transfer mode, dropping every slot's queue when it changes. */
#pragma peephole on
void setMediatorTransferMode(NetworkWiiMediator* self, u32 mode)
{
    s32 i;

    if (self->transfer_mode != mode) {
        for (i = 0; i < 4; i++) {
            self->clearTransferQueue(i);
        }
    }
    self->transfer_mode = mode;
}
#pragma peephole off

/* Whether the terms update finished: 0 while the transfer mode is 0 or there is no terms object. */
s32 isMediatorTermsUpdateFinished(NetworkWiiMediator* self)
{
    if (self->transfer_mode != 0 && getPatTerms() != NULL) {
        return isTermsUpdateFinished(getPatTerms());
    }
    return 0;
}

/* Whether the terms object reached its update-finished state (11). */
u32 isTermsUpdateFinished(PatTerms* terms)
{
    return terms->state_0x0C == 11;
}

/* Forwards to the graded terms progress. */
s32 getTermsProgressLevel(NetworkWiiMediator* self)
{
    return getMediatorTermsProgressLevel(self);
}

/* Grades the terms progress count against the threshold table: the index of the first threshold it does not
 * exceed (16 past the sixteenth), 0 when there is no terms object. */
s32 getMediatorTermsProgressLevel(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        u16 progress = getPatTermsProgress(getPatTerms());
        u16 thresholds[17] = {
            83, 117, 165, 234, 330, 467, 659, 931, 1316, 1859, 2626, 3709, 5239, 7401, 10455, 14768, 20860
        };
        s32 level;

        for (level = 0; level < 16; level++) {
            if (progress <= thresholds[level]) {
                break;
            }
        }
        return level;
    }
    return 0;
}

/* The terms progress count, 0 when there is no terms object. */
u16 getMediatorTermsProgress(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        return getPatTermsProgress(getPatTerms());
    }
    return 0;
}

/* Stores the terms object's +0xE2 byte, when there is a terms object. */
void setMediatorTermsFlag(NetworkWiiMediator* self, u32 flag)
{
    if (getPatTerms() != NULL) {
        setPatTermsFlag(getPatTerms(), flag);
    }
}

#pragma peephole on
void setPatTermsFlag(PatTerms* terms, u32 flag)
{
    terms->flag_0xE2 = flag;
}
#pragma peephole off

/* The terms object's +0xE2 byte, 0 when there is no terms object. */
u8 getMediatorTermsFlag(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        return getPatTermsFlag(getPatTerms());
    }
    return 0;
}

u8 getPatTermsFlag(PatTerms* terms)
{
    return terms->flag_0xE2;
}

/* Activates slot `slot` with its mode and level (while the terms object is up) and empties its queue. */
void NetworkWiiMediator::openTransferSlot(s8 slot, u8 mode, f32 level)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (getMediatorTermsStatus(this) == 0) {
        return;
    }
    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active == 0) {
        entry->active = 1;
        entry->mode = mode;
        entry->flag = 0;
        entry->level = level;
        clearTransferQueue(slot);
    }
}

void NetworkWiiMediator::closeTransferSlot(s8 slot)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        entry->active = 0;
    }
}

void NetworkWiiMediator::closeTransferSlots()
{
    s32 i;

    for (i = 0; i < 4; i++) {
        closeTransferSlot(i);
    }
}

void NetworkWiiMediator::clearTransferQueue(s8 slot)
{
    memset(&transfer_queue[slot << 12], 0, 0x1000);
    transfer_queued[slot] = 0;
    transfer_activity[slot] = 0;
    transfer_timer[slot] = 0;
}

/* Appends one record (a u16 length, then the bytes) to the slot's queue, restarting a full queue;
 * returns the bytes taken, or 0 for a bad slot or an empty record. */
s32 NetworkWiiMediator::pushTransferRecord(s8 slot, const u8* data, s32 size)
{
    u16 length;
    u8* queue;
    s32* queued;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    if (size <= 0) {
        return 0;
    }
    queued = &transfer_queued[slot];
    if ((u32)(size + *queued) > 0x1000) {
        *queued = 0;
    }
    length = size;
    queue = &transfer_queue[slot << 12];
    memcpy(queue + *queued, &length, sizeof(length));
    memcpy(*queued + queue + 2, data, size);
    *queued = size + *queued + 2;
    return size + 2;
}

/* Takes the oldest record of the slot's queue into `out` (when it fits in `max` bytes) and returns its
 * length; also ages the slot's activity, re-judging it every 15 polls. */
s32 NetworkWiiMediator::popTransferRecord(s8 slot, u8* out, s32 max)
{
    u16 length;
    s32* queued;
    s32 offset;
    s32 taken;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    if (transfer_timer[slot] <= 0) {
        if ((transfer_activity[slot] & 0xF0) >= 0xB0) {
            transfer_activity[slot] = 1;
        } else {
            transfer_activity[slot] = 0;
        }
        transfer_timer[slot] = 15;
    }
    transfer_timer[slot]--;
    queued = &transfer_queued[slot];
    if (*queued > 2) {
        transfer_activity[slot] += 0x10;
    }
    if (*queued <= 2) {
        *queued = 0;
        return 0;
    }
    offset = slot << 12;
    memcpy(&length, &transfer_queue[offset], sizeof(length));
    if (max < length) {
        *queued = 0;
        return 0;
    }
    memcpy(out, &transfer_queue[offset] + 2, length);
    taken = length + 2;
    *queued -= taken;
    if (*queued > 0) {
        memmove(&transfer_queue[offset], &transfer_queue[offset] + taken, *queued);
    }
    return length;
}

void setMediatorTransferFlag6DD1(NetworkWiiMediator* self, u8 flag)
{
    self->transfer_flag_6DD1 = flag;
}

u8 NetworkWiiMediator::getTransferFlag6DD1()
{
    return transfer_flag_6DD1;
}

void setMediatorTransferFlag6DD2(NetworkWiiMediator* self, u8 flag)
{
    self->transfer_flag_6DD2 = flag;
}

u8 NetworkWiiMediator::getTransferFlag6DD2()
{
    return transfer_flag_6DD2;
}

void setMediatorTransferLevel(NetworkWiiMediator* self, f32 level)
{
    self->transfer_level = level;
}

f32 NetworkWiiMediator::getTransferLevel()
{
    return transfer_level;
}

/* Sets an active slot's mode; mode 0 drops its queue. */
void NetworkWiiMediator::setTransferSlotMode(s8 slot, u8 mode)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        entry->mode = mode;
        if (entry->mode == 0) {
            clearTransferQueue(slot);
        }
    }
}

BOOL NetworkWiiMediator::getTransferSlotMode(s8 slot)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        return entry->mode;
    }
    return 0;
}

void NetworkWiiMediator::setTransferSlotFlag(s8 slot, u8 flag)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        entry->flag = flag;
    }
}

u8 NetworkWiiMediator::getTransferSlotFlag(s8 slot)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        return entry->flag;
    }
    return 0;
}

/* Whether an active slot's ready flag (the activity's low nibble) is set. */
BOOL NetworkWiiMediator::isTransferSlotReady(s8 slot)
{
    if (slot < 0 || 4 <= slot) {
        return FALSE;
    }
    if (transfer_slots[slot].active == 0) {
        return FALSE;
    }
    return (transfer_activity[slot] & 0xF) != 0;
}

/* One step of the opening: start the library, wait for the NAS service-locator token, open the Pat
 * interface and run its login, then parse the reflect texts; every failure is reported to the Pat
 * interface's session handlers and stops the sequence. */
void NetworkWiiMediator::openingStart()
{
    sNetworkLibraryError error;
    s32 result;

    if (flag_1C != 0) {
        if (getInstance_() == NULL || GameSpyInterfaceThread::getInstance() == NULL) {
            ((sNetworkLibrary*)getNetworkLogger())
                ->logError("NetworkWiiMediator::openingStart() maybe be called before openingInit()\n");
            flag_1C = 0;
        }
        if (getNetworkPool() != NULL && getNetworkPool()->isECStarted()) {
            ((sNetworkLibrary*)getNetworkLogger())
                ->logError("NetworkWiiMediator::openingStart() must be called before ECStart()\n");
            flag_1C = 0;
            error.facility = 0x80000007;
            error.step = 0;
            error.code = 0;
            reportPatError(getInstance_(), 1, error);
        }
    }
    switch (flag_1C) {
    case 1:
        if (library_started != 0) {
            error.facility = 0x80000007;
            error.step = 106;
            error.code = 0;
            reportPatError(getInstance_(), 1, error);
            break;
        }
        result = ((sNetworkLibrary*)getNetworkLogger())->start(0, &error);
        if (result < 0) {
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (result > 0) {
            library_started = 1;
            memset(&svl_result, 0, sizeof(svl_result));
            if (field_24 != 0) {
                snprintf(svl_result.svltoken, sizeof(svl_result.svltoken), "DEBUG AUTHENTICATION TOKEN");
                flag_1C += 2;
            } else {
                GameSpyInterfaceThread::getInstance()->setWaitHandle(&svl_result);
                flag_1C++;
            }
        }
        break;
    case 2:
        result = GameSpyInterfaceThread::getInstance()->runNasLogin();
        if (result < 0) {
            GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (result > 0) {
            flag_1C++;
        }
        break;
    case 3:
        openPatInterface(getInstance_());
        setCallback(getInstance_(), (void (*)())mediatorEventCallback, this, 1);
        setConnectServerType(getInstance_(), 2);
        event_flags = 0;
        resetNetworkState(getInstance_());
        flag_1C++;
        break;
    case 4:
        if (event_flags & 1) {
            getErrorInfo654c(getInstance_(), (u32*)&error);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (event_flags & 2) {
            error.facility = 0x80000007;
            error.step = 0;
            error.code = errorRecordCode613c(getInstance_(), NULL);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if ((u8)getNetworkBinaryState(getInstance_()) != 0) {
            flag_1C++;
        }
        break;
    case 5:
        if (event_flags & 1) {
            getErrorInfo654c(getInstance_(), (u32*)&error);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (event_flags & 2) {
            error.facility = 0x80000007;
            error.step = 0;
            error.code = errorRecordCode613c(getInstance_(), NULL);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (handleNetworkState1(getInstance_()) != 0 || (event_flags & 0x10)) {
            flag_1C++;
        } else if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            flag_1C--;
        }
        break;
    case 6:
        parseReflectLines(this, 0);
        parseReflectLines(this, 1);
        parseReflectLines(this, 2);
        notifyPatEvent(getInstance_(), 1, 0, NULL);
        flag_1C = 0;
        break;
    }
}

BOOL NetworkPool::isECStarted()
{
    return ec_started;
}

/* One step of the closing: shut the Pat session down, release the Pat interface, then stop the library. */
void NetworkWiiMediator::openingStop()
{
    s32 result;

    if (flag_1D != 0 && getInstance_() == NULL) {
        ((sNetworkLibrary*)getNetworkLogger())
            ->logError("Maybe, NetworkWiiMediator::openingStop() is called after openingFinal()\n");
        flag_1D = 0;
    }
    switch (flag_1D) {
    case 1:
        if (isCallback(getInstance_(), 1) == 0) {
            flag_1D = 5;
            break;
        }
        if (hasMultipleRefs60d4(getInstance_()) != 0) {
            flag_1D = 4;
            break;
        }
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            flag_1D = 4;
            break;
        }
        event_flags = 0;
        sendReqShut(getInstance_(), 1);
        flag_1D++;
        break;
    case 2:
        if ((event_flags & 1) || (event_flags & 2) || (event_flags & 0x10)) {
            event_flags = 0;
            resetNetworkState3(getInstance_());
            flag_1D++;
        }
        break;
    case 3:
        if (event_flags & 8) {
            flag_1D++;
        }
        break;
    case 4:
        resetCallback(getInstance_(), 1);
        decrement60d4(getInstance_());
        flag_1D++;
        break;
    case 5:
        result = 1;
        if (library_started != 0) {
            result = ((sNetworkLibrary*)getNetworkLogger())->stop();
        }
        if (result > 0) {
            notifyPatEvent(getInstance_(), 2, 0, NULL);
            library_started = 0;
            flag_1D = 0;
        }
        break;
    }
}

/* Folds a Pat interface event into the event flags the opening and closing steps poll. */
void NetworkWiiMediator::applyEvent(s32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg)
{
    switch (code) {
    case 0x8000:
    case 0x8007:
        event_flags |= 1;
        break;
    case 0x8002:
        event_flags |= 2;
        break;
    case 0x8004:
        if (b != 0) {
            event_flags |= 1;
        }
        break;
    case 0x8005:
        event_flags |= 8;
        break;
    case 0x8006:
        event_flags |= 0x10;
        break;
    }
}
