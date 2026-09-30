/*
 * fn_803D3CE8.cpp - the Network session band, `.text` 0x803D3CE8..0x803D70B8 (101 functions).
 *
 * WHAT IT IS.  The serialization/state half of the Wii network session subsystem: the op-code
 * packet writers of `NetworkSessionStable`, the request pool and its state machine on
 * `NetworkSessionManager` (21 request slots, a two-slot `NetworkRequest` pool at +0x7C, virtual
 * dispatch), and the `NetworkSessionManagerPat` half that owns its own request pair, the per-player
 * records and the 32-entry circle list.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable: every
 * `lis`/`addi` pair in the range resolves to a float constant, a `.data` request descriptor or one of
 * the class log strings, never to a source-file-name literal.  2. `dumpmap.py lookup` answers only
 * `zz_XXXXXXXX_` for the code.  3. The code and the vtables place the band in `Network` (the
 * registered neighbour is `Network/NetworkWiiMediator.cpp`).  The tile spans more than one original TU,
 * so no single evidenced file name covers it and the file keeps the map's `fn_803D3CE8` stem.
 *
 * HOW THE NAMES WERE RECOVERED (rule 7).  The `.data` log strings name their own emitters, and each is
 * loaded by exactly one function in the range: "NetworkSessionStable::downPerformance: ..." (0x805FA550,
 * 0x805FA590) -> `downPerformance`; "::upPerformance ..." (0x805FA5D4, 0x805FA610) -> `upPerformance`;
 * "NetworkSessionStable::move: oob sqn send. ..." (0x805FA64C) -> `move`; "NetworkSessionManager::move:
 * request[%d] is moving ..." (0x805FA788) -> `NetworkSessionManager::move` (the old `slot_18`);
 * "NetworkSessionManagerPat::final ..." (0x805FAB08) -> the Pat flush.  The 21 request descriptors are
 * 12-byte `{0, opcode, 0}` records, so each `requestNNN` is named by its own op code.  The float
 * constants every body loads were read out of the DOL and named from their use (`networkRateMax` =
 * 1.0f, `networkRateScale` = 2.0f, `networkMillisecondsPerSecond` = 1000.0f, ...).  Everything else in
 * the name set that is not evidenced is marked GUESS below.
 *
 * CLASS AND RULE 10.  Both vtables this range owns are emitted by MWCC from the class declarations,
 * never written by hand.  `.data` 0x805FA908..0x805FAAD0 (456 B, the base `NetworkSessionManager`
 * table, 51 relocations) is claimed and byte-identical.  `NetworkSessionManagerPat` declares `move`
 * FIRST so that it is the class's key function - its body lives in the next band (0x803D70B8) - which
 * is why MWCC emits the Pat constructor's `__vt__24NetworkSessionManagerPat` store without emitting a
 * second table into this object; the target's `.data` is the base table alone.  The four record
 * classes are structs with a vtable member, not polymorphic classes: a class makes MWCC initialise the
 * vptr of every element of the `__construct_array`-built arrays (measured: the Pat constructor
 * 252 -> 544 B).  See `tools/units/vtableaudit.py`.
 *
 * LANGUAGE AND SECTIONS.  C++ (`__nw__FUl` / `__dl__FPv` / `__ptmf_scall`); `cflags_network`'s
 * `-Cpp_exceptions on` for
 * the target's `extab`/`extabindex`.  `#pragma dont_inline on` scopes keep the four Pat methods'
 * calls to the base's `clear`/`init`/`release` and to the record destructors as the target's `bl`s
 * (without them MWCC inlines 220 of `clear`'s 276 bytes).
 *
 * PEEPHOLE.  The 21 `request*` rows (364, and the contiguous 368..444 block) need the peephole pass
 * off, scoped by two `#pragma peephole off`/`on` pairs.  With it on, MWCC fuses the descriptor address
 * materialisation into the first word's load (`addi r5,r4,@l` + `lwz r4,0(r5)` becomes
 * `lwzu r4,@l(r5)`), which is 4 B shorter than the target's 120/136/152/168 B; the pass is the
 * lever and not the flag (playbook 39/41: the command line's `-opt nopeephole` is accepted and
 * changes nothing).  All 21 rows are 100 % with the pragma.  `NetworkRequest_begin` and
 * `hasBuffer` sit inside/next to the region and do not move.
 *
 * RULE 10 - the two dispatches.  `.data` 0x805FA908..0x805FAAD0 (456 B, the base
 * `NetworkSessionManager` table, 51 relocations) is claimed and byte-identical; `NetworkBuffer`
 * (table at 0x805F9150) and `NetworkSessionStable` (0x805FA6E8) are another band's and are
 * *declared* classes with no virtual defined here, so MWCC emits no table for them (the
 * `NetworkBuffer` conversion took 11 rows to 100 % and 9 more up - measured, no row down).  The
 * four record classes stay structs with a vtable member: a class makes MWCC initialise the vptr of
 * every element of the `__construct_array`-built arrays (Pat constructor 252 -> 544 B).
 *
 * OTHER LAYOUT FACTS.  `NetworkSessionSlot` is 0x924 B, the stride the target's `mulli` uses (the
 * declaration only named fields to +0xDC, so the array stride was wrong); `NetworkStreamWriter` is
 * 0x20 B, not 0x24 (`send8`'s retail frame is 0x30 with the writer at +0x10, so one writer is 0x20 B;
 * 0x24 also forced our local to +0x0C - the frames themselves do not move with the size).
 *
 * STATUS / RESIDUALS (92.15 % fuzzy, 51 of 101 symbols at 100 %; `.text` 12980 vs 13264 B).
 *  - `NetworkRequest_copyRecord` 15.73 % (target 148 B, ours 52 B): the target copies the 96-byte
 *    record as two words then eleven word pairs; `*dst = *src` makes MWCC emit `lmw`/`stmw` instead
 *    (measured: `-use_lmw_stmw off` for this unit changes nothing).  The shape is not reachable from
 *    the source side; the function is referenced only from this object's own `extab`.
 *  - `slot_144` 46.10 % and `sendBatch_138`/`slot_13C`/`slot_140` 83.8-84.6 %: the buffer dispatch
 *    is now the canonical one; what is left is the staging order of the mapped byte array plus the
 *    `clrlwi` the target gives the `u8` flags argument at the call (ours passes it unmasked - the
 *    byte-for-byte `mr`/`clrlwi` pair is not reachable from any source spelling tried).
 *  - `networkSessionReflectCallback` 54.58 %: the target saves all six incoming argument registers
 *    before building the callee's, ours does the minimal four-move rotation; the two are equivalent
 *    and the naive form is not reachable from the source side (48 B).
 *  - `NetworkSessionStable_getUsableSlot`/`updateRate`/`downPerformance`/`upPerformance`/`move`
 *    81-95 %: the `-O3` colouring of the slot pointer and of the two `f32` rates (identical
 *    instruction multiset, `fmuls f2,f2,f0` against `fmuls f0,f2,f0`, and one `extsb.` retail keeps
 *    unfolded), plus `updateRate`'s other dispatch, `self->vtable->getFloat_8C` (the one
 *    `NetworkSessionStable` call site left as a vtable-member read).
 *  - `NetworkRequest_begin` 91.38 %: the target inlines the three-word `va_list` setup, ours calls
 *    the `__va_start` intrinsic, because the pinned toolchain ships no `<stdarg.h>` and the
 *    CodeWarrior macro form is not reachable (`__builtin_va_start` does not exist).
 *    `tools/units/flipcheck.py` reports `__va_start` as a flip blocker for this reason.
 *  - `.text` is 284 B short of the claim, so `.text`/`extab` cannot flip yet: `copyRecord` (-96 B)
 *    plus the functions whose bodies compress.  `flipcheck` reports `.text` 0x32B4 vs 0x33D0 and
 *    `extab` 0x2E4 vs 0x4EC.
 *  - GUESS names: `NetworkSessionSlotInfo_*`, `NetworkSessionCircleInfo_*`, `NetworkSessionCircleList_*`
 *    and `NetworkSessionPlayerRecord_*` come from their container offsets and `net_va_arg`/`memset`
 *    use; `networkSessionReflectCallbackEx` and the `network<span>*` accessor names are derived from
 *    the callee each forwards to.  The `slot_14C`..`slot_168` wrappers are named for their vtable slot
 *    (offset-derived, the buffer class owner's names are unknown).
 *  - `.sdata2` is EMPTY (was 20 B over the target): every literal the range loaded is the map's
 *    named constant.
 *  - `extab` 0x2E4 vs 0x4EC and `extabindex` vs the target are short because 284 B of `.text` is
 *    still compressed away.
 */

#include "types.h"
#include "Network/fn_803D3CE8.h"

/* `NetworkVaState` and the two variadic intrinsics live in this unit's header (rule 2 keeps the
   declaration with the TU that needs it). */
#define net_va_start(ap) __va_start(&(ap))
#define net_va_arg(ap, type) (*(type*)__va_arg(&(ap), 1))
#include "unsplit/NetworkData.h"

/* Rule 2: the symbols this unit calls but does not own come from their owner's headers, never from a
   declaration here - `getInstance_` (0x803768F0) is inside `enemy/em020_ai.cpp`'s registered range,
   `memset` is `Runtime.PPCEABI.H/memset.c`, and `getNetworkLogger` (0x803C9974, the band's log-manager
   getter) is owned by nobody, so it is declared with the rest of the Network band in
   `unsplit/Network.h` - where it is typed as the class `NetworkLogger` the logging band uses.  The
   sites below keep the older `NetworkSessionManagerLogger` view of that object and cast. */
#include "enemy/em020_ai.h"
#include "unsplit/Network.h"
#include "Runtime.PPCEABI.H/memset.h"

/* EXCEPTIONS.  The lib builds with `-Cpp_exceptions off` while this range's target object carries
   `extab` 1260 B and `extabindex` 912 B (the C++ unwind records of the class below); the pragma turns the
   front-end's exceptions back on and moves no `.text` byte. */

/* ---- this unit's own forward declarations ---- */
extern "C" {
void fn_803D3CE8(NetworkSessionStable*, u32);
void NetworkSessionStable_send8(NetworkSessionStable*);
void NetworkSessionStable_send6(NetworkSessionStable*, s8);
void NetworkSessionStable_send10(NetworkSessionStable*, s8);
void NetworkSessionStable_send11(NetworkSessionStable*, s8, u32, u32, s8, f32);
void NetworkSessionStable_send8or9(NetworkSessionStable*, u32, s8);
void NetworkSessionStable_send4(NetworkSessionStable*, const void*, u32, s8);
void NetworkSessionStable_updateRate(NetworkSessionStable*, s8);
s8 NetworkSessionStable_getUsableSlot(NetworkSessionStable*, s8);

void NetworkRequest_reset(NetworkRequest*);
void* NetworkRequest_deleteElement(NetworkRequest*, s16);
void NetworkRequest_clear(NetworkRequest*);
NetworkRequest* NetworkRequest_construct(NetworkRequest*);
s32 NetworkRequest_isOwned(NetworkRequest*);
void zz_03d5084_ptmf_scall(NetworkRequest*);
void NetworkRequest_begin(NetworkRequest*, NetworkSessionManager*, NetworkRequestDesc, u32, ...);
void NetworkRequest_cancel(NetworkRequest*);
s32 NetworkRequest_isCancelled(NetworkRequest*);
NetworkRequest* NetworkSessionManager_allocRequest(NetworkSessionManager*);
void NetworkSessionManager_deleteRequest(NetworkSessionManager*, NetworkRequest**);
s32 NetworkRequest_getRecord(NetworkRequest*, u32*);
void NetworkRequest_setRecord(NetworkRequest*, u32, u32, u32);
s32 NetworkRequest_getArgument(NetworkRequest*, u32);
}

/* ---- extra neighbouring globals ---- */
/* The four addresses themselves are declared in the band's data header (rule 2).  They cannot come
   from `include/unsplit/Network.h`: that header declares `dtor_803CA338(void*, s32)` where this
   file's own header declares `dtor_803CA338(void*)`, and including both fails to compile - the
   reason `include/unsplit/NetworkData.h` exists.  `getNetworkLogger` and the logger type live in this
   unit's own header. */

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionStable - op-code packet writers                                             */
/* ----------------------------------------------------------------------------------------- */

extern "C" void fn_803D3CE8(NetworkSessionStable* self, u32 value)
{
    NetworkStreamWriter stream;
    s8 term;
    u16 n;

    fn_803CB9B4(&stream);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    n = writeByte(&stream, 1);
    writeSize(&stream, (u16)(n + writeUInt(&stream, value)));
    term = -1;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

extern "C" void NetworkSessionStable_send8(NetworkSessionStable* self)
{
    NetworkStreamWriter stream;
    s8 term;

    fn_803CB9B4(&stream);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    writeSize(&stream, writeByte(&stream, 2));
    term = -1;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

extern "C" void NetworkSessionStable_send6(NetworkSessionStable* self, s8 value)
{
    NetworkStreamWriter stream;
    s8 term;

    fn_803CB9B4(&stream);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    writeSize(&stream, writeByte(&stream, 6));
    term = value;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

extern "C" void NetworkSessionStable_send10(NetworkSessionStable* self, s8 idx)
{
    NetworkStreamWriter stream;
    s8 term;
    u16 n;

    fn_803CB9B4(&stream);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    n = writeByte(&stream, 10);
    writeSize(&stream, (u16)(n + fn_803F8BDC(&stream, &self->slots_14838[idx].state_20)));
    term = -2;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

extern "C" void NetworkSessionStable_send11(NetworkSessionStable* self, s8 a, u32 b, u32 c, s8 d, f32 f)
{
    NetworkStreamWriter stream;
    s8 term;
    u32 scaled;
    u16 n;
    u32 n2;
    u32 n3;
    u32 n4;

    scaled = 0;
    fn_803CB9B4(&stream);
    scaled = (u32)(networkMillisecondsPerSecond * f);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    n = writeByte(&stream, 11);
    n2 = n + fn_803F8BDC(&stream, (const void*)b);
    n3 = n2 + writeUInt(&stream, c);
    n4 = n3 + writeUInt(&stream, scaled);
    writeSize(&stream, (u16)(n4 + writeByte(&stream, (u32)d)));
    term = a;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

extern "C" void NetworkSessionStable_send8or9(NetworkSessionStable* self, u32 has_extra, s8 idx)
{
    NetworkStreamWriter stream;
    s8 term;
    u16 n;

    fn_803CB9B4(&stream);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    if (has_extra != 0) {
        n = writeByte(&stream, 9);
    } else {
        n = writeByte(&stream, 8);
    }
    n = (u16)(n + writeUInt(&stream, self->slots_14838[idx].playerId_40));
    writeSize(&stream, (u16)(n + writeUInt(&stream, self->tick_16CD8)));
    term = idx;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

extern "C" void NetworkSessionStable_send4(NetworkSessionStable* self, const void* data, u32 len, s8 idx)
{
    NetworkStreamWriter stream;
    s8 term;
    u16 n;

    fn_803CB9B4(&stream);
    fn_803F89D0(&stream, self->sendBuffer_0D, 0x400);
    fn_803F8A14(&stream, 0);
    n = writeByte(&stream, 4);
    writeSize(&stream, (u16)(n + writeBytes(&stream, data, len)));
    term = idx;
    NetworkSessionStable_sendStream(self, &stream, 0, 1, &term, 0xFF);
    dtor_803CB958(&stream, -1);
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionStable - performance / state                                                */
/* ----------------------------------------------------------------------------------------- */

extern "C" void NetworkSessionStable_updateRate(NetworkSessionStable* self, s8 idx)
{
    NetworkSessionSlot* slot;
    f32 a;
    f32 b;
    f32 r;

    if (NetworkSessionStable_getUsableSlot(self, idx) < 0) {
        return;
    }
    a = networkRateScale * self->vtable->getFloat_8C(self, idx);
    slot = &self->slots_14838[idx];
    b = networkRateScale * slot->rate2_BC;
    r = a;
    if (b > r) {
        r = b;
    }
    if (r < networkRateMin) {
        r = networkRateMin;
    }
    if (r > networkRateMax) {
        r = networkRateMax;
    }
    slot->rate_B8 = r;
}

/* Runs once every tick: decays the slot's pack interval towards the floor and reports it. */
extern "C" void NetworkSessionStable_downPerformance(NetworkSessionStable* self, s8 idx)
{
    NetworkSessionSlot* slot;
    NetworkSessionManagerLogger* log;
    s32 step;
    f32 t;
    f32 delta;

    slot = &self->slots_14838[idx];
    t = self->time_16CDC;
    if (t < slot->limit2_D8 + networkRateMax) {
        return;
    }
    slot->limit2_D8 = t;
    if (slot->flag_D0 == 0) {
        slot->flag_D0 = 1;
        slot->base_C8 = slot->rate2_BC;
        slot->limit_CC = slot->counter_C0;
    }
    slot->last_C4 = self->time_16CDC;
    if (slot->rate2_BC < self->limit_16CF8) {
        delta = self->limit_16CF8 - slot->rate2_BC;
        delta = delta * networkRateUpLerp;
        if (delta < networkRateDownStep) {
            slot->rate2_BC = self->limit_16CF8;
        } else {
            slot->rate2_BC = slot->rate2_BC + delta;
        }
        log = (NetworkSessionManagerLogger*)getNetworkLogger();
        log->vtable->verbose_0C(log, 1, NetworkSessionStable_downPerformancePackMessage, (u32)idx,
                                slot->rate2_BC);
    }
    if (slot->counter_C0 > self->count_16CFC) {
        step = (slot->counter_C0 - self->count_16CFC) / 2;
        if (step < 16) {
            slot->counter_C0 = self->count_16CFC;
        } else {
            slot->counter_C0 = slot->counter_C0 - step;
        }
        log = (NetworkSessionManagerLogger*)getNetworkLogger();
        log->vtable->verbose_0C(log, 1, NetworkSessionStable_downPerformanceByteMessage, (u32)idx,
                                slot->counter_C0);
    }
}

/* Runs once every tick: grows the slot's pack interval towards the ceiling and reports it. */
extern "C" void NetworkSessionStable_upPerformance(NetworkSessionStable* self, s8 idx)
{
    NetworkSessionSlot* slot;
    NetworkSessionManagerLogger* log;
    f32 t;

    slot = &self->slots_14838[idx];
    t = self->time_16CDC;
    if (t < slot->accel_D4 + networkRateMax) {
        return;
    }
    slot->accel_D4 = t;
    if (slot->last_C4 + networkRateMax < t) {
        slot->last_C4 = t;
        slot->base_C8 = slot->base_C8 - networkRateDecay;
        if (slot->base_C8 < networkRateFloor) {
            slot->base_C8 = networkRateFloor;
        }
        slot->limit_CC = slot->limit_CC + 4;
        if (slot->limit_CC > 1024) {
            slot->limit_CC = 1024;
        }
    }
    if (slot->flag_D0 != 0) {
        slot->flag_D0 = 0;
        slot->base_C8 = slot->base_C8 + (slot->rate2_BC - slot->base_C8) * networkRateDownLerp;
        slot->limit_CC = slot->limit_CC - (slot->limit_CC - slot->counter_C0) / 4;
    }
    if (slot->base_C8 < slot->rate2_BC) {
        if (slot->rate2_BC < slot->base_C8 + networkRateUpStep) {
            slot->rate2_BC = slot->base_C8;
        } else {
            slot->rate2_BC = slot->rate2_BC - (slot->rate2_BC - slot->base_C8) * networkRateUpLerp;
        }
        log = (NetworkSessionManagerLogger*)getNetworkLogger();
        log->vtable->verbose_0C(log, 1, NetworkSessionStable_upPerformancePackMessage, (u32)idx,
                                slot->rate2_BC);
    }
    if (slot->limit_CC < slot->counter_C0) {
        if (slot->limit_CC - 32 < slot->counter_C0) {
            slot->counter_C0 = slot->limit_CC;
        } else {
            slot->counter_C0 = slot->counter_C0 + (slot->limit_CC - slot->counter_C0) / 2;
        }
        log = (NetworkSessionManagerLogger*)getNetworkLogger();
        log->vtable->verbose_0C(log, 1, NetworkSessionStable_upPerformanceByteMessage, (u32)idx,
                                slot->counter_C0);
    }
}

/* Packs the slot's current record into the session's stream buffer and hands it to the socket. */
extern "C" void NetworkSessionStable_move(NetworkSessionStable* self, s8 idx)
{
    NetworkStreamWriterDefault stream;
    NetworkSessionSlot* slot;
    NetworkSessionSlot* used;
    NetworkBuffer* buffer;
    NetworkSessionManagerLogger* log;
    s8 slotIndex;
    s32 size44;
    s32 size5C;

    networkStreamWriter_constructDefault(&stream);
    slotIndex = NetworkSessionStable_getUsableSlot(self, idx);
    if (slotIndex < 0) {
        dtor_803CB8FC(&stream, -1);
        return;
    }
    slot = &self->slots_14838[slotIndex];
    buffer = slot->active_1C;
    networkStreamWriter_putBytes(&stream, self->sendBuffer_0D, 0x400);
    networkStreamWriter_flush(&stream);
    networkStreamWriter_setMode(&stream, 0);
    used = &self->slots_14838[idx];
    networkStreamWriter_putU16(&stream, networkStreamWriter_size(&used->bits_44) & 0xFFFF);
    networkStreamWriter_putU16b(&stream, networkStreamWriter_size(&used->bits_5C) & 0xFFFF);
    networkStreamWriter_putU32(&stream, self->tick_16CD8);
    networkStreamWriter_putU32b(&stream, used->playerId_40);
    networkStreamWriter_enable1(&stream, 1);
    networkStreamWriter_enable2(&stream, 1);
    networkStreamWriter_enable3(&stream, 1);
    networkStreamWriter_commit(&stream);
    networkStreamWriter_bytes(&stream);
    networkStreamWriter_attach(buffer, &stream);
    networkStreamWriter_reserve(buffer, 0, 0, 0);
    size5C = networkStreamWriter_size(&used->bits_5C) & 0xFFFF;
    size44 = networkStreamWriter_size(&used->bits_44) & 0xFFFF;
    log = (NetworkSessionManagerLogger*)getNetworkLogger();
    log->vtable->verbose_0C(log, 3, NetworkSessionStable_moveOutOfBandMessage,
                            (u32)self->field_14826, (u32)slotIndex, (u32)idx, (u32)size44, (u32)size5C);
    dtor_803CB8FC(&stream, -1);
}

/* Resolves the slot the session should transmit on: the slot itself, else its owning slot. */
extern "C" s8 NetworkSessionStable_getUsableSlot(NetworkSessionStable* self, s8 idx)
{
    NetworkSessionSlot* slot;
    NetworkSessionSlot* sub;
    s8 subIdx;

    if (idx < 0 || idx >= 4) {
        return -1;
    }
    slot = &self->slots_14838[idx];
    if (slot->active_1C == 0) {
        return -1;
    }
    if (slot->linked_09 != 0) {
        return idx;
    }
    subIdx = slot->ownerIndex_00;
    if (subIdx < 0) {
        return -1;
    }
    sub = &self->slots_14838[subIdx];
    if (sub->active_1C == 0) {
        return -1;
    }
    if (sub->linked_09 != 0) {
        return subIdx;
    }
    if (sub->ready_0B != 0) {
        return subIdx;
    }
    return -1;
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - construction / pool                                                */
/* ----------------------------------------------------------------------------------------- */

NetworkSessionManager::NetworkSessionManager()
{
    s32 i;

    __construct_array(&this->pool_7C[0], (void*)NetworkRequest_construct, (void*)NetworkRequest_deleteElement, 0xA4, 2);
    this->unused_04 = 0;
    this->unused_08 = 0;
    this->buffer = 0;
    for (i = 0; i < 21; i++) {
        this->requests_10[i] = 0;
        this->request_state_64[i] = 0;
    }
    this->unused_79 = 1;
    this->unused_7A = 1;
    for (i = 0; i < 2; i++) {
        NetworkRequest_reset(&this->pool_7C[i]);
    }
}

extern "C" void NetworkRequest_reset(NetworkRequest* self)
{
    self->state_00 = 0;
    self->interval_4C = networkRequestZero;
    self->timeout_50 = networkRequestZero;
    self->requestId_70 = 0;
    self->unused_24 = 0;
    self->cancelled_74 = 0;
    self->owner_94 = 0;
    self->desc_98 = NetworkRequest_defaultDescriptor[0];
    self->desc_9C = NetworkRequest_defaultDescriptor[1];
    self->desc_A0 = NetworkRequest_defaultDescriptor[2];
    self->count_28 = 0;
    self->record_54 = 0;
    self->record_58 = 0;
    self->record_5C = 0;
    self->unused_60 = 0;
    self->unused_64 = 0;
    self->unused_68 = 0;
    self->unused_6C = 0;
    self->unused_04 = 0;
    self->unused_08 = 0;
    self->buffer = 0;
    self->unused_10 = 0;
    self->unused_14 = 0;
    self->unused_18 = 0;
    self->unused_1C = 0;
    self->unused_20 = 0;
    self->args_2C[0] = 0;
    self->args_2C[1] = 0;
    self->args_2C[2] = 0;
    self->args_2C[3] = 0;
    self->args_2C[4] = 0;
    self->args_2C[5] = 0;
    self->args_2C[6] = 0;
    self->args_2C[7] = 0;
}

extern "C" void* NetworkRequest_deleteElement(NetworkRequest* self, s16 flags)
{
    if (self != 0) {
        NetworkRequest_reset(self);
        dtor_803CA338(self->mutex_78, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" void NetworkRequest_clear(NetworkRequest* self)
{
    NetworkRequest_reset(self);
}

extern "C" NetworkRequest* NetworkRequest_construct(NetworkRequest* self)
{
    networkInstance_initMutex(self->mutex_78);
    NetworkRequest_reset(self);
    return self;
}

NetworkSessionManager::~NetworkSessionManager()
{
    NetworkSessionManager::release();
    __destroy_arr(&this->pool_7C[0], (void*)NetworkRequest_deleteElement, 0xA4, 2);
}


void NetworkSessionManager::init(u32 a, u32 b)
{
    this->unused_04 = a;
    this->unused_08 = b;
    NetworkSessionManager::clear();
}

void NetworkSessionManager::clear()
{
    s32 i;

    this->buffer = 0;
    for (i = 0; i < 21; i++) {
        this->requests_10[i] = 0;
        this->request_state_64[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        NetworkRequest_reset(&this->pool_7C[i]);
    }
}

void NetworkSessionManager::release()
{
    NetworkBuffer* buf;
    s32 i;

    buf = this->buffer;
    if (buf != 0) {
        buf->end();
        buf = this->buffer;
        if (buf != 0) {
            buf->destroy(1);
            this->buffer = 0;
        }
    }
    for (i = 0; i < 0x15; i++) {
        NetworkSessionManager_deleteRequest(this, &this->requests_10[i]);
    }
    for (i = 0; i < 2; i++) {
        NetworkRequest_clear(&this->pool_7C[i]);
    }
}

/* One tick of the request state machine: opens the slot's request, resolves the ones that may
   move this tick and retires the ones whose target has gone away. */
void NetworkSessionManager::move()
{
    NetworkRequest* req;
    NetworkSessionManagerLogger* log;
    NetworkBuffer* buf;
    s32 i;

    buf = this->buffer;
    if (buf != 0) {
        buf->end();
    }
    for (i = 0; i < 21; i++) {
        req = this->requests_10[i];
        if (req != 0 && this->request_state_64[i] == 0) {
            if (i != 2 && this->requests_10[2] != 0) {
                continue;
            }
            switch (i) {
            case 1:
                if (this->request_state_64[2] != 0) {
                    continue;
                }
                break;
            case 2:
                {
                    s32 j;
                    for (j = 0; j < 21; j++) {
                        if (this->request_state_64[j] != 0) {
                            break;
                        }
                    }
                    if (j == 21) {
                        break;
                    }
                    log = (NetworkSessionManagerLogger*)getNetworkLogger();
                    log->vtable->log_14(log, NetworkSessionManager_moveStandByMessage, j);
                    continue;
                }
            case 3:
            case 6:
                if (this->request_state_64[11] != 0 || this->request_state_64[12] != 0 ||
                    this->request_state_64[2] != 0 || this->request_state_64[1] != 0) {
                    continue;
                }
                break;
            case 12:
                if (this->request_state_64[11] != 0) {
                    continue;
                }
                /* falls through */
            case 11:
                if (this->request_state_64[2] != 0 || this->request_state_64[1] != 0 ||
                    this->request_state_64[3] != 0 || this->request_state_64[6] != 0 ||
                    this->request_state_64[10] != 0) {
                    continue;
                }
                break;
            case 10:
                if (this->request_state_64[11] != 0) {
                    continue;
                }
                break;
            default:
                break;
            }
            this->request_state_64[i] = 1;
        }
        if (req != 0 && this->request_state_64[i] != 0) {
            zz_03d5084_ptmf_scall(req);
            if (NetworkRequest_isOwned(req) == 0) {
                NetworkSessionManager_deleteRequest(this, &this->requests_10[i]);
                this->request_state_64[i] = 0;
            }
        }
    }
}

#pragma dont_inline on
extern "C" s32 NetworkRequest_isOwned(NetworkRequest* self)
{
    return self->owner_94 != 0;
}
#pragma dont_inline off

extern "C" void zz_03d5084_ptmf_scall(NetworkRequest* self)
{
    if (self->owner_94 != 0 && __ptmf_scall(self) != 0) {
        NetworkRequest_clear(self);
    }
}

#pragma peephole off
void NetworkSessionManager::request364()
{
    NetworkRequest* req;

    if (this->requests_10[1] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[1] = req;
            NetworkRequest_begin(req, this, networkRequestDesc364, 0);
        }
    }
}
#pragma peephole on

extern "C" void NetworkRequest_begin(NetworkRequest* req, NetworkSessionManager* owner,
                            NetworkRequestDesc desc, u32 count, ...)
{
    NetworkVaState args;
    NetworkSessionManagerLogger* log;
    u32 i;

    NetworkRequest_reset(req);
    log = (NetworkSessionManagerLogger*)getNetworkLogger();
    req->timeout_50 = log->vtable->getTime_60(log);
    req->requestId_70 = NetworkRequest_idCounter;
    NetworkRequest_idCounter = req->requestId_70 + 1;
    req->owner_94 = owner;
    req->desc_98 = desc.id_0;
    req->desc_9C = desc.value_4;
    req->desc_A0 = desc.type_8;
    req->count_28 = count > 8 ? 8 : count;
    net_va_start(args);
    for (i = 0; i < req->count_28; i++) {
        req->args_2C[i] = net_va_arg(args, u32);
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - the lazy request allocators                                        */
/* ----------------------------------------------------------------------------------------- */

#pragma peephole off
void NetworkSessionManager::request368()
{
    NetworkRequest* req;

    if (this->requests_10[2] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[2] = req;
            NetworkRequest_begin(req, this, networkRequestDesc368, 0);
        }
    }
}

s32 NetworkSessionManager::hasBuffer()
{
    return this->buffer != 0;
}

void NetworkSessionManager::request372(u32 a, u32 b)
{
    NetworkRequest* req;

    if (this->requests_10[3] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[3] = req;
            NetworkRequest_begin(req, this, networkRequestDesc372, 2, a, b);
        }
    }
}

void NetworkSessionManager::request376(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[4] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[4] = req;
            NetworkRequest_begin(req, this, networkRequestDesc376, 1, a);
        }
    }
}

void NetworkSessionManager::request380(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[5] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[5] = req;
            NetworkRequest_begin(req, this, networkRequestDesc380, 1, a);
        }
    }
}

void NetworkSessionManager::request384(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[6] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[6] = req;
            NetworkRequest_begin(req, this, networkRequestDesc384, 1, a);
        }
    }
}

void NetworkSessionManager::request388(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[6] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[6] = req;
            NetworkRequest_begin(req, this, networkRequestDesc388, 1, a);
        }
    }
}

void NetworkSessionManager::request432()
{
    NetworkRequest* req;

    if (this->requests_10[7] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[7] = req;
            NetworkRequest_begin(req, this, networkRequestDesc432, 0);
        }
    }
}

void NetworkSessionManager::request416(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[16] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[16] = req;
            NetworkRequest_begin(req, this, networkRequestDesc416, 1, a);
        }
    }
}

void NetworkSessionManager::request420(s8 a)
{
    NetworkRequest* req;

    if (this->requests_10[17] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[17] = req;
            NetworkRequest_begin(req, this, networkRequestDesc420, 1, (u32)a);
        }
    }
}

void NetworkSessionManager::request424()
{
    NetworkRequest* req;

    if (this->requests_10[20] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[20] = req;
            NetworkRequest_begin(req, this, networkRequestDesc424, 0);
        }
    }
}

void NetworkSessionManager::request404(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[8] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[8] = req;
            NetworkRequest_begin(req, this, networkRequestDesc404, 1, a);
        }
    }
}

void NetworkSessionManager::request436()
{
    NetworkRequest* req;

    if (this->requests_10[9] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[9] = req;
            NetworkRequest_begin(req, this, networkRequestDesc436, 0);
        }
    }
}

void NetworkSessionManager::request440()
{
    NetworkRequest* req;

    if (this->requests_10[19] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[19] = req;
            NetworkRequest_begin(req, this, networkRequestDesc440, 0);
        }
    }
}

void NetworkSessionManager::request444()
{
    NetworkRequest* req;

    if (this->requests_10[10] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[10] = req;
            NetworkRequest_begin(req, this, networkRequestDesc444, 0);
        }
    }
}

void NetworkSessionManager::request408()
{
    NetworkRequest* req;

    if (this->requests_10[11] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[11] = req;
            NetworkRequest_begin(req, this, networkRequestDesc408, 0);
        }
    }
}

void NetworkSessionManager::request412(u32 a, u32 b, s8 c)
{
    NetworkRequest* req;

    if (this->requests_10[15] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[15] = req;
            NetworkRequest_begin(req, this, networkRequestDesc412, 3, a, b, (u32)c);
        }
    }
}

void NetworkSessionManager::request392()
{
    NetworkRequest* req;

    if (this->requests_10[12] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[12] = req;
            NetworkRequest_begin(req, this, networkRequestDesc392, 0);
        }
    }
}

void NetworkSessionManager::request396(u32 a, u32 b)
{
    NetworkRequest* req;

    if (this->requests_10[13] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[13] = req;
            NetworkRequest_begin(req, this, networkRequestDesc396, 2, a, b);
        }
    }
}

void NetworkSessionManager::request400(u32 a, u32 b)
{
    NetworkRequest* req;

    if (this->requests_10[14] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[14] = req;
            NetworkRequest_begin(req, this, networkRequestDesc400, 2, a, b);
        }
    }
}

void NetworkSessionManager::request428(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[18] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[18] = req;
            NetworkRequest_begin(req, this, networkRequestDesc428, 1, a);
        }
    }
}
#pragma peephole on

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - accessors / small virtuals                                         */
/* ----------------------------------------------------------------------------------------- */

void NetworkSessionManager::abortRequest4()
{
    NetworkRequest* req = this->requests_10[4];

    if (req != 0 && NetworkRequest_isCancelled(req) == 0) {
        NetworkRequest_cancel(req);
    }
}

#pragma dont_inline on
extern "C" void NetworkRequest_cancel(NetworkRequest* self)
{
    self->cancelled_74 = 1;
}

extern "C" s32 NetworkRequest_isCancelled(NetworkRequest* self)
{
    return self->cancelled_74;
}
#pragma dont_inline off

void NetworkSessionManager::abortRequest14()
{
    NetworkRequest* req = this->requests_10[14];

    if (req != 0 && NetworkRequest_isCancelled(req) == 0) {
        NetworkRequest_cancel(req);
    }
}

void NetworkSessionManager::setFlag79(s8 value)
{
    this->unused_79 = value;
}

void NetworkSessionManager::notify(s32 value)
{
    NetworkSessionStable_setNotifyValue(value);
}

void NetworkSessionManager::setFlag7A(s8 value)
{
    this->unused_7A = value;
}

s32 NetworkSessionManager::getInt(s8 value)
{
    if (this->buffer == 0) {
        return 0;
    }
    return this->buffer->getInt(mapId_1C0(value));
}

f32 NetworkSessionManager::getFloat(s8 value)
{
    if (this->buffer == 0) {
        return networkRequestZero;
    }
    return this->buffer->getFloat(mapId_1C0(value));
}

void NetworkSessionManager::broadcastPlayerSlots(u32 a, u32 b)
{
    s8 data[4];

    data[0] = 0;
    data[1] = 1;
    data[2] = 2;
    data[3] = 3;
    sendBatch_138(a, b, 4, data);
}

void NetworkSessionManager::putTerminatorA(u32 a, u32 b, u8 c)
{
    s8 data;
    NetworkBuffer* buf;

    data = -1;
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 1, 1, &data, c);
    }
}

void NetworkSessionManager::putTerminatorB(u32 a, u32 b)
{
    s8 data;
    NetworkBuffer* buf;

    data = -2;
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 0, 1, &data, 0xFF);
    }
}

void NetworkSessionManager::putTerminatorC(u32 a, u32 b, u8 c)
{
    s8 data;
    NetworkBuffer* buf;

    data = -2;
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 1, 1, &data, c);
    }
}

/* Maps each requested player slot and puts the batch on the current stream buffer. */
void NetworkSessionManager::sendBatch_138(u32 a, u32 b, s32 count, const s8* data)
{
    s8 mapped[8];
    s32 n;
    s32 i;

    n = 0;
    for (i = 0; i < count; i++) {
        s8 value = mapId_1C0(data[i]);
        if (value >= 0) {
            mapped[n++] = value;
        }
    }
    if (this->buffer != 0) {
        this->buffer->put(a, b, 0, n, mapped, 0xFF);
    }
}

/* The same batch write, addressed to an explicit packet header and with the caller's flags. */
void NetworkSessionManager::slot_13C(u32 a, u32 b, s32 count, const s8* data, u8 flags)
{
    s8 mapped[8];
    s32 n;
    s32 i;

    n = 0;
    for (i = 0; i < count; i++) {
        s8 value = mapId_1C0(data[i]);
        if (value >= 0) {
            mapped[n++] = value;
        }
    }
    if (this->buffer != 0) {
        this->buffer->put(a, b, 1, n, mapped, flags);
    }
}

/* Maps one player slot and puts the single byte on the current stream buffer. */
void NetworkSessionManager::slot_140(u32 a, u32 b, s8 idx)
{
    s8 data;
    NetworkBuffer* buf;

    data = mapId_1C0(idx);
    if (data < 0) {
        return;
    }
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 0, 1, &data, 0xFF);
    }
}

/* The same single-slot write with the caller's flags. */
void NetworkSessionManager::slot_144(u32 a, u32 b, s8 idx, u8 flags)
{
    s8 data;
    NetworkBuffer* buf;

    data = mapId_1C0(idx);
    if (data < 0) {
        return;
    }
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 1, 1, &data, flags);
    }
}

void NetworkSessionManager::flush()
{
    if (this->buffer != 0 && canSend_28() != 0) {
        this->buffer->flush();
    }
}

void NetworkSessionManager::slot_14C()
{
    if (this->buffer != 0) {
        this->buffer->slot_30();
    }
}

void NetworkSessionManager::slot_150()
{
    if (this->buffer != 0) {
        this->buffer->slot_34();
    }
}

void NetworkSessionManager::slot_154()
{
    if (this->buffer != 0) {
        this->buffer->slot_50();
    }
}

void NetworkSessionManager::slot_158()
{
    if (this->buffer != 0) {
        this->buffer->slot_54();
    }
}

void NetworkSessionManager::slot_15C()
{
    if (this->buffer != 0) {
        this->buffer->slot_60();
    }
}

void NetworkSessionManager::slot_160()
{
    if (this->buffer != 0) {
        this->buffer->slot_64();
    }
}

void NetworkSessionManager::slot_164()
{
    if (this->buffer != 0) {
        this->buffer->slot_68();
    }
}

void NetworkSessionManager::slot_168()
{
    if (this->buffer != 0) {
        this->buffer->slot_5C();
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - pool + mutex helpers                                               */
/* ----------------------------------------------------------------------------------------- */

extern "C" NetworkRequest* NetworkSessionManager_allocRequest(NetworkSessionManager* self)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (NetworkRequest_isOwned(&self->pool_7C[i]) == 0) {
            return &self->pool_7C[i];
        }
    }
    return 0;
}

extern "C" void NetworkSessionManager_deleteRequest(NetworkSessionManager* self, NetworkRequest** slot)
{
    if (*slot != 0) {
        if (NetworkRequest_isOwned(*slot) != 0) {
            NetworkSessionManagerLogger* log = (NetworkSessionManagerLogger*)getNetworkLogger();
            log->vtable->log_14(log, NetworkSessionManager_deleteRequestMessage);
        }
        NetworkRequest_clear(*slot);
    }
    *slot = 0;
}

extern "C" s32 NetworkRequest_getRecord(NetworkRequest* self, u32* out)
{
    s32 result;

    result = 0;
    LockMutex(self->mutex_78);
    if (self->record_54 != 0) {
        result = 1;
        out[0] = self->record_54;
        out[1] = self->record_58;
        out[2] = self->record_5C;
    }
    UnlockMutex(self->mutex_78);
    return result;
}

extern "C" void NetworkRequest_setRecord(NetworkRequest* self, u32 a, u32 b, u32 c)
{
    LockMutex(self->mutex_78);
    self->record_58 = b;
    self->record_5C = c;
    self->record_54 = a;
    UnlockMutex(self->mutex_78);
}

extern "C" s32 NetworkRequest_getArgument(NetworkRequest* self, u32 idx)
{
    u32 count;
    NetworkSessionManagerLogger* log;

    count = self->count_28;
    if (count <= idx) {
        log = (NetworkSessionManagerLogger*)getNetworkLogger();
        log->vtable->warn_10(log, NetworkRequest_getArgumentMessage, count, idx);
        return 0;
    }
    return (s32)self->args_2C[idx];
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkRequest timers                                                                     */
/* ----------------------------------------------------------------------------------------- */

/* True once the request has been waiting longer than its own interval. */
extern "C" s32 NetworkRequest_isTimedOut(NetworkRequest* self)
{
    NetworkSessionManagerLogger* log;
    s32 result;

    result = 0;
    LockMutex(self->mutex_78);
    if (self->interval_4C != networkRequestTimerIdle) {
        log = (NetworkSessionManagerLogger*)getNetworkLogger();
        if (log->vtable->getTime_60(log) - self->timeout_50 > self->interval_4C) {
            result = 1;
        }
    }
    UnlockMutex(self->mutex_78);
    return result;
}

/* Restarts the wait: the current clock becomes the baseline and the interval is replaced. */
extern "C" void NetworkRequest_restartTimer(NetworkRequest* self, f32 interval)
{
    NetworkSessionManagerLogger* log;

    LockMutex(self->mutex_78);
    log = (NetworkSessionManagerLogger*)getNetworkLogger();
    self->timeout_50 = log->vtable->getTime_60(log);
    self->interval_4C = interval;
    UnlockMutex(self->mutex_78);
}

/* Moves the 0x60-byte record block an out-of-band request carries. */
extern "C" void NetworkRequest_copyRecord(NetworkSessionRecordBlock* dst, const NetworkSessionRecordBlock* src)
{
    *dst = *src;
}

/* ----------------------------------------------------------------------------------------- */
/* The small per-player records the Pat layer owns                                            */
/* ----------------------------------------------------------------------------------------- */

/* The four record classes below each hold one `networkSmallObject_construct` member; the ctor and
   the deleting element destructor are the pair their containers pass to `__construct_array`. */
extern "C" NetworkSessionSlotInfo* NetworkSessionSlotInfo_construct(NetworkSessionSlotInfo* self)
{
    networkSmallObject_construct(&self->smallObject_00);
    return self;
}

extern "C" NetworkSessionSlotInfo* NetworkSessionSlotInfo_dtor(NetworkSessionSlotInfo* self, s16 flags)
{
    if (self != 0) {
        networkSmallObject_dtor(&self->smallObject_00, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* The reflection callbacks: both re-order the six incoming arguments and tail-call the real body. */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
extern "C" void networkSessionReflectCallback(void* a0, void* a1, s8 a2, void* a3, void* a4, void* a5)
{
    networkSessionReflect0(a5, a0, a2, a3, a4, a1);
}

/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
extern "C" void networkSessionReflectCallbackEx(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5)
{
    networkSessionReflect1(a5, a0, a1, a2, a3, a4);
}

extern "C" NetworkSessionCircleList* NetworkSessionCircleList_dtor(NetworkSessionCircleList* self, s16 flags)
{
    if (self != 0) {
        __destroy_arr(&self->items_04[0], (void*)NetworkSessionCircleInfo_dtor, 796, 32);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" NetworkSessionCircleInfo* NetworkSessionCircleInfo_construct(NetworkSessionCircleInfo* self)
{
    networkSmallObject_construct(&self->smallObject_108);
    return self;
}

extern "C" NetworkSessionCircleInfo* NetworkSessionCircleInfo_dtor(NetworkSessionCircleInfo* self, s16 flags)
{
    if (self != 0) {
        networkSmallObject_dtor(&self->smallObject_108, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" GameSpyInterfaceThread* GameSpyInterfaceThread_getInstance(void)
{
    return (GameSpyInterfaceThread*)sGameSpyInterfaceThread;
}

extern "C" NetworkSessionCircleList* NetworkSessionCircleList_construct(NetworkSessionCircleList* self)
{
    __construct_array(&self->items_04[0], (void*)NetworkSessionCircleInfo_construct,
                      (void*)NetworkSessionCircleInfo_dtor, 796, 32);
    return self;
}

extern "C" NetworkSessionPlayerRecord* NetworkSessionPlayerRecord_dtor(NetworkSessionPlayerRecord* self, s16 flags)
{
    if (self != 0) {
        networkSmallObject_dtor(&self->smallObject_08, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" NetworkSessionPlayerRecord* NetworkSessionPlayerRecord_construct(NetworkSessionPlayerRecord* self)
{
    networkSmallObject_construct(&self->smallObject_08);
    return self;
}

/* ----------------------------------------------------------------------------------------- */
/* The Pat layer's own NetworkRequest array                                                   */
/* ----------------------------------------------------------------------------------------- */

extern "C" NetworkRequest* NetworkRequestPat_dtor(NetworkRequest* self, s16 flags)
{
    if (self != 0) {
        NetworkRequestPat_clear(self);
        dtor_803CA338(self->mutex_78, -1);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

extern "C" void NetworkRequestPat_clear(NetworkRequest* self)
{
    NetworkRequestPat_reset(self);
}

extern "C" void NetworkRequestPat_reset(NetworkRequest* self)
{
    self->state_00 = 0;
    self->interval_4C = networkRequestTimerReset;
    self->timeout_50 = networkRequestTimerReset;
    self->requestId_70 = 0;
    self->unused_24 = 0;
    self->cancelled_74 = 0;
    self->owner_94 = 0;
    self->desc_98 = NetworkRequest_defaultDescriptor[0];
    self->desc_9C = NetworkRequest_defaultDescriptor[1];
    self->desc_A0 = NetworkRequest_defaultDescriptor[2];
    self->count_28 = 0;
    self->record_54 = 0;
    self->record_58 = 0;
    self->record_5C = 0;
    self->unused_60 = 0;
    self->unused_64 = 0;
    self->unused_68 = 0;
    self->unused_6C = 0;
    self->unused_04 = 0;
    self->unused_08 = 0;
    self->buffer = 0;
    self->unused_10 = 0;
    self->unused_14 = 0;
    self->unused_18 = 0;
    self->unused_1C = 0;
    self->unused_20 = 0;
    self->args_2C[0] = 0;
    self->args_2C[1] = 0;
    self->args_2C[2] = 0;
    self->args_2C[3] = 0;
    self->args_2C[4] = 0;
    self->args_2C[5] = 0;
    self->args_2C[6] = 0;
    self->args_2C[7] = 0;
}

extern "C" NetworkRequest* NetworkRequestPat_construct(NetworkRequest* self)
{
    networkInstance_initMutex(self->mutex_78);
    NetworkRequestPat_reset(self);
    return self;
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManagerPat                                                                   */
/* ----------------------------------------------------------------------------------------- */

/* Builds the Pat side of the session: its own request pair, its player records and the circle
   list, then the two on-demand singletons the Pat layer drives. */
NetworkSessionManagerPat::NetworkSessionManagerPat()
{
    __construct_array(&this->pool2_1C4[0], (void*)NetworkRequestPat_construct,
                      (void*)NetworkRequestPat_dtor, 0xA4, 2);
    networkSmallObject_construct(&this->field_3CC);
    __construct_array(&this->players_538[0], (void*)NetworkSessionPlayerRecord_construct,
                      (void*)NetworkSessionPlayerRecord_dtor, 72, 4);
    NetworkSessionCircleList_construct(&this->circleList_AF0);
    if (getInstance_() == 0) {
        new PatInterface();
    }
    if (GameSpyInterfaceThread_getInstance() == 0) {
        new GameSpyInterfaceThread();
    }
    this->field_6E75 = 0;
    this->clear();
}

#pragma dont_inline on
NetworkSessionManagerPat::~NetworkSessionManagerPat()
{
    if (this != 0) {
        this->release();
        NetworkSessionCircleList_dtor(&this->circleList_AF0, -1);
        __destroy_arr(&this->players_538[0], (void*)NetworkSessionPlayerRecord_dtor, 72, 4);
        networkSmallObject_dtor(&this->field_3CC, -1);
        __destroy_arr(&this->pool2_1C4[0], (void*)NetworkRequestPat_dtor, 0xA4, 2);
    }
}

#pragma dont_inline on
/* Re-arms the Pat session for a new match: its player records, the -1 slot table, the circle list
   and the stream buffer's timing fields are all reset. */
void NetworkSessionManagerPat::clear()
{
    s32 i;

    this->NetworkSessionManager::clear();
    for (i = 0; i < 21; i++) {
        this->field_360[i] = -1;
    }
    this->field_3B8 = this->field_3BC = networkSessionPatTimeOrigin;
    this->field_3C0 = 0;
    this->field_3C1 = 0;
    this->field_3C2 = 0;
    this->field_3C3 = 0;
    this->field_3C4 = 0;
    this->field_3C8 = 0;
    this->field_3CC.vtable->slot_18(&this->field_3CC);
    memset(&this->field_3EC[0], 0, 48);
    this->tcp_658 = 0;
    this->udp_65C = 0;
    this->field_660 = 0;
    networkPatAttachBuffer((NetworkBuffer*)this);
    this->circleList_AF0.pad_00[0] = 0;
    for (i = 0; i < 32; i++) {
        networkPatResetCircleInfo(this, i);
    }
    this->field_6E74 = 0;
}

/* Starts a Pat session: the interface singletons, the base's own init and then a clear. */
#pragma dont_inline on
void NetworkSessionManagerPat::init(u32 a, u32 b)
{
    if (getInstance_() == 0) {
        new PatInterface();
    }
    this->NetworkSessionManager::init(a, b);
    if (GameSpyInterfaceThread_getInstance() == 0) {
        new GameSpyInterfaceThread();
    }
    this->field_6E75 = 0;
    this->clear();
}

/* Tears the Pat session down: drains the game-spy thread, then runs the base's release. */
void NetworkSessionManagerPat::release()
{
    GameSpyInterfaceThread* thread;
    PatInterface* pat;
    void* context;

    networkPatReleaseBuffer(this);
    if (getInstance_() != 0) {
        PatInterface_clear();
        pat = (PatInterface*)getInstance_();
        if (PatInterface_isReady() == 0) {
            pat = (PatInterface*)getInstance_();
            if (pat != 0) {
                pat->destroy(1);
            }
        }
    }
    context = (void*)this->field_660;
    if (context != 0) {
        networkLog_destroyContext((NetworkSessionManagerLogger*)getNetworkLogger(), context);
        this->field_660 = 0;
    }
    if (GameSpyInterfaceThread_getInstance() != 0) {
        if (this->field_6E75 != 0) {
            NetworkSessionManagerLogger* log = (NetworkSessionManagerLogger*)getNetworkLogger();
            log->vtable->warn_10(log, NetworkSessionManagerPat_finalMessage);
            this->field_6E75 = 0;
            thread = GameSpyInterfaceThread_getInstance();
            thread->canClose();
            thread = GameSpyInterfaceThread_getInstance();
            thread->armCancel();
        }
        thread = GameSpyInterfaceThread_getInstance();
        while (thread->requestClose()) {
            thread = GameSpyInterfaceThread_getInstance();
        }
        thread = GameSpyInterfaceThread_getInstance();
        if (thread != 0) {
            thread->destroy(1);
        }
    }
    this->NetworkSessionManager::release();
}
#pragma dont_inline off
