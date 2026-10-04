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
 * (2) `move` matches (572 B): the error record is the 12-byte signed `NetworkErrorInfo` view forwarded by value
 * (`NetworkPostedError`), the publish test is `1.0f + last < now`, the record count is the `>= 256 ? 256 : n`
 * ternary and the mode byte is signed `set ? 1 : 2`.  PEEPHOLE: the pass is off file-wide (from `move` on; it
 * also closes `slot_1B8`), on again only around `getTimeSincePublish` (97.86 on, 96.25 off).
 * (3) The callback at `+0x04` is called through a cast (`unused_04` is the base's `u32`, and the base's
 * `init(u32, u32)` mangles as `init__21NetworkSessionManagerFUlUl`, so its parameter list cannot be
 * retyped to the callback without renaming that row and every call site).
 * (4) The `.sdata2` word is emitted as the pool entry the original had - anonymous, `@417` here - so it
 * pairs by address but not by symbol name: its bytes are identical and its row does not merge against
 * the map's `lbl_8079C758`.  Naming it would mean claiming a symbol the original did not have, so it
 * stays a pool entry (playbook 58).
 * (5) Bodies written by the network pilot (lane L2): the circle/player getters 0x803DBA2C..0x803DC0E4 and the
 * setters/stubs 0x803DEF38..0x803DF1CC.  Open residuals in them, each blocked on a header another lane owns:
 *   - `NetworkSmallObject` is a struct with a vtable member (`Network/network_writer_types.h`), so a +0x28
 *     dispatch loads the table through the object's own register (`lwz r5,0(r5)`) where retail moves the
 *     object into r3 first (`mr r3,r5; lwz r12,0(r3)`) - the class form of a virtual call
 *     (`exportCircleItem`, `getPlayerRecord`); its +0x28 slot is also typed `const u8*` where both
 *     callers pass a `NetworkSmallObject*` (the cast at each site).
 *   - `NetworkSessionBase::getUserFlagB` (+0x84, `Network/network_transport_types.h`) returns `u8`, so our
 *     callers re-extend it (`clrlwi.`) where retail uses the full word (`cmpwi r3,0` in
 *     `getTimeSincePublish`, a plain `bctr` tail call in `isNetworkSessionManagerPatReady`).
 *   - The base's +0x0C field is typed `NetworkBuffer*` but holds the `NetworkSessionBase` that
 *     `initNetworkSessionStable` allocates (its slots +0x3C/+0x84 are the session's `post`/`getUserFlagB`);
 *     the Pat bodies read it through a `NetworkSessionBase*` cast until the field is retyped (it changes
 *     `Network/NetworkSessionManager.cpp`'s calls, so it is that unit's measured pass).
 * Names derived here (GUESS, from the bodies): `setCircleComment` (0x803DF0C8, copies <= 144 chars to +0xA54),
 * `setCircleMode` (0x803DF144, the +0xAE5 byte and the +0xAE6 pending flag `move` consumes), `post` (0x803DF180,
 * forwards to the session's +0x3C `post`); the circle/player counters are named limit/used from the getters'
 * subtraction (GUESS).
 * (6) NetworkLayer (pilot L2): 0x803DF2EC..0x803E0BE8 is the layer base class `NetworkLayer` (its own
 * "NetworkLayer::move"/"::deleteRequest" strings, table 0x805FB5D0 = `__vt__12NetworkLayer`, emitted here from
 * the class - key function `~NetworkLayer`), declared in `Network/NetworkLayerPat.h`.  It parallels
 * `NetworkSessionManager` one word earlier; its request record `NetworkLayerRequest` carries a real
 * pointer-to-member handler (the reset is MWCC's null-member-pointer copy of `__ptmf_null`, `run` its
 * `__ptmf_scall` call) and a real ctor/dtor (the pool's `__construct_array` pair).  The 22 descriptor
 * constants are the named `NetworkLayerHandler` globals `networkLayerRequestDescNN` (map rows renamed).
 * Residuals: `NetworkLayer::move` keeps the unrolled inner scan's trip count 3 in r0 where retail holds it in
 * a callee-saved register (r29) - one extra saved register shifts every allocation; the constructor's
 * second pool loop is spelled with an explicit pointer (98.17; the indexed spelling scores 97.32) and still
 * swaps the counter/pointer registers.
 * (7) Circle list and player records (pilot L2, 0x803DDA90..0x803DE56C): the circle entries are filled from the
 * received `PatCircleInfo` block, the players from the reflection handlers; names are GUESSES from the bodies
 * (`setCircleInfo`/`addCircleInfo`/`removeCircleInfo`, `resetPlayerRecord`/`addPlayerRecord`/
 * `removePlayerRecord`/`updatePlayerRecord`, `packCircleOptions`; `postEvent`'s fifth argument is a per-event
 * payload - an error record, an index or a state byte).  Residuals: the address object is 0x20 B but the
 * shared `NetworkSmallObject` type is 0x10 B, so `updatePlayerRecord`'s and `slot_1C4`'s stack copy gets a
 * 0x30 frame where retail's is 0x40; `setCircleInfo` compares the option's enable byte with `cmplwi` where retail has
 * `cmpwi` (u8, s8 and bool spellings tried); `packCircleOptions` addresses the name entries from the list
 * base where retail strength-reduces a pointer at +0x08 (the explicit pointer spelling swaps two registers,
 * 96.16 vs 98.32).
 * SEAM (not ours to move - pilot rule 5, filed as a request): `datagap.py` reads the claimed `.data` as at least
 * three TUs (seams in [0x805FB2B8, 0x805FB5D0) and [0x805FB718, 0x805FC1E0)): this manager, `NetworkLayer`
 * (`.text` from 0x803DF2EC) and `NetworkLayerPat` (from its constructor 0x803E0C18).
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
#include "MSL/strlen.h"
#include "types.h"
#include "sys_mem.h"
#include "Network/NetworkSessionManagerPat.h"   /* the unit's own header: its free functions and constants (rule 2) */
#include "Network/network_layer_io.h"            /* sendReqCircleInfoSet - owner Network/network_layer_io.cpp */
#include "Network/NetworkLayerPat.h"             /* NetworkLayer - the layer base class this unit defines */
#include "Network/NetworkCommunityPat.h"         /* networkSmallObject_construct/_setAddress - owner Network/NetworkCommunityPat.cpp */
#include "Network/NetworkPeerBase.h"             /* NetworkSmallObjectSink::destroy - owner Network/NetworkPeerBase.cpp */
#include "Network/NetworkSessionBase.h"          /* LockMutex/UnlockMutex - owner Network/NetworkSessionBase.cpp */
#include "Network/session_mediator_views.h"     /* GameSpyInterfaceThread / NetworkErrorInfo (the Pat side's views) */

/* The callback word the base stores at +0x04 and `move` runs: six arguments, the error record it was
 * handed as the fifth and the base's second word as the sixth.  The typedef exists only because the
 * base's field is a `u32` (see the file header) - this is the original's own cast. */
#pragma peephole off
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
    PatCircleInfo circleInfo;     /* 892 B: the pending records + their count + the mode byte */
    PatCircleOptionList options;  /* 260 B: the name-list options `buildCircleInfoName` packs */

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
                    ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&info);
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
                if (1.0f + this->field_3B8 < getNetworkLogger()->getTime_60()) {
                    this->field_3B8 = getNetworkLogger()->getTime_60();
                    memset(&circleInfo, 0, sizeof(circleInfo));
                    if (this->circleRecordCount_950 != 0) {
                        circleInfo.recordCount_156 =
                            (this->circleRecordCount_950 >= 256) ? 256 : this->circleRecordCount_950;
                        memcpy(circleInfo.records_56, this->circleRecords_954,
                               circleInfo.recordCount_156);
                        this->circleRecordCount_950 = 0;
                    }
                    if (this->field_AE6 != 0) {
                        circleInfo.mode_378 = (this->field_AE5 != 0) ? 1 : 2;
                        this->field_AE6 = 0;
                    }
                    memset(&options, 0, sizeof(options));
                    if (this->nameList_7A0.count_04 != 0) {
                        buildCircleInfoName(this, &options, &this->nameList_7A0);
                        this->nameList_7A0.count_04 = 0;
                    }
                    sendReqCircleInfoSet(getInstance_(), this->circleInfoRequestId_41C, &circleInfo,
                                         (const char*)&options);
                }
            }
        }
    }
}

/* Reports error 0x80050011 for the request slot +0x184 and finishes. */
s32 NetworkSessionManagerPat::slot_184(NetworkRequest* request)
{
    NetworkErrorInfo info;

    info.value_00 = 0x80050011;
    info.code_04 = 0;
    info.extra_08 = 0;
    postEvent(6, 0, info.value_00, 1, &info, this->unused_08);
    return 1;
}

/* Reports event 30 with this console's slot index and finishes. */
s32 NetworkSessionManagerPat::slot_1B8(NetworkRequest* request)
{
    postEvent(30, this->selfIndex_536, 0, 0, NULL, this->unused_08);
    return 1;
}

/* Reports error 0x80050011 for the request slot +0x1A8 and finishes. */
s32 NetworkSessionManagerPat::slot_1A8(NetworkRequest* request)
{
    NetworkErrorInfo info;

    info.value_00 = 0x80050011;
    info.code_04 = 0;
    info.extra_08 = 0;
    postEvent(23, 0, info.value_00, 1, &info, this->unused_08);
    return 1;
}

/* Returns how many circles the last list query filled in. */
s32 NetworkSessionManagerPat::getCircleInfoCount()
{
    return this->circleList_AF0.count_00;
}

/* Empty in the Pat layer. */
void NetworkSessionManagerPat::slot_06C()
{
}

/* Empty in the Pat layer. */
void NetworkSessionManagerPat::slot_068()
{
}

/* Copies circle `idx`'s name into `dst` (at most `size` bytes, always terminated). */
void NetworkSessionManagerPat::getCircleItemName(char* dst, s32 size, s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return;
    }
    if (size > 0) {
        if (size > sizeof(this->circleList_AF0.items_04[0].name_008)) {
            size = sizeof(this->circleList_AF0.items_04[0].name_008);
        }
        memcpy(dst, this->circleList_AF0.items_04[idx].name_008, size - 1);
        dst[size - 1] = 0;
    }
}

/* Copies circle `idx`'s address object into `dst`. */
void NetworkSessionManagerPat::exportCircleItem(NetworkSmallObject* dst, s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return;
    }
    if (dst != NULL) {
        dst->vtable->slot_28(dst, (const u8*)&this->circleList_AF0.items_04[idx].smallObject_108);
    }
}

/* Circle `idx`'s first limit counter, 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemWord_170(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].limitA_170;
}

/* Circle `idx`'s second limit counter, 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemWord_174(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].limitB_174;
}

/* Circle `idx`'s first used counter, 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemWord_178(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].usedA_178;
}

/* Circle `idx`'s second used counter, 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemWord_17C(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].usedB_17C;
}

/* Circle `idx`'s first free count (limit minus used), 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemSize_170_178(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].limitA_170 - this->circleList_AF0.items_04[idx].usedA_178;
}

/* Circle `idx`'s second free count (limit minus used), 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemSize_174_17C(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].limitB_174 - this->circleList_AF0.items_04[idx].usedB_17C;
}

/* Copies circle `idx`'s 0x48-byte record block into `dst`. */
void NetworkSessionManagerPat::getCircleItemRecord(char* dst, s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return;
    }
    if (dst != NULL) {
        memcpy(dst, &this->circleList_AF0.items_04[idx].options_128,
               sizeof(this->circleList_AF0.items_04[idx].options_128));
    }
}

/* Circle `idx`'s flag byte, 0 for an index out of range. */
u32 NetworkSessionManagerPat::getCircleItemByte_180(s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return 0;
    }
    return this->circleList_AF0.items_04[idx].flag_180;
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_09C()
{
    return 0;
}

/* Clears `dst` to the empty string when it has room. */
void NetworkSessionManagerPat::clearString(char* dst, s32 size)
{
    if (size > 0) {
        dst[0] = 0;
    }
}

u32 NetworkSessionManagerPat::getWord_528()
{
    return this->limitA_528;
}

s32 NetworkSessionManagerPat::getWord_524()
{
    return this->limitB_524;
}

u32 NetworkSessionManagerPat::getWord_530()
{
    return this->usedA_530;
}

u32 NetworkSessionManagerPat::getWord_52C()
{
    return this->usedB_52C;
}

u32 NetworkSessionManagerPat::getSize_528_530()
{
    return this->limitA_528 - this->usedA_530;
}

u32 NetworkSessionManagerPat::getSize_524_52C()
{
    return this->limitB_524 - this->usedB_52C;
}

/* Copies player `idx`'s name into `dst` (at most `size` bytes); empty for an absent player. */
void NetworkSessionManagerPat::getPlayerRecordName(s8 idx, char* dst, s32 size)
{
    if (size > 0) {
        if ((u8)idx > 3 || this->players_538[idx].active_00 == 0) {
            dst[0] = 0;
        } else {
            if (size > sizeof(this->players_538[0].name_28)) {
                size = sizeof(this->players_538[0].name_28);
            }
            memcpy(dst, this->players_538[idx].name_28, size - 1);
            dst[size - 1] = 0;
        }
    }
}

/* Clears `dst` to the empty string when it has room; the id is unused in the Pat layer. */
void NetworkSessionManagerPat::clearStringWithId(u32 id, char* dst, s32 size)
{
    if (size > 0) {
        dst[0] = 0;
    }
}

/* Copies player `idx`'s address object into `dst`; false for an absent player or no destination. */
s32 NetworkSessionManagerPat::getPlayerRecord(s8 idx, NetworkSmallObject* dst)
{
    if ((u8)idx > 3) {
        return 0;
    }
    if (this->players_538[idx].active_00 == 0) {
        return 0;
    }
    if (dst == NULL) {
        return 0;
    }
    dst->vtable->slot_28(dst, (const u8*)&this->players_538[idx].smallObject_08);
    return 1;
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_0FC()
{
    return 0;
}

u8 NetworkSessionManagerPat::getByte_534()
{
    return this->flag_534;
}

/* Seconds since the timestamp at +0x3BC, 0 while the session is missing or not flagged. */
#pragma peephole on
f32 NetworkSessionManagerPat::getTimeSincePublish()
{
    NetworkSessionBase* session = (NetworkSessionBase*)this->buffer;

    if (session == NULL || session->getUserFlagB() == 0) {
        return networkRequestTimerReset;
    }
    return getNetworkLogger()->getTime_60() - this->field_3BC;
}
#pragma peephole off


/* Clears circle entry `index` (0..31): ids, name, address, options, counters, records and comment. */
void networkPatResetCircleInfo(NetworkSessionManagerPat* self, s32 index)
{
    NetworkSessionCircleInfo* item;

    if ((u32)index > 31) {
        return;
    }
    item = &self->circleList_AF0.items_04[index];
    item->id_000 = 0;
    item->ownerId_004 = 0;
    item->name_008[0] = 0;
    item->smallObject_108.vtable->slot_18(&item->smallObject_108);
    item->limitA_170 = 0;
    item->limitB_174 = 0;
    item->usedA_178 = 0;
    item->usedB_17C = 0;
    item->options_128.count_04 = 0;
    item->flag_180 = 0;
    item->recordCount_184 = 0;
    item->comment_288[0] = 0;
}

/* Stores circle `index` and grows the list count to cover it. */
void NetworkSessionManagerPat::addCircleInfo(s32 index, const PatCircleInfo* info, PatCircleOptionList* options)
{
    if ((u32)index > 31) {
        return;
    }
    setCircleInfo(index, info, options);
    if (this->circleList_AF0.count_00 <= index) {
        this->circleList_AF0.count_00 = index + 1;
    }
}

/* Clears circle `index`; when it was the last one, shrinks the count past the trailing free entries. */
void NetworkSessionManagerPat::removeCircleInfo(s32 index)
{
    if ((u32)index > 31) {
        return;
    }
    networkPatResetCircleInfo(this, index);
    if (this->circleList_AF0.count_00 <= index + 1) {
        for (index--; index >= 0; index--) {
            if (this->circleList_AF0.items_04[index].id_000 != 0) {
                break;
            }
        }
        this->circleList_AF0.count_00 = index + 1;
    }
}

/* Copies a received circle block into circle entry `index`, then enables the option slots the option list
 * names (at most 32 entries are read; the list's count is clamped in place). */
void NetworkSessionManagerPat::setCircleInfo(s32 index, const PatCircleInfo* info, PatCircleOptionList* options)
{
    NetworkSessionCircleInfo* item;
    u8 i;

    if ((u32)index > 31) {
        return;
    }
    if (info == NULL) {
        return;
    }
    item = &this->circleList_AF0.items_04[index];
    item->id_000 = info->id_000;
    item->ownerId_004 = info->ownerId_368;
    memcpy(item->name_008, info->name_004, 63);
    item->name_008[63] = 0;
    networkSmallObject_setAddress(&item->smallObject_108, 3, info->address_370, sizeof(info->address_370));
    item->limitA_170 = info->limitA_358;
    item->limitB_174 = info->limitB_360;
    item->usedA_178 = info->usedA_35C;
    item->usedB_17C = info->usedB_364;
    item->options_128.count_04 = 0;
    item->flag_180 = info->flag_044 != 0;
    item->recordCount_184 = (info->recordCount_156 > 256) ? 256 : info->recordCount_156;
    memcpy(item->records_188, info->records_56, item->recordCount_184);
    memcpy(item->comment_288, info->comment_158, sizeof(info->comment_158));
    item->comment_288[sizeof(info->comment_158)] = 0;
    item->active_319 = (info->state_379 != 0 && info->state_379 != -1);
    if (options == NULL) {
        return;
    }
    if (options->count_00 > 32) {
        options->count_00 = 32;
    }
    for (i = 0; i < options->count_00; i++) {
        PatCircleOption* option = &options->entries_04[i];
        u32 slot = option->slot_00 - 1;

        if (slot < 8 && option->enabled_01 == 1) {
            item->options_128.slots_08[slot].enabled_00 = 1;
            item->options_128.slots_08[slot].value_04 = option->value_04;
            if (item->options_128.count_04 <= slot) {
                item->options_128.count_04 = slot + 1;
            }
        }
    }
}

/* Adds the received circle at its list slot and reports it (event 40) unless it is our own circle or a
 * circle-list request is running. */
void createCircleLayer(NetworkSessionManagerPat* self, const PatCircleInfo* info, PatCircleOptionList* options)
{
    s32 index;

    if (self->field_6E74 == 0) {
        return;
    }
    index = info->slotNumber_36C - 1;
    if ((u32)index > 31) {
        return;
    }
    self->addCircleInfo(index, info, options);
    if (self->requests_10[5] == 0 && self->circleInfoRequestId_41C != info->id_000) {
        self->postEvent(40, 0, 0, 1, &index, self->unused_08);
    }
}

/* Removes the circle with `id` from the list and reports it (event 42) unless a circle-list request is
 * running. */
void deleteCircleListLayer(NetworkSessionManagerPat* self, s32 id)
{
    s32 index;

    if (self->field_6E74 == 0) {
        return;
    }
    for (index = 0; index < 32; index++) {
        if (id == self->circleList_AF0.items_04[index].id_000) {
            self->removeCircleInfo(index);
            if (self->requests_10[5] == 0) {
                self->postEvent(42, 0, 0, 1, &index, self->unused_08);
            }
            return;
        }
    }
}

/* Applies a received circle update: slot 0 removes the circle, a free slot re-adds it, the same circle is
 * refreshed and reported (event 41). */
void changeCircleListLayer(NetworkSessionManagerPat* self, const PatCircleInfo* info, PatCircleOptionList* options)
{
    s32 index;
    s32 id;

    if (self->field_6E74 == 0) {
        return;
    }
    index = info->slotNumber_36C - 1;
    if (index == -1) {
        deleteCircleListLayer(self, info->id_000);
    } else if ((u32)index <= 31) {
        id = self->circleList_AF0.items_04[index].id_000;
        if (id == 0) {
            deleteCircleListLayer(self, info->id_000);
            createCircleLayer(self, info, options);
        } else if (info->id_000 == id) {
            self->setCircleInfo(index, info, options);
            if (self->requests_10[5] == 0) {
                self->postEvent(41, 0, 0, 1, &index, self->unused_08);
            }
        }
    }
}

/* Clears player record `index` (0..3). */
void NetworkSessionManagerPat::resetPlayerRecord(s8 index)
{
    NetworkSessionPlayerRecord* player;

    if ((u8)index > 3) {
        return;
    }
    player = &this->players_538[index];
    player->active_00 = 0;
    player->announced_01 = 0;
    player->flag_02 = 0;
    player->state_03 = 0;
    player->linked_04 = 0;
    player->smallObject_08.vtable->slot_18(&player->smallObject_08);
    memset(player->name_28, 0, sizeof(player->name_28));
    player->value_3C = 0;
    memset(&player->address_40, 0, sizeof(player->address_40));
}

/* Takes player record `index`: fills it, counts the player and, when asked, posts the join event (7). */
void NetworkSessionManagerPat::addPlayerRecord(s8 index, const u8* address, const char* name, u32 state, s32 counted,
                                               s32 notify)
{
    NetworkSessionPlayerRecord* player;

    if (address == NULL || (u8)index > 3) {
        return;
    }
    player = &this->players_538[index];
    resetPlayerRecord(index);
    player->active_00 = 1;
    updatePlayerRecord(index, address, name, state);
    this->limitA_528++;
    if (counted != 0) {
        this->usedA_530++;
    }
    if (notify != 0) {
        postEvent(7, index, 0, 0, NULL, this->unused_08);
        player->announced_01 = 1;
    }
}

/* Drops player record `index`: uncounts it, removes its Udp peer and posts the leave event (17) when the
 * join was posted. */
void NetworkSessionManagerPat::removePlayerRecord(s8 index, s32 counted)
{
    NetworkSessionPlayerRecord* player;

    if ((u8)index > 3) {
        return;
    }
    player = &this->players_538[index];
    if (player->active_00 == 0) {
        return;
    }
    this->limitA_528--;
    if (counted != 0) {
        this->usedA_530--;
    }
    if (this->udp_65C != NULL) {
        this->udp_65C->remove(&this->players_538[index].address_40);
    }
    if (this->buffer != NULL) {
        player->linked_04 = 0;
    }
    if (player->announced_01 != 0) {
        postEvent(17, index, 0, 0, NULL, this->unused_08);
        player->announced_01 = 0;
    }
    player->active_00 = 0;
}

/* Refreshes an active player record: its address (from 8 raw bytes), its name (19 characters) and its state
 * byte, posting event 27 when the state of an announced player changes. */
void NetworkSessionManagerPat::updatePlayerRecord(s8 index, const u8* address, const char* name, u32 state)
{
    NetworkSessionPlayerRecord* player;
    NetworkSmallObject object;

    networkSmallObject_construct(&object);
    if ((u8)index > 3) {
        NetworkSmallObjectSink::destroy(&object);
        return;
    }
    player = &this->players_538[index];
    if (player->active_00 == 0) {
        NetworkSmallObjectSink::destroy(&object);
        return;
    }
    if (address != NULL) {
        networkSmallObject_setAddress(&object, 3, address, 8);
        player->smallObject_08.vtable->slot_28(&player->smallObject_08, (const u8*)&object);
    }
    if (name != NULL) {
        memcpy(player->name_28, name, sizeof(player->name_28) - 1);
        player->name_28[sizeof(player->name_28) - 1] = 0;
    }
    if (player->state_03 != state) {
        player->state_03 = state;
        if (player->announced_01 != 0) {
            postEvent(27, index, 0, 1, &player->state_03, this->unused_08);
        }
    }
    NetworkSmallObjectSink::destroy(&object);
}

/* Packs the enabled entries of a name list (at most eight, the list's count clamped in place) into `dst`,
 * numbering each by its position; returns how many were packed (at most `max`). */
s32 NetworkSessionManagerPat::packCircleOptions(PatCircleOption* dst, s32 max, NetworkNameList* src)
{
    s32 count;
    u32 i;

    if (dst == NULL || max <= 0 || src == NULL) {
        return 0;
    }
    count = 0;
    if (src->count_04 > 8) {
        src->count_04 = 8;
    }
    for (i = 0; i < src->count_04; i++) {
        dst->slot_00 = i + 1;
        if (src->entries_08[i].enabled_00 == 1) {
            dst->enabled_01 = 1;
            dst->value_04 = src->entries_08[i].value_04;
            dst++;
            count++;
            if (count >= max) {
                return count;
            }
        }
    }
    return count;
}

/* Packs the name list's enabled entries into the circle-info option list `dst` (at most 32). */
void buildCircleInfoName(NetworkSessionManagerPat* self, PatCircleOptionList* dst, NetworkNameList* src)
{
    if (dst != NULL) {
        dst->count_00 = self->packCircleOptions(dst->entries_04, 32, src);
    }
}

/* The layer's member slot of player `value` (0..3), -1 for an absent player or no layer. */
s8 NetworkSessionManagerPat::mapId_1C0(s32 value)
{
    if ((u8)value <= 3 && this->players_538[(s8)value].active_00 != 0 &&
        getNetworkLayerPat(getPatsObject(), 0) != NULL) {
        return getNetworkLayerPat(getPatsObject(), 0)->getMemberSlot(&this->players_538[(s8)value].smallObject_08);
    }
    return -1;
}

/* The player record holding the address of the layer's member slot `value`, -1 when none does. */
s32 NetworkSessionManagerPat::slot_1C4(s8 value)
{
    NetworkSmallObject id;
    s32 i;

    if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
        networkSmallObject_construct(&id);
        getNetworkLayerPat(getPatsObject(), 0)->getMemberAddress(value, &id);
        if (networkSmallObject_isValid(&id) != 0) {
            for (i = 0; i < 4; i++) {
                if (this->players_538[i].active_00 != 0 &&
                    networkSmallObject_isEqual(&id, &this->players_538[i].smallObject_08) != 0) {
                    NetworkSmallObjectSink::destroy(&id);
                    return i;
                }
            }
        }
        NetworkSmallObjectSink::destroy(&id);
    }
    return -1;
}

/* The player slot whose address object equals `id`; -1 (and a warning) when none does. */
s32 NetworkSessionManagerPat::uniqueIdToMember(const NetworkSmallObject* id)
{
    s32 i;

    if (id != NULL) {
        for (i = 0; i < 4; i++) {
            if (this->players_538[i].active_00 != 0 &&
                networkSmallObject_isEqual(id, &this->players_538[i].smallObject_08) != 0) {
                return i;
            }
        }
    }
    getNetworkLogger()->warn_10("NetworkSessionManagerPat::uniqueIdToMember: invalide uniqueId\n");
    return -1;
}

/* True when this console holds the host slot (both indices valid and equal). */
s32 circleAvailable(NetworkSessionManagerPat* self)
{
    if (self->hostIndex_537 < 0) {
        return 0;
    }
    if (self->selfIndex_536 < 0) {
        return 0;
    }
    return self->hostIndex_537 == self->selfIndex_536;
}

/* Drops everything queued for the next circle publish: the name list, the session name, the records and the
 * comment, and the pending mode. */
void NetworkSessionManagerPat::resetCircleState()
{
    this->nameList_7A0.count_04 = 0;
    memset(this->sessionName_850, 0, sizeof(this->sessionName_850));
    this->circleRecordCount_950 = 0;
    memset(this->circleComment_A54, 0, sizeof(this->circleComment_A54));
    this->field_AE5 = 0;
    this->field_AE6 = 0;
}

/* Re-posts the events of every remote player: the join (7) when not yet announced, the state (27) and the
 * +0x3C value (21) when set. */
void NetworkSessionManagerPat::announcePlayers()
{
    s8 i;

    for (i = 0; i < getWord_524(); i++) {
        if (this->players_538[i].active_00 != 0 && i != this->selfIndex_536) {
            if (this->players_538[i].announced_01 != 1) {
                postEvent(7, i, 0, 0, NULL, this->unused_08);
                this->players_538[i].announced_01 = 1;
            }
            if (this->players_538[i].state_03 != 0) {
                postEvent(27, i, 0, 1, &this->players_538[i].state_03, this->unused_08);
            }
            if (this->players_538[i].value_3C != 0) {
                postEvent(21, i, 0, 1, &this->players_538[i].value_3C, this->unused_08);
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

/* Marks the session joined and sets its subhost and host timeouts (10 s / 20 s); -1 while there is no
 * GameSpy thread, no session or the manager is not ready. */
s32 NetworkSessionManagerPat::joinSession()
{
    NetworkSessionBase* session;

    if (GameSpyInterfaceThread::getInstance() == NULL || (session = (NetworkSessionBase*)this->buffer) == NULL ||
        this->field_3C8 == 0) {
        return -1;
    }
    session->markJoined();
    ((NetworkSessionBase*)this->buffer)->setSubhostTimeout(10.0f);
    ((NetworkSessionBase*)this->buffer)->setHostTimeout(20.0f);
    return 0;
}

/* Resets one slot of the session, when there is one. */
void NetworkSessionManagerPat::resetSessionSlot(s8 index)
{
    NetworkSessionBase* session = (NetworkSessionBase*)this->buffer;

    if (session != NULL) {
        session->resetSlot(index);
    }
}

/* Clears the ready flag, drops every connection of the session and releases the manager's buffers. */
void closeNetworkSessionManagerPat(NetworkSessionManagerPat* self)
{
    self->field_3C8 = 0;
    if (self->buffer != NULL) {
        ((NetworkSessionBase*)self->buffer)->disconnectAll();
        ((NetworkSessionBase*)self->buffer)->resetAllSlots();
    }
    networkPatReleaseBuffer(self);
}

/* Logs and hands the host connection index to the session (indices 0..3 only). */
void NetworkSessionManagerPat::setHostConnectionIndex(s8 index)
{
    getNetworkLogger()->signal_0C(2, "setHostConnectionIndex:%d\n", index);
    if ((u8)index <= 3 && this->buffer != NULL) {
        ((NetworkSessionBase*)this->buffer)->setHostIndex(index);
    }
}

/* True once the circle-info request id has been assigned. */
u32 NetworkSessionManagerPat::canSend_28()
{
    return this->circleInfoRequestId_41C > 0;
}

/* Merges `src`'s name entries into the manager's list (at most eight; an unset entry keeps the old one),
 * or takes the whole list when the manager's own is empty. */
void NetworkSessionManagerPat::copyNameList(const NetworkNameList* src)
{
    u32 i;

    if (src == NULL) {
        return;
    }
    if (this->nameList_7A0.count_04 != 0) {
        if (this->nameList_7A0.count_04 < src->count_04) {
            this->nameList_7A0.count_04 = src->count_04;
        }
        if (this->nameList_7A0.count_04 > 8) {
            this->nameList_7A0.count_04 = 8;
        }
        for (i = 0; i < this->nameList_7A0.count_04; i++) {
            if (src->entries_08[i].enabled_00 != 0) {
                memcpy(&this->nameList_7A0.entries_08[i], &src->entries_08[i], sizeof(NetworkNameEntry));
            }
        }
    } else {
        memcpy(&this->nameList_7A0, src, sizeof(NetworkNameList));
    }
}

/* Copies the 0x68-byte block that follows the name list. */
void NetworkSessionManagerPat::copyNameListTail(const u8* src)
{
    if (src != NULL) {
        memcpy(this->nameListTail_7E8, src, sizeof(this->nameListTail_7E8));
    }
}

/* Stores the session name, truncated to 255 characters. */
void NetworkSessionManagerPat::setSessionName(const char* name)
{
    u32 length = (strlen(name) < sizeof(this->sessionName_850) - 1) ? strlen(name)
                                                                     : sizeof(this->sessionName_850) - 1;

    memcpy(this->sessionName_850, name, length);
    this->sessionName_850[length] = 0;
}

/* Queues up to 256 circle record bytes for the next publish. */
void NetworkSessionManagerPat::setCircleRecords(const u8* src, u32 count)
{
    u32 n = (count > sizeof(this->circleRecords_954)) ? sizeof(this->circleRecords_954) : count;

    this->circleRecordCount_950 = n;
    memcpy(this->circleRecords_954, src, n);
}

/* Empty in the Pat layer. */
void NetworkSessionManagerPat::setFlag79(s8 value)
{
}

/* Stores the circle comment, truncated to 144 characters. */
void NetworkSessionManagerPat::setCircleComment(const char* comment)
{
    u32 length = (strlen(comment) < sizeof(this->circleComment_A54) - 1) ? strlen(comment)
                                                                          : sizeof(this->circleComment_A54) - 1;

    memcpy(this->circleComment_A54, comment, length);
    this->circleComment_A54[length] = 0;
}

/* Records the circle mode and flags it for the next publish. */
void NetworkSessionManagerPat::setCircleMode(u8 mode)
{
    this->field_AE5 = mode;
    this->field_AE6 = 1;
}

/* Empty in the Pat layer. */
void NetworkSessionManagerPat::slot_108()
{
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_10C()
{
    return 0;
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_110()
{
    return 0;
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_114()
{
    return 0;
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_118()
{
    return 0;
}

/* Always 0 in the Pat layer. */
u32 NetworkSessionManagerPat::slot_11C()
{
    return 0;
}

/* Hands a packet to the session, when there is one. */
void NetworkSessionManagerPat::post(const u8* data, s32 size, s8 channel, s8 index)
{
    NetworkSessionBase* session = (NetworkSessionBase*)this->buffer;

    if (session != NULL) {
        session->post(data, size, channel, index);
    }
}

/* Whether the manager's session reports itself ready (false while there is no session). */
BOOL isNetworkSessionManagerPatReady(NetworkSessionManagerPat* session_manager)
{
    NetworkSessionBase* session = (NetworkSessionBase*)session_manager->buffer;

    if (session != NULL) {
        return session->getUserFlagB();
    }
    return FALSE;
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkLayer - the layer base class (table 0x805FB5D0)                                     */
/* ----------------------------------------------------------------------------------------- */

NetworkLayer::NetworkLayer()
{
    s32 i;
    NetworkLayerRequest* req;

    this->context_04 = 0;
    this->context_08 = 0;
    for (i = 0; i < 21; i++) {
        this->requests_0C[i] = 0;
        this->requestState_60[i] = 0;
    }
    this->flag_75 = 1;
    this->flag_76 = 1;
    req = this->pool_78;
    for (i = 0; i < 2; i++) {
        req->reset();
        req++;
    }
}

/* Returns a request record to its idle state. */
void NetworkLayerRequest::reset()
{
    this->state_00 = 0;
    this->interval_4C = 0.0f;
    this->timeout_50 = 0.0f;
    this->requestId_70 = 0;
    this->unused_24 = 0;
    this->cancelled_74 = 0;
    this->owner_94 = 0;
    this->handler_98 = 0;
    this->count_28 = 0;
    this->record_54 = 0;
    this->record_58 = 0;
    this->record_5C = 0;
    this->unused_60 = 0;
    this->unused_64 = 0;
    this->unused_68 = 0;
    this->unused_6C = 0;
    this->unused_04 = 0;
    this->unused_08 = 0;
    this->unused_0C = 0;
    this->unused_10 = 0;
    this->unused_14 = 0;
    this->unused_18 = 0;
    this->unused_1C = 0;
    this->unused_20 = 0;
    this->args_2C[0] = 0;
    this->args_2C[1] = 0;
    this->args_2C[2] = 0;
    this->args_2C[3] = 0;
    this->args_2C[4] = 0;
    this->args_2C[5] = 0;
    this->args_2C[6] = 0;
    this->args_2C[7] = 0;
}

NetworkLayerRequest::~NetworkLayerRequest()
{
    clear();
    dtor_803CA338(this->mutex_78, -1);
}

void NetworkLayerRequest::clear()
{
    reset();
}

NetworkLayerRequest::NetworkLayerRequest()
{
    networkInstance_initMutex(this->mutex_78);
    reset();
}

NetworkLayer::~NetworkLayer()
{
    NetworkLayer::release();
}

void NetworkLayer::init(u32 context0, u32 context1)
{
    this->context_04 = context0;
    this->context_08 = context1;
    NetworkLayer::clear();
}

void NetworkLayer::clear()
{
    s32 i;

    for (i = 0; i < 21; i++) {
        this->requests_0C[i] = 0;
        this->requestState_60[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        this->pool_78[i].reset();
    }
}

void NetworkLayer::release()
{
    s32 i;

    for (i = 0; i < 21; i++) {
        deleteRequest(&this->requests_0C[i]);
    }
    for (i = 0; i < 2; i++) {
        this->pool_78[i].clear();
    }
}

/* One tick of the request state machine: starts the requests allowed to move this tick and retires the
 * ones whose owner has let go. */
void NetworkLayer::move()
{
    NetworkLayerRequest* req;
    s32 i;

    for (i = 0; i < 21; i++) {
        req = this->requests_0C[i];
        if (req != 0 && this->requestState_60[i] == 0) {
            if (i != 2 && this->requests_0C[2] != 0) {
                continue;
            }
            switch (i) {
            case 1:
                if (this->requestState_60[2] != 0) {
                    continue;
                }
                break;
            case 2:
                {
                    s32 j;
                    for (j = 0; j < 21; j++) {
                        if (this->requestState_60[j] != 0) {
                            break;
                        }
                    }
                    if (j == 21) {
                        break;
                    }
                    getNetworkLogger()->log_14("NetworkLayer::move: request[%d] is moving, stand by...\n", j);
                    continue;
                }
            default:
                break;
            }
            this->requestState_60[i] = 1;
        }
        if (req != 0 && this->requestState_60[i] != 0) {
            req->run();
            if (req->isOwned() == 0) {
                deleteRequest(&this->requests_0C[i]);
                this->requestState_60[i] = 0;
            }
        }
    }
}

#pragma dont_inline on
s32 NetworkLayerRequest::isOwned()
{
    return this->owner_94 != 0;
}
#pragma dont_inline off

/* Runs the request's handler through its member-function pointer; a handler that reports completion
 * resets the record. */
void NetworkLayerRequest::run()
{
    if (this->owner_94 != 0 && (this->owner_94->*this->handler_98)(this) != 0) {
        clear();
    }
}

/* The request descriptors the starters pass by value: `{0, handler slot, 0}`, a member-function pointer to the
 * pure handler slot of the same request, in the order the target lays them out. */
NetworkLayerHandler networkLayerRequestDesc1C = &NetworkLayer::handle_F4;
NetworkLayerHandler networkLayerRequestDesc20 = &NetworkLayer::handle_F8;
NetworkLayerHandler networkLayerRequestDesc24 = &NetworkLayer::handle_FC;
NetworkLayerHandler networkLayerRequestDesc28 = &NetworkLayer::handle_100;
NetworkLayerHandler networkLayerRequestDesc38 = &NetworkLayer::handle_104;
NetworkLayerHandler networkLayerRequestDesc3C = &NetworkLayer::handle_108;
NetworkLayerHandler networkLayerRequestDesc40 = &NetworkLayer::handle_108;
NetworkLayerHandler networkLayerRequestDesc44 = &NetworkLayer::handle_10C;
NetworkLayerHandler networkLayerRequestDesc48 = &NetworkLayer::handle_110;
NetworkLayerHandler networkLayerRequestDesc4C = &NetworkLayer::handle_114;
NetworkLayerHandler networkLayerRequestDesc50 = &NetworkLayer::handle_118;
NetworkLayerHandler networkLayerRequestDesc54 = &NetworkLayer::handle_118;
NetworkLayerHandler networkLayerRequestDesc58 = &NetworkLayer::handle_11C;
NetworkLayerHandler networkLayerRequestDesc64 = &NetworkLayer::handle_120;
NetworkLayerHandler networkLayerRequestDesc68 = &NetworkLayer::handle_124;
NetworkLayerHandler networkLayerRequestDesc6C = &NetworkLayer::handle_128;
NetworkLayerHandler networkLayerRequestDesc70 = &NetworkLayer::handle_12C;
NetworkLayerHandler networkLayerRequestDesc74 = &NetworkLayer::handle_130;
NetworkLayerHandler networkLayerRequestDesc78 = &NetworkLayer::handle_134;
NetworkLayerHandler networkLayerRequestDesc7C = &NetworkLayer::handle_138;
NetworkLayerHandler networkLayerRequestDesc80 = &NetworkLayer::handle_13C;
NetworkLayerHandler networkLayerRequestDesc84 = &NetworkLayer::handle_140;

void NetworkLayer::request_1C()
{
    NetworkLayerRequest* req;

    if (this->requests_0C[1] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[1] = req;
            req->begin(this, networkLayerRequestDesc1C, 0);
        }
    }
}

/* Starts the request for `owner`: resets it, stamps the start time and a fresh id, takes the handler and
 * copies up to eight word arguments. */
void NetworkLayerRequest::begin(NetworkLayer* owner, NetworkLayerHandler handler, u32 count, ...)
{
    NetworkVaState args;
    u32 i;

    reset();
    this->timeout_50 = getNetworkLogger()->getTime_60();
    this->requestId_70 = NetworkRequest_idCounter;
    NetworkRequest_idCounter = this->requestId_70 + 1;
    this->owner_94 = owner;
    this->handler_98 = handler;
    this->count_28 = count > 8 ? 8 : count;
    __builtin_va_info(&args);
    for (i = 0; i < this->count_28; i++) {
        this->args_2C[i] = *(u32*)__va_arg(&args, 1);
    }
}

void NetworkLayer::request_20()
{
    NetworkLayerRequest* req;

    if (this->requests_0C[2] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[2] = req;
            req->begin(this, networkLayerRequestDesc20, 0);
        }
    }
}

void NetworkLayer::request_24(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[3] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[3] = req;
            req->begin(this, networkLayerRequestDesc24, 1, a);
        }
    }
}

void NetworkLayer::request_28(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[4] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[4] = req;
            req->begin(this, networkLayerRequestDesc28, 1, a);
        }
    }
}

void NetworkLayer::request_38()
{
    NetworkLayerRequest* req;

    if (this->requests_0C[5] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[5] = req;
            req->begin(this, networkLayerRequestDesc38, 0);
        }
    }
}

void NetworkLayer::request_3C(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[6] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[6] = req;
            req->begin(this, networkLayerRequestDesc3C, 2, 0, a);
        }
    }
}

void NetworkLayer::request_40(u32 a, u32 b, u32 c)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[6] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[6] = req;
            req->begin(this, networkLayerRequestDesc40, 4, 1, a, b, c);
        }
    }
}

void NetworkLayer::request_44(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[7] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[7] = req;
            req->begin(this, networkLayerRequestDesc44, 1, a);
        }
    }
}

void NetworkLayer::request_48(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[8] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[8] = req;
            req->begin(this, networkLayerRequestDesc48, 1, a);
        }
    }
}

void NetworkLayer::request_4C(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[9] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[9] = req;
            req->begin(this, networkLayerRequestDesc4C, 1, a);
        }
    }
}

void NetworkLayer::request_50(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[10] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[10] = req;
            req->begin(this, networkLayerRequestDesc50, 2, 0, a);
        }
    }
}

void NetworkLayer::request_54(u32 a, u32 b)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[10] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[10] = req;
            req->begin(this, networkLayerRequestDesc54, 3, 1, a, b);
        }
    }
}

void NetworkLayer::request_58(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[11] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[11] = req;
            req->begin(this, networkLayerRequestDesc58, 1, a);
        }
    }
}

void NetworkLayer::request_64(u32 a, u32 b, u32 c, u32 d)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[12] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[12] = req;
            req->begin(this, networkLayerRequestDesc64, 4, a, b, c, d);
        }
    }
}

void NetworkLayer::request_68(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[13] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[13] = req;
            req->begin(this, networkLayerRequestDesc68, 1, a);
        }
    }
}

void NetworkLayer::request_6C(u32 a, u32 b)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[14] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[14] = req;
            req->begin(this, networkLayerRequestDesc6C, 2, a, b);
        }
    }
}

void NetworkLayer::request_70(u32 a, u32 b)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[15] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[15] = req;
            req->begin(this, networkLayerRequestDesc70, 2, a, b);
        }
    }
}

void NetworkLayer::request_74(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[16] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[16] = req;
            req->begin(this, networkLayerRequestDesc74, 1, a);
        }
    }
}

void NetworkLayer::request_78(u32 a, u32 b)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[17] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[17] = req;
            req->begin(this, networkLayerRequestDesc78, 2, a, b);
        }
    }
}

void NetworkLayer::request_7C(u32 a, u32 b)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[18] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[18] = req;
            req->begin(this, networkLayerRequestDesc7C, 2, a, b);
        }
    }
}

void NetworkLayer::request_80(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[19] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[19] = req;
            req->begin(this, networkLayerRequestDesc80, 1, a);
        }
    }
}

void NetworkLayer::request_84(u32 a)
{
    NetworkLayerRequest* req;

    if (this->requests_0C[20] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[20] = req;
            req->begin(this, networkLayerRequestDesc84, 1, a);
        }
    }
}

/* Hands out a free request of the pool, 0 when both are owned. */
NetworkLayerRequest* NetworkLayer::allocRequest()
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (this->pool_78[i].isOwned() == 0) {
            return &this->pool_78[i];
        }
    }
    return 0;
}

/* Releases the request in `slot` (warning when it is still running) and empties the slot. */
void NetworkLayer::deleteRequest(NetworkLayerRequest** slot)
{
    if (*slot != 0) {
        if ((*slot)->isOwned() != 0) {
            getNetworkLogger()->log_14("NetworkLayer::deleteRequest: request is moving.\n");
        }
        (*slot)->clear();
    }
    *slot = 0;
}

void NetworkLayer::setFlag75(u8 value)
{
    this->flag_75 = value;
}

u8 NetworkLayer::getFlag75()
{
    return this->flag_75;
}

void NetworkLayer::setFlag76(u8 value)
{
    this->flag_76 = value;
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkLayerPat (from its constructor 0x803E0C18)                                          */
/* ----------------------------------------------------------------------------------------- */

/* Copies the request's error record out under its mutex; false while none is set. */
s32 NetworkLayerRequest::getRecord(NetworkRequestError* out)
{
    s32 result;

    result = 0;
    LockMutex(this->mutex_78);
    if (this->record_54 != 0) {
        result = 1;
        out->code_00 = this->record_54;
        out->arg_04 = this->record_58;
        out->arg_08 = this->record_5C;
    }
    UnlockMutex(this->mutex_78);
    return result;
}
