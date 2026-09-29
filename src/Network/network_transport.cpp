/*
 * Network/network_transport.cpp - the Network transport band, `.text` 0x803CCDF8..0x803D3CE8
 * (28400 B, 118 auto units).
 *
 * WHAT IT IS.  The DWC/GameSpy transport layer the game's network code sits on: the peer payload
 * buffers and their error records, the socket peers' `clearBuffer`/`setPeerAndSocket` family, the
 * `NetworkSingleTcp`/`NetworkMultipleUdp`/`NetworkPeerMcs`/`NetworkResolverWii` peers (their own log
 * strings name them, 0x805F9570 and following) and the `NetworkSessionStable` session state machine.
 * The registered neighbours are `Network/fn_803D3CE8.cpp` above (0x803D3CE8) and
 * `Network/initNetworkSessionStable.cpp` (0x803DEA30).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable: every
 * `lis`/`addi` pair in the range resolves to a vtable, a `.data` log string, a `.sdata`/`.sdata2`
 * float or a `.bss` scratch area, never to a source-file name.  2. `dumpmap.py lookup` answers only
 * `zz_XXXXXXXX_` for the code.  3. The vtables (0x805F94E0, 0x805F9510, 0x805F9570, 0x805F99A0,
 * 0x805FA6A8), the log strings and the registered neighbour place the band in `Network`.  The tile
 * spans more than one original TU, so no single evidenced file name covers it: the name is derived
 * from the range's dominant subject and is a **GUESS**.
 *
 * SEAM (unproven, recorded as such - docs/plan.md 8.3).  The right edge is proven: the range's
 * `extab` block 0x800198C8..0x80019E5C and its `extabindex` block 0x8003A0BC..0x8003A524 abut the
 * registered `Network/fn_803D3CE8.cpp` at 0x80019E5C / 0x8003A524 exactly.  The left edge 0x803CCDF8
 * is a discovery byte cap, not a boundary: `tudiscover.py at 0x803CCDF8` reports a one-function
 * closure with no must-link anchor and only weak cut signals on both sides, and the `.sdata2` pool
 * entries the range loads are shared with the band below it (0x8079C690 is read from 0x803CA930 and
 * 0x803CBEC0 as well as from inside this range), so the original TU very likely starts earlier.
 *
 * LANGUAGE.  C++ (`__dl__FPv`, virtual dispatch through the peer vtables, `bctr` tail slots);
 * `cflags_network`'s `-Cpp_exceptions on` is what makes the target's `extab`/`extabindex` appear.
 *
 * DATA - the measured blocker (rule 12, playbook 58).  The range's `.data` 0x805F94E0..0x805FA5xx is
 * a single interleaved run: the peer class tables (0x805F94E0, 0x805F9510, 0x805F9570, 0x805F99A0,
 * 0x805FA6A8) sit *between* the log strings, and each of those tables is a code-pointer run the
 * target object carries.  Claiming the run would make this unit the owner of all five tables, and
 * rule 10 requires an owned table to be **compiler-emitted** from the classes - which the band has
 * not reconstructed (its tables' slot sets span nine classes' worth of functions).  The run is
 * therefore deliberately **not** claimed, and the functions that load a log string or a table
 * address (about 40 of the 118, including `sendPackets`, `receivePackets`, the `NetworkSingleTcp`/
 * `NetworkMultipleUdp` slot managers and the whole `NetworkSessionStable::move`/`execControlOne`
 * pair) are left unwritten rather than declaring an `extern` for unowned data, which rule 12 refuses.
 * Filed as a range request through `tools/units/dataqueue.py`.
 *
 * NAMES (rule 7).  The class and function names come from the range's own evidence: the log strings
 * name their emitters (`NetworkSingleTcp::add`, `NetworkMultipleUdp::receive`, `NetworkPeerMcs::put`,
 * `NetworkSessionStable::init`/`move`/`send`), the vtables give each slot's position and so its role,
 * `docs/memory-dump.md` names `sendPackets`/`receivePackets`/`clearBuffer`/`setPeerAndSocket`/
 * `getAvailableToRead`/`networkSmallObject_dtor`, and `NetworkSessionStable_getOwnIndex` /
 * `NetworkSessionStable_init` were named by the neighbouring lanes.  Every name derived from
 * behaviour alone is marked **GUESS**: the `networkPeerError_*` record accessors (the callers pass a
 * table, a size and a code, so the record is an error record), `networkPeerBuffer_init` (the slot
 * that calls the class's clear and returns 1), the `NetworkSocketHandle` slot names (from their
 * offsets alone) and `NetworkSessionStable_sendStream` (named from the packet tail it is called with
 * and the "put send pool over" string its body loads).  The seven `dtor_XXXXXXXX` names are the
 * map's own - a `dtor_` stem is not a rule-7 pattern, and each of those destructors only chains the
 * base and frees, which pins no class down.
 *
 * FLAGS (brief section 8.2, instruction-level).  `-O3` like the two sibling session units - measured
 * on this source: the lib's `-O4,p` puts 14 rows at 100 % (unit 4.05), `-O3` puts 37 of 38 there
 * (unit 7.37).  `#pragma peephole off` **file-scope** is what makes the rows land: with the pass on,
 * 18 of 30 rows were 100 % and the rest kept retail's unfused forms folded away - a separate
 * `extsh`+`cmpwi` before the free became `extsh.` (the cmpwi absorbed), and the two `li`s of a
 * `memset` argument list were hoisted above the destination's `addi`.  With the pass off every one of
 * those rows is byte-identical (measured: 32/32 rows at 100 % in the same build).  Playbook 39/41 -
 * the pragma is the lever, the command line's `-opt nopeephole` is accepted and changes nothing.
 *
 * VTABLES (found this pass; they are what names the two peer classes).  Every one of these functions'
 * addresses occurs as a slot in one of the two `.data` tables at 0x805F9510 and 0x805F95E0 (a 4-byte
 * big-endian scan for the address over the DOL maps a function to its table and its slot offset, and
 * the log string that follows each table names the class): 0x805F9510 is the payload-buffer peer
 * (`NetworkPeerBuffer`, its slot 0x10, 0x1C, 0x24, 0x28, 0x3C, 0x40, 0x44, 0x48, 0x54 are the rows
 * below) and 0x805F95E0 the socket peer (`NetworkPeerSocket`, slots 0x0C..0x28).  This is why the four
 * 4-byte/8-byte rows are virtual slots with constant bodies rather than free functions, and why the
 * `slot0C`-style names carry a slot offset instead of a role: nothing in an empty body pins one.
 *
 * RESIDUAL.  Written: the 56 rows below (3584 B of the range's 28400 B), 55 of them byte-identical.
 *   - `networkPeerStream_takeRecord` 81.39 % (184 B, same size): the remaining diff is the second
 *     operand of the `||` guard - retail branches *forward* to the shared zero-store (`bgt`) where
 *     ours falls through - plus one `lhz` reload of the address-taken length local.  Both spellings
 *     tried (two separate guards, a combined `||`, a `raw` copy into a register local); the best is
 *     the one here (74.76 % -> 81.39 %).  The shape is the peephole pass's block layout, not a
 *     source form: recorded, not pursued.
 *   - the range still **references three symbols it does not define**, all unowned data the source
 *     declares rather than claims: `networkSocketPool_acquire`/`networkSocketPool_release` are
 *     defined (0x804187F0/0x80418864) in an unsplit band, and `NetworkSingleTcp_remove` (0x803CE470)
 *     is inside this range but still unwritten because its body loads one of the `.data` log strings.
 *     Harmless while the object is `NonMatching`; a link-time residual for whoever flips the unit.
 * Everything else in the range is unwritten.  The blocker for the majority of it is the `.data` run
 * above (about 40 functions load one of its strings or tables); the rest are the four vtable stores
 * (rule 10 - a table emitter needs the classes reconstructed first), the `@eti_800300XX` constants 18
 * functions load, and the `NetworkSessionStable`-slot rows below.  The `@eti_` constants are the hard
 * blocker and they are not a naming problem: `sendPackets` (0x803CD25C) loads 0x80030002 and
 * `fn_803CCF88` (0x803CCF88) 0x80030021, i.e. addresses **inside the extabindex table** - the DOL
 * really holds `lis r4,0x8003 / addi r4,r4,0x21` there, and the nearest map row is `@eti_80030018`
 * with addend 9, so the operand is not any object's data and no source spelling can produce it
 * (dtk's group-relative derivation, playbook 58's class).  18 of the range's functions carry one.
 *   - the `NetworkSessionStable` rows (`fn_803D1558`, `fn_803D2494`, `fn_803D2508`,
 *     `NetworkSessionStable_setConnectionInterval`, `fn_803D25xx`, `getSomething5`, 0x8F4 B in all)
 *     need fields **inside** `NetworkSessionStable`'s and `NetworkSessionSlot`'s padding in
 *     `include/Network/fn_803D3CE8.h` - the slots' per-slot header at slot-0x10 and their +0x04 float,
 *     and the session's +0x14810/+0x14824/+0x16CE4 words.  That header belongs to the neighbouring
 *     unit a later lane owns, so this pass left them unwritten rather than fork the type.
 *
 * DATA.  `.text` plus the object's `extab` (0x800198C8..0x80019E5C) and `extabindex`
 * (0x8003A0BC..0x8003A524) are claimed; no `.data`/`.sdata`/`.sdata2` run is, and the source emits
 * none (section lists compared: the object has `.text`, `extab` and `extabindex` only, so there is no
 * `ours-extra`).  Both unwind sections are claimed **partially** (ours 144/1428 and 216/1128 bytes):
 * the gap is exactly the unwritten functions' records, which is `target-extra` for a unit that is
 * still being written - a later lane closes it by writing them, never by dropping the claim.
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"
/* `unsplit/Network.h` is the Network band's code half: it declares `getNetworkLogger` and the two
   socket-pool helpers the peer teardown calls.  It cannot be included beside `unsplit/OS.h` - the two
   band headers declare `OSCreateThread`/`OSResumeThread` with different signatures (`OS.h` typed,
   `Network.h` untyped), and a TU that sees both fails with `(10197) illegal function overloading`.
   This file needs only `OSLockMutex`/`OSUnlockMutex`, which `Network.h` declares with the same shape,
   so `OS.h` stays out. */
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the
   memset argument setup in source order (`addi` before the two `li`s) - the peephole pass folds
   both.  Scoped off for the file until a row proves otherwise. */
#pragma peephole off

extern "C" {

/* ---------------------------------------------------------------------------------------------- */
/* the peer error record                                                                           */
/* ---------------------------------------------------------------------------------------------- */

/* Copies the three-word record out when the caller supplies somewhere to put it. */
void networkPeerError_get(NetworkPeerError* self, NetworkPeerErrorRecord* out)
{
    if (out != NULL) {
        out->source = self->source_04;
        out->argument = self->argument_08;
        out->code = self->code_0C;
    }
}

/* Empties the record so the next failure can fill it. */
void networkPeerError_clear(NetworkPeerError* self)
{
    memset(&self->source_04, 0, 0xC);
}

/* Records the first failure only: a record that is already filled is left alone. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code)
{
    NetworkPeerError* error = (NetworkPeerError*)self;

    if (error->source_04 == NULL) {
        error->source_04 = source;
        error->argument_08 = argument;
        error->code_0C = code;
    }
}

/* Deleting destructor of the error-record base: frees on request, nothing to chain. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void* dtor_803CCE9C(void* self, s32 flags)
{
    if (self != NULL) {
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* ---------------------------------------------------------------------------------------------- */
/* the small transport object                                                                      */
/* ---------------------------------------------------------------------------------------------- */

/* Deleting destructor of the small transport object: frees the owned block, then the object itself
   when the caller asks for it. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void* networkSmallObject_dtor(void* self, s32 flags)
{
    if (self != NULL) {
        dtor_803C989C(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Retail keeps the `bl` into the object's own destructor rather than folding it (the two live in
   different original objects), so the inline pass is off for this one function. */
#pragma dont_inline on
/* Deleting destructor of a small object embedded in a larger one: chains the object's own
   destructor with the "no free" flag, then frees on request. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void* networkSmallObject_destroy(void* self, s32 flags)
{
    if (self != NULL) {
        networkSmallObject_dtor(self, -1);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

#pragma dont_inline off

/* Constructor companion of the writer band's `networkSmallObject_construct`: builds in place. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void* networkSmallObject_init(void* self)
{
    networkSmallObject_construct(self);
    return self;
}

/* ---------------------------------------------------------------------------------------------- */
/* the payload buffer                                                                              */
/* ---------------------------------------------------------------------------------------------- */

/* Empties the payload and its use count; reports that the buffer is usable. */
s32 clearBuffer(NetworkPeerBuffer* self)
{
    memset(&self->payload_10, 0, 0x2000);
    self->used_2010 = 0;
    return 1;
}

/* Empties the payload buffer through the class's own clear slot. */
s32 networkPeerBuffer_init(NetworkPeerBuffer* self)
{
    self->clearPayload();
    return 1;
}

/* Empties the payload and its use count. */
void clearBuffer2(NetworkPeerBuffer* self)
{
    memset(&self->payload_10, 0, 0x2000);
    self->used_2010 = 0;
}

/* Retail keeps the `bl` into the base destructor (measured: folding it drops the whole call, since
   the inlined body's `flags > 0` test is constant-folded away). */
#pragma dont_inline on
/* Deleting destructor of the payload buffer: chains the error-record base, then frees on request. */
NetworkPeerBuffer* networkPeerBuffer_destroy(NetworkPeerBuffer* self, s32 flags)
{
    if (self != NULL) {
        dtor_803CCE9C(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

#pragma dont_inline off

/* Stores the peer/socket pair into the payload's head two words. */
void setPeerAndSocket(NetworkPeerBuffer* self, const u32* peerAndSocket)
{
    self->payload_10.peer_00 = peerAndSocket[0];
    self->payload_10.socket_04 = peerAndSocket[1];
}

/* ---------------------------------------------------------------------------------------------- */
/* the socket peer                                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* Asks the peer's socket how many bytes are readable; 0 when it holds no socket. */
s32 getAvailableToRead(NetworkPeerSocket* self)
{
    if (self->handle_04 != NULL) {
        return getBytesAvailableToRead(self->handle_04);
    }
    return 0;
}

/* Same question for the peer class that keeps its socket in the same slot (GUESS: identical body,
   the two classes' slot tables differ). */
s32 networkPeer_getAvailableToRead(NetworkPeerSocket* self)
{
    if (self->handle_04 != NULL) {
        return getBytesAvailableToRead(self->handle_04);
    }
    return 0;
}

/* Returns the socket the peer was opened on. */
NetworkSocketHandle* networkPeer_getSocket(NetworkPeerSocket* self)
{
    return self->handle_04;
}

/* Returns the identifier the peer's socket was registered with. */
u32 networkPeer_getPeerId(NetworkPeerSocket* self)
{
    return self->peerId_0C;
}

/* Closes the peer's socket through the socket's own vtable; -1 when there is no socket. */
s32 networkPeer_closeSocket(NetworkPeerSocket* self)
{
    if (self->handle_04 == NULL) {
        return -1;
    }
    return self->handle_04->closeSocket();
}

/* Clears the peer's socket receive buffer through the socket's own vtable; 0 when there is none. */
s32 networkPeer_clearReceiveSocket(NetworkPeerSocket* self)
{
    if (self->handle_04 == NULL) {
        return 0;
    }
    return self->handle_04->clearReceive();
}

/* Raises the deferred-drop flag of a peer that has something to drop. */
void networkPeer_armDrop(NetworkPeerSocket* self)
{
    if (self->armed_10 != 0) {
        self->dropped_18 = 1;
    }
}

/* Empties the peer's own 0x2400-byte receive area. */
void networkPeer_clearReceiveBuffer(NetworkPeerReceive* self)
{
    memset(self->recv_20, 0, 0x2400);
    self->recvUsed_2420 = 0;
}

/* ---------------------------------------------------------------------------------------------- */
/* the byte stream                                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* Appends one byte when the stream's cursor has room for it. */
void networkPeerStream_putByte(NetworkByteStream* self, u8 value)
{
    if (self->cursor_0C + 1 <= self->size_08) {
        self->data_04[self->cursor_0C] = value;
        self->cursor_0C++;
    }
}

/* Hands the stream's leading 0xE-byte record to a sink and drops it from the front. */
void networkPeerStream_forwardRecord(NetworkByteStream* self, NetworkPeerBuffer* sink)
{
    u32 remaining;

    if (self->cursor_0C < 0xE) {
        return;
    }
    if (sink->put(self->data_04, 0xE) > 0) {
        remaining = self->cursor_0C - 0xE;
        self->cursor_0C = remaining;
        if (remaining != 0) {
            memmove(self->data_04, self->data_04 + 0xE, remaining);
        }
    }
}

/* Copies the stream's leading length-prefixed record into the caller's record when both the stream
   and the caller have room, then drops it from the front. */
void networkPeerStream_takeRecord(NetworkByteStream* self, NetworkPeerRecord* record)
{
    u16 length;

    if (self->cursor_0C < 2) {
        return;
    }
    networkPeerStream_readLength(self, &length);
    if (length > self->cursor_0C || length > record->size_04) {
        record->size_04 = 0;
        return;
    }
    record->size_04 = length;
    if (length == 0) {
        return;
    }
    if (record->data_00 != NULL) {
        memcpy(record->data_00, self->data_04, length);
    }
    self->cursor_0C -= length;
    if (self->cursor_0C != 0) {
        memmove(self->data_04, self->data_04 + length, self->cursor_0C);
    }
}

/* Takes the stream's leading byte into the caller's byte and drops it from the front. */
void networkPeerStream_takeByte(NetworkByteStream* self, u8* out)
{
    u32 remaining;
    u8* base;

    if (self->cursor_0C < 1) {
        return;
    }
    base = self->data_04;
    *out = base[0];
    remaining = self->cursor_0C - 1;
    self->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(self->data_04, self->data_04 + 1, remaining);
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the peer name                                                                                   */
/* ---------------------------------------------------------------------------------------------- */

/* Publishes the peer's name (at most 0x1FF bytes): copies it, empties the record table and the
   error code, and refuses while the peer's rename guard is up. */
s32 networkPeer_setName(NetworkPeerConfig* self, const char* name)
{
    u32 length;

    if (strlen(name) < 0x1FF) {
        length = strlen(name);
    } else {
        length = 0x1FF;
    }
    memcpy(self->name_04, name, length);
    self->name_04[length] = 0;
    memset(self->pad_204, 0, 0x10);
    self->count_214 = 0;
    self->code_218 = 0;
    if (self->flag_1560 != 0) {
        self->code_218 = 0x5A;
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- */
/* the record table                                                                                */
/* ---------------------------------------------------------------------------------------------- */

/* Puts the peer's error code back to its idle value. */
void networkPeer_resetCode(NetworkPeerRecordTable* self)
{
    if (self->code_218 == 0) {
        self->code_218 = 0xFF;
    }
}

/* Copies one live record out of the peer's four-entry table. */
void networkPeer_recordGet(NetworkPeerRecordTable* self, s32 index, u32* out)
{
    if (index < 0) {
        return;
    }
    if ((s32)self->count_214 <= index) {
        return;
    }
    memcpy(out, &self->records_204[index], 4);
}

/* ---------------------------------------------------------------------------------------------- */
/* the peer classes' remaining destructors                                                         */
/* ---------------------------------------------------------------------------------------------- */

/* The same kept-`bl` shape as the destructors above: the inline pass is off across this block. */
#pragma dont_inline on
/* Deleting destructor: chains the base destructor, then frees on request. */
NetworkPeerError* dtor_803CD708(NetworkPeerError* self, s32 flags)
{
    if (self != NULL) {
        dtor_803CCE9C(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Deleting destructor: chains the base destructor, then frees on request. */
NetworkPeerError* dtor_803CD764(NetworkPeerError* self, s32 flags)
{
    if (self != NULL) {
        dtor_803CCE9C(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Deleting destructor of the peer state object: frees on request, nothing else to chain. */
NetworkPeerError* dtor_803CF108(NetworkPeerError* self, s32 flags)
{
    if (self != NULL) {
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Deleting destructor of the peer-side object: frees on request, nothing else to chain. */
NetworkPeerError* dtor_803CF694(NetworkPeerError* self, s32 flags)
{
    if (self != NULL) {
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Destroys the object's two owned sub-objects, then frees on request. */
NetworkPeerOwner* dtor_803CF8F4(NetworkPeerOwner* self, s32 flags)
{
    if (self != NULL) {
        dtor_803CA4E8(&self->sub_54, -1);
        networkSmallObject_destroy(&self->small_30, -1);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Deleting destructor of the session-side object: chains the small object's destructor. */
NetworkPeerError* dtor_803D14C0(NetworkPeerError* self, s32 flags)
{
    if (self != NULL) {
        dtor_803C989C(self, 0);
        if ((s16)flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

#pragma dont_inline off

/* ---------------------------------------------------------------------------------------------- */
/* the manager's mutex wrappers and the session accessor                                           */
/* ---------------------------------------------------------------------------------------------- */

/* The whole block below keeps its `bl`s: retail calls every one of these helpers out of line
   (`networkPeer_close` and the two forwarders would otherwise fold the teardown in, and the
   forwarders would stop being the 4-byte tail branches retail has). */
#pragma dont_inline on

/* Releases the peer's socket: shuts the socket down, hands it back to the pool the peer band
   registers with, and clears the peer's own slot. */
void networkPeer_release(NetworkPeerReceive* self)
{
    if (self->handle_04 != NULL) {
        self->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), self->handle_04);
        self->handle_04 = NULL;
    }
}

/* The same teardown for the second peer class in the band (GUESS: identical body, the two classes'
   tables differ). */
void networkPeer_releaseSocket(NetworkPeerReceive* self)
{
    if (self->handle_04 != NULL) {
        self->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), self->handle_04);
        self->handle_04 = NULL;
    }
}

/* Opens the connection's socket and registers the address it was opened on: the socket comes from the
   band's pool, `open`/`setPeer` are the socket's own two slots, and the six address bytes are kept on
   the connection.  Returns 0, or the negative step that failed. */
s32 networkPeer_openSocket(NetworkPeerReceive* self, const u8* address)
{
    if (self->handle_04 != NULL) {
        return -1;
    }
    self->handle_04 = networkSocketPool_acquire(getNetworkLogger());
    if (self->handle_04 == NULL) {
        return -2;
    }
    if (self->handle_04->open(1) < 0) {
        networkPeer_release(self);
        return -3;
    }
    if (self->handle_04->setPeer(address) < 0) {
        networkPeer_release(self);
        return -4;
    }
    memcpy(self->address_08, address, 6);
    return 0;
}

/* Hands the connection's four peer ids to the peer class's own reset slot. */
void networkPeer_resetSlots(NetworkPeerSocket* self)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        self->resetSlot((s8)index);
    }
}

/* Publishes the peer's record: the six-byte address, the connection it registers with, and up to 0x40
   bytes of label with its used length. */
void networkPeer_setInfo(NetworkPeerSocket* self, const NetworkPeerInfo* info)
{
    s32 available;
    s32 labelSize;

    memcpy(self->peerAddress_2420, info->address_00, 6);
    self->connection_2428 = info->connection_08;
    memset(self->label_242C, 0, 0x40);
    available = info->labelSize_4C;
    if (available < 0x40) {
        labelSize = available;
    } else {
        labelSize = 0x40;
    }
    self->labelSize_246C = labelSize;
    memcpy(self->label_242C, info->label_0C, labelSize);
}

/* Closes the peer: drops it from its connection's registry and, when it was armed, releases the
   socket and empties both work areas. */
void networkPeer_close(NetworkPeerSocket* self)
{
    if (self->connection_2428 != NULL) {
        NetworkSingleTcp_remove(self->connection_2428, self);
        if (self->armed_10 != 0) {
            networkPeer_release(self->connection_2428);
            networkPeer_clearReceiveBuffer(self->connection_2428);
        }
    }
    memset(self->work_19, 0, 0x2400);
    self->workUsed_241C = 0;
}

/* The peer class's init slot: close whatever the peer was holding and report it usable. */
s32 networkPeer_init(NetworkPeerSocket* self)
{
    self->close();
    return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* the peer table's constant slots (vtable 0x805F9510)                                             */
/* ---------------------------------------------------------------------------------------------- */

/* Slots 0x0C/0x18/0x20/0x48/0x50 of the peer table are empty or constant; nothing in a body pins a
   role, so the names carry the slot offset the table gives them. */
void networkPeerBuffer_slot0C(NetworkPeerBuffer* self)
{
}

s32 networkPeerBuffer_slot18(NetworkPeerBuffer* self)
{
    return 0;
}

void networkPeerBuffer_slot20(NetworkPeerBuffer* self)
{
}

s32 networkPeerBuffer_slot48(NetworkPeerBuffer* self)
{
    return 0;
}

void networkPeerBuffer_slot50(NetworkPeerBuffer* self)
{
}

/* Slot 0x54 has slot 0x24's body: clear through the class's own clear slot and report it usable. */
s32 networkPeerBuffer_reset(NetworkPeerBuffer* self)
{
    self->clearPayload();
    return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* the stream's record writers and reader                                                          */
/* ---------------------------------------------------------------------------------------------- */

/* Fills the stream's leading 0xE-byte record from a sink and advances the cursor over it. */
void networkPeerStream_pullRecord(NetworkByteStream* self, NetworkStreamSink* sink)
{
    if (self->cursor_0C + 0xE <= self->size_08) {
        if (sink->fill(&self->data_04[self->cursor_0C], 0xE) > 0) {
            self->cursor_0C += 0xE;
        }
    }
}

/* Writes the record's u16 length prefix and then its bytes, when both fit. */
void networkPeerStream_putRecord(NetworkByteStream* self, const NetworkPeerRecord* record)
{
    if (self->cursor_0C + record->size_04 + 2 <= self->size_08) {
        networkPeerStream_putU16(self, record->size_04);
        if (record->data_00 != NULL && record->size_04 != 0) {
            memcpy(&self->data_04[self->cursor_0C], record->data_00, record->size_04);
        }
        self->cursor_0C += record->size_04;
    }
}

/* Writes one 2-byte value, encoded through the log manager's own value slot. */
void networkPeerStream_putU16(NetworkByteStream* self, u16 value){
    u16 encoded;

    if (self->cursor_0C + 2 > self->size_08) {
        return;
    }
    encoded = getNetworkLogger()->encode_4C(value);
    memcpy(&self->data_04[self->cursor_0C], &encoded, 2);
    self->cursor_0C += 2;
}

/* Writes one 4-byte value, encoded through the log manager's own value slot. */
void networkPeerStream_putU32(NetworkByteStream* self, u32 value)
{
    u32 encoded;

    if (self->cursor_0C + 4 > self->size_08) {
        return;
    }
    encoded = getNetworkLogger()->encode_54(value);
    memcpy(&self->data_04[self->cursor_0C], &encoded, 4);
    self->cursor_0C += 4;
}

/* Records the stream's leading length prefix into the caller's u16, decoded through the log
   manager's own value slot, then drops the two bytes from the front. */
void networkPeerStream_readLength(NetworkByteStream* self, u16* out)
{
    u16 encoded;
    u32 remaining;

    if (self->cursor_0C < 2) {
        return;
    }
    memcpy(&encoded, self->data_04, 2);
    *out = getNetworkLogger()->flag_48(encoded);
    remaining = self->cursor_0C - 2;
    self->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(self->data_04, self->data_04 + 2, remaining);
    }
}

/* Takes the stream's leading 4-byte value, decoded through the log manager's own value slot, and
   drops it from the front. */
void networkPeerStream_takeU32(NetworkByteStream* self, u32* out)
{
    u32 value;
    u32 remaining;

    if (self->cursor_0C < 4) {
        return;
    }
    memcpy(&value, self->data_04, 4);
    *out = getNetworkLogger()->decode_50(value);
    remaining = self->cursor_0C - 4;
    self->cursor_0C = remaining;
    if (remaining != 0) {
        memmove(self->data_04, self->data_04 + 4, remaining);
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the manager's mutex wrappers and the session accessor                                           */
/* ---------------------------------------------------------------------------------------------- */

/* Releases the peer's socket through the peer's own helper. */
void networkPeer_disconnect(NetworkPeerReceive* self)
{
    networkPeer_release(self);
}

/* The same teardown for the second peer class in the band (GUESS: identical tail). */
void networkPeer_disconnectSocket(NetworkPeerReceive* self)
{
    networkPeer_releaseSocket(self);
}

#pragma dont_inline off

/* Locks the mutex a peer keeps at its own +0x04. */
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void LockMutex(void* mutex)
{
    NetworkPeerLock* lock = (NetworkPeerLock*)mutex;

    OSLockMutex(&lock->mutex_04);
}

/* Unlocks the mutex a peer keeps at its own +0x04. */
/* untyped: opaque handle passed through - only the peer band owns the mutex layout */
void UnlockMutex(void* mutex)
{
    NetworkPeerLock* lock = (NetworkPeerLock*)mutex;

    OSUnlockMutex(&lock->mutex_04);
}

/* Returns the slot index the session owns.  Declared `s32` on purpose: the caller narrows the
   result with `extsb`, which MWCC only emits for a callee declared wider than its own `s8`. */
s32 NetworkSessionStable_getOwnIndex(NetworkSessionStable* self)
{
    return (s8)self->field_14826;
}

}