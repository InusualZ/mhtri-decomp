/*
 * Network/network_transport.cpp - the Network transport band, `.text` 0x803CCDF8..0x803D3CE8
 * (28400 B, 118 auto units).
 *
 * WHAT IT IS.  The DWC/GameSpy transport layer the game's network code sits on: the abstract peer with
 * its error record and the three peers derived from it (the payload buffer, the Udp peer, the Mcs peer),
 * the `NetworkSingleTcp`/`NetworkMultipleUdp` socket users, the `NetworkResolverWii` name resolver, the
 * session base class's setters and the `NetworkSessionStable` state machine.  The registered neighbours
 * are `Network/fn_803D3CE8.cpp` above (0x803D3CE8) and `Network/initNetworkSessionStable.cpp` (0x803DEA30).
 *
 * MODULE AND NAME.  No `__FILE__` string is reachable (every `lis`/`addi` pair resolves to a vtable, a
 * `.data` log string, a `.sdata`/`.sdata2` float or a `.bss` scratch area) and `dumpmap.py lookup`
 * answers only `zz_XXXXXXXX_` for the code.  The log strings name their emitters (`NetworkSingleTcp::add`,
 * `NetworkMultipleUdp::receive`, `NetworkPeerMcs::put`, `NetworkResolverWii::check`), the vtables give each
 * slot's position and so its role, and `docs/memory-dump.md` names `sendPackets`/`receivePackets`/
 * `clearBuffer`/`setPeerAndSocket`/`getAvailableToRead`/`networkSmallObject_dtor`.  The file name is
 * derived from the range's dominant subject and is a **GUESS**, as are the names derived from behaviour
 * alone: `NetworkPeerBase` (the abstract peer - the class holding the error record every peer
 * constructor chains), `NetworkSessionBase`, the `networkPeerError_*` record accessors, the
 * `NetworkSocketHandle` slot names and every slot name that is only an offset.  The remaining `dtor_`
 * names are the map's own and belong to classes this unit does not model.
 *
 * SEAM (unproven - docs/plan.md 8.3).  The right edge is proven (the `extab` block 0x800198C8..0x80019E5C
 * and the `extabindex` block 0x8003A0BC..0x8003A524 abut `Network/fn_803D3CE8.cpp` exactly).  The left edge
 * 0x803CCDF8 is a discovery byte cap, not a boundary, and the range is more than one original TU: the
 * `.data` run below interleaves each table with the strings of the TU that owned it (see DATA).
 *
 * LANGUAGE.  C++ (`__dl__FPv`, virtual dispatch through the peer tables); `cflags_network`'s
 * `-Cpp_exceptions on` is what makes the target's `extab`/`extabindex` appear.
 *
 * CLASSES AND TABLES.  Six classes emit their tables from this unit (rule 10) - `NetworkPeerBase`
 * 0x805F94E0, `NetworkPeerBuffer` 0x805F9510, `NetworkPeerUdp` 0x805F9540 (its constructor is outside
 * the range: an unowned function stores it), `NetworkPeerMcs` 0x805F95E0, `NetworkResolverBase`
 * 0x805F9938, `NetworkResolverWii` 0x805F9980 - plus `NetworkSessionBase` 0x805F99A0 (0xA0 B, the deleting
 * destructor, 33 pure slots and the four setters).  The map rows carry the manglings (`__vt__...`,
 * `send__14NetworkPeerMcsFPCUc...`, ...), so objdiff pairs every slot by name.  The 51 manglings bake in the GUESSed slot
 * parameter lists, so a signature correction is another rename sweep.  The peers share one
 * abstract base whose pure slots carry the *union* of the derived signatures (a derived slot with a
 * different parameter list would be a new virtual and move every later slot), and the peers that ignore
 * an argument leave it unused.  The peers' deleting destructor is a plain virtual `destroy(s16)` chained
 * by hand with flags 0 (the shape `NetworkPeerGameSpy::destroy` in `Network/fn_8041A87C.cpp` already
 * uses; a real `~NetworkPeerBase()` needs the caller to be a derived class); the resolver classes use
 * real destructors because `NetworkResolverWii`'s stores its own vptr before it dispatches `check()`.
 * A class whose table is not ours (`NetworkSocketHandle`, `NetworkStreamSink`, the neighbours' classes)
 * declares virtuals and defines none, so MWCC emits nothing for it.
 *
 * DATA.  The claim is `.data` 0x805F94E0..0x805FA4EC (4108 B), `.bss` 0x806D2C60..0x806D3650 (the two
 * scratch packets, 0x9F0 B) and `.sbss` 0x80794C98..0x80794CA0.  The object emits `.data` 1376 B (the seven
 * tables, 0x1A0 B, and 0x3C0 B of log strings of the written functions), `.bss` 0x9F0 B (100 %), `.sbss` 4 B
 * (`networkMcsRetryTime`; the second word is unwritten code's) and `.sdata2` 8 B.  What keeps the object from
 * being the target's:
 *   - `.data` order.  The target run interleaves each table with its TU's strings (94E0/9510/9540, the Mcs
 *     `put` string, 95E0, the Tcp/Udp strings, 9938, the resolver `check` string, 9980, 99A0); one TU emits
 *     every table after all the strings, in the reverse of the key functions' definition order (measured on a
 *     probe).  So the run cannot be byte-identical while the unit is one TU, and the `.data` section scores
 *     10.4 % of 4108 B (it read 29.5 % of 2732 B before the claim, a positional overlap of unrelated
 *     strings); every table and string is present, at the right size, paired by name.
 *   - `.sdata2` 8 B.  `NetworkSessionBase::setRate` converts an `s32` to `f32`, so MWCC pools the int-to-float
 *     constant 0x4330000080000000; the target loads `lbl_8079C6D8` instead, whose only reader is that
 *     function but which sits inside `Network/network_shared_data.cpp`'s 200 B `.sdata2` run (one owner
 *     by design, and a partial `.sdata2` claim breaks the link - playbook 23/58).
 *   - `.data`/`.sbss`/`extab`/`extabindex`/`.text` are short by the unwritten functions (`extab` ours
 *     548/1428 B, `extabindex` 732/1128 B, `.data` 1376/4108 B, `.text` 11108/28400 B).
 * Both `.data` and `.sdata2` are therefore flip blockers until the seam is re-drawn per class and the
 * session state machine is written.
 *
 * FLAGS.  `-O3` like the two sibling session units (the lib's `-O4,p` puts 14 rows at 100 %, `-O3` 37 of 38
 * on the first pass's sample) and `-pool off` (configure.py carries the unit-level before/after).  File-scope
 * `#pragma peephole off` is what keeps retail's unfused forms: with the pass on, 18 of 30 rows landed and
 * the rest lost a separate `extsh`+`cmpwi` before the free (`extsh.` absorbed the compare) and had the two
 * `li`s of a `memset` argument list hoisted above the destination's `addi` (playbook 39/41).
 *
 * LEVERS FOUND.  `4 <= peerIndex` (constant on the left) keeps MWCC from merging the two tests of a range
 * check into one unsigned compare; `(s64)17 * ticks` reproduces the 64-bit sleep product (`u64` folds the
 * high word); a do-while outer loop and the declaration order of locals colour the registers retail has;
 * `#pragma dont_inline on` around the destructors and constructors keeps the `bl` into the base.
 *
 * RESIDUALS (81 of 145 rows at 100 %, 6604 of 28400 B, unit 38.8 %).
 *   - The `NETWORK_ERROR_*` constants are immediates the target relocates against `@eti_` extabindex rows
 *     (dtk's group-relative name for a `lis`/`addi` pair that is not an address, playbook 58's class): they
 *     are the residual of every row that stores one (99.2-99.8 %).
 *   - `NetworkMultipleUdp_receive` 94.06 %: only the peer-index/length register pair is swapped (index r31
 *     in retail, r29 ours); declaration order, a `used` pointer and a separate length copy were tried.
 *     `NetworkResolverWii::check` 97.27 %: retail's return-0 tail is shared (one `b`), ours duplicates it.
 *     `NetworkPeerUdp::receive` 99.66 %: capacity registers swapped.
 *   - `networkPeerStream_takeRecord` 81.39 % (same size): retail branches *forward* to the shared
 *     zero-store (`bgt`) where ours falls through, plus one `lhz` reload of the address-taken length local;
 *     both spellings were tried and the block layout is the peephole pass's, not a source form.
 *   - Not written: the session state machine (`NetworkSessionStable_init`/`sendStream`, `fn_803CFEEC`
 *     4968 B, `fn_803D1CE0`, `fn_803D31BC`, the `NetworkSessionStable` interval setters ...).  They need the
 *     neighbour header's session/slot fields (inside its padding: the slots' per-slot header at slot-0x10 and
 *     their +0x04 float, the session's +0x14810/+0x14824/+0x16CE4 words) and about 40 foreign writer-band
 *     callees named first.
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"
#include "unsplit/NetworkStream.h"
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

/* The console's bus clock in Hz (low memory 0x800000F8), spelled exactly as `unsplit/OS.h` defines
   `OS_BUS_CLOCK`: that header cannot be included here (see above), so the one-line spelling is kept locally. */
#define OS_BUS_CLOCK (*(u32*)0x800000F8)

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the
   memset argument setup in source order (`addi` before the two `li`s) - the peephole pass folds
   both.  Scoped off for the file until a row proves otherwise. */
#pragma peephole off

extern "C" {

/* The scratch packets the two peers frame their traffic in (the range's `.bss` claim), and the time the
   Mcs peer last tried to connect. */
u8 networkUdpPacketBuffer[0x5E0];
u8 networkMcsPacketBuffer[0x410];
f32 networkMcsRetryTime;

/* ---------------------------------------------------------------------------------------------- */
/* the peer error record                                                                           */
/* ---------------------------------------------------------------------------------------------- */

/* Copies the three-word record out when the caller supplies somewhere to put it. */
void networkPeerError_get(NetworkPeerBase* self, NetworkPeerErrorRecord* out)
{
    if (out != NULL) {
        out->source = self->source_04;
        out->argument = self->argument_08;
        out->code = self->code_0C;
    }
}

/* Empties the record so the next failure can fill it. */
void networkPeerError_clear(NetworkPeerBase* self)
{
    memset(&self->source_04, 0, 0xC);
}

/* Records the first failure only: a record that is already filled is left alone. */
/* untyped: opaque handle passed through - the callers hand the peer they already hold */
void networkPeerError_set(void* self, const void* source, u32 argument, s32 code)
{
    NetworkPeerBase* error = (NetworkPeerBase*)self;

    if (error->source_04 == NULL) {
        error->source_04 = source;
        error->argument_08 = argument;
        error->code_0C = code;
    }
}

/* Deleting destructor of the abstract peer: frees on request, nothing to chain. */
NetworkPeerBase* NetworkPeerBase::destroy(s16 flags)
{
    if (this != NULL) {
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
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
s32 NetworkPeerBuffer::move()
{
    memset(this->payload_10, 0, 0x2000);
    this->used_2010 = 0;
    return 1;
}

/* Empties the payload buffer through the class's own clear slot. */
s32 NetworkPeerBuffer::init()
{
    this->reset();
    return 1;
}

/* Empties the payload and its use count. */
void NetworkPeerBuffer::reset()
{
    memset(this->payload_10, 0, 0x2000);
    this->used_2010 = 0;
}

/* Retail keeps the `bl` into the base destructor (measured: folding it drops the whole call, since
   the inlined body's `flags > 0` test is constant-folded away). */
#pragma dont_inline on
/* Deleting destructor of the payload buffer: chains the error-record base, then frees on request. */
NetworkPeerBase* NetworkPeerBuffer::destroy(s16 flags)
{
    if (this != NULL) {
        NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

#pragma dont_inline off

/* Binds the Udp peer to its slot on the shared Udp socket. */
/* untyped: caller-owned payload - the binding record the Udp peer reads */
void NetworkPeerUdp::setContext(const void* context)
{
    this->peerIndex_10 = ((const NetworkPeerUdpBinding*)context)->peerIndex;
    this->udp_14 = ((const NetworkPeerUdpBinding*)context)->udp;
}

/* ---------------------------------------------------------------------------------------------- */
/* the socket peer                                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* Asks the peer's socket how many bytes are readable; 0 when it holds no socket. */
s32 getAvailableToRead(NetworkSocketUser* self)
{
    if (self->handle_04 != NULL) {
        return getBytesAvailableToRead(self->handle_04);
    }
    return 0;
}

/* Same question for the peer class that keeps its socket in the same slot (GUESS: identical body,
   the two classes' slot tables differ). */
s32 networkPeer_getAvailableToRead(NetworkSocketUser* self)
{
    if (self->handle_04 != NULL) {
        return getBytesAvailableToRead(self->handle_04);
    }
    return 0;
}

/* Returns the stream's byte block. */
u8* networkPeer_getSocket(NetworkByteStream* self)
{
    return self->data_04;
}

/* Returns how many bytes the stream holds. */
u32 networkPeer_getPeerId(NetworkByteStream* self)
{
    return self->cursor_0C;
}

/* Closes the peer's socket through the socket's own vtable; -1 when there is no socket. */
s32 networkPeer_closeSocket(NetworkSocketUser* self)
{
    if (self->handle_04 == NULL) {
        return -1;
    }
    return self->handle_04->closeSocket();
}

/* Clears the peer's socket receive buffer through the socket's own vtable; 0 when there is none. */
s32 networkPeer_clearReceiveSocket(NetworkSocketUser* self)
{
    if (self->handle_04 == NULL) {
        return 0;
    }
    return self->handle_04->clearReceive();
}

/* Raises the deferred-drop flag of a peer that has something to drop. */
void NetworkPeerMcs::armDrop()
{
    if (this->armed_10 != 0) {
        this->dropped_18 = 1;
    }
}

/* Empties the peer's own 0x2400-byte receive area. */
void networkPeer_clearReceiveBuffer(NetworkSingleTcp* self)
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
void networkPeerStream_forwardRecord(NetworkByteStream* self, NetworkStreamSink* sink)
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
s32 NetworkResolverWii::setName(const char* name)
{
    u32 length;

    if (strlen(name) < 0x1FF) {
        length = strlen(name);
    } else {
        length = 0x1FF;
    }
    memcpy(this->name_04, name, length);
    this->name_04[length] = 0;
    memset(this->records_204, 0, 0x10);
    this->count_214 = 0;
    this->code_218 = 0;
    if (this->addrInfo_1560 != NULL) {
        this->code_218 = 0x5A;
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- */
/* the record table                                                                                */
/* ---------------------------------------------------------------------------------------------- */

/* Puts the peer's error code back to its idle value. */
void NetworkResolverWii::resetCode()
{
    if (this->code_218 == 0) {
        this->code_218 = 0xFF;
    }
}

/* Copies one live record out of the peer's four-entry table. */
void NetworkResolverWii::recordGet(s32 index, u32* out)
{
    if (index < 0) {
        return;
    }
    if ((s32)this->count_214 <= index) {
        return;
    }
    memcpy(out, &this->records_204[index], 4);
}

/* ---------------------------------------------------------------------------------------------- */
/* the peer classes' remaining destructors                                                         */
/* ---------------------------------------------------------------------------------------------- */

/* The same kept-`bl` shape as the destructors above: the inline pass is off across this block. */
#pragma dont_inline on
/* Deleting destructor: chains the base destructor, then frees on request. */
NetworkPeerBase* NetworkPeerUdp::destroy(s16 flags)
{
    if (this != NULL) {
        NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Deleting destructor: chains the base destructor, then frees on request. */
NetworkPeerBase* NetworkPeerMcs::destroy(s16 flags)
{
    if (this != NULL) {
        NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Deleting destructor of the peer state object: frees on request, nothing else to chain. */
NetworkResolverBase::~NetworkResolverBase()
{
}

/* Deleting destructor of the peer-side object: frees on request, nothing else to chain. */
NetworkSessionBase::~NetworkSessionBase()
{
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
NetworkPeerBase* dtor_803D14C0(NetworkPeerBase* self, s32 flags)
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
   (`networkPeer_closeSocket` and the two forwarders would otherwise fold the teardown in, and the
   forwarders would stop being the 4-byte tail branches retail has). */
#pragma dont_inline on

/* Releases the peer's socket: shuts the socket down, hands it back to the pool the peer band
   registers with, and clears the peer's own slot. */
void networkPeer_release(NetworkSingleTcp* self)
{
    if (self->handle_04 != NULL) {
        self->handle_04->shutdownSocket();
        networkSocketPool_release(getNetworkLogger(), self->handle_04);
        self->handle_04 = NULL;
    }
}

/* The same teardown for the second peer class in the band (GUESS: identical body, the two classes'
   tables differ). */
void networkPeer_releaseSocket(NetworkSingleTcp* self)
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
s32 networkPeer_openSocket(NetworkSingleTcp* self, const NetworkPeerAddress* address)
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
    memcpy(&self->address_08, address, 6);
    return 0;
}

/* Hands the connection's four peer ids to the peer class's own reset slot. */
void networkPeer_resetSlots(NetworkSessionBase* self)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        self->resetSlot((s8)index);
    }
}

/* Publishes the peer's record: the six-byte address, the connection it registers with, and up to 0x40
   bytes of label with its used length. */
/* untyped: caller-owned payload - the record the Mcs peer publishes itself from */
void NetworkPeerMcs::setContext(const void* context)
{
    const NetworkPeerInfo* info = (const NetworkPeerInfo*)context;
    s32 available;
    s32 labelSize;

    memcpy(&this->peerAddress_2420, &info->address_00, 6);
    this->connection_2428 = info->connection_08;
    memset(this->label_242C, 0, 0x40);
    available = info->labelSize_4C;
    if (available < 0x40) {
        labelSize = available;
    } else {
        labelSize = 0x40;
    }
    this->labelSize_246C = labelSize;
    memcpy(this->label_242C, info->label_0C, labelSize);
}

/* Closes the peer: drops it from its connection's registry and, when it was armed, releases the
   socket and empties both work areas. */
void NetworkPeerMcs::reset()
{
    if (this->connection_2428 != NULL) {
        NetworkSingleTcp_remove(this->connection_2428, this);
        if (this->armed_10 != 0) {
            networkPeer_release(this->connection_2428);
            networkPeer_clearReceiveBuffer(this->connection_2428);
        }
    }
    memset(this->work_19, 0, 0x2400);
    this->workUsed_241C = 0;
}

/* The peer class's init slot: close whatever the peer was holding and report it usable. */
s32 NetworkPeerMcs::init()
{
    this->reset();
    return 1;
}

/* ---------------------------------------------------------------------------------------------- */
/* the peer table's constant slots (vtable 0x805F9510)                                             */
/* ---------------------------------------------------------------------------------------------- */

/* Slots 0x0C/0x18/0x20 of the payload-buffer table and 0x18/0x20 of the Udp peer table are empty or
   constant; nothing in a body pins a role, so the names carry the slot offset the table gives them. */
/* untyped: caller-owned payload - the buffer ignores it */
void NetworkPeerBuffer::setContext(const void* context)
{
}

s32 NetworkPeerBuffer::put(const u8* packet, s32 length, u32 a, u32 b, u32 c)
{
    return 0;
}

void NetworkPeerBuffer::armDrop()
{
}

s32 NetworkPeerUdp::put(const u8* packet, s32 length, u32 a, u32 b, u32 c)
{
    return 0;
}

void NetworkPeerUdp::armDrop()
{
}

/* The Udp peer's init slot: reset the peer's queue through its own reset slot and report it usable. */
s32 NetworkPeerUdp::init()
{
    this->reset();
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
void networkPeer_disconnect(NetworkSingleTcp* self)
{
    networkPeer_release(self);
}

/* The same teardown for the second peer class in the band (GUESS: identical tail). */
void networkPeer_disconnectSocket(NetworkSingleTcp* self)
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

/* ---------------------------------------------------------------------------------------------- */
/* the payload buffer and the two peers that speak through a shared scratch packet                 */
/* ---------------------------------------------------------------------------------------------- */

#pragma dont_inline on

/* Builds the error-record base: its table, then an empty record. */
NetworkPeerBase::NetworkPeerBase()
{
    memset(&source_04, 0, 0xC);
}

/* Builds the payload buffer: the error base, the buffer's own table, then an empty payload. */
NetworkPeerBuffer::NetworkPeerBuffer()
{
    memset(this->payload_10, 0, 0x2000);
    this->used_2010 = 0;
}

/* Queues bytes behind the ones already in the payload; -1 (with the record filled) when they do not fit. */
s32 NetworkPeerBuffer::send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind)
{
    if (data == NULL || size <= 0) {
        return 0;
    }
    if (this->used_2010 + size > 0x2000) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0x2000, 0x80000000);
        return -1;
    }
    memcpy(this->payload_10 + this->used_2010, data, size);
    this->used_2010 += size;
    return size;
}

/* Takes the leading packet out of the payload into the caller's buffer; 0 when none is complete or it
   does not fit, else its length. */
s32 NetworkPeerBuffer::receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind)
{
    NetworkStreamWriterDefault stream;
    s32 capacity;
    s32 length;

    networkStreamWriter_constructDefault(&stream);
    capacity = *size;
    *size = 0;
    *size2 = 0;
    *kind = 0;
    networkStreamReader_attach(&stream, this->payload_10, this->used_2010);
    if (make_sure_enough_space(&stream) == 0) {
        dtor_803CB8FC(&stream, -1);
        return 0;
    }
    if (capacity < read_size_from_buffer(&stream)) {
        dtor_803CB8FC(&stream, -1);
        return 0;
    }
    length = copy_from_buffer(&stream, out, capacity);
    if (length < 0) {
        dtor_803CB8FC(&stream, -1);
        return 0;
    }
    this->used_2010 -= networkStreamReader_consumePacket(&stream);
    *size = length;
    dtor_803CB8FC(&stream, -1);
    return length;
}

/* Frames up to two payloads (each a length prefix, the second with a kind byte) into the shared
   scratch packet and sends it to the peer's slot on the Udp socket; the byte count, or -1. */
s32 NetworkPeerUdp::send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind)
{
    u16 prefix;
    u16 prefix2;
    s8 kindByte;
    u8* cursor;
    s32 total;
    s32 result;

    kindByte = kind;
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return -1;
    }
    cursor = networkUdpPacketBuffer;
    if (data == NULL || size <= 0) {
        prefix = 0;
        memcpy(cursor, &prefix, 2);
        cursor += 2;
        total = 2;
    } else {
        prefix = getNetworkLogger()->encode_4C((u16)size);
        memcpy(cursor, &prefix, 2);
        cursor += 2;
        memcpy(cursor, data, size);
        cursor += size;
        total = size + 2;
    }
    if (data2 == NULL || size2 <= 0) {
        prefix2 = 0;
        memcpy(cursor, &prefix2, 2);
        total += 2;
    } else {
        prefix2 = getNetworkLogger()->encode_4C((u16)(size2 + 1));
        memcpy(cursor, &prefix2, 2);
        memcpy(cursor + 2, &kindByte, 1);
        total += 3;
        memcpy(cursor + 3, data2, size2);
        total += size2;
    }
    if (getNetworkLogger()->flag_48(prefix) == 0 && getNetworkLogger()->flag_48(prefix2) == 0) {
        return 0;
    }
    result = NetworkMultipleUdp_send(this->udp_14, this->peerIndex_10, networkUdpPacketBuffer, total);
    if (result < 0) {
        s32 error = NetworkMultipleUdp_getError(this->udp_14);
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, getAvailableToRead(this->udp_14), error);
        result = -1;
    }
    return result;
}

/* Reads one framed packet off the peer's slot into the caller's two buffers (the second one led by a
   kind byte); the raw byte count, 0 when nothing is queued, -1 on failure. */
s32 NetworkPeerUdp::receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind)
{
    u16 length2;
    u8* cursor;
    u16 length;
    s32 received;
    s32 capacity;
    s32 capacity2;

    capacity = *size;
    capacity2 = *size2;
    *size = 0;
    *size2 = 0;
    *kind = 0;
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return -1;
    }
    received = NetworkMultipleUdp_receive(this->udp_14, this->peerIndex_10, networkUdpPacketBuffer, 0x5DC);
    if (received < 0) {
        s32 error = NetworkMultipleUdp_getError(this->udp_14);
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, getAvailableToRead(this->udp_14), error);
        return -1;
    }
    if (received == 0) {
        return received;
    }
    cursor = networkUdpPacketBuffer;
    memcpy(&length, cursor, 2);
    cursor += 2;
    length = getNetworkLogger()->flag_48(length);
    if (length != 0 && length <= capacity) {
        memcpy(out, cursor, length);
        cursor += length;
        *size = length;
    } else {
        *size = 0;
    }
    memcpy(&length2, cursor, 2);
    length2 = getNetworkLogger()->flag_48(length2) - 1;
    if (length2 != 0 && length2 <= capacity2) {
        memcpy(kind, cursor + 2, 1);
        memcpy(out2, cursor + 3, length2);
        *size2 = length2;
    } else {
        *kind = 0;
        *size2 = 0;
    }
    return received;
}

/* Clears the peer's queued bytes on the shared Udp socket; -1 (with the record filled) when the peer
   has no socket, else 1. */
s32 NetworkPeerUdp::move()
{
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return -1;
    }
    NetworkMultipleUdp_reset(this->udp_14, this->peerIndex_10);
    return 1;
}

/* The tail-called twin of the clear: nothing is reported. */
void NetworkPeerUdp::reset()
{
    if (this->udp_14 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_UDP_UNATTACHED, 0, 0);
        return;
    }
    NetworkMultipleUdp_reset(this->udp_14, this->peerIndex_10);
}

#pragma dont_inline off

/* ---------------------------------------------------------------------------------------------- */
/* the Mcs peer                                                                                    */
/* ---------------------------------------------------------------------------------------------- */

#pragma dont_inline on

/* Builds the Mcs peer: the error base, the peer's table, its flags, the shared Mcs scratch packet and
   its own work area emptied, and no connection yet. */
NetworkPeerMcs::NetworkPeerMcs(u8 armed)
{
    this->armed_10 = armed;
    this->state_14 = 0;
    this->dropped_18 = 0;
    memset(networkMcsPacketBuffer, 0, 0x410);
    memset(this->work_19, 0, 0x2400);
    this->workUsed_241C = 0;
    memset(&this->peerAddress_2420, 0, 6);
    this->connection_2428 = NULL;
}

/* Frames the payload behind a length prefix, sends it on the connection (a listening peer does not
   need the socket to be idle first) and keeps a copy in the work area; the framed byte count, or -1. */
s32 NetworkPeerMcs::send(const u8* data, s32 size, const u8* data2, s32 size2, s8 kind)
{
    u16 prefix;
    s32 error;

    if (this->connection_2428 == NULL) {
        return 0;
    }
    if (data == NULL) {
        return 0;
    }
    if (size <= 0) {
        return 0;
    }
    if ((u32)(size + 2) > 0x410) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0x410, 0x80000000);
        return -1;
    }
    if ((u32)(size + this->workUsed_241C + 2) > 0x2400) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, 0x2400, 0x80000000);
        return -1;
    }
    prefix = getNetworkLogger()->encode_4C((u16)size);
    memcpy(networkMcsPacketBuffer, &prefix, 2);
    memcpy(networkMcsPacketBuffer + 2, data, size);
    if ((this->armed_10 == 0 || networkPeer_clearReceiveSocket(this->connection_2428) != 0)
        && NetworkSingleTcp_send(this->connection_2428, networkMcsPacketBuffer, size + 2) < 0) {
        error = NetworkSingleTcp_getError(this->connection_2428);
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_SEND, networkPeer_getAvailableToRead(this->connection_2428), error);
        return -1;
    }
    memcpy(this->work_19 + this->workUsed_241C, networkMcsPacketBuffer, size + 2);
    this->workUsed_241C += size + 2;
    return size + 2;
}

/* Takes the leading length-prefixed packet out of the work area into the caller's buffer; 0 when none
   is complete or it does not fit, -1 on failure, else the bytes the packet took. */
s32 NetworkPeerMcs::receive(u8* out, s32* size, u8* out2, s32* size2, u8* kind)
{
    u16 length;
    s32 error;
    s32 taken;
    u8* work;
    s32 capacity;

    if (this->connection_2428 == NULL) {
        return 0;
    }
    if (this->armed_10 == 0 && networkPeer_getAvailableToRead(this->connection_2428) != 0) {
        error = NetworkSingleTcp_getError(this->connection_2428);
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, networkPeer_getAvailableToRead(this->connection_2428), error);
        return -1;
    }
    capacity = *size;
    *size = 0;
    if (size2 != NULL) {
        *size2 = 0;
    }
    if (kind != NULL) {
        *kind = 0;
    }
    if ((s32)this->workUsed_241C < 2) {
        return 0;
    }
    work = this->work_19;
    memcpy(&length, work, 2);
    length = getNetworkLogger()->flag_48(length);
    if (length == 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PEER_RECEIVE, 0, 0x80000000);
        return -1;
    }
    if ((s32)this->workUsed_241C < (s32)(length + 2)) {
        return 0;
    }
    if (capacity < length) {
        return 0;
    }
    memcpy(out, work + 2, length);
    *size = length;
    taken = length + 2;
    this->workUsed_241C -= taken;
    if ((s32)this->workUsed_241C > 0) {
        memmove(work, this->work_19 + taken, this->workUsed_241C);
    }
    return length + 2;
}

/* Appends received bytes to the work area; -1 (logged, record filled) when its 0x2400 bytes would
   overflow. */
s32 NetworkPeerMcs::put(const u8* data, s32 length, u32 a, u32 b, u32 c)
{
    u32 used;

    if (length <= 0) {
        return 0;
    }
    used = this->workUsed_241C;
    if (used + length > 0x2400) {
        getNetworkLogger()->log_14("NetworkPeerMcs::put: mPacketRecvBuf over. please check sizeof(NetworkPeerMcs::mPacketRecvBuf)=0x%x<(0x%x+0x%x)\n",
                                   0x2400, used, length);
        networkPeerError_set(this, (const void*)NETWORK_ERROR_PUT_OVERFLOW, 0x2400, 0x80000000);
        return -1;
    }
    memcpy(this->work_19 + used, data, length);
    this->workUsed_241C += length;
    return length;
}

/* Walks the peer's connect machine one step: a listening peer re-registers with its connection once
   the retry interval has passed; a connecting peer opens the socket (state 0), waits for it to finish
   and publishes its label (state 1).  1 when it made progress, 0 when idle, -1 on failure. */
s32 NetworkPeerMcs::move()
{
    s32 result;
    s32 error;

    if (this->armed_10 == 0) {
        if (this->connection_2428 == NULL) {
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, 0, 0x80000000);
            return -1;
        }
        if (networkPeer_getAvailableToRead(this->connection_2428) != 0) {
            error = NetworkSingleTcp_getError(this->connection_2428);
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, networkPeer_getAvailableToRead(this->connection_2428), error);
            return -1;
        }
        if (networkPeer_clearReceiveSocket(this->connection_2428) == 0) {
            return 0;
        }
        if (getNetworkLogger()->getTime_60() < networkMcsRetryInterval + networkMcsRetryTime) {
            return 0;
        }
        networkMcsRetryTime = getNetworkLogger()->getTime_60();
        memset(this->work_19, 0, 0x2400);
        this->workUsed_241C = 0;
        NetworkSingleTcp_add(this->connection_2428, this);
        return 1;
    }
    if (this->connection_2428 == NULL) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, 0, 0x80000000);
        this->state_14 = 0;
        return -1;
    }
    if (this->dropped_18 != 0) {
        networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_STATE, 0, 0x80000000);
        this->state_14 = 0;
        return -1;
    }
    switch (this->state_14) {
    case 0:
        networkPeer_clearReceiveBuffer(this->connection_2428);
        if (networkPeer_openSocket(this->connection_2428, &this->peerAddress_2420) < 0) {
            error = NetworkSingleTcp_getError(this->connection_2428);
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, networkPeer_getAvailableToRead(this->connection_2428), error);
            this->state_14 = 0;
            return -1;
        }
        memset(this->work_19, 0, 0x2400);
        this->workUsed_241C = 0;
        this->state_14++;
        break;
    case 1:
        result = networkPeer_closeSocket(this->connection_2428);
        if (result < 0) {
            error = NetworkSingleTcp_getError(this->connection_2428);
            networkPeerError_set(this, (const void*)NETWORK_ERROR_MCS_SOCKET, networkPeer_getAvailableToRead(this->connection_2428), error);
            this->state_14 = 0;
            return -1;
        }
        if (result > 0) {
            networkMcsRetryTime = getNetworkLogger()->getTime_60();
            NetworkSingleTcp_send(this->connection_2428, (u8*)this->label_242C, this->labelSize_246C);
            this->state_14 = 0;
            return 1;
        }
        break;
    default:
        break;
    }
    return 0;
}

#pragma dont_inline off

/* ---------------------------------------------------------------------------------------------- */
/* the Tcp connection and the Udp socket                                                           */
/* ---------------------------------------------------------------------------------------------- */

/* Registers a peer in the connection's first free slot of four; the slot, or -1 (logged) when full. */
s32 NetworkSingleTcp_add(NetworkSingleTcp* self, NetworkPeerMcs* peer)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (self->peers_10[index] == NULL) {
            self->peers_10[index] = peer;
            getNetworkLogger()->signal_0C(3, "NetworkSingleTcp::add: %d\n", index);
            return index;
        }
    }
    getNetworkLogger()->warn_10("NetworkSingleTcp::add: cannot add peer.\n");
    return -1;
}

/* Unregisters a peer from the connection (logged when it is not registered). */
void NetworkSingleTcp_remove(NetworkSingleTcp* self, NetworkPeerMcs* peer)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (self->peers_10[index] == peer) {
            self->peers_10[index] = NULL;
            getNetworkLogger()->signal_0C(3, "NetworkSingleTcp::remove: %d\n", index);
            return;
        }
    }
    getNetworkLogger()->warn_10("NetworkSingleTcp::remove: cannot remove peer.\n");
}

/* Reads what the socket holds into the receive area, cuts it into length-prefixed packets and hands
   the complete ones to every registered peer; an invalid length releases the socket. */
void receivePatInterfaces(PatReceiver* receiver)
{
    NetworkSingleTcp* self = (NetworkSingleTcp*)receiver;
    s32 received;
    s32 offset;
    s32 index;
    u16 length;

    if (self->handle_04 != NULL && self->handle_04->clearReceive() != 0) {
        do {
            received = self->handle_04->receive(self->recv_20 + self->recvUsed_2420, 0x2400 - self->recvUsed_2420, NULL);
            if (received < 1) {
                break;
            }
            self->recvUsed_2420 += received;
            offset = 0;
            while (offset + 2 <= (s32)self->recvUsed_2420) {
                memcpy(&length, self->recv_20 + offset, 2);
                length = getNetworkLogger()->flag_48(length);
                if (length == 0 || length > 0x400) {
                    getNetworkLogger()->warn_10("NetworkSingleTcp::move: invalid packet. %d\n", length);
                    networkPeer_release(self);
                    return;
                }
                if ((s32)self->recvUsed_2420 < offset + length + 2) {
                    break;
                }
                offset += length + 2;
            }
            for (index = 0; index < 4; index++) {
                if (self->peers_10[index] != NULL && self->peers_10[index]->put(self->recv_20, offset, 0, 0, 0) < 0) {
                    getNetworkLogger()->log_14("NetworkSingleTcp::move: [%d] put failed. 0x%x(0x%x)/0x%x\n",
                                               index, self->recvUsed_2420, received, offset);
                }
            }
            self->recvUsed_2420 -= offset;
            if ((s32)self->recvUsed_2420 > 0) {
                memmove(self->recv_20, self->recv_20 + offset, self->recvUsed_2420);
            }
        } while (received > 0);
    }
}

/* Sends bytes on the connection's socket; -1 (logged) when it has none. */
s32 NetworkSingleTcp_send(NetworkSingleTcp* self, const u8* data, s32 size)
{
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkSingleTcp::send: socket is NULL\n");
        return -1;
    }
    return self->handle_04->send(data, size, NULL);
}

/* The socket's last error code; 0 when the connection has no socket. */
s32 NetworkSingleTcp_getError(NetworkSingleTcp* self)
{
    if (self->handle_04 != NULL) {
        return networkSocketHandle_getLastError(self->handle_04);
    }
    return 0;
}

/* Reads datagrams off the shared socket and queues each one behind the peer whose address sent it
   (logged when the peer's 0x1770-byte queue would overflow). */
void flushPatRequests(PatRequestQueue* queue)
{
    NetworkMultipleUdp* self = (NetworkMultipleUdp*)queue;
    NetworkPeerAddress sender;
    s32 received;
    u16 length;
    s32 index;

    if (self->handle_04 != NULL) {
        while (1) {
            received = self->handle_04->receive(self->datagram_26, 0x5DC, &sender);
            if (received < 1) {
                break;
            }
            length = received;
            for (index = 0; index < 4; index++) {
                if (memcmp(&self->addresses_0E[index], &sender, 6) == 0) {
                    if ((s32)(length + self->used_63C4[index]) > 0x1770) {
                        getNetworkLogger()->log_14("NetworkMultipleUdp::move: buf_recv_peer over. please check NetworkMultipleUdp::MAX_SIZE_BUF_PEER\n");
                    } else {
                        memcpy(self->received_602[index] + self->used_63C4[index], self->datagram_26, length);
                        self->used_63C4[index] += length;
                    }
                    break;
                }
            }
        }
    }
}

/* Forgets the peer that owns an address: its address and its queued bytes (logged). */
void NetworkMultipleUdp_remove(NetworkMultipleUdp* self, const NetworkPeerAddress* address)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        if (memcmp(&self->addresses_0E[index], address, 6) == 0) {
            memset(&self->addresses_0E[index], 0, 6);
            self->used_63C4[index] = 0;
            getNetworkLogger()->signal_0C(3, "NetworkMultipleUdp::remove: %d.%d.%d.%d:%d\n",
                                          address->ip_00[0], address->ip_00[1], address->ip_00[2], address->ip_00[3],
                                          address->port_04);
            return;
        }
    }
}

/* Drops the bytes queued for one peer slot (logged when the slot or the socket is invalid). */
void NetworkMultipleUdp_reset(NetworkMultipleUdp* self, s32 peerIndex)
{
    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::reset: peer_id is invalid -> %d\n", peerIndex);
        return;
    }
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::reset: socket is NULL\n");
        return;
    }
    self->used_63C4[peerIndex] = 0;
}

/* Sends bytes to one peer slot's address; -1 (logged) when the slot or the socket is invalid. */
s32 NetworkMultipleUdp_send(NetworkMultipleUdp* self, s32 peerIndex, const u8* data, s32 size)
{
    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::send: peer_id is invalid -> %d\n", peerIndex);
        return -1;
    }
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::send: socket is NULL\n");
        return -1;
    }
    return self->handle_04->send(data, size, &self->addresses_0E[peerIndex]);
}

/* Takes the two length-prefixed payloads a peer slot has queued into the caller's buffer; 0 when they
   are incomplete or do not fit, -1 (logged) on an invalid slot or socket, else the bytes taken. */
s32 NetworkMultipleUdp_receive(NetworkMultipleUdp* self, s32 peerIndex, u8* out, s32 capacity)
{
    u16 length;
    u16 length2;
    s32 taken;
    s32 total;
    u8* queue;
    s32* used;

    if (peerIndex < 0 || 4 <= peerIndex) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: peer_id is invalid -> %d\n", peerIndex);
        return -1;
    }
    if (self->handle_04 == NULL) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: socket is NULL\n");
        return -1;
    }
    used = &self->used_63C4[peerIndex];
    if (*used < 2) {
        return 0;
    }
    queue = self->received_602[peerIndex];
    memcpy(&length, queue, 2);
    length = getNetworkLogger()->flag_48(length);
    if (length == 0 || length > 0x400) {
        getNetworkLogger()->warn_10("NetworkMultipleUdp::receive: invalid packet %d from %d.%d.%d.%d:%d\n", length,
                                    self->addresses_0E[peerIndex].ip_00[0], self->addresses_0E[peerIndex].ip_00[1],
                                    self->addresses_0E[peerIndex].ip_00[2], self->addresses_0E[peerIndex].ip_00[3],
                                    self->addresses_0E[peerIndex].port_04);
        *used = 0;
        return 0;
    }
    taken = length + 2;
    if (*used < taken) {
        return 0;
    }
    if (*used < taken + 2) {
        return 0;
    }
    memcpy(&length2, queue + taken, 2);
    length2 = getNetworkLogger()->flag_48(length2);
    taken += 2;
    if (*used < taken + length2) {
        return 0;
    }
    total = taken + length2;
    if (capacity < total) {
        return 0;
    }
    memcpy(out, queue, total);
    *used -= total;
    if (*used > 0) {
        memmove(queue, queue + total, *used);
    }
    return total;
}

/* The socket's last error code; 0 when the Udp socket has none. */
s32 NetworkMultipleUdp_getError(NetworkMultipleUdp* self)
{
    if (self->handle_04 != NULL) {
        return networkSocketHandle_getLastError(self->handle_04);
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- */
/* the name resolver                                                                               */
/* ---------------------------------------------------------------------------------------------- */

#pragma dont_inline on

/* Builds the resolver base: its table, an empty name and an empty address table. */
NetworkResolverBase::NetworkResolverBase()
{
    memset(this->name_04, 0, 0x200);
    memset(this->records_204, 0, 0x10);
    this->count_214 = 0;
}

/* The lookup thread's entry: resolves the name, no result of its own. */
/* untyped: opaque handle passed through - the OS thread hands back the argument it was created with */
void* networkResolver_threadEntry(void* self)
{
    networkResolver_lookup((NetworkResolverWii*)self);
    return NULL;
}

/* Builds the Wii resolver: the base, its own table, the idle state and no result list. */
NetworkResolverWii::NetworkResolverWii()
{
    this->code_218 = 0xFF;
    this->addrInfo_1560 = NULL;
}

/* Waits for a running lookup to finish, frees its result list and destroys the base. */
NetworkResolverWii::~NetworkResolverWii()
{
    if (code_218 == 0) {
        code_218 = 0xFF;
    }
    while (check() == 0) {
        OSSleepTicks((s64)17 * ((OS_BUS_CLOCK / 4) / 1000));
    }
    if (addrInfo_1560 != NULL) {
        SOFreeAddrInfo(addrInfo_1560);
        addrInfo_1560 = NULL;
    }
}

#pragma dont_inline off

/* Resolves the name on the lookup thread and keeps the result. */
void networkResolver_lookup(NetworkResolverWii* self)
{
    self->result_1538 = SOGetAddrInfo(self->lookupName_153C, NULL, &self->hints_1540, &self->addrInfo_1560);
}

/* Walks the lookup state: an address literal resolves at once, anything else starts the lookup thread
   (state 0), waits for it (0x0A), collects up to four addresses (0x0F) and then reports how many it has
   (0x14).  A failure parks the machine in 0x5A. */
s32 NetworkResolverWii::check()
{
    u8 literal[4];
    SOSockAddrIn sockaddr;
    SOAddrInfo* node;
    s32 result;
    s32 count;

    switch (this->code_218) {
    case 0:
        getNetworkLogger()->signal_0C(1, "NetworkResolverWii::check(): NAME[%s]\n", this->name_04);
        if (this->name_04[0] == 0) {
            this->code_218 = 0x5A;
            return (s32)0x80020002;
        }
        result = SOInetAtoN(this->name_04, literal);
        if (result == 1) {
            this->code_218 = 0x14;
            memcpy(&this->records_204[0], literal, 4);
            this->count_214 = 1;
            return 1;
        }
        if (result < 0) {
            this->code_218 = 0x5A;
            return result;
        }
        this->lookupName_153C = this->name_04;
        memset(&this->hints_1540, 0, 0x20);
        this->hints_1540.family = 2;
        if (OSCreateThread(this->thread_220, (void*)networkResolver_threadEntry, this,
                           this->stack_538 + sizeof(this->stack_538), 0x1000, 0xE, 1) == 0) {
            return -1;
        }
        this->result_1538 = 0;
        OSResumeThread(this->thread_220);
        this->code_218 = 0xA;
        return 0;
    case 10:
        if (OSIsThreadTerminated(this->thread_220) != 0) {
            result = this->result_1538;
            if (result < 0) {
                this->code_218 = 0x5A;
                if (this->addrInfo_1560 != NULL) {
                    SOFreeAddrInfo(this->addrInfo_1560);
                    this->addrInfo_1560 = NULL;
                }
                return result;
            }
            this->code_218 = 0xF;
        }
        return 0;
    case 15:
        this->code_218 = 0x14;
        this->count_214 = 0;
        node = this->addrInfo_1560;
        while (node != NULL && (s32)this->count_214 < 4) {
            memcpy(&sockaddr, node->addr, 8);
            memcpy(&this->records_204[this->count_214], &sockaddr.addr, 4);
            node = node->next;
            this->count_214++;
        }
        if (this->addrInfo_1560 != NULL) {
            SOFreeAddrInfo(this->addrInfo_1560);
            this->addrInfo_1560 = NULL;
        }
        count = this->count_214;
        if (count == 0) {
            this->code_218 = 0x5A;
            return -1;
        }
        return count;
    case 20:
        return this->count_214;
    default:
        return -1;
    }
}

/* ---------------------------------------------------------------------------------------------- */
/* the session's setters and nonce                                                                 */
/* ---------------------------------------------------------------------------------------------- */

/* Publishes the value the session's put path consults. */
void NetworkSessionStable_setNotifyValue(u32 value)
{
    networkSessionNotifyValue = value;
}

/* Sets how many host and subhost slots the session allows (GUESS on both meanings). */
void NetworkSessionBase::setLimits(u32 maxHosts, u32 maxSubhosts)
{
    networkSessionMaxHosts = maxHosts;
    networkSessionMaxSubhosts = maxSubhosts;
}

/* Sets the host timeout in seconds (GUESS: the name follows the word it writes). */
void NetworkSessionBase::setHostTimeout(f32 seconds)
{
    networkSessionHostTimeout = seconds;
}

/* Sets the subhost timeout in seconds (GUESS: the name follows the word it writes). */
void NetworkSessionBase::setSubhostTimeout(f32 seconds)
{
    networkSessionSubhostTimeout = seconds;
}

/* Stores a window (the quotient of two counts) and the per-unit step, the reciprocal of the divisor. */
void NetworkSessionBase::setRate(s32 count, s32 divisor)
{
    networkSessionRateWindow = count / divisor;
    networkSessionRateStep = networkSessionUnit / (f32)divisor;
}

/* Draws a session nonce from the clock and `rand`, redrawing until it is neither 0 nor -1. */
u32 networkSessionNonce_generate(void)
{
    f32 scale = networkNonceScale;
    u32 nonce;

    do {
        u32 stamp = (u32)(scale * getNetworkLogger()->getTime_60());
        nonce = rand() + stamp;
    } while (nonce == 0 || nonce == 0xFFFFFFFF);
    return nonce;
}

/* True when a nonce is neither 0 nor -1. */
s32 networkSessionNonce_isValid(u32 nonce)
{
    s32 valid = 0;

    if (nonce != 0 && nonce != 0xFFFFFFFF) {
        valid = 1;
    }
    return valid;
}

}
