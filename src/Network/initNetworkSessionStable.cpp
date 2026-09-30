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
 * -1), allocates the session (`NetworkSessionStable`, `Network/NetworkSessionStable.h`), runs its `init`
 * with the reflect callback `networkSessionReflectCallback` and the band's work buffer, seeds the host,
 * subhost and connection intervals from the float pool (0x8079C764 / 0x80793930 / 0x80793934), flips
 * the +0x3C8 ready flag and returns the session's own slot index (the callee's -1 when no slot was
 * free).  Module `Network` (the class names and the registered neighbour).
 *
 * LANGUAGE.  The task proposed this unit as `.c`, but the target allocates through `__nw__FUl`, the
 * C++ global `operator new` (defined in `sys_mem.cpp`), and dispatches five methods through the
 * session object's vtable - both C++-only.  The file is therefore `.cpp`; the function keeps C linkage
 * (`extern "C"`) so the unmangled `initNetworkSessionStable` symbol is unchanged.
 *
 * FLAGS.  Per-unit `-O3` in `configure.py` (the lib default `-O4,p` hoists the constant setup into the
 * prologue and lays the two early returns out inline): the source scores 81.39 % at `-O4,p` and
 * 100.00 % at `-O3` - the same flag shape as the sibling `Network/NetworkSessionManager.cpp`.  C++ exceptions come
 * from the lib flag `-Cpp_exceptions on` (`cflags_network`); they are what makes MWCC emit the
 * `extab`/`extabindex` pair, and the `new` expression gives the record the target's 24-byte shape.
 * `#pragma peephole off` is the last byte of the match: with the pass on MWCC folds the first
 * dispatch's vptr-load base into the `mr r3,r31` copy (`lwz r12,0(r31)`) where retail keeps
 * `lwz r12,0(r3)`.
 *
 * NAMES.  The session is the real class `NetworkSessionStable`; the setters are the virtual slots the
 * base class names (`setHostTimeout`, `setSubhostTimeout`, `setConnectionInterval`).  `getOwnIndex` is
 * declared `s32` on purpose: the target's call site narrows with `extsb`, which MWCC only emits for a
 * callee the caller declares wider than its own `s8` return.
 */
#include "types.h"
#include "sys_mem.h"
#include "Network/NetworkSessionManager.h"
#include "unsplit/NetworkData.h"   /* the band's unowned constants (rule 2) */

/* Retail keeps the unfused `lwz r12,0(r3)` in the first dispatch: with the peephole pass on MWCC folds
 * the base register into the `mr r3,r31` copy and emits `lwz r12,0(r31)`, the one byte that kept this
 * unit at 99.92 %. */
#pragma peephole off

extern "C" {

s8 initNetworkSessionStable(struct NetworkSessionStableInit* self);

}

/* The owner that opens the session: its session pointer at +0x0C, the ready flag at +0x3C8 and the work
   buffer at +0x3CC; offsets are the ones the body addresses. */
struct NetworkSessionStableInit {
    /* +0x000 */ u8  pad_000[0x00C];
    /* +0x00C */ NetworkSessionStable* session_0C;
    /* +0x010 */ u8  pad_010[0x3B8];
    /* +0x3C8 */ u8  ready_3C8;
    /* +0x3C9 */ u8  pad_3C9[0x03];
    /* +0x3CC */ u8  work_3CC[0x0C];
};   /* size: 0x3D8 */

s8 initNetworkSessionStable(NetworkSessionStableInit* self)
{
    NetworkSessionStable* session;

    if (self->session_0C != NULL) {
        return -1;
    }
    session = new NetworkSessionStable();
    self->session_0C = session;
    if (session == NULL) {
        return -2;
    }
    session->init(1, (NetworkSessionCallback)networkSessionReflectCallback, self, self->work_3CC, 0);
    self->session_0C->setSubhostTimeout(networkSessionPeriodSeconds);
    self->session_0C->setConnectionInterval(networkSessionTimeoutSeconds);
    self->session_0C->setHostTimeout(networkSessionIntervalSeconds);
    self->ready_3C8 = 1;
    return self->session_0C->getOwnIndex();
}
