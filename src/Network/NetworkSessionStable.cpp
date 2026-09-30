/*
 * Network/NetworkSessionStable.cpp - the session state machine's own helpers (the four `NetworkSessionBase` setters, the nonce pair,
 *   the small-object destructors, the slot reset and the own-index accessor); the state machine itself
 *   (`NetworkSessionStable_init`/`sendStream`, the function at 0x803CFEEC ...) is not written yet.
 *
 * One translation unit of the retail Network transport band, split out of `Network/network_transport.cpp`
 * (docs/network-transport-split.md holds the evidence and the confidence of each cut).  `.text`
 * 0x803CF6D8..0x803D3CE8, `.data` 0x805F9A40..0x805FA4EC (the TU's data runs on to 0x805FA788), extab
 * 0x80019AB4..0x80019E5C, extabindex 0x8003A344..0x8003A524.
 *
 * EDGE.  The left edge is 0x803CF6D8, not 0x803CF730: `tudiscover at 0x803CF6F4` puts `setHostTimeout` (F6E4)
 * ..`setRate` in one certain match set with the rest of this range, and `setLimits` (F6D8) is the same kind of
 * function (writes globals only Stable reads, called only from Stable's function at 0x803D2314); the destructor at
 * 0x803CF694 is the vtable key function of `NetworkSessionBase` and stays there.
 *
 * NAMES.  The file name is a GUESS from the log strings; the nonce pair is placed here because only this unit's
 * functions call it.  Every name here is the map's or a derived one; the derived ones are marked GUESS in
 * `Network/network_transport_types.h`.
 *
 * TABLE.  No table is emitted yet: the `NetworkUnitPacket` and `NetworkSessionStable` tables (0x805FA6A8,
 * 0x805FA6E8) lie beyond the `.data` claim, and their classes are not written.
 *
 * FLAGS.  C++ under `cflags_network` (`-Cpp_exceptions on` gives the `extab`), per-unit `-O3`/`-pool off` (`configure.py`);
 * file-scope `#pragma peephole off` (playbook 39); each `dont_inline` region keeps a retail `bl` that `-inline auto` folds.
 *
 * RESIDUALS.  The state machine is not written: 50 of 57 rows (17.4 KB of 17.8 KB) - they need the neighbour
 * header's session/slot fields and about 40 foreign writer-band callees named first;
 * `networkSessionNonce_generate` 99.67 %.  The object also emits the 8-byte int-to-float `.sdata2` constant `setRate`
 * needs, which the target loads from `Network/network_shared_data.cpp`'s run (playbook 58, a sole-referencer rule
 * this unit cannot meet).  The `.data` claim stops at 0x805FA4EC although the TU's data runs on to
 * 0x805FA788 (its jump table, log strings and the two tables sit there, unclaimed).
 */
#include "types.h"
#include "Network/network_transport.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"
#include "unsplit/NetworkStream.h"
/* `unsplit/Network.h` is the Network band's code half (`getNetworkLogger` and the socket-pool helpers).  It
   cannot be included beside `unsplit/OS.h`: the two band headers declare `OSCreateThread`/`OSResumeThread` with
   different signatures and a TU that sees both fails with `(10197) illegal function overloading`. */
#include "unsplit/Network.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The range keeps retail's unfused forms: a separate `extsh`+`cmpwi` before the free, and the memset argument setup
   in source order (`addi` before the two `li`s) - the peephole pass folds both.  Scoped off for the file. */
#pragma peephole off

extern "C" {

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

#pragma dont_inline on

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

/* Hands the connection's four peer ids to the peer class's own reset slot. */
void networkPeer_resetSlots(NetworkSessionBase* self)
{
    s32 index;

    for (index = 0; index < 4; index++) {
        self->resetSlot((s8)index);
    }
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

/* Returns the slot index the session owns.  Declared `s32` on purpose: the caller narrows the
   result with `extsb`, which MWCC only emits for a callee declared wider than its own `s8`. */
s32 NetworkSessionStable_getOwnIndex(NetworkSessionStable* self)
{
    return (s8)self->field_14826;
}

}
