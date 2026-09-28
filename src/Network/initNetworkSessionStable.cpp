/*
 * Network/initNetworkSessionStable.cpp - `initNetworkSessionStable`
 * (0x803DEA30..0x803DEB38, 264 B).
 *
 * BOUNDARY.  One function, both seams weak: `tudiscover at initNetworkSessionStable` reports the
 * closure as the function alone, the nearest left cut 0x803DD750 and right cut 0x803DEDE0 are both
 * single-signal (codegen fingerprint change, share 0.093 / 0.113).  The closure edge is taken on both
 * sides, so **both seams are unproven** - the original TU may extend up to 0x803DEDE0 (its next
 * function, `0x803DEB38`, and the six following it are just as plausibly the same file).
 * Sections: `.text` 0x803DEA30..0x803DEB38, `extab` 0x8001A630..0x8001A648,
 * `extabindex` 0x8003AB9C..0x8003ABA8 (the object's one unwind record).
 *
 * WHAT IT IS.  The NetworkSessionStable opener: it rejects a second init (+0xC already set returns
 * -1), allocates the 0x16D08-byte session object, constructs it, runs the session's `init` slot
 * (+0x0C) with the reflect callback `networkSessionReflectCallback` and the band's work buffer, seeds
 * the host, subhost and connection intervals from the float pool (0x8079C764 / 0x80793930 /
 * 0x80793934), flips the +0x3C8 ready flag and returns the session's own slot index (the callee's -1
 * when no slot was free).  Module `Network` (the class names and the registered neighbour).
 *
 * LANGUAGE.  The task proposed this unit as `.c`, but the target allocates through `__nw__FUl`, the
 * C++ global `operator new` (defined in `sys_mem.cpp`), and dispatches five methods through the
 * session object's vtable - both C++-only.  The file is therefore `.cpp`; the function keeps C linkage
 * (`extern "C"`) so the unmangled `initNetworkSessionStable` symbol is unchanged.  That also satisfies
 * rule 2/9: the allocator is declared once in `include/sys_mem.h` and reached through a `new`
 * expression rather than named here.
 *
 * FLAGS.  Per-unit `-O3` in `configure.py` (the lib default `-O4,p` hoists the constant setup into the
 * prologue and lays the two early returns out inline): the source scores 81.393936 % at `-O4,p` and
 * 100.00000 % at `-O3` (re-measured on this revision, the lib flag and the peephole pragma in place) - the
 * same flag shape as the sibling `Network/fn_803D3CE8.cpp`.  The lib flag and one file-scope pragma, each
 * with the instruction evidence:
 *
 *   - C++ exceptions come from the **lib flag** `-Cpp_exceptions on` (`cflags_network`, flags-audit
 *     2026-09-28), not a file pragma: this file used to spell `#pragma exceptions on` because the lib
 *     default sets them off, and measured byte-identical with the pragma removed and the flag on.  Either
 *     way it is what makes MWCC emit the `extab`/`extabindex` pair at all: without it our object carries
 *     neither section (1232 B against the target's 1928) while `.text` stays byte-identical.  The `new`
 *     expression below is then what gives the record the target's 24-byte shape - exceptions alone stay at
 *     the 8-byte header.
 *   - `#pragma peephole off` is the last byte of the match.  With the pass on, MWCC folds the first
 *     dispatch's vptr-load base into the `mr r3,r31` copy (`lwz r12,0(r31)` at +0x78) where retail
 *     keeps `lwz r12,0(r3)`; with it off `.text`, `extab` and `extabindex` are all byte-identical
 *     (99.92424 % -> 100.00000 %, the same 264 B).
 *
 * NAMES.  The range's one owned symbol keeps its real name (`initNetworkSessionStable`); the two
 * neighbouring names it uses (`constructNetworkSessionObject`, which it calls, and
 * `networkSessionReflectCallback`, whose address it materialises into an argument - it is never called
 * here) and the three band constants it loads were named from their use here.  The five vtable targets
 * it dispatches through are renamed from the class's own evidence: `NetworkSessionStable_init` (its own
 * `NetworkSessionStable::init: my nonce is 0x%08x` log), `_setHostInterval` / `_setSubhostInterval` (they
 * write the two period fields the class's `move` reads for its `host[%d]` / `subhost[%d]` channels),
 * `_getOwnIndex` (the slot index `set` assigns and `init` hands back) and `_setConnectionInterval` (it
 * forwards its value to each of the four slots' connection objects).  Only `constructNetworkSessionObject`
 * has no registered owner - rule 2, filed.
 *
 * BODY.  Reconstructed from the disassembly; the allocation/construct sequence and the vtable
 * dispatch are modelled as a class with the target's slots (no vtable is emitted - no virtual is
 * defined here).  MWCC lays the first declared virtual at +0x08, so the class's index is
 * `(slot - 0x08) / 4`; the target dispatches through +0x0C, +0x54, +0x58, +0x60 and +0x6C, i.e. the
 * declared indices 1, 19, 20, 22 and 25 - `init` (the first call, with the reflect callback and the work
 * buffer), `setHostInterval`, `setSubhostInterval`, `setConnectionInterval` and `getOwnIndex` (names from
 * the NAMES evidence).  `getOwnIndex` is declared `s32` on purpose: the target's call site narrows with
 * `extsb r3, r3` at +0xEC, which MWCC only emits for a callee the caller declares wider than its own `s8`
 * return.  The allocation is a real `new` expression on the non-polymorphic `NetworkSessionObjectAllocation`
 * proxy below, and that is what makes MWCC emit the target's 24-byte `extab` record (its guarded range
 * ends at the publishing `stw` and its last word relocates to `__dl__FPv`).
 *
 * RESIDUAL.  None in the object: `.text` (264 B), `extab` (0x18) and `extabindex` (0xC) are
 * byte-identical to the target object and all ten relocations name the same symbols at the same
 * offsets; `flipcheck.py` prints READY.  The dispatch names are derived, not read off a mangled symbol:
 * `init` and the two interval setters rest on the class's own log strings, `getOwnIndex` on the value
 * `set` returns, and `setConnectionInterval` on its use alone, so a later lane may refine that one.
 */
#include "types.h"
#include "sys_mem.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"   /* the band's unowned constants (rule 2) */

/* Retail keeps the unfused `lwz r12,0(r3)` in the first dispatch: with the peephole pass on MWCC folds
 * the base register into the `mr r3,r31` copy and emits `lwz r12,0(r31)`, the one byte that kept this
 * unit at 99.92 %. */
#pragma peephole off

extern "C" {

s8  initNetworkSessionStable(struct NetworkSessionStableInit* self);
void constructNetworkSessionObject(void);   /* the session object's constructor, 0x803CF7FC (unclaimed band) */

}

/* The `init` slot's callback argument type.  It is unprototyped here (the target only materialises the
   callback's address into an argument register), so the real six-argument
   `networkSessionReflectCallback` is cast to it at the call site. */
typedef void (*NetworkReflectCallback)();

/* The session the opener builds; declared only so the vtable's +0x0C slot can name its owner. */
typedef struct NetworkSessionStableInit NetworkSessionStableInit;

/* The session object's vtable, indexed by the slots the target dispatches through: +0x0C, +0x54,
 * +0x58, +0x60 and +0x6C.  Only index 1 carries a reconstructed signature; the rest are positional
 * padding.  Declaring them (never defining them) emits no vtable into our object. */
class NetworkSessionObject {
public:
    virtual void v00();
    virtual void init(s32 a, NetworkReflectCallback callback, struct NetworkSessionStableInit* owner,
                      u8* work, s32 flag);
    virtual void v02();
    virtual void v03();
    virtual void v04();
    virtual void v05();
    virtual void v06();
    virtual void v07();
    virtual void v08();
    virtual void v09();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual void setHostInterval(f32 value);
    virtual void setSubhostInterval(f32 value);
    virtual void v21();
    virtual void setConnectionInterval(f32 value);
    virtual void v23();
    virtual void v24();
    virtual s32  getOwnIndex();
};   /* size: 0x04 (the vtable pointer) */

/* The allocation's own type.  The `new` expression is what makes MWCC emit the target's 24-byte `extab`
 * record: its guarded range ends at the `stw` that publishes the result and its last word relocates to
 * `__dl__FPv`, the delete MWCC runs if the constructor throws.  The constructor is inline and calls the
 * neighbouring 0x803CF7FC `constructNetworkSessionObject()`, so the allocation lowers to the target's own
 * `bl __nw__FUl` + `bl constructNetworkSessionObject` pair.  It is deliberately *not* the polymorphic
 * `NetworkSessionObject` view above: `new` on a class with virtuals would also make MWCC emit that class's
 * vtable and store its vptr, which the target does not do - the neighbouring constructor installs the object's
 * real `NetworkSessionStable_VTable` (0x805FA6E8, read off the constructor's own `lis`/`addi` pair) itself.
 * The real class is `NetworkSessionStable` (the neighbouring band's header carries a partial view of it,
 * ending at 0x16D00), so the allocation keeps its own proxy and the size this allocation's own
 * `li r3, 1; addi r3, r3, 27912` gives - 0x16D08, 8 bytes past that view.  Only the size is evidenced
 * here, so the layout stays one documented filler.
 * size: 0x16D08 */
class NetworkSessionObjectAllocation {
public:
    NetworkSessionObjectAllocation() { constructNetworkSessionObject(); }

    /* +0x000 */ u8 pad_00[0x16D08];
};

/* the session the opener builds; offsets are the ones the body addresses */
struct NetworkSessionStableInit {
    /* +0x000 */ u8  pad_000[0x00C];
    /* +0x00C */ NetworkSessionObject* session_0C;
    /* +0x010 */ u8  pad_010[0x3B8];
    /* +0x3C8 */ u8  ready_3C8;
    /* +0x3C9 */ u8  pad_3C9[0x03];
    /* +0x3CC */ u8  work_3CC[0x0C];
};   /* size: 0x3D8 */

s8 initNetworkSessionStable(NetworkSessionStableInit* self)
{
    NetworkSessionObject* session;

    if (self->session_0C != NULL) {
        return -1;
    }
    session = (NetworkSessionObject*)new NetworkSessionObjectAllocation();
    self->session_0C = session;
    if (session == NULL) {
        return -2;
    }
    session->init(1, (NetworkReflectCallback)networkSessionReflectCallback, self, self->work_3CC, 0);
    self->session_0C->setSubhostInterval(networkSessionPeriodSeconds);
    self->session_0C->setConnectionInterval(networkSessionTimeoutSeconds);
    self->session_0C->setHostInterval(networkSessionIntervalSeconds);
    self->ready_3C8 = 1;
    return self->session_0C->getOwnIndex();
}
