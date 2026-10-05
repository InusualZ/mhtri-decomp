/*
 * Network/NetworkWiiMediator.cpp - `NetworkWiiMediator`: the head (`mediatorEventCallback`, the constructors and
 *   destructors, `getReflectService`, `update`, the SC/SO queries), the class band (the account/opening/reflect query
 *   surface and the reflect sub-machine) and the opening part (the game-info copy pair, the terms entry points, the
 *   transfer-slot family and the opening/EC steps).  C++; every unmangled helper is `extern "C"`.
 * RANGE. .text 0x80413450-0x80417BC0 (165 functions); .rodata 0x80570E70-0x80570E98 (read by 0x80416A30), .data
 *   0x806024B8-0x80602988, .sdata2 0x8079C870-0x8079C878 (the constructor's 0.0f, `initMediatorTerms`'s 1.0f), extab,
 *   extabindex.  The `.data` (three jump tables, the opening strings, the two mediator tables) is one D/S/V run with no
 *   seam until V->D at 0x80602988.  The left cut is 0x80413450, not 0x80413384: `NetworkRandom`'s table 0x806024A0
 *   precedes the first jump table 0x806024B8, so `NetworkRandom` is `Network/NetworkPool.cpp`'s.
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md: the `-O4,p` epilogue-swap /
 *   hoisted-`li` signature of playbook 27).  The constructors and destructors sit under `#pragma peephole off` (retail
 *   stages the table address in r0, keeps `extsh`+`cmpwi` on the destructor flag); the opening part keeps its file-scope
 *   `#pragma peephole off` (retail's `extsb`+`cmpwi` on the slot argument), back on around `setMediatorTransferMode` and
 *   `setPatTermsFlag` (the raw `stb` of a u32), `#pragma auto_inline off` (retail calls `clearTransferQueue`) and
 *   `#pragma pool_data off` (each log string its own `lis`/`addi`), from where it starts.
 * NAMES. The `NetworkWiiMediator::*` spellings are the runtime dump's, `ECStart`/`openingStart`/`openingStop` the
 *   strings'. GUESSes: `NetworkMediator` (the `sNetworkLibrary`/`sNetworkLibraryWii` pairing), `update`,
 *   `deleteNetworkPool`, `enableMediatorLinkError`/`disableMediatorLinkError` (the `flag_21` setters); the helpers
 *   after the field they touch (`getReflectField30`, `getReflectName3C`, `getReflectPageRange`;
 *   `getReflectField30/34/38` and `getAccountQuery1..5` are positional; the `pat_*`/`line_table_*`/`buffer_*` fields
 *   from the functions that use them), the singleton forwarders after the PatInterface helper they call
 *   (`updatePatField854`, `queryOpeningFlag208`), the packet helpers after what they do (`parseReflectPacket`,
 *   `buildReflectPacket`, `validateReflectName`, `getReflectModeFromLanguage`, `isShiftJisLeadByte`); the opening
 *   members (the ones other units call keep a C name: `setGameInfo2d1c`, `initMediatorTerms`, `setMediatorTransfer*`)
 *   and the callees named in their owners' headers (`reportPatError`, `notifyPatEvent`, `openPatInterface`,
 *   `NetworkPool::start`), `NetworkPool::isECStarted`; the terms wrappers 0x80416800..0x80416C18 (`cancelTermsUpdate`,
 *   `getTermsProgressLevel`, `getMediatorTermsProgress*`, `set/getMediatorTermsFlag`, `isPatTermsReady`,
 *   `set/getPatTermsFlag`) and the `menu/menu_plsearch.cpp` callees (`initPatTerms`, `requestPatTermsCheck`,
 *   `requestPatTermsUpdate`, `cancelPatTermsUpdate`, `getPatTermsProgress`).  The six PatInterface sub-state tests keep
 *   their semantic names (`isSubState_8254_3`, `isSubState_894F_2..6`).
 * RESIDUALS. 34 rows unwritten (objdiff scores them zero): the head's word copies, `isMessageRestricted` and
 *   `checkMediatorLink` (0x80413980..0x80413BE0), `getCountryCode`/`getLanguage` (0x80413BF8..0x80413C64); the word
 *   filter 0x804155D4 (2520 B), `postMediatorRecord` and the `NetworkPool` forwarding wrappers (0x804155D4..0x80416120,
 *   0x804161D0..0x804165A8); `readVoice` (0x80416DB0); the sound and voice helpers 0x80417080..0x804172CC.
 *   `flipcheck`: 22 functions retail's `.comment` marks force-active are not marked in ours (row 36: unreferenced, the
 *   linker would deadstrip them) - `disableMediatorLinkError`, `getMediatorField24`, `updateServerTime`,
 *   `queryOpeningFlag250`, `queryOpeningFlag290`, `getAccountQuery1..5`, `isShiftJisLeadByte`, `ECStart` and others;
 *   `.rodata` 0x22 against 0x28.  Partial rows:
 *  - `isShiftJisLeadByte`: retail evaluates the two byte ranges as four unsigned compares; MWCC folds each range into
 *    one `addi`/`clrlwi`/`cmplwi` and the `>= 224` test into branchless code for every spelling tried;
 *  - `setMediatorBufferA`/`B`: retail schedules `addi r3,r3,0x7C` before the `memcpy`'s `li r5,0x106` (the store
 *    direction only: the load direction, `getMediatorBuffer*`, is byte-identical; a typed array, `&self->buffer[0]` and
 *    a byte offset tried);
 *  - `parseReflectPacket`: the lead-byte guard lands in `cr1` where retail reuses `cr0`, and retail re-reads `in[1]`
 *    (`lbz`+`extsb`) before the `out[1] = in[1] - 32` store; the switch and if-chain spellings emit the same object;
 *  - `parseReflectLines`: retail keeps `mr r0,r28` before the `*table = text` store (a temporary and a scoped
 *    `#pragma peephole off` tried);
 *  - `buildReflectPacket`: the same instructions and size (344 B); a relocation artefact of the split;
 *  - `updateServerTime`: the 64-bit tail `return 0` emits `li r3,0` where retail emits `li r4,0` + `mr r3,r4`;
 *  - `getReflectPage`: `mr r4,r31` where retail masks the `u8` page (`clrlwi r4,r31,24`); a `(u32)` cast does not help;
 *  - `isNameSymbolChar`: the `u8` local's mask lands in r3, retail r0 (a `u8` parameter costs `validateReflectName`);
 *  - `popTransferRecord`: retail computes `&length` before the queue address for the first `memcpy`;
 *    `pushTransferRecord`: r29-r31 permuted; `setGameInfo2d1c`: the title length in r29 where retail reuses r28;
 *  - `.data`: MWCC names the jump tables anonymously while the map calls them `jumptable_806024B8` (177 entries),
 *    `jumptable_8060277C` (10), `jumptable_806027A4` (32), so objdiff reports their relocations unpaired.
 * SHAPES. `NetworkMediator` (table 0x80602978: destructor 0x804136D0, a pure `update`) and `NetworkWiiMediator :
 *   NetworkMediator` (table 0x80602968: destructor 0x80413724, `update` 0x804138EC) are emitted here (rule 10), with
 *   `PatTerms` (`menu/PatTerms.h`) as the member at +0x6DD8.
 *  - `initializeNetworkMediator` and `reflectInit` allocate with real `new PatInterface();` / `new
 *    NetworkReflectService();` / `new GameSpyInterfaceThread();` (playbook 62): each class has a declared constructor
 *    and a padding member making `sizeof` the allocation size (0xD640 / 0x816C / 0x44A0);
 *  - the slot range test is `slot < 0 || 4 <= slot` with an early return; the transfer queue is one flat array indexed
 *    by `slot << 12`; `applyEvent` takes the code as `s32` (the `addis` before the unsigned case compares);
 *  - each terms leaf accessor sits right after its first caller, the layout MWCC gives an uninlined inline function;
 *  - `getNASToken` returns the token inside `DWCSvlResult` (`DWCi/dwc_nasfunc.h`);
 *  - `NetworkPool::isECStarted` (0x8041793C) is defined here: retail emits the pool's accessor in this range.
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

/* The network singleton as the `PatInterface` it is: its owner's header (`enemy/em020_ai.h`) spells it
 * `NetworkInstance`, so the two views meet here instead of in two declarations. */
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
    switch (getCountryCode()) {
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
/* Decodes one reflect reply packet at `in` into `out` (the 0xC3 lead-byte escapes, the upper-case fold, the symbol
 * map); 1 for a decoded byte, 2 for a two-byte escape. */
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
/* Scans one reflected name: the characters a `*`-escape covers go to `skipCount` and the matched escape to `flags`
 * (which of the four name forms it is); returns the characters left over. */
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

/* One step of the opening: start the library, wait for the NAS token, open the Pat interface and log in, then parse
 * the reflect texts; a failure is reported to the session handlers and stops the sequence. */
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
