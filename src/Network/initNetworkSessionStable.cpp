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
 * -1), allocates the 0x16D08-byte session object, constructs it, seeds three of its sliders from the
 * float pool (0x8079C764 / 0x80793930 / 0x80793934), installs the reflect callback `networkSessionReflectCallback`
 * through vtable slot +0xC, flips the +0x3C8 ready flag and returns the object's own "start" result.
 * Module `Network` (the class names and the registered neighbour).
 *
 * LANGUAGE.  The task proposed this unit as `.c`, but the target allocates through `__nw__FUl`, the
 * C++ global `operator new` (defined in `sys_mem.cpp`), and dispatches five methods through the
 * session object's vtable - both C++-only.  The file is therefore `.cpp`; the function keeps C linkage
 * (`extern "C"`) so the unmangled `initNetworkSessionStable` symbol is unchanged.  That also satisfies
 * rule 2/9: the allocator is declared once in `include/sys_mem.h` and called as `operator new`.
 *
 * The range's one owned symbol already has its real name (`initNetworkSessionStable`); the two
 * callees it makes were named from their use here (`constructNetworkSessionObject`,
 * `networkSessionReflectCallback`) and the four constants it loads from the band data header, so no
 * generated spelling survives in this file.
 *
 * BODY.  Reconstructed from the disassembly; the allocation/construct sequence and the vtable
 * dispatch are modelled as a class with the target's slots (no vtable is emitted - no virtual is
 * defined here).  Residual: the five dispatch sites are typed by offset only, so the source names are
 * `v03`/`v21`/... rather than the real method names.
 */
#include "types.h"
#include "sys_mem.h"
#include "Network/fn_803D3CE8.h"
#include "unsplit/NetworkData.h"   /* the band's unowned constants (rule 2) */

extern "C" {

s32 initNetworkSessionStable(void* self);
void constructNetworkSessionObject(void);   /* the session object's constructor, 0x803CF7FC (unclaimed band) */

}

/* The session object's vtable, indexed by the slots the target dispatches through: +0x0C, +0x54,
 * +0x58, +0x60, +0x6C.  Only those five carry a reconstructed signature; the rest are positional
 * padding.  Declaring them (never defining them) emits no vtable into our object. */
class NetworkSessionObject {
public:
    virtual void v00();
    virtual void v01();
    virtual void v02();
    virtual void v03(s32 a, void* callback, void* owner, void* work, s32 flag);
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
    virtual void v19();
    virtual void v20();
    virtual void v21(f32 value);
    virtual void v22(f32 value);
    virtual void v23();
    virtual void v24(f32 value);
    virtual void v25();
    virtual void v26();
    virtual s32  v27();
};   /* size: 0x04 (the object is 0x16D08 bytes, allocated elsewhere) */

/* the session the opener builds; offsets are the ones the body addresses */
typedef struct NetworkSessionStableInit {
    /* +0x000 */ u8  pad_000[0x00C];
    /* +0x00C */ NetworkSessionObject* session_0C;
    /* +0x010 */ u8  pad_010[0x3B8];
    /* +0x3C8 */ u32 ready_3C8;
    /* +0x3CC */ u8  work_3CC[0x0C];
} NetworkSessionStableInit;   /* size: 0x3D8 */

s32 initNetworkSessionStable(void* selfv)
{
    NetworkSessionStableInit* self = (NetworkSessionStableInit*)selfv;
    NetworkSessionObject* session;

    if (self->session_0C != NULL) {
        return -1;
    }
    session = (NetworkSessionObject*)operator new(0x16D08);
    if (session != NULL) {
        constructNetworkSessionObject();
    }
    self->session_0C = session;
    if (session == NULL) {
        return -2;
    }
    session->v03(1, (void*)networkSessionReflectCallback, self, self->work_3CC, 0);
    session->v22(networkSessionPeriodSeconds);
    session->v24(networkSessionTimeoutSeconds);
    session->v21(networkSessionIntervalSeconds);
    self->ready_3C8 = 1;
    return session->v27();
}
