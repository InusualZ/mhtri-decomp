/*
 * Network/PatInterface.cpp - the `PatInterface` singleton (reference count, error records, session handler slots, call
 *   stack, clock, accessors), the request state machine the session units drive (0x803FE8E4..: the block-1..4 resets
 *   and the `handleNetworkStateN` dispatchers), and the packet layer (0x804006A8..: the `sendReq*` request builders and
 *   the `recv*` handlers).  Bodies follow the address order.
 * RANGE. .text 0x803FCC34-0x804123F8 (459 functions); .rodata 0x80570E20-0x80570E70, .data 0x80600978-0x80602428,
 *   .sdata 0x80793968-0x80793988, .sbss 0x80794CB0-0x80794CB8, .sdata2 0x8079C7D0-0x8079C868, extab, extabindex.  One
 *   TU: the constructor (0x803FCC34) stores the 161-slot table 0x80602198 whose slots run to `recvCommand` 0x804120B8,
 *   followed by the V->S seam at 0x80602428 ("NHTTPStartup").  Left: the vtable-then-string seam ending
 *   `Network/PatConnection.cpp`; right: `onNHTTPDestroyed` opens `Network/NetworkPool.cpp` (only its
 *   `stepCleanup__11NetworkPoolFv` takes the address); the data edges follow the readers (`splitcheck --unit`:
 *   data-order PASS).
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md).  File-scope `#pragma peephole off`
 *   (memset/memcpy argument order, the unfused `extsb`+`cmpwi` of `isOpeningAnnounce`, the `clrlwi r0,r0,16` before the
 *   ticket size's `sth`, the `bge`+`b` pairs), on for `setSomething` (its s32 store keeps no `clrlwi` only with the
 *   pass on) and for the state machine except `handleNetworkState2Binary` (retail's unfused `extsb r0,r0` + `cmpwi
 *   r0,0`, playbook 39).
 * NAMES. The runtime dump names 14 of the state machine's 21 functions.  GUESSes from the bodies:
 *   `advanceNetworkState5` (gates on the +0x6133 == 5 byte); the `Fmp`/`Binary` suffixes of the three block-2
 *   dispatchers the map spells alike (the request family each calls); `sendReqOpcode1B`/`1F` (the wire opcode);
 *   `NetworkStateMachine`'s fields (the object is 0x16D08 B, beyond the dump's views); `writeUInt32Shared` (the 4-byte
 *   tail-branch twin of `writeUInt32`); `getSomething6`/`getSomething9` (the map's `getSomethingN` scheme);
 *   `dispatchSessionHandlers` (0x8041233C, walks the eight handler slots); `clearErrorRecord613c`,
 *   `buildErrorInfo613c`, `getErrorInfoOrCode654c` (the `errorRecordCode613c` scheme); `initItemList`, `readItemList`,
 *   `createItemListStack` (at most 64 16-byte rows off the call stack), `releaseItemListStack` and the records
 *   `PatItem`/`PatItemList`/`PatItemNotice`/`PatUserPositionNotice`/ `PatDetailSearchResult`; the `recv*` parameter
 *   types (r4 the packet table index `recvAnsNg` stores, r5 the 8-byte header).  Data rows from their content:
 *   `sessionTimeoutParam`/`Param2` (0x8079C7D8/DDA, the bytes {1,2,3}), `requestHeaderWord0`/`1` (0x8079C7E0/E4),
 *   `maskedUserName` (0x80793968, "******").  GUESSes named by the mediator band for the field or slot each touches:
 *   `setPatReflectPageRange`, `PatInterface_clear`, `PatInterface_isReady`, `getPatServerTime`, `setPatBuffer`,
 *   `setPatRange`, `getPatAccountName`, `setPatReflectField30/34/38`, `setPatReflectName3C/5C`, `setPatField854/860`.
 * RESIDUALS. 151 rows unwritten (objdiff scores them zero), in address order:
 *   - the singleton: `setPatReflectPageRange` (0x803FD04C), `stepPatInterface`, `buildErrorInfo613c`,
 *     `getErrorInfoOrCode654c`, `postError`, `pushStack`, `setConnectServerType`/`chooseServerAddress`/
 *     `getFmpSelection`/`copyServerBlock` (0x803FDCB0..0x803FDE84), `saveFmpSelection`..`updatePatInterface`
 *     (0x803FE03C..0x803FE184), `getPatServerTime`, `getBinaryToken`, `getPatAccountName`, `getSomething4` (its
 *     header-declared `s8` return would add an `extsb` retail lacks), `createItemListStack`/`releaseItemListStack`/
 *     `appendItemList` (0x803FE4D8..0x803FE73C);
 *   - the request builders: `sendReqLayerChildInfo`, `sendReqLayerUserInfoSet`, `sendReqLayerUserListHead`,
 *     `sendReqLayerUserSearchHead`, the layer notices, mediation and detail-search requests and circle create/info/join
 *     (0x80401EC8..0x8040275C), `sendReqCircleMatchOptionSet`, `sendReqCircleInfoSet`/`ListLayer`/`ListHead`
 *     (0x80402930..0x80402B04), `sendReqCircleUserList` through `sendReqBinaryUser` (0x80402CC8..0x80403670),
 *     `sendReqUserSearchHead`, `sendReqUserSearchInfo`..`sendReqFriendAdd` (0x80403978..0x80403C80),
 *     `sendReqFriendList`/`sendReqBlackAdd`, `sendReqBlackList`/`sendReqChannelInfo`;
 *   - `recvReqMemoryCheck` (0x80404EE8), a `recv*` virtual with no body;
 *   - the item readers/writers, `recvCommand` and `dispatchSessionHandlers` (0x8040E48C..0x804123F8, 84 rows).
 *   The packet table is not emitted.  Partial rows:
 *  - `handleNetworkState2Binary`: the case-60 tokenizer leaves a loop from inside two nested loops (the conformant
 *    `&& != 0` plus `break` is 76 B short); retail recomputes the offset from `i` with `lbzx` off `self`, unrolls the
 *    token fill by 8 with an `andi.` remainder and saves 2 registers in line (ours 5, `_savegpr_27`); `-O2`, `-opt
 *    nostrength`, `u8*`/`u32*` aliases and every loop form tried are worse;
 *  - `sendReqUserObject`, `sendReqUnknownCheck`: the masked tag index (retail `clrlwi r0,r0,24` before `stbx`; `count`
 *    is a local whose range MWCC proves, so it folds the u8 conversion; an inlined `appendTag` helper and every index
 *    spelling tried are byte-identical); the error path is retail's through a by-value `postError_288(self, info)`; the
 *    vcall stages its vptr through a scratch register; `sendReqUnknownCheck` keeps one extra `li` (its two `1`
 *    constants do not CSE);
 *  - `handleNetworkState2Fmp`: one more callee-saved register (ours caches `serverCount_6604[1]` in r30) and the
 *    `memcpy` argument order in case 50;
 *  - `handleNetworkState2`, `handleNetworkState4`: retail's unfused `lbz; extsb; cmpwi r0,0` / `subf` + `cmpwi r0,0`
 *    where ours folds (not a peephole row; `-opt nocse`/`nopropagation`/`nodeadcode`/`nolifetimes` byte-identical), and
 *    argument setups one slot out of place;
 *  - `handleNetworkState1`: allocator colouring (the switch value in r4, retail r6; the transient `dataSent` in r4/r6);
 *  - `sendReqLoginInfo`: the remaining operand rows after `(u8)(loginInfoSent_82B4 + 255) <= 1` (retail's
 *    `addi`/`clrlwi`/`cmplwi` on the memory-loaded u8);
 *  - `sendReqOpcode1B`: retail loads both request-header words before storing either (a brace initialiser, temporaries
 *    and a struct local tried);
 *  - `reportPatError`: retail materialises `&pendingError_654C` after the code load;
 *  - `recvAnsAgreementPageInfo`: retail addresses the page count as `infoPtr+12`, ours folds it to `r1+28`.
 * SHAPES. The destructor 0x803FCF8C is the key function, so this object emits `__vt__12PatInterface` (rule 10); the
 *   class derives from `PatConnection` (`Network/PatConnection.h`).  The `recv*` handlers are virtuals (slots
 *   +0x10..+0x280) that `recvCommand` reaches through the packet table's member pointers (`__ptmf_scall`), so their map
 *   rows carry the manglings; the `sendReq*` builders and the accessors keep C linkage (the map names them unmangled)
 *   and take `NetworkInstance*` (a typedef of `PatInterface`).  Each handler logs `"%s:<name> ok\n"` with
 *   `getServerName`.
 *  - an aliasing local (`NetworkStateMachine* st = ...`, `u8 state = <field>`) is its own web: write the expression at
 *    the use site; a state byte taken into a local is `s32`;
 *  - retail branches over a body and leaves the return last: write the positive test with the return as the case's last
 *    statement, the branch-taken block as the `else`, `a && b && c` negated (playbook 34, 71);
 *  - `cmpwi` in retail means the source compared signed: `(s32)field`, `(s32)count < 30`, `(s32)field == -1`;
 *  - a u8 comparison survives only on a memory-loaded operand: `(u8)(field + 255) <= 1`;
 *  - a local byte buffer's declared size is the frame (`u8 block[16]` for retail's 0x30); an SDA extern needs its size
 *    (`extern const char maskedUserName[7]`, playbook 64; with `[]` MWCC emits `lis`/`addi` and two `R_PPC_ADDR16_*`
 *    relocations);
 *  - `count = count + 1; tags[count - 1] = v` (an alignment effect, kept for its score); `tags[count++] = v` with a
 *    known count is retail's `li`/`li`/`stb`; `u32* tokens = self->binaryTokens_D60C;` in `handleNetworkState2Binary`;
 *  - locals of one size are laid out in reverse declaration order (playbook 63); a failed `readBodySlice` returns its
 *    own result; the stack-clamped list answers divide into a `maxCount` local; `x == 0 ? a : b` decides which constant
 *    loads first; the position notice's loop indexes `list.items_04[i]` (a walking pointer swaps r3/r4);
 *  - the state machine's constants are this unit's `.sdata2`/`.sdata` rows, declared `extern` in the unit header and
 *    never defined (playbook 29: defining them rebuilds the pool); `buildErrorInfo613c`'s codes 0x80050038/0x80050044
 *    read as relocations in the split object (the `block_relocations` case).
 */

#include "types.h"
#include "Network/PatInterface.h"
#include "Network/PatConnection.h"          /* the request writers */
#include "Network/NetworkWiiMediator.h"     /* setMediatorState68A / getMediatorState68A */
#include "unsplit/Network.h"                /* getNetworkLogger */
#include "unsplit/Runtime.PPCEABI.H.h"      /* strlen, strcmp */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#pragma peephole off

/* ==== the singleton ============================================================================================ */

PatInterface* PatInterface::mpInstance;

/* Publishes the singleton, clears its handler slots, names and server tables, chooses the default server and pulls
 * the mediator's mirrored reflect state, server blocks, flags and ticket in. */
PatInterface::PatInterface()
{
    s32 i;
    PatServerAddress address;

    mpInstance = this;
    refCount_60D4 = 0;
    for (i = 0; i < 8; i++) {
        eventCallbacks_60D8[i] = NULL;
        eventCallbackArgs_60F8[i] = 0;
    }
    flag_6558 = 0;
    shutdownFlag_6559 = 0;
    loginDataSize_655C = 0;
    loginDataCursor_6560 = 0;
    loginDataPtr_6564 = 0;
    memset(reflectName3C_6568, 0, sizeof(reflectName3C_6568));
    memset(mediaVersion_6588, 0, sizeof(mediaVersion_6588));
    memset(mediaVersionText_65A8, 0, sizeof(mediaVersionText_65A8));
    memset(reflectName5C_65C8, 0, sizeof(reflectName5C_65C8));
    serverType_65F0 = -1;
    memset(serverIndex_65F4, -1, sizeof(serverIndex_65F4));
    memset(serverCount_6604, 0, sizeof(serverCount_6604));
    memset(serverReserve_6614, 0, sizeof(serverReserve_6614));
    memset(&serverAddress_6A2C, 0, sizeof(serverAddress_6A2C));
    memset(&lmpServer_6B32, 0, sizeof(lmpServer_6B32));
    memset(fmpSlots_6C40, 0, sizeof(fmpSlots_6C40));
    serverCount_6604[0] = 1;
    snprintf(serverAddress_6A2C.host_000, 256, "mmh-t1-opn03.mmh-service.capcom.co.jp");
    serverAddress_6A2C.port_104 = 8200;
    serverCount_6604[2] = 1;
    chooseServerAddress(this, 2, 0);
    memset(&rfpServer_814E, 0, sizeof(rfpServer_814E));
    serverCount_6604[3] = 0;
    updatePatInterface(this, 0, 0, 0);
    setTermVersion(this, 0);
    termsSize_826C = 0;
    maintenanceSize_8270 = 0;
    announceSize_8274 = 0;
    noChargeSize_8278 = 0;
    replySize_8BC4 = 0;
    vulgarityHighSize_827C = 0;
    userListSize_8280 = 0;
    vulgaritySize_8284 = 0;
    patchMessageSize_8288 = 0;
    termsBufferPtr_828C = NULL;
    maintenanceBuffer_8290 = NULL;
    announceBuffer_8294 = NULL;
    noChargeBuffer_8298 = NULL;
    replyBuffer_8BC8 = NULL;
    vulgarityHighBuffer_829C = NULL;
    userListPtr_82A0 = NULL;
    vulgarityPtr_82A4 = NULL;
    patchMessageBuffer_82A8 = NULL;
    commonKeyBuffer_8948 = NULL;
    commonKeyReady_894C = 0;
    ticket_8BB0 = NULL;
    getReflectPageRange(getInstance(), (u32*)&eventCallbacks_60D8[0], &eventCallbackArgs_60F8[0]);
    getReflectField30(getInstance(), &loginDataSize_655C);
    getReflectField34(getInstance(), &loginDataCursor_6560);
    getReflectField38(getInstance(), &loginDataPtr_6564);
    getReflectName3C(getInstance(), reflectName3C_6568, sizeof(reflectName3C_6568));
    getReflectName5C(getInstance(), reflectName5C_65C8, sizeof(reflectName5C_65C8));
    getReflectPageBuffer((char*)getInstance(), (char**)&replyBuffer_8BC8, (unsigned int*)&replySize_8BC4);
    getMediatorBufferA(getInstance(), (u8*)&address);
    if (address.host_000[0] != 0) {
        memcpy(&lmpServer_6B32, &address, sizeof(PatServerAddress));
        chooseServerAddress(this, 0, 0);
    }
    getMediatorBufferB(getInstance(), (u8*)&address);
    if (address.host_000[0] != 0) {
        memcpy(&rfpServer_814E, &address, sizeof(PatServerAddress));
        chooseServerAddress(this, 3, 0);
        serverCount_6604[3] = 1;
    }
    getMediatorFlag78C(getInstance(), &warningKind_8BBC);
    getMediatorNameBuffer(getInstance(), (u32*)&commonKeyBuffer_8948, &commonKeyReady_894C);
    getMediatorField288(getInstance(), (u32*)&ticket_8BB0);
    setPatReflectPageRange(this, 0, 0);
}

/* Closes the connection, hands the server blocks and the flags back to the mediator and retracts the singleton. */
PatInterface::~PatInterface()
{
    while (disconnect() == 0) {
    }
    setMediatorBufferA(getInstance(), (const u8*)&lmpServer_6B32);
    setMediatorBufferB(getInstance(), (const u8*)&rfpServer_814E);
    setMediatorFlag78C(getInstance(), warningKind_8BBC);
    setMediatorFlag78B(getInstance(), commonKeyReady_894C);
    mpInstance = NULL;
}

/* Restores the connection's defaults, then clears the error records, the clock, the state machine's state bytes, the
 * login and user rows, the binary reply and the call stack (18 KB free). */
void PatInterface::resetDefaults()
{
    PatConnection::resetDefaults();
    memset(&errorRecord_613C, 0, sizeof(errorRecord_613C));
    memset(&shutdownRecord_6344, 0, sizeof(shutdownRecord_6344));
    memset(&pendingError_654C, 0, sizeof(pendingError_654C));
    memset(&searchMode_6118, -1, 3);
    busy_611B = 0;
    mySearchValue_65E8 = 0;
    mySearchValue_65EC = 0;
    clock_611C = 0.0f;
    memset(&clockAtSync_6120, 0, 8);
    gameTimeBase_6128 = 0;
    serverTime_612C = 0;
    sessionArmed_6130 = 0;
    termsArmed_6131 = 0;
    sessionState_6132 = 0;
    subState_6133 = 0;
    binaryState_6134 = 0;
    requestState_6135 = 0;
    shutdownMode_6136 = 0;
    fmpState_6137 = 0;
    flag_6138 = 0;
    patState_8254 = 0;
    loginFields_82AC[0] = 0;
    loginFields_82AC[1] = 0;
    fmpPhase_894D = 0;
    patPhase_894E = 0;
    connectionPhase_894F = 0;
    memset(&sessionReady_8950, 0, 513);
    memset(userRow_8B54.shortName_04, 0, sizeof(userRow_8B54.shortName_04));
    userRowCount_8BB4 = 0;
    userRows_8BB8 = NULL;
    binarySize_D404 = 0;
    binaryTextReady_D408 = 0;
    memset(binaryText_D409, 0, 512);
    memset(binaryTokens_D60C, 0, sizeof(binaryTokens_D60C));
    warningValue_8BC0 = 0;
    fmpListActive_6C38 = 0;
    fmpListReady_6C3C = 0;
    fmpSelected_8040 = -1;
    fmpQueryValue_8044 = -1;
    memset(fmpReply_8048, 0, sizeof(fmpReply_8048));
    layerMovePending_D62C = 0;
    matchStartValue_D630 = 0;
    stackRemaining_8BD4 = 0;
    stackUsed_8BD8 = 0x4800;
}

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
/* free: retail C linkage - the map names it unmangled */
void PatInterface_clear(PatInterface* self)
{
    if (PatInterface_isReady(self) == 0) {
        resetCallback((NetworkInstance*)self, 0);
        resetCallback((NetworkInstance*)self, 2);
    }
}

/* Whether the singleton is referenced. */
/* free: retail C linkage - the map names it unmangled */
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
    return self->serverCount_6604[1];
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

/* ==== the request state machine (the peephole pass is on here, off again for handleNetworkState2Binary) ===== */

#pragma peephole on

extern "C" {

/* defined here */
s32 resetNetworkState(NetworkInstance* self);
s32 resetNetworkState3(NetworkInstance* self);
s32 handleNetworkState1(NetworkInstance* self);
s32 advanceNetworkState5(NetworkInstance* self);
s32 sendReqServerTime(NetworkInstance* self);
s32 sendReqShut(NetworkInstance* self, s32 mode);
s32 sendReqTicket(NetworkInstance* self);
s32 sendReqCommonKey(NetworkInstance* self);
s32 sendReqLoginInfo(NetworkInstance* self);
s32 sendReqChargeInfo(NetworkInstance* self, s32 mode);
s32 sendReqUserListData(NetworkInstance* self, s32 mode, u32 count);
s32 sendReqUserObject(NetworkInstance* self, s32 index, NetworkUserRow* row);
s32 sendReqOpcode1B(NetworkInstance* self, u32 a, u32 b);
s32 sendReqOpcode1F(NetworkInstance* self);
s32 sendServerTimeout(NetworkInstance* self, const u32* values);
s32 sendReqUnknownCheck(NetworkInstance* self, const u8* tags, const u8* data, u32 size);

/* the band's request emitters this range does not own are declared in `Network/network_layer_io.h` */

} /* extern "C" */

/* ------------------------------------------------------------------------------------------------ */

s32 resetNetworkState(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    if (st->sessionArmed_6130 == 0) {
        st->sessionArmed_6130 = 1;
        st->sessionState_6132 = 0;
        st->subState_6133 = 0;
        st->requestState_6135 = 0;
        st->shutdownMode_6136 = 0;
        st->flag_6558 = 0;
        st->shutdownFlag_6559 = 0;
        st->patPhase_894E = 0;
    }
    return 0;
}

s32 resetNetworkState3(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    if (st->termsArmed_6131 == 0) {
        st->termsArmed_6131 = 1;
        st->sessionState_6132 = 0;
        st->subState_6133 = 0;
        st->shutdownMode_6136 = 0;
        st->sessionArmed_6130 = 0;
        st->flag_6558 = 0;
        st->shutdownFlag_6559 = 0;
    }
    return 0;
}

/* Drives the request state machine one step for the current `+0x6132` state.  The `dont_inline` region
 * is load-bearing: with `-inline auto` MWCC folds the 56-byte `resetNetworkState3` into case 255's
 * `bl`, which retail keeps (the proc is also called from another TU, so the original build did not
 * inline it here). */
#pragma dont_inline on
s32 handleNetworkState1(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 flag;
    s32 mode;

    switch (st->sessionState_6132) {
    case 1:
        if (st->binaryState_6134 == 10) {
            st->binaryState_6134 = 90;
        }
        break;
    case 5:
        st->sessionState_6132 += 5;
        break;
    case 10:
        if (isSubState_8254_3((PatInterface*)st) != 0) {
            st->sessionState_6132 = 200;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqAuthenticationToken(self, getNASToken(getInstance()));
        break;
    case 20:
        if (isOpeningMaintenanceServer((PatInterface*)st) == 0) {
            if (st->maintenanceSize_8270 != 0) {
                *st->maintenanceBuffer_8290 = 0;
            }
            st->sessionState_6132 += 10;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqMaintenance(self);
        break;
    case 30:
        if (isOpeningMaintenanceTerms((PatInterface*)st) != 0) {
            if (st->termsSize_826C != 0) {
                *st->termsBufferPtr_828C = 0;
            }
            st->sessionState_6132 = 245;
            break;
        }
        st->dataTotal_825C = 0;
        st->sessionState_6132 += 5;
        sendReqTermsVersion(self);
        break;
    case 40:
        if (st->termsChanged_8268 == 0 || st->dataTotal_825C == 0 || st->termsSize_826C == 0) {
            if (st->termsSize_826C != 0) {
                *st->termsBufferPtr_828C = 0;
            }
            st->sessionState_6132 += 20;
            break;
        }
        if (st->dataTotal_825C >= st->termsSize_826C) {
            st->dataTotal_825C = st->termsSize_826C - 1;
        }
        memset(st->termsBufferPtr_828C, 0, st->termsSize_826C);
        st->dataSent_8260 = 0;
        st->sessionState_6132 += 5;
        break;
    case 45:
    {
        u32 len;

        st->sessionState_6132 += 5;
        len = st->dataTotal_825C - st->dataSent_8260;
        sendReqTerms(self, st->termsVersion_8264, st->dataSent_8260,
                     len < 8192 ? len : 8192);
        break;
    }
    case 55:
        if (st->dataTotal_825C > st->dataSent_8260) {
            st->sessionState_6132 -= 10;
        } else {
            st->sessionState_6132 += 5;
        }
        break;
    case 60:
        if (isOpeningMaintenanceServer((PatInterface*)st) != 0) {
            if (st->announceSize_8274 != 0) {
                *st->announceBuffer_8294 = 0;
            }
            st->sessionState_6132 = 245;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqAnnounce(self);
        break;
    case 70:
        if (st->noChargeSize_8278 == 0) {
            st->sessionState_6132 += 10;
            break;
        }
        st->sessionState_6132 += 5;
        sendReqNoCharge(self);
        break;
    case 80:
        st->dataTotal_825C = 0;
        st->sessionState_6132 += 5;
        sendReqVulgarityInfoLow(self, 2);
        break;
    case 90:
        if (st->dataTotal_825C == 0 || st->vulgaritySize_8284 == 0) {
            if (st->vulgaritySize_8284 != 0) {
                *st->vulgarityPtr_82A4 = 0;
            }
            st->sessionState_6132 += 20;
            break;
        }
        if (st->dataTotal_825C >= st->vulgaritySize_8284) {
            st->dataTotal_825C = st->vulgaritySize_8284 - 1;
        }
        memset(st->vulgarityPtr_82A4, 0, st->vulgaritySize_8284);
        st->dataSent_8260 = 0;
        st->sessionState_6132 += 5;
        break;
    case 95:
    {
        u32 len;

        st->sessionState_6132 += 5;
        len = st->dataTotal_825C - st->dataSent_8260;
        sendReqVulgarityLow(self, 2, st->sendSlice_8258, st->dataSent_8260, len < 8192 ? len : 8192);
        break;
    }
    case 105:
        if (st->dataTotal_825C > st->dataSent_8260) {
            st->sessionState_6132 -= 10;
        } else {
            st->sessionState_6132 += 5;
        }
        break;
    case 110:
        st->dataTotal_825C = 0;
        st->sessionState_6132 += 5;
        sendReqVulgarityInfoLow(self, 1);
        break;
    case 120:
        if (st->dataTotal_825C == 0 || st->userListSize_8280 == 0) {
            if (st->userListSize_8280 != 0) {
                *st->userListPtr_82A0 = 0;
            }
            st->sessionState_6132 += 20;
            break;
        }
        if (st->dataTotal_825C >= st->userListSize_8280) {
            st->dataTotal_825C = st->userListSize_8280 - 1;
        }
        memset(st->userListPtr_82A0, 0, st->userListSize_8280);
        st->dataSent_8260 = 0;
        st->sessionState_6132 += 5;
        break;
    case 125:
    {
        u32 len;

        st->sessionState_6132 += 5;
        len = st->dataTotal_825C - st->dataSent_8260;
        sendReqVulgarityLow(self, 1, st->sendSlice_8258, st->dataSent_8260, len < 8192 ? len : 8192);
        break;
    }
    case 135:
        if (st->dataTotal_825C > st->dataSent_8260) {
            st->sessionState_6132 -= 10;
        } else {
            st->sessionState_6132 += 5;
        }
        break;
    case 140:
        st->sessionState_6132 += 5;
        sendReqCommonKey(self);
        break;
    case 150:
        st->sessionState_6132 += 5;
        sendReqLmpConnect(self);
        break;
    case 160:
        st->sessionState_6132 = 245;
        break;
    case 200:
        st->sessionState_6132 += 5;
        sendReqMediaVersionInfo(self);
        break;
    case 210:
        st->sessionState_6132 = 245;
        break;
    case 245:
        if (st->binaryState_6134 != 10) {
            st->sessionState_6132 += 1;
            break;
        }
        st->sessionState_6132 += 5;
        flag = 0;
        if (isOpeningMaintenanceServer((PatInterface*)st) != 0 || isSubState_8254_3((PatInterface*)st) != 0) {
            flag = 1;
        }
        mode = 2;
        if (flag != 0) {
            mode = 1;
        }
        sendReqShut(self, mode);
        break;
    case 255:
        resetNetworkState3(self);
        chooseServerAddress(st, 0, 0);
        updatePatInterface((PatInterface*)st, 0, 0, 0);
        return 1;
    default:
        break;
    }
    return 0;
}

#pragma dont_inline off

/* Clears the `+0x6133` sub-state once the `+0x894D` phase has advanced past 1. */
s32 advanceNetworkState5(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 state = st->subState_6133;

    if (state == 5) {
        if (st->fmpPhase_894D != 1) {
            return -state;
        }
        st->subState_6133 = 0;
        return 1;
    }
    return 0;
}

/* Drives the block-2 account sub-machine: login, charge, ticket and the user list. */
s32 handleNetworkState2(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    switch (st->requestState_6135) {
    case 5:
        if (st->fmpPhase_894D == 1) {
            if (st->loginInfoSent_82B4 == 0) {
                st->requestState_6135 = st->requestState_6135 + 5;
                break;
            }
            st->requestState_6135 = 120;
            break;
        }
        return -(s32)(st->requestState_6135 + 1);
    case 10:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqLoginInfo(self);
        break;
    case 20:
        if (st->connectionPhase_894F == 1) {
            st->requestState_6135 = st->requestState_6135 + 10;
            break;
        }
        if (isSubState_894F_5((PatInterface*)st) != 0 || isSubState_894F_3((PatInterface*)st) != 0) {
            u32 size = st->replySize_8BC4;

            if (size != 0) {
                u32 len = size - 1;

                if (strlen(st->termText_8951) < len) {
                    len = strlen(st->termText_8951);
                }
                memcpy(st->replyBuffer_8BC8, st->termText_8951, len);
                st->replyBuffer_8BC8[len] = 0;
            }
            st->requestState_6135 = 250;
            break;
        }
        if (isSubState_894F_2((PatInterface*)st) != 0 || isSubState_894F_4or6((PatInterface*)st) != 0) {
            st->requestState_6135 = 225;
            break;
        }
        return -st->requestState_6135;
    case 30:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqTicket(self);
        break;
    case 40:
        if (isSubState_894F_6((PatInterface*)st) != 0) {
            st->requestState_6135 = 250;
            break;
        }
        st->requestState_6135 += 5;
        sendReqOpcode1B(self, 1, 6);
        break;
    case 50:
        if ((s32)st->userRowCount_8BB4 > 0) {
            u32 total;
            u32 rows;

            st->userRows_8BB8 = createStack(st, st->userRowCount_8BB4 * 92, &total);
            rows = total / 92;
            if ((s32)st->userRowCount_8BB4 > (s32)rows) {
                st->userRowCount_8BB4 = rows;
                growStackSize(st, total - rows * 92);
            }
            st->requestState_6135 += 5;
            sendReqUserListData(self, 1, st->userRowCount_8BB4);
            break;
        }
        st->requestState_6135 = st->requestState_6135 + 10;
        break;
    case 60:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqOpcode1F(self);
        break;
    case 70:
    {
        s32 count;
        s32 i;

        count = (s32)st->userRowCount_8BB4;
        if (count > 0) {
            for (i = 0; i < count; i++) {
                if (((NetworkUserRow*)st->userRows_8BB8)[i].shortName_04[0] == 0) {
                    return -(s32)st->requestState_6135;
                }
            }
        } else {
            return -(s32)(st->requestState_6135 + 1);
        }
        st->requestState_6135 += 10;
        break;
    }
    case 80:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqServerTime(self);
        break;
    case 90:
        dispatchSessionHandlers(st, 3, 0, 0, st->userRowCount_8BB4, st->userRows_8BB8);
        return 1;
    case 100:
        st->loginInfoSent_82B4 = 1;
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqChargeInfo(self, 1);
        break;
    case 110:
        setConnectionPaths((NetworkInstance*)getInstance(), st->userIdText_82D5,
                           st->userPasswordText_8301);
        setMediatorState68A(getInstance(), 0);
        st->requestState_6135 = 10;
        break;
    case 120:
        st->loginInfoSent_82B4 = 3;
        setMediatorState68A(getInstance(), 0);
        st->requestState_6135 = 10;
        break;
    case 230:
        if (isSubState_894F_2((PatInterface*)st) != 0) {
            st->requestState_6135 = 30;
            break;
        }
        if (isSubState_894F_6((PatInterface*)st) != 0) {
            st->requestState_6135 += 5;
            sendReqRfpConnect(self);
            break;
        }
        st->requestState_6135 += 20;
        break;
    case 240:
        if (isSubState_894F_6((PatInterface*)st) != 0) {
            st->requestState_6135 = 30;
            break;
        }
        st->requestState_6135 += 10;
        break;
    case 250:
        return 1;
    default:
        break;
    }
    return 0;
}

/* Drives the block-2 FMP sub-machine: the friend-list versions, rows and info requests. */
s32 handleNetworkState2Fmp(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    switch (st->requestState_6135) {
    case 5:
        if (st->fmpPhase_894D == 2) {
            if (st->loginInfoSent_82B4 == 0) {
                st->requestState_6135 = st->requestState_6135 + 5;
            } else {
                u8 mediatorState;

                getMediatorState68A(getInstance(), &mediatorState);
                if (mediatorState == 1) {
                    st->requestState_6135 = 100;
                    break;
                }
                st->requestState_6135 += 5;
            }
        } else {
            return -(s32)(st->requestState_6135 + 1);
        }
        /* falls through */
    case 10:
    {
        NetworkUserRow* rows;
        s32 count;
        s32 i;

        count = (s32)st->userRowCount_8BB4;
        rows = (NetworkUserRow*)st->userRows_8BB8;
        for (i = 0; i < count; i++) {
            if (rows[i].id_00 == st->userRow_8B54.id_00) {
                break;
            }
        }
        if (i >= count) {
            return -(s32)st->requestState_6135;
        }
        st->userRow_8B54.field_30 = 2;
        st->userRow_8B54.field_2C = getSomething3(st);
        st->userRow_8B54.field_34 = getSomething6(st);
        st->userRow_8B54.field_38 = getSomething9(st);
        st->requestState_6135 += 5;
        sendReqUserObject(self, i, &st->userRow_8B54);
        growStackSize(st, count * 92);
        st->userRows_8BB8 = 0;
        break;
    }
    case 20:
        if (st->sessionReady_8950 == 1) {
            st->requestState_6135 = st->requestState_6135 + 5;
            sendReqTicket(self);
            break;
        }
        return -(s32)st->requestState_6135;
    case 30:
        st->requestState_6135 = st->requestState_6135 + 5;
        resetNetworkState4(self);
        break;
    case 35:
    {
        s32 result = handleNetworkState4(self, 80);

        if (result > 0) {
            if ((s32)st->serverCount_6604[1] > 0) {
                st->requestState_6135 += 5;
                break;
            }
            return -(s32)st->requestState_6135;
        }
        if (result < 0) {
            return -(s32)(st->requestState_6135 + result);
        }
        break;
    }
    case 40:
    {
        u32 bestIndex = (u32)-1;
        s32 bestValue = -1;
        s32 secondValue = -1;
        u32 i;

        st->fmpQueryValue_8044 = (u32)-1;
        for (i = 0; (s32)i < (s32)st->serverCount_6604[1]; i++) {
            u32 remaining;

            if ((s32)st->fmpSlots_6C40[i].total_14 <= 0) {
                continue;
            }
            remaining = st->fmpSlots_6C40[i].total_14 - st->fmpSlots_6C40[i].done_10;
            if (bestValue <= (s32)remaining) {
                secondValue = bestValue;
                st->fmpQueryValue_8044 = bestIndex;
                bestValue = (s32)remaining;
                bestIndex = i;
            } else if (secondValue <= (s32)remaining) {
                secondValue = (s32)remaining;
                st->fmpQueryValue_8044 = i;
            }
        }
        if ((s32)st->fmpQueryValue_8044 == -1) {
            st->requestState_6135 += 20;
            break;
        }
        st->serverIndex_65F4[1] = st->fmpQueryValue_8044;
        st->requestState_6135 += 5;
        sendReqFmpInfo(self, st->fmpSlots_6C40[st->serverIndex_65F4[1]].payload_00, 1);
        break;
    }
    case 50:
        memcpy(st->fmpReply_8048, st->serverReserve_6614[1], 262);
        st->requestState_6135 += 10;
        break;
    case 60:
    {
        s32 bestIndex = -1;
        s32 bestValue = -1;
        u32 i;

        for (i = 0; (s32)i < (s32)st->serverCount_6604[1]; i++) {
            u32 remaining;

            if ((s32)st->fmpSlots_6C40[i].total_14 <= 0) {
                continue;
            }
            remaining = st->fmpSlots_6C40[i].total_14 - st->fmpSlots_6C40[i].done_10;
            if (bestValue <= (s32)remaining) {
                bestValue = (s32)remaining;
                bestIndex = i;
            }
        }
        if (bestIndex < 0) {
            return -(s32)st->requestState_6135;
        }
        st->serverIndex_65F4[1] = (u32)bestIndex;
        st->requestState_6135 += 5;
        sendReqFmpInfo(self, st->fmpSlots_6C40[bestIndex].payload_00, 1);
        break;
    }
    case 70:
        if (st->eventCallbacks_60D8[4] == 0) {
            dispatchSessionHandlers(st, 0x8001, 0, 0, st->serverCount_6604[1],
                                    (const u8*)st->fmpSlots_6C40);
        }
        return 1;
    case 100:
        st->loginInfoSent_82B4 = 2;
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqChargeInfo(self, 2);
        break;
    case 110:
        setConnectionPaths((NetworkInstance*)getInstance(), st->userIdText_82D5,
                           st->userPasswordText_8301);
        st->requestState_6135 = 200;
        break;
    case 200:
        st->requestState_6135 = st->requestState_6135 + 5;
        sendReqLoginInfo(self);
        break;
    case 210:
        if (st->connectionPhase_894F == 1) {
            st->requestState_6135 = 10;
            break;
        }
        return -(s32)st->requestState_6135;
    default:
        break;
    }
    return 0;
}

/* Drives the block-2 binary/circle sub-machine. */
#pragma peephole off
s32 handleNetworkState2Binary(NetworkInstance* self)
{
    u8 state = ((NetworkStateMachine*)self)->requestState_6135;

    switch (state) {
    case 5:
        if (((NetworkStateMachine*)self)->fmpPhase_894D == 5) {
            ((NetworkStateMachine*)self)->binaryState_6134 = 90;
            ((NetworkStateMachine*)self)->requestState_6135 = 0;
            ((NetworkStateMachine*)self)->flag_6558 = 0;
            ((NetworkStateMachine*)self)->patPhase_894E = ((NetworkStateMachine*)self)->fmpPhase_894D;
            break;
        }
        ((NetworkStateMachine*)self)->fmpQueryValue_8044 = (u32)-1;
        if (((NetworkStateMachine*)self)->fmpPhase_894D == 3) {
            ((NetworkStateMachine*)self)->shutdownFlag_6559 = 1;
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            break;
        }
        return -state;
    case 10:
        ((NetworkStateMachine*)self)->requestState_6135 = state + 5;
        sendReqServerTime(self);
        break;
    case 20:
        if (isCallback(self, 3) != 0) {
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            sendReqCircleInfoNoticeSet(self);
            break;
        }
        ((NetworkStateMachine*)self)->requestState_6135 += 10;
        break;
    case 30:
        if (((NetworkStateMachine*)self)->binaryActive_D400 != 0) {
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            sendReqBinaryHead(self, ((NetworkStateMachine*)self)->binaryActive_D400, 6);
            break;
        }
        ((NetworkStateMachine*)self)->requestState_6135 = 70;
        break;
    case 40:
        if (((NetworkStateMachine*)self)->binaryTextReady_D408 != 0 && ((NetworkStateMachine*)self)->dataTotal_825C != 0) {
            ((NetworkStateMachine*)self)->requestState_6135 += 5;
            sendReqBinaryData(self, ((NetworkStateMachine*)self)->binaryActive_D400, ((NetworkStateMachine*)self)->binarySize_D404, 0,
                              ((NetworkStateMachine*)self)->dataTotal_825C);
            break;
        }
        ((NetworkStateMachine*)self)->requestState_6135 += 10;
        break;
    case 50:
        ((NetworkStateMachine*)self)->requestState_6135 += 5;
        sendReqBinaryFoot(self, ((NetworkStateMachine*)self)->binaryActive_D400);
        break;
    case 60:
        if (((NetworkStateMachine*)self)->binaryTextReady_D408 != 0) {
            u32 i = 0;
            u32 index = 0;
            u32 j;
            u32 k;

            u32* tokens = ((NetworkStateMachine*)self)->binaryTokens_D60C;
            memset(((NetworkStateMachine*)self)->binaryTokens_D60C, 0, sizeof(((NetworkStateMachine*)self)->binaryTokens_D60C));
            for (j = 0; j < 8; j++) {
                while (((NetworkStateMachine*)self)->binaryText_D409[i] != '\t' && ((NetworkStateMachine*)self)->binaryText_D409[i] != 0) {
                    i++;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == 0) {
                    break;
                }
                i++;
                while (((NetworkStateMachine*)self)->binaryText_D409[i] == '\t') {
                    i++;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == '\r' || ((NetworkStateMachine*)self)->binaryText_D409[i] == '\n') {
                    ((NetworkStateMachine*)self)->binaryText_D409[i] = 0;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == 0) {
                    break;
                }
                tokens[index] = i;
                index++;
                i++;
                while (((NetworkStateMachine*)self)->binaryText_D409[i] != '\r' && ((NetworkStateMachine*)self)->binaryText_D409[i] != '\n' &&
                       ((NetworkStateMachine*)self)->binaryText_D409[i] != 0) {
                    i++;
                }
                while (((NetworkStateMachine*)self)->binaryText_D409[i] == '\r' || ((NetworkStateMachine*)self)->binaryText_D409[i] == '\n') {
                    ((NetworkStateMachine*)self)->binaryText_D409[i] = 0;
                    i++;
                }
                if (((NetworkStateMachine*)self)->binaryText_D409[i] == 0) {
                    break;
                }
            }
            for (k = index; k < 8; k++) {
                tokens[k] = i;
            }
        }
        ((NetworkStateMachine*)self)->requestState_6135 += 10;
        break;
    case 70:
        ((NetworkStateMachine*)self)->requestState_6135 += 5;
        reqUserSearchInfoMine(self, 0);
        break;
    case 80:
        ((NetworkStateMachine*)self)->requestState_6135 = 0;
        return 1;
    default:
        break;
    }
    return 0;
}
#pragma peephole on

/* ------------------------------------------------------------------------------------------------ */
/* the request emitters                                                                              */
/* ------------------------------------------------------------------------------------------------ */

s32 sendReqServerTime(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 2, 0);
    writeUInt32(st, st->loginDataCursor_6560);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqShut(NetworkInstance* self, s32 mode)
{
    s32 ret;

    ((NetworkStateMachine*)self)->shutdownFlag_6559 = 0;
    ((NetworkStateMachine*)self)->shutdownMode_6136 = mode;
    ret = flushBuffer(((NetworkStateMachine*)self), 4, 0);
    writeUInt8(((NetworkStateMachine*)self), mode);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqTicket(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 11, 0);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendServerTimeout(NetworkInstance* self, const u32* values)
{
    SessionTimeoutPayload payload;
    s32 ret;

    payload.code_00 = sessionTimeoutParam;
    payload.reason_02 = sessionTimeoutParam2;
    ret = flushBuffer(((NetworkStateMachine*)self), 17, 0);
    putItemTaggedLongs(((NetworkStateMachine*)self), values, 3, (const u8*)&payload);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqCommonKey(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 18, 0);
    encryptBuffer(st);
    return (u16)ret;
}

s32 sendReqUnknownCheck(NetworkInstance* self, const u8* tags, const u8* data, u32 size)
{
    u8 block[8];
    u32 count = 0;
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 20, 0);
    writeUInt8Array(((NetworkStateMachine*)self), data, size);
    if (tags != 0) {
        if (tags[0] != 0) {
            count = 1;
            block[0] = 1;
        }
        if (tags[1] != 0) {
            count = count + 1;
            block[count - 1] = 2;
        }
    }
    putItemTaggedBytes(((NetworkStateMachine*)self), tags, count, block);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqLoginInfo(NetworkInstance* self)
{
    u8 block[16];
    u32 count = 0;
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 23, 0);
    if (((NetworkStateMachine*)self)->loginInfoSent_82B4 == 0) {
        block[0] = 7;
        block[1] = 8;
        count = 3;
        block[2] = 9;
    } else if ((u8)(((NetworkStateMachine*)self)->loginInfoSent_82B4 + 255) <= 1) {
        block[0] = 6;
        count = 2;
        block[1] = 9;
    } else if (((NetworkStateMachine*)self)->loginInfoSent_82B4 == 3) {
        block[0] = 10;
        count = 2;
        block[1] = 9;
    }
    putItemAny(((NetworkStateMachine*)self), (u8*)((NetworkStateMachine*)self)->loginFields_82AC, count, block);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqChargeInfo(NetworkInstance* self, s32 mode)
{
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 25, 0);
    writeUInt8(((NetworkStateMachine*)self), mode);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqOpcode1B(NetworkInstance* self, u32 a, u32 b)
{
    u32 header[2];
    s32 ret;

    header[0] = requestHeaderWord0;
    header[1] = requestHeaderWord1;
    ret = flushBuffer(((NetworkStateMachine*)self), 27, 0);
    writeUInt32Shared(((NetworkStateMachine*)self), a);
    writeUInt32Shared(((NetworkStateMachine*)self), b);
    writeUInt8Array2(((NetworkStateMachine*)self), 8, (const u8*)header);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqUserListData(NetworkInstance* self, s32 mode, u32 count)
{
    s32 ret;

    ret = flushBuffer(((NetworkStateMachine*)self), 29, 0);
    writeUInt32Shared(((NetworkStateMachine*)self), mode);
    writeUInt32Shared(((NetworkStateMachine*)self), count);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 sendReqOpcode1F(NetworkInstance* self)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;
    s32 ret;

    ret = flushBuffer(st, 31, 0);
    encryptBuffer(st);
    return (u16)ret;
}

/* Sends one user's row, tagging every field that differs from the local row. */
s32 sendReqUserObject(NetworkInstance* self, s32 index, NetworkUserRow* row)
{
    u32 count = 0;
    NetworkUserRow* found;
    u8 tags[16];
    s32 flag;
    s32 ret;

    if (index < 0 || index >= (s32)((NetworkStateMachine*)self)->userRowCount_8BB4) {
        NetworkPostedError info;

        info.code_00 = -2147483648;
        info.param1_04 = 0;
        info.param2_08 = 0;
        ((PatInterface*)self)->postError(info);
        return -1;
    }
    found = (NetworkUserRow*)(((NetworkStateMachine*)self)->userRows_8BB8 + index * 92);
    if (strcmp(row->accountName_0C, found->accountName_0C) != 0) {
        count = 1;
        tags[0] = 3;
    }
    if (row->field_30 != found->field_30) {
        count = count + 1;
        tags[count - 1] = 5;
    }
    if (row->field_2C != found->field_2C) {
        count = count + 1;
        tags[count - 1] = 4;
    }
    if (row->field_34 != found->field_34) {
        count = count + 1;
        tags[count - 1] = 6;
    }
    if (row->field_38 != found->field_38) {
        count = count + 1;
        tags[count - 1] = 7;
    }
    if (strcmp(row->playerName_3C, found->playerName_3C) != 0) {
        count = count + 1;
        tags[count - 1] = 8;
    }
    if (strcmp(found->shortName_04, maskedUserName) == 0) {
        flag = 1;
    } else {
        flag = count == 0 ? 2 : 3;
    }
    ret = flushBuffer(((NetworkStateMachine*)self), 33, 0);
    writeBool(((NetworkStateMachine*)self), flag);
    writeUInt32Shared(((NetworkStateMachine*)self), (u32)(index + 1));
    putUserSlotObjects(((NetworkStateMachine*)self), (const u8*)row, count, tags);
    encryptBuffer(((NetworkStateMachine*)self));
    return (u16)ret;
}

s32 resetNetworkState4(NetworkInstance* self)
{
    ((NetworkStateMachine*)self)->fmpState_6137 = 0;
    return 0;
}

/* Drives the block-4 FMP list sub-machine (version, head, data, foot). */
s32 handleNetworkState4(NetworkInstance* self, s32 arg)
{
    NetworkStateMachine* st = (NetworkStateMachine*)self;

    switch (st->fmpState_6137) {
    case 0:
        if ((s32)st->serverType_65F0 != 0 && (s32)st->serverType_65F0 != 1) {
            return -st->fmpState_6137;
        }
        st->fmpState_6137 += 5;
        sendReqFmpListVersion(self);
        break;
    case 10:
        if (st->fmpListActive_6C38 == 0) {
            return -st->fmpState_6137;
        }
        if (st->fmpListReady_6C3C != 0) {
            st->serverCount_6604[1] = 0;
            memset(st->fmpSlots_6C40, 0, 5120);
        }
        st->replySent_8BCC = 0;
        st->fmpState_6137 += 5;
        sendReqFmpListHead(self, 1, arg);
        break;
    case 20:
        if ((s32)st->replyTotal_8BD0 > 0) {
            st->fmpState_6137 += 5;
        } else {
            st->fmpState_6137 += 20;
        }
        break;
    case 25:
    {
        u32 finished;
        u32 count;

        st->fmpState_6137 += 5;
        finished = st->replySent_8BCC;
        count = st->replyTotal_8BD0 - finished;
        sendReqFmpListData(self, finished + 1, (s32)count < 30 ? (s32)count : 30);
        break;
    }
    case 35:
        if ((s32)st->replySent_8BCC < 80 && (s32)(st->replyTotal_8BD0 - st->replySent_8BCC) > 0) {
            st->fmpState_6137 -= 10;
        } else {
            st->fmpState_6137 += 5;
        }
        break;
    case 40:
        st->fmpState_6137 += 5;
        sendReqFmpListFoot(self);
        break;
    case 50:
        st->serverIndex_65F4[1] = getFmpSlotIndex(st, st->fmpSelected_8040);
        return 1;
    default:
        break;
    }
    return 0;
}

/* ==== the packet layer ======================================================================================= */

#pragma peephole off

inline const char* PatInterface::getServerName()
{
    if (serverType_65F0 == 1) {
        return "FMP";
    }
    if (serverType_65F0 == 0) {
        return "LMP";
    }
    if (serverType_65F0 == 2) {
        return "OPN";
    }
    if (serverType_65F0 == 3) {
        return "RFP";
    }
    return "???";
}

/* The log line every handler starts with: the connected server's tag, the format's own arguments, a trailing "". */
#define PAT_TRACE(fmt)                                                   \
    {                                                                    \
        const char* serverName = getServerName();                        \
        getNetworkLogger()->signal_0C(2, fmt, serverName, "");           \
    }
#define PAT_TRACE_OPCODE(fmt)                                                                          \
    {                                                                                                  \
        const char* serverName = getServerName();                                                      \
        getNetworkLogger()->signal_0C(2, fmt, serverName, header->opcode_04[0], header->opcode_04[1], ""); \
    }

/* Keeps a generic failure as the pending error unless one is already kept. */
#define PAT_KEEP_FAILURE()                       \
    if (pendingError_654C.code_00 == 0) {        \
        pendingError_654C.code_00 = 0x80000000;  \
        pendingError_654C.param1_04 = 0;         \
        pendingError_654C.param2_08 = 0;         \
    }

/* ---- the request builders: each opens a request with its op-code, writes the items, seals it and returns the
 * request id the answer is matched against (narrowed to 16 bits) ------------------------------------------------- */

/* Keeps a generic failure as the error and reports it (the server type has no such request). */
#define PAT_POST_FAILURE(pat)                    \
    {                                            \
        NetworkPostedError error;                \
        error.code_00 = 0x80000000;              \
        error.param1_04 = 0;                     \
        error.param2_08 = 0;                     \
        (pat)->postError(error);                 \
    }

/* Requests the FMP list's version from the lobby or FMP server. */
u32 sendReqFmpListVersion(NetworkInstance* self)
{
    u32 id;

    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 35, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 79, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the FMP list's head: the list version, the range and the item tags (all of them while the list is stale). */
u32 sendReqFmpListHead(NetworkInstance* self, s32 first, s32 count)
{
    u8 tags[7] = { 1, 8, 9, 7, 10, 11, 12 };
    u8 tagCount;
    u32 id;

    tagCount = ((PatInterface*)self)->fmpListReady_6C3C != 0 ? 7 : 3;
    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 37, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 81, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    writeUInt32((PatInterface*)self, ((PatInterface*)self)->fmpListActive_6C38);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    writeUInt8Array2((PatInterface*)self, tagCount, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the FMP list. */
u32 sendReqFmpListData(NetworkInstance* self, u32 start, u32 count)
{
    u32 id;

    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 39, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 83, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    writeUInt32Shared((PatInterface*)self, start);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the FMP list request. */
u32 sendReqFmpListFoot(NetworkInstance* self)
{
    u32 id;

    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 41, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 85, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one FMP slot's details: two item tags when `brief`, else all nine. */
u32 sendReqFmpInfo(NetworkInstance* self, u32 value, s32 brief)
{
    u8 tags[9] = { 2, 3, 1, 8, 9, 7, 10, 11, 12 };
    u8 tagCount;
    u32 id;

    tagCount = brief == 0 ? 9 : 2;
    switch (((PatInterface*)self)->serverType_65F0) {
    case 0:
        id = flushBuffer((PatInterface*)self, 43, 0);
        break;
    case 1:
        id = flushBuffer((PatInterface*)self, 87, 0);
        break;
    default:
        PAT_POST_FAILURE((PatInterface*)self);
        return -1;
    }
    writeUInt32((PatInterface*)self, value);
    writeUInt8Array2((PatInterface*)self, tagCount, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the RFP server's address. */
u32 sendReqRfpConnect(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 45, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the lobby server's address. */
u32 sendReqLmpConnect(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 47, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the terms version. */
u32 sendReqTermsVersion(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 49, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one slice of the terms body. */
u32 sendReqTerms(NetworkInstance* self, u32 a, u32 b, u32 len)
{
    u32 id = flushBuffer((PatInterface*)self, 51, 0);
    writeUInt32((PatInterface*)self, a);
    writeUInt32((PatInterface*)self, b);
    writeUInt32((PatInterface*)self, len);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the maintenance text. */
u32 sendReqMaintenance(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 53, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the announcement text. */
u32 sendReqAnnounce(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 55, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the no-charge text; returns 0, not the request id. */
u32 sendReqNoCharge(NetworkInstance* self)
{
    flushBuffer((PatInterface*)self, 57, 0);
    encryptBuffer((PatInterface*)self);
    return 0;
}

/* Requests the media version. */
u32 sendReqMediaVersionInfo(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 59, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the low-level vulgarity list's slice descriptor. */
u32 sendReqVulgarityInfoLow(NetworkInstance* self, s32 mode)
{
    u32 id = flushBuffer((PatInterface*)self, 65, 0);
    writeUInt32((PatInterface*)self, mode);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one slice of the low-level vulgarity list. */
u32 sendReqVulgarityLow(NetworkInstance* self, s32 mode, u32 slice, u32 offset, u32 length)
{
    u32 id = flushBuffer((PatInterface*)self, 67, 0);
    writeUInt32((PatInterface*)self, mode);
    writeUInt32((PatInterface*)self, slice);
    writeUInt32((PatInterface*)self, offset);
    writeUInt32((PatInterface*)self, length);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends the authentication token. */
u32 sendReqAuthenticationToken(NetworkInstance* self, const char* token)
{
    u32 id = flushBuffer((PatInterface*)self, 69, 0);
    writeString((PatInterface*)self, token);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a file's checksum. */
s32 sendReqBinaryChecksum(NetworkInstance* self, u8 fileId)
{
    u32 id = flushBuffer((PatInterface*)self, 71, 0);
    writeUInt8((PatInterface*)self, fileId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a file's head; `mode` selects how the answers are consumed. */
s32 sendReqBinaryHead(NetworkInstance* self, u8 fileId, s8 mode)
{
    u32 id;

    ((PatInterface*)self)->binaryMode_611A = mode;
    id = flushBuffer((PatInterface*)self, 73, 0);
    writeUInt8((PatInterface*)self, fileId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests one chunk of a file. */
s32 sendReqBinaryData(NetworkInstance* self, u8 fileId, u32 handle, u32 offset, u32 size)
{
    u32 id = flushBuffer((PatInterface*)self, 75, 0);
    writeUInt8((PatInterface*)self, fileId);
    writeUInt32((PatInterface*)self, handle);
    writeUInt32((PatInterface*)self, offset);
    writeUInt32((PatInterface*)self, size);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends a file transfer. */
s32 sendReqBinaryFoot(NetworkInstance* self, u8 fileId)
{
    u32 id = flushBuffer((PatInterface*)self, 77, 0);
    writeUInt8((PatInterface*)self, fileId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Starts the layer session, asking for the layer and user items. */
u32 sendReqLayerStart(NetworkInstance* self)
{
    u32 id;

    ((PatInterface*)self)->layerMovePending_D62C = 0;
    id = flushBuffer((PatInterface*)self, 89, 0);
    writeLayerItemRequest((PatInterface*)self);
    writeUserItemRequest((PatInterface*)self);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the layer session. */
u32 sendReqLayerEnd(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 91, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a jump to the layer at `path`. */
u32 sendReqLayerJump(NetworkInstance* self, const u8* path, u32 value)
{
    u32 id = flushBuffer((PatInterface*)self, 94, 0);
    writeUnkShortArray((PatInterface*)self, path);
    writeUInt32((PatInterface*)self, value);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Starts creating a layer under `layerId`. */
u32 sendReqLayerCreateHead(NetworkInstance* self, s16 layerId)
{
    u32 id = flushBuffer((PatInterface*)self, 96, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends the new layer's settings (its name only when it has one) and tags. */
u32 sendReqLayerCreateSet(NetworkInstance* self, s16 layerId, PatLayerData* layer, PatTagList* tags)
{
    u8 items[3] = { 9, 10, 3 };
    u8 itemCount;
    u32 id;

    itemCount = 3;
    if (layer->name_014[0] == 0) {
        itemCount = 2;
    }
    id = flushBuffer((PatInterface*)self, 98, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    writeLayerDownData((PatInterface*)self, layer, itemCount, items);
    writeUnkByteIntStruct((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends creating a layer; without `move` no layer move stays pending. */
u32 sendReqLayerCreateFoot(NetworkInstance* self, s16 layerId, u8 move)
{
    u32 id;

    if (move == 0) {
        ((PatInterface*)self)->layerMovePending_D62C = 0;
    }
    id = flushBuffer((PatInterface*)self, 100, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    writeUInt8((PatInterface*)self, move);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a move down into the layer `layerId`, sending the layer's item 12. */
u32 sendReqLayerDown(NetworkInstance* self, s16 layerId, PatLayerData* layer)
{
    u8 items[1] = { 12 };
    u32 id = flushBuffer((PatInterface*)self, 102, 0);
    writeShortPlusOne((PatInterface*)self, layerId);
    writeLayerDownData((PatInterface*)self, layer, 1, items);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a move up a layer. */
u32 sendReqLayerUp(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 105, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Tells the server the jump to `path` is ready. */
u32 sendReqLayerJumpReady(NetworkInstance* self, const u8* path, u32 value)
{
    u32 id = flushBuffer((PatInterface*)self, 108, 0);
    writeUnkShortArray((PatInterface*)self, path);
    writeUInt32((PatInterface*)self, value);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Tells the server to go ahead with the jump to `path`. */
u32 sendReqLayerJumpGo(NetworkInstance* self, const u8* path, u32 value)
{
    u32 id = flushBuffer((PatInterface*)self, 110, 0);
    writeUnkShortArray((PatInterface*)self, path);
    writeUInt32((PatInterface*)self, value);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends the layer's changed settings (item 23 when it carries the extra value) and tags. */
u32 sendReqLayerInfoSet(NetworkInstance* self, PatLayerData* layer, PatTagList* tags)
{
    u8 items[24];
    u8 itemCount;
    u32 id;

    itemCount = 0;
    if (layer->value_23E != 0) {
        items[itemCount++] = 23;
    }
    id = flushBuffer((PatInterface*)self, 112, 0);
    writeLayerDownData((PatInterface*)self, layer, itemCount, items);
    writeUnkByteIntStruct((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the layer at `path`'s settings: the user count items for mode 4, the layer items for mode 3. */
u32 sendReqLayerInfo(NetworkInstance* self, const u8* path, const PatTagList* tags, s8 mode)
{
    u32 id = flushBuffer((PatInterface*)self, 115, 0);
    writeUnkShortArray((PatInterface*)self, path);
    if (mode == 4) {
        u8 items[2] = { 0x15, 0x16 };
        writeUInt8Array2((PatInterface*)self, 2, items);
    } else if (mode == 3) {
        writeLayerItemRequest((PatInterface*)self);
    }
    writeUnk2ByteArray((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the parent layer's settings with the layer items. */
u32 sendReqLayerParentInfo(NetworkInstance* self, const PatTagList* tags)
{
    u32 id = flushBuffer((PatInterface*)self, 117, 0);
    writeLayerItemRequest((PatInterface*)self);
    writeUnk2ByteArray((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the child layer list's head with the layer items. */
u32 sendReqLayerChildListHead(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 121, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    writeLayerItemRequest((PatInterface*)self);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the child layer list. */
u32 sendReqLayerChildListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 123, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the child layer list request. */
u32 sendReqLayerChildListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 125, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the sibling layer list's head with the layer items. */
u32 sendReqLayerSiblingListHead(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 127, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    writeLayerItemRequest((PatInterface*)self);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the sibling layer list. */
u32 sendReqLayerSiblingListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 129, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the sibling layer list request. */
u32 sendReqLayerSiblingListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 131, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the host of the layer at `path`. */
u32 sendReqLayerHost(NetworkInstance* self, const u8* path)
{
    u32 id = flushBuffer((PatInterface*)self, 133, 0);
    writeUnkShortArray((PatInterface*)self, path);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the layer's users with five user items. */
u32 sendReqLayerUserList(NetworkInstance* self)
{
    u8 items[5] = { 1, 2, 3, 6, 7 };
    u32 id = flushBuffer((PatInterface*)self, 139, 0);
    writeUInt8Array2((PatInterface*)self, 5, items);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the layer user list. */
u32 sendReqLayerUserListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 143, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the layer user list request. */
u32 sendReqLayerUserListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 145, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the layer user search. */
u32 sendReqLayerUserSearchData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 149, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the layer user search request. */
u32 sendReqLayerUserSearchFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 151, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests leaving the circle. */
u32 sendReqCircleLeave(NetworkInstance* self, s32 circleId)
{
    u32 id = flushBuffer((PatInterface*)self, 187, 0);
    writeUInt32Shared((PatInterface*)self, circleId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the circle's match start. */
u32 sendReqCircleMatchStart(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 198, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the circle's match end. */
u32 sendReqCircleMatchEnd(NetworkInstance* self, s32 mode)
{
    u32 id = flushBuffer((PatInterface*)self, 201, 0);
    writeUInt8((PatInterface*)self, mode);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the circle list. */
u32 sendReqCircleListData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 210, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the circle list request. */
u32 sendReqCircleListFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 212, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests kicking `userId` from the circle with an empty kick list. */
u32 sendReqCircleKick(NetworkInstance* self, const char* userId)
{
    PatKickList kick;
    u32 id = flushBuffer((PatInterface*)self, 214, 0);
    writeString((PatInterface*)self, userId);
    memset(&kick, 0, sizeof(PatKickList));
    kick.kind_000 = 1;
    writeUInt8((PatInterface*)self, 1);
    writeUInt8Array((PatInterface*)self, kick.data_002, kick.size_102);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the circle's host. */
u32 sendReqCircleHost(NetworkInstance* self, s32 circleId)
{
    u32 id = flushBuffer((PatInterface*)self, 222, 0);
    writeUInt32Shared((PatInterface*)self, circleId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends this user's search tags. */
u32 sendReqUserSearchSet(NetworkInstance* self, PatTagList* tags)
{
    u32 id = flushBuffer((PatInterface*)self, 252, 0);
    writeUnkByteIntStruct((PatInterface*)self, tags);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends this user's binary with its value. */
u32 sendReqUserBinarySet(NetworkInstance* self, u32 value, const u8* data, u32 size)
{
    u32 id = flushBuffer((PatInterface*)self, 254, 0);
    writeUInt32((PatInterface*)self, value);
    writeUInt8Array((PatInterface*)self, data, size);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Sends a binary notice: its kind, an optional text (an empty one when null) and two values. */
u32 sendReqUserBinaryNotice(NetworkInstance* self, u8 kind, const char* text, u32 a, u32 b)
{
    u32 id = flushBuffer((PatInterface*)self, 256, 0);
    writeUInt8((PatInterface*)self, kind);
    if (text != NULL) {
        writeString((PatInterface*)self, text);
    } else {
        writeUInt16((PatInterface*)self, 0);
    }
    writeUInt32((PatInterface*)self, a);
    writeUInt32((PatInterface*)self, b);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of the user search. */
u32 sendReqUserSearchData(NetworkInstance* self, s32 first, s32 count)
{
    u32 id = flushBuffer((PatInterface*)self, 261, 0);
    writeUInt32Shared((PatInterface*)self, first);
    writeUInt32Shared((PatInterface*)self, count);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Ends the user search request. */
u32 sendReqUserSearchFoot(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 263, 0);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Answers a friend request (GUESS name: op-code 276, after the friend request 273). */
u32 sendReqFriendAccept(NetworkInstance* self, const char* userId, u8 accept)
{
    u32 id = flushBuffer((PatInterface*)self, 276, 0);
    writeString((PatInterface*)self, userId);
    writeUInt8((PatInterface*)self, accept);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Removes a friend (GUESS name: op-code 279). */
u32 sendReqFriendDelete(NetworkInstance* self, const char* userId)
{
    u32 id = flushBuffer((PatInterface*)self, 279, 0);
    writeString((PatInterface*)self, userId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Removes a user from the black list (GUESS name: op-code 285). */
u32 sendReqBlackDelete(NetworkInstance* self, const char* userId)
{
    u32 id = flushBuffer((PatInterface*)self, 285, 0);
    writeString((PatInterface*)self, userId);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests a range of a reflect channel. */
u32 sendReqChannelData(NetworkInstance* self, u32 handle, u32 offset, u32 size)
{
    u32 id = flushBuffer((PatInterface*)self, 293, 0);
    writeUInt8((PatInterface*)self, ((PatInterface*)self)->warningKind_8BBC);
    writeUInt8((PatInterface*)self, handle);
    writeUInt32((PatInterface*)self, offset);
    writeUInt32((PatInterface*)self, size);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Requests the reflect connection. */
u32 sendReqConnect(NetworkInstance* self)
{
    u32 id = flushBuffer((PatInterface*)self, 295, 0);
    writeUInt8((PatInterface*)self, ((PatInterface*)self)->warningKind_8BBC);
    encryptBuffer((PatInterface*)self);
    return (u16)id;
}

/* Logs a packet whose op-code has no table entry. */
s32 PatInterface::recvNotProvided(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE_OPCODE("%s:recvNotProvided[%02Xx%02X]\n");
    return -1;
}

/* Keeps a negative reply's code and message as the error record unless an error is pending. */
s32 PatInterface::recvAnsNg(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE_OPCODE("%s:recvAnsNg[%02Xx%02X]\n");
    if (pendingError_654C.code_00 == 0) {
        readInt32(this, &errorRecord_613C.code_000);
        readString(this, &length, errorRecord_613C.message_008, 512);
        errorRecord_613C.index_004 = index;
    }
    return -1;
}

/* Reads an alert, keeps it as the error record unless an error is pending, and reports it to the session handlers. */
s32 PatInterface::recvAnsAlert(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatErrorRecord alert;

    PAT_TRACE_OPCODE("%s:recvAnsAlert[%02Xx%02X]\n");
    readInt32(this, &alert.code_000);
    readString(this, &length, alert.message_008, 512);
    alert.index_004 = index;
    if (pendingError_654C.code_00 == 0) {
        memcpy(&errorRecord_613C, &alert, sizeof(PatErrorRecord));
    }
    dispatchSessionHandlers(this, 0x8002, header->requestId_02, header->status_07, 1, (const u8*)&alert);
    return -3;
}

/* Answers the server's line check with an empty request. */
s32 PatInterface::recvReqLineCheck(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvReqLineCheck ok\n");
    flushBuffer(this, 1, 0);
    encryptBuffer(this);
    return 0;
}

/* Takes the server's game and date time and re-bases the clock on them. */
s32 PatInterface::recvAnsServerTime(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsServerTime ok\n");
    readUInt32_(this, &gameTimeBase_6128);
    readUInt32_(this, &serverTime_612C);
    clockAtSync_6120 = clock_611C;
    if (requestState_6135 == 15 || requestState_6135 == 85) {
        requestState_6135 += 5;
    }
    return 0;
}

/* Reads the shutdown answer and advances the session, or hands it to the session handlers. */
s32 PatInterface::recvAnsShut(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsShut ok\n");
    readUInt8(this, &value);
    if (serverType_65F0 == 2) {
        sessionState_6132 += 5;
    } else {
        dispatchSessionHandlers(this, 0x8006, header->requestId_02, header->status_07, 1, &value);
    }
    return 0;
}

/* Keeps the server's shutdown notice (its code and message) in the shutdown record. */
s32 PatInterface::recvNtcShut(s32 index, const PatPacketHeader* header)
{
    u8 code;
    u32 length;

    PAT_TRACE("%s:recvNtcShut ok\n");
    readUInt8(this, &code);
    readString(this, &length, shutdownRecord_6344.message_008, 512);
    shutdownRecord_6344.code_000 = code;
    shutdownRecord_6344.index_004 = index;
    return -2;
}

/* Takes the address and port to reconnect to; only the opening server may send it. */
s32 PatInterface::recvNtcRecconect(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvNtcRecconect ok\n");
    if (serverType_65F0 != 2) {
        PAT_KEEP_FAILURE();
        return -1;
    }
    memset(&serverAddress_6A2C, 0, sizeof(PatServerAddress));
    readString(this, &length, serverAddress_6A2C.host_000, 256);
    readUInt16(this, &serverAddress_6A2C.port_104);
    flag_6558 = 0;
    sessionState_6132 = 1;
    return 0;
}

/* Answers the connection request with the login block's tagged items. */
s32 PatInterface::recvReqConnection(s32 index, const PatPacketHeader* header)
{
    u32 value;
    u8 count;
    PatTicket* ticket;

    PAT_TRACE("%s:recvReqConnection ok\n");
    readUInt32_(this, &value);
    flag_6558 = 1;
    u8 tags[10] = { 3, 4, 5, 6, 7, 8 };
    count = 6;
    if (loginInfoSent_82B4 == 0 || reflectName5C_65C8[0] != 0) {
        tags[count++] = 1;
    }
    ticket = ticket_8BB0;
    if (ticket != NULL && ticket->size_400 != 0) {
        tags[count++] = 2;
    }
    tags[count++] = 9;
    tags[count++] = 10;
    flushBuffer(this, 9, 0);
    putSomethingList(this, (u8*)loginFields_82AC, count, tags);
    encryptBuffer(this);
    return 0;
}

/* Reads the login state for the connected server and advances that server's state machine. */
s32 PatInterface::recvNtcLogin(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvNtcLogin ok\n");
    if (serverType_65F0 == 2) {
        readUInt8(this, &patState_8254);
        sessionState_6132 += 5;
    } else if ((u32)serverType_65F0 <= 1) {
        readUInt8(this, &fmpPhase_894D);
        requestState_6135 += 5;
    } else if (serverType_65F0 == 3) {
        readUInt8(this, &fmpPhase_894D);
        subState_6133 += 5;
    } else {
        PAT_KEEP_FAILURE();
        return -1;
    }
    return 0;
}

/* Reads the login ticket into the caller's buffer. */
s32 PatInterface::recvAnsTicket(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsTicket ok\n");
    if (ticket_8BB0 != NULL) {
        readUInt8Array(this, &length, ticket_8BB0->data_000, 1024);
        ticket_8BB0->size_400 = (u16)length;
    }
    requestState_6135 += 5;
    return 0;
}

/* Reads the login ticket and acknowledges it. */
s32 PatInterface::recvReqTicket(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvReqTicket ok\n");
    if (ticket_8BB0 != NULL) {
        readUInt8Array(this, &length, ticket_8BB0->data_000, 1024);
        ticket_8BB0->size_400 = (u16)length;
    }
    flushBuffer(this, 14, 0);
    encryptBuffer(this);
    return 0;
}

/* Reads a warning (kind, value, text into the reply buffer) and acknowledges it. */
s32 PatInterface::recvReqWarning(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvReqWarning ok\n");
    readUInt8(this, &warningKind_8BBC);
    readUInt32_(this, &warningValue_8BC0);
    if (replySize_8BC4 != 0) {
        readString(this, &length, (char*)replyBuffer_8BC8, replySize_8BC4 < 1024 ? replySize_8BC4 : 1024);
    }
    flushBuffer(this, 16, 0);
    encryptBuffer(this);
    requestState_6135 = 230;
    return 0;
}

/* Reads the 256-byte common key into its buffer. */
s32 PatInterface::recvAnsCommonKey(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsCommonKey ok\n");
    if (commonKeyBuffer_8948 != NULL) {
        memset(commonKeyBuffer_8948, 0, 256);
        readUInt8Array(this, &length, commonKeyBuffer_8948, 256);
        commonKeyReady_894C = 1;
    }
    sessionState_6132 += 5;
    return 0;
}

/* Reads the login answer: the session flag, its message and the charge items. */
s32 PatInterface::recvAnsLoginInfo(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsLoginInfo ok\n");
    readUInt8(this, &sessionReady_8950);
    readString(this, &length, termText_8951, 512);
    readChargeInfo(this, (u8*)loginFields_82AC);
    connectionPhase_894F = sessionReady_8950;
    requestState_6135 += 5;
    return 0;
}

/* Reads the charge items of the login block. */
s32 PatInterface::recvAnsChargeInfo(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsChargeInfo ok\n");
    readChargeInfo(this, (u8*)loginFields_82AC);
    requestState_6135 += 5;
    return 0;
}

/* Reads the user list's head: the number of rows to come. */
s32 PatInterface::recvAnsUserListHead(s32 index, const PatPacketHeader* header)
{
    s32 first;

    PAT_TRACE("%s:recvAnsUserListHead ok\n");
    readInt32(this, &first);
    readInt32(this, (s32*)&userRowCount_8BB4);
    requestState_6135 += 5;
    return 0;
}

/* Reads the user rows, shrinking the row stack to the count the server sends. */
s32 PatInterface::recvAnsUserListData(s32 index, const PatPacketHeader* header)
{
    s32 count;
    s32 first;

    PAT_TRACE("%s:recvAnsUserListData ok\n");
    readInt32(this, &first);
    readInt32(this, &count);
    if ((s32)userRowCount_8BB4 > count) {
        growStackSize(this, (userRowCount_8BB4 - count) * 92);
        userRowCount_8BB4 = count;
    }
    memset(userRows_8BB8, 0, userRowCount_8BB4 * 92);
    readUserObjects(this, (NetworkUserRow*)userRows_8BB8, userRowCount_8BB4);
    requestState_6135 += 5;
    return 0;
}

/* Ends the user list. */
s32 PatInterface::recvAnsUserListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserListFoot ok\n");
    requestState_6135 += 5;
    return 0;
}

/* Reads this client's own user row with the session flag and message. */
s32 PatInterface::recvAnsUserObject(s32 index, const PatPacketHeader* header)
{
    s32 value;
    u32 length;

    PAT_TRACE("%s:recvAnsUserObject ok\n");
    readUInt8(this, &sessionReady_8950);
    readString(this, &length, termText_8951, 512);
    readInt32(this, &value);
    readUserObjects(this, &userRow_8B54, 1);
    requestState_6135 += 5;
    return 0;
}

/* Reads the FMP list version and marks the list stale when it changed. */
s32 PatInterface::recvAnsFmpListVersion(s32 index, const PatPacketHeader* header)
{
    u32 version;

    PAT_TRACE("%s:recvAnsFmpListVersion ok\n");
    readUInt32_(this, &version);
    fmpListReady_6C3C = fmpListActive_6C38 != version;
    fmpListActive_6C38 = version;
    fmpState_6137 += 5;
    return 0;
}

/* Reads the FMP list's head: the number of slots to come. */
s32 PatInterface::recvAnsFmpListHead(s32 index, const PatPacketHeader* header)
{
    s32 first;

    PAT_TRACE("%s:recvAnsFmpListHead ok\n");
    readInt32(this, &first);
    readInt32(this, (s32*)&replyTotal_8BD0);
    fmpState_6137 += 5;
    return 0;
}

/* Reads FMP slots: the whole run into the table while the list is stale, else only the progress of known slots. */
s32 PatInterface::recvAnsFmpListData(s32 index, const PatPacketHeader* header)
{
    s32 count;
    s32 first;
    NetworkFmpSlot slot;
    u8 reserve[262];
    s32 i;
    s32 j;

    PAT_TRACE("%s:recvAnsFmpListData ok\n");
    readInt32(this, &first);
    readInt32(this, &count);
    if (count > (s32)(80 - replySent_8BCC)) {
        count = 80 - replySent_8BCC;
    }
    if (fmpListReady_6C3C != 0) {
        readFmpCompoundData(this, &fmpSlots_6C40[replySent_8BCC], reserve, count);
        serverCount_6604[1] += count;
    } else {
        for (i = 0; i < count; i++) {
            memset(&slot, 0, sizeof(NetworkFmpSlot));
            readFmpCompoundData(this, &slot, reserve, 1);
            if (slot.payload_00 != 0) {
                for (j = 0; j < (s32)serverCount_6604[1]; j++) {
                    if (slot.payload_00 == fmpSlots_6C40[j].payload_00) {
                        fmpSlots_6C40[j].done_10 = slot.done_10;
                        fmpSlots_6C40[j].total_14 = slot.total_14;
                        break;
                    }
                }
            }
        }
    }
    replySent_8BCC += count;
    fmpState_6137 += 5;
    return 0;
}

/* Ends the FMP list. */
s32 PatInterface::recvAnsFmpListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFmpListFoot ok\n");
    fmpState_6137 += 5;
    return 0;
}

/* Reads the chosen FMP slot's details and reports them to the session handlers. */
s32 PatInterface::recvAnsFmpInfo(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFmpInfo ok\n");
    memset(serverReserve_6614[1], 0, sizeof(serverReserve_6614[1]));
    readFmpCompoundData(this, &fmpSlots_6C40[serverIndex_65F4[1]], serverReserve_6614[1], 1);
    dispatchSessionHandlers(this, 0x8008, header->requestId_02, header->status_07, 0, NULL);
    requestState_6135 += 5;
    return 0;
}

/* Reads the RFP server's address. */
s32 PatInterface::recvAnsRfpConnect(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsRfpConnect ok\n");
    memset(&rfpServer_814E, 0, sizeof(PatServerAddress));
    readString(this, &length, rfpServer_814E.host_000, 256);
    readUInt16(this, &rfpServer_814E.port_104);
    serverCount_6604[3] = 1;
    requestState_6135 += 5;
    return 0;
}

/* Reads the lobby server's address. */
s32 PatInterface::recvAnsLmpConnect(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsLmpConnect ok\n");
    memset(&lmpServer_6B32, 0, sizeof(PatServerAddress));
    readString(this, &length, lmpServer_6B32.host_000, 256);
    readUInt16(this, &lmpServer_6B32.port_104);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the terms version (noting whether it changed) and the size of the terms body. */
s32 PatInterface::recvAnsTermsVersion(s32 index, const PatPacketHeader* header)
{
    u32 version;

    PAT_TRACE("%s:recvAnsTermsVersion ok\n");
    readUInt32_(this, &version);
    termsChanged_8268 = termsVersion_8264 != version;
    termsVersion_8264 = version;
    readUInt32_(this, &dataTotal_825C);
    sessionState_6132 += 5;
    return 0;
}

/* Reads one slice of the terms body. */
s32 PatInterface::recvAnsTerms(s32 index, const PatPacketHeader* header)
{
    s32 result;

    PAT_TRACE("%s:recvAnsTerms ok\n");
    result = readBodySlice(this, termsBufferPtr_828C, termsSize_826C);
    if (result < 0) {
        return result;
    }
    sessionState_6132 += 5;
    return 0;
}

/* Reads the maintenance text into its buffer. */
s32 PatInterface::recvAnsMaintenance(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsMaintenance ok\n");
    readString(this, &length, (char*)maintenanceBuffer_8290, maintenanceSize_8270);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the announcement text into its buffer. */
s32 PatInterface::recvAnsAnnounce(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsAnnounce ok\n");
    readString(this, &length, (char*)announceBuffer_8294, announceSize_8274);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the no-charge text into its buffer. */
s32 PatInterface::recvAnsNoCharge(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsNoCharge ok\n");
    readString(this, &length, (char*)noChargeBuffer_8298, noChargeSize_8278);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the media version, the patch message and the media version records. */
s32 PatInterface::recvAnsMediaVersionInfo(s32 index, const PatPacketHeader* header)
{
    u32 length;

    PAT_TRACE("%s:recvAnsMediaVersionInfo ok\n");
    readString(this, &length, mediaVersion_6588, 32);
    readString(this, &length, (char*)patchMessageBuffer_82A8, patchMessageSize_8288);
    memset(mediaVersionText_65A8, 0, 32);
    readMediaVersionData(this, NULL, 1);
    sessionState_6132 += 5;
    return 0;
}

/* Reads the high-level vulgarity list's slice descriptor and size. */
s32 PatInterface::recvAnsVulgarityInfoHigh(s32 index, const PatPacketHeader* header)
{
    u32 value;

    PAT_TRACE("%s:recvAnsVulgarityInfoHigh ok\n");
    readUInt32_(this, &value);
    readUInt32_(this, &sendSlice_8258);
    readUInt32_(this, &dataTotal_825C);
    sessionState_6132 += 5;
    return 0;
}

/* Reads one slice of the high-level vulgarity list. */
s32 PatInterface::recvAnsVulgarityHigh(s32 index, const PatPacketHeader* header)
{
    s32 result;
    u32 mode;

    PAT_TRACE("%s:recvAnsVulgarityHigh ok\n");
    readUInt32_(this, &mode);
    if (mode == 2) {
        result = readBodySlice(this, vulgarityHighBuffer_829C, vulgarityHighSize_827C);
        if (result < 0) {
            return result;
        }
    } else {
        result = readBodySlice(this, vulgarityHighBuffer_829C, vulgarityHighSize_827C);
        if (result < 0) {
            return result;
        }
    }
    sessionState_6132 += 5;
    return 0;
}

/* Reads the low-level vulgarity list's slice descriptor and size. */
s32 PatInterface::recvAnsVulgarityInfoLow(s32 index, const PatPacketHeader* header)
{
    u32 value;

    PAT_TRACE("%s:recvAnsVulgarityInfoLow ok\n");
    readUInt32_(this, &value);
    readUInt32_(this, &sendSlice_8258);
    readUInt32_(this, &dataTotal_825C);
    sessionState_6132 += 5;
    return 0;
}

/* Reads one slice of the low-level vulgarity list into the buffer its mode selects. */
s32 PatInterface::recvAnsVulgarityLow(s32 index, const PatPacketHeader* header)
{
    s32 result;
    u32 mode;

    PAT_TRACE("%s:recvAnsVulgarityLow ok\n");
    readUInt32_(this, &mode);
    if (mode == 2) {
        result = readBodySlice(this, vulgarityPtr_82A4, vulgaritySize_8284);
        if (result < 0) {
            return result;
        }
    } else {
        result = readBodySlice(this, userListPtr_82A0, userListSize_8280);
        if (result < 0) {
            return result;
        }
    }
    sessionState_6132 += 5;
    return 0;
}

/* Acknowledges the authentication token. */
s32 PatInterface::recvAnsAuthenticationToken(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsAuthenticationToken ok\n");
    sessionState_6132 += 5;
    return 0;
}

/* Reads a file's binary version and reports it to the session handlers. */
s32 PatInterface::recvAnsBinaryVersion(s32 index, const PatPacketHeader* header)
{
    u32 version;
    u8 fileId;

    PAT_TRACE("%s:recvAnsBinaryVersion ok\n");
    readUInt8(this, &fileId);
    readUInt32_(this, &version);
    dispatchSessionHandlers(this, 0x8009, header->requestId_02, header->status_07, 1, (const u8*)&version);
    return 0;
}

/* Reads a binary transfer's head: reported to the session handlers, or kept as the text transfer's size. */
s32 PatInterface::recvAnsBinaryHead(s32 index, const PatPacketHeader* header)
{
    u32 values[2];

    PAT_TRACE("%s:recvAnsBinaryHead ok\n");
    readUInt32_(this, &values[0]);
    readUInt32_(this, &values[1]);
    switch (binaryMode_611A) {
    case 5:
        dispatchSessionHandlers(this, 0x800A, header->requestId_02, header->status_07, 2, (const u8*)values);
        break;
    case 6:
        binaryTextReady_D408 = binarySize_D404 != values[0];
        binarySize_D404 = values[0];
        dataTotal_825C = values[1];
        requestState_6135 += 5;
        break;
    }
    return 0;
}

/* Reads one binary chunk onto the call stack: reported to the session handlers, or copied into the text buffer. */
s32 PatInterface::recvAnsBinaryData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 length;
    PatBinaryChunk chunk;

    PAT_TRACE("%s:recvAnsBinaryData ok\n");
    readUInt32_(this, &chunk.fileHandle_0);
    readUInt32_(this, &chunk.offset_4);
    readUInt32_(this, &chunk.size_8);
    chunk.data_C = createStack(this, chunk.size_8, &stackSize);
    if (chunk.size_8 > stackSize) {
        chunk.size_8 = stackSize;
    }
    readUInt8Array(this, &length, chunk.data_C, chunk.size_8);
    switch (binaryMode_611A) {
    case 5:
        dispatchSessionHandlers(this, 0x800B, header->requestId_02, header->status_07, 1, (const u8*)&chunk);
        break;
    case 6:
        memset(binaryText_D409, 0, 512);
        if (chunk.offset_4 == 0) {
            binarySize_D404 = chunk.fileHandle_0;
            memcpy(binaryText_D409, chunk.data_C, chunk.size_8 < 511 ? chunk.size_8 : 511);
        }
        requestState_6135 += 5;
        break;
    }
    growStackSize(this, stackSize);
    return 0;
}

/* Ends a binary transfer. */
s32 PatInterface::recvAnsBinarFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBinaryFoot ok\n");
    switch (binaryMode_611A) {
    case 5:
        dispatchSessionHandlers(this, 0x800C, header->requestId_02, header->status_07, 0, NULL);
        break;
    case 6:
        requestState_6135 += 5;
        break;
    }
    return 0;
}

/* Reads the layer the session starts in and reports it. */
s32 PatInterface::recvAnsLayerStart(s32 index, const PatPacketHeader* header)
{
    PatLayerData layer;

    PAT_TRACE("%s:recvAnsLayerStart\n");
    memset(&layer, 0, sizeof(PatLayerData));
    readLayerData(this, &layer, 1);
    dispatchSessionHandlers(this, 0x800D, header->requestId_02, header->status_07, 1, (const u8*)&layer);
    return 0;
}

/* Reports the end of the layer session. */
s32 PatInterface::recvAnsLayerEnd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerEnd\n");
    dispatchSessionHandlers(this, 0x800E, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer's user count notice and reports it. */
s32 PatInterface::recvNtcLayerUserNum(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvNtcLayerUserNum\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readUInt8(this, &info.kind_000);
    readLayerCountData(this, &info.layer_004, 1);
    dispatchSessionHandlers(this, 0x800F, NULL, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reports the answer to a layer jump. */
s32 PatInterface::recvAnsLayerJump(s32 index, const PatPacketHeader* header)
{
    layerMovePending_D62C = 0;
    PAT_TRACE("%s:recvAnsLayerJump\n");
    dispatchSessionHandlers(this, 0x8010, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the created layer's number and reports the head of a layer creation. */
s32 PatInterface::recvAnsLayerCreateHead(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    PAT_TRACE("%s:recvAnsLayerCreateHead\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8011, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer's number and reports a layer creation's settings. */
s32 PatInterface::recvAnsLayerCreateSet(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    PAT_TRACE("%s:recvAnsLayerCreateSet\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8012, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer's number and reports the end of a layer creation. */
s32 PatInterface::recvAnsLayerCreateFoot(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    PAT_TRACE("%s:recvAnsLayerCreateFoot\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8013, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer moved down into and reports it. */
s32 PatInterface::recvAnsLayerDown(s32 index, const PatPacketHeader* header)
{
    s16 layerId;

    layerMovePending_D62C = 0;
    PAT_TRACE("%s:recvAnsLayerDown\n");
    readShortMinusOne(this, &layerId);
    dispatchSessionHandlers(this, 0x8014, header->requestId_02, header->status_07, 1, (const u8*)&layerId);
    return 0;
}

/* Reads the user who entered the layer and reports it. */
s32 PatInterface::recvNtcLayerIn(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUser user;

    PAT_TRACE("%s:recvNtcLayerIn\n");
    memset(&user, 0, sizeof(PatLayerUser));
    readString(this, &length, user.userId_000, 8);
    readLayerUserData(this, &user, 1);
    dispatchSessionHandlers(this, 0x8015, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reports the answer to moving up a layer. */
s32 PatInterface::recvAnsLayerUp(s32 index, const PatPacketHeader* header)
{
    layerMovePending_D62C = 0;
    PAT_TRACE("%s:recvAnsLayerUp\n");
    dispatchSessionHandlers(this, 0x8016, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the user who left the layer and reports it. */
s32 PatInterface::recvNtcLayerOut(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUser user;

    PAT_TRACE("%s:recvNtcLayerOut\n");
    memset(&user, 0, sizeof(PatLayerUser));
    readString(this, &length, user.userId_000, 8);
    dispatchSessionHandlers(this, 0x8017, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the layer jump's ready notice and reports it. */
s32 PatInterface::recvNtcLayerJumpReady(s32 index, const PatPacketHeader* header)
{
    u32 value;

    PAT_TRACE("%s:recvNtcLayerJumpReady\n");
    readUInt32_(this, &value);
    dispatchSessionHandlers(this, 0x8018, NULL, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reports the layer jump's go notice. */
s32 PatInterface::recvNtcLayerJumpGo(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvNtcLayerJumpGo\n");
    dispatchSessionHandlers(this, 0x8019, NULL, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a layer setting. */
s32 PatInterface::recvAnsLayerInfoSe(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerInfoSet\n");
    dispatchSessionHandlers(this, 0x801A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer's changed settings and reports them. */
s32 PatInterface::recvNtcLayerInfoSet(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvNtcLayerInfoSet\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readUnkShortArrayStruct(this, info.layer_004.path_004);
    readLayerData(this, &info.layer_004, 1);
    readUnkByteIntStruct(this, &info.tags_244, 1);
    dispatchSessionHandlers(this, 0x801B, NULL, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reads a layer's settings and reports them as the current layer's or another one's. */
s32 PatInterface::recvAnsLayerInfo(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvAnsLayerInfo\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readUnkShortArrayStruct(this, info.layer_004.path_004);
    readLayerData(this, &info.layer_004, 1);
    readUnkByteIntStruct(this, &info.tags_244, 1);
    if (info.layer_004.isCurrent_07D != 0) {
        dispatchSessionHandlers(this, 0x801C, header->requestId_02, header->status_07, 1, (const u8*)&info);
    } else {
        dispatchSessionHandlers(this, 0x801D, header->requestId_02, header->status_07, 1, (const u8*)&info);
    }
    return 0;
}

/* Reads the parent layer's settings and reports them. */
s32 PatInterface::recvAnsLayerParentInfo(s32 index, const PatPacketHeader* header)
{
    PatLayerInfo info;

    PAT_TRACE("%s:recvAnsLayerParentInfo\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readLayerData(this, &info.layer_004, 1);
    readUnkByteIntStruct(this, &info.tags_244, 1);
    dispatchSessionHandlers(this, 0x801D, header->requestId_02, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reads a child layer's number and settings and reports them. */
s32 PatInterface::recvAnsLayerChildInfo(s32 index, const PatPacketHeader* header)
{
    s16 layerId;
    PatLayerInfo info;

    PAT_TRACE("%s:recvAnsLayerChildInfo\n");
    memset(&info, 0, sizeof(PatLayerInfo));
    readShortMinusOne(this, &layerId);
    readLayerData(this, &info.layer_004, 1);
    info.layer_004.layerId_054 = layerId;
    readUnkByteIntStruct(this, &info.tags_244, 1);
    dispatchSessionHandlers(this, 0x801E, header->requestId_02, header->status_07, 1, (const u8*)&info);
    return 0;
}

/* Reads the child layer list's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerChildListHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerChildListHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x801F, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads child layer rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerChildListData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerInfo* info;
    PatLayerInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerChildListData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerInfo*)createStack(this, values[1] * sizeof(PatLayerInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerData(this, &info->layer_004, 1);
        readUnkByteIntStruct(this, &info->tags_244, 1);
    }
    dispatchSessionHandlers(this, 0x8020, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the child layer list. */
s32 PatInterface::recvAnsLayerChildListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerChildListFoot\n");
    dispatchSessionHandlers(this, 0x8021, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the sibling layer list's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerSiblingListHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerSiblingListHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x8022, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads sibling layer rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerSiblingListData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerInfo* info;
    PatLayerInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerSiblingListData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerInfo*)createStack(this, values[1] * sizeof(PatLayerInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerData(this, &info->layer_004, 1);
        readUnkByteIntStruct(this, &info->tags_244, 1);
    }
    dispatchSessionHandlers(this, 0x8023, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the sibling layer list. */
s32 PatInterface::recvAnsLayerSiblingListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerSiblingListFoot\n");
    dispatchSessionHandlers(this, 0x8024, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer host (path, id and name) and reports it. */
s32 PatInterface::recvAnsLayerHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUserInfo host;

    PAT_TRACE("%s:recvAnsLayerHost\n");
    memset(&host, 0, sizeof(PatLayerUserInfo));
    readUnkShortArrayStruct(this, host.user_000.path_028);
    readString(this, &length, host.user_000.userId_000, 8);
    readString(this, &length, host.user_000.name_008, 32);
    dispatchSessionHandlers(this, 0x8025, header->requestId_02, header->status_07, 1, (const u8*)&host);
    return 0;
}

/* Reads a new layer host (path, id and name) and reports it. */
s32 PatInterface::recvNtcLayerHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUserInfo host;

    PAT_TRACE("%s:recvNtcLayerHost\n");
    memset(&host, 0, sizeof(PatLayerUserInfo));
    readUnkShortArrayStruct(this, host.user_000.path_028);
    readString(this, &length, host.user_000.userId_000, 8);
    readString(this, &length, host.user_000.name_008, 32);
    dispatchSessionHandlers(this, 0x8026, NULL, header->status_07, 1, (const u8*)&host);
    return 0;
}

/* Reports the answer to a layer user setting. */
s32 PatInterface::recvAnsLayerUserInfoSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerUserInfoSet\n");
    dispatchSessionHandlers(this, 0x8027, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer user's changed settings and reports them. */
s32 PatInterface::recvNtcLayerUserInfoSet(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatLayerUser user;

    PAT_TRACE("%s:recvNtcLayerUserInfoSet\n");
    memset(&user, 0, sizeof(PatLayerUser));
    readString(this, &length, user.userId_000, 8);
    readLayerUserData(this, &user, 1);
    dispatchSessionHandlers(this, 0x8028, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the layer's users onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerUserList(s32 index, const PatPacketHeader* header)
{
    s32 count;
    u32 stackSize;
    u32 maxCount;
    PatLayerUser* list;

    PAT_TRACE("%s:recvAnsLayerUserList\n");
    readInt32(this, &count);
    list = (PatLayerUser*)createStack(this, count * sizeof(PatLayerUser), &stackSize);
    maxCount = stackSize / sizeof(PatLayerUser);
    if (count > (s32)maxCount) {
        count = maxCount;
    }
    readLayerUserData(this, list, count);
    dispatchSessionHandlers(this, 0x8029, header->requestId_02, header->status_07, count, (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the layer user list's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerUserListHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerUserListHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x802A, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads layer user rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerUserListData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerUserInfo* info;
    PatLayerUserInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerUserListData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerUserInfo*)createStack(this, values[1] * sizeof(PatLayerUserInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerUserInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerUserData(this, &info->user_000, 1);
        readUnkByteIntStruct(this, &info->tags_140, 1);
    }
    dispatchSessionHandlers(this, 0x802B, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the layer user list. */
s32 PatInterface::recvAnsLayerUserListFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerUserListFoot\n");
    dispatchSessionHandlers(this, 0x802C, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the layer user search's head (first index and count) and reports it. */
s32 PatInterface::recvAnsLayerUserSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerUserSearchHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x802D, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads the layer user search's rows onto the call stack and reports them. */
s32 PatInterface::recvAnsLayerUserSearchData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatLayerUserInfo* info;
    PatLayerUserInfo* list;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerUserSearchData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatLayerUserInfo*)createStack(this, values[1] * sizeof(PatLayerUserInfo), &stackSize);
    maxCount = stackSize / sizeof(PatLayerUserInfo);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readLayerUserData(this, &info->user_000, 1);
        readUnkByteIntStruct(this, &info->tags_140, 1);
    }
    dispatchSessionHandlers(this, 0x802E, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the layer user search. */
s32 PatInterface::recvAnsLayerUserSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerUserSearchFoot\n");
    dispatchSessionHandlers(this, 0x802F, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer member's binary item notice (the sender and its items) and reports it unless the items overflowed. */
s32 PatInterface::recvNtcLayerBinary(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 reserved;
    u32 length;
    u8* buffer;
    PatItemNotice notice;
    PatNtcCompound sender;

    PAT_TRACE("%s:recvNtcLayerBinary\n");
    memset(&notice, 0, sizeof(PatItemNotice));
    readString(this, &length, notice.userId_00, 8);
    readNtcCompoundData(this, &sender, 1);
    createItemListStack(this, &notice.items_08, &reserved);
    buffer = createStack(this, 8192, &stackSize);
    readItemList(this, &notice.items_08, buffer, stackSize);
    if (notice.items_08.count_02 <= notice.items_08.capacity_03) {
        dispatchSessionHandlers(this, 0x8033, NULL, header->status_07, 1, (const u8*)&notice);
    }
    growStackSize(this, stackSize);
    releaseItemListStack(this, &notice.items_08, reserved);
    return 0;
}

/* Reads a layer member's position notice (six tagged items) and reports it unless the items overflowed. */
s32 PatInterface::recvNtcLayerUserPosition(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 length;
    u8* buffer;
    PatItemList list;
    PatUserPositionNotice notice;
    PatItem items[6];
    s32 i;

    PAT_TRACE("%s:recvNtcLayerUserPosition\n");
    memset(&notice, 0, sizeof(PatUserPositionNotice));
    readString(this, &length, notice.userId_00, 8);
    memset(items, 0, sizeof(items));
    initItemList(this, &list, items, 6);
    buffer = createStack(this, 8192, &stackSize);
    readItemList(this, &list, buffer, stackSize);
    if (list.count_02 <= list.capacity_03) {
        for (i = 0; i < list.count_02; i++) {
            switch (list.items_04[i].tag_00) {
            case 1:
                notice.position_08[0] = list.items_04[i].value_08.real;
                break;
            case 2:
                notice.position_08[1] = list.items_04[i].value_08.real;
                break;
            case 3:
                notice.position_08[2] = list.items_04[i].value_08.real;
                break;
            case 4:
                notice.value_14[0] = list.items_04[i].value_08.word;
                break;
            case 5:
                notice.value_14[1] = list.items_04[i].value_08.word;
                break;
            case 6:
                notice.value_14[2] = list.items_04[i].value_08.word;
                break;
            }
        }
        dispatchSessionHandlers(this, 0x8034, NULL, header->status_07, 1, (const u8*)&notice);
    }
    growStackSize(this, stackSize);
    return 0;
}

/* Reads a layer chat (the sender and the text) and reports it. */
s32 PatInterface::recvNtcLayerChat(s32 index, const PatPacketHeader* header)
{
    u32 length;
    u8 kind;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcLayerChat\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readUInt8(this, &kind);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8035, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a layer tell. */
s32 PatInterface::recvAnsLayerTell(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerTell\n");
    dispatchSessionHandlers(this, 0x8036, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a layer tell (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcLayerTell(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcLayerTell\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8037, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reads a low-priority layer tell and reports it. */
s32 PatInterface::recvNtcLayerTellLow(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcLayerTellLow\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8038, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reads the answer to a mediation lock and reports it. */
s32 PatInterface::recvAnsLayerMediationLock(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsLayerMediationLock\n");
    readUInt8(this, &value);
    dispatchSessionHandlers(this, 0x8039, header->requestId_02, header->status_07, 1, &value);
    return 0;
}

/* Reads a mediation lock notice and reports it. */
s32 PatInterface::recvNtcLayerMediationLock(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatMediation entry;

    PAT_TRACE("%s:recvNtcLayerMediationLock\n");
    memset(&entry, 0, sizeof(PatMediation));
    readString(this, &length, entry.userId_0, 8);
    readUInt8(this, &entry.value_8);
    readMediationData(this, &entry, 1);
    dispatchSessionHandlers(this, 0x803A, NULL, header->status_07, 1, (const u8*)&entry);
    return 0;
}

/* Reads the answer to a mediation unlock and reports it. */
s32 PatInterface::recvAnsLayerMediationUnlock(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsLayerMediationUnlock\n");
    readUInt8(this, &value);
    dispatchSessionHandlers(this, 0x803B, header->requestId_02, header->status_07, 1, &value);
    return 0;
}

/* Reads a mediation unlock notice (the user id is restored after the entry's items) and reports it. */
s32 PatInterface::recvNtcLayerMediationUnlock(s32 index, const PatPacketHeader* header)
{
    u32 length;
    char userId[8];
    PatMediation entry;

    PAT_TRACE("%s:recvNtcLayerMediationUnlock\n");
    memset(&entry, 0, sizeof(PatMediation));
    readString(this, &length, userId, 8);
    readUInt8(this, &entry.value_8);
    readMediationData(this, &entry, 1);
    memcpy(entry.userId_0, userId, 8);
    dispatchSessionHandlers(this, 0x803C, NULL, header->status_07, 1, (const u8*)&entry);
    return 0;
}

/* Reads the mediation list (at most 32 entries) and reports it. */
s32 PatInterface::recvAnsLayerMediationList(s32 index, const PatPacketHeader* header)
{
    u8 count;
    u8 kind;
    s32 n;
    PatMediation entries[32];

    PAT_TRACE("%s:recvAnsLayerMediationList\n");
    memset(entries, 0, sizeof(entries));
    readUInt8(this, &kind);
    readUInt8(this, &count);
    n = (s32)count < 32 ? (s32)count : 32;
    readMediationData(this, entries, n);
    dispatchSessionHandlers(this, 0x803D, header->requestId_02, header->status_07, n, (const u8*)entries);
    return 0;
}

/* Reads the detail search's head (first index, count, total) and reports the first two. */
s32 PatInterface::recvAnsLayerDetailSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 total;
    s32 values[2];

    PAT_TRACE("%s:recvAnsLayerDetailSearchHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    readInt32(this, &total);
    dispatchSessionHandlers(this, 0x803E, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads one detail search answer (the layer, its tags and its users) onto the call stack and reports it. */
s32 PatInterface::recvAnsLayerDetailSearchData(s32 index, const PatPacketHeader* header)
{
    s32 values[2];
    u32 stackSize;
    u32 userStackSize;
    PatDetailSearchResult* result;
    u32 maxCount;
    s32 i;

    PAT_TRACE("%s:recvAnsLayerDetailSearchData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    if (values[1] > 1) {
        values[1] = 1;
    }
    result = (PatDetailSearchResult*)createStack(this, sizeof(PatDetailSearchResult), &stackSize);
    if (stackSize < sizeof(PatDetailSearchResult)) {
        values[1] = 0;
    }
    if (values[1] != 0) {
        readLayerData(this, &result->layer_00C, 1);
        readUnkByteIntStruct(this, &result->tags_24C, 1);
        readInt32(this, &result->userCount_000);
        result->users_004 =
            (PatLayerUserInfo*)createStack(this, result->userCount_000 * sizeof(PatLayerUserInfo), &userStackSize);
        stackSize += userStackSize;
        maxCount = userStackSize / sizeof(PatLayerUserInfo);
        if (result->userCount_000 > (s32)maxCount) {
            result->userCount_000 = maxCount;
        }
        for (i = 0; i < result->userCount_000; i++) {
            readLayerUserData(this, &result->users_004[i].user_000, 1);
            readUnkByteIntStruct(this, &result->users_004[i].tags_140, 1);
        }
    }
    dispatchSessionHandlers(this, 0x803F, header->requestId_02, header->status_07, values[1], (const u8*)result);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the detail search. */
s32 PatInterface::recvAnsLayerDetailSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsLayerDetailSearchFoot\n");
    dispatchSessionHandlers(this, 0x8040, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the created circle's id and reports it. */
s32 PatInterface::recvAnsCircleCreate(s32 index, const PatPacketHeader* header)
{
    s32 circleId;

    PAT_TRACE("%s:recvAnsCircleCreate ok\n");
    readInt32(this, &circleId);
    dispatchSessionHandlers(this, 0x8042, header->requestId_02, header->status_07, 1, (const u8*)&circleId);
    return 0;
}

/* Reads one circle's info and tag list and reports it. */
s32 PatInterface::recvAnsCircleInfo(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvAnsCircleInfo ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    dispatchSessionHandlers(this, 0x8043, header->requestId_02, header->status_07, 1, (const u8*)&circle);
    return 0;
}

/* Reads the joined circle and this member's slot and reports them. */
s32 PatInterface::recvAnsCircleJoin(s32 index, const PatPacketHeader* header)
{
    PatCircleUser user;

    PAT_TRACE("%s:recvAnsCircleJoin ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    dispatchSessionHandlers(this, 0x8044, header->requestId_02, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads a member who joined the circle (id, name, slot, state) and reports it. */
s32 PatInterface::recvNtcCircleJoin(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleJoin\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    readCircleSlot(this, &user.slot_04);
    readUInt8(this, &user.state_05);
    dispatchSessionHandlers(this, 0x8045, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the left circle's id and reports it. */
s32 PatInterface::recvAnsCircleLeave(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvAnsCircleLeave ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8046, header->requestId_02, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads a member who left the circle (id, slot, state) and reports it. */
s32 PatInterface::recvNtcCircleLeave(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleLeave\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readString(this, &length, user.userId_06, 8);
    readCircleSlot(this, &user.slot_04);
    readUInt8(this, &user.state_05);
    dispatchSessionHandlers(this, 0x8047, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the broken-up circle's id and reports it. */
s32 PatInterface::recvAnsCircleBreak(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvAnsCircleBreak ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8048, header->requestId_02, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads the id of a circle that broke up and reports it. */
s32 PatInterface::recvNtcCircleBreak(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvNtcCircleBreak ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8049, NULL, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reports the answer to a match option setting. */
s32 PatInterface::recvAnsCircleMatchOptionSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleMatchOptionSet ok\n");
    dispatchSessionHandlers(this, 0x804A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a member's changed match options and reports them. */
s32 PatInterface::recvNtcCircleMatchOptionSet(s32 index, const PatPacketHeader* header)
{
    PatCircleMatch member;

    PAT_TRACE("%s:recvNtcCircleMatchOptionSet\n");
    memset(&member, 0, sizeof(PatCircleMatch));
    readCircleMatchData(this, &member, 1);
    dispatchSessionHandlers(this, 0x804B, NULL, header->status_07, 1, (const u8*)&member);
    return 0;
}

/* Reads a member's id and match options and reports them. */
s32 PatInterface::recvAnsCircleMatchOptionGet(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleMatch member;

    PAT_TRACE("%s:recvAnsCircleMatchOptionGet\n");
    memset(&member, 0, sizeof(PatCircleMatch));
    readString(this, &length, member.userId_08, 8);
    readCircleMatchData(this, &member, 1);
    dispatchSessionHandlers(this, 0x804C, header->requestId_02, header->status_07, 1, (const u8*)&member);
    return 0;
}

/* Reports the answer to a match start. */
s32 PatInterface::recvAnsCircleMatchStart(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleMatchStart ok\n");
    dispatchSessionHandlers(this, 0x804D, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the match start notice (the members' slots, ids, flags and values, an optional tail) and reports it. */
s32 PatInterface::recvNtcCircleMatchStart(s32 index, const PatPacketHeader* header)
{
    u16 remaining;
    u32 length;
    PatMatchStart start;
    PatCircleMatch members[4];
    PatCircleMatch* member;
    s32 i;

    PAT_TRACE("%s:recvNtcCircleMatchStart ok\n");
    memset(&start, 0, sizeof(PatMatchStart));
    memset(members, 0, sizeof(members));
    member = members;
    start.members_04 = member;
    beginReadBlock(this, &remaining);
    readInt32(this, &start.count_00);
    remaining -= 4;
    if (start.count_00 < 1 || start.count_00 > 4) {
        PAT_KEEP_FAILURE();
        return -1;
    }
    memset(member, 0, sizeof(members));
    for (i = 0; i < start.count_00; i++) {
        readCircleSlot(this, &member->slot_07);
        remaining -= 1;
        readString(this, &length, member->userId_08, 8);
        remaining -= (u16)(length + 2);
        readUInt8Array(this, &length, &member->flag_00, 1);
        remaining -= (u16)(length + 2);
        readUInt16(this, &member->value_04);
        remaining -= 2;
        member++;
    }
    if (remaining != 0) {
        readUInt8(this, &start.flag_08);
        readUInt32_(this, &start.values_0C[0]);
        readUInt32_(this, &start.values_0C[1]);
        readUInt32_(this, &start.values_0C[2]);
        readUInt32_(this, &start.values_0C[3]);
    }
    endReadBlock(this);
    readInt32(this, &matchStartValue_D630);
    dispatchSessionHandlers(this, 0x804E, NULL, header->status_07, start.count_00, (const u8*)&start);
    return 0;
}

/* Reports the answer to a match end. */
s32 PatInterface::recvAnsCircleMatchEnd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleMatchEnd ok\n");
    dispatchSessionHandlers(this, 0x804F, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the circle whose settings changed and reports it. */
s32 PatInterface::recvAnsCircleInfoSet(s32 index, const PatPacketHeader* header)
{
    s32 value;

    PAT_TRACE("%s:recvAnsCircleInfoSet ok\n");
    readInt32(this, &value);
    dispatchSessionHandlers(this, 0x8051, header->requestId_02, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads a circle's changed info and tag list and reports it. */
s32 PatInterface::recvNtcCircleInfoSet(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvNtcCircleInfoSet ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    dispatchSessionHandlers(this, 0x8052, NULL, header->status_07, 1, (const u8*)&circle);
    return 0;
}

/* Reads the layer's circles onto the call stack and reports them. */
s32 PatInterface::recvAnsCircleListLayer(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatCircleEntry* info;
    PatCircleEntry* list;
    s32 i;

    PAT_TRACE("%s:recvAnsCircleListLayer ok\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatCircleEntry*)createStack(this, values[1] * sizeof(PatCircleEntry), &stackSize);
    maxCount = stackSize / sizeof(PatCircleEntry);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readCircleInfoDataArray(this, info, 1);
        readUnkByteIntStruct(this, &info->tags_37C, 1);
    }
    dispatchSessionHandlers(this, 0x8053, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the circle search's head (first index and count) and reports it. */
s32 PatInterface::recvAnsCircleSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsCircleSearchHead ok\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    dispatchSessionHandlers(this, 0x8054, header->requestId_02, header->status_07, 2, (const u8*)values);
    return 0;
}

/* Reads the circle search's rows onto the call stack and reports them. */
s32 PatInterface::recvAnsCircleSearchData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatCircleEntry* info;
    PatCircleEntry* list;
    s32 i;

    PAT_TRACE("%s:recvAnsCircleSearchData ok\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatCircleEntry*)createStack(this, values[1] * sizeof(PatCircleEntry), &stackSize);
    maxCount = stackSize / sizeof(PatCircleEntry);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readCircleInfoDataArray(this, info, 1);
        readUnkByteIntStruct(this, &info->tags_37C, 1);
    }
    dispatchSessionHandlers(this, 0x8055, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the circle search. */
s32 PatInterface::recvAnsCircleSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleSearchFoot ok\n");
    dispatchSessionHandlers(this, 0x8056, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a kick. */
s32 PatInterface::recvAnsCircleKick(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleKick\n");
    dispatchSessionHandlers(this, 0x8057, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a kick notice (its kind and byte string) and reports it. */
s32 PatInterface::recvNtcCircleKick(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatKickList kick;

    PAT_TRACE("%s:recvNtcCircleKick\n");
    memset(&kick, 0, sizeof(PatKickList));
    readUInt8(this, &kick.kind_000);
    readUInt8Array(this, &length, kick.data_002, 256);
    kick.size_102 = (u16)length;
    dispatchSessionHandlers(this, 0x8058, NULL, header->status_07, 1, (const u8*)&kick);
    return 0;
}

/* Reports the answer to clearing the kick list. */
s32 PatInterface::recvAnsCircleDeleteKickList(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleDeleteKickList\n");
    dispatchSessionHandlers(this, 0x8059, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a host handover. */
s32 PatInterface::recvAnsCircleHostHandover(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleHostHandover ok\n");
    dispatchSessionHandlers(this, 0x805A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the new host of a handover (circle, slot, id, name, flag) and reports it. */
s32 PatInterface::recvNtcCircleHostHandover(s32 index, const PatPacketHeader* header)
{
    u32 length;
    u8 flag;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleHostHandover ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    readUInt8Array(this, &length, &flag, 1);
    dispatchSessionHandlers(this, 0x805B, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the circle's host (circle, slot, id, name) and reports it. */
s32 PatInterface::recvAnsCircleHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvAnsCircleHost ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    dispatchSessionHandlers(this, 0x805C, header->requestId_02, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads a new circle host (circle, slot, id, name) and reports it. */
s32 PatInterface::recvNtcCircleHost(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatCircleUser user;

    PAT_TRACE("%s:recvNtcCircleHost ok\n");
    memset(&user, 0, sizeof(PatCircleUser));
    readInt32(this, &user.circleId_00);
    readCircleSlot(this, &user.slot_04);
    readString(this, &length, user.userId_06, 8);
    readString(this, &length, user.name_0E, 32);
    dispatchSessionHandlers(this, 0x805D, NULL, header->status_07, 1, (const u8*)&user);
    return 0;
}

/* Reads the circle's members (at most four) with their match options and reports them. */
s32 PatInterface::recvAnsCircleUserList(s32 index, const PatPacketHeader* header)
{
    s32 count;
    PatCircleMatch members[4];

    PAT_TRACE("%s:recvAnsCircleUserList ok\n");
    readInt32(this, &count);
    if (count > 4) {
        count = 4;
    }
    memset(members, 0, sizeof(members));
    readCircleMatchData(this, members, count);
    dispatchSessionHandlers(this, 0x805E, header->requestId_02, header->status_07, count, (const u8*)members);
    return 0;
}

/* Reads a circle member's binary item notice (the circle, the sender and its items) and reports it unless the items
 * overflowed. */
s32 PatInterface::recvNtcCircleBinary(s32 index, const PatPacketHeader* header)
{
    s32 circleId;
    u32 stackSize;
    u32 reserved;
    u32 length;
    u8* buffer;
    PatItemNotice notice;
    PatNtcCompound sender;

    PAT_TRACE("%s:recvNtcCircleBinary\n");
    readInt32(this, &circleId);
    memset(&notice, 0, sizeof(PatItemNotice));
    readString(this, &length, notice.userId_00, 8);
    readNtcCompoundData(this, &sender, 1);
    createItemListStack(this, &notice.items_08, &reserved);
    buffer = createStack(this, 8192, &stackSize);
    readItemList(this, &notice.items_08, buffer, stackSize);
    if (notice.items_08.count_02 <= notice.items_08.capacity_03) {
        dispatchSessionHandlers(this, 0x805F, NULL, header->status_07, 1, (const u8*)&notice);
    }
    growStackSize(this, stackSize);
    releaseItemListStack(this, &notice.items_08, reserved);
    return 0;
}

/* Reads a circle chat (the sender and the text) and reports it. */
s32 PatInterface::recvNtcChat(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcChat\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8060, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a circle tell. */
s32 PatInterface::recvAnsCircleTell(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleTell\n");
    dispatchSessionHandlers(this, 0x8061, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a circle tell (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcCircleTell(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcCircleTell\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8062, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to the circle notice setting and advances the request state. */
s32 PatInterface::recvAnsCircleInfoNoticeSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsCircleInfoNoticeSet ok\n");
    dispatchSessionHandlers(this, 0x8063, header->requestId_02, header->status_07, 0, NULL);
    requestState_6135 += 5;
    return 0;
}

/* Reads a circle created in the layer and reports it while a layer move is pending. */
s32 PatInterface::recvNtcCircleListLayerCreate(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvNtcCircleListLayerCreate ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    if (layerMovePending_D62C != 0) {
        dispatchSessionHandlers(this, 0x8064, NULL, header->status_07, 1, (const u8*)&circle);
    }
    return 0;
}

/* Reads a changed circle of the layer and reports it while a layer move is pending. */
s32 PatInterface::recvNtcCircleListLayerChange(s32 index, const PatPacketHeader* header)
{
    PatCircleEntry circle;

    PAT_TRACE("%s:recvNtcCircleListLayerChange ok\n");
    memset(&circle, 0, sizeof(PatCircleEntry));
    readInt32(this, &circle.circleId_000);
    readCircleInfoDataArray(this, &circle, 1);
    readUnkByteIntStruct(this, &circle.tags_37C, 1);
    if (layerMovePending_D62C != 0) {
        dispatchSessionHandlers(this, 0x8065, NULL, header->status_07, 1, (const u8*)&circle);
    }
    return 0;
}

/* Reads a deleted circle's id and reports it while a layer move is pending. */
s32 PatInterface::recvNtcCircleListLayerDelete(s32 index, const PatPacketHeader* header)
{
    s32 circleId;

    PAT_TRACE("%s:recvNtcCircleListLayerDelete ok\n");
    readInt32(this, &circleId);
    if (layerMovePending_D62C != 0) {
        dispatchSessionHandlers(this, 0x8066, NULL, header->status_07, 1, (const u8*)&circleId);
    }
    return 0;
}

/* Reports the answer to an MCS creation. */
s32 PatInterface::recvAnsMcsCreate(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsMcsCreate\n");
    dispatchSessionHandlers(this, 0x8067, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the MCS creation notice's value and reports it. */
s32 PatInterface::recvNtcMcsCreate(s32 index, const PatPacketHeader* header)
{
    s8 value;

    PAT_TRACE("%s:recvNtcMcsCreate\n");
    readUInt8_(this, &value);
    dispatchSessionHandlers(this, 0x8068, NULL, header->status_07, 1, (const u8*)&value);
    return 0;
}

/* Reads the MCS server a match starts on (address, port, name) and reports it. */
s32 PatInterface::recvNtcMcsStart(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatMcsServer server;

    PAT_TRACE("%s:recvNtcMcsStart\n");
    memset(&server, 0, sizeof(PatMcsServer));
    readString(this, &length, server.host_000, 256);
    readUInt16(this, &server.port_104);
    readString(this, &length, server.name_106, 32);
    dispatchSessionHandlers(this, 0x8069, NULL, header->status_07, 1, (const u8*)&server);
    return 0;
}

/* Reports the answer to a tell. */
s32 PatInterface::recvAnsTell(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsTell\n");
    dispatchSessionHandlers(this, 0x806A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a tell (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcTell(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcTell\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x806B, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a user binary. */
s32 PatInterface::recvAnsBinaryUser(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBinaryUser ok\n");
    dispatchSessionHandlers(this, 0x806C, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a user's binary notice (the sender and the bytes) and reports it. */
s32 PatInterface::recvNtcBinaryUser(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatBinaryMessage notice;

    PAT_TRACE("%s:recvNtcBinaryUser ok\n");
    memset(&notice.sender_104, 0, sizeof(PatNtcCompound));
    readString(this, &length, notice.sender_104.userId_04, 8);
    readNtcCompoundData(this, &notice.sender_104, 1);
    readUInt8Array(this, &length, notice.data_000, 256);
    notice.size_100 = (u16)length;
    dispatchSessionHandlers(this, 0x806D, NULL, header->status_07, 1, (const u8*)&notice);
    return 0;
}

/* Reads the server's binary notice; nothing is reported. */
s32 PatInterface::recvNtcBinaryServer(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatBinaryMessage notice;

    PAT_TRACE("%s:recvNtcBinaryServer ok\n");
    memset(&notice.sender_104, 0, sizeof(PatNtcCompound));
    readNtcCompoundData(this, &notice.sender_104, 1);
    readUInt8Array(this, &length, notice.data_000, 256);
    notice.size_100 = (u16)length;
    return 0;
}

/* Reports the answer to a user search setting. */
s32 PatInterface::recvAnsUserSearchSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserSearchSet\n");
    dispatchSessionHandlers(this, 0x806E, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a user binary setting. */
s32 PatInterface::recvAnsUserBinarySet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserBinarySet\n");
    dispatchSessionHandlers(this, 0x806F, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a user binary notice. */
s32 PatInterface::recvAnsUserBinaryNotice(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserBinaryNotice\n");
    dispatchSessionHandlers(this, 0x8070, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a user's binary notice (kind, id, value, bytes) and reports it. */
s32 PatInterface::recvNtcUserBinaryNotice(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatUserBinaryNotice notice;

    PAT_TRACE("%s:recvNtcUserBinaryNotice\n");
    readUInt8(this, &notice.kind_008);
    readString(this, &length, notice.userId_000, 8);
    readUInt32_(this, &notice.value_110);
    readUInt8Array(this, &length, notice.data_009, 256);
    notice.size_10C = length;
    dispatchSessionHandlers(this, 0x8071, NULL, header->status_07, 1, (const u8*)&notice);
    return 0;
}

/* Reads the user search's head and reports its first value unless the search is silent. */
s32 PatInterface::recvAnsUserSearchHead(s32 index, const PatPacketHeader* header)
{
    s32 values[2];

    PAT_TRACE("%s:recvAnsUserSearchHead\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    if (searchMode_6118 == 0) {
        dispatchSessionHandlers(this, 0x8030, header->requestId_02, header->status_07, 1, (const u8*)values);
    }
    return 0;
}

/* Reads the user search's rows onto the call stack and reports them unless the search is silent. */
s32 PatInterface::recvAnsUserSearchData(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatUserSearch* info;
    PatUserSearch* list;
    s32 i;

    PAT_TRACE("%s:recvAnsUserSearchData\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatUserSearch*)createStack(this, values[1] * sizeof(PatUserSearch), &stackSize);
    maxCount = stackSize / sizeof(PatUserSearch);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    for (i = 0, info = list; i < values[1]; info++, i++) {
        readUserSearchData(this, info, 1);
        readUnkByteIntStruct(this, &info->tags_230, 1);
    }
    if (searchMode_6118 == 0) {
        dispatchSessionHandlers(this, 0x8031, header->requestId_02, header->status_07, values[1], (const u8*)list);
    }
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the end of the user search unless the search is silent. */
s32 PatInterface::recvAnsUserSearchFoot(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserSearchFoot\n");
    if (searchMode_6118 == 0) {
        dispatchSessionHandlers(this, 0x8032, header->requestId_02, header->status_07, 0, NULL);
    }
    return 0;
}

/* Reads one user search row and reports it as the search kind's event. */
s32 PatInterface::recvAnsUserSearchInfo(s32 index, const PatPacketHeader* header)
{
    PatUserSearch row;

    PAT_TRACE("%s:recvAnsUserSearchInfo\n");
    memset(&row, 0, sizeof(PatUserSearch));
    readUserSearchData(this, &row, 1);
    readUnkByteIntStruct(this, &row.tags_230, 1);
    switch (searchKind_6119) {
    case 1:
        dispatchSessionHandlers(this, 0x8072, header->requestId_02, header->status_07, 1, (const u8*)&row);
        break;
    case 2:
        dispatchSessionHandlers(this, 0x8073, header->requestId_02, header->status_07, 1, (const u8*)&row);
        break;
    }
    return 0;
}

/* Reads this client's own search row and keeps its two values. */
s32 PatInterface::recvAnsUserSearchInfoMine(s32 index, const PatPacketHeader* header)
{
    PatUserSearch row;

    PAT_TRACE("%s:recvAnsUserSearchInfoMine\n");
    memset(&row, 0, sizeof(PatUserSearch));
    readUserSearchData(this, &row, 1);
    mySearchValue_65E8 = row.value_228;
    mySearchValue_65EC = row.value_22C;
    requestState_6135 += 5;
    return 0;
}

/* Reports the answer to a user status setting. */
s32 PatInterface::recvAnsUserStatusSet(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsUserStatusSet\n");
    dispatchSessionHandlers(this, 0x8074, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a user status record and reports it. */
s32 PatInterface::recvAnsUserStatus(s32 index, const PatPacketHeader* header)
{
    u8 status[7];

    PAT_TRACE("%s:recvAnsUserStatus\n");
    memset(status, -1, sizeof(status));
    readUserStatusData(this, status, 1);
    dispatchSessionHandlers(this, 0x8075, header->requestId_02, header->status_07, 1, status);
    return 0;
}

/* Reports the answer to a friend request. */
s32 PatInterface::recvAnsFriendAdd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFriendAdd\n");
    dispatchSessionHandlers(this, 0x8076, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a friend request notice (the friend and its state) and reports it. */
s32 PatInterface::recvNtcFriendAdd(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatFriend entry;

    PAT_TRACE("%s:recvNtcFriendAdd\n");
    memset(&entry, 0, sizeof(PatFriend));
    readString(this, &length, entry.userId_04, 8);
    readFriendData(this, &entry, 1);
    readUInt8_(this, &entry.state_2C);
    dispatchSessionHandlers(this, 0x8077, NULL, header->status_07, 1, (const u8*)&entry);
    return 0;
}

/* Reports the answer to a friend acceptance. */
s32 PatInterface::recvAnsFriendAccept(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFriendAccept\n");
    dispatchSessionHandlers(this, 0x8078, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads a friend acceptance notice (the sender's id and items, the text) and reports it. */
s32 PatInterface::recvNtcFriendAccept(s32 index, const PatPacketHeader* header)
{
    u32 length;
    PatChatMessage chat;

    PAT_TRACE("%s:recvNtcFriendAccept\n");
    memset(&chat, 0, sizeof(PatChatMessage));
    readString(this, &length, chat.sender_100.userId_08, 8);
    readChatData(this, &chat.sender_100, 1);
    readString(this, &length, chat.text_000, 256);
    dispatchSessionHandlers(this, 0x8079, NULL, header->status_07, 1, (const u8*)&chat);
    return 0;
}

/* Reports the answer to a friend deletion. */
s32 PatInterface::recvAnsFriendDelete(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsFriendDelete\n");
    dispatchSessionHandlers(this, 0x807A, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the friend list onto the call stack and reports it. */
s32 PatInterface::recvAnsFriendList(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatFriend* list;

    PAT_TRACE("%s:recvAnsFriendList\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatFriend*)createStack(this, values[1] * sizeof(PatFriend), &stackSize);
    maxCount = stackSize / sizeof(PatFriend);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    readFriendData(this, list, values[1]);
    dispatchSessionHandlers(this, 0x807B, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reports the answer to a black list addition. */
s32 PatInterface::recvAnsBlackAdd(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBlackAdd\n");
    dispatchSessionHandlers(this, 0x807C, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reports the answer to a black list deletion. */
s32 PatInterface::recvAnsBlackDelete(s32 index, const PatPacketHeader* header)
{
    PAT_TRACE("%s:recvAnsBlackDelete\n");
    dispatchSessionHandlers(this, 0x807D, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}

/* Reads the black list onto the call stack and reports it. */
s32 PatInterface::recvAnsBlackList(s32 index, const PatPacketHeader* header)
{
    u32 stackSize;
    u32 maxCount;
    s32 values[2];
    PatBlackListEntry* list;

    PAT_TRACE("%s:recvAnsBlackList\n");
    readInt32(this, &values[0]);
    readInt32(this, &values[1]);
    list = (PatBlackListEntry*)createStack(this, values[1] * sizeof(PatBlackListEntry), &stackSize);
    maxCount = stackSize / sizeof(PatBlackListEntry);
    if (values[1] > (s32)maxCount) {
        values[1] = maxCount;
    }
    readBlackListData(this, list, values[1]);
    dispatchSessionHandlers(this, 0x807E, header->requestId_02, header->status_07, values[1], (const u8*)list);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the agreement's page count and reports it. */
s32 PatInterface::recvAnsAgreementPageNum(s32 index, const PatPacketHeader* header)
{
    u8 version;
    u8 pageCount;

    PAT_TRACE("%s:recvAnsAgreementPageNum\n");
    readUInt8(this, &version);
    readUInt8(this, &pageCount);
    dispatchSessionHandlers(this, 0x807F, header->requestId_02, header->status_07, 1, &pageCount);
    return 0;
}

/* Reads the agreement's page info (its page records onto the call stack) and reports it. */
s32 PatInterface::recvAnsAgreementPageInfo(s32 index, const PatPacketHeader* header)
{
    PatAgreementInfo info;
    PatAgreementInfo* infoPtr;
    u32 stackSize;
    u8 kind;

    PAT_TRACE("%s:recvAnsAgreementPageInfo\n");
    readUInt8(this, &kind);
    infoPtr = &info;
    readUInt8(this, &infoPtr->version_00);
    readUInt8(this, &infoPtr->pageCount_0C);
    infoPtr->pages_10 = createStack(this, infoPtr->pageCount_0C * 40, &stackSize);
    if (infoPtr->pageCount_0C * 40 > stackSize) {
        PAT_KEEP_FAILURE();
        growStackSize(this, stackSize);
        return -1;
    }
    readAgreementPageData(this, infoPtr->pages_10, infoPtr->pageCount_0C);
    readAgreementInfoData(this, infoPtr, 1);
    dispatchSessionHandlers(this, 0x8080, header->requestId_02, header->status_07, 1, (const u8*)infoPtr);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads one agreement page (its text onto the call stack) and reports it. */
s32 PatInterface::recvAnsAgreementPage(s32 index, const PatPacketHeader* header)
{
    PatAgreementPage page;
    u32 stackSize;
    u32 length;
    u8 pageIndex;
    u8 kind;

    PAT_TRACE("%s:recvAnsAgreementPage\n");
    readUInt8(this, &kind);
    readUInt8(this, &pageIndex);
    page.page_0 = pageIndex;
    readUInt32_(this, &page.value_4);
    readUInt32_(this, &page.size_8);
    page.data_C = createStack(this, page.size_8, &stackSize);
    if (page.size_8 > stackSize) {
        page.size_8 = stackSize;
    }
    readUInt8Array(this, &length, page.data_C, page.size_8);
    dispatchSessionHandlers(this, 0x8081, header->requestId_02, header->status_07, 1, (const u8*)&page);
    growStackSize(this, stackSize);
    return 0;
}

/* Reads the agreement answer and reports it. */
s32 PatInterface::recvAnsAgreement(s32 index, const PatPacketHeader* header)
{
    u8 value;

    PAT_TRACE("%s:recvAnsAgreement\n");
    readUInt8(this, &value);
    dispatchSessionHandlers(this, 0x8082, header->requestId_02, header->status_07, 0, NULL);
    return 0;
}
