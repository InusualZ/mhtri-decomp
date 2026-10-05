/*
 * Network/NetworkSessionManagerPat.cpp - `NetworkSessionManagerPat`: its key function `move` (so the class table is
 *   emitted here), the request handlers and callbacks, the circle list and player records, and
 *   `initNetworkSessionStable`, the NetworkSessionStable opener.
 * RANGE. .text 0x803D70B8-0x803DF2EC (106 functions); .data 0x805FAB48-0x805FB2B8 (the Pat strings, the jump tables and
 *   `__vt__24NetworkSessionManagerPat` 0x805FB0F0), .sdata 0x80793930-0x80793940 (the session's three timeouts), .sdata2
 *   0x8079C758-0x8079C778, extab, extabindex.  The right edge is the `.data` V->S seam at 0x805FB2B8.
 * FLAGS. The game-root `cflags_main` (configure.py; docs/network.md); file-scope `#pragma peephole off`
 *   from `move` on (retail keeps `lwz r12,0(r3)` after the `mr r3,r31` copy; it also closes `slot_1B8`), on again only
 *   around `getTimeSincePublish`.
 * NAMES. `moveStartSession` and `matchPhase_66C` are its log string's ("moveStartSession::mMatchPhase(0) NG").  GUESSes
 *   from the bodies: `handleCircleLeave`, the chat/leave/log helpers, every message record view, `setCircleComment`
 *   (0x803DF0C8, copies <= 144 chars to +0xA54), `setCircleMode` (0x803DF144), `post` (0x803DF180), the limit/used
 *   counters, `setCircleInfo`/`addCircleInfo`/`removeCircleInfo`, the `*PlayerRecord` family, `packCircleOptions`.
 *   Where a body identifies nothing, an override is named for the vtable offset it fills (`slot_068`, as `slot_13C` in
 *   the base).
 * RESIDUALS. `networkPatReleaseBuffer`: unwritten (232 B; deletes the Tcp/Udp objects through the virtual destructor).
 *  - `isNetworkSessionManagerPatReady`, `slot_19C`, `getTimeSincePublish`: `NetworkSessionBase::getUserFlagB` (+0x84,
 *    `Network/network_transport_types.h`) returns `u8`, so our callers re-extend it (`clrlwi.`) where retail uses the
 *    full word (`cmpwi r3,0`, a plain `bctr` tail call);
 *  - `networkSessionReflect1`, `moveStartSession`: one callee-saved register colouring each;
 *  - `networkSessionReflect0`: the event-code select's scheduling;
 *  - `setCircleInfo`: the option's enable byte compares with `cmplwi` where retail has `cmpwi` (u8, s8, bool tried);
 *  - `packCircleOptions`: the name entries are addressed from the list base where retail strength-reduces a pointer at
 *    +0x08 (the explicit pointer spelling swaps two registers);
 *  - `updatePlayerRecord`: not characterised (the objdiff row);
 *  - `.sdata2`: the pool entries pair by address, not by name (anonymous `@NNN` against the map's `lbl_8079C758`;
 *    naming one would claim a symbol the original did not have, playbook 58);
 *  - `.sdata` 0xC against 0x10; `flipcheck` reads both small-data pools as a partial pool of a TU spanning several
 *    units (fold candidate with the session/transport units, low confidence).
 * SHAPES. `Network/NetworkSessionManager.h` declares `move` first so it is the key function; the table is compiler
 *   output with one override per filled slot (`python tools/units/vtableaudit.py --at 0x805FB0F8 --json` regenerates
 *   the 112-row census; the `.data` offsets are the slot offsets).  The `+0x03C` slot is the empty `setFlag79` override
 *   at 0x803DF0C4 (`void f() {}` is exactly `blr`).
 *  - `move`: the error record is the 12-byte signed `NetworkErrorInfo` view passed by value (`NetworkPostedError`; 0x10
 *    lowers ten rows), the publish test is `1.0f + last < now`, the record count the `>= 256 ? 256 : n` ternary, the
 *    mode byte signed `set ? 1 : 2`;
 *  - every handler is one state machine over `NetworkRequest::state_00` (0 start, its own waits, 100 cancelled, 110
 *    failed); `requestFlags_30C[i]`/`requestIds_360[i]` are the reply bits and ids `networkSessionReflect1` sets;
 *  - `32 <= id` keeps retail's two signed compares (`id >= 32` merges them into one `cmplwi`); `-(!x)` gives retail's
 *    `cntlzw`/`srwi`/`neg` where `-(x == 0)` gives `extrwi`;
 *  - the stack copies are real `NetworkUniqueId`/`NetworkSessionSlotInfo` locals declared in the block retail constructs
 *    them in (MWCC's scope-exit destructors and cleanup records); every +0x28 `copyFrom` is a virtual call, its slot
 *    still typed `const u8*` by the sink's declaration, so each site casts;
 *  - the callback at +0x04 is called through a cast (`init__21NetworkSessionManagerFUlUl` fixes the base's parameter
 *    list); the base's +0x0C `NetworkBuffer*` field holds a `NetworkSessionBase` and is read through a cast;
 *    `getNetworkBinaryState` (an owner-header `u32`) is narrowed with `(u8)` at each call;
 *  - `initNetworkSessionStable` is C++ (`__nw__FUl`, five virtual calls) with C linkage; its `new` expression gives the
 *    target's 24-byte extab record; `getOwnIndex` is declared `s32` so the call site narrows with `extsb` as retail.
 *  - `updateSession`'s error code 0x80060034 is a `config.yml` relocation block.
 */

#include "Network/NetworkSessionManager.h"
#include "unsplit/Network.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL/strlen.h"
#include "types.h"
#include "sys_mem.h"
#include "Network/NetworkSessionManagerPat.h"   /* the unit's own header: its free functions and constants (rule 2) */
#include "Network/NetworkLayerPat.h"             /* NetworkLayerPat - the layer the manager drives */
#include "Network/NetworkUniqueId.h"            /* NetworkUniqueId - owner Network/NetworkUniqueId.cpp */
#include "Network/NetworkSessionBase.h"          /* LockMutex/UnlockMutex - owner Network/NetworkSessionBase.cpp */
#include "Network/gamespy_interface_types.h"    /* GameSpyInterfaceThread / NetworkErrorInfo - owner Network/GameSpyInterfaceThread.cpp */
#include "Network/PatInterface.h"              /* PatInterface - owner Network/PatInterface.cpp */
#include "Network/NetworkWiiMediator.h"         /* NetworkWiiMediator::pushTransferRecord - owner Network/NetworkWiiMediator.cpp */
#include "Network/sNetworkLibraryWii.h"         /* sNetworkPatInstance - owner Network/sNetworkLibraryWii.cpp */
#include "enemy/em020_ai.h"                     /* getInstance_ - owner enemy/em020_ai.cpp */
#include "MSL_C/alloc.h"                        /* rand - owner MSL_C/alloc.cpp */
#include "sound/fn_800E46E8.h"                  /* getInstance (the mediator singleton) - owner sound/fn_800E46E8.cpp */


/* The steps every request handler shares: 0 starts the request, 100 reports a cancelled one, 110 a failed
 * one (which also raises the session error, event 3); the steps between are each handler's own waits. */
enum {
    REQUEST_START = 0,
    REQUEST_CANCELLED = 100,
    REQUEST_FAILED = 110
};

/* `requestFlags_30C` bits every handler tests first. */
enum {
    REQUEST_SESSION_LOST = 0x1,
    REQUEST_ABORTED = 0x2
};

/* The callback word the base stores at +0x04 and `move` runs: six arguments, the error record it was
 * handed as the fifth and the base's second word as the sixth.  The typedef exists only because the
 * base's field is a `u32` (see the file header) - this is the original's own cast. */
/* The session's three timeouts (`.sdata`, this unit's): `initNetworkSessionStable` hands them to a new session,
 * the circle block received in `networkSessionReflect1` may replace them and `networkPatAttachBuffer` puts
 * them back. */
f32 networkSessionTimeoutSeconds = 48.0f;
f32 networkSessionIntervalSeconds = 60.0f;
f32 networkSessionLimitSeconds = 60.0f;

/* The record `connectPeer` hands the session with a new slot: the GameSpy thread and the caller's word. */
typedef struct PatPeerConnect {
    /* +0x00 */ GameSpyInterfaceThread* thread_00;
    /* +0x04 */ u32 value_04;
} PatPeerConnect;   /* size: 0x8 */

/* The server messages `networkSessionReflect1` receives (GUESS on every name: the layouts are the offsets the
 * handler reads).  A circle record is the circle block followed by its option list. */
typedef struct PatCircleRecord {
    /* +0x000 */ PatCircleInfo info_000;
    /* +0x37C */ PatCircleOptionList options_37C;
} PatCircleRecord;   /* size: 0x480 (the stride of the circle list messages) */

/* A player joined or left the circle (0x8045 / 0x8047). */
typedef struct PatPlayerNotice {
    /* +0x00 */ u32 circleId_00;
    /* +0x04 */ s8 slot_04;
    /* +0x05 */ u8 kind_05;          /* 2 = a counted player */
    /* +0x06 */ u8 address_06[0x08]; /* its first byte is non-zero for a valid notice */
    /* +0x0E */ char name_0E[0x14];
} PatPlayerNotice;   /* size: 0x24 (approximation - the last field read is the name) */

/* One entry of the member list (0x805E) and the member state notice (0x804B). */
typedef struct PatMemberEntry {
    /* +0x00 */ u8 pad_00[0x06];
    /* +0x06 */ s8 state_06;
    /* +0x07 */ s8 slot_07;
    /* +0x08 */ u8 address_08[0x08];
    /* +0x10 */ char name_10[0x14];
    /* +0x24 */ u8 pad_24[0x0C];
} PatMemberEntry;   /* size: 0x30 */

/* One match member (0x804E): its peer address and slot. */
typedef struct PatMatchMember {
    /* +0x00 */ NetworkPeerAddress address_00;
    /* +0x06 */ u8 pad_06;
    /* +0x07 */ s8 slot_07;
    /* +0x08 */ u8 pad_08[0x28];
} PatMatchMember;   /* size: 0x30 */

/* The match start notice (0x804E): the member array, the match kind and the three session timeouts. */
typedef struct PatMatchNotice {
    /* +0x00 */ u32 circleId_00;
    /* +0x04 */ PatMatchMember* members_04;
    /* +0x08 */ u8 kind_08;          /* 1 = a lone member is not a match */
    /* +0x09 */ u8 pad_09[0x03];
    /* +0x0C */ u32 timeout_0C;      /* 16..80 s */
    /* +0x10 */ u32 interval_10;     /* 20..100 s */
    /* +0x14 */ u32 pad_14;
    /* +0x18 */ u32 limit_18;        /* 20..100 s */
} PatMatchNotice;   /* size: 0x1C */

/* One field of a binary notice (0x805F). */
typedef struct PatBinaryField {
    /* +0x00 */ u8 type_00;          /* 1 */
    /* +0x01 */ u8 pad_01[0x07];
    /* +0x08 */ u8 kind_08;          /* 1 value, 2 value + reply, 3 link state */
    /* +0x09 */ u8 pad_09[0x07];
    /* +0x10 */ u8 type_10;          /* 2 */
    /* +0x11 */ u8 pad_11[0x07];
    /* +0x18 */ u32 value_18;
} PatBinaryField;   /* size: 0x1C */

/* The binary notice: the sender's 8-byte address first, then its fields. */
typedef struct PatBinaryNotice {
    /* +0x00 */ u8 address_00[0x08]; /* its first byte is non-zero for a valid notice */
    /* +0x08 */ u32 pad_08;
    /* +0x0C */ PatBinaryField* fields_0C;
} PatBinaryNotice;   /* size: 0x10 */

/* A chat message relayed by the server (0x8060 / 0x8062). */
typedef struct PatServerChat {
    /* +0x000 */ char text_000[0x100];
    /* +0x100 */ u32 tag_100;
    /* +0x104 */ s32 time_104;
    /* +0x108 */ u8 address_108[0x08];
    /* +0x110 */ char name_110[0x13];
} PatServerChat;   /* size: 0x124 (approximation - the last field read is the name) */

/* The payload of event 37 (`slot_190`): an enable word and a time (GUESS on both names: the body stores 1
 * and `networkRequestTimerReset`). */
typedef struct PatEventTimer {
    /* +0x00 */ s32 enabled_00;
    /* +0x04 */ f32 time_04;
} PatEventTimer;   /* size: 0x8 */

#pragma peephole off
typedef void (*PatErrorCallback)(u32, u32, u32, u32, NetworkErrorInfo*, u32);

/* One frame of the Pat layer, in retail's order: pump the two channels, hand a finished GameSpy error to the session,
 * run the base's `move`, then publish the pending circle records and the session name once a second has passed. */
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
                if (info.param1_04 == 0x4B) {
                    ((PatErrorCallback)this->unused_04)(3, 0, info.code_00, 1, &info, this->unused_08);
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

/* Logs this console in (event 1): the account check, the reflect callback, the login, FMP and binary stages (each
 * ended with a shut request), the circle notice settings, then this console's ids, a random session key and the
 * hunter name. */
s32 NetworkSessionManagerPat::updateSession(NetworkRequest* request)
{
    NetworkFmpSlot slot;
    NetworkErrorInfo error;
    u8 userId[8];
    s32 kind;
    s32 index;
    s32 result;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 != 0) {
            setSessionLog(request, 0x80050012, 106, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            this->connected_3C0 = 1;
            request->state_00 = 5;
        }
        break;
    case 5:
        result = getNetworkLogger()->checkAccount_38(0, &error);
        if (result < 0) {
            setSessionLog(request, error.code_00, error.param1_04, error.param2_08);
            request->state_00 = REQUEST_CANCELLED;
        } else if (result > 0) {
            request->state_00 = 10;
        }
        break;
    case 10:
        openPatInterface(getInstance_());
        setCallback(getInstance_(), (void (*)())networkSessionReflectCallbackEx, this, 3);
        if (getErrorInfo654c(getInstance_(), NULL) != 0) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if ((u8)getNetworkBinaryState(getInstance_()) != 0) {
            request->state_00 = 80;
        } else {
            setConnectServerType(getInstance_(), 0);
            this->requestFlags_30C[1] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 15;
        }
        break;
    case 15:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[1] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            result = handleNetworkState2(getInstance_());
            if (result < 0) {
                setSessionLog(request, 0x80050012, 108, result);
                request->state_00 = REQUEST_CANCELLED;
            } else if (result > 0) {
                this->requestFlags_30C[1] = 0;
                sendReqShut(getInstance_(), 2);
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        if ((this->requestFlags_30C[1] & REQUEST_SESSION_LOST) || (this->requestFlags_30C[1] & REQUEST_ABORTED) ||
            (this->requestFlags_30C[1] & 0x10)) {
            this->requestFlags_30C[1] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 25;
        }
        break;
    case 25:
        if (this->requestFlags_30C[1] & 0x8) {
            if (isSubState_894F_4or6((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80060034, 0, 0);
                request->state_00 = REQUEST_CANCELLED;
            } else if (isSubState_894F_5((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80060035, 0, 0);
                request->state_00 = REQUEST_CANCELLED;
            } else if (isSubState_894F_3((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80050012, 114, 0);
                request->state_00 = REQUEST_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (getSomething4(getInstance_()) < 0) {
            request->setRecord(0x80050033, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (getSomething4(getInstance_()) > 0) {
            this->requestFlags_30C[1] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 35;
        }
        setSomething(getInstance_(), 0);
        break;
    case 35:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[1] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            result = handleNetworkState2Fmp(getInstance_());
            if (result < 0) {
                setSessionLog(request, 0x80050012, 109, result);
                request->state_00 = REQUEST_CANCELLED;
            } else if (result > 0) {
                this->requestFlags_30C[1] = 0;
                sendReqShut(getInstance_(), 2);
                request->state_00 = 40;
            }
        }
        break;
    case 40:
        if ((this->requestFlags_30C[1] & REQUEST_SESSION_LOST) || (this->requestFlags_30C[1] & REQUEST_ABORTED) ||
            (this->requestFlags_30C[1] & 0x10)) {
            this->requestFlags_30C[1] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 45;
        }
        break;
    case 45:
        if (this->requestFlags_30C[1] & 0x8) {
            request->state_00 = 50;
        }
        break;
    case 50:
        setConnectServerType(getInstance_(), 1);
        this->requestFlags_30C[1] = 0;
        resetNetworkState(getInstance_());
        request->state_00 = 55;
        break;
    case 55:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[1] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setSessionLog(request, 0x80050012, 110, result);
                request->state_00 = REQUEST_CANCELLED;
            } else if (result > 0) {
                request->state_00 = 60;
            }
        }
        break;
    case 60:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (getSomething4(getInstance_()) < 0) {
            setSessionLog(request, 0x80050033, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (getSomething4(getInstance_()) > 0) {
            getFmpSelection(getInstance_(), &kind, &index);
            if (kind != 1 || index < 0) {
                setSessionLog(request, 0x80050002, 0, 0);
                request->state_00 = REQUEST_CANCELLED;
                break;
            }
            copyFmpSlot(getInstance_(), &slot, index);
            this->requestFlags_30C[1] = 0;
            sendReqFmpInfo(getInstance_(), slot.payload_00, 1);
            request->state_00 = 65;
        }
        setSomething(getInstance_(), 0);
        break;
    case 65:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[1] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[1] & 0x20) {
            this->requestFlags_30C[1] = 0;
            sendReqShut(getInstance_(), 2);
            request->state_00 = 70;
        }
        break;
    case 70:
        if ((this->requestFlags_30C[1] & REQUEST_SESSION_LOST) || (this->requestFlags_30C[1] & REQUEST_ABORTED) ||
            (this->requestFlags_30C[1] & 0x10)) {
            this->requestFlags_30C[1] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 75;
        }
        break;
    case 75:
        if (this->requestFlags_30C[1] & 0x8) {
            setConnectServerType(getInstance_(), 1);
            this->requestFlags_30C[1] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 77;
        }
        break;
    case 77:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[1] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setSessionLog(request, 0x80050012, 110, result);
                request->state_00 = REQUEST_CANCELLED;
            } else if (result > 0) {
                request->state_00 = 80;
            }
        }
        break;
    case 80:
        this->requestFlags_30C[1] = 0;
        sendReqCircleInfoNoticeSet(getInstance_());
        request->state_00 = 85;
        break;
    case 85:
        if (this->requestFlags_30C[1] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[1] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[1] & 0x2000000) {
            request->state_00 = 90;
        }
        break;
    case 90:
        if (GameSpyInterfaceThread::getInstance() != NULL) {
            GameSpyInterfaceThread::getInstance()->initialize();
            this->field_6E75 = 1;
        }
        getSelectedID(getInstance_(), userId);
        this->field_3CC.importFrom(3, userId, sizeof(userId));
        getNetworkLogger()->readMatchOptions_78(0, &this->matchOptions_3EC);
        this->matchOptions_3EC.sessionKey_04 = rand() % 10000 + 10000;
        getSelectedID(getInstance_(), this->matchOptions_3EC.userId_08);
        getSelectedHunterName(getInstance_(), this->matchOptions_3EC.name_10);
        postEvent(1, 0, 0, 0, NULL, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(1, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(1, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Reports error 0x80050011 for the request slot +0x184 and finishes. */
s32 NetworkSessionManagerPat::slot_184(NetworkRequest* request)
{
    NetworkErrorInfo info;

    info.code_00 = 0x80050011;
    info.param1_04 = 0;
    info.param2_08 = 0;
    postEvent(6, 0, info.code_00, 1, &info, this->unused_08);
    return 1;
}

/* Reports event 30 with this console's slot index and finishes. */
s32 NetworkSessionManagerPat::slot_1B8(NetworkRequest* request)
{
    postEvent(30, this->selfIndex_536, 0, 0, NULL, this->unused_08);
    return 1;
}

/* Arms the session for a fresh request pair (event 8); error 0x8005003A while a circle id is still assigned. */
s32 NetworkSessionManagerPat::slot_188(NetworkRequest* request)
{
    NetworkErrorInfo info;

    if (canSend_28() != 0) {
        info.code_00 = 0x8005003A;
        info.param1_04 = 0;
        info.param2_08 = 0;
        postEvent(8, 0, 0, 1, &info, this->unused_08);
        return 1;
    }
    this->matchRunning_3C1 = 0;
    networkPatAttachBuffer(this);
    postEvent(8, 0, 0, 0, NULL, this->unused_08);
    return 1;
}

/* Publishes this console's +0x3C value to the circle and reports it (event 20); error 0x80050037 while no
 * circle id is assigned. */
s32 NetworkSessionManagerPat::slot_1A0(NetworkRequest* request)
{
    s32 value = request->getArgument(0);
    NetworkErrorInfo info;

    if (canSend_28() == 0) {
        info.code_00 = 0x80050037;
        info.param1_04 = 0;
        info.param2_08 = 0;
        postEvent(20, 0, info.code_00, 1, &info, this->unused_08);
        return 1;
    }
    this->players_538[this->selfIndex_536].value_3C = value;
    sendNtcCircleUserValue(getInstance_(), this->circleInfoRequestId_41C, value, 0);
    postEvent(20, this->selfIndex_536, 0, 1, &value, this->unused_08);
    return 1;
}

/* Reports each timed-out circle id of the argument list (event 9: its records, or error 0x80050002 for an
 * unknown id, which is also sent to the server), then the end of the list (event 10). */
s32 NetworkSessionManagerPat::handleServerTimeout(NetworkRequest* request)
{
    u32 count = request->getArgument(0);
    s32* ids = (s32*)request->getArgument(1);
    u32 i;

    for (i = 0; i < count; i++) {
        s32 id = ids[i];
        u32 values[3];
        NetworkErrorInfo info;

        if (id < 0 || 32 <= id || this->circleList_AF0.items_04[id].id_000 <= 0) {
            info.code_00 = 0x80050002;
            info.param1_04 = 0;
            info.param2_08 = id;
            values[0] = 0x80050002;
            values[1] = 0;
            values[2] = id;
            sendServerTimeout(getInstance_(), values);
            postEvent(9, 0, info.code_00, 1, &info, this->unused_08);
        } else {
            postEvent(9, 0, id, this->circleList_AF0.items_04[id].recordCount_184,
                      this->circleList_AF0.items_04[id].records_188, this->unused_08);
        }
    }
    postEvent(10, 0, 0, 0, NULL, this->unused_08);
    return 1;
}

/* Reports `count` reset timers (event 37, one per index) and the end of the list (event 38). */
s32 NetworkSessionManagerPat::slot_190(NetworkRequest* request)
{
    s32 count = request->getArgument(0);
    PatEventTimer timer;
    s32 i;

    request->getArgument(1);
    timer.enabled_00 = 1;
    timer.time_04 = networkRequestTimerReset;
    for (i = 0; i < count; i++) {
        postEvent(37, 0, i, 1, &timer, this->unused_08);
    }
    postEvent(38, 0, 0, 0, NULL, this->unused_08);
    return 1;
}

/* Shuts the session down step by step: the resolver, the GameSpy thread, the server (shut mode 1) while the interface
 * is referenced, callback 3, then waits for the log to drain and reports the end (event 2). */
s32 NetworkSessionManagerPat::shutdown(NetworkRequest* request)
{
    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            request->state_00 = 40;
            break;
        }
        if (this->resolver_660 != NULL) {
            this->resolver_660->resetCode();
            if (this->resolver_660->check() == 0) {
                break;
            }
            networkLog_destroyContext((NetworkSessionManagerLogger*)getNetworkLogger(), this->resolver_660);
            this->resolver_660 = NULL;
        }
        if (GameSpyInterfaceThread::getInstance() != NULL) {
            if (this->field_6E75 != 0) {
                this->field_6E75 = 0;
                GameSpyInterfaceThread::getInstance()->canClose();
                GameSpyInterfaceThread::getInstance()->armCancel();
            }
            if (GameSpyInterfaceThread::getInstance()->requestClose()) {
                break;
            }
        }
        if (getInstance_() == NULL) {
            request->state_00 = 30;
        } else if (isCallback(getInstance_(), 3) == 0) {
            request->state_00 = 30;
        } else if (hasMultipleRefs60d4(getInstance_()) != 0) {
            request->state_00 = 20;
        } else if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            request->state_00 = 20;
        } else {
            this->requestFlags_30C[2] = 0;
            sendReqShut(getInstance_(), 1);
            request->state_00 = 5;
        }
        break;
    case 5:
        if ((this->requestFlags_30C[2] & REQUEST_SESSION_LOST) || (this->requestFlags_30C[2] & REQUEST_ABORTED) ||
            (this->requestFlags_30C[2] & 0x10)) {
            this->requestFlags_30C[2] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 10;
        }
        break;
    case 10:
        if (this->requestFlags_30C[2] & 0x8) {
            request->state_00 = 20;
        }
        break;
    case 20:
        resetCallback(getInstance_(), 3);
        decrement60d4(getInstance_());
        request->state_00 = 30;
        break;
    case 30:
        if (getNetworkLogger()->isVerbose_3C() > 0) {
            request->state_00 = 40;
        }
        break;
    case 40:
        this->connected_3C0 = 0;
        postEvent(2, 0, 0, 0, NULL, this->unused_08);
        return 1;
    }
    return 0;
}

/* Sends this console's match mode (event 26); inside a match setup it re-takes the host's layer slot as subhost and
 * reports the members (event 25); a pending error code (+0x3C4) is reported (event 12) once no circle request runs. */
s32 NetworkSessionManagerPat::handleCircleMatchOptionSet(NetworkRequest* request)
{
    s32 mode = request->getArgument(0);
    PatMatchOptions options;
    NetworkErrorInfo error;
    NetworkErrorInfo pending;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (canSend_28() == 0) {
            setSessionLog(request, 0x80050037, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            memset(&options, 0, sizeof(options));
            options.mode_06 = (s8)mode;
            this->requestFlags_30C[18] = 0;
            this->requestIds_360[18] = sendReqCircleMatchOptionSet(getInstance_(), &options);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[18] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[18] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[18] & 0x100000) {
            request->state_00 = 10;
        }
        break;
    case 10:
        if (this->field_3C4 == 0 && (this->field_3C3 == 0 || this->matchOptions_3EC.mode_06 != 2)) {
            this->matchOptions_3EC.mode_06 = (s8)mode;
        }
        this->players_538[this->selfIndex_536].state_03 = (mode != 0);
        if (this->players_538[this->selfIndex_536].announced_01 != 0) {
            postEvent(26, this->selfIndex_536, 0, 1, &this->players_538[this->selfIndex_536].state_03,
                      this->unused_08);
            if (this->field_3C3 != 0) {
                if (this->matchPhase_66C != 0) {
                    s8 slot = mapId_1C0(this->hostIndex_537);

                    if (slot >= 0 && this->buffer != NULL) {
                        ((NetworkSessionStable*)this->buffer)->setSubhostIndex(slot);
                    } else {
                        error.code_00 = 0x80000000;
                        error.param1_04 = 0;
                        error.param2_08 = 0;
                        ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
                        setSessionLogSessionLost(request);
                        request->state_00 = REQUEST_FAILED;
                        break;
                    }
                }
                postEvent(25, this->selfIndex_536, -(!this->matchPhase_66C), this->matchMemberCount_664,
                          this->matchMembers_668, this->unused_08);
            }
        }
        if (this->field_3C4 != 0 && this->requests_10[6] == NULL && this->requests_10[11] == NULL) {
            if (this->players_538[this->selfIndex_536].announced_01 != 0) {
                pending.code_00 = this->field_3C4;
                pending.param1_04 = 0;
                pending.param2_08 = 0;
                postEvent(12, 0, pending.code_00, 1, &pending, this->unused_08);
            }
            networkPatAttachBuffer(this);
        }
        this->field_3C2 = 0;
        this->field_3C3 = 0;
        this->field_3C4 = 0;
        return 1;
    case REQUEST_CANCELLED:
        this->field_3C2 = 0;
        this->field_3C3 = 0;
        this->field_3C4 = 0;
        request->getRecord(&error);
        postEvent(26, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        this->field_3C2 = 0;
        this->field_3C3 = 0;
        this->field_3C4 = 0;
        request->getRecord(&error);
        postEvent(26, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Runs the circle list query (event 5): the search with the packed conditions, then the found circles ten at a
 * time (at most 32), then the query's end; the conditions are cleared either way. */
s32 NetworkSessionManagerPat::slot_178(NetworkRequest* request)
{
    s32 count = request->getArgument(0);
    s32 first;
    s32 batch;
    NetworkErrorInfo error;
    PatCircleFilter filters[8];
    s32 filterCount;
    u32 sortFlag;

    switch (request->state_00) {
    case REQUEST_START:
        this->field_6E74 = 0;
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (count <= 0) {
            setSessionLog(request, 0x80050002, 0, count);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) == 0) {
            filterCount = packCircleConditions(filters, 8, &this->conditionList_7E8);
            this->requestFlags_30C[4] = 0;
            sortFlag = (this->conditionList_7E8.head_00 & 4) ? 1 : 0;
            this->requestIds_360[4] = sendReqCircleListHead(getInstance_(), 1, count, filters, filterCount, sortFlag);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[4] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[4] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[4] & 0x1000) {
            this->circleList_AF0.count_00 = 0;
            if (this->listTotal_AEC > 0) {
                this->listCursor_AE8 = 0;
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 10:
        this->requestFlags_30C[4] = 0;
        first = this->listCursor_AE8;
        batch = (this->listTotal_AEC - first < 10) ? this->listTotal_AEC - first : 10;
        this->requestIds_360[4] = sendReqCircleListData(getInstance_(), first + 1, batch);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_30C[4] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[4] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 20;
        } else if (this->requestFlags_30C[4] & 0x2000) {
            if (this->circleList_AF0.count_00 < 32 && this->listTotal_AEC - this->listCursor_AE8 > 0) {
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_30C[4] = 0;
        this->requestIds_360[4] = sendReqCircleListFoot(getInstance_());
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_30C[4] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[4] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[4] & 0x4000) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = REQUEST_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        set611b(getInstance_());
        memset(&this->conditionList_7E8, 0, sizeof(this->conditionList_7E8));
        postEvent(5, 0, 0, 0, NULL, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        if (error.code_00 != 0x80050001 && error.code_00 != 0x80050002) {
            set611b(getInstance_());
        }
        memset(&this->conditionList_7E8, 0, sizeof(this->conditionList_7E8));
        postEvent(5, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        set611b(getInstance_());
        memset(&this->conditionList_7E8, 0, sizeof(this->conditionList_7E8));
        postEvent(5, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Leaves the circle (event 16): ends a mode-2 match or clears this console's match mode with the server first,
 * publishes what is still pending, then runs the leave steps; a circle that went away meanwhile just ends. */
s32 NetworkSessionManagerPat::handleCircleLeave(NetworkRequest* request)
{
    PatCircleInfo circleInfo;
    PatCircleOptionList options;
    PatMatchOptions matchOptions;
    NetworkErrorInfo error;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (canSend_28() == 0) {
            setSessionLog(request, 0x80050037, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->matchOptions_3EC.mode_06 == 2) {
            this->requestFlags_30C[11] = 0;
            this->requestIds_360[11] = sendReqCircleMatchEnd(getInstance_(), 0);
            request->state_00 = 5;
        } else if (this->players_538[this->selfIndex_536].state_03 != 0) {
            memset(&matchOptions, 0, sizeof(matchOptions));
            matchOptions.mode_06 = 0;
            this->requestFlags_30C[11] = 0;
            this->requestIds_360[11] = sendReqCircleMatchOptionSet(getInstance_(), &matchOptions);
            request->state_00 = 15;
        } else {
            request->state_00 = 20;
        }
        break;
    case 5:
        if (this->requestFlags_30C[11] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[11] & REQUEST_ABORTED) {
            if (this->circleLost_535 != 0) {
                request->state_00 = 40;
            } else {
                setSessionLogAborted(request);
                request->state_00 = REQUEST_CANCELLED;
            }
        } else if (this->requestFlags_30C[11] & 0x400000) {
            request->state_00 = 20;
        }
        break;
    case 15:
        if (this->requestFlags_30C[11] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[11] & REQUEST_ABORTED) {
            if (this->circleLost_535 != 0) {
                request->state_00 = 40;
            } else {
                setSessionLogAborted(request);
                request->state_00 = REQUEST_CANCELLED;
            }
        } else if (this->requestFlags_30C[11] & 0x100000) {
            request->state_00 = 20;
        }
        break;
    case 20:
        if (circleAvailable(this) == 0 ||
            (this->circleRecordCount_950 == 0 && this->nameList_7A0.count_04 == 0 && this->field_AE6 == 0)) {
            request->state_00 = 30;
            break;
        }
        this->field_3B8 = getNetworkLogger()->getTime_60();
        memset(&circleInfo, 0, sizeof(circleInfo));
        if (this->circleRecordCount_950 != 0) {
            circleInfo.recordCount_156 = (this->circleRecordCount_950 >= 256) ? 256 : this->circleRecordCount_950;
            memcpy(circleInfo.records_56, this->circleRecords_954, circleInfo.recordCount_156);
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
        this->requestIds_360[11] = sendReqCircleInfoSet(
            getInstance_(), this->circleInfoRequestId_41C, &circleInfo, (const char*)&options);
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_30C[11] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[11] & REQUEST_ABORTED) {
            if (this->circleLost_535 != 0) {
                request->state_00 = 40;
            } else {
                setSessionLogAborted(request);
                request->state_00 = REQUEST_CANCELLED;
            }
        } else if (this->requestFlags_30C[11] & 0x10000) {
            request->state_00 = 30;
        }
        break;
    case 30:
        beginCircleLeave();
        request->state_00 = 35;
        break;
    case 35:
        switch (stepCircleLeave(&error)) {
        case 1:
            request->state_00 = 40;
            break;
        case -1:
            setSessionLog(request, error.code_00, error.param1_04, error.param2_08);
            request->state_00 = REQUEST_CANCELLED;
            break;
        case -2:
            request->setRecord(error.code_00, error.param1_04, error.param2_08);
            request->state_00 = REQUEST_FAILED;
            break;
        }
        break;
    case 40:
        postEvent(16, 0, 0, 0, NULL, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(16, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(16, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Creates a circle for `maxPlayers` (1..4, `reserved` held back) and hosts it (event 4): the pending name, comment,
 * records and mode, the match options, then reads the circle back; a failure after the create leaves it again. */
s32 NetworkSessionManagerPat::handleCircleCreate(NetworkRequest* request)
{
    s32 maxPlayers = request->getArgument(0);
    s32 reserved = request->getArgument(1);
    PatCircleInfo info;
    PatCircleOptionList options;
    NetworkErrorInfo error;
    s32 index;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (maxPlayers < 1 || maxPlayers > 4 || reserved < 0 || maxPlayers <= reserved) {
            setSessionLog(request, 0x80050002, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (canSend_28() != 0) {
            setSessionLog(request, 0x8005003A, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            memset(&info, 0, sizeof(info));
            info.limitB_360 = maxPlayers;
            info.usedB_364 = reserved;
            if (this->sessionName_850[0] != 0) {
                info.flag_044 = 1;
                memcpy(info.sessionName_045, this->sessionName_850, sizeof(info.sessionName_045));
                info.sessionNameEnd_054 = 0;
            }
            if (this->circleComment_A54[0] != 0) {
                memcpy(info.comment_158, this->circleComment_A54, sizeof(info.comment_158));
                info.commentEnd_1E8 = 0;
            }
            if (this->circleRecordCount_950 != 0) {
                info.recordCount_156 = (this->circleRecordCount_950 >= 256) ? 256 : this->circleRecordCount_950;
                memcpy(info.records_56, this->circleRecords_954, info.recordCount_156);
                this->circleRecordCount_950 = 0;
            }
            if (this->field_AE5 != 0) {
                info.mode_378 = 1;
            }
            memset(&options, 0, sizeof(options));
            if (this->nameList_7A0.count_04 != 0) {
                buildCircleInfoName(this, &options, &this->nameList_7A0);
                this->nameList_7A0.count_04 = 0;
            }
            this->field_3B8 = getNetworkLogger()->getTime_60();
            this->field_AE6 = 0;
            this->limitB_524 = maxPlayers;
            this->usedB_52C = reserved;
            this->requestFlags_30C[3] = 0;
            this->requestIds_360[3] = sendReqCircleCreate(getInstance_(), &info, &options);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[3] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[3] & REQUEST_ABORTED) {
            this->limitB_524 = 0;
            this->usedB_52C = 0;
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[3] & 0x40) {
            request->state_00 = 10;
        }
        break;
    case 10:
        this->requestFlags_30C[3] = 0;
        this->requestIds_360[3] = sendReqCircleMatchOptionSet(getInstance_(), &this->matchOptions_3EC);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_30C[3] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[3] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 90;
        } else if (this->requestFlags_30C[3] & 0x100000) {
            request->state_00 = 20;
        }
        break;
    case 20:
        this->requestFlags_30C[3] = 0;
        this->requestIds_360[3] = sendReqCircleInfo(getInstance_(), this->circleInfoRequestId_41C, 0);
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_30C[3] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[3] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 90;
        } else if (this->requestFlags_30C[3] & 0x8000) {
            request->state_00 = 30;
        }
        break;
    case 30:
        resetCircleState();
        if (this->field_6E74 != 0) {
            for (index = 0; index < 32; index++) {
                if (this->circleInfoRequestId_41C == this->circleList_AF0.items_04[index].id_000) {
                    break;
                }
            }
            if (index >= 32) {
                setSessionLog(request, 0x80050037, 0, 0);
                request->state_00 = 90;
                break;
            }
            postEvent(4, this->selfIndex_536, 0, 1, &index, this->unused_08);
        } else {
            postEvent(4, this->selfIndex_536, 0, 0, NULL, this->unused_08);
        }
        this->players_538[this->selfIndex_536].announced_01 = 1;
        announcePlayers();
        if (this->players_538[this->hostIndex_537].announced_01 != 0) {
            postEvent(14, this->hostIndex_537, 0, 0, NULL, this->unused_08);
        }
        return 1;
    case 90:
        beginCircleLeave();
        request->state_00 = 95;
        break;
    case 95:
        switch (stepCircleLeave(&error)) {
        case 1:
            request->state_00 = REQUEST_CANCELLED;
            break;
        case -1:
            ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
        case -2:
            request->setRecord(error.code_00, error.param1_04, error.param2_08);
            request->state_00 = REQUEST_FAILED;
            break;
        }
        break;
    case REQUEST_CANCELLED:
        resetCircleState();
        request->getRecord(&error);
        postEvent(4, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        resetCircleState();
        request->getRecord(&error);
        postEvent(4, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Joins circle `index` of the last list (event 6): join, match options, members, circle and host, then waits (30 s)
 * until every member is linked; a failure after the join, or the circle going away (0x80050036), leaves it again. */
s32 NetworkSessionManagerPat::handleCircleJoin(NetworkRequest* request)
{
    s32 index = request->getArgument(0);
    PatCircleInfo info;
    NetworkErrorInfo error;
    s8 i;

    if (this->circleLost_535 != 0 && request->state_00 != 25 && request->state_00 != 35 &&
        request->state_00 != 45 && request->state_00 != 55 && request->state_00 < 90) {
        setSessionLog(request, 0x80050036, 0, 0);
        request->state_00 = 90;
    }
    switch (request->state_00) {
    case REQUEST_START:
        this->nameList_7A0.count_04 = 0;
        this->circleRecordCount_950 = 0;
        this->field_AE5 = 0;
        this->field_AE6 = 0;
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (canSend_28() != 0) {
            setSessionLog(request, 0x8005003A, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (index < 0 || index >= getCircleInfoCount()) {
            setSessionLog(request, 0x80050002, 0, index);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            memset(&info, 0, sizeof(info));
            info.id_000 = this->circleList_AF0.items_04[index].id_000;
            info.ownerId_368 = this->circleList_AF0.items_04[index].ownerId_004;
            if (this->sessionName_850[0] != 0) {
                info.flag_044 = 1;
                memcpy(info.sessionName_045, this->sessionName_850, sizeof(info.sessionName_045));
                info.sessionNameEnd_054 = 0;
            }
            this->limitB_524 = this->circleList_AF0.items_04[index].limitB_174;
            this->requestFlags_30C[6] = 0;
            this->requestIds_360[6] = sendReqCircleJoin(getInstance_(), &info);
            request->state_00 = 15;
        }
        break;
    case 15:
        if (this->requestFlags_30C[6] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[6] & REQUEST_ABORTED) {
            this->limitB_524 = 0;
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[6] & 0x20000) {
            this->circleInfoRequestId_41C = this->circleList_AF0.items_04[index].id_000;
            request->state_00 = 20;
        }
        break;
    case 20:
        this->requestFlags_30C[6] = 0;
        this->requestIds_360[6] = sendReqCircleMatchOptionSet(getInstance_(), &this->matchOptions_3EC);
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_30C[6] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[6] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 90;
        } else if (this->requestFlags_30C[6] & 0x100000) {
            request->state_00 = 30;
        }
        break;
    case 30:
        this->requestFlags_30C[6] = 0;
        this->requestIds_360[6] = sendReqCircleUserList(getInstance_());
        request->state_00 = 35;
        break;
    case 35:
        if (this->requestFlags_30C[6] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[6] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 90;
        } else if (this->requestFlags_30C[6] & 0x80) {
            request->state_00 = 40;
        }
        break;
    case 40:
        this->requestFlags_30C[6] = 0;
        this->requestIds_360[6] = sendReqCircleInfo(getInstance_(), this->circleInfoRequestId_41C, 0);
        request->state_00 = 45;
        break;
    case 45:
        if (this->requestFlags_30C[6] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[6] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 90;
        } else if (this->requestFlags_30C[6] & 0x8000) {
            request->state_00 = 50;
        }
        break;
    case 50:
        this->requestFlags_30C[6] = 0;
        this->requestIds_360[6] = sendReqCircleHost(getInstance_(), this->circleInfoRequestId_41C);
        request->state_00 = 55;
        break;
    case 55:
        if (this->requestFlags_30C[6] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[6] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = 90;
        } else if (this->requestFlags_30C[6] & 0x40000) {
            sendNtcCircleUserValue(getInstance_(), this->circleInfoRequestId_41C, this->players_538[this->selfIndex_536].value_3C,
                        1);
            request->restartTimer(30.0f);
            request->state_00 = 60;
        }
        break;
    case 60:
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
            break;
        }
        for (i = 0; i < getWord_524(); i++) {
            if (this->players_538[i].active_00 != 0 && i != this->selfIndex_536 && this->players_538[i].flag_02 == 0) {
                if (request->isTimedOut() != 0) {
                    setSessionLog(request, 0x80000002, 0, 0);
                    request->state_00 = 90;
                }
                return 0;
            }
        }
        resetCircleState();
        postEvent(6, this->selfIndex_536, 0, 0, NULL, this->unused_08);
        this->players_538[this->selfIndex_536].announced_01 = 1;
        announcePlayers();
        if (this->players_538[this->hostIndex_537].announced_01 != 0) {
            postEvent(14, this->hostIndex_537, 0, 0, NULL, this->unused_08);
        }
        return 1;
    case 90:
        beginCircleLeave();
        request->state_00 = 95;
        break;
    case 95:
        switch (stepCircleLeave(&error)) {
        case 1:
            request->state_00 = REQUEST_CANCELLED;
            break;
        case -1:
            ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
        case -2:
            request->setRecord(error.code_00, error.param1_04, error.param2_08);
            request->state_00 = REQUEST_FAILED;
            break;
        }
        break;
    case REQUEST_CANCELLED:
        resetCircleState();
        request->getRecord(&error);
        postEvent(6, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        resetCircleState();
        request->getRecord(&error);
        postEvent(6, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Starts the match session (event 28): checks each member's negotiation (event 36, 35 for this console's own), waits
 * for GameSpy phase 1 and every link, then raises the session's flag B (`matchPhase_66C` = 3). */
s32 NetworkSessionManagerPat::moveStartSession(NetworkRequest* request)
{
    NetworkErrorInfo error;
    u8 member;
    s32 i;

    if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
        if (request->state_00 < 90) {
            request->state_00 = 90;
        }
    } else if (canSend_28() == 0) {
        setSessionLog(request, 0x80050037, 0, 0);
        request->state_00 = REQUEST_CANCELLED;
    }
    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (this->matchRunning_3C1 != 0) {
            setSessionLog(request, 0x80050046, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        this->matchRunning_3C1 = 1;
        if (this->buffer != NULL) {
            ((NetworkSessionBase*)this->buffer)->discardAll();
        }
        if (this->matchPhase_66C == 0 || this->matchPhase_66C == 3) {
            setSessionLog(request, 0x80050011, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (this->limitA_528 > 1 &&
            (GameSpyInterfaceThread::getInstance() == NULL || GameSpyInterfaceThread::getInstance()->getState() != 1)) {
            if (GameSpyInterfaceThread::getInstance() == NULL) {
                setSessionLog(request, 0x80050011, 0, -1);
            } else {
                GameSpyInterfaceThread::getInstance()->getErrorStruct(&error);
                setSessionLog(request, error.code_00, error.param1_04, error.param2_08);
            }
            this->matchPhase_66C = 0;
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
            if (getNetworkLayerPat(getPatsObject(), 0)->getMemberStatus(&this->field_3CC) == 1) {
                this->matchPhase_66C = 0;
                postEvent(35, 0, this->errorCode_794, 1, &this->errorCode_794, this->unused_08);
            } else {
                for (i = 0; i < this->matchMemberCount_664; i++) {
                    member = this->matchMembers_668[i];
                    if (this->players_538[(s8)member].active_00 != 0 && (s8)member != this->selfIndex_536 &&
                        getNetworkLayerPat(getPatsObject(), 0)->getMemberStatus(
                            &this->players_538[(s8)member].smallObject_08) == 1) {
                        postEvent(36, (s8)member, -1, 0, NULL, this->unused_08);
                    }
                }
            }
        }
        request->state_00++;
        break;
    case 1:
        if (this->matchPhase_66C == 0) {
            setSessionLog(request, this->errorCode_794, this->errorParam1_798, this->errorParam2_79C);
            request->state_00 = 90;
            break;
        }
        if (this->limitA_528 > 1) {
            if (GameSpyInterfaceThread::getInstance() == NULL || GameSpyInterfaceThread::getInstance()->getState() != 1 ||
                GameSpyInterfaceThread::getInstance()->getPhase() < 0) {
                if (GameSpyInterfaceThread::getInstance() == NULL) {
                    setSessionLog(request, 0x80050011, 0, -2);
                } else {
                    GameSpyInterfaceThread::getInstance()->getErrorStruct(&error);
                    setSessionLog(request, error.code_00, error.param1_04, error.param2_08);
                }
                this->matchPhase_66C = 0;
                request->state_00 = 90;
                break;
            }
            if (GameSpyInterfaceThread::getInstance()->getPhase() != 1) {
                break;
            }
        }
        for (i = 0; i < this->matchMemberCount_664; i++) {
            member = this->matchMembers_668[i];
            if (this->players_538[(s8)member].active_00 != 0 && getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                u8 status = getNetworkLayerPat(getPatsObject(), 0)->getMemberStatus(
                    &this->players_538[(s8)member].smallObject_08);

                if (status == 0) {
                    return 0;
                }
                if (status == 2) {
                    return 0;
                }
            }
        }
        sendNtcCircleMatchState(getInstance_(), this->circleInfoRequestId_41C, 3);
        request->state_00++;
        break;
    case 2:
        if (this->matchPhase_66C == 0) {
            getNetworkLogger()->warn_10("moveStartSession::mMatchPhase(0) NG\n");
            setSessionLog(request, this->errorCode_794, this->errorParam1_798, this->errorParam2_79C);
            request->state_00 = 90;
            break;
        }
        for (i = 0; i < this->matchMemberCount_664; i++) {
            member = this->matchMembers_668[i];
            if (this->players_538[(s8)member].active_00 != 0 && (s8)member != this->selfIndex_536 &&
                this->players_538[(s8)member].linked_04 != 3) {
                return 0;
            }
        }
        if (this->buffer == NULL) {
            setSessionLog(request, 0x80050037, 0, 0);
            request->state_00 = 90;
            break;
        }
        this->matchPhase_66C = 3;
        ((NetworkSessionBase*)this->buffer)->setUserFlagB(1);
        this->field_3BC = getNetworkLogger()->getTime_60();
        postEvent(28, 0, 0, 0, NULL, this->unused_08);
        return 1;
    case 90:
        request->state_00 = 95;
        break;
    case 95:
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else {
            request->state_00 = REQUEST_CANCELLED;
        }
        break;
    case REQUEST_CANCELLED:
        this->matchPhase_66C = 0;
        request->getRecord(&error);
        postEvent(28, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        this->matchPhase_66C = 0;
        request->getRecord(&error);
        postEvent(28, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Starts a match in this console's circle (event 24 with the match members); only the host may.  When this
 * console was announced it takes the host's layer slot as the session's subhost. */
s32 NetworkSessionManagerPat::handleCircleMatchStart(NetworkRequest* request)
{
    NetworkErrorInfo error;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (circleAvailable(this) == 0) {
            setSessionLog(request, 0x80050032, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            this->requestFlags_30C[7] = 0;
            this->requestIds_360[7] = sendReqCircleMatchStart(getInstance_());
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[7] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[7] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[7] & 0x200000) {
            request->state_00 = 10;
        }
        break;
    case 10:
        if (this->players_538[this->selfIndex_536].announced_01 != 0) {
            s8 slot = mapId_1C0(this->hostIndex_537);

            if (slot >= 0 && this->buffer != NULL) {
                ((NetworkSessionStable*)this->buffer)->setSubhostIndex(slot);
            }
            postEvent(24, this->selfIndex_536, 0, this->matchMemberCount_664, this->matchMembers_668,
                      this->unused_08);
        }
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(24, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(24, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Ends the running match (event 32): clears the session's flag B and, in match mode 2, tells the server
 * first; error 0x80050045 while no match is running. */
s32 NetworkSessionManagerPat::handleCircleMatchEnd(NetworkRequest* request)
{
    NetworkErrorInfo error;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->matchRunning_3C1 == 0) {
            setSessionLog(request, 0x80050045, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            if (this->buffer != NULL) {
                ((NetworkSessionBase*)this->buffer)->setUserFlagB(0);
            }
            if (this->matchOptions_3EC.mode_06 != 2) {
                request->state_00 = 10;
            } else {
                this->requestFlags_30C[10] = 0;
                this->requestIds_360[10] = sendReqCircleMatchEnd(getInstance_(), 1);
                request->state_00 = 5;
            }
        }
        break;
    case 5:
        if (this->requestFlags_30C[10] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[10] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[10] & 0x400000) {
            request->state_00 = 10;
        }
        break;
    case 10:
        this->matchRunning_3C1 = 0;
        this->matchPhase_66C = 0;
        postEvent(32, 0, 0, 0, NULL, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(32, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(32, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Asks the server for the circle list (event 39 once it arrived); the requested count must be positive. */
s32 NetworkSessionManagerPat::handleCircleListLayer(NetworkRequest* request)
{
    s32 count = request->getArgument(0);
    NetworkErrorInfo error;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (count <= 0) {
            setSessionLog(request, 0x80050002, 0, count);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            this->requestFlags_30C[5] = 0;
            this->requestIds_360[5] = sendReqCircleListLayer(getInstance_());
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[5] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[5] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[5] & 0x800) {
            request->state_00 = 10;
        }
        break;
    case 10:
        postEvent(39, 0, 0, 0, NULL, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(39, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(39, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Opens or closes this console's circle to new members (the flag at +0x534, event 15); only the host may. */
s32 NetworkSessionManagerPat::handleCircleInfoSet(NetworkRequest* request)
{
    s32 open = request->getArgument(0);
    PatCircleInfo info;
    NetworkErrorInfo error;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (circleAvailable(this) == 0) {
            setSessionLog(request, 0x80050032, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->flag_534 == (open != 0)) {
            request->state_00 = 10;
        } else {
            memset(&info, 0, sizeof(info));
            if (open != 0) {
                info.state_379 = 1;
                info.state_37A = 1;
            } else {
                info.state_379 = -1;
                info.state_37A = -1;
            }
            this->requestFlags_30C[8] = 0;
            this->requestIds_360[8] = sendReqCircleInfoSet(
                getInstance_(), this->circleInfoRequestId_41C, &info, NULL);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[8] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[8] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[8] & 0x10000) {
            request->state_00 = 10;
        }
        break;
    case 10:
        this->flag_534 = (open != 0);
        postEvent(15, 0, 0, 1, &this->flag_534, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(15, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(15, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Sends a chat message (event 18 with this console's own copy): over the session when it runs, else through
 * the server - to everyone for target -1, or to the target player's user id. */
s32 NetworkSessionManagerPat::slot_19C(NetworkRequest* request)
{
    const char* text = (const char*)request->getArgument(0);
    u32 tag = request->getArgument(1);
    s32 target = request->getArgument(2);
    PatMatchOptions options;
    NetworkErrorInfo error;
    char userId[8];
    u32 length;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (canSend_28() == 0) {
            setSessionLog(request, 0x80050037, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (text[0] == 0) {
            setSessionLog(request, 0x80050002, 0, -1);
            request->state_00 = REQUEST_CANCELLED;
            break;
        }
        if (target != -1) {
            if (target < 0 || target >= getWord_524() || target == this->selfIndex_536) {
                setSessionLog(request, 0x80050002, 0, target);
                request->state_00 = REQUEST_CANCELLED;
                break;
            }
            if (this->players_538[target].active_00 == 0) {
                setSessionLog(request, 0x80050011, 0, target);
                request->state_00 = REQUEST_CANCELLED;
                break;
            }
        }
        if (this->buffer != NULL && ((NetworkSessionBase*)this->buffer)->getUserFlagB() != 0) {
            sendSessionChat(text, tag, target);
            request->state_00 = 10;
            break;
        }
        memset(&options, 0, sizeof(options));
        *(u32*)&options = tag;
        if (target == -1) {
            sendNtcCircleChat(getInstance_(), &options, text);
            request->state_00 = 10;
        } else {
            this->players_538[target].smallObject_08.exportTo((u8*)userId, sizeof(userId));
            this->requestFlags_30C[15] = 0;
            this->requestIds_360[15] = sendReqCircleTell(getInstance_(), userId, &options, text);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[15] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[15] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[15] & 0x4000000) {
            request->state_00 = 10;
        }
        break;
    case 10: {
        NetworkSessionSlotInfo message;

        message.smallObject_00.copyFrom((const u8*)&this->field_3CC);
        memcpy(message.name_20, this->matchOptions_3EC.name_10, sizeof(message.name_20));
        message.nameEnd_33 = 0;
        memset(&message.flag_34, 0, sizeof(message.flag_34));
        length = (strlen(text) < sizeof(message.text_35) - 1) ? strlen(text) : sizeof(message.text_35) - 1;
        memcpy(message.text_35, text, length);
        message.text_35[length] = 0;
        message.textEnd_235 = 0;
        message.tag_238 = tag;
        message.time_23C = getServerDateTime(getInstance_());
        postEvent(18, this->selfIndex_536, 0, 1, &message, this->unused_08);
        return 1;
    }
    case REQUEST_CANCELLED:
        request->getRecord(&error);
        postEvent(18, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&error);
        postEvent(18, 0, error.code_00, 1, &error, this->unused_08);
        postEvent(3, 0, error.code_00, 1, &error, this->unused_08);
        return 1;
    }
    return 0;
}

/* Asks the server to remove player `index` from this console's circle (event 22); only the host may, and
 * never for itself or an absent player. */
s32 NetworkSessionManagerPat::slot_1A4(NetworkRequest* request)
{
    s8 index = request->getArgument(0);
    char userId[8];
    NetworkErrorInfo info;

    switch (request->state_00) {
    case REQUEST_START:
        if (this->connected_3C0 == 0) {
            setSessionLog(request, 0x80050001, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (circleAvailable(this) == 0) {
            setSessionLog(request, 0x80050032, 0, 0);
            request->state_00 = REQUEST_CANCELLED;
        } else if (index < 0 || getWord_524() <= index) {
            setSessionLog(request, 0x80050002, 0, index);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->players_538[index].active_00 == 0 || index == this->selfIndex_536) {
            setSessionLog(request, 0x80050011, 0, index);
            request->state_00 = REQUEST_CANCELLED;
        } else {
            this->players_538[index].smallObject_08.exportTo((u8*)userId, sizeof(userId));
            this->requestFlags_30C[17] = 0;
            this->requestIds_360[17] = sendReqCircleKick(getInstance_(), userId);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[17] & REQUEST_SESSION_LOST) {
            setSessionLogSessionLost(request);
            request->state_00 = REQUEST_FAILED;
        } else if (this->requestFlags_30C[17] & REQUEST_ABORTED) {
            setSessionLogAborted(request);
            request->state_00 = REQUEST_CANCELLED;
        } else if (this->requestFlags_30C[17] & 0x1000000) {
            request->state_00 = 10;
        }
        break;
    case 10:
        postEvent(22, index, 0, 0, NULL, this->unused_08);
        return 1;
    case REQUEST_CANCELLED:
        request->getRecord(&info);
        postEvent(22, 0, info.code_00, 1, &info, this->unused_08);
        return 1;
    case REQUEST_FAILED:
        request->getRecord(&info);
        postEvent(22, 0, info.code_00, 1, &info, this->unused_08);
        postEvent(3, 0, info.code_00, 1, &info, this->unused_08);
        return 1;
    }
    return 0;
}

/* Drops the pending circle publish and rewinds the leave steps. */
void NetworkSessionManagerPat::beginCircleLeave()
{
    resetCircleState();
    this->leaveState_3B4 = 0;
}

/* One step of leaving the circle: asks the server unless the circle is already gone; 1 when done, 0 while
 * waiting, -1 / -2 with `error` filled when the request was cancelled / the session was lost. */
s32 NetworkSessionManagerPat::stepCircleLeave(NetworkErrorInfo* error)
{
    u32 record[0x82];

    switch (this->leaveState_3B4) {
    case 0:
        if (canSend_28() == 0 || this->circleLost_535 != 0) {
            this->leaveState_3B4 = 10;
        } else {
            this->requestFlags_30C[11] = 0;
            this->requestIds_360[11] = sendReqCircleLeave(getInstance_(), this->circleInfoRequestId_41C);
            this->leaveState_3B4 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_30C[11] & REQUEST_SESSION_LOST) {
            getErrorInfoOrCode654c(getInstance_(), 0x80050031, error);
            return -2;
        }
        if (this->requestFlags_30C[11] & REQUEST_ABORTED) {
            if (this->circleLost_535 != 0) {
                this->leaveState_3B4 = 10;
                break;
            }
            errorRecordCode613c(getInstance_(), record);
            error->code_00 = 0x80050012;
            error->param1_04 = 0;
            error->param2_08 = record[0];
            return -1;
        }
        if (this->requestFlags_30C[11] & 0x80000) {
            this->leaveState_3B4 = 10;
        }
        break;
    case 10:
        networkPatAttachBuffer(this);
        return 1;
    }
    return 0;
}

/* Resets the Pat side of the session: the joined circle, the counters and slots, the player records, the
 * session's flag B, the match record, the three timeouts and the pending publish. */
void networkPatAttachBuffer(NetworkSessionManagerPat* self)
{
    s32 i;

    self->matchOptions_3EC.mode_06 = 0;
    self->circleInfoRequestId_41C = 0;
    memset(self->circleName_424, 0, sizeof(self->circleName_424));
    self->limitB_524 = 0;
    self->limitA_528 = 0;
    self->usedB_52C = 0;
    self->usedA_530 = 0;
    self->flag_534 = 0;
    self->circleLost_535 = 0;
    self->selfIndex_536 = -1;
    self->hostIndex_537 = -2;
    if (self->buffer != NULL) {
        ((NetworkSessionStable*)self->buffer)->clearSubhostIndex();
    }
    for (i = 0; i < 4; i++) {
        self->resetPlayerRecord(i);
    }
    if (self->buffer != NULL) {
        ((NetworkSessionBase*)self->buffer)->setUserFlagB(0);
    }
    self->matchMemberCount_664 = 0;
    self->matchPhase_66C = 0;
    memset(self->matchData_66E, 0, sizeof(self->matchData_66E));
    networkSessionTimeoutSeconds = 48.0f;
    networkSessionIntervalSeconds = networkSessionPeriodSeconds;
    networkSessionLimitSeconds = networkSessionPeriodSeconds;
    self->nameList_7A0.count_04 = 0;
    self->conditionList_7E8.count_04 = 0;
    memset(self->sessionName_850, 0, sizeof(self->sessionName_850));
    self->circleRecordCount_950 = 0;
    memset(self->circleComment_A54, 0, sizeof(self->circleComment_A54));
    self->field_AE5 = 0;
    self->field_AE6 = 0;
}

/* The Pat holder the library constructor published. */
NetworkPat* getPatsObject(void)
{
    return sNetworkPatInstance;
}

/* Reports error 0x80050011 for the request slot +0x1A8 and finishes. */
s32 NetworkSessionManagerPat::slot_1A8(NetworkRequest* request)
{
    NetworkErrorInfo info;

    info.code_00 = 0x80050011;
    info.param1_04 = 0;
    info.param2_08 = 0;
    postEvent(23, 0, info.code_00, 1, &info, this->unused_08);
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
void NetworkSessionManagerPat::exportCircleItem(NetworkUniqueId* dst, s32 idx)
{
    if (idx < 0 || idx >= getCircleInfoCount()) {
        return;
    }
    if (dst != NULL) {
        dst->copyFrom((const u8*)&this->circleList_AF0.items_04[idx].smallObject_108);
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
s32 NetworkSessionManagerPat::getPlayerRecord(s8 idx, NetworkUniqueId* dst)
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
    dst->copyFrom((const u8*)&this->players_538[idx].smallObject_08);
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


/* The session's event callback (via `networkSessionReflectCallback`): 1 a slot connected, 2 a user packet (event 29),
 * 3 a slot failed (events 35/36), 4 a chat packet, 5 a terms update for the friend on that slot. */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged by networkSessionReflectCallback */
void networkSessionReflect0(void* a0, void* a1, s8 a2, void* a3, void* a4, void* a5)
{
    NetworkSessionManagerPat* self = (NetworkSessionManagerPat*)a0;
    const u8* data = (const u8*)a5;
    PatChatHeader header;
    s32 player = self->slot_1C4(a2);
    u8 kind;
    s32 code;

    switch ((s32)a1) {
    case 1:
        if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
            getNetworkLayerPat(getPatsObject(), 0)->onSessionConnected(a2);
        }
        break;
    case 2:
        if ((s8)player >= 0 && self->players_538[(s8)player].announced_01 != 0) {
            self->postEvent(29, player, (s32)a3, (s32)a4, data, self->unused_08);
        }
        break;
    case 3:
        if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
            NetworkUniqueId address;

            getNetworkLayerPat(getPatsObject(), 0)->getMemberAddress(a2, &address);
            if (address.equals(&self->field_3CC) != 0) {
                self->reportSessionError((const NetworkErrorInfo*)data);
                self->errorCode_794 = ((const NetworkErrorInfo*)data)->code_00;
                self->errorParam1_798 = ((const NetworkErrorInfo*)data)->param1_04;
                self->errorParam2_79C = ((const NetworkErrorInfo*)data)->param2_08;
            }
            getNetworkLayerPat(getPatsObject(), 0)->onSessionFailed(a2, (const NetworkErrorInfo*)data);
        }
        if ((s8)player >= 0 && self->matchRunning_3C1 != 0) {
            if ((s8)player == self->selfIndex_536) {
                self->matchPhase_66C = 0;
                if (isNetworkSessionManagerPatReady(self) != 0) {
                    self->postEvent(35, 0, self->errorCode_794, 1, &self->errorCode_794, self->unused_08);
                }
            } else {
                if (self->udp_65C != NULL) {
                    self->udp_65C->remove(&self->players_538[(s8)player].address_40);
                }
                if (self->matchPhase_66C != 0 && self->players_538[(s8)player].announced_01 != 0) {
                    self->postEvent(36, player, ((const NetworkErrorInfo*)data)->code_00, 1, data, self->unused_08);
                }
            }
        }
        break;
    case 4:
        kind = data[0];
        if ((s8)player >= 0) {
            switch (kind) {
            case 1:
                self->readChatHeader(&header, data);
                if (self->players_538[(s8)player].announced_01 != 0) {
                    code = 31;
                    if ((s8)player == self->selfIndex_536) {
                        code = 30;
                    }
                    self->postEvent(code, player, 0, 0, NULL, self->unused_08);
                }
                break;
            case 2:
                self->receiveSessionChat(data, (u32)a4);
                break;
            }
        }
        break;
    case 5:
        if (getMediatorTermsStatus(getInstance()) != 0 && isMediatorTermsUpdateFinished(getInstance()) != 0 &&
            getNetworkLayerPat(getPatsObject(), 0) != NULL) {
            s8 friendIndex = getNetworkLayerPat(getPatsObject(), 0)->findSessionFriend(a2);

            getInstance()->pushTransferRecord(friendIndex, (const u8*)((const u32*)data)[1], (s32)((const u32*)data)[0]);
        }
        break;
    }
}

/* The server's message callback (`setCallback(..., 3)` through `networkSessionReflectCallbackEx`): each code sets the
 * reply bits the waiting handlers poll, or updates the circle, player and match records directly. */
void networkSessionReflect1(NetworkSessionManagerPat* self, s32 code, s32 requestId, s32 flag, s32 count,
                            const u8* data)
{
    s32 i;

    switch (code) {
    case 0x8000:
    case 0x8007:
        for (i = 0; i < 21; i++) {
            if (self->requests_10[i] != NULL) {
                break;
            }
        }
        if (i == 21) {
            NetworkErrorInfo error;

            getErrorInfoOrCode654c(getInstance_(), 0x80050031, &error);
            self->postEvent(3, 0, error.code_00, 1, &error, self->unused_08);
        }
        for (i = 0; i < 21; i++) {
            self->requestFlags_30C[i] |= REQUEST_SESSION_LOST;
        }
        break;
    case 0x8006:
        self->requestFlags_30C[1] |= 0x10;
        self->requestFlags_30C[2] |= 0x10;
        for (i = 0; i < 21; i++) {
            self->requestFlags_30C[i] |= REQUEST_SESSION_LOST;
        }
        break;
    case 0x8002:
        self->requestFlags_30C[1] |= REQUEST_ABORTED;
        self->requestFlags_30C[2] |= REQUEST_ABORTED;
        for (i = 0; i < 21; i++) {
            if (requestId == self->requestIds_360[i]) {
                self->requestFlags_30C[i] |= REQUEST_ABORTED;
            }
        }
        break;
    case 0x8004:
        if (flag != 0) {
            for (i = 0; i < 21; i++) {
                self->requestFlags_30C[i] |= REQUEST_SESSION_LOST;
            }
        }
        break;
    case 0x8005:
        self->requestFlags_30C[1] |= 0x8;
        self->requestFlags_30C[2] |= 0x8;
        break;
    case 0x8008:
        self->requestFlags_30C[1] |= 0x20;
        break;
    case 0x8042:
        self->circleInfoRequestId_41C = *(const s32*)data;
        if (self->circleInfoRequestId_41C <= 0) {
            NetworkErrorInfo error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
            break;
        }
        self->limitA_528 = 0;
        self->usedA_530 = 0;
        self->selfIndex_536 = 0;
        self->hostIndex_537 = 0;
        self->addPlayerRecord(0, self->matchOptions_3EC.userId_08, self->matchOptions_3EC.name_10, 0, 0, 0);
        self->requestFlags_30C[3] |= 0x40;
        break;
    case 0x8054:
        self->listTotal_AEC = ((const s32*)data)[1];
        self->requestFlags_30C[4] |= 0x1000;
        break;
    case 0x8055:
        if (count <= 0) {
            NetworkErrorInfo error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
            break;
        }
        if (count > 32 - self->circleList_AF0.count_00) {
            count = 32 - self->circleList_AF0.count_00;
        }
        for (i = 0; i < count; i++) {
            self->addCircleInfo(self->circleList_AF0.count_00, &((const PatCircleRecord*)data)->info_000,
                                (PatCircleOptionList*)&((const PatCircleRecord*)data)->options_37C);
            data += sizeof(PatCircleRecord);
        }
        self->listCursor_AE8 += count;
        self->requestFlags_30C[4] |= 0x2000;
        break;
    case 0x8056:
        self->requestFlags_30C[4] |= 0x4000;
        break;
    case 0x8063:
        self->requestFlags_30C[1] |= 0x2000000;
        break;
    case 0x8053:
        self->circleList_AF0.count_00 = 0;
        self->field_6E74 = 1;
        for (i = 0; i < 32; i++) {
            networkPatResetCircleInfo(self, i);
        }
        for (i = 0; i < count; i++) {
            self->addCircleInfo(((const PatCircleRecord*)data)->info_000.slotNumber_36C - 1,
                                &((const PatCircleRecord*)data)->info_000,
                                (PatCircleOptionList*)&((const PatCircleRecord*)data)->options_37C);
            data += sizeof(PatCircleRecord);
        }
        self->requestFlags_30C[5] |= 0x800;
        break;
    case 0x8064:
        createCircleLayer(self, &((const PatCircleRecord*)data)->info_000,
                          (PatCircleOptionList*)&((const PatCircleRecord*)data)->options_37C);
        break;
    case 0x8065:
        changeCircleListLayer(self, &((const PatCircleRecord*)data)->info_000,
                              (PatCircleOptionList*)&((const PatCircleRecord*)data)->options_37C);
        break;
    case 0x8066:
        deleteCircleListLayer(self, *(const s32*)data);
        break;
    case 0x8044: {
        self->circleInfoRequestId_41C = *(const s32*)data;
        self->selfIndex_536 = ((const PatPlayerNotice*)data)->slot_04;
        if (self->circleInfoRequestId_41C <= 0 || ((const PatPlayerNotice*)data)->slot_04 < 0 ||
            self->getWord_524() <= ((const PatPlayerNotice*)data)->slot_04) {
            NetworkErrorInfo error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
            break;
        }
        self->addPlayerRecord(((const PatPlayerNotice*)data)->slot_04, self->matchOptions_3EC.userId_08,
                              self->matchOptions_3EC.name_10, 0, 0, 0);
        self->requestFlags_30C[6] |= 0x20000;
        break;
    }
    case 0x8045: {
        u8 slot = ((const PatPlayerNotice*)data)->slot_04;

        if ((s8)data[6] != 0 && (s8)slot >= 0 && (s8)slot < self->getWord_524()) {
            self->addPlayerRecord((s8)slot, ((const PatPlayerNotice*)data)->address_06,
                                  ((const PatPlayerNotice*)data)->name_0E, 0,
                                  ((const PatPlayerNotice*)data)->kind_05 == 2,
                                  self->requests_10[3] == NULL && self->requests_10[6] == NULL);
        }
        break;
    }
    case 0x8046:
        self->requestFlags_30C[11] |= 0x80000;
        break;
    case 0x8047: {
        u8 slot = ((const PatPlayerNotice*)data)->slot_04;

        if ((s8)data[6] != 0 && (s8)slot >= 0 && (s8)slot < self->getWord_524()) {
            self->removePlayerRecord((s8)slot, ((const PatPlayerNotice*)data)->kind_05 == 2);
        }
        break;
    }
    case 0x8043:
        self->circleOwnerId_420 = ((const PatCircleInfo*)data)->ownerId_368;
        memcpy(self->circleName_424, ((const PatCircleInfo*)data)->name_004, 63);
        self->circleName_424[63] = 0;
        self->limitA_528 = ((const PatCircleInfo*)data)->limitA_358;
        self->usedA_530 = ((const PatCircleInfo*)data)->usedA_35C;
        self->limitB_524 = ((const PatCircleInfo*)data)->limitB_360;
        self->usedB_52C = ((const PatCircleInfo*)data)->usedB_364;
        self->requestFlags_30C[3] |= 0x8000;
        self->requestFlags_30C[6] |= 0x8000;
        break;
    case 0x8051:
        if (requestId == self->requestIds_360[11]) {
            self->requestFlags_30C[11] |= 0x10000;
        }
        if (requestId == self->requestIds_360[8]) {
            self->requestFlags_30C[8] |= 0x10000;
        }
        break;
    case 0x805E:
        for (i = 0; i < count; i++) {
            u8 slot = ((const PatMemberEntry*)data)->slot_07;

            if ((s8)slot >= 0 && (s8)slot < self->getWord_524() && (s8)slot != self->selfIndex_536 &&
                self->players_538[(s8)slot].active_00 == 0) {
                self->addPlayerRecord((s8)slot, ((const PatMemberEntry*)data)->address_08,
                                      ((const PatMemberEntry*)data)->name_10,
                                      ((const PatMemberEntry*)data)->state_06 != 0, 0, 0);
            }
            data += sizeof(PatMemberEntry);
        }
        self->requestFlags_30C[6] |= 0x80;
        break;
    case 0x805C: {
        u8 slot = ((const PatPlayerNotice*)data)->slot_04;

        if ((s8)slot < 0 || (s8)slot >= self->getWord_524()) {
            NetworkErrorInfo error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
            break;
        }
        self->hostIndex_537 = (s8)slot;
        self->requestFlags_30C[6] |= 0x40000;
        break;
    }
    case 0x805D: {
        u8 slot = ((const PatPlayerNotice*)data)->slot_04;
        s8 subhost;

        if ((s8)slot >= 0 && (s8)slot < self->getWord_524() && self->players_538[(s8)slot].active_00 != 0 &&
            self->hostIndex_537 != (s8)slot) {
            self->hostIndex_537 = (s8)slot;
            if (self->players_538[(s8)slot].announced_01 != 0) {
                self->postEvent(14, (s8)slot, 0, 0, NULL, self->unused_08);
            }
            subhost = self->mapId_1C0(self->hostIndex_537);
            if (subhost >= 0 && self->buffer != NULL) {
                ((NetworkSessionStable*)self->buffer)->setSubhostIndex(subhost);
            }
        }
        break;
    }
    case 0x804A:
        self->requestFlags_30C[3] |= 0x100000;
        self->requestFlags_30C[6] |= 0x100000;
        if (requestId == self->requestIds_360[18]) {
            self->field_3C2 = 1;
            self->requestFlags_30C[18] |= 0x100000;
        }
        if (requestId == self->requestIds_360[11]) {
            self->matchOptions_3EC.mode_06 = 0;
            self->players_538[self->selfIndex_536].state_03 = 0;
            self->requestFlags_30C[11] |= 0x100000;
        }
        break;
    case 0x804B:
        self->updatePlayerRecord(((const PatMemberEntry*)data)->slot_07, NULL, NULL,
                                 ((const PatMemberEntry*)data)->state_06 != 0);
        break;
    case 0x804D:
        self->requestFlags_30C[7] |= 0x200000;
        break;
    case 0x804E: {
        const PatMatchMember* member = ((const PatMatchNotice*)data)->members_04;
        s32 index;
        u8 slot;

        self->matchMemberCount_664 = 0;
        self->matchPhase_66C = 0;
        memset(self->matchData_66E, 0, sizeof(self->matchData_66E));
        if (count > 4) {
            count = 4;
        }
        for (index = 0; index < count; index++, member++) {
            slot = member->slot_07;

            if ((s8)slot >= 0 && (s8)slot < self->getWord_524()) {
                NetworkSessionPlayerRecord* player = &self->players_538[(s8)slot];

                if (player->active_00 != 0) {
                    if ((s8)slot == self->selfIndex_536) {
                        self->matchPhase_66C = 1;
                        if (((const PatMatchNotice*)data)->timeout_0C >= 16 &&
                            ((const PatMatchNotice*)data)->timeout_0C <= 80) {
                            networkSessionTimeoutSeconds = ((const PatMatchNotice*)data)->timeout_0C;
                        }
                        if (((const PatMatchNotice*)data)->interval_10 >= 20 &&
                            ((const PatMatchNotice*)data)->interval_10 <= 100) {
                            networkSessionIntervalSeconds = ((const PatMatchNotice*)data)->interval_10;
                        }
                        if (((const PatMatchNotice*)data)->limit_18 >= 20 &&
                            ((const PatMatchNotice*)data)->limit_18 <= 100) {
                            networkSessionLimitSeconds = ((const PatMatchNotice*)data)->limit_18;
                        }
                        self->matchOptions_3EC.mode_06 = 2;
                    } else {
                        memcpy(player->address_40.ip_00, member->address_00.ip_00, sizeof(player->address_40.ip_00));
                        player->address_40.port_04 = member->address_00.port_04;
                    }
                    self->matchMembers_668[self->matchMemberCount_664++] = slot;
                }
            }
        }
        if (self->matchPhase_66C != 0 && ((const PatMatchNotice*)data)->kind_08 == 1 &&
            self->matchMemberCount_664 != 1) {
            self->matchPhase_66C = 0;
        }
        if (circleAvailable(self) != 0) {
            break;
        }
        if (self->field_3C2 != 0) {
            self->field_3C3 = 1;
            break;
        }
        if (self->players_538[self->selfIndex_536].announced_01 != 0) {
            if (self->matchPhase_66C != 0) {
                s8 subhost = self->mapId_1C0(self->hostIndex_537);

                if (subhost >= 0 && self->buffer != NULL) {
                    ((NetworkSessionStable*)self->buffer)->setSubhostIndex(subhost);
                } else {
                    NetworkErrorInfo error;

                    error.code_00 = 0x80000000;
                    error.param1_04 = 0;
                    error.param2_08 = 0;
                    ((NetworkInstanceDispatch*)getInstance_())->postError(*(NetworkPostedError*)&error);
                    break;
                }
            }
            self->postEvent(25, self->selfIndex_536, -(!self->matchPhase_66C), self->matchMemberCount_664,
                            self->matchMembers_668, self->unused_08);
        }
        break;
    }
    case 0x804F:
        if (requestId == self->requestIds_360[10]) {
            self->matchOptions_3EC.mode_06 = 1;
            self->players_538[self->selfIndex_536].state_03 = 1;
            self->requestFlags_30C[10] = 0x400000;
        }
        if (requestId == self->requestIds_360[11]) {
            self->matchOptions_3EC.mode_06 = 0;
            self->players_538[self->selfIndex_536].state_03 = 0;
            self->requestFlags_30C[11] = 0x400000;
        }
        break;
    case 0x805F: {
        const PatBinaryField* field = ((const PatBinaryNotice*)data)->fields_0C;

        if ((s8)((const PatBinaryNotice*)data)->address_00[0] != 0 && field->type_00 == 1) {
            u8 kind = field->kind_08;

            if (field->type_10 == 2) {
                u32 value = field->value_18;
                NetworkUniqueId address;
                s8 player;

                address.importFrom(3, data, 8);
                switch (kind) {
                case 2:
                    sendNtcCircleUserValueReply(getInstance_(), self->circleInfoRequestId_41C,
                                self->players_538[self->selfIndex_536].value_3C, data);
                case 1:
                    player = self->uniqueIdToMember(&address);
                    if (player >= 0) {
                        self->players_538[player].flag_02 = 1;
                        if (value != self->players_538[player].value_3C) {
                            self->players_538[player].value_3C = value;
                            if (self->players_538[player].announced_01 != 0) {
                                self->postEvent(21, player, 0, 1, &self->players_538[player].value_3C,
                                                self->unused_08);
                            }
                        }
                    }
                    break;
                case 3:
                    player = self->uniqueIdToMember(&address);
                    if (player >= 0 && value == 3) {
                        self->players_538[player].linked_04 = value;
                    }
                    break;
                }
            }
        }
        break;
    }
    case 0x8057:
        self->requestFlags_30C[17] |= 0x1000000;
        break;
    case 0x8049:
    case 0x8058:
        self->circleLost_535 = 1;
        if (self->field_3C2 != 0) {
            self->field_3C4 = (code == 0x8049) ? 0x80050037 : 0x80050036;
            break;
        }
        if (self->requests_10[6] == NULL && self->requests_10[11] == NULL) {
            if (self->players_538[self->selfIndex_536].announced_01 != 0) {
                NetworkErrorInfo error;

                error.code_00 = (code == 0x8049) ? 0x80050037 : 0x80050036;
                error.param1_04 = 0;
                error.param2_08 = 0;
                self->postEvent(12, 0, error.code_00, 1, &error, self->unused_08);
            }
            networkPatAttachBuffer(self);
        }
        break;
    case 0x8061:
        self->requestFlags_30C[15] |= 0x4000000;
        break;
    case 0x8060:
    case 0x8062:
        if ((s8)data[0] != 0 && (s8)data[0x108] != 0) {
            NetworkSessionSlotInfo message;
            s8 player;

            message.smallObject_00.importFrom(3, ((const PatServerChat*)data)->address_108, 8);
            memcpy(message.name_20, ((const PatServerChat*)data)->name_110, sizeof(message.name_20));
            message.nameEnd_33 = 0;
            memset(&message.flag_34, 0, sizeof(message.flag_34));
            memcpy(message.text_35, ((const PatServerChat*)data)->text_000, 0xFF);
            message.text_35[0xFF] = 0;
            message.textEnd_235 = 0;
            message.tag_238 = ((const PatServerChat*)data)->tag_100;
            message.time_23C = ((const PatServerChat*)data)->time_104;
            player = self->uniqueIdToMember(&message.smallObject_00);
            if (player >= 0) {
                self->postEvent(19, player, 0, 1, &message, self->unused_08);
            }
        }
        break;
    case 0x8067:
        self->requestFlags_30C[9] |= 0x8000000;
        break;
    case 0x8068:
        if ((s8)data[0] == 1) {
            self->matchPhase_66C = 0;
        }
        break;
    case 0x8069:
        memcpy(self->matchData_66E, data, sizeof(self->matchData_66E));
        break;
    }
}

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
    item->smallObject_108.clear();
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
    item->smallObject_108.importFrom(3, info->address_370, sizeof(info->address_370));
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
    player->smallObject_08.clear();
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
    NetworkUniqueId object;

    if ((u8)index > 3) {
        return;
    }
    player = &this->players_538[index];
    if (player->active_00 == 0) {
        return;
    }
    if (address != NULL) {
        object.importFrom(3, address, 8);
        player->smallObject_08.copyFrom((const u8*)&object);
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

/* Packs the set conditions of `src` (at most eight, the list's count clamped in place) into search filters,
 * mapping each condition kind to the server's operator; returns how many were packed (at most `max`). */
s32 NetworkSessionManagerPat::packCircleConditions(PatCircleFilter* dst, s32 max, PatConditionList* src)
{
    s32 count;
    PatCondition* condition;
    u32 i;

    if (dst == NULL || max <= 0 || src == NULL) {
        return 0;
    }
    count = 0;
    if (src->count_04 > 8) {
        src->count_04 = 8;
    }
    for (i = 0, condition = src->entries_08; i < src->count_04; condition++, i++) {
        dst->slot_04 = condition->slot_00 + 1;
        switch (condition->kind_01) {
        case 1:
            dst->op_00 = 5;
            break;
        case 2:
            dst->op_00 = 6;
            break;
        case 3:
            dst->op_00 = 4;
            break;
        case 4:
            dst->op_00 = 3;
            break;
        case 5:
            dst->op_00 = 2;
            break;
        case 6:
            dst->op_00 = 1;
            break;
        default:
            continue;
        }
        if (condition->enabled_04 == 1) {
            dst->enabled_05 = 1;
            dst->value_08 = condition->value_08;
            dst++;
            count++;
            if (count >= max) {
                break;
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
    s32 i;

    if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
        NetworkUniqueId id;

        getNetworkLayerPat(getPatsObject(), 0)->getMemberAddress(value, &id);
        if (id.isValid() != 0) {
            for (i = 0; i < 4; i++) {
                if (this->players_538[i].active_00 != 0 &&
                    id.equals(&this->players_538[i].smallObject_08) != 0) {
                    return i;
                }
            }
        }
    }
    return -1;
}

/* The player slot whose address object equals `id`; -1 (and a warning) when none does. */
s32 NetworkSessionManagerPat::uniqueIdToMember(const NetworkUniqueId* id)
{
    s32 i;

    if (id != NULL) {
        for (i = 0; i < 4; i++) {
            if (this->players_538[i].active_00 != 0 &&
                id->equals(&this->players_538[i].smallObject_08) != 0) {
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

/* Opens a session slot (kind 6) for the layer member at `address` and connects it with the GameSpy thread;
 * the slot, 0 for this console's own address, -1 while the session is not ready, -2 when no slot is free. */
s8 NetworkSessionManagerPat::connectPeer(const NetworkUniqueId* address, u32 value)
{
    PatPeerConnect peer;
    s32 slot;

    if (GameSpyInterfaceThread::getInstance() == NULL || this->buffer == NULL || address == NULL || value == 0 ||
        this->field_3C8 == 0) {
        return -1;
    }
    if (this->field_3CC.equals(address) != 0) {
        return 0;
    }
    slot = ((NetworkSessionBase*)this->buffer)->set(6, (const u8*)address);
    if ((s8)slot < 0) {
        return -2;
    }
    peer.value_04 = value;
    peer.thread_00 = GameSpyInterfaceThread::getInstance();
    ((NetworkSessionBase*)this->buffer)->connect((s8)slot, (u32)&peer, 1);
    return (s8)slot;
}

/* Sends the error record (when given) and the GameSpy thread's own error (when set) to the server. */
void NetworkSessionManagerPat::reportSessionError(const NetworkErrorInfo* info)
{
    u32 values[3];
    NetworkErrorInfo threadError;

    if (info != NULL) {
        values[0] = info->code_00;
        values[1] = info->param1_04;
        values[2] = info->param2_08;
        sendServerTimeout(getInstance_(), values);
    }
    if (GameSpyInterfaceThread::getInstance() != NULL) {
        GameSpyInterfaceThread::getInstance()->getErrorStruct(&threadError);
        if (threadError.code_00 != 0) {
            values[0] = threadError.code_00;
            values[1] = threadError.param1_04;
            values[2] = threadError.param2_08;
            sendServerTimeout(getInstance_(), values);
        }
    }
}

/* Hands an event to the callback at +0x04; an error event without a code of its own (a negative value with a
 * record, other than events 12, 28 and 35) takes the singleton's pending error first. */
/* untyped: caller-owned payload - an error record, an index or a state byte, by event */
void NetworkSessionManagerPat::postEvent(s32 code, s8 slot, s32 value, s32 kind, const void* payload, u32 context)
{
    if (value < 0 && payload != NULL && code != 28 && code != 35 && code != 12 && getInstance_() != NULL &&
        getErrorInfo654c(getInstance_(), NULL) != 0) {
        getErrorInfoOrCode654c(getInstance_(), 0x80050031, (NetworkErrorInfo*)payload);
        value = ((const NetworkErrorInfo*)payload)->code_00;
    }
    ((PatErrorCallback)this->unused_04)(code, slot, value, kind, (NetworkErrorInfo*)payload, context);
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
        memcpy(&this->conditionList_7E8, src, sizeof(this->conditionList_7E8));
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

/* Reads the 10-byte chat header at `data` into `out`. */
void NetworkSessionManagerPat::readChatHeader(PatChatHeader* out, const u8* data)
{
    u8 target;
    NetworkUnitPacket packet;

    packet.bind((u8*)data, 10);
    ((NetworkByteStream*)&packet)->takeByte(&out->kind_00);
    ((NetworkByteStream*)&packet)->takeByte(&target);
    ((NetworkByteStream*)&packet)->takeU32(&out->sender_04);
    ((NetworkByteStream*)&packet)->takeU32(&out->tag_08);
    out->target_01 = target;
}

/* Sends a chat packet over the session: the header, this console's address and the text, to the target's
 * layer slot (or to everyone for -1); nothing when the target has no slot. */
void NetworkSessionManagerPat::sendSessionChat(const char* text, u32 tag, s8 target)
{
    if (this->buffer != NULL) {
        u8 buffer[0x280];
        NetworkUnitPacket packet;
        NetworkPeerRecord record;
        s8 slot;

        record.data_00 = (void*)text;
        record.size_04 = strlen(text);
        packet.attach(buffer, sizeof(buffer));
        ((NetworkByteStream*)&packet)->putByte(2);
        ((NetworkByteStream*)&packet)->putByte(target);
        ((NetworkByteStream*)&packet)->putU32(this->selfIndex_536);
        ((NetworkByteStream*)&packet)->putU32(tag);
        ((NetworkByteStream*)&packet)->pullRecord((NetworkStreamSink*)&this->field_3CC);
        ((NetworkByteStream*)&packet)->putRecord(&record);
        slot = -2;
        if (target != -1) {
            slot = mapId_1C0(target);
            if (slot < 0) {
                return;
            }
        }
        ((NetworkSessionBase*)this->buffer)->sendOp4(((NetworkByteStream*)&packet)->getData(),
                                                     ((NetworkByteStream*)&packet)->getSize(), slot);
    }
}

/* Reports a chat packet received over the session (event 19) when it comes from an announced player whose
 * address matches the one the packet carries. */
void NetworkSessionManagerPat::receiveSessionChat(const u8* data, u32 size)
{
    u8 text[0x200];
    NetworkUnitPacket packet;
    NetworkUniqueId sender;
    NetworkPeerRecord record;
    u32 senderWord;
    u32 tag;
    u8 kind;
    u8 target;
    s8 index;
    u32 length;

    record.data_00 = text;
    record.size_04 = sizeof(text);
    packet.bind((u8*)data, size);
    ((NetworkByteStream*)&packet)->takeByte(&kind);
    ((NetworkByteStream*)&packet)->takeByte(&target);
    ((NetworkByteStream*)&packet)->takeU32(&senderWord);
    ((NetworkByteStream*)&packet)->takeU32(&tag);
    ((NetworkByteStream*)&packet)->forwardRecord((NetworkStreamSink*)&sender);
    ((NetworkByteStream*)&packet)->takeRecord(&record);
    index = senderWord;
    if (index < 0 || index >= getWord_524()) {
        return;
    }
    if (index == this->selfIndex_536) {
        return;
    }
    if (this->players_538[index].announced_01 != 0) {
        NetworkSessionSlotInfo message;

        if (sender.equals(&this->players_538[index].smallObject_08) == 0) {
            return;
        }
        message.smallObject_00.copyFrom((const u8*)&sender);
        memcpy(message.name_20, this->players_538[index].name_28, sizeof(message.name_20));
        message.nameEnd_33 = 0;
        memset(&message.flag_34, 0, sizeof(message.flag_34));
        length = (record.size_04 < sizeof(message.text_35) - 1) ? record.size_04 : sizeof(message.text_35) - 1;
        memcpy(message.text_35, record.data_00, length);
        message.text_35[length] = 0;
        message.textEnd_235 = 0;
        message.tag_238 = tag;
        message.time_23C = getServerDateTime(getInstance_());
        postEvent(19, index, 0, 1, &message, this->unused_08);
    }
}

/* Reports `code`/`arg_a`/`arg_b` to the server and records them as the request's error. */
void NetworkSessionManagerPat::setSessionLog(NetworkRequest* request, u32 code, u32 arg_a, u32 arg_b)
{
    u32 values[3];

    values[0] = code;
    values[1] = arg_a;
    values[2] = arg_b;
    sendServerTimeout(getInstance_(), values);
    clearErrorRecord613c(getInstance_());
    request->setRecord(code, arg_a, arg_b);
}

/* Records the cancelled-request error (0x80050012) the singleton builds as the request's error. */
void NetworkSessionManagerPat::setSessionLogAborted(NetworkRequest* request)
{
    NetworkErrorInfo info;

    buildErrorInfo613c(getInstance_(), 0x80050012, &info);
    request->setRecord(info.code_00, info.param1_04, info.param2_08);
}

/* Records the session-lost error (the singleton's own record, else 0x80050031) as the request's error. */
void NetworkSessionManagerPat::setSessionLogSessionLost(NetworkRequest* request)
{
    NetworkErrorInfo info;

    getErrorInfoOrCode654c(getInstance_(), 0x80050031, &info);
    request->setRecord(info.code_00, info.param1_04, info.param2_08);
}
