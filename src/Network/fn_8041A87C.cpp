/*
 * fn_8041A87C.cpp - the GameSpy interface / peer band, `.text` 0x8041A87C..0x8041DF10.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup` - every address in the range answers a `zz_XXXXXXXX_`
 * dump name; the three exceptions `getErrorStruct`, `runThread` and `startThread` are spelled as the
 * map spells them).  No `unkNN` identifier survives: every field is named from its use.
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
 * Camellia's start).
 *
 * LANGUAGE AND SECTIONS.  C++ (mangled `__dl__FPv` delete, vtables).  Per-file `#pragma exceptions on`
 * because the `Network` lib sets `-Cpp_exceptions off` while retail's object carries `extab` 380 B and
 * `extabindex` 540 B (playbook 30's pragma pair).  Nothing in the range's `.data` is claimed: the
 * 0x80603xxx string pool is shared with the NetworkWiiMediator band above it (that band loads the
 * same literals), so per playbook 58 it can be neither claimed nor named here.
 *
 * FLAGS.  The object deviates from the lib on two points, both in `configure.py` with the evidence:
 * `-O3` + `-inline noauto` in place of `-O4,p` + `-inline auto` (retail calls the file-static helpers
 * - fn_8041DCDC is 72 B against our 156 B when the inliner folds them in), and the exceptions pragma
 * above.  Unit: 91.95 % fuzzy, 21/71 functions byte-identical, .text 13716 B against 13972 B.
 *
 * RESIDUALS.  fn_8041B720 70.72 / fn_8041C514 66.42 / fn_8041C84C 64.20 - retail drives the
 * receiver-table loops with `mtctr`+`bdnz` and a pointer advanced by 4 where ours emits a
 * `clrlwi`/`cmplw`+`blt` rotation from the same source shape; fn_8041DA10 77.90 (536 B retail
 * against our 476).  Data: `extabindex` 540/540 exact, `extab` 360 against 380 - the target's last
 * entry is the 20-byte cleanup record whose `.relaextab` reloc points at `dtor_803CA338`, a local
 * object with a destructor the range's own code does not show.
 */

#include "types.h"
#include "Network/fn_8041A87C.h"

/* The target object carries `extab`/`extabindex` (380/540 B) while the `Network` lib is built with
 * exceptions off, so the front-end is told per file (the pragma pair of playbook 30). */
#pragma exceptions on

/* The debug manager's virtual slots: the target re-runs `bl fn_803C9974` at *every* logging site
 * (never once per function), so each site expands to its own block that fetches the singleton and
 * passes it as the slot's receiver. */
#define SIGNAL_LOG(...) do { NetworkLogger* lm = fn_803C9974(); lm->vtable->signal_0C(lm, __VA_ARGS__); } while (0)
#define WARN_LOG(...)   do { NetworkLogger* lm = fn_803C9974(); lm->vtable->warn_10(lm, __VA_ARGS__); } while (0)
#define INFO_LOG(...)   do { NetworkLogger* lm = fn_803C9974(); lm->vtable->log_14(lm, __VA_ARGS__); } while (0)

extern "C" {

/* ---- the runtime's own library entry points (the same spelling the sibling units use) --------- */
void* memset(void* dst, s32 c, u32 n);
void* memcpy(void* dst, const void* src, u32 n);
void* memmove(void* dst, const void* src, u32 n);
s32 memcmp(const void* a, const void* b, u32 n);
s32 snprintf(char* buffer, u32 size, const char* fmt, ...);

/* ---- this unit's own bodies, in address order ------------------------------------------------ */
void fn_8041A87C(NetworkGameSpyInterface* self);
s32 fn_8041AAA8(NetworkGameSpyInterface* self);
s32 fn_8041AAD0(NetworkGameSpyInterface* self);
s32 fn_8041AD30(NetworkGameSpyInterface* self);
void fn_8041AEA8(NetworkGameSpyInterface* self, u32 code, s32 a, void* b, const GameSpyEventMsg* msg);
void fn_8041B194(void);
void fn_8041B26C(void);
s32 fn_8041B270(NetworkInstance* self, u32 peer, u16 value, const void* data, u32 size);
void fn_8041B334(s32 result, s32 unused, const GameSpyAddress* src, GameSpyResultInfo* info);
void fn_8041B514(GameSpyAddress* out, const GameSpyAddress* in);
void fn_8041B538(s32 unused0, s32 socket, s32 unused1, s32 unused2, s32 unused3, const void* profile, u32 size);
void fn_8041B720(s32 socket, s32 result, s32 unused, s32 timeout);
void fn_8041B894(u32 socket, s32 address, s32 size);
void fn_8041B984(u32 socket, s32 result);
void fn_8041BAB4(void);
void fn_8041BAF4(GameSpyInterfaceThread* self, s32 limit);
s32 fn_8041BB04(GameSpyInterfaceThread* self);
void fn_8041BD64(GameSpyInterfaceThread* self, u8 index, s32 a, s32 b);
void fn_8041BDAC(GameSpyInterfaceThread* self, s32 error, s32 a, s32 b, u8 index, s32 e);
void fn_8041BE10(GameSpyInterfaceThread* self, s32 error, s32 value);
void fn_8041BE94(GameSpyInterfaceThread* self, s32 error, u8 index, u32 value);
void fn_8041C010(GameSpyInterfaceThread* self);
s32 fn_8041C170(GameSpyInterfaceThread* self, s32 count, u32 value, s32 a, u16 b, s32 c, s32 d, s32 e);
s32 fn_8041C2C0(GameSpyInterfaceThread* self);
s32 fn_8041C30C(GameSpyInterfaceThread* self);
u8 fn_8041C360(GameSpyInterfaceThread* self);
s32 fn_8041C3A4(GameSpyInterfaceThread* self);
s32 fn_8041C3AC(GameSpyInterfaceThread* self);
void fn_8041C3C8(GameSpyInterfaceThread* self);
void fn_8041C3DC(GameSpyInterfaceThread* self);
s32 fn_8041C420(GameSpyInterfaceThread* self);
s32 fn_8041C4B4(GameSpyInterfaceThread* self);
s32 fn_8041C514(GameSpyInterfaceThread* self, s32 handle);
void fn_8041C5AC(GameSpyInterfaceThread* self);
void* fn_8041C66C(GameSpyInterfaceThread* self);
void* fn_8041C724(GameSpyInterfaceThread* self, s16 flags);
void fn_8041C790(GameSpyInterfaceThread* self);
void fn_8041C848(GameSpyInterfaceThread* self);
u8 fn_8041C84C(GameSpyInterfaceThread* self, void* receiver, u32 id);
void fn_8041C938(GameSpyInterfaceThread* self, s32 index);
u8 fn_8041C9D8(GameSpyInterfaceThread* self, u32 id);
s8 fn_8041CA58(GameSpyInterfaceThread* self, u32 id);
void fn_8041CA94(GameSpyInterfaceThread* self);
s32 fn_8041CED0(GameSpyInterfaceThread* self);
s32 fn_8041CEF4(GameSpyInterfaceThread* self);
void fn_8041D1A4(GameSpyInterfaceThread* self);
void fn_8041D1E4(GameSpyInterfaceThread* self, s32 code, s32 a, s32 b);
s32 fn_8041D20C(GameSpyInterfaceThread* self, u8 index, const void* data, s32 size);
void fn_8041D344(GameSpyInterfaceThread* self, s32 arg);
s32 runThread(void* self);
s32 startThread(GameSpyInterfaceThread* self);
void fn_8041D528(GameSpyInterfaceThread* self, s16 size);
void fn_8041D530(GameSpyInterfaceThread* self, const GameSpyPeerId* a, const GameSpyPeerId* b);
u8 fn_8041D628(GameSpyInterfaceThread* self);
u8 fn_8041D630(GameSpyInterfaceThread* self);
s32 fn_8041D638(GameSpyInterfaceThread* self, const void* profile, u32 size);
u32 fn_8041D748(GameSpyInterfaceThread* self);
void fn_8041D750(NetworkPeerGameSpy* self, const u32* id);
s32 fn_8041D7C0(NetworkPeerGameSpy* self, const u16* a, s32 aLen, const u16* b, s32 bLen, s8 flag);
s32 fn_8041DA10(NetworkPeerGameSpy* self, void* a, s32* aLen, void* b, s32* bLen, u8* flag);
s32 fn_8041DC28(NetworkPeerGameSpy* self, const void* data, u32 size);
s32 fn_8041DCDC(NetworkPeerGameSpy* self);
void fn_8041DD24(void);
s32 fn_8041DD28(NetworkPeerCallbackVtable** self);
void fn_8041DD58(NetworkPeerGameSpy* self);
void* fn_8041DDB4(NetworkPeerGameSpy* self, s16 flags);
void* fn_8041DE20(NetworkTimedHandler* self);
void* fn_8041DE64(NetworkTimedHandler* self, s16 flags);
void fn_8041DEA8(NetworkTimedHandler* self, s32 a, s32 b, s32 c);
void fn_8041DEFC(NetworkTimedHandler* self);

}

/* ------------------------------------------------------------------------------------------------ */

/* Runs the connect-attempt callback sub-machine, one step per frame. */
extern "C" void fn_8041A87C(NetworkGameSpyInterface* self)
{
    u8 step;

    if (self->callbackStep_17 != 0 && getInstance_() == NULL) {
        WARN_LOG(lbl_80603154);
        self->callbackStep_17 = 0;
    }
    step = self->callbackStep_17;
    switch (step) {
    case 1:
        if (self->connectStep_15 == 0) {
            self->callbackStep_17 = 6;
            break;
        }
        if (isCallback(getInstance_(), 7) == 0) {
            self->callbackStep_17 = 5;
            break;
        }
        if (fn_803FD658(getInstance_()) != 0) {
            self->callbackStep_17 = 4;
            break;
        }
        if ((u8)getSomething5(getInstance_()) == 0) {
            self->callbackStep_17 = 4;
            break;
        }
        self->flags_0C = 0;
        sendReqShut(getInstance_(), 1);
        self->callbackStep_17 = (u8)(self->callbackStep_17 + 1);
        break;
    case 2:
        if ((self->flags_0C & 1) != 0 || (self->flags_0C & 2) != 0 || (self->flags_0C & 0x10) != 0) {
            self->flags_0C = 0;
            resetNetworkState3(getInstance_());
            self->callbackStep_17 = (u8)(self->callbackStep_17 + 1);
        }
        break;
    case 3:
        if ((self->flags_0C & 8) != 0) {
            self->callbackStep_17 = (u8)(step + 1);
        }
        break;
    case 4:
        resetCallback(getInstance_(), 7);
        self->callbackStep_17 = (u8)(self->callbackStep_17 + 1);
        break;
    case 5:
        decrement60d4(getInstance_());
        self->callbackStep_17 = (u8)(self->callbackStep_17 + 1);
        break;
    case 6: {
        NetworkLogger* lm = fn_803C9974();
        if (lm->vtable->isVerbose_3C(lm) > 0) {
            self->callbackStep_17 = (u8)(self->callbackStep_17 + 1);
        }
        break;
    }
    case 7:
        self->callbackStep_17 = 0;
        self->connectStep_15 = 0;
        fn_8041A5A8(self, 0x6003, 0, 0, 0, NULL);
        break;
    default:
        break;
    }
}

/* Dispatches to the search sub-machine (task 1) or the connect sub-machine (task 2). */
extern "C" s32 fn_8041AAA8(NetworkGameSpyInterface* self)
{
    switch (self->task_10) {
    case 1:
        return fn_8041AAD0(self);
    case 2:
        return fn_8041AD30(self);
    default:
        return 0;
    }
}

/* Runs the GameSpy connect sub-machine, one step per frame. */
extern "C" s32 fn_8041AAD0(NetworkGameSpyInterface* self)
{
    u32 pending;
    u32 offset;
    u32 size;
    u32 info[3];

    switch (self->searchStep_14) {
    case 0:
        self->flags_0C = 0;
        fn_80403F60(getInstance_(), self->channel_18);
        self->searchStep_14 = 5;
        return 0;
    case 5:
        pending = self->flags_0C;
        if ((pending & 1) != 0) {
            self->searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            self->searchStep_14 = 0x64;
        } else if ((pending & 0x80) != 0) {
            self->writePos_8168 = 0;
            self->searchStep_14 = 0x0A;
        }
        return 0;
    case 0x0A:
        self->flags_0C = 0;
        offset = self->writePos_8168;
        size = self->limit_8164 - offset;
        if (size >= 0x2000) {
            size = 0x2000;
        }
        fn_80403FE4(getInstance_(), self->channel_18, offset, size);
        self->searchStep_14 = 0x0F;
        return 0;
    case 0x0F:
        pending = self->flags_0C;
        if ((pending & 1) != 0) {
            self->searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            self->searchStep_14 = 0x64;
        } else if ((pending & 0x100) != 0) {
            if (self->limit_8164 - self->writePos_8168 != 0) {
                self->searchStep_14 = 0x0A;
            } else {
                self->searchStep_14 = 0x14;
            }
        }
        return 0;
    case 0x14:
        fn_8041A5A8(self, 0x6004, 0, 0, 1, &self->channel_18);
        return 1;
    case 0x64:
        info[0] = 0x80000007;
        info[1] = 0;
        info[2] = (u32)fn_803FD694(getInstance_(), 0);
        fn_8041A5A8(self, 0x6004, 0, (s32)info[0], 1, info);
        return 1;
    case 0x6E:
        fn_803FD794(getInstance_(), info);
        fn_8041A5A8(self, 0x6004, 0, (s32)info[0], 1, info);
        fn_8041A5A8(self, 0x6001, 0, (s32)info[0], 1, info);
        return 1;
    default:
        return 0;
    }
}

/* Runs the GameSpy NAT/connect sub-machine, one step per frame. */
extern "C" s32 fn_8041AD30(NetworkGameSpyInterface* self)
{
    u32 pending;
    u32 info[3];

    switch (self->searchStep_14) {
    case 0:
        self->flags_0C = 0;
        fn_80404070(getInstance_());
        self->searchStep_14 = 5;
        return 0;
    case 5:
        pending = self->flags_0C;
        if ((pending & 1) != 0) {
            self->searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            self->searchStep_14 = 0x64;
        } else if ((pending & 0x200) != 0) {
            self->searchStep_14 = 0x0A;
        }
        return 0;
    case 0x0A:
        fn_8041A5A8(self, 0x6005, 0, 0, 0, NULL);
        return 1;
    case 0x64:
        info[0] = 0x80000007;
        info[1] = 0;
        info[2] = (u32)fn_803FD694(getInstance_(), 0);
        fn_8041A5A8(self, 0x6005, 0, (s32)info[0], 1, info);
        return 1;
    case 0x6E:
        fn_803FD794(getInstance_(), info);
        fn_8041A5A8(self, 0x6005, 0, (s32)info[0], 1, info);
        fn_8041A5A8(self, 0x6001, 0, (s32)info[0], 1, info);
        return 1;
    default:
        return 0;
    }
}

/* Applies a DWC event to the interface, then folds the event bits into the state machine's flags. */
extern "C" void fn_8041AEA8(NetworkGameSpyInterface* self, u32 code, s32 a, void* b, const GameSpyEventMsg* msg)
{
    u32 limit;
    u32 size;
    s32 i;
    GameSpyChannel* dst;
    const GameSpyChannel* src;
    u32 info[3];

    switch (code) {
    case 0x8000:
    case 0x8007:
        if (self->task_10 <= 0) {
            fn_803FD794(getInstance_(), info);
            fn_8041A5A8(self, 0x6001, 0, (s32)info[0], 1, info);
        }
        self->flags_0C |= 1;
        break;
    case 0x8002:
        self->flags_0C |= 2;
        break;
    case 0x8004:
        if (a != 0) {
            self->flags_0C |= 1;
        }
        break;
    case 0x8005:
        self->flags_0C |= 8;
        break;
    case 0x8006:
        self->flags_0C |= 1;
        break;
    case 0x8080:
        if (msg == NULL) {
            break;
        }
        if (msg->channelView.channel_00 != self->channel_18) {
            info[0] = 0x80000000;
            info[1] = 0;
            info[2] = 0;
            getInstance_()->vtable->postError_288(getInstance_(), (NetworkErrorInfo*)info);
            break;
        }
        self->peerId_1C = msg->channelView.peerId_04;
        self->channelCount_8020 = msg->channelView.count_0C;
        if (self->channelCount_8020 > 8) {
            self->channelCount_8020 = 8;
        }
        limit = msg->channelView.limit_08;
        self->limit_8164 = limit > 0x8000 ? 0x7FFF : limit;
        src = msg->channelView.channels_10;
        dst = self->channels_8024;
        for (i = 0; i < self->channelCount_8020; i++) {
            dst->ownerId_00 = src->ownerId_00;
            dst->peerId_04 = src->peerId_04;
            memcpy(dst->address_08, src->address_08, 0x1F);
            dst->tail_27 = 0;
            src++;
            dst++;
        }
        self->flags_0C |= 0x80;
        break;
    case 0x8081:
        if (msg == NULL) {
            break;
        }
        if (msg->dataView.channel_00 != self->channel_18 ||
            msg->dataView.writePos_04 != self->writePos_8168) {
            info[0] = 0x80000000;
            info[1] = 0;
            info[2] = 0;
            getInstance_()->vtable->postError_288(getInstance_(), (NetworkErrorInfo*)info);
            break;
        }
        size = msg->dataView.size_08;
        limit = self->limit_8164 - self->writePos_8168;
        if (size > limit) {
            size = limit;
        }
        memcpy(self->recvArea_20 + self->writePos_8168, msg->dataView.data_0C, size);
        self->writePos_8168 += size;
        self->flags_0C |= 0x100;
        break;
    case 0x8082:
        self->flags_0C |= 0x200;
        break;
    default:
        break;
    }
}

/* Drops every socket of the three-slot table, one request record at a time. */
extern "C" void fn_8041B194(void)
{
    s32 i;

    SIGNAL_LOG(3, lbl_806031B0);
    if (fn_803D6A98() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    for (i = 0; i < 3; i++) {
        if (lbl_806D3650[i] != 0) {
            fn_8041BDAC((GameSpyInterfaceThread*)fn_803D6A98(), -0x2DB0, 1, 0, (u8)i, -1);
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
    if (memcmp(data, lbl_80794380, 6) == 0) {
        fn_80514400((void*)data, size, &header);
        return 1;
    }
    return 0;
}

/* Maps a GameSpy connect result onto the interface's request state and error record. */
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
    if (fn_803D6A98() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    if (info->connected_00 == 0) {
        fn_8041D1E4((GameSpyInterfaceThread*)fn_803D6A98(), 0x80000007, 0x5F, -error);
    }
}

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
    if (fn_803D6A98() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    peerId = fn_8041D748((GameSpyInterfaceThread*)fn_803D6A98());
    if (fn_8041D638((GameSpyInterfaceThread*)fn_803D6A98(), profile, size) == 0) {
        fn_8050DF90(socket, lbl_80793990, 2);
        fn_8041BE94((GameSpyInterfaceThread*)fn_803D6A98(), -0x2DA0, 0xFF, peerId);
        return;
    }
    if (fn_8050DF80(socket, lbl_806031A0) != 0) {
        SIGNAL_LOG(3, lbl_80603278);
        for (i = 0; i < 3; i++) {
            if (lbl_806D3650[i] == 0) {
                lbl_806D3650[i] = socket;
                fn_8041BE94((GameSpyInterfaceThread*)fn_803D6A98(), 0, (u8)i, peerId);
                break;
            }
        }
        if (i >= 3 && getInstance_() != NULL) {
            info[0] = 0x80000007;
            info[1] = 0x5F;
            info[2] = 0x2D6A;
            getInstance_()->vtable->postError_288(getInstance_(), (NetworkErrorInfo*)info);
        }
    } else {
        fn_8041BE94((GameSpyInterfaceThread*)fn_803D6A98(), -0x2DAE, 0xFF, peerId);
    }
}

/* Handles a socket accept callback: registers the socket or fails the request. */
extern "C" void fn_8041B720(s32 socket, s32 result, s32 unused, s32 timeout)
{
    s32 i;
    u32 info[6];
    s32 error;

    SIGNAL_LOG(3, lbl_80603298, result);
    if (fn_803D6A98() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    if (result != 0) {
        error = timeout > 0 ? -0x2DA0 : -0x2DAD;
        fn_8041BE94((GameSpyInterfaceThread*)fn_803D6A98(), error, 0xFF, 0);
        return;
    }
    for (i = 0; i < 3; i++) {
        if (lbl_806D3650[i] == 0) {
            lbl_806D3650[i] = socket;
            fn_8041BE94((GameSpyInterfaceThread*)fn_803D6A98(), 0, (u8)i, 0);
            break;
        }
    }
    if (i >= 3 && getInstance_() != NULL) {
        info[0] = 0x80000007;
        info[1] = 0x5F;
        info[2] = 0x2D6A;
        getInstance_()->vtable->postError_288(getInstance_(), (NetworkErrorInfo*)info);
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
    if (fn_803D6A98() == NULL) {
        SIGNAL_LOG(3, lbl_806031D0);
        return;
    }
    for (i = 0; i < 3; i++) {
        if (socket == lbl_806D3650[i]) {
            fn_8041BD64((GameSpyInterfaceThread*)fn_803D6A98(), (u8)i, address, size);
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
    if (fn_803D6A98() == NULL) {
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
            fn_8041BDAC((GameSpyInterfaceThread*)fn_803D6A98(), error, 0, 0, (u8)i, -1);
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
extern "C" void fn_8041BAF4(GameSpyInterfaceThread* self, s32 limit)
{
    self->stage_98 = 0;
    self->handle_94 = limit;
}

/* Runs the NAS-login handshake the timed handler drives, one step per frame. */
extern "C" s32 fn_8041BB04(GameSpyInterfaceThread* self)
{
    s32 result;
    s32 error;

    switch (self->stage_98) {
    case 0:
        if (fn_8050A8A0() == 0) {
            self->stage_98 = 4;
            return 0;
        }
        self->stage_98 = self->stage_98 + 1;
        return 0;
    case 1:
        result = fn_8050A8D0();
        switch (result) {
        case 3:
            SIGNAL_LOG(3, lbl_80603320);
            self->stage_98 = self->stage_98 + 1;
            break;
        case 4:
            SIGNAL_LOG(3, lbl_80603334);
            self->stage_98 = 4;
            break;
        case 5:
            SIGNAL_LOG(3, lbl_80603348);
            fn_8041D1E4(self, 0x80000000, 0, 0);
            self->stage_98 = 4;
            break;
        default:
            break;
        }
        return 0;
    case 2:
        if (fn_8050A990() == 0) {
            fn_8050A9A0();
            self->stage_98 = 4;
        } else if (fn_8050A9B0(&lbl_80793994, self->handle_94) == 0) {
            fn_8050A9A0();
            self->stage_98 = 4;
        } else {
            self->stage_98 = self->stage_98 + 1;
        }
        return 0;
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
            self->stage_98 = 4;
            return 0;
        case 5:
            SIGNAL_LOG(3, lbl_80603378);
            fn_8050A9A0();
            self->stage_98 = 4;
            return 0;
        default:
            break;
        }
        return 0;
    case 4:
        fn_8041CEF4(self);
        return -1;
    default:
        break;
    }
    return 0;
}

/* Hands the index's receiver slot to the object stored for it (a tail call). */
extern "C" void fn_8041BD64(GameSpyInterfaceThread* self, u8 index, s32 a, s32 b)
{
    GameSpyReceiver* receiver;

    receiver = (GameSpyReceiver*)self->receivers_14[self->receiverState_24[index]];
    if (receiver == NULL) {
        return;
    }
    receiver->vtable->handle_18(receiver, a, b, 0, 0, 0);
}

/* Fails the index's request record when `error` is set, then closes the slot. */
extern "C" void fn_8041BDAC(GameSpyInterfaceThread* self, s32 error, s32 a, s32 b, u8 index, s32 e)
{
    if (error != 0) {
        fn_8041D1E4(self, 0x80000007, 0x5F, -error);
    }
    self->slotState_54[index] = 0xFF;
}

/* Records the outcome of the DWC request the interface is waiting on. */
extern "C" void fn_8041BE10(GameSpyInterfaceThread* self, s32 error, s32 value)
{
    self->running_6C = 2;
    if (error == 0) {
        self->value_30 = value;
        if (self->cancelPending_122 == 1) {
            self->state_2C = 2;
            return;
        }
        self->state_2C = 1;
        return;
    }
    fn_8041D1E4(self, 0x80000007, 0x5F, -error);
    fn_8041C3DC(self);
    self->state_2C = -2;
}

/* Publishes a completed request: rewrites the slot tables and the negotiation result. */
extern "C" void fn_8041BE94(GameSpyInterfaceThread* self, s32 error, u8 index, u32 value)
{
    u8 count;
    u8 i;
    u32 peer;

    if (error == 0) {
        count = self->receiverCount_28;
        self->receiverIds_34[count - 1] = self->value_30;
        self->slotState_54[count - 1] = 1;
        peer = value;
        if (peer == 0) {
            peer = self->peerId_4470;
        }
        for (i = 0; i < count; i++) {
            if (self->slotIds_44[i] == peer) {
                if (i != index) {
                    self->slotIds_44[i] = 0;
                    if (lbl_806D3650[i] != 0) {
                        fn_8050E250(lbl_806D3650[i]);
                    }
                }
                break;
            }
        }
        self->slotIds_44[index] = peer;
        self->slotState_54[index] = 1;
        self->receiverState_24[index] = 3;
        for (i = 0; i < count; i++) {
            if (self->receiverIds_34[i] == self->slotIds_44[index]) {
                self->receiverState_24[index] = i;
                break;
            }
        }
        self->negotiationDone_446E = 1;
        self->negotiationResult_446D = 1;
        return;
    }
    fn_8041D1E4(self, 0x80000007, 0x5F, -error);
    self->negotiationDone_446E = -1;
    self->negotiationResult_446D = 1;
}

/* Opens the GameSpy socket and installs the callback set, then applies the pending requests. */
extern "C" void fn_8041C010(GameSpyInterfaceThread* self)
{
    s8 addressEnd;
    char address[7];
    s32 state;
    s32 phase;

    state = 1;
    phase = self->state_2C;
    if (phase <= 0) {
        INFO_LOG(lbl_80603388, phase);
        self->phase_74 = -1;
        return;
    }
    snprintf(address, 7, lbl_80793998, self->bufferSize_4482);
    addressEnd = 0;
    if (fn_8050DEC0(&lbl_80794CE0, address, 0x2000, 0x2000, (NetworkCallback)fn_8041B194) == 0) {
        fn_8050E2A0(lbl_80794CE0, (NetworkCallback)fn_8041B270);
        fn_8050DF70(lbl_80794CE0, (NetworkCallback)fn_8041B538);
    } else {
        fn_8041D1E4(self, 0x80000007, 0x5F, 0x2DA2);
        state = 0;
    }
    if (state != 0 && self->closePending_123 != 0) {
        self->closePending_123 = 0;
        fn_8041C420(self);
        state = 0;
    }
    if (self->cancelPending_122 != 0) {
        self->cancelPending_122 = 0;
        fn_8041C3C8(self);
        return;
    }
    if (state != 0) {
        self->phase_74 = 1;
        return;
    }
    self->phase_74 = -1;
}

/* Starts a GameSpy match for `count` players and stores the peer id it was given. */
extern "C" s32 fn_8041C170(GameSpyInterfaceThread* self, s32 count, u32 value, s32 a, u16 b,
                           s32 c, s32 d, s32 e)
{
    s32 phase;

    phase = self->phase_74;
    if (phase > 0) {
        INFO_LOG(lbl_806033C8, phase);
        return -1;
    }
    if (count <= 1) {
        return -1;
    }
    if (self->state_2C <= 0) {
        return -1;
    }
    self->paramA_7C = a;
    self->paramB_80 = b;
    self->paramC_84 = c;
    self->paramD_88 = d;
    self->paramE_8C = e;
    snprintf(self->name_9C, 0x80, lbl_80603408, b);
    if (count > 4) {
        WARN_LOG(lbl_80603448, count, 4);
        self->receiverCount_28 = 4;
    } else {
        self->receiverCount_28 = (u8)count;
    }
    self->value_30 = value;
    self->idByte_11C = (s8)(value >> 24);
    self->idByte_11D = (u8)(value >> 16);
    self->idByte_11E = (u8)(value >> 8);
    self->idByte_11F = (u8)value;
    self->phase_74 = 2;
    self->openRequested_125 = 1;
    return 1;
}

/* Ticks the GameSpy clock and counts one more frame. */
extern "C" s32 fn_8041C2C0(GameSpyInterfaceThread* self)
{
    s32 time[8];

    fn_800E89D8();
    fn_804167B4(time);
    fn_8050C5F0((const void*)time[3]);
    self->frame_70 = self->frame_70 + 1;
    return 1;
}

/* Moves the interface into its running state, or reports the shutdown. */
extern "C" s32 fn_8041C30C(GameSpyInterfaceThread* self)
{
    if (self->field_68 < 0) {
        self->state_2C = -1;
        return -1;
    }
    if (self->running_6C != 0) {
        return 0;
    }
    self->state_2C = 0;
    self->value_30 = 0;
    self->frame_70 = 0;
    self->phase_74 = 0;
    self->initRequested_124 = 1;
    return 0;
}

/* Reports whether a close is already pending, arming the step flag if so. */
extern "C" u8 fn_8041C360(GameSpyInterfaceThread* self)
{
    if (self->closePending_123 != 0 || self->cancelPending_122 != 0) {
        self->stepRequested_126 = 1;
        return 1;
    }
    if (self->started_120 != 0) {
        self->stopRequested_121 = 1;
    }
    return self->started_120;
}

/* Returns the interface phase. */
extern "C" s32 fn_8041C3A4(GameSpyInterfaceThread* self)
{
    return self->phase_74;
}

/* Returns the running state, or -1 while the interface is not running. */
extern "C" s32 fn_8041C3AC(GameSpyInterfaceThread* self)
{
    if (self->running_6C != 0) {
        return self->state_2C;
    }
    return -1;
}

/* Clears the running state and the pending close. */
extern "C" void fn_8041C3C8(GameSpyInterfaceThread* self)
{
    self->running_6C = 0;
    self->state_2C = 0;
    self->phase_74 = 0;
}

/* Arms the cancellation when a close arrives while the request is still in flight. */
extern "C" void fn_8041C3DC(GameSpyInterfaceThread* self)
{
    if (self->field_68 < 0) {
        return;
    }
    if (self->running_6C == 0) {
        return;
    }
    if (self->started_120 == 0) {
        return;
    }
    self->cancelPending_122 = 1;
    if (self->state_2C == 1) {
        self->state_2C = 2;
    }
}

/* Tears the socket, the session and the request pool down under the interface mutex. */
extern "C" s32 fn_8041C420(GameSpyInterfaceThread* self)
{
    OSLockMutex(self->mutex_4450);
    if (self->sessionOpen_4484 != 0) {
        fn_805135E0(self->session_447C);
        fn_80512C50();
        self->sessionOpen_4484 = 0;
    }
    if (lbl_80794CE0 != 0) {
        fn_8050DED0(lbl_80794CE0);
        memset(lbl_806D3650, 0, 0x10);
        lbl_80794CE0 = 0;
    }
    OSUnlockMutex(self->mutex_4450);
    self->phase_74 = 0;
    return 0;
}

/* Reports whether the interface can be closed right now. */
extern "C" s32 fn_8041C4B4(GameSpyInterfaceThread* self)
{
    if (self->field_68 < 0) {
        return -1;
    }
    if (self->state_2C != 1) {
        return -1;
    }
    if (self->phase_74 <= 0) {
        return -1;
    }
    if (self->started_120 != 0) {
        self->closePending_123 = 1;
        return -2;
    }
    return 0;
}

/* Replies to a pending request by filling the first free return handle. */
extern "C" s32 fn_8041C514(GameSpyInterfaceThread* self, s32 handle)
{
    u8 count;
    u8 i;

    if (self->field_68 < 0) {
        return -1;
    }
    if (self->state_2C != 1) {
        return -1;
    }
    if (self->phase_74 <= 0) {
        return -1;
    }
    if (self->started_120 != 0) {
        count = self->receiverCount_28;
        for (i = 0; i < count; i++) {
            if (self->slotHandles_58[i] == 0) {
                self->slotHandles_58[i] = handle;
                break;
            }
        }
        return -2;
    }
    return 0;
}

/* Resets the slot tables and the negotiation state. */
extern "C" void fn_8041C5AC(GameSpyInterfaceThread* self)
{
    self->flag_78 = 0;
    memset(self->receivers_14, 0, 0x10);
    memset(self->receiverIds_34, 0, 0x10);
    memset(self->slotIds_44, 0, 0x10);
    memset(self->slotState_54, 0, 4);
    memset(self->slotHandles_58, 0, 0x10);
    self->negotiation_446C = 0;
    self->negotiationResult_446D = 0;
    self->negotiationDone_446E = 0;
    self->bufferSize_4482 = 0x2AF8;
    memset(self->receiverState_24, 3, 4);
    if (self->errorParam1_08 != 0x4B) {
        fn_8041D1A4(self);
    }
}

/* Constructs the interface thread and starts its worker thread. */
extern "C" void* fn_8041C66C(GameSpyInterfaceThread* self)
{
    self->vtable_00 = lbl_806036A0;
    lbl_80794CE4 = self;
    self->field_68 = 0;
    self->running_6C = 0;
    self->frame_70 = 0;
    self->value_30 = 0;
    fn_8041C790(self);
    fn_8041D1A4(self);
    self->phase_74 = 0;
    self->started_120 = 0;
    self->initRequested_124 = 0;
    self->openRequested_125 = 0;
    self->stepRequested_126 = 0;
    self->result_90 = 0;
    self->stopRequested_121 = 0;
    self->mutexReady_444C = 0;
    memset(self->mutex_4450, 0, 0x18);
    startThread(self);
    memset(lbl_806D3650, 0, 0x10);
    lbl_80794CE0 = 0;
    self->sessionOpen_4484 = 0;
    return self;
}

/* Deleting destructor: restores the base vtable, empties the singleton and frees on request. */
extern "C" void* fn_8041C724(GameSpyInterfaceThread* self, s16 flags)
{
    if (self != NULL) {
        self->vtable_00 = lbl_806036A0;
        fn_8041C848(self);
        lbl_80794CE4 = NULL;
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Resets the per-request tables and the pending-request flags. */
extern "C" void fn_8041C790(GameSpyInterfaceThread* self)
{
    self->flag_78 = 0;
    memset(self->receivers_14, 0, 0x10);
    memset(self->receiverIds_34, 0, 0x10);
    memset(self->slotIds_44, 0, 0x10);
    memset(self->slotState_54, 0, 4);
    memset(self->slotHandles_58, 0, 0x10);
    self->receiverCount_28 = 0;
    self->state_2C = 0;
    self->cancelPending_122 = 0;
    self->closePending_123 = 0;
    memset(&self->busy_128, 0, 4);
    self->field_4468 = 0;
    self->negotiation_446C = 0;
    self->negotiationResult_446D = 0;
    self->negotiationDone_446E = 0;
}

/* Empty body: the interface thread's vtable cleanup hook. */
extern "C" void fn_8041C848(GameSpyInterfaceThread* self)
{
}

/* Registers a receiver for `id` in the first free slot. */
extern "C" u8 fn_8041C84C(GameSpyInterfaceThread* self, void* receiver, u32 id)
{
    u8 slot;
    u8 i;

    if (receiver == NULL) {
        return 0xFF;
    }
    for (slot = 0; slot < self->receiverCount_28; slot++) {
        if (self->receivers_14[slot] == 0) {
            break;
        }
    }
    if (slot >= self->receiverCount_28) {
        return 0xFE;
    }
    self->receivers_14[slot] = (u32)receiver;
    self->receiverIds_34[slot] = id;
    for (i = 0; i < self->receiverCount_28; i++) {
        if (id == self->slotHandles_58[i]) {
            break;
        }
    }
    if (i >= self->receiverCount_28) {
        for (i = 0; i < self->receiverCount_28; i++) {
            if (id == self->slotIds_44[i]) {
                self->receiverState_24[i] = slot;
                break;
            }
        }
    }
    return slot;
}

/* Releases the receiver slot at `index` and clears its id. */
extern "C" void fn_8041C938(GameSpyInterfaceThread* self, s32 index)
{
    u8 count;
    u8 i;

    fn_8041C514(self, (s32)self->receiverIds_34[index]);
    count = self->receiverCount_28;
    for (i = 0; i < count; i++) {
        if (index == self->receiverState_24[i]) {
            self->receiverState_24[i] = 3;
            break;
        }
    }
    self->receivers_14[index] = 0;
    self->receiverIds_34[index] = 0;
}

/* Returns the state of the slot `id` maps to, or the pending negotiation result. */
extern "C" u8 fn_8041C9D8(GameSpyInterfaceThread* self, u32 id)
{
    u8 i;
    u8 count;

    count = self->receiverCount_28;
    for (i = 0; i < count; i++) {
        if (self->slotIds_44[i] == id) {
            return self->slotState_54[i];
        }
    }
    if (self->negotiationResult_446D != 0 && id != self->value_30 &&
        (id == self->peerId_4470 || id == self->selfPeerId_4474)) {
        return (u8)self->negotiationDone_446E;
    }
    return 0;
}

/* Returns the slot index `id` maps to, or -1. */
extern "C" s8 fn_8041CA58(GameSpyInterfaceThread* self, u32 id)
{
    s8 i;

    for (i = 0; i < self->receiverCount_28; i++) {
        if (self->slotIds_44[i] == id) {
            return i;
        }
    }
    return -1;
}

/* Runs the interface's per-frame step: the negotiation sub-machine, the request sweep and the
   close/cancel bookkeeping. */
extern "C" void fn_8041CA94(GameSpyInterfaceThread* self)
{
    u32 info[3];
    s32 r;
    s32 error;
    s32 i;
    s8 slot;

    if (self->running_6C != 0 && self->state_2C >= 0) {
        OSLockMutex(self->mutex_4450);
        if (self->frame_70 == 1 && self->state_2C == 0) {
            r = fn_8050C770();
            if (r != 0) {
                if (r == 1) {
                    fn_8041BE10(self, 0, 0);
                } else {
                    fn_8041BE10(self, -0x2DA1, 0);
                }
            }
        }
        if (self->field_68 == 0 && self->state_2C == 1 && self->phase_74 == 1 && lbl_80794CE0 != 0) {
            if (self->negotiation_446C != 0) {
                switch (self->negotiationStep_4480) {
                case 0:
                    self->peerMatch_4478 = (self->value_30 != self->peerId_4470);
                    if (lbl_806D3660.result_04 == 0) {
                        self->session_447C = self->peerId_4470 ^ self->selfPeerId_4474;
                        fn_8050E290(lbl_80794CE0);
                        r = fn_805132C0(self->session_447C, self->peerMatch_4478, (NetworkCallback)fn_8041B26C,
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
                            fn_8041D1E4(self, 0x80000007, 0x5F, -error);
                        }
                        self->sessionOpen_4484 = 1;
                        self->negotiationStep_4480 = 1;
                    } else {
                        self->negotiationStep_4480 = 2;
                    }
                    break;
                case 1:
                    if (lbl_806D3660.result_04 != 0) {
                        fn_80512C50();
                        self->sessionOpen_4484 = 0;
                        self->negotiationStep_4480 = 2;
                    }
                    break;
                case 2:
                    if (lbl_806D3660.active_00 != 0) {
                        if (self->peerMatch_4478 == 1) {
                            if (fn_8050DFA0(lbl_80794CE0, info,
                                            fn_80512210(lbl_806D3660.session_0C,
                                                        lbl_806D3660.encoded_0A, NULL),
                                            self->profile_4485, 0x14, 0x2710, lbl_806031A0, 0) == 0) {
                                self->negotiationStep_4480 = 3;
                                break;
                            }
                            fn_8041D1E4(self, 0x80000007, 0x5F, 0x2DAD);
                        }
                        self->negotiationDone_446E = -1;
                        self->negotiationResult_446D = 1;
                    } else {
                        self->negotiationDone_446E = -1;
                        self->negotiationResult_446D = 1;
                    }
                    self->negotiation_446C = 0;
                    break;
                case 3:
                    if (self->negotiationDone_446E != 0) {
                        self->negotiation_446C = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            fn_80513BB0();
            fn_8050DF20(lbl_80794CE0);
        }
        OSUnlockMutex(self->mutex_4450);
    }
    if (self->closePending_123 != 0) {
        self->closePending_123 = 0;
        if (self->field_68 == 0 && self->state_2C >= 1 && self->phase_74 > 0) {
            fn_8041C420(self);
        }
    }
    for (i = 0; i < self->receiverCount_28; i++) {
        if (self->slotHandles_58[i] != 0) {
            slot = fn_8041CA58(self, self->slotHandles_58[i]);
            if (slot < 0) {
                self->slotHandles_58[i] = 0;
            } else {
                if (lbl_806D3650[slot] != 0) {
                    fn_8050E250(lbl_806D3650[slot]);
                }
                self->slotIds_44[slot] = 0;
                self->slotState_54[slot] = 0;
                self->slotHandles_58[i] = 0;
            }
        }
    }
    if (self->cancelPending_122 != 0) {
        if (self->field_68 == 0) {
            if (self->running_6C != 0) {
                if (self->frame_70 == 1 && (self->state_2C == 2 || self->state_2C == -2)) {
                    self->cancelPending_122 = 0;
                    fn_8041C3C8(self);
                }
            } else {
                self->cancelPending_122 = 0;
            }
        } else {
            self->cancelPending_122 = 0;
        }
    }
}

/* Returns the request result, arming the step flag first. */
extern "C" s32 fn_8041CED0(GameSpyInterfaceThread* self)
{
    if (self->field_68 < 0) {
        return -1;
    }
    self->stepRequested_126 = 1;
    return self->result_90;
}

/* Drains the DWC error queue and folds the reported type into the interface state. */
extern "C" s32 fn_8041CEF4(GameSpyInterfaceThread* self)
{
    s32 code;
    s32 type;

    if (fn_805073C0(&code, &type) != 0 && code < 0 && type != 0) {
        INFO_LOG(lbl_806034A0, code, type);
        switch (type) {
        case 1:
            fn_8041D1E4(self, 0x80000007, 0x4A, -code);
            fn_80507470();
            break;
        case 2:
            fn_8041D1E4(self, 0x80000007, 0x49, -code);
            fn_80507470();
            break;
        case 3:
            fn_8041D1E4(self, 0x80000007, 0x49, -code);
            self->errorReported_10 = 0;
            self->state_2C = code;
            if (self->running_6C != 0) {
                if (self->sessionOpen_4484 != 0) {
                    fn_805135E0(self->session_447C);
                    fn_80512C50();
                    self->sessionOpen_4484 = 0;
                }
                if (lbl_80794CE0 != 0) {
                    fn_8050DED0(lbl_80794CE0);
                    memset(lbl_806D3650, 0, 0x10);
                    lbl_80794CE0 = 0;
                }
                self->running_6C = 0;
            }
            self->phase_74 = 0;
            fn_80507470();
            break;
        case 6:
            fn_8041D1E4(self, 0x80000007, 0x49, -code);
            self->errorReported_10 = 0;
            self->field_68 = -1;
            self->state_2C = code;
            if (self->running_6C != 0) {
                if (self->sessionOpen_4484 != 0) {
                    fn_805135E0(self->session_447C);
                    fn_80512C50();
                    self->sessionOpen_4484 = 0;
                }
                if (lbl_80794CE0 != 0) {
                    fn_8050DED0(lbl_80794CE0);
                    memset(lbl_806D3650, 0, 0x10);
                    lbl_80794CE0 = 0;
                }
                self->running_6C = 0;
            }
            self->phase_74 = 0;
            fn_80507470();
            break;
        case 7:
            self->errorReported_10 = 1;
            fn_8041D1E4(self, 0x80000007, 0x4B, -code);
            self->errorReported_10 = 0;
            self->field_68 = -1;
            self->state_2C = code;
            break;
        default:
            break;
        }
        return type;
    }
    return 0;
}

/* Copies the interface's error record out for the game to read. */
extern "C" void getErrorStruct(GameSpyInterfaceThread* self, NetworkErrorInfo* out)
{
    if (out != NULL) {
        out->code_00 = self->errorCode_04;
        out->param1_04 = self->errorParam1_08;
        out->param2_08 = self->errorParam2_0C;
    }
}

/* Clears the error record and marks it reported. */
extern "C" void fn_8041D1A4(GameSpyInterfaceThread* self)
{
    memset(&self->errorCode_04, 0, 0xC);
    self->errorReported_10 = 1;
}

/* Stores the first error the interface sees, unless it has already been reported. */
extern "C" void fn_8041D1E4(GameSpyInterfaceThread* self, s32 code, s32 a, s32 b)
{
    if (self->errorCode_04 == 0 || self->errorReported_10 != 0) {
        self->errorCode_04 = code;
        self->errorParam1_08 = a;
        self->errorParam2_0C = b;
    }
}

/* Sends a buffer out over the socket the index maps to. */
extern "C" s32 fn_8041D20C(GameSpyInterfaceThread* self, u8 index, const void* data, s32 size)
{
    s32 sent;

    sent = 0;
    if (self->field_68 < 0 || self->state_2C != 1 || self->phase_74 <= 0) {
        INFO_LOG(lbl_806034F4, size);
    } else {
        if (self->mutexReady_444C != 0) {
            OSLockMutex(self->mutex_4450);
            if (lbl_806D3650[index] != 0) {
                sent = size;
                fn_8050E150(lbl_806D3650[index], data, size, 0);
            }
            OSUnlockMutex(self->mutex_4450);
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
extern "C" void fn_8041D344(GameSpyInterfaceThread* self, s32 arg)
{
    OSInitMutex(self->mutex_4450);
    self->mutexReady_444C = 1;
    for (;;) {
        if (self->initRequested_124 != 0) {
            self->running_6C = 1;
            fn_8041C2C0(self);
            self->initRequested_124 = 0;
        } else if (self->openRequested_125 != 0) {
            fn_8041C010(self);
            self->openRequested_125 = 0;
        } else if (self->stepRequested_126 != 0) {
            fn_8041CA94(self);
            self->stepRequested_126 = 0;
        }
        if (self->stopRequested_121 != 0) {
            self->stopRequested_121 = 0;
            self->started_120 = 0;
            break;
        }
        OSSleepTicks((u64)(*(volatile u32*)0x800000F8 / 4 / 1000) * 17);
    }
    self->mutexReady_444C = 0;
    INFO_LOG(lbl_80603548);
}

/* The worker thread's entry point. */
extern "C" s32 runThread(void* self)
{
    fn_8041D344((GameSpyInterfaceThread*)self, (s32)self);
    return 0;
}

/* Spawns the interface's worker thread. */
extern "C" s32 startThread(GameSpyInterfaceThread* self)
{
    s32 thread;

    thread = OSCreateThread(self->thread_130, runThread, self, &self->threadParam_4448, 0x4000, 0x0E, 1);
    if (thread != 0) {
        self->started_120 = 1;
        self->threadParam_4448 = 0;
        OSResumeThread(self->thread_130);
        INFO_LOG(lbl_80603590);
    } else {
        INFO_LOG(lbl_806035E4);
    }
    return thread;
}

/* Sets the socket address buffer size. */
extern "C" void fn_8041D528(GameSpyInterfaceThread* self, s16 size)
{
    self->bufferSize_4482 = size;
}

/* Starts a NAT negotiation between the two peer ids. */
extern "C" void fn_8041D530(GameSpyInterfaceThread* self, const GameSpyPeerId* a, const GameSpyPeerId* b)
{
    s32 i;

    if (a == NULL || b == NULL || self->negotiation_446C != 0) {
        return;
    }
    self->negotiationResult_446D = 0;
    self->negotiationDone_446E = 0;
    self->peerId_4470 = a->peerId_00;
    self->selfPeerId_4474 = b->peerId_00;
    lbl_806D3660.result_04 = 0;
    if (a->mode_04 == b->mode_04) {
        lbl_806D3660.result_04 = 1;
        lbl_806D3660.active_00 = 1;
        lbl_806D3660.session_0C = a->session_08;
        {
            NetworkLogger* lm = fn_803C9974();
            lbl_806D3660.encoded_0A = lm->vtable->encode_4C(lm, a->port_0C);
        }
    }
    self->profile_4485[0] = (u8)self->selfPeerId_4474;
    self->profile_4485[4] = (u8)self->paramA_7C;
    self->profile_4485[8] = (u8)self->paramB_80;
    self->profile_4485[0x0C] = (u8)self->paramC_84;
    self->profile_4485[0x10] = (u8)self->paramD_88;
    self->negotiationStep_4480 = 0;
    self->negotiation_446C = 1;
}

/* Reports whether a negotiation is running. */
extern "C" u8 fn_8041D628(GameSpyInterfaceThread* self)
{
    return self->negotiation_446C;
}

/* Reports the negotiation's outcome. */
extern "C" u8 fn_8041D630(GameSpyInterfaceThread* self)
{
    return (u8)self->negotiationDone_446E;
}

/* Compares a received peer profile against the one this interface published. */
extern "C" s32 fn_8041D638(GameSpyInterfaceThread* self, const void* profile, u32 size)
{
    if (size == 0x14) {
        if (memcmp(profile, self->profile_4485, 0x14) == 0) {
            return 1;
        }
        if (memcmp(profile, self->profile_4485, 4) != 0) {
            SIGNAL_LOG(3, lbl_80603638);
        }
        if (memcmp((const u8*)profile + 4, self->profile_4485 + 4, 0x10) != 0) {
            SIGNAL_LOG(3, lbl_80603660);
        }
        return 0;
    }
    SIGNAL_LOG(3, lbl_80603680, size);
    return 0;
}

/* Returns this interface's own peer id. */
extern "C" u32 fn_8041D748(GameSpyInterfaceThread* self)
{
    return self->selfPeerId_4474;
}

/* Binds the peer to the interface and resets both buffers. */
extern "C" void fn_8041D750(NetworkPeerGameSpy* self, const u32* id)
{
    self->interface_6634 = (GameSpyInterfaceThread*)id[0];
    self->peer_6638 = id[1];
    self->field_6630 = (s32)fn_8041C84C(self->interface_6634, self, self->peer_6638);
    memset(self->sendBuffer_14, 0, 0x600);
    memset(self->recvBuffer_614, 0, 0x6000);
    self->received_10 = 0;
}

/* Builds and sends a framed peer message from the two optional payloads. */
extern "C" s32 fn_8041D7C0(NetworkPeerGameSpy* self, const u16* a, s32 aLen, const u16* b, s32 bLen,
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
    if (fn_8041C9D8(self->interface_6634, self->peer_6638) < 0) {
        fn_803CCF14(self, lbl_806036A0, 0, -1);
        return -1;
    }
    record = self->sendBuffer_14;
    if (a == NULL || aLen <= 0) {
        aLen16 = 0;
        memcpy(record, &aLen16, 2);
        cursor = record + 2;
        total = 2;
    } else {
        {
            NetworkLogger* lm = fn_803C9974();
            aLen16 = lm->vtable->encode_4C(lm, (u16)aLen);
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
            NetworkLogger* lm = fn_803C9974();
            bLen16 = lm->vtable->encode_4C(lm, (u16)(bLen + 1));
        }
        memcpy(cursor, &bLen16, 2);
        memcpy(cursor + 2, &flagByte, 1);
        memcpy(cursor + 3, b, (u32)(u16)bLen);
        total = total + 3 + bLen;
    }
    {
        NetworkLogger* lm = fn_803C9974();
        if (lm->vtable->flag_48(lm, aLen16) == 0) {
            NetworkLogger* lm2 = fn_803C9974();
            if (lm2->vtable->flag_48(lm2, bLen16) == 0) {
                return 0;
            }
        }
    }
    slot = fn_8041CA58(self->interface_6634, self->peer_6638);
    if (slot < 0) {
        fn_803CCF14(self, lbl_806036A0, 0, -2);
        return -1;
    }
    if (fn_8041D20C(self->interface_6634, (u8)slot, self->sendBuffer_14, total) == 0) {
        fn_803CCF14(self, lbl_806036A0, 0, -3);
        return -1;
    }
    return total;
}

/* Pulls one framed message out of the peer's receive buffer. */
extern "C" s32 fn_8041DA10(NetworkPeerGameSpy* self, void* a, s32* aLen, void* b, s32* bLen,
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
    if (fn_8041C9D8(self->interface_6634, self->peer_6638) < 0) {
        fn_803CCF14(self, lbl_806036A0, 0, 0);
        return -1;
    }
    if (self->received_10 < 4) {
        return 0;
    }
    LockMutex(self->mutex_6614);
    cursor = self->recvBuffer_614;
    memcpy(&aLen16, cursor, 2);
    {
        NetworkLogger* lm = fn_803C9974();
        aLen16 = (u16)lm->vtable->flag_48(lm, aLen16);
    }
    bLen16 = 0;
    if (self->received_10 >= (u32)aLen16 + 4) {
        memcpy(&bLen16, cursor + aLen16 + 2, 2);
        {
            NetworkLogger* lm = fn_803C9974();
            payload = (u16)lm->vtable->flag_48(lm, bLen16);
        }
        bLen16 = payload;
        cursor = self->recvBuffer_614;
    }
    if (self->received_10 < (u32)(aLen16 + payload + 4)) {
        UnlockMutex(self->mutex_6614);
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
    self->received_10 = self->received_10 - consumed;
    if (self->received_10 != 0) {
        memmove(self->recvBuffer_614, cursor, self->received_10);
    }
    UnlockMutex(self->mutex_6614);
    return consumed;
}

/* Appends a buffer to the peer's receive queue. */
extern "C" s32 fn_8041DC28(NetworkPeerGameSpy* self, const void* data, u32 size)
{
    LockMutex(self->mutex_6614);
    if (self->received_10 + size > 0x6000) {
        INFO_LOG(lbl_806036B0);
        UnlockMutex(self->mutex_6614);
        return -1;
    }
    memcpy(self->recvBuffer_614 + self->received_10, data, size);
    self->received_10 = self->received_10 + size;
    UnlockMutex(self->mutex_6614);
    return 0;
}

/* Reports whether the peer has a message queued. */
extern "C" s32 fn_8041DCDC(NetworkPeerGameSpy* self)
{
    s8 state;

    state = fn_8041C9D8(self->interface_6634, self->peer_6638);
    if (state > 0) {
        return 1;
    }
    return state & (state >> 31);
}

/* Empty body: the peer's vtable placeholder. */
extern "C" void fn_8041DD24(void)
{
}

/* Ticks the callback object's virtual slot. */
extern "C" s32 fn_8041DD28(NetworkPeerCallbackVtable** self)
{
    (*self)->tick_28(*self);
    return 1;
}

/* Releases the peer's interface slot and drops its receive queue. */
extern "C" void fn_8041DD58(NetworkPeerGameSpy* self)
{
    fn_8041C938(self->interface_6634, self->field_6630);
    LockMutex(self->mutex_6614);
    memset(self->recvBuffer_614, 0, 0x6000);
    self->received_10 = 0;
    UnlockMutex(self->mutex_6614);
}

/* Deleting destructor: destroys the queue mutex and the base, then frees on request. */
extern "C" void* fn_8041DDB4(NetworkPeerGameSpy* self, s16 flags)
{
    if (self != NULL) {
        dtor_803CA338(self->mutex_6614, -1);
        dtor_803CCE9C(self, 0);
        if (flags > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* Constructs the timed handler. */
extern "C" void* fn_8041DE20(NetworkTimedHandler* self)
{
    self->vtable_00 = lbl_80603740;
    fn_8041DEA8(self, (s32)lbl_80603740, 0, 0);
    return self;
}

/* Deleting destructor for the timed handler. */
extern "C" void* fn_8041DE64(NetworkTimedHandler* self, s16 flags)
{
    if (self != NULL && flags > 0) {
        operator delete(self);
    }
    return self;
}

/* Initialises the timed handler's interval, limit and timeout. */
extern "C" void fn_8041DEA8(NetworkTimedHandler* self, s32 a, s32 b, s32 c)
{
    fn_8041DEFC(self);
    self->timeout_14 = 1000;
    self->limit_0C = c;
    self->interval_08 = b;
}

/* Clears the timed handler's state, ready flag and expiry flag. */
extern "C" void fn_8041DEFC(NetworkTimedHandler* self)
{
    self->state_04 = 0;
    self->ready_10 = 0;
    self->expired_18 = 0;
}
