/*
 * Network/NetworkSessionManagerPat.cpp - `NetworkSessionManagerPat`'s key function, i.e. the TU in
 * which the class's vtable is emitted (`.text` 0x803D70B8..0x803D72F4, 572 B).
 *
 * PHASE 4 FOLD (docs/splits/phase4, window e).  The unit is now the candidate range 0x803D70B8..0x803E44C8: it also
 * holds `initNetworkSessionStable` (0x803DEA30..0x803DEB38), absorbed from the former Matching unit
 * `Network/initNetworkSessionStable.cpp` (second header below).  That unit was built with `cflags_network` minus
 * `-O4,p` plus `-O3`; this row keeps the survivor's `cflags_main`, so the merged unit is `NonMatching`.  The rest of
 * the range's text (53452 B) is unwritten.
 *
 * WHY THIS UNIT EXISTS.  MWCC emits a class's vtable in the translation unit that defines the class's
 * **key function** - the first non-inline, non-pure virtual declared in the class - and
 * `NetworkSessionManagerPat::move` is that function (`include/Network/NetworkSessionManager.h` declares it first
 * for exactly this reason).  The target says which TU that was: `__vt__24NetworkSessionManagerPat` sits
 * at 0x805FB0F0, one byte past the end of this band's own `.data` run start (0x805FAAD0, where the two
 * Pat message strings and the three jump tables of this band's other functions live), i.e. this band,
 * not `Network/NetworkSessionManager.cpp` - whose object carries the *base* table alone.  So the table is claimed
 * here with the key function, and the class's other ~100 overrides (whose bodies are still `fn_`
 * rows in the two bands) are the residual: see the file header of `Network/NetworkSessionManager.h` for the
 * slot census.
 *
 * SECTIONS.  `.text` 0x803D70B8..0x803D72F4 (exactly the key function's extent), `.data`
 * 0x805FB0F0..0x805FB2B8 (exactly `__vt__24NetworkSessionManagerPat`, 0x1C8 B) and `.sdata2`
 * 0x8079C758..0x8079C75C (4 B = 1.0f, the circle-info interval `move` compares against).  The key
 * function has no `extab`/`extabindex` record (no extabindex entry names it, unlike the 22 functions of
 * the rest of the band), so none is claimed.  The `.data`/`.sdata2` claims are the *owning symbol's*
 * extent, not the whole contiguous `.data` run 0x805FAAD0..0x805FB2B8: that run is this band's, but its
 * strings and jump tables belong to functions that are not reconstructed yet, and claiming bytes our
 * object does not emit would be a claim without an emission (playbook 78).  A later pass that
 * reconstructs them extends the claim to the whole run.
 *
 * RESIDUALS.  (1) The table is now complete at the reloc level: our object carries a relocation at
 * **all 112** slots the target relocates, each naming the symbol the target's object names.  It did not
 * before - the base leaves 62 of those slots pure, so 61 of them were emitted as `0x00000000` and the
 * 62nd as the base's `setFlag79` - and closing them is what `NetworkSessionManagerPat`'s declaration
 * block in `include/Network/NetworkSessionManager.h` is for: one override per filled slot.  What is still
 * UNWRITTEN is the 62 overriding bodies.  They live in bands no unit has claimed (0x803D72F4..0x803DDB64,
 * 0x803DE56C/0x803DE5F4, 0x803DEF38..0x803DF178 - 20,068 B), so each declaration's name and parameter
 * list is a reconstruction from that body: `handleCircleJoin` calls `sendReqCircleJoin`,
 * `getCircleInfoCount` is `lwz r3,0xAF0(r3)`, the 21 request handlers read their argument as a
 * `NetworkRequest*` and call `NetworkRequest_getArgument`.  Where a body identifies nothing, the name is
 * the vtable offset it fills (`slot_068`), which is this class's own scheme (`slot_13C` in the base).
 * The map rows carry those mangled spellings because the TARGET object's relocation name is generated
 * from the map: the pairing is the declaration's, so a later pass that refines a name refines the map
 * row with it.  Census (never transcribe it - regenerate): `python tools/units/vtableaudit.py --at
 * 0x805FB0F8 --json` gives all 112 rows (index, address, target, owner); the `.data` offsets ARE the
 * slot offsets (`+0x008` deleting destructor, `+0x018` the key function, four bytes a slot, 114 words
 * to `+0x1C8`), so both objects' `.rela.data` compare slot for slot.
 * The map boundary this work moved: the target's `+0x03C` points at 0x803DF0C4, a 4-byte `blr` inside
 * what the map had as `fn_803DF0A4`'s 0x24-byte extent.  A slot holds a function entry, and `void f() {}`
 * is exactly `blr` - so 0x803DF0C4 is the empty `setFlag79` override and 0x803DF0A4 ends at 0x20; the
 * extent was shrunk and the 4-byte row added with it.
 * (2) `move` itself is reconstructed from the disassembly and is 8 bytes short of the target's 572
 * (93.13986 %), which also moves the `extabindex` record's length word by the same 8 bytes - that is
 * the only remaining difference in this object's `extab`/`extabindex`.
 * (3) The callback at `+0x04` is called through a cast (`unused_04` is the base's `u32`, and the base's
 * `init(u32, u32)` mangles as `init__21NetworkSessionManagerFUlUl`, so its parameter list cannot be
 * retyped to the callback without renaming that row and every call site).
 * (4) The `.sdata2` word is emitted as the pool entry the original had - anonymous, `@417` here - so it
 * pairs by address but not by symbol name: its bytes are identical and its row does not merge against
 * the map's `lbl_8079C758`.  Naming it would mean claiming a symbol the original did not have, so it
 * stays a pool entry (playbook 58).
 */

/* Absorbed unit `Network/initNetworkSessionStable.cpp` (phase 4 fold):

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

#include "Network/NetworkSessionManager.h"
#include "unsplit/Network.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "types.h"
#include "sys_mem.h"
#include "Network/NetworkSessionManagerPat.h"   /* the unit's own header: its free functions and constants (rule 2) */
#include "Network/network_layer_io.h"            /* sendReqCircleInfoSet - owner Network/network_layer_io.cpp */
#include "Network/session_mediator_views.h"     /* GameSpyInterfaceThread / NetworkErrorInfo (the Pat side's views) */

/* The callback word the base stores at +0x04 and `move` runs: six arguments, the error record it was
 * handed as the fifth and the base's second word as the sixth.  The typedef exists only because the
 * base's field is a `u32` (see the file header) - this is the original's own cast. */
typedef void (*PatErrorCallback)(u32, u32, u32, u32, NetworkErrorInfo*, u32);

/*
 * One frame of the Pat layer's per-frame work, in the order the target runs it: pump the two channels,
 * hand a finished GameSpy error to the session, run the base's own `move`, then - when the session can
 * send and the circle is available - publish the pending circle records and the session name once a
 * second has passed since the last publish.
 */
void NetworkSessionManagerPat::move()
{
    NetworkErrorInfo info;        /* the error record the thread filled in */
    NetworkErrorInfo forwarded;   /* the three words `NetworkInstance::postError` takes back */
    PatCircleInfo circleInfo;     /* 892 B: the pending records + their count + the mode byte */
    char name[0x104];             /* 260 B: the session name `buildCircleInfoName` packs */

    if (this->tcp_658 != NULL) {
        this->tcp_658->move();
    }
    if (this->udp_65C != NULL) {
        this->udp_65C->move();
    }
    if (GameSpyInterfaceThread::getInstance() != NULL) {
        if (this->field_6E75 != 0) {
            if (GameSpyInterfaceThread::getInstance()->getResult() < 0) {
                GameSpyInterfaceThread::getInstance()->getErrorStruct(&info);
                if (info.code_04 == 0x4B) {
                    ((PatErrorCallback)this->unused_04)(3, 0, info.value_00, 1, &info, this->unused_08);
                    forwarded.value_00 = info.value_00;
                    forwarded.code_04 = info.code_04;
                    forwarded.extra_08 = info.extra_08;
                    ((NetworkInstanceDispatch*)getInstance_())->postError(&forwarded);
                    GameSpyInterfaceThread::getInstance()->clearError();
                }
            }
        }
    }
    NetworkSessionManager::move();
    if (this->canSend_28() != 0) {
        if (circleAvailable(this) != 0) {
            if (this->circleRecordCount_950 != 0 || this->nameList_7A0.count_04 != 0 ||
                this->field_AE6 != 0) {
                if (getNetworkLogger()->getTime_60() > 1.0f + this->field_3B8) {
                    this->field_3B8 = getNetworkLogger()->getTime_60();
                    memset(&circleInfo, 0, sizeof(circleInfo));
                    if (this->circleRecordCount_950 != 0) {
                        u32 count = this->circleRecordCount_950;

                        if (count > 256) {
                            count = 256;
                        }
                        circleInfo.recordCount_156 = (u16)count;
                        memcpy(circleInfo.records_56, this->circleRecords_954,
                               circleInfo.recordCount_156);
                        this->circleRecordCount_950 = 0;
                    }
                    if (this->field_AE6 != 0) {
                        circleInfo.mode_378 = (this->field_AE5 != 0) ? 2 : 1;
                        this->field_AE6 = 0;
                    }
                    memset(name, 0, sizeof(name));
                    if (this->nameList_7A0.count_04 != 0) {
                        buildCircleInfoName(this, name, &this->nameList_7A0);
                        this->nameList_7A0.count_04 = 0;
                    }
                    sendReqCircleInfoSet(getInstance_(), this->circleInfoRequestId_41C, &circleInfo,
                                         name);
                }
            }
        }
    }
}


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
