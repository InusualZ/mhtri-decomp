/*
 * fn_8041A87C.cpp - the GameSpy interface / peer band, `.text` 0x8041A87C..0x8041DF10.
 *
 * SHAPE.  The band's four object types - `NetworkGameSpyInterface`, the worker thread
 * `GameSpyInterfaceThread`, `NetworkPeerGameSpy` and `NetworkTimedHandler` - own their bodies as
 * member functions, and every call the target takes through an object's vtable goes through a
 * declared `virtual` on the *foreign* class being dispatched (`NetworkLogger`, `GameSpyReceiver`,
 * `NetworkPeerCallback`): only a real virtual emits retail's `lwz r12, 0x0(r3)` / `lwz r12, <slot>(r12)`
 * shape, while a struct of function pointers loads through a scratch register.  The unit's *own*
 * vtables stay referenced (`lbl_806036A0`, `lbl_80603740`), never declared: their entries are all
 * defined here, so a `virtual` spelling makes MWCC emit a second copy of the table in `.data`
 * (measured: a class whose virtuals are all defined in its TU gains a 20-byte `.data` vtable).
 *
 * Naming note: `dumpmap.py lookup` answers only `zz_XXXXXXXX_` for this range's code, so the
 * `fn_` stems of the unsplit callees this file still calls stay (they are the map's placeholders).
 * The band's own names come from the code and the pool: the map carries each member's mangled
 * spelling, and the seven the pool spells out are `NetworkGameSpyInterface::startMatch`,
 * `::ConnectToAnybody`, `::executeError`, `::sendUnreliable`, `::tGameSpyInterface`,
 * `::GameSpyInterfaceThreadInit` and `NetworkPeerGameSpy::put`.  No `unkNN` identifier survives.
 *
 * WHAT IT IS.  The Wii network layer's GameSpy half: the `NetworkGameSpyInterface` connect and NAT
 * sub-machines (the DWC callback codes 0x8000..0x8082, the 0x6001..0x6005 work records), the
 * interface's worker thread `GameSpyInterfaceThread` (init/open/step request bytes, a three-slot
 * socket table at 0x806D3650, the NAT-negotiation sub-machine and its error record), and
 * `NetworkPeerGameSpy`'s 0x600-byte send / 0x6000-byte receive buffers.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string covers the range: the
 * `.data` pool it loads (0x80603000..0x806036xx) is class format strings ("connectAttemptCallback is
 * called.", "NetworkGameSpyInterface::startMatch ...", "NetworkPeerGameSpy::put: buf_recv_peer over
 * ..."), never a source-file name.  2. `dumpmap.py lookup` answers only `zz_XXXXXXXX_` for the code.
 *  3. The classes plus the registered neighbour `Network/NetworkWiiMediator.c` place the band in
 * `Network`, and the `.data` vtables it stores (`lbl_806036A0`, `lbl_80603740`) name no source file,
 * so the file keeps the map's stem (brief option 4).  The seam is unproven (discovery cap at
 * Camellia's start).  Open naming question: the *thread* class's own log strings read
 * `NetworkGameSpyInterface::<method>`, and the header gives that name to the connect sub-machine's
 * object instead - recorded here rather than renamed, since the sub-machine's real name is unproven.
 *
 * LANGUAGE AND SECTIONS.  C++ (mangled `__dl__FPv` delete, vtables).  `-Cpp_exceptions on` now comes
 * from `cflags_network` (flags-audit 2026-09-28) while retail's object carries `extab` 380 B and
 * `extabindex` 540 B (playbook 30's pragma pair).  Nothing in the range's `.data` is claimed: the
 * 0x80603xxx string pool is shared with the NetworkWiiMediator band above it (that band loads the
 * same literals), so per playbook 58 it can be neither claimed nor named here.  Our object keeps the
 * empty `ours-extra` set: the view classes are never constructed, so no vtable is emitted.
 *
 * DATA CLAIMED (2026-09-28).  `.sbss` 0x80794CE0..0x80794CE8 - the pair the worker thread owns:
 * 0x80794CE0 (stored four times here, read ten) and 0x80794CE4 `sGameSpyInterfaceThread`, the
 * singleton this unit publishes at `.text`+0x1E14 and its destructor zeroes at +0x1EE0 (both
 * `stw rX, 0(0)` + `R_PPC_EMB_SDA21`).  The band header `include/unsplit/Network.h` declared both,
 * which is rule 12.  The *other* unit that names the singleton, `Network/fn_803D3CE8.cpp`, only
 * loads it (`GameSpyInterfaceThread_getInstance`), so the definer is this unit and the claim is
 * here.  One contiguous run, symbol- and 8-byte-aligned, so the split needs no interior auto band.
 *
 * FLAGS.  The object deviates from the lib on three points, all in `configure.py` or in this file
 * with the evidence: `-O3` + `-inline noauto` in place of `-O4,p` + `-inline auto` (retail calls the
 * file-static helpers - the peer's `isQueued` is 72 B against our 64 B when the inliner folds them
 * in), the lib's `-Cpp_exceptions on`, and a file-wide `#pragma peephole off` (retail keeps the
 * *unfused* folds across the band - `clrlwi`+`slwi` in place of `clrlslwi`, `extsh`+`cmpwi` in place
 * of a folded compare, `extsb`+`cmpwi` in place of `extsb.`), with the five functions whose retail
 * bodies DO carry the folded forms bracketed back on.
 *
 * SHAPES THAT MATTER (measured, easy to undo by accident).  The runtime entry points the band needs
 * are the owners' headers (`Runtime.PPCEABI.H/memcpy.h`, `memset.h`, `unsplit/Runtime.PPCEABI.H.h`),
 * never local declarations (rule 2; the swap is score-neutral, but playbook 60 says measure it).
 * `registerReceiver` returns `s32`, not the u8 the call sites cast - retail's early returns are
 * `li r3, -1` / `-2` and the tail masks the slot it returns (`bind` then stores with a plain `stw`).
 * `sessionOpen_4484` is `s8` (retail `lbz`+`extsb` at every test), `bufferSize_4482` is read unsigned
 * (`(u16)` at its one use), and `profile_4485` is five `u32` at the *odd* offset 0x4485, which needs
 * the class's `#pragma pack(1)` (retail stores them `stw r0, 0x4485(r31)`, `0x4489`, ...).
 * `NetworkLogger::flag_48` returns `u16` (`include/unsplit/Network.h`): retail stores the result with
 * a raw `sth` and masks it only where it is widened.  An extern whose size is unknown is addressed
 * absolutely - `lbl_80793990[3]`/`lbl_80793998[4]` give retail's `li r5, sym@sda21` where `char[]`
 * gave `lis`/`addi`; `natNegMessageMagic` - the shared NATNEG signature, declared in the unit that
 * owns the bytes (`include/DWCi/DWCi_NatNeg.h`) - stays unsized because the target relocates it
 * ADDR16_HA/LO.  The
 * error record a caller builds is three constants stored twice (retail's five 12-byte frame objects
 * at 0x08..0x43): the by-value dispatch is emulated with a per-site `u32 info[6]` whose `[3],[4],[5]`
 * half is written first (`fn_8041B720` 74.25 -> 99.89, `fn_8041B538` 93.77 -> 99.92, `applyEvent`'s
 * two sites).  A `switch` whose cases all leave 0 is written `break` + one trailing `return 0;`, and
 * `default: break;` (playbook 34): `runSearch`/`runConnect`/`runNasLogin` only match that way.  The
 * clamp in `runSearch` is the ternary `size = size >= 0x2000 ? 0x2000 : size;` (retail plants the
 * constant first and copies in the other arm).  `ConnectToAnybody(s32 arg)` takes the thread argument
 * even though its body ignores it - retail's caller sets r4 - so the map row is `...Fl`.
 *
 * RESIDUALS.  Unit 98.48 % fuzzy, 53/71 functions byte-identical, `.text` 13968 B against 13972 B,
 * mean 99.09 %, every function >= 80 %.  isQueued 83.89: retail keeps the *unfused* `extsb`+`cmpwi`
 * and branches to the clamp; with the peephole on ours fuses to `extsb.` and inverts, with it off it
 * is unfused but still inverts and puts the slot in r4 - both measure below the fused shape, so it
 * stays (the shapesearch candidate that scores higher negates the guard: a different shape that
 * measures 84.44).  unregisterReceiver 91.375 keeps its `count` local cached in r30 across the
 * `replyRequest` call; reading `receiverCount_28` at the loop - retail's `lbz r4, 0x28(r28)` *after*
 * the call - has the right instruction stream but rotates every callee-saved register (90.93), and
 * assigning the local after the call measures 85.93.  startMatch 93.63, send 95.68, receive 94.99,
 * publishRequest 96.83, executeError 97.88, updateCallbackStep 96.22 (five `clrlwi.`/`rlwinm.` pairs
 * retail keeps unfused; a scoped peephole pragma pair inside the function does nothing and switching
 * the function to `peephole off` measures 95.68), startNegotiation 98.23 (the third guard's branch
 * polarity - retail `bne return`, ours `beq body` + `b return`), tGameSpyInterface 98.48,
 * checkPeerProfile 98.38, fn_8041B334 98.52, applyEvent 94.42 (the record pairs land as two 24-byte
 * arrays where retail packs five 12-byte objects, and the `ticks * 17` 64-bit multiply materialises
 * a fresh `li r0, 0x0` where retail reuses its zero) and the two `fn_8041B5xx` callbacks are
 * register colourings of statements that measure byte-identical on their own.
 *
 * POSTERROR.  The four `postError` sites dispatch through `NetworkInstanceDispatch`
 * (`include/unsplit/Network.h`), the singleton read as the polymorphic class it is: declaring its
 * 161 slots makes the call retail's `lwz r12, 0x0(r3)` / `lwz r12, 0x288(r12)`, where the data-slot
 * view this file used to carry loaded the table into a scratch register (`lwz r5, 0x0(r3)`).  With
 * it `fn_8041B538` and `fn_8041B720` are 100 % (was 99.92/99.89) and `applyEvent` gains 0.1; the
 * sibling `Network/network_state.cpp` keeps its own view because its record is passed **by value**
 * there, which is a different call shape (measured: the pointer form costs that row 2.4 points).
 *
 * `runThread` and the thread body it calls are emitted in the reverse of the target's
 * address order (the map has `runThread` at 0x8041D31C and the body at 0x8041D344): a source-order
 * defect that costs no row.  Data: `extabindex` 540/540 exact, `.rela.text` 5616/5616, `extab` 360
 * against 380 and `.relaextab` 12 B short - the target's last extab entry is the 20-byte cleanup
 * record whose reloc points at `dtor_803CA338`, a local object with a destructor the range's own code
 * does not show.
 */

#include "types.h"
#include "Network/fn_8041A87C.h"
#include "Runtime.PPCEABI.H/memcpy.h"    /* memcpy  - owner Runtime.PPCEABI.H/memcpy.c */
#include "Runtime.PPCEABI.H/memset.h"    /* memset  - owner Runtime.PPCEABI.H/memset.c */
#include "unsplit/Runtime.PPCEABI.H.h"   /* memmove / memcmp / snprintf - no registered owner */

/* retail keeps the *unfused* peephole forms across this band: `clrlwi`+`slwi` in place of
 * `clrlslwi`, `extsh`+`cmpwi` in place of a folded compare, `extsb`+`cmpwi` in place of
 * `extsb.`.  The five functions below re-enable the pass - their retail bodies DO carry the
 * folded forms - and each is bracketed rather than left open (playbook 32). */
#pragma peephole off

/* The target object carries `extab`/`extabindex` (380/540 B) while the `Network` lib is built with
 * exceptions off, so the front-end is told per file (the pragma pair of playbook 30). */

/* The debug manager's virtual slots: the target re-runs `bl getNetworkLogger` at *every* logging site
 * (never once per function), so each site expands to its own block that fetches the singleton and
 * dispatches through its vtable - a real virtual call, which is the only shape MWCC emits as
 * `lwz r12, 0x0(r3)` / `lwz r12, 0xC(r12)`. */
#define SIGNAL_LOG(...) do { NetworkLogger* lm = getNetworkLogger(); lm->signal_0C(__VA_ARGS__); } while (0)
#define WARN_LOG(...)   do { NetworkLogger* lm = getNetworkLogger(); lm->warn_10(__VA_ARGS__); } while (0)
#define INFO_LOG(...)   do { NetworkLogger* lm = getNetworkLogger(); lm->log_14(__VA_ARGS__); } while (0)

extern "C" {

/* ---- this unit's free (callback and entry-point) bodies, in address order --------------------- -
 * Every other body in the range is a member of one of the four classes, so those are declared in
 * include/Network/fn_8041A87C.h and not here.  The DWC callbacks are installed through
 * `(NetworkCallback)`, so they keep the C spelling and the flat parameter lists retail shows. */
void fn_8041B194(void);
void fn_8041B26C(void);
s32 fn_8041B270(NetworkInstance* self, u32 peer, u16 value, const void* data, u32 size);
void fn_8041B334(s32 result, s32 unused, const GameSpyAddress* src, GameSpyResultInfo* info);
void fn_8041B514(GameSpyAddress* out, const GameSpyAddress* in);
void fn_8041B538(s32 unused0, s32 socket, s32 unused1, s32 unused2, s32 unused3, const void* profile,
                 u32 size);
void fn_8041B720(s32 socket, s32 result, s32 unused, s32 timeout);
void fn_8041B894(u32 socket, s32 address, s32 size);
void fn_8041B984(u32 socket, s32 result);
void fn_8041BAB4(void);
s32 runThread(void* self);
void fn_8041DD24(void);
s32 fn_8041DD28(NetworkPeerCallback* self);

}

/* ------------------------------------------------------------------------------------------------ */

/* Runs the connect-attempt callback sub-machine, one step per frame. */
#pragma peephole on
void NetworkGameSpyInterface::updateCallbackStep()
{
    u8 step;

    if (callbackStep_17 != 0 && getInstance_() == NULL) {
        WARN_LOG(lbl_80603154);
        callbackStep_17 = 0;
    }
    step = callbackStep_17;
    switch (step) {
    case 1:
        if (connectStep_15 == 0) {
            callbackStep_17 = 6;
            break;
        }
        if (isCallback(getInstance_(), 7) == 0) {
            callbackStep_17 = 5;
            break;
        }
        if (fn_803FD658(getInstance_()) != 0) {
            callbackStep_17 = 4;
            break;
        }
        if ((u8)getSomething5(getInstance_()) == 0) {
            callbackStep_17 = 4;
            break;
        }
        flags_0C = 0;
        sendReqShut(getInstance_(), 1);
        callbackStep_17 = (u8)(callbackStep_17 + 1);
        break;
    case 2:
        if ((flags_0C & 1) != 0 || (flags_0C & 2) != 0 || (flags_0C & 0x10) != 0) {
            flags_0C = 0;
            resetNetworkState3(getInstance_());
            callbackStep_17 = (u8)(callbackStep_17 + 1);
        }
        break;
    case 3:
        if ((flags_0C & 8) != 0) {
            callbackStep_17 = (u8)(step + 1);
        }
        break;
    case 4:
        resetCallback(getInstance_(), 7);
        callbackStep_17 = (u8)(callbackStep_17 + 1);
        break;
    case 5:
        decrement60d4(getInstance_());
        callbackStep_17 = (u8)(callbackStep_17 + 1);
        break;
    case 6: {
        NetworkLogger* lm = getNetworkLogger();
        if (lm->isVerbose_3C() > 0) {
            callbackStep_17 = (u8)(callbackStep_17 + 1);
        }
        break;
    }
    case 7:
        callbackStep_17 = 0;
        connectStep_15 = 0;
        fn_8041A5A8(this, 0x6003, 0, 0, 0, NULL);
        break;
    default:
        break;
    }
}
#pragma peephole off

/* Dispatches to the search sub-machine (task 1) or the connect sub-machine (task 2). */
s32 NetworkGameSpyInterface::dispatchTask()
{
    switch (task_10) {
    case 1:
        return runSearch();
    case 2:
        return runConnect();
    default:
        return 0;
    }
}

/* Runs the GameSpy connect sub-machine, one step per frame. */
s32 NetworkGameSpyInterface::runSearch()
{
    u32 pending;
    u32 offset;
    u32 size;
    u32 info[3];

    switch (searchStep_14) {
    case 0:
        flags_0C = 0;
        fn_80403F60(getInstance_(), channel_18);
        searchStep_14 = 5;
        break;
    case 5:
        pending = flags_0C;
        if ((pending & 1) != 0) {
            searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            searchStep_14 = 0x64;
        } else if ((pending & 0x80) != 0) {
            writePos_8168 = 0;
            searchStep_14 = 0x0A;
        }
        break;
    case 0x0A:
        flags_0C = 0;
        offset = writePos_8168;
        size = limit_8164 - offset;
        size = size >= 0x2000 ? 0x2000 : size;
        fn_80403FE4(getInstance_(), channel_18, offset, size);
        searchStep_14 = 0x0F;
        break;
    case 0x0F:
        pending = flags_0C;
        if ((pending & 1) != 0) {
            searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            searchStep_14 = 0x64;
        } else if ((pending & 0x100) != 0) {
            if (limit_8164 - writePos_8168 != 0) {
                searchStep_14 = 0x0A;
            } else {
                searchStep_14 = 0x14;
            }
        }
        break;
    case 0x14:
        fn_8041A5A8(this, 0x6004, 0, 0, 1, &channel_18);
        return 1;
    case 0x64:
        info[0] = 0x80000007;
        info[1] = 0;
        info[2] = (u32)fn_803FD694(getInstance_(), 0);
        fn_8041A5A8(this, 0x6004, 0, (s32)info[0], 1, info);
        return 1;
    case 0x6E:
        fn_803FD794(getInstance_(), info);
        fn_8041A5A8(this, 0x6004, 0, (s32)info[0], 1, info);
        fn_8041A5A8(this, 0x6001, 0, (s32)info[0], 1, info);
        return 1;
    default:
        break;
    }
    return 0;
}

/* Runs the GameSpy NAT/connect sub-machine, one step per frame. */
s32 NetworkGameSpyInterface::runConnect()
{
    u32 pending;
    u32 info[3];

    switch (searchStep_14) {
    case 0:
        flags_0C = 0;
        fn_80404070(getInstance_());
        searchStep_14 = 5;
        break;
    case 5:
        pending = flags_0C;
        if ((pending & 1) != 0) {
            searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            searchStep_14 = 0x64;
        } else if ((pending & 0x200) != 0) {
            searchStep_14 = 0x0A;
        }
        break;
    case 0x0A:
        fn_8041A5A8(this, 0x6005, 0, 0, 0, NULL);
        return 1;
    case 0x64:
        info[0] = 0x80000007;
        info[1] = 0;
        info[2] = (u32)fn_803FD694(getInstance_(), 0);
        fn_8041A5A8(this, 0x6005, 0, (s32)info[0], 1, info);
        return 1;
    case 0x6E:
        fn_803FD794(getInstance_(), info);
        fn_8041A5A8(this, 0x6005, 0, (s32)info[0], 1, info);
        fn_8041A5A8(this, 0x6001, 0, (s32)info[0], 1, info);
        return 1;
    default:
        break;
    }
    return 0;
}

/* Applies a DWC event to the interface, then folds the event bits into the state machine's flags. */
#pragma peephole on
void NetworkGameSpyInterface::applyEvent(u32 code, s32 a, void* b, const GameSpyEventMsg* msg)
{
    u32 limit;
    GameSpyChannel* dst;
    u32 size;
    const GameSpyChannel* src;
    s32 i;

    switch (code) {
    case 0x8000:
    case 0x8007: {
        u32 info[3];

        if (task_10 <= 0) {
            fn_803FD794(getInstance_(), info);
            fn_8041A5A8(this, 0x6001, 0, (s32)info[0], 1, info);
        }
        flags_0C |= 1;
        break;
    }
    case 0x8002:
        flags_0C |= 2;
        break;
    case 0x8004:
        if (a != 0) {
            flags_0C |= 1;
        }
        break;
    case 0x8005:
        flags_0C |= 8;
        break;
    case 0x8006:
        flags_0C |= 1;
        break;
    case 0x8080:
        if (msg == NULL) {
            break;
        }
        if (msg->channelView.channel_00 != channel_18) {
            u32 info[6];
            NetworkInstance* inst;

            info[3] = 0x80000000;
            info[4] = 0;
            info[5] = 0;
            info[0] = 0x80000000;
            info[1] = 0;
            info[2] = 0;
            inst = getInstance_();
            ((NetworkInstanceDispatch*)inst)->postError((NetworkErrorInfo*)info);
            break;
        }
        peerId_1C = msg->channelView.peerId_04;
        channelCount_8020 = msg->channelView.count_0C;
        if (channelCount_8020 > 8) {
            channelCount_8020 = 8;
        }
        limit = msg->channelView.limit_08;
        limit_8164 = limit;
        if (limit_8164 > 0x8000) {
            limit_8164 = 0x7FFF;
        }
        src = msg->channelView.channels_10;
        dst = channels_8024;
        for (i = 0; i < channelCount_8020; i++) {
            dst->ownerId_00 = src->ownerId_00;
            dst->peerId_04 = src->peerId_04;
            memcpy(dst->address_08, src->address_08, 0x1F);
            dst->tail_27 = 0;
            src++;
            dst++;
        }
        flags_0C |= 0x80;
        break;
    case 0x8081:
        if (msg == NULL) {
            break;
        }
        if (msg->dataView.channel_00 != channel_18 ||
            msg->dataView.writePos_04 != writePos_8168) {
            u32 info[6];
            NetworkInstance* inst;

            info[3] = 0x80000000;
            info[4] = 0;
            info[5] = 0;
            info[0] = 0x80000000;
            info[1] = 0;
            info[2] = 0;
            inst = getInstance_();
            ((NetworkInstanceDispatch*)inst)->postError((NetworkErrorInfo*)info);
            break;
        }
        size = msg->dataView.size_08;
        limit = limit_8164 - writePos_8168;
        if (size > limit) {
            size = limit;
        }
        memcpy(recvArea_20 + writePos_8168, msg->dataView.data_0C, size);
        writePos_8168 += size;
        flags_0C |= 0x100;
        break;
    case 0x8082:
        flags_0C |= 0x200;
        break;
    default:
        break;
    }
}
#pragma peephole off

/* Drops every socket of the three-slot table, one request record at a time. */
extern "C" void fn_8041B194(void)
{
    s32 i;

    SIGNAL_LOG(3, lbl_806031B0);
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    for (i = 0; i < 3; i++) {
        if (lbl_806D3650[i] != 0) {
            ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->failRequest(-0x2DB0, 1, 0, (u8)i, -1);
            lbl_806D3650[i] = 0;
        }
    }
}

/* Empty body: the socket table's accept callback placeholder, installed but never used. */
extern "C" void fn_8041B26C(void)
{
}

/* Sends a framed GameSpy header through the DWC socket layer. */
extern "C" s32 fn_8041B270(NetworkInstance* self, u32 peer, u16 value, const void* data, u32 size)
{
    GameSpyHeader header;

    if (size == 0 || data == NULL) {
        return 0;
    }
    memset(&header, 0, sizeof(header));
    header.code_1 = 2;
    header.value_4 = peer;
    header.peer_2 = SOHtoNs(value);
    if (memcmp(data, natNegMessageMagic, 6) == 0) {
        DWCi_NatNegSendPacket((void*)data, size, &header);
        return 1;
    }
    return 0;
}

/* Maps a GameSpy connect result onto the interface's request state and error record. */
#pragma peephole on
extern "C" void fn_8041B334(s32 result, s32 unused, const GameSpyAddress* src, GameSpyResultInfo* info)
{
    s32 error = 0;

    info->result_04 = 1;
    info->connected_00 = 0;
    switch (result) {
    case 0:
        fn_8041B514(&info->address_08, src);
        info->connected_00 = 1;
        SIGNAL_LOG(3, lbl_806031E8);
        break;
    case 1:
        SIGNAL_LOG(3, lbl_80603204);
        error = -0x2DA6;
        break;
    case 2:
        SIGNAL_LOG(3, lbl_80603218);
        error = -0x2DA7;
        break;
    case 3:
        SIGNAL_LOG(3, lbl_80603228);
        error = -0x2DA8;
        break;
    case 4:
        SIGNAL_LOG(3, lbl_80603238);
        error = -0x2DA9;
        break;
    default:
        SIGNAL_LOG(3, lbl_80603248);
        error = -0x2DA9;
        break;
    }
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    if (info->connected_00 == 0) {
        ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->setError(0x80000007, 0x5F, -error);
    }
}
#pragma peephole off

/* Copies the 8-byte GameSpy header out of a received frame. */
extern "C" void fn_8041B514(GameSpyAddress* out, const GameSpyAddress* in)
{
    out->first_00 = in->first_00;
    out->second_01 = in->second_01;
    out->port_02 = in->port_02;
    out->value_04 = in->value_04;
}

/* Handles a connect-attempt callback: validates the reply and publishes the socket. */
extern "C" void fn_8041B538(s32 unused0, s32 socket, s32 unused1, s32 unused2, s32 unused3,
                            const void* profile, u32 size)
{
    u32 peerId;
    s32 i;
    u32 info[6];

    SIGNAL_LOG(3, lbl_80603254);
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    peerId = ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->getPeerId();
    if (((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->checkPeerProfile(profile, size) == 0) {
        fn_8050DF90(socket, lbl_80793990, 2);
        ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->publishRequest(-0x2DA0, 0xFF, peerId);
        return;
    }
    if (fn_8050DF80(socket, lbl_806031A0) != 0) {
        SIGNAL_LOG(3, lbl_80603278);
        for (i = 0; i < 3; i++) {
            if (lbl_806D3650[i] == 0) {
                lbl_806D3650[i] = socket;
                ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->publishRequest(0, (u8)i, peerId);
                break;
            }
        }
        if (i >= 3 && getInstance_() != NULL) {
            u32 code = 0x80000007;
            u32 type = 0x5F;
            u32 detail = 0x2D6A;
            NetworkInstance* inst;

            info[3] = code;
            info[4] = type;
            info[5] = detail;
            info[0] = code;
            info[1] = type;
            info[2] = detail;
            inst = getInstance_();
            ((NetworkInstanceDispatch*)inst)->postError((NetworkErrorInfo*)info);
        }
    } else {
        ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->publishRequest(-0x2DAE, 0xFF, peerId);
    }
}

/* Handles a socket accept callback: registers the socket or fails the request. */
extern "C" void fn_8041B720(s32 socket, s32 result, s32 unused, s32 timeout)
{
    s32 i;
    u32 info[6];
    s32 error;

    SIGNAL_LOG(3, lbl_80603298, result);
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    if (result == 0) {
        for (i = 0; i < 3; i++) {
            if (lbl_806D3650[i] == 0) {
                lbl_806D3650[i] = socket;
                ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->publishRequest(0, (u8)i, 0);
                break;
            }
        }
        if (i >= 3 && getInstance_() != NULL) {
            u32 code = 0x80000007;
            u32 type = 0x5F;
            u32 detail = 0x2D6A;

            info[3] = code;
            info[4] = type;
            info[5] = detail;
            info[0] = code;
            info[1] = type;
            info[2] = detail;
            NetworkInstance* inst = getInstance_();

            ((NetworkInstanceDispatch*)inst)->postError((NetworkErrorInfo*)info);
        }
    } else {
        error = timeout > 0 ? -0x2DA0 : -0x2DAD;
        ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->publishRequest(error, 0xFF, 0);
    }
}

/* Forwards receive data to the socket's receiver slot. */
extern "C" void fn_8041B894(u32 socket, s32 address, s32 size)
{
    s32 i;

    if (size <= 0) {
        SIGNAL_LOG(3, lbl_806032C0);
        return;
    }
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    for (i = 0; i < 3; i++) {
        if (socket == lbl_806D3650[i]) {
            ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->dispatchReceiver((u8)i, address, size);
            return;
        }
    }
}

/* Handles a socket close callback: unregisters the socket and fails its record. */
extern "C" void fn_8041B984(u32 socket, s32 result)
{
    s32 error;
    s32 i;

    SIGNAL_LOG(3, lbl_806032E0, result);
    if (GameSpyInterfaceThread_getInstance() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    switch (result) {
    case 2:
        error = -0x2DAF;
        break;
    case 3:
        error = -0x2DB0;
        break;
    case 4:
        error = -0x2DB1;
        break;
    default:
        error = 0;
        break;
    }
    for (i = 0; i < 3; i++) {
        if (socket == lbl_806D3650[i]) {
            ((GameSpyInterfaceThread*)GameSpyInterfaceThread_getInstance())->failRequest(error, 0, 0, (u8)i, -1);
            lbl_806D3650[i] = 0;
            return;
        }
    }
}

/* Logs a ping callback. */
extern "C" void fn_8041BAB4(void)
{
    SIGNAL_LOG(3, lbl_80603308);
}

/* Stores the id the timed handler waits for. */
void GameSpyInterfaceThread::setWaitHandle(s32 limit)
{
    stage_98 = 0;
    handle_94 = limit;
}

/* Runs the NAS-login handshake the timed handler drives, one step per frame. */
s32 GameSpyInterfaceThread::runNasLogin()
{
    s32 result;
    s32 error;

    switch (stage_98) {
    case 0:
        if (fn_8050A8A0() == 0) {
            stage_98 = 4;
            break;
        }
        stage_98 = stage_98 + 1;
        break;
    case 1:
        result = fn_8050A8D0();
        switch (result) {
        case 3:
            SIGNAL_LOG(3, lbl_80603320);
            stage_98 = stage_98 + 1;
            break;
        case 4:
            SIGNAL_LOG(3, lbl_80603334);
            stage_98 = 4;
            break;
        case 5:
            SIGNAL_LOG(3, lbl_80603348);
            setError(0x80000000, 0, 0);
            stage_98 = 4;
            break;
        default:
            break;
        }
        break;
    case 2:
        if (fn_8050A990() == 0) {
            fn_8050A9A0();
            stage_98 = 4;
        } else if (fn_8050A9B0(&lbl_80793994, handle_94) == 0) {
            fn_8050A9A0();
            stage_98 = 4;
        } else {
            stage_98 = stage_98 + 1;
        }
        break;
    case 3:
        result = fn_8050A9D0();
        switch (result) {
        case 3:
            SIGNAL_LOG(3, lbl_8060335C);
            fn_8050A9A0();
            return 1;
        case 4:
            SIGNAL_LOG(3, lbl_8060336C);
            fn_8050A9A0();
            stage_98 = 4;
            break;
        case 5:
            SIGNAL_LOG(3, lbl_80603378);
            fn_8050A9A0();
            stage_98 = 4;
            break;
        default:
            break;
        }
        break;
    case 4:
        executeError();
        return -1;
    default:
        break;
    }
    return 0;
}

/* Hands the index's receiver slot to the object stored for it (a tail call). */
void GameSpyInterfaceThread::dispatchReceiver(u8 index, s32 a, s32 b)
{
    GameSpyReceiver* receiver;

    receiver = (GameSpyReceiver*)receivers_14[receiverState_24[index]];
    if (receiver == NULL) {
        return;
    }
    receiver->handle_18(a, b, 0, 0, 0);
}

/* Fails the index's request record when `error` is set, then closes the slot. */
void GameSpyInterfaceThread::failRequest(s32 error, s32 a, s32 b, u8 index, s32 e)
{
    if (error != 0) {
        setError(0x80000007, 0x5F, -error);
    }
    slotState_54[index] = 0xFF;
}

/* Records the outcome of the DWC request the interface is waiting on. */
void GameSpyInterfaceThread::setRequestResult(s32 error, s32 value)
{
    running_6C = 2;
    if (error == 0) {
        value_30 = value;
        if (cancelPending_122 == 1) {
            state_2C = 2;
            return;
        }
        state_2C = 1;
        return;
    }
    setError(0x80000007, 0x5F, -error);
    armCancel();
    state_2C = -2;
}

/* Publishes a completed request: rewrites the slot tables and the negotiation result. */
void GameSpyInterfaceThread::publishRequest(s32 error, u8 index, u32 value)
{
    u8 i;
    u32 peer;

    if (error == 0) {
        receiverIds_34[receiverCount_28 - 1] = value_30;
        slotState_54[receiverCount_28 - 1] = 1;
        peer = value;
        if (peer == 0) {
            peer = peerId_4470;
        }
        for (i = 0; i < receiverCount_28; i++) {
            if (slotIds_44[i] == peer) {
                if (i != index) {
                    slotIds_44[i] = 0;
                    if (lbl_806D3650[i] != 0) {
                        fn_8050E250(lbl_806D3650[i]);
                    }
                }
                break;
            }
        }
        slotIds_44[index] = peer;
        slotState_54[index] = 1;
        receiverState_24[index] = 3;
        for (i = 0; i < receiverCount_28; i++) {
            if (receiverIds_34[i] == slotIds_44[index]) {
                receiverState_24[index] = i;
                break;
            }
        }
        negotiationDone_446E = 1;
        negotiationResult_446D = 1;
        return;
    }
    setError(0x80000007, 0x5F, -error);
    negotiationDone_446E = -1;
    negotiationResult_446D = 1;
}

/* Opens the GameSpy socket and installs the callback set, then applies the pending requests. */
void GameSpyInterfaceThread::ConnectToAnybody(s32 arg)
{
    char address[7];
    s32 phase;
    s32 state;

    state = 1;
    phase = state_2C;
    if (phase <= 0) {
        INFO_LOG(lbl_80603388, phase);
        phase_74 = -1;
        return;
    }
    snprintf(address, 7, lbl_80793998, (u16)bufferSize_4482);
    address[6] = 0;
    if (fn_8050DEC0(&lbl_80794CE0, address, 0x2000, 0x2000, (NetworkCallback)fn_8041B194) == 0) {
        fn_8050E2A0(lbl_80794CE0, (NetworkCallback)fn_8041B270);
        fn_8050DF70(lbl_80794CE0, (NetworkCallback)fn_8041B538);
    } else {
        setError(0x80000007, 0x5F, 0x2DA2);
        state = 0;
    }
    if (state != 0 && closePending_123 != 0) {
        closePending_123 = 0;
        closeSession();
        state = 0;
    }
    if (cancelPending_122 != 0) {
        cancelPending_122 = 0;
        clearPendingClose();
        return;
    }
    if (state != 0) {
        phase_74 = 1;
        return;
    }
    phase_74 = -1;
}

/* Starts a GameSpy match for `count` players and stores the peer id it was given. */
s32 GameSpyInterfaceThread::startMatch(s32 count, u32 value, s32 a, u16 b,
                           s32 c, s32 d, s32 e)
{
    s32 phase;

    phase = phase_74;
    if (phase > 0) {
        INFO_LOG(lbl_806033C8, phase);
        return -1;
    }
    if (count <= 1) {
        return -1;
    }
    if (state_2C <= 0) {
        return -1;
    }
    paramA_7C = a;
    paramB_80 = b;
    paramC_84 = c;
    paramD_88 = d;
    paramE_8C = e;
    snprintf(name_9C, 0x80, lbl_80603408, b);
    if (count > 4) {
        WARN_LOG(lbl_80603448, count, 4);
        receiverCount_28 = 4;
    } else {
        receiverCount_28 = (u8)count;
    }
    value_30 = value;
    idByte_11C = (s8)(value >> 24);
    idByte_11D = (u8)(value >> 16);
    idByte_11E = (u8)(value >> 8);
    idByte_11F = (u8)value;
    phase_74 = 2;
    openRequested_125 = 1;
    return 1;
}

/* Ticks the GameSpy clock and counts one more frame. */
s32 GameSpyInterfaceThread::updateClock()
{
    s32 time[8];

    getInstance();
    fn_804167B4(time);
    fn_8050C5F0((const void*)time[3]);
    frame_70 = frame_70 + 1;
    return 1;
}

/* Moves the interface into its running state, or reports the shutdown. */
s32 GameSpyInterfaceThread::initialize()
{
    if (field_68 < 0) {
        state_2C = -1;
        return -1;
    }
    if (running_6C != 0) {
        return 0;
    }
    state_2C = 0;
    value_30 = 0;
    frame_70 = 0;
    phase_74 = 0;
    initRequested_124 = 1;
    return 0;
}

/* Reports whether a close is already pending, arming the step flag if so. */
u8 GameSpyInterfaceThread::requestClose()
{
    if (closePending_123 != 0 || cancelPending_122 != 0) {
        stepRequested_126 = 1;
        return 1;
    }
    if (started_120 != 0) {
        stopRequested_121 = 1;
    }
    return started_120;
}

/* Returns the interface phase. */
s32 GameSpyInterfaceThread::getPhase()
{
    return phase_74;
}

/* Returns the running state, or -1 while the interface is not running. */
s32 GameSpyInterfaceThread::getState()
{
    if (running_6C != 0) {
        return state_2C;
    }
    return -1;
}

/* Clears the running state and the pending close. */
void GameSpyInterfaceThread::clearPendingClose()
{
    running_6C = 0;
    state_2C = 0;
    phase_74 = 0;
}

/* Arms the cancellation when a close arrives while the request is still in flight. */
void GameSpyInterfaceThread::armCancel()
{
    if (field_68 < 0) {
        return;
    }
    if (running_6C == 0) {
        return;
    }
    if (started_120 == 0) {
        return;
    }
    cancelPending_122 = 1;
    if (state_2C == 1) {
        state_2C = 2;
    }
}

/* Tears the socket, the session and the request pool down under the interface mutex. */
s32 GameSpyInterfaceThread::closeSession()
{
    OSLockMutex(mutex_4450);
    if (sessionOpen_4484 != 0) {
        DWCi_NatNegEndSession(session_447C);
        DWCi_NatNegCleanup();
        sessionOpen_4484 = 0;
    }
    if (lbl_80794CE0 != 0) {
        fn_8050DED0(lbl_80794CE0);
        memset(lbl_806D3650, 0, 0x10);
        lbl_80794CE0 = 0;
    }
    OSUnlockMutex(mutex_4450);
    phase_74 = 0;
    return 0;
}

/* Reports whether the interface can be closed right now. */
s32 GameSpyInterfaceThread::canClose()
{
    if (field_68 < 0) {
        return -1;
    }
    if (state_2C != 1) {
        return -1;
    }
    if (phase_74 <= 0) {
        return -1;
    }
    if (started_120 != 0) {
        closePending_123 = 1;
        return -2;
    }
    return 0;
}

/* Replies to a pending request by filling the first free return handle. */
s32 GameSpyInterfaceThread::replyRequest(s32 handle)
{
    s32 i;

    if (field_68 < 0) {
        return -1;
    }
    if (state_2C != 1) {
        return -1;
    }
    if (phase_74 <= 0) {
        return -1;
    }
    if (started_120 != 0) {
        for (i = 0; i < receiverCount_28; i++) {
            if (slotHandles_58[i] == 0) {
                slotHandles_58[i] = handle;
                break;
            }
        }
        return -2;
    }
    return 0;
}

/* Resets the slot tables and the negotiation state. */
void GameSpyInterfaceThread::resetSlots()
{
    flag_78 = 0;
    memset(receivers_14, 0, 0x10);
    memset(receiverIds_34, 0, 0x10);
    memset(slotIds_44, 0, 0x10);
    memset(slotState_54, 0, 4);
    memset(slotHandles_58, 0, 0x10);
    negotiation_446C = 0;
    negotiationResult_446D = 0;
    negotiationDone_446E = 0;
    bufferSize_4482 = 0x2AF8;
    memset(receiverState_24, 3, 4);
    if (errorParam1_08 != 0x4B) {
        clearError();
    }
}

/* Stores the thread's vtable, publishes the singleton, resets the tables and starts its worker
 * thread.  The map row at 0x8041C66C is the mangled constructor name, and callers only reach it as
 * `new GameSpyInterfaceThread()`. */
GameSpyInterfaceThread::GameSpyInterfaceThread()
{
    vtable_00 = lbl_806036A0;
    sGameSpyInterfaceThread = this;
    field_68 = 0;
    running_6C = 0;
    frame_70 = 0;
    value_30 = 0;
    resetState();
    clearError();
    phase_74 = 0;
    started_120 = 0;
    initRequested_124 = 0;
    openRequested_125 = 0;
    stepRequested_126 = 0;
    result_90 = 0;
    stopRequested_121 = 0;
    mutexReady_444C = 0;
    memset(mutex_4450, 0, 0x18);
    GameSpyInterfaceThreadInit();
    memset(lbl_806D3650, 0, 0x10);
    lbl_80794CE0 = 0;
    sessionOpen_4484 = 0;
}

/* Deleting destructor: restores the base vtable, empties the singleton and frees on request. */
void* GameSpyInterfaceThread::destroy(s16 flags)
{
    if (this != NULL) {
        vtable_00 = lbl_806036A0;
        onDestroy();
        sGameSpyInterfaceThread = NULL;
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Resets the per-request tables and the pending-request flags. */
void GameSpyInterfaceThread::resetState()
{
    flag_78 = 0;
    memset(receivers_14, 0, 0x10);
    memset(receiverIds_34, 0, 0x10);
    memset(slotIds_44, 0, 0x10);
    memset(slotState_54, 0, 4);
    memset(slotHandles_58, 0, 0x10);
    receiverCount_28 = 0;
    state_2C = 0;
    cancelPending_122 = 0;
    closePending_123 = 0;
    memset(&busy_128, 0, 4);
    field_4468 = 0;
    negotiation_446C = 0;
    negotiationResult_446D = 0;
    negotiationDone_446E = 0;
}

/* Empty body: the interface thread's vtable cleanup hook. */
void GameSpyInterfaceThread::onDestroy()
{
}

/* Registers a receiver for `id` in the first free slot. */
s32 GameSpyInterfaceThread::registerReceiver(void* receiver, u32 id)
{
    u8 slot;
    u8 i;

    if (receiver == NULL) {
        return -1;
    }
    for (slot = 0; slot < receiverCount_28; slot++) {
        if (receivers_14[slot] == 0) {
            break;
        }
    }
    if (slot >= receiverCount_28) {
        return -2;
    }
    receivers_14[slot] = (u32)receiver;
    receiverIds_34[slot] = id;
    for (i = 0; i < receiverCount_28; i++) {
        if (id == slotHandles_58[i]) {
            break;
        }
    }
    if (i >= receiverCount_28) {
        for (i = 0; i < receiverCount_28; i++) {
            if (id == slotIds_44[i]) {
                receiverState_24[i] = slot;
                break;
            }
        }
    }
    return slot;
}

/* Releases the receiver slot at `index` and clears its id. */
void GameSpyInterfaceThread::unregisterReceiver(s32 index)
{
    u8 count;
    u8 i;

    count = receiverCount_28;
    replyRequest((s32)receiverIds_34[index]);
    for (i = 0; i < count; i++) {
        if (index == receiverState_24[i]) {
            receiverState_24[i] = 3;
            break;
        }
    }
    receivers_14[index] = 0;
    receiverIds_34[index] = 0;
}

/* Returns the state of the slot `id` maps to, or the pending negotiation result. */
u8 GameSpyInterfaceThread::getSlotState(u32 id)
{
    u8 i;

    for (i = 0; i < receiverCount_28; i++) {
        if (slotIds_44[i] == id) {
            return slotState_54[i];
        }
    }
    if (negotiationResult_446D != 0 && id != value_30 &&
        (id == peerId_4470 || id == selfPeerId_4474)) {
        return (u8)negotiationDone_446E;
    }
    return 0;
}

/* Returns the slot index `id` maps to, or -1. */
s8 GameSpyInterfaceThread::findSlot(u32 id)
{
    s8 i;

    for (i = 0; i < receiverCount_28; i++) {
        if (slotIds_44[i] == id) {
            return i;
        }
    }
    return -1;
}

/* Runs the interface's per-frame step: the negotiation sub-machine, the request sweep and the
   close/cancel bookkeeping. */
void GameSpyInterfaceThread::step()
{
    u32 info[3];
    s32 r;
    s32 error;
    s32 i;
    s8 slot;

    if (running_6C != 0 && state_2C >= 0) {
        OSLockMutex(mutex_4450);
        if (frame_70 == 1 && state_2C == 0) {
            r = fn_8050C770();
            if (r != 0) {
                if (r == 1) {
                    setRequestResult(0, 0);
                } else {
                    setRequestResult(-0x2DA1, 0);
                }
            }
        }
        if (field_68 == 0 && state_2C == 1 && phase_74 == 1 && lbl_80794CE0 != 0) {
            if (negotiation_446C != 0) {
                switch (negotiationStep_4480) {
                case 0:
                    peerMatch_4478 = (value_30 != peerId_4470);
                    if (lbl_806D3660.result_04 == 0) {
                        session_447C = peerId_4470 ^ selfPeerId_4474;
                        fn_8050E290(lbl_80794CE0);
                        r = DWCi_NatNegStartSession(session_447C, peerMatch_4478, (NetworkCallback)fn_8041B26C,
                                        (NetworkCallback)fn_8041B334, &lbl_806D3660);
                        if (r != 0) {
                            error = 0;
                            switch (r) {
                            case 1:
                                error = -0x2DA3;
                                break;
                            case 2:
                                error = -0x2DA4;
                                break;
                            case 3:
                                error = -0x2DA5;
                                break;
                            default:
                                break;
                            }
                            lbl_806D3660.result_04 = 1;
                            lbl_806D3660.active_00 = 0;
                            setError(0x80000007, 0x5F, -error);
                        }
                        sessionOpen_4484 = 1;
                        negotiationStep_4480 = 1;
                    } else {
                        negotiationStep_4480 = 2;
                    }
                    break;
                case 1:
                    if (lbl_806D3660.result_04 != 0) {
                        DWCi_NatNegCleanup();
                        sessionOpen_4484 = 0;
                        negotiationStep_4480 = 2;
                    }
                    break;
                case 2:
                    if (lbl_806D3660.active_00 != 0) {
                        if (peerMatch_4478 == 1) {
                            if (fn_8050DFA0(lbl_80794CE0, info,
                                            DWCi_formatAddress(lbl_806D3660.session_0C,
                                                        DWCi_htons(lbl_806D3660.encoded_0A), NULL),
                                            profile_4485, 0x14, 0x2710, lbl_806031A0, 0) == 0) {
                                negotiationStep_4480 = 3;
                                break;
                            }
                            setError(0x80000007, 0x5F, 0x2DAD);
                        }
                        negotiationDone_446E = -1;
                        negotiationResult_446D = 1;
                    } else {
                        negotiationDone_446E = -1;
                        negotiationResult_446D = 1;
                    }
                    negotiation_446C = 0;
                    break;
                case 3:
                    if (negotiationDone_446E != 0) {
                        negotiation_446C = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            DWCi_NatNegProcess();
            fn_8050DF20(lbl_80794CE0);
        }
        OSUnlockMutex(mutex_4450);
    }
    if (closePending_123 != 0) {
        closePending_123 = 0;
        if (field_68 == 0 && state_2C >= 1 && phase_74 > 0) {
            closeSession();
        }
    }
    for (i = 0; i < receiverCount_28; i++) {
        if (slotHandles_58[i] != 0) {
            slot = findSlot(slotHandles_58[i]);
            if (slot < 0) {
                slotHandles_58[i] = 0;
            } else {
                if (lbl_806D3650[slot] != 0) {
                    fn_8050E250(lbl_806D3650[slot]);
                }
                slotIds_44[slot] = 0;
                slotState_54[slot] = 0;
                slotHandles_58[i] = 0;
            }
        }
    }
    if (cancelPending_122 != 0) {
        if (field_68 == 0) {
            if (running_6C != 0) {
                if (frame_70 == 1 && (state_2C == 2 || state_2C == -2)) {
                    cancelPending_122 = 0;
                    clearPendingClose();
                }
            } else {
                cancelPending_122 = 0;
            }
        } else {
            cancelPending_122 = 0;
        }
    }
}

/* Returns the request result, arming the step flag first. */
s32 GameSpyInterfaceThread::getResult()
{
    if (field_68 < 0) {
        return -1;
    }
    stepRequested_126 = 1;
    return result_90;
}

/* Drains the DWC error queue and folds the reported type into the interface state. */
s32 GameSpyInterfaceThread::executeError()
{
    s32 code;
    s32 type;

    if (fn_805073C0(&code, &type) != 0 && code < 0 && type != 0) {
        INFO_LOG(lbl_806034A0, code, type);
        switch (type) {
        case 1:
            setError(0x80000007, 0x4A, -code);
            fn_80507470();
            break;
        case 2:
            setError(0x80000007, 0x49, -code);
            fn_80507470();
            break;
        case 3:
            setError(0x80000007, 0x49, -code);
            errorReported_10 = 0;
            state_2C = code;
            if (running_6C != 0) {
                if (sessionOpen_4484 != 0) {
                    DWCi_NatNegEndSession(session_447C);
                    DWCi_NatNegCleanup();
                    sessionOpen_4484 = 0;
                }
                if (lbl_80794CE0 != 0) {
                    fn_8050DED0(lbl_80794CE0);
                    memset(lbl_806D3650, 0, 0x10);
                    lbl_80794CE0 = 0;
                }
                running_6C = 0;
            }
            phase_74 = 0;
            fn_80507470();
            break;
        case 6:
            setError(0x80000007, 0x49, -code);
            errorReported_10 = 0;
            field_68 = -1;
            state_2C = code;
            if (running_6C != 0) {
                if (sessionOpen_4484 != 0) {
                    DWCi_NatNegEndSession(session_447C);
                    DWCi_NatNegCleanup();
                    sessionOpen_4484 = 0;
                }
                if (lbl_80794CE0 != 0) {
                    fn_8050DED0(lbl_80794CE0);
                    memset(lbl_806D3650, 0, 0x10);
                    lbl_80794CE0 = 0;
                }
                running_6C = 0;
            }
            phase_74 = 0;
            fn_80507470();
            break;
        case 7:
            errorReported_10 = 1;
            setError(0x80000007, 0x4B, -code);
            errorReported_10 = 0;
            field_68 = -1;
            state_2C = code;
            break;
        default:
            break;
        }
        return type;
    }
    return 0;
}

/* Copies the interface's error record out for the game to read. */
void GameSpyInterfaceThread::getErrorStruct(NetworkErrorInfo* out)
{
    if (out != NULL) {
        out->code_00 = errorCode_04;
        out->param1_04 = errorParam1_08;
        out->param2_08 = errorParam2_0C;
    }
}

/* Clears the error record and marks it reported. */
void GameSpyInterfaceThread::clearError()
{
    memset(&errorCode_04, 0, 0xC);
    errorReported_10 = 1;
}

/* Stores the first error the interface sees, unless it has already been reported. */
void GameSpyInterfaceThread::setError(s32 code, s32 a, s32 b)
{
    if (errorCode_04 == 0 || errorReported_10 != 0) {
        errorCode_04 = code;
        errorParam1_08 = a;
        errorParam2_0C = b;
    }
}

/* Sends a buffer out over the socket the index maps to. */
s32 GameSpyInterfaceThread::sendUnreliable(u8 index, const void* data, s32 size)
{
    s32 sent;

    sent = 0;
    if (field_68 < 0 || state_2C != 1 || phase_74 <= 0) {
        INFO_LOG(lbl_806034F4, size);
    } else {
        if (mutexReady_444C != 0) {
            OSLockMutex(mutex_4450);
            if (lbl_806D3650[index] != 0) {
                sent = size;
                fn_8050E150(lbl_806D3650[index], data, size, 0);
            }
            OSUnlockMutex(mutex_4450);
        } else {
            if (lbl_806D3650[index] != 0) {
                sent = size;
                fn_8050E150(lbl_806D3650[index], data, size, 0);
            }
        }
    }
    return sent;
}

/* The worker thread's body: drains the request flags until the stop flag is set. */
void GameSpyInterfaceThread::tGameSpyInterface(s32 arg)
{
    u64 ticks;

    OSInitMutex(mutex_4450);
    mutexReady_444C = 1;
    for (;;) {
        if ((s8)initRequested_124 != 0) {
            running_6C = 1;
            updateClock();
            initRequested_124 = 0;
        } else if ((s8)openRequested_125 != 0) {
            ConnectToAnybody(arg);
            openRequested_125 = 0;
        } else if ((s8)stepRequested_126 != 0) {
            step();
            stepRequested_126 = 0;
        }
        if (stopRequested_121 != 0) {
            stopRequested_121 = 0;
            started_120 = 0;
            break;
        }
        ticks = (u64)(*(volatile u32*)0x800000F8 / 4 / 1000);
        OSSleepTicks(ticks * 17);
    }
    mutexReady_444C = 0;
    INFO_LOG(lbl_80603548);
}

/* The worker thread's entry point. */
extern "C" s32 runThread(void* self)
{
    ((GameSpyInterfaceThread*)self)->tGameSpyInterface((s32)self);
    return 0;
}

/* Spawns the interface's worker thread. */
s32 GameSpyInterfaceThread::GameSpyInterfaceThreadInit()
{
    s32 thread;

    thread = OSCreateThread(thread_130, runThread, this, &threadParam_4448, 0x4000, 0x0E, 1);
    if (thread != 0) {
        started_120 = 1;
        threadParam_4448 = 0;
        OSResumeThread(thread_130);
        INFO_LOG(lbl_80603590);
    } else {
        INFO_LOG(lbl_806035E4);
    }
    return thread;
}

/* Sets the socket address buffer size. */
void GameSpyInterfaceThread::setBufferSize(s16 size)
{
    bufferSize_4482 = size;
}

/* Starts a NAT negotiation between the two peer ids. */
#pragma peephole on
void GameSpyInterfaceThread::startNegotiation(const GameSpyPeerId* a, const GameSpyPeerId* b)
{
    s32 i;

    if (a == NULL || b == NULL || negotiation_446C != 0) {
        return;
    }
    negotiationResult_446D = 0;
    negotiationDone_446E = 0;
    peerId_4470 = a->peerId_00;
    selfPeerId_4474 = b->peerId_00;
    lbl_806D3660.result_04 = 0;
    if (a->mode_04 == b->mode_04) {
        lbl_806D3660.result_04 = 1;
        lbl_806D3660.active_00 = 1;
        lbl_806D3660.session_0C = a->session_08;
        {
            NetworkLogger* lm = getNetworkLogger();
            lbl_806D3660.encoded_0A = lm->encode_4C(a->port_0C);
        }
    }
    profile_4485[0] = selfPeerId_4474;
    profile_4485[1] = paramA_7C;
    profile_4485[2] = paramB_80;
    profile_4485[3] = paramC_84;
    profile_4485[4] = paramD_88;
    negotiationStep_4480 = 0;
    negotiation_446C = 1;
}
#pragma peephole off

/* Reports whether a negotiation is running. */
u8 GameSpyInterfaceThread::isNegotiating()
{
    return negotiation_446C;
}

/* Reports the negotiation's outcome. */
u8 GameSpyInterfaceThread::getNegotiationResult()
{
    return (u8)negotiationDone_446E;
}

/* Compares a received peer profile against the one this interface published. */
s32 GameSpyInterfaceThread::checkPeerProfile(const void* profile, u32 size)
{
    if (size == 0x14) {
        if (memcmp(profile, profile_4485, 0x14) == 0) {
            return 1;
        }
        if (memcmp(profile, profile_4485, 4) != 0) {
            SIGNAL_LOG(3, lbl_80603638);
        }
        if (memcmp((const u8*)profile + 4, (const u8*)profile_4485 + 4, 0x10) != 0) {
            SIGNAL_LOG(3, lbl_80603660);
        }
        return 0;
    }
    SIGNAL_LOG(3, lbl_80603680, size);
    return 0;
}

/* Returns this interface's own peer id. */
u32 GameSpyInterfaceThread::getPeerId()
{
    return selfPeerId_4474;
}

/* Binds the peer to the interface and resets both buffers. */
void NetworkPeerGameSpy::bind(const u32* id)
{
    interface_6634 = (GameSpyInterfaceThread*)id[0];
    peer_6638 = id[1];
    field_6630 = interface_6634->registerReceiver(this, peer_6638);
    memset(sendBuffer_14, 0, 0x600);
    memset(recvBuffer_614, 0, 0x6000);
    received_10 = 0;
}

/* Builds and sends a framed peer message from the two optional payloads. */
s32 NetworkPeerGameSpy::send(const u16* a, s32 aLen, const u16* b, s32 bLen,
                           s8 flag)
{
    s8 flagByte;
    u16 aLen16;
    u16 bLen16;
    u8* record;
    u8* cursor;
    s32 total;
    s8 slot;

    flagByte = flag;
    slot = interface_6634->getSlotState(peer_6638);
    if (slot < 0) {
        networkPeerError_set(this, lbl_806036A0, 0, -1);
        return -1;
    }
    record = sendBuffer_14;
    if (a == NULL || aLen <= 0) {
        aLen16 = 0;
        memcpy(record, &aLen16, 2);
        cursor = record + 2;
        total = 2;
    } else {
        {
            NetworkLogger* lm = getNetworkLogger();
            aLen16 = lm->encode_4C((u16)aLen);
        }
        memcpy(record, &aLen16, 2);
        memcpy(record + 2, a, (u32)(u16)aLen);
        cursor = record + 2 + aLen;
        total = aLen + 2;
    }
    if (b == NULL || bLen <= 0) {
        bLen16 = 0;
        memcpy(cursor, &bLen16, 2);
        total = total + 2;
    } else {
        {
            NetworkLogger* lm = getNetworkLogger();
            bLen16 = lm->encode_4C((u16)(bLen + 1));
        }
        memcpy(cursor, &bLen16, 2);
        memcpy(cursor + 2, &flagByte, 1);
        memcpy(cursor + 3, b, (u32)(u16)bLen);
        total = total + 3 + bLen;
    }
    {
        NetworkLogger* lm = getNetworkLogger();
        if (lm->flag_48(aLen16) == 0) {
            NetworkLogger* lm2 = getNetworkLogger();
            if (lm2->flag_48(bLen16) == 0) {
                return 0;
            }
        }
    }
    slot = interface_6634->findSlot(peer_6638);
    if (slot < 0) {
        networkPeerError_set(this, lbl_806036A0, 0, -2);
        return -1;
    }
    if (interface_6634->sendUnreliable((u8)slot, sendBuffer_14, total) == 0) {
        networkPeerError_set(this, lbl_806036A0, 0, -3);
        return -1;
    }
    return total;
}

/* Pulls one framed message out of the peer's receive buffer. */
s32 NetworkPeerGameSpy::receive(void* a, s32* aLen, void* b, s32* bLen,
                           u8* flag)
{
    u16 aLen16;
    u16 bLen16;
    u16 payload;
    u8* cursor;
    s32 aMax;
    s32 bMax;
    s32 consumed;
    s8 slot;

    aMax = *aLen;
    bMax = *bLen;
    payload = 0;
    *aLen = 0;
    *bLen = 0;
    *flag = 0;
    slot = interface_6634->getSlotState(peer_6638);
    if (slot < 0) {
        networkPeerError_set(this, lbl_806036A0, 0, 0);
        return -1;
    }
    if (received_10 < 4) {
        return 0;
    }
    LockMutex(mutex_6614);
    cursor = recvBuffer_614;
    memcpy(&aLen16, cursor, 2);
    {
        NetworkLogger* lm = getNetworkLogger();
        aLen16 = lm->flag_48(aLen16);
    }
    bLen16 = 0;
    if (received_10 >= (u32)aLen16 + 4) {
        memcpy(&bLen16, cursor + aLen16 + 2, 2);
        {
            NetworkLogger* lm = getNetworkLogger();
            payload = lm->flag_48(bLen16);
        }
        bLen16 = payload;
        cursor = recvBuffer_614;
    }
    if (received_10 < (u32)(aLen16 + payload + 4)) {
        UnlockMutex(mutex_6614);
        return 0;
    }
    cursor += 2;
    if (aLen16 != 0 && aLen16 <= aMax) {
        memcpy(a, cursor, aLen16);
        *aLen = aLen16;
    }
    cursor += aLen16;
    cursor += 2;
    if (bLen16 > 1) {
        payload = bLen16 - 1;
        if (payload <= bMax) {
            memcpy(flag, cursor, 1);
            cursor += 1;
            memcpy(b, cursor, payload);
            cursor += payload;
            *bLen = payload;
        }
    }
    consumed = aLen16 + bLen16 + 4;
    received_10 = received_10 - consumed;
    if (received_10 != 0) {
        memmove(recvBuffer_614, cursor, received_10);
    }
    UnlockMutex(mutex_6614);
    return consumed;
}

/* Appends a buffer to the peer's receive queue. */
s32 NetworkPeerGameSpy::put(const void* data, u32 size)
{
    LockMutex(mutex_6614);
    if (received_10 + size > 0x6000) {
        INFO_LOG(lbl_806036B0);
        UnlockMutex(mutex_6614);
        return -1;
    }
    memcpy(recvBuffer_614 + received_10, data, size);
    received_10 = received_10 + size;
    UnlockMutex(mutex_6614);
    return 0;
}

/* Reports whether the peer has a message queued. */
#pragma peephole on
s32 NetworkPeerGameSpy::isQueued()
{
    s8 state;

    state = (s8)interface_6634->getSlotState(peer_6638);
    if (state > 0) {
        return 1;
    }
    return state & (state >> 31);
}
#pragma peephole off

/* Empty body: the peer's vtable placeholder. */
extern "C" void fn_8041DD24(void)
{
}

/* Ticks the callback object's virtual slot. */
extern "C" s32 fn_8041DD28(NetworkPeerCallback* self)
{
    self->tick_28();
    return 1;
}

/* Releases the peer's interface slot and drops its receive queue. */
void NetworkPeerGameSpy::release()
{
    interface_6634->unregisterReceiver(field_6630);
    LockMutex(mutex_6614);
    memset(recvBuffer_614, 0, 0x6000);
    received_10 = 0;
    UnlockMutex(mutex_6614);
}

/* Deleting destructor: destroys the queue mutex and the base, then frees on request. */
void* NetworkPeerGameSpy::destroy(s16 flags)
{
    if (this != NULL) {
        dtor_803CA338(mutex_6614, -1);
        /* C cast: this class still hand-models `void* vtable_00`, unrelated to NetworkPeerBase; the measured alternatives failed. */
        ((NetworkPeerBase*)this)->NetworkPeerBase::destroy(0);
        if (flags > 0) {
            operator delete(this);
        }
    }
    return this;
}

/* Constructs the timed handler. */
void* NetworkTimedHandler::create()
{
    vtable_00 = lbl_80603740;
    init((s32)lbl_80603740, 0, 0);
    return this;
}

/* Deleting destructor for the timed handler. */
void* NetworkTimedHandler::destroy(s16 flags)
{
    if (this != NULL && flags > 0) {
        operator delete(this);
    }
    return this;
}

/* Initialises the timed handler's interval, limit and timeout. */
void NetworkTimedHandler::init(s32 a, s32 b, s32 c)
{
    clear();
    timeout_14 = 1000;
    limit_0C = c;
    interval_08 = b;
}

/* Clears the timed handler's state, ready flag and expiry flag. */
void NetworkTimedHandler::clear()
{
    state_04 = 0;
    ready_10 = 0;
    expired_18 = 0;
}
