/*
 * Network/NetworkSessionManagerPat.cpp - `NetworkSessionManagerPat`'s key function, i.e. the TU in
 * which the class's vtable is emitted (`.text` 0x803D70B8..0x803D72F4, 572 B).
 *
 * WHY THIS UNIT EXISTS.  MWCC emits a class's vtable in the translation unit that defines the class's
 * **key function** - the first non-inline, non-pure virtual declared in the class - and
 * `NetworkSessionManagerPat::move` is that function (`include/Network/fn_803D3CE8.h` declares it first
 * for exactly this reason).  The target says which TU that was: `__vt__24NetworkSessionManagerPat` sits
 * at 0x805FB0F0, one byte past the end of this band's own `.data` run start (0x805FAAD0, where the two
 * Pat message strings and the three jump tables of this band's other functions live), i.e. this band,
 * not `Network/fn_803D3CE8.cpp` - whose object carries the *base* table alone.  So the table is claimed
 * here with the key function, and the class's other ~100 overrides (whose bodies are still `fn_`
 * rows in the two bands) are the residual: see the file header of `Network/fn_803D3CE8.h` for the
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
 * RESIDUALS.  (1) The class overrides five of the table's 114 slots, so the emitted table is right at
 * `+0x08`..`+0x18` and at the 45 slots that inherit the base's implementations (50 of the target's 112
 * relocated slots, plus the two RTTI words), and wrong at the other 62: **48** slots point at this
 * band's functions (0x803D70B8..0x803DDB64) and **14** at adjustor thunks / other bands (0x803DEF38,
 * 0x803DEF4C, 0x803DF010, 0x803DF028, 0x803DF0A4, 0x803DF0C4, 0x803DF154, 0x803DF158, 0x803DF160,
 * 0x803DF168, 0x803DF170, 0x803DF178, 0x803DE56C, 0x803DE5F4); ours hold the base's pure stubs there.
 * The list is regenerated, never transcribed - one command per object, paired by slot offset:
 *     python tools/units/relocaudit.py --unit Network/NetworkSessionManagerPat   # the reference view
 *     build/binutils/powerpc-eabi-objdump.exe -r build/RMHE08/{obj,src}/Network/NetworkSessionManagerPat.o
 * (the `.data` offsets are the slot offsets: `+0x008` is the deleting destructor, `+0x018` the key
 * function, and every slot is 4 bytes, so the 114th ends at `+0x1C8`).  A slot becomes right by
 * declaring that override in the class and reconstructing its body in the band that owns the address;
 * the addresses are the map rows listed above, so each one is a named unit's work, not this unit's.
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
#include "Network/fn_803D3CE8.h"
#include "unsplit/Network.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

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

    if (this->receiver_658 != NULL) {
        receivePatInterfaces(this->receiver_658);
    }
    if (this->requestQueue_65C != NULL) {
        flushPatRequests(this->requestQueue_65C);
    }
    if (GameSpyInterfaceThread_getInstance() != NULL) {
        if (this->field_6E75 != 0) {
            if (GameSpyInterfaceThread_getInstance()->getResult() < 0) {
                GameSpyInterfaceThread_getInstance()->getErrorStruct(&info);
                if (info.code_04 == 0x4B) {
                    ((PatErrorCallback)this->unused_04)(3, 0, info.value_00, 1, &info, this->unused_08);
                    forwarded.value_00 = info.value_00;
                    forwarded.code_04 = info.code_04;
                    forwarded.extra_08 = info.extra_08;
                    ((NetworkInstanceDispatch*)getInstance_())->postError(&forwarded);
                    GameSpyInterfaceThread_getInstance()->clearError();
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
