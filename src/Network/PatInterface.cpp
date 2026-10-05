/*
 * Network/PatInterface.cpp - the `PatInterface` singleton's own range: its reference count, error records, session
 * handler slots, call stack, clock and the accessors the session units use.
 *
 * `.text` 0x803FCC34..0x803FE8E4.  Sections of the candidate unit: extab 0x8001BCAC..0x8001BD9C; extabindex 0x8003BF40..0x8003C06C; .text 0x803FCC34..0x803FE8E4; .data 0x80600978..0x80600BA0; .sbss 0x80794CB0..0x80794CB8; .sdata2 0x8079C7D0..0x8079C7D8.
 *
 * WHAT IT IS. the `PatInterface` singleton: `__ct__12PatInterfaceFv`, `PatInterface_clear`, `PatInterface_isReady`, the reflect page range,
 *   the callback and the call-stack helpers, the server-type choice (`setConnectServerType`, `chooseServerAddress`), the
 *   game/server time getters and the Fmp accessors (87 functions, 66 named; `mpInstance__12PatInterface` is its `.sbss` word).
 *
 * WHY IT SITS HERE. phase 1 grade medium: the `.ctors` closure is empty and a `.data` vtable-then-string seam ends exactly at 0x803FCC34, where
 *   `__ct__12PatInterfaceFv` starts; the right edge is the registered `Network/network_state.cpp`.
 *
 * WRITTEN.  The accessors (C linkage: the map names them unmangled) and the five buffer setters (members).  Not
 *   written: the constructor 0x803FCC34 (it needs the base class `PatConnection`, ctor 0x803FAE9C, table
 *   0x806006E8), the destructor 0x803FCF8C and `resetDefaults` (slots +0x08/+0x0C), `stepPatInterface`,
 *   `postError` (+0x288), `buildErrorInfo613c`, `getErrorInfoOrCode654c`, `pushStack`, `setConnectServerType`,
 *   `chooseServerAddress`, `getFmpSelection`, the FMP/server helpers 0x803FDE48..0x803FE154, `updatePatInterface`
 *   (a tail call into `PatConnection`'s unnamed 0x803FB5E0), `getPatServerTime`, `getPatAccountName`,
 *   `getSomething4` (its declared `s8` return would add an `extsb` retail does not have), 0x803FE410 and the
 *   stack/notice helpers 0x803FE4D8..0x803FE5A0.
 *
 * TU.  The scouting report reads this unit + `Network/network_state.cpp` + `Network/network_layer_io.cpp`'s head as
 *   ONE ~88 KB TU (the table 0x80602198 lies in network_layer_io's `.data`); recorded, the units are not merged.
 *
 * NAMES (GUESS, integrator 2026-10-04, from the bodies, in the scheme of `errorRecordCode613c`/`getErrorInfo654c`):
 *   `clearErrorRecord613c` 0x803FD674 clears the 0x208-byte record at +0x613C while no error is pending (+0x654C);
 *   `buildErrorInfo613c` 0x803FD6D8 fills a caller's three-word error from a code and the record's code;
 *   `getErrorInfoOrCode654c` 0x803FD7BC copies the pending error and substitutes a negative code for 0x80000000.
 *
 * FLAGS. `-O3 -inline noauto` (configure.py, with the evidence) and a file-scope `#pragma peephole off` (memset/memcpy
 *   argument order, the unfused `extsb`+`cmpwi` of isOpeningAnnounce: 60 rows at 100 against 56 with the pass on),
 *   switched back on around `setSomething` (its s32 store keeps no `clrlwi` only with the pass on).  That the
 *   peephole setting differs from network_layer_io's file-wide `off` is one argument against the one-TU reading.
 *
 * RESIDUALS.  reportPatError 76.71 (retail materialises `&pendingError_654C` after the code load, ours before).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/PatInterface.cpp`), and the pass that writes the bodies defines them.
 */

#include "Network/PatInterface.h"

#include "Network/network_layer_io.h"   /* dispatchSessionHandlers */
#include "unsplit/Network.h"            /* getNetworkLogger */
#include "unsplit/Runtime.PPCEABI.H.h"  /* strlen */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off

/* Takes a reference on the singleton; the first one clears the ticket's size and restores the defaults. */
void openPatInterface(NetworkInstance* self)
{
    if (((PatInterface*)self)->refCount_60D4++ == 0) {
        if (((PatInterface*)self)->ticket_8BB0 != NULL) {
            ((PatInterface*)self)->ticket_8BB0->size_400 = 0;
        }
        ((PatInterface*)self)->resetDefaults();
    }
}

/* Takes a reference on the singleton; the first one restores the defaults. */
void increment60d4(NetworkInstance* self)
{
    if (((PatInterface*)self)->refCount_60D4++ == 0) {
        ((PatInterface*)self)->resetDefaults();
    }
}

/* Drops a reference on the singleton. */
void decrement60d4(NetworkInstance* self)
{
    if (((PatInterface*)self)->refCount_60D4 > 0) {
        ((PatInterface*)self)->refCount_60D4--;
    }
}

/* Unbinds the first and third session handlers unless the singleton is still referenced. */
/* free: retail C linkage - the map names it unmangled (`PatInterface_clear`) */
void PatInterface_clear(PatInterface* self)
{
    if (PatInterface_isReady(self) == 0) {
        resetCallback((NetworkInstance*)self, 0);
        resetCallback((NetworkInstance*)self, 2);
    }
}

/* Whether the singleton is referenced. */
/* free: retail C linkage - the map names it unmangled (`PatInterface_isReady`) */
s32 PatInterface_isReady(PatInterface* self)
{
    return self->refCount_60D4 > 0;
}

/* Whether the singleton holds more than one reference. */
s32 hasMultipleRefs60d4(NetworkInstance* self)
{
    return ((PatInterface*)self)->refCount_60D4 > 1;
}

/* Clears the error record unless an error is pending. */
void clearErrorRecord613c(NetworkInstance* self)
{
    if (((PatInterface*)self)->pendingError_654C.code_00 == 0) {
        memset(&((PatInterface*)self)->errorRecord_613C, 0, sizeof(PatErrorRecord));
    }
}

/* Copies the error record out when `record` is non-null and returns its code. */
/* untyped: caller-owned payload - the 0x208-byte record (request net3-d-03c2#14 retypes it) */
s32 errorRecordCode613c(NetworkInstance* self, const void* record)
{
    if (record != NULL) {
        memcpy((void*)record, &((PatInterface*)self)->errorRecord_613C, sizeof(PatErrorRecord));
    }
    return ((PatInterface*)self)->errorRecord_613C.code_000;
}

/* Copies the pending error out when `info` is non-null and returns its code (0 while none is kept). */
s32 getErrorInfo654c(NetworkInstance* self, u32* info)
{
    if (info != NULL) {
        info[0] = ((PatInterface*)self)->pendingError_654C.code_00;
        info[1] = ((PatInterface*)self)->pendingError_654C.param1_04;
        info[2] = ((PatInterface*)self)->pendingError_654C.param2_08;
    }
    return ((PatInterface*)self)->pendingError_654C.code_00;
}

/* Keeps `error` as the pending error unless one is kept, then reports the pending one with event `kind`. */
void reportPatError(NetworkInstance* self, u32 kind, sNetworkLibraryError error)
{
    if (((PatInterface*)self)->pendingError_654C.code_00 == 0) {
        ((PatInterface*)self)->pendingError_654C.code_00 = error.facility;
        ((PatInterface*)self)->pendingError_654C.param1_04 = error.step;
        ((PatInterface*)self)->pendingError_654C.param2_08 = error.code;
    }
    dispatchSessionHandlers((PatInterface*)self, kind, NULL, ((PatInterface*)self)->pendingError_654C.code_00, 1,
                            (const u8*)&((PatInterface*)self)->pendingError_654C);
}

/* Reports event `code` with its payload to the session handlers. */
void notifyPatEvent(NetworkInstance* self, u32 code, u32 count, const u8* data)
{
    dispatchSessionHandlers((PatInterface*)self, code, NULL, 0, count, data);
}

/* Whether session handler `index` is bound (0 with a warning for a bad index). */
s32 isCallback(NetworkInstance* self, s32 index)
{
    if ((u32)index > 7) {
        getNetworkLogger()->warn_10("PatInterface:isCallback(): mpEventCbFunc[%d] is bound over.\n", index);
        return 0;
    }
    return ((PatInterface*)self)->eventCallbacks_60D8[index] != NULL;
}

/* Binds session handler `index` (0..7) with its argument: 1, or -1 with a warning for a bad index. */
/* untyped: caller-owned payload - handed back to the callback */
s32 setCallback(NetworkInstance* self, void (*callback)(), void* arg, u32 index)
{
    if (index > 7) {
        getNetworkLogger()->warn_10("PatInterface:setCallback(): mpEventCbFunc[%d] is bound over.\n", index);
        return -1;
    }
    ((PatInterface*)self)->eventCallbacks_60D8[index] = (PatEventCallback)callback;
    ((PatInterface*)self)->eventCallbackArgs_60F8[index] = (u32)arg;
    return 1;
}

/* Unbinds session handler `index`. */
void resetCallback(NetworkInstance* self, s32 index)
{
    setCallback(self, NULL, NULL, index);
}

/* Takes `size` bytes off the call stack with no extra alignment. */
u8* createStack(NetworkStateMachine* self, u32 size, u32* outSize)
{
    u32 align = 0;
    return pushStack(self, size, outSize, &align);
}

/* Gives `size` bytes back to the call stack (at most what is in use). */
void growStackSize(NetworkStateMachine* self, u32 size)
{
    if (self->stackRemaining_8BD4 < size) {
        size = self->stackRemaining_8BD4;
    }
    self->stackRemaining_8BD4 -= size;
    self->stackUsed_8BD8 += size;
}

/* 1 when the busy flag is already set, else sets it and returns 0. */
s32 testAndSet611b(NetworkInstance* self)
{
    if (((PatInterface*)self)->busy_611B != 0) {
        return 1;
    }
    ((PatInterface*)self)->busy_611B = 1;
    return 0;
}

/* Clears the busy flag. */
void set611b(NetworkInstance* self)
{
    ((PatInterface*)self)->busy_611B = 0;
}

/* The game time: the server's game time at the last sync plus the clock's progress since. */
u32 getGameTime(NetworkInstance* self)
{
    return (u32)(((PatInterface*)self)->clock_611C - ((PatInterface*)self)->clockAtSync_6120) +
           ((PatInterface*)self)->gameTimeBase_6128;
}

/* The server's date-time at the last sync. */
u32 getServerTime(PatInterface* self)
{
    return self->serverTime_612C;
}

/* The server's date-time now: the last sync's plus the game time. */
s32 getServerDateTime(NetworkInstance* self)
{
    return getServerTime((PatInterface*)self) + getGameTime(self);
}

/* The FMP slot the query settled on. */
u32 getFmpSelected(NetworkStateMachine* self)
{
    return self->fmpSelected_8040;
}

/* The number of live FMP slots. */
s32 getFmpSize(NetworkStateMachine* self)
{
    return self->fmpSlotCount_6608;
}

/* Copies FMP slot `index` to `out`: -1 for a null `out`, -2 out of range, else `index`. */
s32 copyFmpSlot(NetworkInstance* self, NetworkFmpSlot* out, s32 index)
{
    if (out == NULL) {
        return -1;
    }
    if (index < 0 || index >= getFmpSize((PatInterface*)self)) {
        return -2;
    }
    memcpy(out, &((PatInterface*)self)->fmpSlots_6C40[index], sizeof(NetworkFmpSlot));
    return index;
}

/* The index of the FMP slot whose payload is `value`, or -1. */
u32 getFmpSlotIndex(NetworkStateMachine* self, u32 value)
{
    s32 i;

    if (value == 0) {
        return -1;
    }
    for (i = 0; i < getFmpSize(self); i++) {
        if (value == self->fmpSlots_6C40[i].payload_00) {
            return i;
        }
    }
    return -1;
}

/* Records the terms version the caller holds and marks it unchanged. */
void setTermVersion(PatInterface* self, u32 value)
{
    self->termsVersion_8264 = value;
    self->termsChanged_8268 = 0;
}

/* The terms version last read. */
s32 getTermsVersion(PatInterface* self)
{
    return self->termsVersion_8264;
}

/* The media version string. */
char* getMediaVersion(PatInterface* self)
{
    return self->mediaVersion_6588;
}

/* The media version records' text. */
char* getStr1(PatInterface* self)
{
    return self->mediaVersionText_65A8;
}

/* Whether the PAT state reads 1 (maintenance terms). */
s32 isOpeningMaintenanceTerms(PatInterface* self)
{
    return self->patState_8254 == 1;
}

/* Whether the PAT state reads 1 or 2 (maintenance server). */
s32 isOpeningMaintenanceServer(PatInterface* self)
{
    return self->patState_8254 == 1 || self->patState_8254 == 2;
}

/* Whether the PAT handshake is up (state 3). */
s32 isSubState_8254_3(PatInterface* self)
{
    return self->patState_8254 == 3;
}

/* Whether an announcement text has arrived. */
s32 isOpeningAnnounce(PatInterface* self)
{
    return self->announceBuffer_8294 != NULL && (s8)self->announceBuffer_8294[0] != 0;
}

/* Whether the connection phase is 4 or 6. */
s32 isSubState_894F_4or6(PatInterface* self)
{
    return self->connectionPhase_894F == 4 || self->connectionPhase_894F == 6;
}

/* Whether the connection phase is 6. */
s32 isSubState_894F_6(PatInterface* self)
{
    return self->connectionPhase_894F == 6;
}

/* Whether the connection phase is 5. */
s32 isSubState_894F_5(PatInterface* self)
{
    return self->connectionPhase_894F == 5;
}

/* Whether the connection phase is 2. */
s32 isSubState_894F_2(PatInterface* self)
{
    return self->connectionPhase_894F == 2;
}

/* Whether the connection phase is 3. */
s32 isSubState_894F_3(PatInterface* self)
{
    return self->connectionPhase_894F == 3;
}

/* The last warning's value. */
u32 getWarningUInt2(PatInterface* self)
{
    return self->warningValue_8BC0;
}

/* Binds the terms buffer. */
void PatInterface::setTermsBuffer(s8* buffer, u32 size)
{
    termsBufferPtr_828C = (u8*)buffer;
    termsSize_826C = size;
}

/* Binds the maintenance text buffer (at most 1024 bytes are used). */
void PatInterface::setMaintenanceBuffer(s8* buffer, u32 size)
{
    maintenanceBuffer_8290 = (u8*)buffer;
    maintenanceSize_8270 = size < 1024 ? size : 1024;
}

/* Binds the announcement text buffer (at most 1024 bytes are used). */
void PatInterface::setAnnounceBuffer(s8* buffer, u32 size)
{
    announceBuffer_8294 = (u8*)buffer;
    announceSize_8274 = size < 1024 ? size : 1024;
}

/* Binds the no-charge text buffer (at most 1024 bytes are used). */
void PatInterface::setNoChargeBuffer(s8* buffer, u32 size)
{
    noChargeBuffer_8298 = (u8*)buffer;
    noChargeSize_8278 = size < 1024 ? size : 1024;
}

/* Binds the high-level vulgarity list's buffer (at most 32 KB are used). */
void setPatBuffer(PatInterface* self, u32 index, char* buffer, u32 size)
{
    self->vulgarityHighBuffer_829C = (u8*)buffer;
    self->vulgarityHighSize_827C = size < 0x8000 ? size : 0x8000;
}

/* Binds one of the low-level vulgarity list's two buffers (`index` 2 selects the second; at most 32 KB used). */
void setPatRange(PatInterface* self, u32 index, u32 address, u32 size)
{
    if (index == 2) {
        self->vulgarityPtr_82A4 = (u8*)address;
        self->vulgaritySize_8284 = size < 0x8000 ? size : 0x8000;
    } else {
        self->userListPtr_82A0 = (u8*)address;
        self->userListSize_8280 = size < 0x8000 ? size : 0x8000;
    }
}

/* Binds the patch message buffer (at most 1024 bytes are used). */
void PatInterface::setPatchMessageBuffer(s8* buffer, u32 size)
{
    patchMessageBuffer_82A8 = (u8*)buffer;
    patchMessageSize_8288 = size < 1024 ? size : 1024;
}

/* Arms or disarms the binary sub-machine. */
void setPatByteD400(NetworkInstance* self, u8 value)
{
    ((PatInterface*)self)->binaryActive_D400 = value;
}

/* Copies this client's 8-byte short id out. */
void getSelectedID(NetworkInstance* self, u8* out)
{
    memcpy(out, ((PatInterface*)self)->userRow_8B54.shortName_04, 8);
}

/* Copies this client's hunter name (32 bytes) out. */
void getSelectedHunterName(NetworkInstance* self, char* out)
{
    memcpy(out, ((PatInterface*)self)->userRow_8B54.accountName_0C, 32);
}

#pragma peephole on
/* Sets the +0x6138 flag. */
void setSomething(NetworkInstance* self, s32 value)
{
    ((PatInterface*)self)->flag_6138 = value;
}

#pragma peephole off

/* The login data getters: the reflect values the mediator mirrored. */
u32 getSomething3(NetworkStateMachine* self)
{
    return self->loginDataSize_655C;
}

u32 getSomething6(NetworkStateMachine* self)
{
    return self->loginDataCursor_6560;
}

u32 getSomething9(NetworkStateMachine* self)
{
    return self->loginDataPtr_6564;
}

/* This client's own search row values. */
u32 getSomething8(NetworkStateMachine* self)
{
    return self->mySearchValue_65E8;
}

u32 getSomething7(NetworkStateMachine* self)
{
    return self->mySearchValue_65EC;
}

/* Sets the +0x6138 flag. */
void setPatByte6138On(NetworkInstance* self)
{
    setSomething(self, 1);
}

/* The mediator's reflect values +0x30/+0x34/+0x38. */
void setPatReflectField30(PatInterface* self, u32 value)
{
    self->loginDataSize_655C = value;
}

void setPatReflectField34(PatInterface* self, u32 value)
{
    self->loginDataCursor_6560 = value;
}

void setPatReflectField38(PatInterface* self, u32 value)
{
    self->loginDataPtr_6564 = value;
}

/* Copies the mediator's reflect name +0x3C (at most 31 characters). */
void setPatReflectName3C(PatInterface* self, char* name)
{
    u32 length = strlen(name) < 31 ? strlen(name) : 31;

    memcpy(self->reflectName3C_6568, name, length);
    self->reflectName3C_6568[length] = 0;
}

/* Copies the mediator's reflect name +0x5C (at most 31 characters). */
void setPatReflectName5C(PatInterface* self, char* name)
{
    u32 length = strlen(name) < 31 ? strlen(name) : 31;

    memcpy(self->reflectName5C_65C8, name, length);
    self->reflectName5C_65C8[length] = 0;
}

/* Sets this client's row id. */
void setPatField854(PatInterface* self, u32 value)
{
    self->userRow_8B54.id_00 = value;
}

/* Copies this client's account name (at most 31 characters). */
void setPatField860(PatInterface* self, const char* name)
{
    u32 length = strlen(name) < 31 ? strlen(name) : 31;

    memcpy(self->userRow_8B54.accountName_0C, name, length);
    self->userRow_8B54.accountName_0C[length] = 0;
}
