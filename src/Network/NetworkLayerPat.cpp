/*
 * Network/NetworkLayerPat.cpp - the `NetworkLayerPat` layer (derived from `NetworkLayer`): the constructor, the request
 *   handlers and their state machine `stepRequest`, the member helpers and the three `setCollectionLog*` siblings.
 * RANGE. .text 0x803E0BE8-0x803EF668 (158 functions); .data 0x805FB718-0x805FC390 (the class's strings, its table
 *   `__vt__15NetworkLayerPat` 0x805FC1E0, then the two inline `NetworkRequest::getArgument` strings
 *   0x805FC324/0x805FC358 the handlers at 0x803E3C70/0x803E9C00 read - not a seam), .sdata 0x80793940-0x80793948, .sbss
 *   0x80794CA8-0x80794CB0, .sdata2 0x8079C780-0x8079C7A0 (read only from this range), extab, extabindex.  The edges
 *   mirror the session manager's shape: `NetworkLayer`'s last function `setFlag76` before (as the manager's
 *   `setSessionLog*` trio before `NetworkLayer`), `NetworkCommunity`'s constructor (it stores 0x805FC440) after.
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md); file-scope `#pragma peephole off`
 *   (playbook 39: retail keeps `clrlwi`+`cmpwi` unfused).
 * NAMES. The class name is the pool strings' ("NetworkLayerPat::move ...").  The runtime dump names none of the
 *   functions below, so each is derived from its body, one GUESS line per group:
 *   GUESS (state machine, slot +0x104, issues `sendReqLayerUp`/`ChildInfo`/`UserList`): `stepRequest`.
 *   GUESS (request handlers, from the `sendReq*` they issue): `handleServerList`, `handleServerSelect`, `handleLayerInfo`.
 *   GUESS (request handlers, from the `sendReq*` they issue): `handleLayerCreate`, `handleLayerJump`, `handleChat`.
 *   GUESS (slot-offset accessors, the consumers' `_XX` scheme): `sendUserFields_5C`, `readSelectedServer_88`.
 *   GUESS (slot-offset accessors, the consumers' `_XX` scheme): `setPresence_A0`, `getCommunityValueE0_B4`.
 *   GUESS (voice/NAT helpers): `findFriendByPeerId`, `sendPairState`, `sendPeerRecord`, `checkMemberSessions`.
 *   GUESS (friend and server helpers): `pollFriendSlot`, `exportServerRec`, `getPatTerms` (menu_plsearch's terms).
 *   GUESS (the leave-and-reread runs the failing handlers finish with): `stepLayerLeave`, `stepChildListRead`.
 *   GUESS (the own pool): `allocRequest`, `deleteRequest`, `requestLayerCreate`, `handleLayerReserve` (event 36).
 *   GUESS (own request type; retail carries a second copy of every member): `NetworkLayerPatRequest`.
 *   GUESS (its members, `NetworkLayerRequest`'s names for the same bodies): `clear`, `reset`, `isOwned`, `run`.
 *   GUESS (its members, `NetworkLayerRequest`'s names for the same bodies): `begin`, `getRecord`, `getArgument`.
 *   GUESS (its members, `NetworkLayerRequest`'s names for the same bodies): `setRecord`.
 *   GUESS (reflect helpers 0x803EA82C..0x803EB678): `importLayerInfo`, `importCommunityRec`, `importFriendRec`.
 *   GUESS (reflect helpers 0x803EA82C..0x803EB678): `storeFriendDetail`, `findFriendByUserId`, `addFriendSlot`.
 *   GUESS (reflect helpers 0x803EA82C..0x803EB678): `removeFriendSlot`, `unpackLayerSettings`.
 *   GUESS (records and fields): `restoreSentProfile` (NetworkCommunityPat), `NetLayerInfoRec`, `jumpTicket_3D8`.
 *   GUESS (records and fields): `matchKey_0x00F0`; the callee `sendNtcLayerUserTransfer` (0x80403230).
 *   `strtok` (0x8045F858) is the C library's own name, which the dump does not carry - not a GUESS.
 *   Callees named at integration: `sendReqLayerUserInfoSet` (0x80401AF4, recvAnsLayerUserInfoSet's slot),
 *   `sendReqLayerTell` (0x8040211C, the `sendReqCircleTell` shape).
 * RESIDUALS. 7 rows unwritten: `reflect` (0x803EBB9C, 8976 B) and three inline ctor/dtor pairs (0x803E6D7C/0x803E6DD8 after `handleChat`, 0x803EDEAC..0x803EDF94
 *   after `reflect`) - copies of `NetworkSessionSlotInfo`'s and two other records' members whose manglings
 *   `Network/NetworkSessionManager.cpp` already defines, so the map cannot carry them twice; our calls reach those
 *   copies by name.  `.data` cannot match until `reflect`'s tables and strings are emitted (`vtableaudit` keeps the
 *   0x805FC1E8 run); `flipcheck`: the claimed `.sdata` 8 B is not emitted.  The inline members of
 *   `NetworkLayerPatRequest` are defined at the end of the file, callees after callers, so no call is inlined.
 *   Not unwritten: the empty constructor/destructor bodies of `NetFriendNotice`, `NetFriendStatusNotice` and
 *   `NetLayerMediationEntry` are complete - MWCC emits the member construction and destruction (all at 100 %).
 *   Partial rows:
 *  - `moveRequests`: the unrolled scan's trip count in r25 (the base's own `move` residual), so retail saves from r20
 *    (`_savegpr_20`/`_restgpr_20`) where ours saves from r21;
 *  - `handleChat`: its message record's constructor and destructor are the inline copies `fn_803E6DD8`/`dtor_803E6D7C` in
 *    retail; ours calls `NetworkSessionSlotInfo`'s own members (the copies cannot be named, see above);
 *  - `clear`: two instructions scheduled apart at the end;
 *  - `setFriendTransferModeById`: retail indexes the mode array as `lbzx` off `this` (u8/u32 mode, peephole on/off and
 *    an entry pointer tried);
 *  - `sendUserPosition_60`, `applyLoginRecord`, `readSelectedServer_88`, `readUserRows`, `handleUserSearch`,
 *    `pollFriendSlot`: a float compare's operand order, an early return's `bge`+`b` pair, a bool store's `clrlwi`, the
 *    `addis`/`mulli` order of an unrolled remainder, two registers swapped;
 *  - `setFriendTransferMode_E0`: retail passes the u32 mode to the mediator's `setTransferSlotMode` unnarrowed; a u32
 *    parameter there lowers the mediator's own row, so the u8 stays;
 *  - `downsampleVoice`: retail tests the count with `cmplwi`+`blelr` (`i != 0` and `i > 0` both give `cmpwi`+`beqlr`);
 *  - `sendUserFields_5C`: two registers swapped;
 *  - `move`: two `pairState_C7F0` reads use `lbzx` off a precomputed row (the voice-ready result is the mediator's
 *    `u32` `isVoiceReady`, kept in a `u32` local: compared and stored without a mask, as retail);
 *  - `handleLayerCreate`, `handleLayerJump`: retail loads the friend peer's `port_0C` with `lhz` for the `s16`
 *    `setBufferSize` argument (ours `lha`), and gives `getFmpSelected`'s result (and in `handleLayerJump` two more
 *    temporaries) a different callee-saved register; declaration orders tried;
 *  - `addFriendSlot`: the voice-ready and accepted-peer results are stored to their byte arrays after a `clrlwi` retail
 *    does not have (a u8 cast, a u8/u32 local tried); `storeFriendDetail`: retail keeps `index * 0x104` for the copy's
 *    destination, ours multiplies again; `importCommunityRec`: the state byte's `clrlwi` (a file-wide peephole on
 *    costs more); `removeFriendSlot`: the unrolled clear's remainder `addis`/`mulli` order.
 * SHAPES. The constructor calls `NetworkLayer::NetworkLayer` and stores 0x805FC1E0 (79 slots); the table is emitted here
 *   (key function `stepRequest`, declared first; rule 10) and every override's map row carries the compiler's mangling.
 *  - every handler is the state machine over `NetworkLayerRequest::state_00` (0 start, waits, 100 cancelled, 110
 *    failed); `requestFlags_310[slot]`/`requestIds_368[slot]` are the reply bits and ids `reflect` sets (slot 21 is the
 *    layer's own, +0x1C0);
 *  - the `sendReqLayer*` results are declared `u32` (the caller stores the whole register, playbook 66); +0xF19C is
 *    reached with `addis`+`lwz`, what a real offset that size compiles to;
 *  - locals are laid out in reverse declaration order (`handleLayerInfo` declares the field list first); a list read
 *    reuses its count variable as the batch's first row (`handleChildList`); the filter packers walk a pointer beside
 *    the counter, incremented first (`packLayerSettings`);
 *  - `setFlag75` and the base's slot take `u32` (no `clrlwi`; the base's store under a local `#pragma peephole on`);
 *    `popTransferRecord` returns `s32` and `GameSpyInterfaceThread::isNegotiating` `BOOL`, as `move` uses them;
 *  - the friend table holds real `NetworkUniqueId`s (`NetFriendRec`); the records with a unique id inside
 *    (`NetFriendRec`, `NetFriendTable`, `NetFriendList`, `NetLayerRequest`, `NetCommunityList`, `NetLayerMediationList`,
 *    the two notices) declare and define their ctors/dtors out of line (retail calls them); the layer and each community
 *    record keep table and sessions as separate members; `NetFriendRoster` is only `handleUserList`'s stack payload;
 *  - records the handlers send are views in `Network/NetworkLayerPat.h`; the layer requests' field list and record are
 *    `PatInterface.cpp`'s `PatTagList`/`PatLayerData`, the address its 16-byte `path`;
 *  - `handleConnect`'s 0x80060034 immediate (0x803E30F4) is a `config.yml` relocation block, as for `updateSession`.
 */
#include "Network/NetworkLayerPat.h"
#include "Network/NetworkPat.h"
#include "Network/NetworkCommunityPat.h"   /* restoreSentProfile - the community layer's profile */
#include "Network/network_pat_control.h"
#include "enemy/em020_ai.h"
#include "unsplit/Network.h"
#include "Network/NetworkSessionManagerPat.h"
#include "Network/NetworkSessionBase.h"   /* LockMutex/UnlockMutex - owner Network/NetworkSessionBase.cpp */
#include "Network/gamespy_interface_types.h"  /* GameSpyInterfaceThread / NetworkErrorInfo - owner Network/GameSpyInterfaceThread.cpp */
#include "Network/NetworkWiiMediator.h"   /* the mediator's terms flag and transfer slots - owner Network/NetworkWiiMediator.cpp */
#include "sound/fn_800E46E8.h"            /* getInstance (the mediator singleton) - owner sound/fn_800E46E8.cpp */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL/strlen.h"
#include "menu/menu_plsearch.h"           /* sPatTerms - owner menu/menu_plsearch.cpp */
#include "Network/PatInterface.h"          /* getErrorInfo654c / buildErrorInfo613c / clearErrorRecord613c - owner Network/PatInterface.cpp */


/* The log codes the request reports (0x8006xxxx = the layer's own error range). */
enum {
    LAYER_ERR_NOT_CONNECTED = 0x80060001,
    LAYER_ERR_BAD_ARGUMENT = 0x80060002,
    LAYER_ERR_CANNOT_START = 0x80060011,
    LAYER_ERR_SERVER = 0x80060012,
    LAYER_ERR_NO_LAYER = 0x80060032,
    LAYER_ERR_SERVER_REJECTED = 0x80060036   /* GUESS: the selected FMP server's info state reads 5 */
};

/* The request slots (`NetworkLayer::requests_0C`'s index, also the index of `requestFlags_310`/`requestIds_368`). */
enum {
    SLOT_CONNECT = 1,        /* `closeSession_1C` -> `handleConnect` */
    SLOT_DISCONNECT = 2,     /* `shutdown_20` -> `handleDisconnect` */
    SLOT_SERVER_LIST = 3,    /* `requestServers_24` -> `handleServerList` */
    SLOT_SERVER_SELECT = 4,  /* `selectServer_28` -> `handleServerSelect` */
    SLOT_LAYER_UP = 5,       /* `request_38` -> `stepRequest` */
    SLOT_LAYER_CREATE = 6,   /* `request_40` -> `handleLayerCreate` */
    SLOT_LAYER_INFO = 7,     /* `request_44` -> `handleLayerInfo` */
    SLOT_CHILD_LIST = 8,     /* `requestCities_48` -> `handleChildList` */
    SLOT_SIBLING_LIST = 9,   /* `request_4C` -> `handleSiblingList` */
    SLOT_USER_LIST = 10,     /* the user-list reader `readUserRows` */
    SLOT_USER_INFO = 11,         /* `request_58` -> `handleUserInfo` */
    SLOT_CHAT = 12,              /* `sendMessage_64` -> `handleChat` */
    SLOT_DETAIL_SEARCH = 13,     /* `setPageSize_68` -> `handleDetailSearch` */
    SLOT_USER_SEARCH = 14,       /* `handleUserSearch`'s own search */
    SLOT_LAYER_JUMP = 15,        /* `request_70` -> `handleLayerJump` */
    SLOT_LAYER_INFO_BY_ID = 16,  /* `request_74` -> `handleLayerInfoById` */
    SLOT_LAYER_INFO_SET = 17,    /* `request_78` -> `handleLayerInfoSet` */
    SLOT_MEDIATION_LOCK = 18,    /* `request_7C` -> `handleMediationLock` */
    SLOT_MEDIATION_UNLOCK = 19,  /* `request_80` -> `handleMediationUnlock` */
    SLOT_MEDIATION_LIST = 20,    /* `requestAccount_84` -> `handleMediationList` */
    SLOT_LAYER_RESERVE = 21      /* the layer's own request (`ownRequests_1C0`) -> `handleLayerReserve` */
};

/* The events `notifyLayerEvent` reports. */
enum {
    EVENT_CONNECT = 1,
    EVENT_DISCONNECT = 2,
    EVENT_ERROR = 3,
    EVENT_LAYER_UP = 4,
    EVENT_FRIEND = 6,
    EVENT_LAYER_INFO = 8,
    EVENT_CHILD_LIST = 9,
    EVENT_SIBLING_LIST = 10,
    EVENT_USER_LIST = 11,
    EVENT_USER_INFO = 12,
    EVENT_FRIEND_STATUS = 13,
    EVENT_CHAT = 15,
    EVENT_SERVER_LIST = 18,
    EVENT_SERVER_SELECT = 19,
    EVENT_DETAIL_SEARCH = 25,
    EVENT_USER_SEARCH = 26,
    EVENT_LAYER_INFO_BY_ID = 28,
    EVENT_LAYER_INFO_SET = 29,
    EVENT_MEDIATION_LOCK = 31,
    EVENT_MEDIATION_UNLOCK = 33,
    EVENT_MEDIATION_LIST = 35,
    EVENT_VOICE = 39,
    EVENT_SESSION_FAILED = 40
};

/* The request's steps. */
enum {
    STEP_START = 0,
    STEP_WAIT_UP = 5,
    STEP_LEAVE = 10,
    STEP_WAIT_CHILD_INFO = 15,
    STEP_USER_LIST = 20,
    STEP_WAIT_USER_LIST = 25,
    STEP_DONE = 30,
    STEP_CANCELLED = 100,
    STEP_FAILED = 110
};

/* `requestFlags_310` reply bits. */
enum {
    FLAG_SESSION_LOST = 0x1,
    FLAG_CANCELLED = 0x2,
    FLAG_UP_REPLY = 0x200,
    FLAG_CHILD_INFO_REPLY = 0x100000,
    FLAG_USER_LIST_REPLY = 0x400,
    FLAG_USER_HEAD_REPLY = 0x800,
    FLAG_USER_DATA_REPLY = 0x1000,
    FLAG_USER_FOOT_REPLY = 0x2000,
    FLAG_LAYER_START_REPLY = 0x40,
    FLAG_MEDIATION_LOCK_REPLY = 0x40,
    FLAG_MEDIATION_UNLOCK_REPLY = 0x80,
    FLAG_MEDIATION_LIST_REPLY = 0x100,
    FLAG_USER_INFO_REPLY = 0x4000,
    FLAG_LIST_HEAD_REPLY = 0x8000,
    FLAG_LIST_DATA_REPLY = 0x10000,
    FLAG_LIST_FOOT_REPLY = 0x20000,
    FLAG_LAYER_INFO_BY_ID_REPLY = 0x40000,
    FLAG_LAYER_INFO_REPLY = 0x80000,
    FLAG_SEARCH_HEAD_REPLY = 0x1000000,
    FLAG_SEARCH_DATA_REPLY = 0x2000000,
    FLAG_SEARCH_FOOT_REPLY = 0x4000000,
    FLAG_TELL_REPLY = 0x10000000,
    FLAG_STATE3_REPLY = 0x8,            /* GUESS on the names of the five server-select bits */
    FLAG_SHUT_REPLY = 0x10,
    FLAG_FMP_INFO_REPLY = 0x20,
    FLAG_LAYER_END_REPLY = 0x80,
    FLAG_LAYER_JUMP_REPLY = 0x8000000,
    FLAG_LAYER_INFO_SET_REPLY = 0x20000000,
    FLAG_DOWN_REPLY = 0x100,            /* the layer-create slot's own bits */
    FLAG_HOST_REPLY = 0x40000,
    FLAG_CREATE_HEAD_REPLY = 0x200000,
    FLAG_CREATE_SET_REPLY = 0x400000,
    FLAG_CREATE_FOOT_REPLY = 0x800000,
    FLAG_JUMP_READY_REPLY = 0x400,      /* the layer-jump slot's own bits */
    FLAG_JUMP_GO_REPLY = 0x800
};

/* The layer-create request's event. */
enum {
    EVENT_LAYER_CREATE = 5,
    EVENT_LAYER_JUMP = 27,
    EVENT_LAYER_RESERVE = 36
};

#pragma peephole off

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

s32 NetworkLayerPat::stepRequest(NetworkLayerRequest* request)
{
    NetworkRequestError error;

    switch (request->state_00) {
    case STEP_START:
        if (connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (memberCount_4C8 <= 0) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (pendingRequestId_F19C >= 0) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        requestFlags_310[SLOT_LAYER_UP] = 0;
        requestIds_368[SLOT_LAYER_UP] = sendReqLayerUp(getInstance_());
        request->state_00 = STEP_WAIT_UP;
        break;

    case STEP_WAIT_UP:
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_UP_REPLY) {
            request->state_00 = STEP_LEAVE;
        }
        break;

    case STEP_LEAVE:
        if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
            closeNetworkSessionManagerPat(getNetworkSessionManagerPat(getPatsObject(), 0));
        }
        if (GameSpyInterfaceThread::getInstance() != NULL) {
            GameSpyInterfaceThread::getInstance()->canClose();
            GameSpyInterfaceThread::getInstance()->initialize();
        }
        requestFlags_310[SLOT_LAYER_UP] = 0;
        requestIds_368[SLOT_LAYER_UP] = sendReqLayerChildInfo(getInstance_(), -1, 0);
        request->state_00 = STEP_WAIT_CHILD_INFO;
        break;

    case STEP_WAIT_CHILD_INFO:
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_CHILD_INFO_REPLY) {
            if (hostMode_4C4 != 0) {
                request->state_00 = STEP_USER_LIST;
            } else {
                request->state_00 = STEP_DONE;
            }
        }
        break;

    case STEP_USER_LIST:
        requestFlags_310[SLOT_LAYER_UP] = 0;
        requestIds_368[SLOT_LAYER_UP] = sendReqLayerUserList(getInstance_());
        request->state_00 = STEP_WAIT_USER_LIST;
        break;

    case STEP_WAIT_USER_LIST:
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_310[SLOT_LAYER_UP] & FLAG_USER_LIST_REPLY) {
            request->state_00 = STEP_DONE;
        }
        break;

    case STEP_DONE:
        busy_3D1 = 0;
        notifyLayerEvent(4, 0, 0, NULL, context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return true;

    case STEP_CANCELLED:
        busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(4, error.code_00, 1, &error, context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return true;

    case STEP_FAILED:
        busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(4, error.code_00, 1, &error, context_08);
        notifyLayerEvent(3, error.code_00, 1, &error, context_08);
        return true;
    }
    return false;
}

/* Copies the server's unique id into `id`. */
void NetworkLayerPat::readServerId_2C(NetworkUniqueId* id)
{
    if (id != NULL) {
        id->copyFrom((const u8*)&this->serverId_3DC);
    }
}

/* Copies the server name (at most 19 characters) into `out`. */
void NetworkLayerPat::readServerName_30(char* out, s32 size)
{
    u32 length;

    if (size > 0) {
        length = size - 1;
        length = (length < 19) ? length : 19;
        if (length != 0) {
            memcpy(out, this->serverName_3FC, length);
        }
        out[length] = 0;
    }
}

/* This layer has no server text: clears `out`. */
void NetworkLayerPat::readServerText_34(char* out, s32 size)
{
    if (size > 0) {
        out[0] = 0;
    }
}

/* Builds this console's layer id (kind 3, its 16 id bytes) in `out`. */
void NetworkLayerPat::exportLayerId_8C(NetworkLayerId* out)
{
    if (out != NULL) {
        NetworkLayerIdImportFrom(out, 3, (const u8*)&this->address_474, 16);
    }
}

/* Copies the user name (at most `size` - 1 characters, 63 at most) into `out`. */
void NetworkLayerPat::readUserName_90(char* out, s32 size)
{
    if (size > 0) {
        if (size > 64) {
            size = 64;
        }
        if (size - 1 > 0) {
            memcpy(out, this->userName_484, size - 1);
        }
        out[size - 1] = 0;
    }
}

/* Stores `text` (at most 63 characters) as the comment. */
void NetworkLayerPat::setComment_94(const char* text)
{
    u32 length;

    if (strlen(text) < 63) {
        length = strlen(text);
    } else {
        length = 63;
    }
    if (length != 0) {
        memcpy(this->comment_F060, text, length);
    }
    this->comment_F060[length] = 0;
}

/* Stores the settings record. */
void NetworkLayerPat::submitSettings_98(NetLayerSettings* settings)
{
    if (settings != NULL) {
        memcpy(&this->settings_F0A0, settings, sizeof(NetLayerSettings));
    }
}

/* Stores the request record. */
void NetworkLayerPat::submitRequest_9C(NetLayerRequest* request)
{
    if (request != NULL) {
        copyNetLayerRequest(&this->request_F0C4, request);
    }
}

/* Stores the select record. */
void NetworkLayerPat::submitSelect_A4(NetLayerRequest* request)
{
    if (request != NULL) {
        copyNetLayerRequest(&this->select_F130, request);
    }
}

/* The number of communities in the list. */
s32 NetworkLayerPat::getCommunityCount_A8()
{
    return this->communities_F1AC.count_0x00000;
}

/* Community `index`'s state byte, 0 out of range. */
u8 NetworkLayerPat::getCommunityState_AC(s32 index)
{
    if (index < 0 || index >= getCommunityCount_A8()) {
        return 0;
    }
    return this->communities_F1AC.entries_0x00004[index].state_0x0118;
}

/* Copies community `index`'s comment (at most `size` - 1 characters, 63 at most) into `out`. */
void NetworkLayerPat::readCommunityComment_B0(s32 index, char* out, s32 size)
{
    if (size > 0) {
        if (index < 0 || index >= getCommunityCount_A8()) {
            out[0] = 0;
        } else {
            if (size > 64) {
                size = 64;
            }
            if (size - 1 > 0) {
                memcpy(out, this->communities_F1AC.entries_0x00004[index].comment_0x00A0, size - 1);
            }
            out[size - 1] = 0;
        }
    }
}

/* Community `index`'s +0xE0 word, 0 out of range. */
u32 NetworkLayerPat::getCommunityValueE0_B4(s32 index)
{
    if (index < 0 || index >= getCommunityCount_A8()) {
        return 0;
    }
    return this->communities_F1AC.entries_0x00004[index].value_0x00E0;
}

/* Community `index`'s +0xE4 word, 0 out of range. */
u32 NetworkLayerPat::getCommunityValueE4_B8(s32 index)
{
    if (index < 0 || index >= getCommunityCount_A8()) {
        return 0;
    }
    return this->communities_F1AC.entries_0x00004[index].value_0x00E4;
}

/* Community `index`'s +0xE8 word, 0 out of range. */
u32 NetworkLayerPat::getCommunityValueE8_BC(s32 index)
{
    if (index < 0 || index >= getCommunityCount_A8()) {
        return 0;
    }
    return this->communities_F1AC.entries_0x00004[index].value_0x00E8;
}

/* Copies community `index`'s settings record into `out` (count 0 out of range). */
void NetworkLayerPat::readCommunitySettings_C0(s32 index, NetLayerSettings* out)
{
    if (out != NULL) {
        if (index < 0 || index >= getCommunityCount_A8()) {
            out->count_0x00 = 0;
        } else {
            memcpy(out, &this->communities_F1AC.entries_0x00004[index].settingsCount_0x00F4, sizeof(NetLayerSettings));
        }
    }
}

/* Copies community `index`'s 64-byte header into `out` (zeroes out of range). */
void NetworkLayerPat::readCommunityHeader_C4(s32 index, u8* out)
{
    if (out != NULL) {
        if (index < 0 || index >= getCommunityCount_A8()) {
            memset(out, 0, 64);
        } else {
            memcpy(out, this->communities_F1AC.entries_0x00004[index].header_0x0000, 64);
        }
    }
}

/* Copies room `index`'s 64-byte header into `out` (zeroes out of range). */
void NetworkLayerPat::readRoomHeader_C8(s32 index, u8* out)
{
    if (out != NULL) {
        if (index < 0 || (u32)index >= this->rooms_18A4.count_0x000) {
            memset(out, 0, 64);
        } else {
            memcpy(out, this->rooms_18A4.entries_0x004[index].header_0x00, 64);
        }
    }
}

/* Forwards the mediator's terms flag. */
void NetworkLayerPat::setMediatorValue_D8(u32 value)
{
    setMediatorTermsFlag(getInstance(), value);
}

/* The mediator's terms flag. */
u8 NetworkLayerPat::getMediatorValue_DC()
{
    return getMediatorTermsFlag(getInstance());
}

/* Whether the mediator's transfer slot `slot` is ready. */
BOOL NetworkLayerPat::isFriendTransferReady_E8(s8 slot)
{
    return getInstance()->isTransferSlotReady(slot);
}

/* Friend slot `slot`'s +0xC084 flag, 0 for an empty or invalid slot. */
u8 NetworkLayerPat::getFriendFlagC084_EC(s8 slot)
{
    if ((u8)slot > 99) {
        return 0;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 != 0) {
        return this->friendFlagC084_C084[slot];
    }
    return 0;
}

/* Whether friend slot `slot`'s transfer is active, 0 for an empty or invalid slot. */
BOOL NetworkLayerPat::getFriendTransferFlag_F0(s8 slot)
{
    if ((u8)slot > 99) {
        return 0;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 != 0) {
        return this->friendTransfer_C0E8[slot];
    }
    return 0;
}

/* Whether the mediator runs friend slot `slot` in a transfer mode and its +0xC14C flag is set. */
BOOL NetworkLayerPat::isFriendTransferActive_E4(s8 slot)
{
    BOOL result;

    if ((u8)slot <= 99) {
        result = FALSE;
        if (getInstance()->getTransferSlotMode(slot) != 0 && this->friendTransferMode_C14C[slot] != 0) {
            result = TRUE;
        }
    } else {
        result = FALSE;
    }
    return result;
}

/* Stores the request's error record under its mutex. */
void NetworkLayerRequest::setRecord(u32 code, u32 arg_a, u32 arg_b)
{
    LockMutex(this->mutex_78);
    this->record_58 = arg_a;
    this->record_5C = arg_b;
    this->record_54 = code;
    UnlockMutex(this->mutex_78);
}

/* The starter's word argument `index`, 0 (and a warning) past the count it was given. */
s32 NetworkLayerRequest::getArgument(u32 index)
{
    u32 count;
    NetworkLogger* log;

    count = this->count_28;
    if (count <= index) {
        log = getNetworkLogger();
        log->warn_10("NetworkRequest::getArgument: arg no over %d <= %d\n", count, index);
        return 0;
    }
    return (s32)this->args_2C[index];
}

/* 0x803E55AC (0x84): whether the request has waited longer than its interval (never while the interval is 0). */
s32 NetworkLayerRequest::isTimedOut()
{
    NetworkLogger* log;
    s32 result;

    result = 0;
    LockMutex(this->mutex_78);
    if (0.0f != this->interval_4C) {
        log = getNetworkLogger();
        if (log->getTime_60() - this->timeout_50 > this->interval_4C) {
            result = 1;
        }
    }
    UnlockMutex(this->mutex_78);
    return result;
}

/* 0x803E5630 (0x60): restarts the wait: the clock becomes the baseline and the interval is replaced. */
void NetworkLayerRequest::restartTimer(f32 interval)
{
    NetworkLogger* log;

    LockMutex(this->mutex_78);
    log = getNetworkLogger();
    this->timeout_50 = log->getTime_60();
    this->interval_4C = interval;
    UnlockMutex(this->mutex_78);
}

/* The layer event callback `setReflectCallback` installs (its first word). */
typedef void (*NetworkLayerEventCallback)(u32 kind, s32 code, u32 has_info, NetworkRequestError* info, u32 context);

/* Reports one layer event to the installed callback; a failure code is replaced by the singleton's own error. */
void NetworkLayerPat::notifyLayerEvent(u32 kind, s32 code, u32 has_info, NetworkRequestError* info, u32 context)
{
    if (code < 0 && info != NULL && getInstance_() != NULL && getErrorInfo654c(getInstance_(), NULL) != 0) {
        getErrorInfoOrCode654c(getInstance_(), 0x80060033, (NetworkErrorInfo*)info);
        code = info->code_00;
    }
    ((NetworkLayerEventCallback)this->context_04)(kind, code, has_info, info, context);
}

/* Polls every friend slot that does not carry the server's own id. */
void NetworkLayerPat::pollLayerSlots()
{
    u32 i;

    if (this->memberCount_4C8 >= 0 && this->hostMode_4C4 != 0) {
        for (i = 0; i < 100; i++) {
            if (this->serverId_3DC.equals(&this->friends_3568.entries_0x004[i].id_0x00) == 0) {
                pollFriendSlot(i);
            }
        }
    }
}

/* The record `notifyLayerSlotSummary` reports: the members in use, the peak and the two status words. */
typedef struct NetLayerSlotSummary {
    /* +0x00 */ s32 code_00;   /* always -1 */
    /* +0x04 */ s32 used_04;
    /* +0x08 */ s32 peak_08;
    /* +0x0C */ u32 status_0C;
    /* +0x10 */ u32 status_10;
} NetLayerSlotSummary;   /* size: 0x14 */

/* Reports the member counts (friends when hosting, else the server's count) as a kind-0x14 event. */
void NetworkLayerPat::notifyLayerSlotSummary()
{
    NetLayerSlotSummary summary;
    s32 used;
    s32 peak;

    if (this->memberCount_4C8 >= 0) {
        summary.code_00 = -1;
        if (this->hostMode_4C4 != 0) {
            used = this->friends_3568.count_0x000;
        } else {
            used = this->memberUsed_F02C;
        }
        summary.used_04 = used;
        peak = this->status_F030;
        if (used > peak) {
            peak = used;
        }
        summary.peak_08 = peak;
        summary.status_0C = this->status_F034;
        summary.status_10 = this->status_F038;
        notifyLayerEvent(20, 0, 1, (NetworkRequestError*)&summary, this->context_08);
    }
}

/* Records the session-lost error (the singleton's own record, else 0x80060033) as the request's error. */
void NetworkLayerPat::setCollectionLogSessionLost(NetworkLayerRequest* request)
{
    NetworkErrorInfo info;

    getErrorInfoOrCode654c(getInstance_(), 0x80060033, &info);
    request->setRecord(info.code_00, info.param1_04, info.param2_08);
}

/* Records the cancelled-request error (0x80060012) the singleton builds as the request's error. */
void NetworkLayerPat::setCollectionLogAborted(NetworkLayerRequest* request)
{
    NetworkErrorInfo info;

    buildErrorInfo613c(getInstance_(), 0x80060012, &info);
    request->setRecord(info.code_00, info.param1_04, info.param2_08);
}

/* Reports `code` (+ two arguments) to the server, clears the singleton's record and keeps it as the request's error. */
void NetworkLayerPat::setCollectionLog(NetworkLayerRequest* request, u32 code, u32 arg_a, u32 arg_b)
{
    u32 values[3];

    values[0] = code;
    values[1] = arg_a;
    values[2] = arg_b;
    sendServerTimeout(getInstance_(), values);
    clearErrorRecord613c(getInstance_());
    request->setRecord(code, arg_a, arg_b);
}

/* Asks for the layer `depth` levels above this one (the parent itself when `depth` is 1), with fields 1..4. */
/* Halves `inSize` bytes of samples into `out`: each output sample is the mean of an input pair. */
s32 NetworkLayerPat::downsampleVoice(s16* out, s32 outSize, const s16* in, s32 inSize)
{
    s32 size = inSize / 2;
    u32 i;

    if (outSize < size) {
        return 0;
    }
    for (i = (u32)size / 2; i > 0; i--) {
        *out = (in[0] + in[1]) / 2;
        in += 2;
        out++;
    }
    return size;
}

/* Mixes `inSize` bytes of a peer's samples into `out` at twice the rate (each input sample adds an interpolated and a
 * plain output sample), scaled by the mediator's transfer level and clamped to 16 bits. */
s32 NetworkLayerPat::mixVoice(s16* out, s32 outSize, const s16* in, s32 inSize)
{
    s32 level = 2.0f * getInstance()->getTransferLevel();
    s32 size = inSize * 2;
    s32 prev;
    s32 cur;
    s32 value;
    u32 i;
    u32 j;

    if (outSize < size) {
        return 0;
    }
    prev = in[0];
    for (i = 0, j = 0; i + 1 < (u32)size / 2; i += 2) {
        cur = in[(s32)j];
        value = out[0] + level * ((prev + cur) / 2);
        if (value > 32767) {
            value = 32767;
        }
        if (value < -32768) {
            value = -32768;
        }
        out[0] = value;
        value = out[1] + cur * level;
        if (value > 32767) {
            value = 32767;
        }
        if (value < -32768) {
            value = -32768;
        }
        out[1] = value;
        prev = cur;
        j++;
        out += 2;
    }
    return size;
}

/* Runs the layer: the requests, the pending presence, and inside a room the voice transfer and the peers' NAT
 * negotiation (this console negotiates its own pairs; the host also pairs the other members). */
void NetworkLayerPat::move()
{
    PatTagList tags;
    s32 mixed;
    s32 voice;
    s32 sent;
    s32 size;
    s32 i;

    moveRequests();
    if (this->memberCount_4C8 < 0) {
        return;
    }
    if (this->flag_3D2 != 0 && 1.0f + this->timers_3C4[1] < getNetworkLogger()->getTime_60()) {
        this->timers_3C4[1] = getNetworkLogger()->getTime_60();
        buildLayerInfoFields(&tags, &this->presence_450);
        sendReqUserSearchSet(getInstance_(), &tags);
        this->flag_3D2 = 0;
    }
    if (this->memberCount_4C8 != 2) {
        return;
    }
    if (this->busy_3D1 == 0) {
        u32 ready = getInstance()->isVoiceReady();

        if (ready != this->friendFlagC084_C084[this->transferSlot_C07C]) {
            this->friendFlagC084_C084[this->transferSlot_C07C] = ready;
            for (i = 0; i < 100; i++) {
                refreshFriendTransfer(i, 0);
            }
        }
        if (getMediatorTermsStatus(getInstance()) != 0 && isMediatorTermsUpdateFinished(getInstance()) != 0) {
            memset(this->voiceMixed_6EB94.mData, 0, sizeof(this->voiceMixed_6EB94.mData));
            this->voiceMixed_6EB94.mSize = 0;
            for (i = 0; i < 100; i++) {
                if ((s8)i != this->transferSlot_C07C && this->friends_3568.entries_0x004[i].valid_0x35 != 0 &&
                    isFriendTransferActive_E4(i) != 0 && getFriendTransferFlag_F0(i) != 0) {
                    size = getInstance()->popTransferRecord(i, (u8*)this->voiceInput_6E5AC, sizeof(this->voiceInput_6E5AC));
                    if (size > 0) {
                        mixed = mixVoice(this->voiceMixed_6EB94.mData, sizeof(this->voiceMixed_6EB94.mData),
                                         this->voiceInput_6E5AC, size);
                        if (this->voiceMixed_6EB94.mSize < mixed) {
                            this->voiceMixed_6EB94.mSize = mixed;
                        }
                    }
                }
            }
            voice = getInstance()->readVoice((u8*)this->voiceInput_6E5AC, 528);
            if (getInstance()->isVoiceSilent(this->voiceInput_6E5AC, voice) != 0) {
                voice = 0;
            }
            size = this->voiceMixed_6EB94.mSize;
            if (size == 528) {
                if (voice > 0 && getPatTerms() != NULL) {
                    suppressPatTermsEcho(getPatTerms(), (const u8*)this->voiceInput_6E5AC, (u8*)this->voiceMixed_6EB94.mData,
                                        528);
                }
                notifyLayerEvent(EVENT_VOICE, 0, 1, (NetworkRequestError*)&this->voiceMixed_6EB94, this->context_08);
            } else if (size != 0) {
                getNetworkLogger()->warn_10("NetworkLayerPat::move mVoiceMixed.mSize is invalid. %d\n", size);
            }
            sent = 0;
            if (voice > 0) {
                sent = downsampleVoice(this->voiceSend_6E8A0, sizeof(this->voiceSend_6E8A0), this->voiceInput_6E5AC, voice);
            }
            if (sent > 0 && getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                for (i = 0; i < 100; i++) {
                    if ((s8)i != this->transferSlot_C07C && this->friends_3568.entries_0x004[i].valid_0x35 != 0 &&
                        isFriendTransferActive_E4(i) != 0 && getFriendTransferFlag_F0(i) != 0) {
                        getNetworkSessionManagerPat(getPatsObject(), 0)->post((const u8*)this->voiceSend_6E8A0, sent, 0,
                                                                              this->friendSession_EF00[(s8)i]);
                    }
                }
            }
        }
        this->timers_3C4[2] = getNetworkLogger()->getTime_60();
    }
    if (this->transferSlot_C07C >= 0) {
        for (i = 0; i < 100; i++) {
            if (i != this->transferSlot_C07C && this->friends_3568.entries_0x004[i].valid_0x35 != 0 &&
                this->friendPeers_C1B0[i].peerId_00 != 0 && this->pairState_C7F0[this->transferSlot_C07C][i] == 1 &&
                GameSpyInterfaceThread::getInstance() != NULL &&
                (s8)GameSpyInterfaceThread::getInstance()->getSlotState(this->friendPeers_C1B0[i].peerId_00) <= 0) {
                this->pairState_C7F0[this->transferSlot_C07C][i] = 4;
                this->pairState_C7F0[i][this->transferSlot_C07C] = 4;
                sendPairState(this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00, this->friendPeers_C1B0[i].peerId_00, 4);
            }
        }
        if (this->negotiateTo_43C != 0) {
            if (this->negotiated_440 == 0 &&
                this->negotiateTo_43C == this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00 &&
                GameSpyInterfaceThread::getInstance() != NULL &&
                GameSpyInterfaceThread::getInstance()->isNegotiating() == 0) {
                s8 slot = findFriendByPeerId(this->negotiateFrom_438);

                if (slot >= 0 && this->transferSlot_C07C >= 0) {
                    this->pairState_C7F0[slot][this->transferSlot_C07C] =
                        GameSpyInterfaceThread::getInstance()->getNegotiationResult();
                    this->pairState_C7F0[this->transferSlot_C07C][slot] =
                        GameSpyInterfaceThread::getInstance()->getNegotiationResult();
                    sendPairState(this->negotiateFrom_438, this->negotiateTo_43C,
                                  GameSpyInterfaceThread::getInstance()->getNegotiationResult());
                    this->negotiated_440 = 1;
                } else {
                    sendPairState(this->negotiateFrom_438, this->negotiateTo_43C, 3);
                    this->negotiateFrom_438 = 0;
                    this->negotiateTo_43C = 0;
                }
            }
        } else if (this->negotiateFrom_438 != 0 &&
                   this->negotiateFrom_438 == this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00 &&
                   GameSpyInterfaceThread::getInstance() != NULL &&
                   GameSpyInterfaceThread::getInstance()->isNegotiating() == 0) {
            sendPairState(this->negotiateFrom_438, this->negotiateTo_43C, 3);
            this->negotiateFrom_438 = 0;
            this->negotiateTo_43C = 0;
        }
    }
    if (this->transferSlot_C07C == this->sessionState_C080 && this->memberUsed_F02C >= 2 &&
        (this->negotiated_440 != 0 || (this->negotiateTo_43C == 0 && this->negotiateFrom_438 == 0)) &&
        GameSpyInterfaceThread::getInstance() != NULL && GameSpyInterfaceThread::getInstance()->getPhase() == 1) {
        s8 a;
        s8 b;

        for (a = 0; a < 100; a++) {
            if (a != this->transferSlot_C07C && this->friends_3568.entries_0x004[a].valid_0x35 != 0 &&
                this->friendPeers_C1B0[a].peerId_00 != 0 &&
                (this->pairState_C7F0[this->transferSlot_C07C][a] == 0 ||
                 this->pairState_C7F0[this->transferSlot_C07C][a] == 4)) {
                GameSpyPeerId* peer = &this->friendPeers_C1B0[a];

                sendPairState(this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00, peer->peerId_00, 2);
                this->negotiateFrom_438 = this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00;
                this->negotiateTo_43C = peer->peerId_00;
                this->negotiated_440 = 0;
                GameSpyInterfaceThread::getInstance()->startNegotiation(&this->friendPeers_C1B0[this->transferSlot_C07C],
                                                                       peer);
                break;
            }
        }
        if (this->negotiateTo_43C == 0 && this->negotiateFrom_438 == 0) {
            for (a = 0; a < 99; a++) {
                if (a != this->transferSlot_C07C && this->friends_3568.entries_0x004[a].valid_0x35 != 0 &&
                    this->friendPeers_C1B0[a].peerId_00 != 0) {
                    for (b = a + 1; b < 100; b++) {
                        if (b != this->transferSlot_C07C && this->friends_3568.entries_0x004[b].valid_0x35 != 0 &&
                            this->friendPeers_C1B0[b].peerId_00 != 0 &&
                            (this->pairState_C7F0[a][b] == 0 || this->pairState_C7F0[a][b] == 4)) {
                            sendPairState(this->friendPeers_C1B0[a].peerId_00, this->friendPeers_C1B0[b].peerId_00, 2);
                            this->negotiateFrom_438 = this->friendPeers_C1B0[a].peerId_00;
                            this->negotiateTo_43C = this->friendPeers_C1B0[b].peerId_00;
                            this->negotiated_440 = 0;
                            break;
                        }
                    }
                    if (this->negotiateTo_43C != 0 || this->negotiateFrom_438 != 0) {
                        break;
                    }
                }
            }
        }
    }
}

/* Enters (kind 0) or creates (kind 1, with two member limits) the child layer `layerId`: the down or create
 * exchange, the parent's child info and user list, then, in a two-member room, the member session and the GameSpy
 * match; a failure leaves to the parent layer and re-reads the city list. */
s32 NetworkLayerPat::handleLayerCreate(NetworkLayerRequest* request)
{
    s32 kind = request->getArgument(0);
    s32 layerId = request->getArgument(1);
    s32 limitA = 0;
    s32 limitB = 0;
    NetworkRequestError error;
    PatTagList tags;
    PatServerBlock server;
    PatLayerData down;
    PatLayerData create;
    s32 result;
    s32 index;
    s8 slot;
    s32 fmp;

    if (kind == 1) {
        limitA = request->getArgument(2);
        limitB = request->getArgument(3);
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (layerId < 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (kind == 1 && ((u32)(limitA - 1) > 99 || limitA <= limitB || limitB < 0)) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (this->memberCount_4C8 >= 2) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 3);
            request->state_00 = STEP_CANCELLED;
        } else {
            this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
            switch (kind) {
            case 0:
                if (this->pendingRequestId_F19C >= 0) {
                    setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
                    request->state_00 = STEP_CANCELLED;
                } else {
                    this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerChildInfo(getInstance_(), layerId, 0);
                    request->state_00 = 5;
                }
                break;
            case 1:
                if (this->pendingRequestId_F19C == layerId) {
                    request->state_00 = 30;
                } else if (this->pendingRequestId_F19C >= 0) {
                    setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
                    request->state_00 = STEP_CANCELLED;
                } else {
                    this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerCreateHead(getInstance_(), layerId);
                    request->state_00 = 25;
                }
                break;
            default:
                setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
                request->state_00 = STEP_CANCELLED;
                break;
            }
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CHILD_INFO_REPLY) {
            request->state_00 = 10;
        }
        break;
    case 10:
        memset(&down, 0, sizeof(down));
        down.matchKey_070 = this->matchKey_3D4;
        this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
        this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerDown(getInstance_(), layerId, &down);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_DOWN_REPLY) {
            request->state_00 = 50;
        }
        break;
    case 25:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CREATE_HEAD_REPLY) {
            request->state_00 = 30;
        }
        break;
    case 30:
        memset(&create, 0, sizeof(create));
        if (this->comment_F060[0] != 0) {
            memcpy(create.name_014, this->comment_F060, sizeof(this->comment_F060) - 1);
        }
        create.memberLimitA_064 = limitA;
        create.memberLimitB_068 = limitB;
        buildLayerInfoFields(&tags, &this->settings_F0A0);
        this->pendingRequestId_F19C = -1;
        this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
        this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerCreateSet(getInstance_(), layerId, &create, &tags);
        request->state_00 = 35;
        break;
    case 35:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 40;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CREATE_SET_REPLY) {
            request->state_00 = 40;
        }
        break;
    case 40:
        this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
        this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerCreateFoot(getInstance_(), layerId, 0);
        request->state_00 = 45;
        break;
    case 45:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CREATE_FOOT_REPLY) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = STEP_CANCELLED;
            } else {
                this->memberCount_4C8++;
                this->address_474.path_08[this->memberCount_4C8] = layerId + 1;
                request->state_00 = 50;
            }
        }
        break;
    case 50:
        this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
        this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerChildInfo(getInstance_(), -1, 0);
        request->state_00 = 55;
        break;
    case 55:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CHILD_INFO_REPLY) {
            if (this->hostMode_4C4 != 0) {
                request->state_00 = 60;
            } else {
                request->state_00 = 70;
            }
        }
        break;
    case 60:
        this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
        this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerUserList(getInstance_());
        request->state_00 = 65;
        break;
    case 65:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_USER_LIST_REPLY) {
            if (this->memberCount_4C8 == 2) {
                if (kind == 0) {
                    request->state_00 = 67;
                    break;
                }
                this->sessionState_C080 = this->transferSlot_C07C;
            }
            request->state_00 = 70;
        }
        break;
    case 67:
        this->requestFlags_310[SLOT_LAYER_CREATE] = 0;
        this->requestIds_368[SLOT_LAYER_CREATE] = sendReqLayerHost(getInstance_(), (const u8*)&this->address_474);
        request->state_00 = 68;
        break;
    case 68:
        if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_CREATE] & FLAG_HOST_REPLY) {
            request->state_00 = 70;
        }
        break;
    case 70:
        if (this->memberCount_4C8 == 2) {
            if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                index = initNetworkSessionStable(
                    (struct NetworkSessionStableInit*)getNetworkSessionManagerPat(getPatsObject(), 0));
                if (index < 0) {
                    setCollectionLog(request, 0x80050037, 0, index);
                    request->state_00 = 90;
                    break;
                }
                this->friendSession_EF00[this->transferSlot_C07C] = index;
                this->memberStatus_EF64[this->transferSlot_C07C] = 2;
                if (this->transferSlot_C07C == this->sessionState_C080 &&
                    getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                    getNetworkSessionManagerPat(getPatsObject(), 0)->setHostConnectionIndex(index);
                }
            }
            request->state_00 = 71;
        }
    case 71:
        if (this->memberCount_4C8 == 2 && this->memberUsed_F02C > 1) {
            if (GameSpyInterfaceThread::getInstance() != NULL) {
                if (GameSpyInterfaceThread::getInstance()->getState() != 1) {
                    if (GameSpyInterfaceThread::getInstance()->getState() < 0) {
                        GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
                        setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
                        request->state_00 = 90;
                    }
                    break;
                }
                if (GameSpyInterfaceThread::getInstance()->getPhase() <= 0) {
                    copyServerBlock(getInstance_(), 1, (u8*)&server);
                    GameSpyInterfaceThread::getInstance()->resetSlots();
                    GameSpyInterfaceThread::getInstance()->setBufferSize(
                        this->friendPeers_C1B0[this->transferSlot_C07C].port_0C);
                    fmp = getFmpSelected(getInstance_());
                    if (GameSpyInterfaceThread::getInstance()->startMatch(
                            this->status_F034, this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00,
                            server.address_100[3] + (server.address_100[2] << 8) +
                                ((server.address_100[0] << 24) + (server.address_100[1] << 16)),
                            server.port_104, fmp, (this->address_474.path_08[1] << 16) + this->address_474.path_08[2],
                            this->matchKey_3D4) < 0) {
                        GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
                        setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
                        request->state_00 = 90;
                        break;
                    }
                    request->restartTimer(60.0f);
                    request->state_00 = 75;
                } else {
                    GameSpyInterfaceThread::getInstance()->canClose();
                    break;
                }
            }
            sendPeerRecord(-1, 1);
            break;
        }
    case 75:
        if (this->memberCount_4C8 == 2) {
            if (this->memberUsed_F02C > 1) {
                if (GameSpyInterfaceThread::getInstance() != NULL) {
                    if (request->isTimedOut()) {
                        setCollectionLog(request, 0x8003003A, 0, -1);
                        request->state_00 = 90;
                        break;
                    }
                    if (GameSpyInterfaceThread::getInstance()->getPhase() != 1) {
                        if (GameSpyInterfaceThread::getInstance()->getPhase() <= 0) {
                            GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
                            setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
                            request->state_00 = 90;
                        }
                        break;
                    }
                    result = checkMemberSessions();
                    if (result < 1) {
                        if (result == -1) {
                            setCollectionLog(request, this->sessionError_444.code_00, this->sessionError_444.param1_04,
                                             this->sessionError_444.param2_08);
                            request->state_00 = 90;
                        }
                        break;
                    }
                }
                for (slot = 0; slot < 100; slot++) {
                    refreshFriendTransfer(slot, 1);
                }
            }
            if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                getNetworkSessionManagerPat(getPatsObject(), 0)->joinSession();
            }
        }
    case 80:
        this->busy_3D1 = 0;
        memset(this->comment_F060, 0, sizeof(this->comment_F060));
        this->settings_F0A0.count_0x00 = 0;
        notifyLayerEvent(EVENT_LAYER_CREATE, 0, 1, (NetworkRequestError*)&layerId, this->context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return 1;
    case 90:
        clearListPending1();
        request->state_00 = 95;
        break;
    case 95:
        result = stepLayerLeave(request);
        if (result < 0) {
            request->state_00 = STEP_FAILED;
        } else if (result > 0) {
            request->state_00 = 96;
        }
        break;
    case 96:
        clearListPending2();
        request->state_00 = 97;
        break;
    case 97:
        result = stepChildListRead(request);
        if (result < 0) {
            request->state_00 = STEP_FAILED;
        } else if (result > 0) {
            request->state_00 = STEP_CANCELLED;
        }
        break;
    case STEP_CANCELLED:
        this->busy_3D1 = 0;
        memset(this->comment_F060, 0, sizeof(this->comment_F060));
        this->settings_F0A0.count_0x00 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_CREATE, error.code_00, 1, &error, this->context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return 1;
    case STEP_FAILED:
        this->busy_3D1 = 0;
        memset(this->comment_F060, 0, sizeof(this->comment_F060));
        this->settings_F0A0.count_0x00 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_CREATE, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* 0x803EA128 (0x218): leaves to the parent layer, then re-reads its child info and (while hosting) its user list. */
s32 NetworkLayerPat::stepLayerLeave(NetworkLayerRequest* request)
{
    switch (this->listPending_3C0[1]) {
    case 0:
        this->requestFlags_310[SLOT_LAYER_UP] = 0;
        this->requestIds_368[SLOT_LAYER_UP] = sendReqLayerUp(getInstance_());
        this->listPending_3C0[1] = 5;
        break;
    case 5:
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            return -1;
        }
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            return -2;
        }
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_UP_REPLY) {
            this->listPending_3C0[1] = 10;
        }
        break;
    case 10:
        if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
            closeNetworkSessionManagerPat(getNetworkSessionManagerPat(getPatsObject(), 0));
        }
        if (GameSpyInterfaceThread::getInstance() != NULL) {
            GameSpyInterfaceThread::getInstance()->canClose();
            GameSpyInterfaceThread::getInstance()->initialize();
        }
        this->requestFlags_310[SLOT_LAYER_UP] = 0;
        this->requestIds_368[SLOT_LAYER_UP] = sendReqLayerChildInfo(getInstance_(), -1, 0);
        this->listPending_3C0[1] = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            return -1;
        }
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            return -2;
        }
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_CHILD_INFO_REPLY) {
            if (this->hostMode_4C4 != 0) {
                this->listPending_3C0[1] = 20;
            } else {
                this->listPending_3C0[1] = 30;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_LAYER_UP] = 0;
        this->requestIds_368[SLOT_LAYER_UP] = sendReqLayerUserList(getInstance_());
        this->listPending_3C0[1] = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            return -1;
        }
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            return -2;
        }
        if (this->requestFlags_310[SLOT_LAYER_UP] & FLAG_USER_LIST_REPLY) {
            this->listPending_3C0[1] = 30;
        }
        break;
    case 30:
        return 1;
    }
    return 0;
}

/* 0x803EA34C (0x2D0): re-reads the child list (the cities) in batches of 10 under the singleton's busy flag. */
s32 NetworkLayerPat::stepChildListRead(NetworkLayerRequest* request)
{
    s32 first;
    s32 batch;

    switch (this->listPending_3C0[2]) {
    case 0:
        if (testAndSet611b(getInstance_()) == 0) {
            this->requestFlags_310[SLOT_CHILD_LIST] = 0;
            this->requestIds_368[SLOT_CHILD_LIST] = sendReqLayerChildListHead(getInstance_(), 1, 40);
            this->listPending_3C0[2] = 5;
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            this->listPending_3C0[2] = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            this->listPending_3C0[2] = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_LIST_HEAD_REPLY) {
            this->cities_540.count_0x000 = 0;
            if (this->listTotal_F1A4 > 0) {
                this->listCursor_F1A0 = 0;
                this->listPending_3C0[2] = 10;
            } else {
                this->listPending_3C0[2] = 20;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_CHILD_LIST] = 0;
        first = this->listCursor_F1A0;
        batch = (this->listTotal_F1A4 - first < 10) ? this->listTotal_F1A4 - first : 10;
        this->requestIds_368[SLOT_CHILD_LIST] = sendReqLayerChildListData(getInstance_(), first + 1, batch);
        this->listPending_3C0[2] = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            this->listPending_3C0[2] = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            this->listPending_3C0[2] = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_LIST_DATA_REPLY) {
            if (this->cities_540.count_0x000 < 40 && this->listTotal_F1A4 - this->listCursor_F1A0 > 0) {
                this->listPending_3C0[2] = 10;
            } else {
                this->listPending_3C0[2] = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_CHILD_LIST] = 0;
        this->requestIds_368[SLOT_CHILD_LIST] = sendReqLayerChildListFoot(getInstance_());
        this->listPending_3C0[2] = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            this->listPending_3C0[2] = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            this->listPending_3C0[2] = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_LIST_FOOT_REPLY) {
            this->listPending_3C0[2] = 30;
        }
        break;
    case 30:
        set611b(getInstance_());
        return 1;
    case STEP_CANCELLED:
        set611b(getInstance_());
        return -2;
    case STEP_FAILED:
        set611b(getInstance_());
        return -1;
    }
    return 0;
}

/* 0x803EEBA4 (0xFC): sends this console's binary record (its GameSpy peer id) to friend slot `slot`, or to every
 * member when `broadcast` is set. */
void NetworkLayerPat::sendPeerRecord(s8 slot, s32 broadcast)
{
    u8 userId[8];

    if (this->transferSlot_C07C < 0) {
        return;
    }
    if (broadcast != 0) {
        sendLayerBinaryRecord(getInstance_(), this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00,
                              this->friendPeers_C1B0[this->transferSlot_C07C].mode_04,
                              this->friendPeers_C1B0[this->transferSlot_C07C].session_08,
                              this->friendPeers_C1B0[this->transferSlot_C07C].port_0C, NULL, broadcast);
        return;
    }
    if ((u8)slot > 99) {
        return;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 == 0) {
        return;
    }
    if (slot == this->transferSlot_C07C) {
        return;
    }
    this->friends_3568.entries_0x004[slot].id_0x00.exportTo(userId, sizeof(userId));
    sendLayerBinaryRecord(getInstance_(), this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00,
                          this->friendPeers_C1B0[this->transferSlot_C07C].mode_04,
                          this->friendPeers_C1B0[this->transferSlot_C07C].session_08,
                          this->friendPeers_C1B0[this->transferSlot_C07C].port_0C, userId, broadcast);
}

/* 0x803EECFC (0xD0): 1 when every flagged member's session is connected, 0 while one is pending or connecting,
 * -1 when one failed. */
s32 NetworkLayerPat::checkMemberSessions()
{
    s32 i;

    for (i = 0; i < 100; i++) {
        if (this->friends_3568.entries_0x004[i].valid_0x35 != 0 && this->friendFlagEFC8_EFC8[i] != 0) {
            if (this->memberStatus_EF64[i] == 0) {
                return 0;
            }
            if (this->memberStatus_EF64[i] == 2) {
                return 0;
            }
            if (this->memberStatus_EF64[i] == 1) {
                return -1;
            }
        }
    }
    return 1;
}

/* 0x803E247C (0x8): the live terms object. */
extern "C" struct PatTerms* getPatTerms(void)
{
    return sPatTerms;
}

/* Sends this console's user fields to the layer as an item list (item 64 first; at most 64 fields, each by kind). */
void NetworkLayerPat::sendUserFields_5C(NetUserFields* fields)
{
    PatItemList list;
    u32 reserved;
    u8 flag = 1;
    u8 i;

    if (this->memberCount_4C8 >= 0 && this->memberUsed_F02C > 1 && fields != NULL) {
        createItemListStack(getInstance_(), &list, &reserved);
        appendItemList(getInstance_(), &list, 64, 1, &flag, 0);
        if (fields->count_000 > 64) {
            fields->count_000 = 64;
        }
        for (i = 0; i < fields->count_000; i++) {
            switch (fields->fields_008[i].kind_00) {
            case 1:
                appendItemList(getInstance_(), &list, i, 1, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 2:
                appendItemList(getInstance_(), &list, i, 2, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 3:
                appendItemList(getInstance_(), &list, i, 3, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 4:
                appendItemList(getInstance_(), &list, i, 4, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 5:
                appendItemList(getInstance_(), &list, i, 5, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 6:
                appendItemList(getInstance_(), &list, i, 6, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 7:
                appendItemList(getInstance_(), &list, i, 7, (const u8*)&fields->fields_008[i].data_08, 0);
                break;
            case 8:
                appendItemList(getInstance_(), &list, i, 8, fields->fields_008[i].data_08, 0);
                break;
            case 9:
                appendItemList(getInstance_(), &list, i, 9, fields->fields_008[i].data_08, fields->fields_008[i].size_0C);
                break;
            default:
                appendItemList(getInstance_(), &list, i, 0, NULL, 0);
                break;
            }
        }
        sendNtcLayerBinary(getInstance_(), list);
        releaseItemListStack(getInstance_(), &list, reserved);
    }
}

/* Reports friend slot `index` to the layer's callback (as the host only): its record and detail, then its status
 * when it has one. */
void NetworkLayerPat::pollFriendSlot(u32 index)
{
    NetFriendNotice notice;
    NetFriendStatusNotice status;

    if (this->memberCount_4C8 < 0) {
        return;
    }
    if (this->hostMode_4C4 != 0) {
        if (this->friends_3568.entries_0x004[index].valid_0x35 == 0) {
            return;
        }
        copyNetFriendRec(&notice.rec_000, &this->friends_3568.entries_0x004[index]);
        memcpy(&notice.detail_038, &this->details_595C[index], sizeof(NetFriendDetail));
        notifyLayerEvent(EVENT_FRIEND, 0, 1, (NetworkRequestError*)&notice, this->context_08);
        if (this->friendStatus_BEEC[index] != 0) {
            status.id_00.copyFrom((const u8*)&this->friends_3568.entries_0x004[index].id_0x00);
            status.status_20 = this->friendStatus_BEEC[index];
            notifyLayerEvent(EVENT_FRIEND_STATUS, 0, 1, (NetworkRequestError*)&status, this->context_08);
        }
    }
}

/* Builds the notice's friend record. */
NetFriendNotice::NetFriendNotice()
{
}

/* Destroys the notice's friend record. */
NetFriendNotice::~NetFriendNotice()
{
}

/* Builds the notice's unique id. */
NetFriendStatusNotice::NetFriendStatusNotice()
{
}

/* Destroys the notice's unique id. */
NetFriendStatusNotice::~NetFriendStatusNotice()
{
}

/* Takes the presence pairs `move` sends: a copy when none is pending, else the enabled pairs merged into the pending
 * record (its count grown to the new one, at most 4). */
void NetworkLayerPat::setPresence_A0(const NetLayerSettings* presence)
{
    u32 i;

    if (presence != NULL) {
        if (this->flag_3D2 != 0) {
            if ((u32)this->presence_450.count_0x00 < (u32)presence->count_0x00) {
                this->presence_450.count_0x00 = presence->count_0x00;
            }
            if ((u32)this->presence_450.count_0x00 > 4) {
                this->presence_450.count_0x00 = 4;
            }
            for (i = 0; i < (u32)this->presence_450.count_0x00; i++) {
                if (presence->pairs_0x04[i].enabled_0x0 != 0) {
                    memcpy(&this->presence_450.pairs_0x04[i], &presence->pairs_0x04[i], sizeof(NetLayerSettingPair));
                }
            }
        } else {
            memcpy(&this->presence_450, presence, sizeof(NetLayerSettings));
            this->flag_3D2 = 1;
        }
    }
}

/* The valid friend slot whose GameSpy peer id is `peerId`, -1 when none is. */
s8 NetworkLayerPat::findFriendByPeerId(u32 peerId)
{
    s8 i;

    if (peerId != 0) {
        for (i = 0; i < 100; i++) {
            if (this->friends_3568.entries_0x004[i].valid_0x35 != 0 && peerId == this->friendPeers_C1B0[i].peerId_00) {
                return i;
            }
        }
    }
    return -1;
}

/* Reports the negotiation state of a peer pair to the layer. */
void NetworkLayerPat::sendPairState(u32 from, u32 to, s8 state)
{
    if (from != to) {
        sendNtcLayerBinaryNatState(getInstance_(), from, to, state);
    }
}

/* Selects server `server`: leaves the current layer and server when it is another one, connects to the new one and
 * enters its top layer (as the host it also reads the user list), or only re-enters the layer when it is the same. */
s32 NetworkLayerPat::handleServerSelect(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    NetLayerAddress address;
    u32 server = request->getArgument(0);
    s32 slot;
    s32 result;

    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->pendingRequestId_F19C >= 0) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        slot = getFmpSlotIndex(getInstance_(), server);
        if (slot < 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, server);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (server == getFmpSelected(getInstance_())) {
            if (this->memberCount_4C8 < 0) {
                request->state_00 = 50;
            } else if (this->memberCount_4C8 > 0) {
                request->state_00 = 60;
            } else {
                request->state_00 = 70;
            }
            break;
        }
        saveFmpSelection(getInstance_());
        chooseServerAddress(getInstance_(), 1, slot);
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        sendReqFmpInfo(getInstance_(), server, 1);
        request->state_00 = 5;
        break;
    case 5:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_FMP_INFO_REPLY) {
            request->state_00 = 10;
        }
        break;
    case 10:
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            request->state_00 = 30;
        } else if (this->memberCount_4C8 < 0) {
            request->state_00 = 20;
        } else {
            resetLayerState();
            this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
            sendReqLayerEnd(getInstance_());
            request->state_00 = 15;
        }
        break;
    case 15:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            request->state_00 = 30;
        } else if ((this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) ||
                   (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_LAYER_END_REPLY)) {
            request->state_00 = 20;
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        sendReqShut(getInstance_(), 2);
        request->state_00 = 25;
        break;
    case 25:
        if ((this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) ||
            (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) ||
            (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SHUT_REPLY)) {
            request->state_00 = 30;
        }
        break;
    case 30:
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        resetNetworkState3(getInstance_());
        request->state_00 = 35;
        break;
    case 35:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_STATE3_REPLY) {
            request->state_00 = 40;
        }
        break;
    case 40:
        setConnectServerType(getInstance_(), 1);
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        resetNetworkState(getInstance_());
        request->state_00 = 45;
        break;
    case 45:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setCollectionLog(request, LAYER_ERR_SERVER, 110, result);
                request->state_00 = STEP_FAILED;
            } else if (result > 0) {
                if (server != getFmpSelected(getInstance_())) {
                    if (isFmpServerRejected(getInstance_()) != 0) {
                        setCollectionLog(request, LAYER_ERR_SERVER_REJECTED, 0, 0);
                    } else {
                        setCollectionLog(request, LAYER_ERR_SERVER, 110, 0);
                    }
                }
                request->state_00 = 50;
            }
        }
        break;
    case 50:
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        sendReqLayerStart(getInstance_());
        request->state_00 = 55;
        break;
    case 55:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_LAYER_START_REPLY) {
            if (this->hostMode_4C4 != 0) {
                request->state_00 = 80;
            } else {
                request->state_00 = 90;
            }
        }
        break;
    case 60:
        address.id_00 = this->address_474.id_00;
        address.id_04 = this->address_474.id_04;
        memset(address.path_08, 0, 6);
        address.path_08[0] = 1;
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        sendReqLayerJump(getInstance_(), (const u8*)&address, 0);
        request->state_00 = 65;
        break;
    case 65:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 70;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_LAYER_JUMP_REPLY) {
            memset(this->address_474.path_08, 0, 6);
            this->address_474.path_08[0] = 1;
            this->memberCount_4C8 = 0;
            request->state_00 = 70;
        }
        break;
    case 70:
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        sendReqLayerChildInfo(getInstance_(), -1, 0);
        request->state_00 = 75;
        break;
    case 75:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CHILD_INFO_REPLY) {
            if (this->hostMode_4C4 != 0) {
                request->state_00 = 80;
            } else {
                request->state_00 = 90;
            }
        }
        break;
    case 80:
        this->requestFlags_310[SLOT_SERVER_SELECT] = 0;
        sendReqLayerUserList(getInstance_());
        request->state_00 = 85;
        break;
    case 85:
        if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_SELECT] & FLAG_USER_LIST_REPLY) {
            request->state_00 = 90;
        }
        break;
    case 90:
        memset(&error, 0, sizeof(error));
        request->getRecord(&error);
        if (error.code_00 != 0) {
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->busy_3D1 = 0;
        notifyLayerEvent(EVENT_SERVER_SELECT, 0, 0, NULL, this->context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return 1;
    case STEP_CANCELLED:
        this->busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_SERVER_SELECT, error.code_00, 1, &error, this->context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return 1;
    case STEP_FAILED:
        this->busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_SERVER_SELECT, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Reads the server list (the FMP slots) and reports up to `count` servers; with a server already selected it skips the
 * query. */
s32 NetworkLayerPat::handleServerList(NetworkLayerRequest* request)
{
    NetServerList list;
    NetworkFmpSlot slot;
    NetworkRequestError error;
    s32 kind;
    s32 index;
    s32 count = request->getArgument(0);
    s32 result;
    s32 i;

    if (count > 80) {
        count = 80;
    }
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (count <= 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) == 0) {
            getFmpSelection(getInstance_(), &kind, &index);
            switch (kind) {
            case 0:
                request->state_00 = 20;
                break;
            case 1:
                request->state_00 = 10;
                break;
            default:
                setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
                request->state_00 = STEP_CANCELLED;
                break;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_SERVER_LIST] = 0;
        resetNetworkState4(getInstance_());
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_SERVER_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SERVER_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState4(getInstance_(), count);
            if (result < 0) {
                setCollectionLog(request, LAYER_ERR_SERVER, 113, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        set611b(getInstance_());
        list.count_000 = ((u32)count < getFmpSize(getInstance_())) ? count : getFmpSize(getInstance_());
        for (i = 0; i < list.count_000; i++) {
            copyFmpSlot(getInstance_(), &slot, i);
            exportServerRec(&list.entries_008[i], &slot);
        }
        notifyLayerEvent(EVENT_SERVER_LIST, 0, 1, (NetworkRequestError*)&list, this->context_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        if (error.code_00 != LAYER_ERR_NOT_CONNECTED && error.code_00 != LAYER_ERR_BAD_ARGUMENT) {
            set611b(getInstance_());
        }
        notifyLayerEvent(EVENT_SERVER_LIST, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_SERVER_LIST, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Copies the selected server (an FMP slot) into `out` when one is selected. */
void NetworkLayerPat::readSelectedServer_88(NetServerRec* out)
{
    NetworkFmpSlot slot;
    s32 kind;
    s32 index;

    if (out != NULL) {
        getFmpSelection(getInstance_(), &kind, &index);
        if (kind == 1) {
            if (index < 0) {
                return;
            }
            copyFmpSlot(getInstance_(), &slot, index);
            exportServerRec(out, &slot);
        }
    }
}

/* Copies an FMP slot into a server record: id, name, text, the three words and the time. */
void NetworkLayerPat::exportServerRec(NetServerRec* out, const NetworkFmpSlot* slot)
{
    out->id_00 = slot->payload_00;
    memcpy(out->name_04, slot->name_18, sizeof(out->name_04) - 1);
    out->name_04[sizeof(out->name_04) - 1] = 0;
    memcpy(out->text_24, slot->text_38, sizeof(out->text_24) - 1);
    out->text_24[sizeof(out->text_24) - 1] = 0;
    out->done_28 = slot->done_10;
    out->total_2C = slot->total_14;
    out->value_30 = slot->value_3C;
    out->time_38 = slot->time_08;
}

s32 NetworkLayerPat::handleLayerInfo(NetworkLayerRequest* request)
{
    PatTagList fields;
    NetworkRequestError error;
    NetLayerAddress address;
    s32 depth;
    s32 level;
    u8 i;

    depth = request->getArgument(0);
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (depth <= 0 || this->memberCount_4C8 - depth < 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        fields.count_000 = 4;
        for (i = 0; i < fields.count_000; i++) {
            fields.values_004[i].tag_0 = i + 1;
            fields.values_004[i].type_1 = 1;
        }
        if (depth == 1) {
            this->parentInfo_F1A8 = 1;
            this->requestFlags_310[SLOT_LAYER_INFO] = 0;
            this->requestIds_368[SLOT_LAYER_INFO] = sendReqLayerParentInfo(getInstance_(), &fields);
            request->state_00 = 5;
            break;
        }
        address.id_00 = this->address_474.id_00;
        address.id_04 = this->address_474.id_04;
        memcpy(address.path_08, this->address_474.path_08, 6);
        for (level = this->memberCount_4C8 - depth + 1; level <= this->memberCount_4C8; level++) {
            address.path_08[level] = 0;
        }
        this->parentInfo_F1A8 = 0;
        this->requestFlags_310[SLOT_LAYER_INFO] = 0;
        this->requestIds_368[SLOT_LAYER_INFO] = sendReqLayerInfo(getInstance_(), (const u8*)&address, &fields, 3);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_LAYER_INFO] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_LAYER_INFO] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_LAYER_INFO] & FLAG_LAYER_INFO_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10:
        notifyLayerEvent(EVENT_LAYER_INFO, 0, 1, (NetworkRequestError*)&this->layerInfo_4CC, this->context_08);
        return 1;

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_INFO, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_INFO, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Builds the entry's unique id. */
NetLayerMediationEntry::NetLayerMediationEntry()
{
}

/* Destroys the entry's unique id. */
NetLayerMediationEntry::~NetLayerMediationEntry()
{
}

/* Moves to the layer `id` names (in a community, `inCommunity` set): the jump-ready exchange and the FMP server
 * switch when the id is on another server, else a plain jump; then the child info, the user list and, in a
 * two-member room, the member session and the GameSpy match. */
s32 NetworkLayerPat::handleLayerJump(NetworkLayerRequest* request)
{
    const NetworkLayerId* id = (const NetworkLayerId*)request->getArgument(0);
    s32 inCommunity = request->getArgument(1);
    s32 kind;
    s32 index;
    NetLayerAddress target;
    NetworkRequestError error;
    PatTagList tags;
    PatServerBlock server;
    NetworkCommunityProfile* profile;
    u32 size;
    u32 offset;
    s32 i;
    s32 slot;
    s32 result;
    s32 depth;
    s32 fmp;
    s8 friendSlot;

    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (id == NULL) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else {
            NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
            if (target.id_00 != this->address_474.id_00 || target.id_04 == 0 || target.path_08[0] != 1) {
                setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
                request->state_00 = STEP_CANCELLED;
                break;
            }
            this->matchKey_3D4 = 0;
            if (inCommunity != 0) {
                for (i = 0; i < this->communities_F1AC.count_0x00000; i++) {
                    if (NetworkUniqueIdEquals((const NetworkLayerId*)this->communities_F1AC.entries_0x00004[i].header_0x0000,
                                              id)) {
                        break;
                    }
                }
                if (i >= this->communities_F1AC.count_0x00000) {
                    setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 1);
                    request->state_00 = STEP_CANCELLED;
                    break;
                }
                this->matchKey_3D4 = this->communities_F1AC.entries_0x00004[i].matchKey_0x00F0;
            }
            if (this->pendingRequestId_F19C >= 0) {
                setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
                request->state_00 = STEP_CANCELLED;
            } else if (target.id_04 != this->address_474.id_04) {
                request->state_00 = 10;
            } else if (memcmp(&target, &this->address_474, sizeof(target)) != 0) {
                request->state_00 = 60;
            } else {
                request->state_00 = 70;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
        sendReqLayerJumpReady(getInstance_(), (const u8*)&target, this->matchKey_3D4);
        request->state_00++;
        break;
    case 11:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_JUMP_READY_REPLY) {
            request->state_00++;
        }
        break;
    case 12:
        saveFmpSelection(getInstance_());
        NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
        slot = getFmpSlotIndex(getInstance_(), target.id_04);
        if (slot >= 0) {
            chooseServerAddress(getInstance_(), 1, slot);
        } else {
            getFmpSelection(getInstance_(), &kind, &index);
            chooseServerAddress(getInstance_(), 1, index == 0 ? 1 : 0);
        }
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        sendReqFmpInfo(getInstance_(), target.id_04, slot >= 0);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_FMP_INFO_REPLY) {
            request->state_00 = 20;
        }
        break;
    case 20:
        NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
        if ((s32)getFmpSlotIndex(getInstance_(), target.id_04) < 0) {
            setCollectionLog(request, LAYER_ERR_SERVER, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            request->state_00 = 40;
        } else if (this->memberCount_4C8 < 0) {
            request->state_00 = 30;
        } else {
            resetLayerState();
            this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
            sendReqLayerEnd(getInstance_());
            request->state_00 = 25;
        }
        break;
    case 25:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            request->state_00 = 40;
        } else if ((this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) ||
                   (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_LAYER_END_REPLY)) {
            request->state_00 = 30;
        }
        break;
    case 30:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        sendReqShut(getInstance_(), 2);
        request->state_00 = 35;
        break;
    case 35:
        if ((this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) ||
            (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) ||
            (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SHUT_REPLY)) {
            request->state_00 = 40;
        }
        break;
    case 40:
        if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
            closeNetworkSessionManagerPat(getNetworkSessionManagerPat(getPatsObject(), 0));
        }
        if (GameSpyInterfaceThread::getInstance() != NULL) {
            GameSpyInterfaceThread::getInstance()->canClose();
            GameSpyInterfaceThread::getInstance()->initialize();
        }
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        resetNetworkState3(getInstance_());
        request->state_00 = 45;
        break;
    case 45:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_STATE3_REPLY) {
            NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
            slot = getFmpSlotIndex(getInstance_(), target.id_04);
            chooseServerAddress(getInstance_(), 1, slot);
            this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
            setConnectServerType(getInstance_(), 1);
            resetNetworkState(getInstance_());
            request->state_00++;
        }
        break;
    case 46:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setCollectionLog(request, LAYER_ERR_SERVER, 110, result);
                request->state_00 = STEP_FAILED;
            } else if (result > 0) {
                NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
                if (target.id_04 != getFmpSelected(getInstance_())) {
                    if (isFmpServerRejected(getInstance_()) != 0) {
                        setCollectionLog(request, LAYER_ERR_SERVER_REJECTED, 0, 0);
                    } else {
                        setCollectionLog(request, LAYER_ERR_SERVER, 110, 0);
                    }
                }
                request->state_00 = 50;
            }
        }
        break;
    case 50:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        sendReqLayerStart(getInstance_());
        request->state_00 = 55;
        break;
    case 55:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_LAYER_START_REPLY) {
            if (getNetworkCommunityPat(getPatsObject(), 0) != NULL) {
                profile = getNetworkCommunityPat(getPatsObject(), 0)->restoreSentProfile();
                size = profile->windowSize_024;
                if (size != 0) {
                    offset = profile->windowOffset_020;
                    sendReqUserBinarySet(getInstance_(), offset, &profile->data_028[offset], (u16)size);
                }
            }
            if (this->presence_450.count_0x00 != 0) {
                this->timers_3C4[1] = getNetworkLogger()->getTime_60();
                buildLayerInfoFields(&tags, &this->presence_450);
                sendReqUserSearchSet(getInstance_(), &tags);
            }
            this->flag_3D2 = 0;
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = 70;
            } else {
                request->state_00++;
            }
        }
        break;
    case 56:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
        sendReqLayerJumpGo(getInstance_(), (const u8*)&target, this->jumpTicket_3D8);
        request->state_00++;
        break;
    case 57:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 70;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_JUMP_GO_REPLY) {
            NetworkLayerIdExportTo(id, (u8*)&this->address_474, sizeof(this->address_474));
            depth = 1;
            if (this->address_474.path_08[1] != 0) {
                depth = 2;
                if (this->address_474.path_08[2] != 0) {
                    depth = 3;
                }
            }
            this->memberCount_4C8 = depth - 1;
            request->state_00 = 70;
        }
        break;
    case 60:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        NetworkLayerIdExportTo(id, (u8*)&target, sizeof(target));
        sendReqLayerJump(getInstance_(), (const u8*)&target, this->matchKey_3D4);
        request->state_00 = 65;
        break;
    case 65:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            if (this->memberCount_4C8 != 0) {
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 70;
            }
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_LAYER_JUMP_REPLY) {
            NetworkLayerIdExportTo(id, (u8*)&this->address_474, sizeof(this->address_474));
            depth = 1;
            if (this->address_474.path_08[1] != 0) {
                depth = 2;
                if (this->address_474.path_08[2] != 0) {
                    depth = 3;
                }
            }
            this->memberCount_4C8 = depth - 1;
            request->state_00 = 70;
        }
        break;
    case 70:
        if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
            closeNetworkSessionManagerPat(getNetworkSessionManagerPat(getPatsObject(), 0));
        }
        if (GameSpyInterfaceThread::getInstance() != NULL) {
            GameSpyInterfaceThread::getInstance()->canClose();
            GameSpyInterfaceThread::getInstance()->initialize();
        }
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        sendReqLayerChildInfo(getInstance_(), -1, 0);
        request->state_00 = 75;
        break;
    case 75:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CHILD_INFO_REPLY) {
            if (this->hostMode_4C4 != 0) {
                request->state_00 = 80;
            } else {
                request->state_00 = 90;
            }
        }
        break;
    case 80:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        sendReqLayerUserList(getInstance_());
        request->state_00 = 85;
        break;
    case 85:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_USER_LIST_REPLY) {
            if (this->memberCount_4C8 == 2) {
                request->state_00 = 87;
            } else {
                request->state_00 = 90;
            }
        }
        break;
    case 87:
        this->requestFlags_310[SLOT_LAYER_JUMP] = 0;
        this->requestIds_368[SLOT_LAYER_JUMP] = sendReqLayerHost(getInstance_(), (const u8*)&this->address_474);
        request->state_00 = 88;
        break;
    case 88:
        if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_JUMP] & FLAG_HOST_REPLY) {
            request->state_00 = 90;
        }
        break;
    case 90:
        if (this->memberCount_4C8 == 2) {
            if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                result = initNetworkSessionStable(
                    (struct NetworkSessionStableInit*)getNetworkSessionManagerPat(getPatsObject(), 0));
                if (result < 0) {
                    setCollectionLog(request, 0x80050037, 0, result);
                    request->state_00 = 94;
                    break;
                }
                this->friendSession_EF00[this->transferSlot_C07C] = result;
                this->memberStatus_EF64[this->transferSlot_C07C] = 2;
                if (this->transferSlot_C07C == this->sessionState_C080 &&
                    getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                    getNetworkSessionManagerPat(getPatsObject(), 0)->setHostConnectionIndex(result);
                }
            }
            request->state_00 = 91;
        }
    case 91:
        if (this->memberCount_4C8 == 2 && this->memberUsed_F02C > 1) {
            if (GameSpyInterfaceThread::getInstance() != NULL) {
                if (GameSpyInterfaceThread::getInstance()->getState() != 1) {
                    if (GameSpyInterfaceThread::getInstance()->getState() < 0) {
                        GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
                        setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
                        request->state_00 = 94;
                    }
                    break;
                }
                if (GameSpyInterfaceThread::getInstance()->getPhase() <= 0) {
                    copyServerBlock(getInstance_(), 1, (u8*)&server);
                    GameSpyInterfaceThread::getInstance()->resetSlots();
                    GameSpyInterfaceThread::getInstance()->setBufferSize(
                        this->friendPeers_C1B0[this->transferSlot_C07C].port_0C);
                    fmp = getFmpSelected(getInstance_());
                    if (GameSpyInterfaceThread::getInstance()->startMatch(
                            this->status_F034, this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00,
                            server.address_100[3] + (server.address_100[2] << 8) +
                                ((server.address_100[0] << 24) + (server.address_100[1] << 16)),
                            server.port_104, fmp, (this->address_474.path_08[1] << 16) + this->address_474.path_08[2],
                            this->matchKey_3D4) < 0) {
                        GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
                        setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
                        request->state_00 = 94;
                        break;
                    }
                    request->restartTimer(60.0f);
                    request->state_00 = 92;
                } else {
                    GameSpyInterfaceThread::getInstance()->canClose();
                    break;
                }
            }
            sendPeerRecord(-1, 1);
            break;
        }
    case 92:
        if (this->memberCount_4C8 == 2) {
            if (this->memberUsed_F02C > 1) {
                if (GameSpyInterfaceThread::getInstance() != NULL) {
                    if (request->isTimedOut()) {
                        setCollectionLog(request, 0x8003003A, 0, -2);
                        request->state_00 = 94;
                        break;
                    }
                    if (GameSpyInterfaceThread::getInstance()->getPhase() != 1) {
                        if (GameSpyInterfaceThread::getInstance()->getPhase() <= 0) {
                            GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
                            setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
                            request->state_00 = 94;
                        }
                        break;
                    }
                    result = checkMemberSessions();
                    if (result < 1) {
                        if (result == -1) {
                            setCollectionLog(request, this->sessionError_444.code_00, this->sessionError_444.param1_04,
                                             this->sessionError_444.param2_08);
                            request->state_00 = 94;
                        }
                        break;
                    }
                }
                for (friendSlot = 0; friendSlot < 100; friendSlot++) {
                    refreshFriendTransfer(friendSlot, 1);
                }
            }
            if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
                getNetworkSessionManagerPat(getPatsObject(), 0)->joinSession();
            }
        }
    case 93:
        memset(&error, 0, sizeof(error));
        request->getRecord(&error);
        if (error.code_00 != 0) {
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->busy_3D1 = 0;
        notifyLayerEvent(EVENT_LAYER_JUMP, 0, 0, NULL, this->context_08);
        pollLayerSlots();
        notifyLayerSlotSummary();
        return 1;
    case 94:
        clearListPending1();
        request->state_00 = 95;
        break;
    case 95:
        result = stepLayerLeave(request);
        if (result < 0) {
            request->state_00 = STEP_FAILED;
        } else if (result > 0) {
            request->state_00 = STEP_CANCELLED;
        }
        break;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_JUMP, error.code_00, 1, &error, this->context_08);
        if (this->busy_3D1 != 0) {
            pollLayerSlots();
            notifyLayerSlotSummary();
        }
        this->busy_3D1 = 0;
        return 1;
    case STEP_FAILED:
        this->busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_JUMP, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Widens the profile's window back to the last sent one (none at 0x100) and returns the profile. */
inline NetworkCommunityProfile* NetworkCommunityPat::restoreSentProfile()
{
    if (this->sentOffset_500 != 0x100) {
        this->profile_3D8.windowOffset_020 = this->sentOffset_500;
        this->profile_3D8.windowSize_024 = this->sentSize_504;
    }
    return &this->profile_3D8;
}

/* Locks the mediation of member `lock` with `key`; reports the server id and the two arguments when it is done. */
s32 NetworkLayerPat::handleMediationLock(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    s32 lock;
    s32 key;

    lock = request->getArgument(0);
    key = request->getArgument(1);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->requestFlags_310[SLOT_MEDIATION_LOCK] = 0;
        this->requestIds_368[SLOT_MEDIATION_LOCK] = sendReqLayerMediationLock(getInstance_(), lock, key);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_MEDIATION_LOCK] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_MEDIATION_LOCK] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_MEDIATION_LOCK] & FLAG_MEDIATION_LOCK_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10: {
        NetLayerMediationEntry entry;

        entry.id_00.copyFrom((const u8*)&this->serverId_3DC);
        entry.lock_20 = lock;
        entry.key_21 = key;
        notifyLayerEvent(EVENT_MEDIATION_LOCK, 0, 1, (NetworkRequestError*)&entry, this->context_08);
        return 1;
    }

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_MEDIATION_LOCK, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_MEDIATION_LOCK, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Unlocks the mediation of member `lock`; reports the server id and the argument when it is done. */
s32 NetworkLayerPat::handleMediationUnlock(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    s32 lock;

    lock = request->getArgument(0);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->requestFlags_310[SLOT_MEDIATION_UNLOCK] = 0;
        this->requestIds_368[SLOT_MEDIATION_UNLOCK] = sendReqLayerMediationUnlock(getInstance_(), lock, 0);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_MEDIATION_UNLOCK] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_MEDIATION_UNLOCK] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_MEDIATION_UNLOCK] & FLAG_MEDIATION_UNLOCK_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10: {
        NetLayerMediationEntry entry;

        entry.id_00.copyFrom((const u8*)&this->serverId_3DC);
        entry.lock_20 = lock;
        entry.key_21 = 0;
        notifyLayerEvent(EVENT_MEDIATION_UNLOCK, 0, 1, (NetworkRequestError*)&entry, this->context_08);
        return 1;
    }

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_MEDIATION_UNLOCK, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_MEDIATION_UNLOCK, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Asks for the mediation list (at most 32 entries); reports the list when it is done. */
s32 NetworkLayerPat::handleMediationList(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    s32 count;

    count = request->getArgument(0);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->requestFlags_310[SLOT_MEDIATION_LIST] = 0;
        if (count > 32) {
            count = 32;
        }
        this->requestIds_368[SLOT_MEDIATION_LIST] = sendReqLayerMediationList(getInstance_(), 1, count);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_MEDIATION_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_MEDIATION_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_MEDIATION_LIST] & FLAG_MEDIATION_LIST_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10:
        notifyLayerEvent(EVENT_MEDIATION_LIST, 0, 1, (NetworkRequestError*)&this->mediationList_6E128,
                         this->context_08);
        return 1;

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_MEDIATION_LIST, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_MEDIATION_LIST, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Asks for the layer whose id `id` names (it must be a first-level layer of this server); reports the layer record. */
s32 NetworkLayerPat::handleLayerInfoById(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    NetLayerAddress address;
    const NetworkLayerId* id;

    id = (const NetworkLayerId*)request->getArgument(0);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (id == NULL) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        NetworkLayerIdExportTo(id, (u8*)&address, 16);
        if (address.id_00 != this->address_474.id_00 || address.id_04 == 0 || address.path_08[0] != 1) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->requestFlags_310[SLOT_LAYER_INFO_BY_ID] = 0;
        this->requestIds_368[SLOT_LAYER_INFO_BY_ID] = sendReqLayerInfo(getInstance_(), (const u8*)&address, NULL, 4);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_LAYER_INFO_BY_ID] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_LAYER_INFO_BY_ID] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_LAYER_INFO_BY_ID] & FLAG_LAYER_INFO_BY_ID_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10:
        notifyLayerEvent(EVENT_LAYER_INFO_BY_ID, 0, 1, (NetworkRequestError*)&this->layerRecord_6E024, this->context_08);
        return 1;

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_INFO_BY_ID, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_INFO_BY_ID, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Stores `size` bytes of `data` (at most 256) as the layer's binary info; reports the bytes when it is done. */
s32 NetworkLayerPat::handleLayerInfoSet(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    const u8* data;
    u32 size;
    u32 length;

    data = (const u8*)request->getArgument(0);
    size = request->getArgument(1);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START: {
        PatLayerData record;
        PatTagList fields;

        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (data == NULL || size == 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, size);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(&record, 0, sizeof(PatLayerData));
        memset(&fields, 0, sizeof(PatTagList));
        length = (size < 256) ? size : 256;
        memcpy(record.binary_13E, data, length);
        record.value_23E = length;
        this->requestFlags_310[SLOT_LAYER_INFO_SET] = 0;
        this->requestIds_368[SLOT_LAYER_INFO_SET] = sendReqLayerInfoSet(getInstance_(), &record, &fields);
        request->state_00 = 5;
        break;
    }

    case 5:
        if (this->requestFlags_310[SLOT_LAYER_INFO_SET] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_LAYER_INFO_SET] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_LAYER_INFO_SET] & FLAG_LAYER_INFO_SET_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10: {
        NetLayerBinary binary;

        length = (size < 256) ? size : 256;
        memcpy(binary.data_04, data, length);
        binary.size_00 = length;
        notifyLayerEvent(EVENT_LAYER_INFO_SET, 0, 1, (NetworkRequestError*)&binary, this->context_08);
        return 1;
    }

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_INFO_SET, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_INFO_SET, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Reads the child layer list (the cities) in batches of 10 rows: head, data until 40 rows or the end, foot. */
s32 NetworkLayerPat::handleChildList(NetworkLayerRequest* request)
{
    s32 count = request->getArgument(0);   /* the rows asked for; reused as the batch's first row */
    s32 batch;
    NetworkRequestError error;

    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (count <= 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) == 0) {
            this->requestFlags_310[SLOT_CHILD_LIST] = 0;
            this->requestIds_368[SLOT_CHILD_LIST] = sendReqLayerChildListHead(getInstance_(), 1, count);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_LIST_HEAD_REPLY) {
            this->cities_540.count_0x000 = 0;
            if (this->listTotal_F1A4 > 0) {
                this->listCursor_F1A0 = 0;
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_CHILD_LIST] = 0;
        count = this->listCursor_F1A0;
        batch = (this->listTotal_F1A4 - count < 10) ? this->listTotal_F1A4 - count : 10;
        this->requestIds_368[SLOT_CHILD_LIST] = sendReqLayerChildListData(getInstance_(), count + 1, batch);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 20;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_LIST_DATA_REPLY) {
            if (this->cities_540.count_0x000 < 40 && this->listTotal_F1A4 - this->listCursor_F1A0 > 0) {
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_CHILD_LIST] = 0;
        this->requestIds_368[SLOT_CHILD_LIST] = sendReqLayerChildListFoot(getInstance_());
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_CHILD_LIST] & FLAG_LIST_FOOT_REPLY) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        set611b(getInstance_());
        notifyLayerEvent(EVENT_CHILD_LIST, 0, 1, (NetworkRequestError*)&this->cities_540, this->context_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        if (error.code_00 != LAYER_ERR_NOT_CONNECTED && error.code_00 != LAYER_ERR_BAD_ARGUMENT) {
            set611b(getInstance_());
        }
        notifyLayerEvent(EVENT_CHILD_LIST, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_CHILD_LIST, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Reads the sibling layer list (the rooms) the same way; only a layer member can ask for it. */
s32 NetworkLayerPat::handleSiblingList(NetworkLayerRequest* request)
{
    s32 count = request->getArgument(0);   /* the rows asked for; reused as the batch's first row */
    s32 batch;
    NetworkRequestError error;

    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (count <= 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->memberCount_4C8 <= 0) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) == 0) {
            this->requestFlags_310[SLOT_SIBLING_LIST] = 0;
            this->requestIds_368[SLOT_SIBLING_LIST] = sendReqLayerSiblingListHead(getInstance_(), 1, count);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_LIST_HEAD_REPLY) {
            this->rooms_18A4.count_0x000 = 0;
            if (this->listTotal_F1A4 > 0) {
                this->listCursor_F1A0 = 0;
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_SIBLING_LIST] = 0;
        count = this->listCursor_F1A0;
        batch = (this->listTotal_F1A4 - count < 10) ? this->listTotal_F1A4 - count : 10;
        this->requestIds_368[SLOT_SIBLING_LIST] = sendReqLayerSiblingListData(getInstance_(), count + 1, batch);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 20;
        } else if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_LIST_DATA_REPLY) {
            if (this->rooms_18A4.count_0x000 < 40 && this->listTotal_F1A4 - this->listCursor_F1A0 > 0) {
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_SIBLING_LIST] = 0;
        this->requestIds_368[SLOT_SIBLING_LIST] = sendReqLayerSiblingListFoot(getInstance_());
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_SIBLING_LIST] & FLAG_LIST_FOOT_REPLY) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        set611b(getInstance_());
        notifyLayerEvent(EVENT_SIBLING_LIST, 0, 1, (NetworkRequestError*)&this->rooms_18A4, this->context_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        if (error.code_00 != LAYER_ERR_NOT_CONNECTED && error.code_00 != LAYER_ERR_BAD_ARGUMENT &&
            error.code_00 != LAYER_ERR_CANNOT_START) {
            set611b(getInstance_());
        }
        notifyLayerEvent(EVENT_SIBLING_LIST, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_SIBLING_LIST, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Packs the enabled settings pairs (at most four, `max` fields) as numbered info fields; the field count. */
/* Builds the record's unique id. */
NetFriendRec::NetFriendRec()
{
}

/* Destroys the record's unique id. */
NetFriendRec::~NetFriendRec()
{
}

/* Builds the 100 friend records. */
NetFriendTable::NetFriendTable()
{
}

/* Destroys the 100 friend records. */
NetFriendTable::~NetFriendTable()
{
}

/* Builds the 100 friend-list entries. */
NetFriendList::NetFriendList()
{
}

/* Destroys the 100 friend-list entries. */
NetFriendList::~NetFriendList()
{
}

/* Builds the request record's unique id. */
NetLayerRequest::NetLayerRequest()
{
}

/* Destroys the request record's unique id. */
NetLayerRequest::~NetLayerRequest()
{
}

/* Builds the 40 community records. */
NetCommunityList::NetCommunityList()
{
}

/* Destroys the 40 community records. */
NetCommunityList::~NetCommunityList()
{
}

/* Builds the 32 mediation entries. */
NetLayerMediationList::NetLayerMediationList()
{
}

/* Destroys the 32 mediation entries. */
NetLayerMediationList::~NetLayerMediationList()
{
}

/* Builds the layer: the base, the members, the Pat interface when there is none yet, then a cleared state. */
NetworkLayerPat::NetworkLayerPat()
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    clear();
}

/* Releases the layer before its members and the base are destroyed. */
NetworkLayerPat::~NetworkLayerPat()
{
    release();
}

/* Reads the user list of this layer (`mode` 0) or of the child `depth` below it (`mode` 1) and reports it split into
 * the friend records and their session records. */
s32 NetworkLayerPat::handleUserList(NetworkLayerRequest* request)
{
    NetworkRequestError error;
    NetLayerAddress address;
    s32 mode = request->getArgument(0);
    s32 count = request->getArgument(1);
    s32 depth = -1;
    s32 result;

    if (mode == 1) {
        depth = request->getArgument(2);
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (count <= 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (mode == 1) {
            if (depth < 0) {
                setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
                request->state_00 = STEP_CANCELLED;
                break;
            }
            if (this->memberCount_4C8 < 0 || this->memberCount_4C8 + 1 >= 3) {
                setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
                request->state_00 = STEP_CANCELLED;
                break;
            }
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) == 0) {
            if (mode == 1) {
                address.id_00 = this->address_474.id_00;
                address.id_04 = this->address_474.id_04;
                memcpy(address.path_08, this->address_474.path_08, 6);
                address.path_08[this->memberCount_4C8 + 1] = depth + 1;
                clearListPending0();
                readUserList(&address, 0, 1, count, request);
                request->state_00++;
            } else {
                clearListPending0();
                readUserList(&this->address_474, 0, 1, count, request);
                request->state_00++;
            }
        }
        break;
    case 2:
        result = readUserList(NULL, 0, 1, count, request);
        if (result == -1) {
            request->state_00 = STEP_FAILED;
        } else if (result < 0) {
            request->state_00 = STEP_CANCELLED;
        } else if (result > 0) {
            request->state_00++;
        }
        break;
    case 3: {
        NetFriendRoster roster;
        NetFriendRoster* list;
        s32 i;
        s32 j;

        set611b(getInstance_());
        list = &roster;
        for (i = 0; i < 100; i++) {
            clearFriendRec(&roster.table_0000.entries_0x004[i]);
        }
        list->table_0000.count_0x000 =
            (this->friendList_6BC30.count_0x00 < 100) ? this->friendList_6BC30.count_0x00 : 100;
        for (j = 0; j < list->table_0000.count_0x000; j++) {
            copyNetFriendRec(&list->table_0000.entries_0x004[j], &this->friendList_6BC30.entries_0x04[j].rec_00);
            memcpy(&list->sessions_15E4[j], &this->friendList_6BC30.entries_0x04[j].session_38,
                   sizeof(NetFriendSession));
        }
        notifyLayerEvent(EVENT_USER_LIST, 0, 1, (NetworkRequestError*)list, this->context_08);
        return 1;
    }
    case STEP_CANCELLED:
        request->getRecord(&error);
        if (error.code_00 != LAYER_ERR_NOT_CONNECTED && error.code_00 != LAYER_ERR_BAD_ARGUMENT &&
            error.code_00 != LAYER_ERR_CANNOT_START) {
            set611b(getInstance_());
        }
        notifyLayerEvent(EVENT_USER_LIST, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_USER_LIST, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Searches users by the select record's id, text and filters: over this layer's tree (`scope` 4/8: this layer or its
 * parent; 2/20: the whole server) through the user-list reader, or with the plain user search (`scope` 1). */
s32 NetworkLayerPat::handleUserSearch(NetworkLayerRequest* request)
{
    NetLayerFilter filters[4];
    char name[0x20];
    NetworkRequestError error;
    NetLayerAddress address;
    char userId[8];
    s32 count = request->getArgument(0);
    s32 scope = request->getArgument(1);
    s32 kind;
    s32 filterCount;
    s32 first;
    s32 batch;
    s32 result;

    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) != 0) {
            break;
        }
        if (count <= 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memcpy(&address, &this->address_474, sizeof(address));
        switch (scope) {
        case 1:
            kind = 0;
            break;
        case 4:
            kind = 1;
            break;
        case 8:
            if (this->memberCount_4C8 <= 0) {
                kind = -1;
            } else {
                address.path_08[this->memberCount_4C8] = 0;
                kind = 1;
            }
            break;
        case 2:
            memset(address.path_08, 0, 6);
            address.path_08[0] = 1;
            kind = 2;
            break;
        case 20:
            kind = 2;
            break;
        default:
            kind = -2;
            break;
        }
        if (kind < 0) {
            if (kind == -1) {
                setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
            } else {
                setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            }
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->select_F130.id_34.exportTo((u8*)userId, 8);
        memcpy(name, this->select_F130.text_54, 19);
        name[19] = 0;
        filterCount = packLayerFilters(filters, 4, &this->select_F130);
        if (kind != 0) {
            clearListPending0();
            readUserRows(&address, kind == 2, 1, count, userId, name, filters, filterCount, request);
            request->state_00++;
        } else {
            this->requestFlags_310[SLOT_USER_SEARCH] = 0;
            this->requestIds_368[SLOT_USER_SEARCH] =
                sendReqUserSearchHead(getInstance_(), 1, count, userId, name, filters, filterCount, 0);
            request->state_00 = 5;
        }
        break;
    case 2:
        result = readUserRows(NULL, 0, 1, count, NULL, NULL, NULL, 0, request);
        if (result == -1) {
            request->state_00 = STEP_FAILED;
        } else if (result < 0) {
            request->state_00 = STEP_CANCELLED;
        } else if (result > 0) {
            request->state_00 = 30;
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_USER_HEAD_REPLY) {
            s32 i;

            this->friendList_6BC30.count_0x00 = 0;
            for (i = 0; i < 100; i++) {
                this->friendList_6BC30.entries_0x04[i].rec_00.valid_0x35 = 0;
            }
            if (this->listTotal_F1A4 > 0) {
                this->listCursor_F1A0 = 0;
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_USER_SEARCH] = 0;
        first = this->listCursor_F1A0;
        batch = (this->listTotal_F1A4 - first < 10) ? this->listTotal_F1A4 - first : 10;
        this->requestIds_368[SLOT_USER_SEARCH] = sendReqUserSearchData(getInstance_(), first + 1, batch);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 20;
        } else if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_USER_DATA_REPLY) {
            if (this->friendList_6BC30.count_0x00 < 100 && this->listTotal_F1A4 - this->listCursor_F1A0 > 0) {
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_USER_SEARCH] = 0;
        this->requestIds_368[SLOT_USER_SEARCH] = sendReqUserSearchFoot(getInstance_());
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_USER_SEARCH] & FLAG_USER_FOOT_REPLY) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        initNetLayerRequest(&this->select_F130);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_USER_SEARCH, 0, 1, (NetworkRequestError*)&this->friendList_6BC30, this->context_08);
        return 1;
    case STEP_CANCELLED:
        initNetLayerRequest(&this->select_F130);
        request->getRecord(&error);
        if (error.code_00 != LAYER_ERR_NOT_CONNECTED) {
            set611b(getInstance_());
        }
        notifyLayerEvent(EVENT_USER_SEARCH, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        initNetLayerRequest(&this->select_F130);
        request->getRecord(&error);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_USER_SEARCH, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Starts a user-list read with no id, text or filters. */
s32 NetworkLayerPat::readUserList(NetLayerAddress* address, BOOL wide, u32 first, s32 count,
                                  NetworkLayerRequest* request)
{
    return readUserRows(address, wide, first, count, NULL, NULL, NULL, -1, request);
}

/* One step of the user-list reader: the head (list or search), the data batches into the friend list, the foot. */
s32 NetworkLayerPat::readUserRows(NetLayerAddress* address, BOOL wide, u32 first, s32 count, const char* userId,
                                  const char* name, NetLayerFilter* filters, s32 filterCount,
                                  NetworkLayerRequest* request)
{
    NetworkRequestError error;
    s32 cursor;
    s32 batch;
    s32 i;

    switch (this->listPending_3C0[0]) {
    case 0: {
        s32 kind = wide ? 1 : 2;

        this->requestFlags_310[SLOT_USER_LIST] = 0;
        if (filterCount < 0) {
            this->requestIds_368[SLOT_USER_LIST] =
                sendReqLayerUserListHead(getInstance_(), kind, (const u8*)address, first, count);
        } else {
            this->requestIds_368[SLOT_USER_LIST] = sendReqLayerUserSearchHead(
                getInstance_(), kind, (const u8*)address, first, count, userId, name, filters, filterCount);
        }
        this->listPending_3C0[0] = 5;
        break;
    }
    case 5:
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            return -1;
        }
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            return -2;
        }
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_USER_HEAD_REPLY) {
            this->friendList_6BC30.count_0x00 = 0;
            for (i = 0; i < 100; i++) {
                this->friendList_6BC30.entries_0x04[i].rec_00.valid_0x35 = 0;
            }
            if (this->listTotal_F1A4 != 0) {
                this->listCursor_F1A0 = 0;
                this->listPending_3C0[0] = 10;
            } else {
                this->listPending_3C0[0] = 20;
            }
        }
        break;
    case 10:
        cursor = this->listCursor_F1A0;
        batch = (this->listTotal_F1A4 - cursor < 10) ? this->listTotal_F1A4 - cursor : 10;
        this->requestFlags_310[SLOT_USER_LIST] = 0;
        if (filterCount < 0) {
            this->requestIds_368[SLOT_USER_LIST] =
                sendReqLayerUserListData(getInstance_(), this->listCursor_F1A0 + 1, batch);
        } else {
            this->requestIds_368[SLOT_USER_LIST] =
                sendReqLayerUserSearchData(getInstance_(), this->listCursor_F1A0 + 1, batch);
        }
        this->listPending_3C0[0] = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            return -1;
        }
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            this->listPending_3C0[0] = 20;
        } else if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_USER_DATA_REPLY) {
            if (this->friendList_6BC30.count_0x00 < 100 && this->listTotal_F1A4 - this->listCursor_F1A0 > 0) {
                this->listPending_3C0[0] = 10;
            } else {
                this->listPending_3C0[0] = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_USER_LIST] = 0;
        if (filterCount < 0) {
            this->requestIds_368[SLOT_USER_LIST] = sendReqLayerUserListFoot(getInstance_());
        } else {
            this->requestIds_368[SLOT_USER_LIST] = sendReqLayerUserSearchFoot(getInstance_());
        }
        this->listPending_3C0[0] = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            return -1;
        }
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            return -2;
        }
        if (this->requestFlags_310[SLOT_USER_LIST] & FLAG_USER_FOOT_REPLY) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            return (error.code_00 != 0) ? -2 : 1;
        }
        break;
    }
    return 0;
}

s32 NetworkLayerPat::packLayerSettings(PatTagValue* out, s32 max, NetLayerSettings* settings)
{
    s32 n;
    NetLayerSettingPair* pair;
    u32 i;

    if (out == NULL || max <= 0 || settings == NULL) {
        return 0;
    }
    n = 0;
    if ((u32)settings->count_0x00 > 4) {
        settings->count_0x00 = 4;
    }
    for (i = 0, pair = settings->pairs_0x04; i < (u32)settings->count_0x00; pair++, i++) {
        out->tag_0 = i + 1;
        if ((u32)pair->enabled_0x0 == 1) {
            out->type_1 = 1;
            out->value_4 = pair->value_0x4;
            out++;
            n++;
            if (n >= max) {
                return n;
            }
        }
    }
    return n;
}

/* Packs the request's items (at most four, `max` filters) as search filters, mapping each item's operator. */
s32 NetworkLayerPat::packLayerFilters(NetLayerFilter* out, s32 max, NetLayerRequest* request)
{
    s32 n;
    NetLayerRequestItem* item;
    u32 i;

    if (out == NULL || max <= 0 || request == NULL) {
        return 0;
    }
    n = 0;
    if ((u32)request->count_0x00 > 4) {
        request->count_0x00 = 4;
    }
    for (i = 0, item = request->items_0x04; i < (u32)request->count_0x00; item++, i++) {
        out->field_04 = item->slot_0x00 + 1;
        switch (item->enabled_0x01) {
        case 1:
            out->op_00 = 5;
            break;
        case 2:
            out->op_00 = 6;
            break;
        case 3:
            out->op_00 = 4;
            break;
        case 4:
            out->op_00 = 3;
            break;
        case 5:
            out->op_00 = 2;
            break;
        case 6:
            out->op_00 = 1;
            break;
        default:
            continue;
        }
        if ((u32)item->flag_0x04 == 1) {
            out->enabled_05 = 1;
            out->value_08 = item->value_0x08;
            out++;
            n++;
            if (n >= max) {
                break;
            }
        }
    }
    return n;
}

/* Builds `out`'s field list from the settings pairs. */
void NetworkLayerPat::buildLayerInfoFields(PatTagList* out, NetLayerSettings* settings)
{
    if (out != NULL) {
        out->count_000 = packLayerSettings(out->values_004, 32, settings);
    }
}

/* Searches the communities matching the stored request record, one row at a time up to 40 (event 25). */
s32 NetworkLayerPat::handleDetailSearch(NetworkLayerRequest* request)
{
    s32 count = request->getArgument(0);
    s32 filterCount;
    NetLayerFilter filters[4];
    NetworkRequestError error;

    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (count <= 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, count);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        request->state_00++;
    case 1:
        if (testAndSet611b(getInstance_()) == 0) {
            filterCount = packLayerFilters(filters, 4, &this->request_F0C4);
            this->requestFlags_310[SLOT_DETAIL_SEARCH] = 0;
            this->requestIds_368[SLOT_DETAIL_SEARCH] =
                sendReqLayerDetailSearchHead(getInstance_(), 2, 1, count, filters, filterCount);
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_SEARCH_HEAD_REPLY) {
            this->communities_F1AC.count_0x00000 = 0;
            if (this->listTotal_F1A4 > 0) {
                this->listCursor_F1A0 = 0;
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_DETAIL_SEARCH] = 0;
        this->requestIds_368[SLOT_DETAIL_SEARCH] =
            sendReqLayerDetailSearchData(getInstance_(), this->listCursor_F1A0 + 1, 1);
        request->state_00 = 15;
        break;
    case 15:
        if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = 20;
        } else if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_SEARCH_DATA_REPLY) {
            if (this->communities_F1AC.count_0x00000 < 40 && this->listTotal_F1A4 - this->listCursor_F1A0 > 0) {
                request->state_00 = 10;
            } else {
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_DETAIL_SEARCH] = 0;
        this->requestIds_368[SLOT_DETAIL_SEARCH] = sendReqLayerDetailSearchFoot(getInstance_());
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_DETAIL_SEARCH] & FLAG_SEARCH_FOOT_REPLY) {
            memset(&error, 0, sizeof(error));
            request->getRecord(&error);
            if (error.code_00 != 0) {
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        set611b(getInstance_());
        notifyLayerEvent(EVENT_DETAIL_SEARCH, 0, 1, (NetworkRequestError*)&this->communities_F1AC,
                         this->context_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        if (error.code_00 != LAYER_ERR_NOT_CONNECTED && error.code_00 != LAYER_ERR_BAD_ARGUMENT) {
            set611b(getInstance_());
        }
        notifyLayerEvent(EVENT_DETAIL_SEARCH, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        set611b(getInstance_());
        notifyLayerEvent(EVENT_DETAIL_SEARCH, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Asks for the record of the user whose key the argument points at; reports that key back when it is done. */
s32 NetworkLayerPat::handleUserInfo(NetworkLayerRequest* request)
{
    NetLayerUserRecord record;
    NetworkRequestError error;
    const u32* key;

    key = (const u32*)request->getArgument(0);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (key == NULL) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(&record, 0, sizeof(NetLayerUserRecord));
        record.key_38 = *key;
        this->requestFlags_310[SLOT_USER_INFO] = 0;
        this->requestIds_368[SLOT_USER_INFO] = sendReqLayerUserInfoSet(getInstance_(), &record);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_USER_INFO] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_USER_INFO] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_USER_INFO] & FLAG_USER_INFO_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10:
        notifyLayerEvent(EVENT_USER_INFO, 0, 1, (NetworkRequestError*)key, this->context_08);
        return 1;

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_USER_INFO, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_USER_INFO, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Fills the current layer's info record: the name, four words and the settings of a layer answer. */
void NetworkLayerPat::importLayerInfo(NetLayerInfoRec* out, const PatLayerData* layer, PatTagList* tags)
{
    memcpy(out->name_00, layer->name_014, sizeof(out->name_00) - 1);
    out->name_00[sizeof(out->name_00) - 1] = 0;
    out->count_40 = layer->counts_058[1];
    out->count_44 = layer->counts_058[0];
    out->memberLimit_48 = layer->memberLimitA_064;
    out->value_4C = layer->value_06C;
    unpackLayerSettings(&out->settings_50, tags);
}

/* Fills a community record from a layer answer: its id, the name and leader the comment carries (tab-separated), the
 * layer's name, words, match key and settings, its state, and an empty member table. */
void NetworkLayerPat::importCommunityRec(NetCommunityRec* rec, const PatLayerData* layer, PatTagList* tags)
{
    s32 field;
    char* token;
    u32 nameLength;
    u32 leaderLength;
    s32 i;

    if (rec == NULL || layer == NULL) {
        return;
    }
    NetworkLayerIdImportFrom((NetworkLayerId*)rec->header_0x0000, 3, layer->path_004, sizeof(layer->path_004));
    memset(rec->name_0x0040, 0, sizeof(rec->name_0x0040));
    memset(rec->leader_0x0060, 0, sizeof(rec->leader_0x0060));
    field = 0;
    for (token = strtok((char*)layer->comment_07E, "\t"); token != NULL; token = strtok(NULL, "\t")) {
        if (field >= 3) {
            break;
        }
        if (field == 0) {
            if (strlen(token) < sizeof(rec->name_0x0040) - 1) {
                nameLength = strlen(token);
            } else {
                nameLength = sizeof(rec->name_0x0040) - 1;
            }
            memcpy(rec->name_0x0040, token, nameLength);
            rec->name_0x0040[nameLength] = 0;
        }
        if (field == layer->isCurrent_07D - 1) {
            if (strlen(token) < sizeof(rec->leader_0x0060) - 1) {
                leaderLength = strlen(token);
            } else {
                leaderLength = sizeof(rec->leader_0x0060) - 1;
            }
            memcpy(rec->leader_0x0060, token, leaderLength);
            rec->leader_0x0060[leaderLength] = 0;
            break;
        }
        field++;
    }
    memcpy(rec->comment_0x00A0, layer->name_014, sizeof(rec->comment_0x00A0) - 1);
    rec->comment_0x00A0[sizeof(rec->comment_0x00A0) - 1] = 0;
    rec->value_0x00E0 = layer->counts_058[1];
    rec->value_0x00E4 = layer->counts_058[0];
    rec->value_0x00E8 = layer->memberLimitA_064;
    rec->value_0x00EC = layer->value_06C;
    rec->matchKey_0x00F0 = layer->matchKey_070;
    rec->settingsCount_0x00F4 = 0;
    unpackLayerSettings((NetLayerSettings*)&rec->settingsCount_0x00F4, tags);
    rec->state_0x0118 = layer->value_076 == 2;
    rec->members_0x011C.count_0x000 = 0;
    for (i = 0; i < 100; i++) {
        rec->members_0x011C.entries_0x004[i].valid_0x35 = 0;
    }
}

/* Clears one friend record: its id, its data and its flags. */
s32 NetworkLayerPat::clearFriendRec(NetFriendRec* rec)
{
    if (rec == NULL) {
        return -1;
    }
    rec->id_0x00.clear();
    memset(rec->name_0x20, 0, sizeof(rec->name_0x20));
    memset(&rec->flag_0x34, 0, 1);
    rec->valid_0x35 = 0;
    return 1;
}

/* Clears everything the layer keeps for friend slot `index` and closes the mediator's transfer slot. */
s32 NetworkLayerPat::clearFriendSlot(u32 index)
{
    if (index > 99) {
        return -1;
    }
    this->details_595C[index].size_0x000 = 0;
    this->friendStatus_BEEC[index] = 0;
    this->friendFlagC084_C084[index] = 0;
    this->friendTransfer_C0E8[index] = 0;
    this->friendTransferMode_C14C[index] = 0;
    memset(&this->friendPeers_C1B0[index], 0, sizeof(this->friendPeers_C1B0[index]));
    memset(this->pairState_C7F0[index], 0, sizeof(this->pairState_C7F0[index]));
    this->friendSession_EF00[index] = -1;
    this->memberStatus_EF64[index] = 0;
    this->friendFlagEFC8_EFC8[index] = 0;
    this->friendSessions_4B4C[index].state_00 = 0;
    getInstance()->closeTransferSlot(index);
    return 1;
}

/* Fills a friend record from a binary user id and a name (none: an empty name) and marks it valid. */
s32 NetworkLayerPat::importFriendRec(NetFriendRec* rec, const u8* userId, const char* name)
{
    if (rec == NULL || userId == NULL) {
        return -1;
    }
    rec->id_0x00.importFrom(3, userId, 8);
    if (name != NULL) {
        memcpy(rec->name_0x20, name, sizeof(rec->name_0x20) - 1);
        rec->name_0x20[sizeof(rec->name_0x20) - 1] = 0;
    } else {
        memset(rec->name_0x20, 0, sizeof(rec->name_0x20));
    }
    memset(&rec->flag_0x34, 0, 1);
    rec->valid_0x35 = 1;
    return 1;
}

/* Keeps friend slot `index`'s binary (at most 0x100 bytes) and key word from a user answer. */
s32 NetworkLayerPat::storeFriendDetail(u32 index, const PatLayerUser* user)
{
    if (index > 99) {
        return -1;
    }
    if (user == NULL) {
        return -2;
    }
    this->details_595C[index].size_0x000 = user->binarySize_13C;
    if (this->details_595C[index].size_0x000 > 0x100) {
        this->details_595C[index].size_0x000 = 0x100;
    }
    memcpy(this->details_595C[index].data_0x004, user->binary_03C, this->details_595C[index].size_0x000);
    this->friendStatus_BEEC[index] = user->key_038;
    return 1;
}

/* The friend slot whose record carries the binary user id `userId`: -1 without an id, -2 when none does. */
s32 NetworkLayerPat::findFriendByUserId(const u8* userId)
{
    NetworkUniqueId id;
    s32 i;

    if (userId == NULL) {
        return -1;
    }
    id.importFrom(3, userId, 8);
    for (i = 0; i < 100; i++) {
        if (this->friends_3568.entries_0x004[i].valid_0x35 != 0 && id.equals(&this->friends_3568.entries_0x004[i].id_0x00)) {
            return i;
        }
    }
    return -2;
}

/* Adds a user to the first free friend slot and opens its transfer slot; this console's own slot becomes the transfer
 * slot with a fresh peer id and port.  The slot, or -1 (no id or user), -2 (already a friend), -3 (no free slot). */
s32 NetworkLayerPat::addFriendSlot(const u8* userId, const PatLayerUser* user)
{
    NetworkUniqueId id;
    NetFriendRec* rec;
    s8 slot;
    u8 active;

    if (userId == NULL || user == NULL) {
        return -1;
    }
    if (findFriendByUserId(userId) >= 0) {
        return -2;
    }
    if (this->friends_3568.count_0x000 < 100) {
        for (slot = 0; slot < 100; slot++) {
            if (this->friends_3568.entries_0x004[slot].valid_0x35 == 0) {
                rec = &this->friends_3568.entries_0x004[slot];
                importFriendRec(rec, userId, user->name_008);
                storeFriendDetail(slot, user);
                getInstance()->openTransferSlot(slot, this->flag_76, 1.0f);
                id.importFrom(3, userId, 8);
                if (this->serverId_3DC.equals(&id)) {
                    this->transferSlot_C07C = slot;
                    this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00 = this->serverValue_42C;
                    this->friendPeers_C1B0[this->transferSlot_C07C].peerId_00 ^= rand() << 16;
                    this->friendPeers_C1B0[this->transferSlot_C07C].mode_04 = getSomething7(getInstance_());
                    this->friendPeers_C1B0[this->transferSlot_C07C].session_08 = this->serverValue_430;
                    this->friendPeers_C1B0[this->transferSlot_C07C].port_0C = rand() % 20000 + 10000;
                    this->friendFlagC084_C084[slot] = getInstance()->isVoiceReady();
                    active = 0;
                    if (this->friendTransfer_C0E8 != NULL && isFriendTransferActive_E4(slot)) {
                        active = 1;
                    }
                    this->friendTransfer_C0E8[slot] = active;
                    this->friendTransferMode_C14C[slot] = 1;
                } else if (getNetworkCommunityPat(getPatsObject(), 0) != NULL) {
                    this->friendTransferMode_C14C[slot] =
                        getNetworkCommunityPat(getPatsObject(), 0)->isAcceptedPeer(&rec->id_0x00);
                }
                this->friends_3568.count_0x000++;
                return slot;
            }
        }
    }
    return -3;
}

/* Removes the friend with binary user id `userId` (a copy of its record goes to `out`): ends a NAT pair it is part of,
 * clears its pair states, session slot, record and slot, and closes its transfer slot.  The slot, or negative. */
s32 NetworkLayerPat::removeFriendSlot(const u8* userId, NetFriendRec* out)
{
    s32 slot;
    NetFriendRec* rec;
    s32 i;

    slot = findFriendByUserId(userId);
    if (slot >= 0) {
        this->friends_3568.count_0x000--;
        rec = &this->friends_3568.entries_0x004[slot];
        if (out != NULL) {
            copyNetFriendRec(out, rec);
            out->valid_0x35 = 0;
        }
        if (this->negotiateFrom_438 != 0 && this->negotiateFrom_438 == this->friendPeers_C1B0[slot].peerId_00) {
            if (this->negotiated_440 != 0) {
                sendPairState(this->negotiateFrom_438, this->negotiateTo_43C, 0);
                this->negotiated_440 = 0;
                this->negotiateTo_43C = 0;
            }
            this->negotiateFrom_438 = 0;
        }
        if (this->negotiateTo_43C != 0 && this->negotiateTo_43C == this->friendPeers_C1B0[slot].peerId_00) {
            this->negotiateTo_43C = 0;
        }
        for (i = 0; i < 100; i++) {
            this->pairState_C7F0[i][slot] = 0;
        }
        if (getNetworkSessionManagerPat(getPatsObject(), 0) != NULL) {
            getNetworkSessionManagerPat(getPatsObject(), 0)->resetSessionSlot(this->friendSession_EF00[slot]);
        }
        clearFriendRec(rec);
        clearFriendSlot(slot);
        getInstance()->closeTransferSlot(slot);
    }
    return slot;
}

/* Takes the numbered fields 1..4 of `tags` (at most 32 read) as the enabled settings pairs; the count is the highest
 * number seen. */
void NetworkLayerPat::unpackLayerSettings(NetLayerSettings* out, PatTagList* tags)
{
    u8 i;
    u32 slot;

    if (out == NULL || tags == NULL) {
        return;
    }
    out->count_0x00 = 0;
    memset(out->pairs_0x04, 0, sizeof(out->pairs_0x04));
    if (tags->count_000 > 32) {
        tags->count_000 = 32;
    }
    for (i = 0; i < tags->count_000; i++) {
        slot = tags->values_004[i].tag_0 - 1;
        if (slot < 4 && (s32)tags->values_004[i].type_1 == 1) {
            out->pairs_0x04[slot].enabled_0x0 = 1;
            out->pairs_0x04[slot].value_0x4 = tags->values_004[i].value_4;
            if ((u32)out->count_0x00 <= slot) {
                out->count_0x00 = slot + 1;
            }
        }
    }
}

/* Forgets the layer: this console's address and name, the member count and every friend slot. */
void NetworkLayerPat::resetLayerState()
{
    s32 i;

    memset(&this->address_474, 0, sizeof(NetLayerAddress));
    memset(this->userName_484, 0, sizeof(this->userName_484));
    this->hostMode_4C4 = 0;
    this->memberCount_4C8 = -1;
    this->friends_3568.count_0x000 = 0;
    for (i = 0; i < 100; i++) {
        clearFriendRec(&this->friends_3568.entries_0x004[i]);
        clearFriendSlot(i);
    }
    this->transferSlot_C07C = -1;
    this->sessionState_C080 = -2;
    this->memberUsed_F02C = 0;
    this->status_F030 = 0;
    this->status_F034 = 0;
    this->status_F038 = 0;
    this->status_F03C = 0;
}

/* Leaves the layer and shuts the connection down (event 2), skipping the steps that no longer apply. */
s32 NetworkLayerPat::handleDisconnect(NetworkLayerRequest* request)
{
    switch (request->state_00) {
    case 0:
        if (this->connected_3D0 == 0) {
            request->state_00 = 50;
        } else if (getInstance_() == NULL) {
            request->state_00 = 40;
        } else if (isCallback(getInstance_(), 4) == 0) {
            request->state_00 = 40;
        } else if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            request->state_00 = 30;
        } else if (this->memberCount_4C8 < 0) {
            request->state_00 = 10;
        } else {
            resetLayerState();
            this->requestFlags_310[SLOT_DISCONNECT] = 0;
            sendReqLayerEnd(getInstance_());
            request->state_00 = 5;
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_DISCONNECT] & FLAG_SESSION_LOST) {
            request->state_00 = 20;
        } else if ((this->requestFlags_310[SLOT_DISCONNECT] & FLAG_CANCELLED) ||
                   (this->requestFlags_310[SLOT_DISCONNECT] & 0x80)) {
            if (hasMultipleRefs60d4(getInstance_()) != 0) {
                request->state_00 = 30;
            } else {
                request->state_00 = 10;
            }
        }
        break;
    case 10:
        this->requestFlags_310[SLOT_DISCONNECT] = 0;
        sendReqShut(getInstance_(), 1);
        request->state_00 = 15;
        break;
    case 15:
        if ((this->requestFlags_310[SLOT_DISCONNECT] & FLAG_SESSION_LOST) ||
            (this->requestFlags_310[SLOT_DISCONNECT] & FLAG_CANCELLED) ||
            (this->requestFlags_310[SLOT_DISCONNECT] & 0x10)) {
            request->state_00 = 20;
        }
        break;
    case 20:
        this->requestFlags_310[SLOT_DISCONNECT] = 0;
        resetNetworkState3(getInstance_());
        request->state_00 = 25;
        break;
    case 25:
        if (this->requestFlags_310[SLOT_DISCONNECT] & 0x8) {
            request->state_00 = 30;
        }
        break;
    case 30:
        resetCallback(getInstance_(), 4);
        decrement60d4(getInstance_());
        request->state_00 = 40;
        break;
    case 40:
        if (getNetworkLogger()->isVerbose_3C() > 0) {
            request->state_00 = 50;
        }
        break;
    case 50:
        this->connected_3D0 = 0;
        notifyLayerEvent(EVENT_DISCONNECT, 0, 0, NULL, this->context_08);
        return 1;
    }
    return 0;
}

/* Sends a chat line (event 15): to a channel (kinds 2, 14, 10), or as a tell to friend slot `target` (kind 1);
 * reports this console's own message, stamped with the server time, when it is done. */
s32 NetworkLayerPat::handleChat(NetworkLayerRequest* request)
{
    PatMatchOptions options;
    char hunterName[0x20];
    NetworkRequestError error;
    u8 userId[8];
    const char* text;
    u32 tag;
    u32 kind;
    u32 target;
    s32 channel;
    u32 length;

    text = (const char*)request->getArgument(0);
    tag = request->getArgument(1);
    kind = request->getArgument(2);
    target = request->getArgument(3);
    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (text[0] == 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        switch (kind) {
        case 1:
            channel = 0;
            break;
        case 2:
            channel = 1;
            break;
        case 14:
            channel = 3;
            break;
        case 10:
            channel = 2;
            break;
        default:
            channel = -1;
            break;
        }
        if (channel < 0) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(&options, 0, sizeof(options));
        options.tag_00 = tag;
        if (channel != 0) {
            sendNtcLayerChat(getInstance_(), channel, &options, text);
            request->state_00 = 10;
            break;
        }
        if (target >= 100) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, target);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->friends_3568.entries_0x004[target].valid_0x35 == 0) {
            setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, target);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->friends_3568.entries_0x004[target].id_0x00.exportTo(userId, 8);
        this->requestFlags_310[SLOT_CHAT] = 0;
        this->requestIds_368[SLOT_CHAT] = sendReqLayerTell(getInstance_(), userId, &options, text);
        request->state_00 = 5;
        break;

    case 5:
        if (this->requestFlags_310[SLOT_CHAT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->requestFlags_310[SLOT_CHAT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->requestFlags_310[SLOT_CHAT] & FLAG_TELL_REPLY) {
            request->state_00 = 10;
        }
        break;

    case 10: {
        NetworkSessionSlotInfo message;

        message.smallObject_00.copyFrom((const u8*)&this->serverId_3DC);
        getSelectedHunterName(getInstance_(), hunterName);
        memcpy(message.name_20, hunterName, sizeof(message.name_20));
        message.nameEnd_33 = 0;
        memset(&message.flag_34, 0, 1);
        if (strlen(text) < 511) {
            length = strlen(text);
        } else {
            length = 511;
        }
        memcpy(message.text_35, text, length);
        message.text_35[length] = 0;
        message.textEnd_235 = 0;
        message.tag_238 = tag;
        message.time_23C = getServerDateTime(getInstance_());
        notifyLayerEvent(EVENT_CHAT, 0, 1, (NetworkRequestError*)&message, this->context_08);
        return 1;
    }

    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_CHAT, error.code_00, 1, &error, this->context_08);
        return 1;

    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_CHAT, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* The first idle request of the layer's own pool, NULL when both are in use. */
NetworkLayerPatRequest* NetworkLayerPat::allocRequest()
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (this->pool_1C8[i].isOwned() == 0) {
            return &this->pool_1C8[i];
        }
    }
    return 0;
}

/* Releases the request in `slot` (warning when it is still running) and empties the slot. */
void NetworkLayerPat::deleteRequest(NetworkLayerPatRequest** slot)
{
    if (*slot != 0) {
        if ((*slot)->isOwned() != 0) {
            getNetworkLogger()->log_14("NetworkLayerPat::deleteRequest: request is moving.\n");
        }
        (*slot)->clear();
    }
    *slot = 0;
}

/* Runs the 21 base request slots and the layer's own one: a request starts only when the requests it conflicts
 * with are idle (the shutdown slot 2 waits for every other one), and a finished one is released. */
void NetworkLayerPat::moveRequests()
{
    s32 i;
    s32 j;
    BOOL start;

    for (i = 0; i < 22; i++) {
        start = FALSE;
        if (i < 21) {
            if (this->requests_0C[i] != 0 && this->requestState_60[i] == 0) {
                start = TRUE;
            }
        } else if (this->ownRequests_1C0[i - 21] != 0 && this->ownRequestState_1C4[i - 21] == 0) {
            start = TRUE;
        }
        if (start) {
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
                for (j = 0; j < 22; j++) {
                    if (j < 21) {
                        if (this->requestState_60[j] != 0) {
                            break;
                        }
                    } else if (this->ownRequestState_1C4[j - 21] != 0) {
                        break;
                    }
                }
                if (j == 22) {
                    break;
                }
                getNetworkLogger()->log_14("NetworkLayerPat::move: request[%d] is moving, stand by...\n", j);
                continue;
            case 4:
            case 5:
            case 6:
            case 15:
                if (this->requestState_60[7] != 0 || this->requestState_60[8] != 0 || this->requestState_60[9] != 0 ||
                    this->requestState_60[10] != 0 || this->requestState_60[14] != 0) {
                    continue;
                }
                break;
            case 7:
            case 8:
            case 9:
            case 10:
            case 14:
                if (this->requestState_60[4] != 0 || this->requestState_60[5] != 0 || this->requestState_60[6] != 0 ||
                    this->requestState_60[15] != 0) {
                    continue;
                }
                break;
            }
            if (i < 21) {
                this->requestState_60[i] = 1;
            } else {
                this->ownRequestState_1C4[i - 21] = 1;
            }
        }
        if (i < 21) {
            if (this->requests_0C[i] != 0 && this->requestState_60[i] != 0) {
                this->requests_0C[i]->run();
                if (this->requests_0C[i]->isOwned() == 0) {
                    NetworkLayer::deleteRequest(&this->requests_0C[i]);
                    this->requestState_60[i] = 0;
                }
            }
        } else if (this->ownRequests_1C0[i - 21] != 0 && this->ownRequestState_1C4[i - 21] != 0) {
            this->ownRequests_1C0[i - 21]->run();
            if (this->ownRequests_1C0[i - 21]->isOwned() == 0) {
                deleteRequest(&this->ownRequests_1C0[i - 21]);
                this->ownRequestState_1C4[i - 21] = 0;
            }
        }
    }
}

/* Creates the Pat interface singleton when there is none, installs the callback pair and clears the layer. */
void NetworkLayerPat::setReflectCallback(u32 callback, u32 user)
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    NetworkLayer::setReflectCallback(callback, user);
    clear();
}

/* Returns the layer to its initial state: the request slots and ids, the timers, the server and layer records,
 * the lists and the stored request records. */
void NetworkLayerPat::clear()
{
    s32 i;

    NetworkLayer::clear();
    this->ownRequests_1C0[0] = 0;
    this->ownRequestState_1C4[0] = 0;
    for (i = 0; i < 2; i++) {
        this->pool_1C8[i].reset();
    }
    for (i = 0; i < 22; i++) {
        this->requestIds_368[i] = -1;
    }
    this->timers_3C4[0] = -3600.0f;
    this->timers_3C4[1] = -3600.0f;
    this->timers_3C4[2] = -3600.0f;
    this->connected_3D0 = 0;
    this->busy_3D1 = 0;
    this->flag_3D2 = 0;
    this->flag_3D3 = 0;
    this->serverId_3DC.clear();
    memset(this->serverName_3FC, 0, sizeof(this->serverName_3FC));
    memset(&this->serverFlag_410, 0, 1);
    memset(&this->lastPosition_414, 0, sizeof(this->lastPosition_414));
    this->serverValue_42C = 0;
    this->serverValue_430 = 0;
    this->serverPort_434 = 0;
    this->presence_450.count_0x00 = 0;
    resetLayerState();
    memset(&this->layerInfo_4CC, 0, sizeof(this->layerInfo_4CC));
    this->cities_540.count_0x000 = 0;
    this->rooms_18A4.count_0x000 = 0;
    memset(this->comment_F060, 0, sizeof(this->comment_F060));
    this->settings_F0A0.count_0x00 = 0;
    initNetLayerRequest(&this->request_F0C4);
    initNetLayerRequest(&this->select_F130);
    this->pendingRequestId_F19C = -1;
    this->communities_F1AC.count_0x00000 = 0;
    this->friendList_6BC30.count_0x00 = 0;
    for (i = 0; i < 100; i++) {
        this->friendList_6BC30.entries_0x04[i].rec_00.valid_0x35 = 0;
    }
    this->layerRecord_6E024.id_40 = 0;
    this->mediationList_6E128.count_000 = 0;
}

/* Finalizes the Pat interface once nothing holds it, releases the base and the layer's own requests. */
void NetworkLayerPat::release()
{
    PatInterface* pat;
    s32 i;

    if (getInstance_() != NULL) {
        PatInterface_clear((PatInterface*)getInstance_());
        if (PatInterface_isReady((PatInterface*)getInstance_()) == 0) {
            pat = (PatInterface*)getInstance_();
            delete pat;
        }
    }
    NetworkLayer::release();
    for (i = 0; i < 1; i++) {
        deleteRequest(&this->ownRequests_1C0[i]);
        this->ownRequestState_1C4[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        this->pool_1C8[i].clear();
    }
}

/* Clears the first of the three list-pending flags. */
void NetworkLayerPat::clearListPending0()
{
    this->listPending_3C0[0] = 0;
}

/* Clears the second of the three list-pending flags. */
void NetworkLayerPat::clearListPending1()
{
    this->listPending_3C0[1] = 0;
}

/* Clears the third of the three list-pending flags. */
void NetworkLayerPat::clearListPending2()
{
    this->listPending_3C0[2] = 0;
}

/* Sets friend slot `slot`'s +0xC084 flag from bit 0 of `flags` and its transfer flag from bit 1. */
void NetworkLayerPat::setFriendFlags(s8 slot, u32 flags)
{
    if ((u8)slot > 99) {
        return;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 == 0) {
        return;
    }
    this->friendFlagC084_C084[slot] = (flags & 1) != 0;
    this->friendTransfer_C0E8[slot] = (flags & 2) != 0;
}

/* The friend slot whose record carries the unique id `id`, -1 when none does. */
s32 NetworkLayerPat::findFriendById(const NetworkUniqueId* id)
{
    s32 i;

    if (id == NULL) {
        return -1;
    }
    for (i = 0; i < 100; i++) {
        if (this->friends_3568.entries_0x004[i].valid_0x35 != 0 &&
            id->equals(&this->friends_3568.entries_0x004[i].id_0x00) != 0) {
            return i;
        }
    }
    return -1;
}

/* The session slot linked to the friend with unique id `id`, -1 when there is none. */
s8 NetworkLayerPat::getMemberSlot(const NetworkUniqueId* id)
{
    s8 index;

    index = findFriendById(id);
    if (index >= 0) {
        return this->friendSession_EF00[index];
    }
    return -1;
}

/* Copies the unique id of the friend linked to session slot `slot` into `out` (cleared first). */
void NetworkLayerPat::getMemberAddress(s8 slot, NetworkUniqueId* out)
{
    s8 index;

    if (out != NULL) {
        out->clear();
        index = findSessionFriend(slot);
        if (index >= 0) {
            out->copyFrom((const u8*)&this->friends_3568.entries_0x004[index].id_0x00);
        }
    }
}

/* The negotiation state of the friend with unique id `id` (0 when it is not a friend). */
u8 NetworkLayerPat::getMemberStatus(const NetworkUniqueId* id)
{
    s8 index;

    index = findFriendById(id);
    if (index >= 0) {
        return this->memberStatus_EF64[index];
    }
    return 0;
}

/* The friend slot linked to session slot `slot`, -1 when none is. */
s8 NetworkLayerPat::findSessionFriend(s8 slot)
{
    s8 i;

    for (i = 0; i < 100; i++) {
        if (this->friends_3568.entries_0x004[i].valid_0x35 != 0 && slot == this->friendSession_EF00[i]) {
            return i;
        }
    }
    return -1;
}

/* Marks the friend linked to session slot `slot` connected. */
void NetworkLayerPat::onSessionConnected(s8 slot)
{
    s8 index;

    index = findSessionFriend(slot);
    if (index >= 0) {
        this->memberStatus_EF64[index] = 3;
    }
}

/* Unlinks the friend of session slot `slot` and marks it failed: while no request runs, a failure of the transfer
 * friend is reported (event 40); during one, the error (refined by the GameSpy thread's own) is kept. */
void NetworkLayerPat::onSessionFailed(s8 slot, const NetworkErrorInfo* error)
{
    NetworkErrorInfo threadError;
    s8 index;

    index = findSessionFriend(slot);
    if (index >= 0) {
        this->friendSession_EF00[index] = -1;
        this->memberStatus_EF64[index] = 1;
        if (this->busy_3D1 == 0) {
            if (index == this->transferSlot_C07C) {
                notifyLayerEvent(EVENT_SESSION_FAILED, error->code_00, 1, (NetworkRequestError*)error,
                                 this->context_08);
            }
        } else if (this->friendFlagEFC8_EFC8[index] != 0) {
            this->sessionError_444.code_00 = error->code_00;
            this->sessionError_444.param1_04 = error->param1_04;
            this->sessionError_444.param2_08 = error->param2_08;
            if (GameSpyInterfaceThread::getInstance() != NULL) {
                GameSpyInterfaceThread::getInstance()->getErrorStruct(&threadError);
                if (threadError.code_00 != 0) {
                    this->sessionError_444.param1_04 = threadError.param1_04;
                    this->sessionError_444.param2_08 = threadError.param2_08;
                }
            }
        }
    }
}

/* Sets the transfer mode of the friend with unique id `id` (only in a two-member layer) and refreshes it. */
void NetworkLayerPat::setFriendTransferModeById(const NetworkUniqueId* id, u8 mode)
{
    s32 i;

    if (id != NULL && this->memberCount_4C8 == 2) {
        for (i = 0; i < 100; i++) {
            if (this->friends_3568.entries_0x004[i].valid_0x35 != 0 &&
                id->equals(&this->friends_3568.entries_0x004[i].id_0x00) != 0 &&
                mode != this->friendTransferMode_C14C[(s8)i]) {
                this->friendTransferMode_C14C[(s8)i] = mode;
                refreshFriendTransfer(i, 0);
                getInstance()->clearTransferQueue(i);
            }
        }
    }
}

/* Friend slot `slot`'s transfer state for the server: the transfer friend's +0xC084 flag, plus 2 while the slot's
 * transfer is active (0 for an empty or invalid slot). */
u32 NetworkLayerPat::getFriendTransferState(s8 slot)
{
    u8 state;
    BOOL active;

    if ((u8)slot > 99) {
        return 0;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 == 0) {
        return 0;
    }
    state = this->friendFlagC084_C084[this->transferSlot_C07C];
    active = FALSE;
    if (this->friendTransfer_C0E8 != NULL && isFriendTransferActive_E4(slot) != 0) {
        active = TRUE;
    }
    return state | (active ? 2 : 0);
}

/* Re-sends friend slot `slot`'s transfer state to the server (not for the transfer friend itself). */
void NetworkLayerPat::refreshFriendTransfer(s8 slot, u32 flag)
{
    u8 userId[8];
    u32 state;

    if ((u8)slot > 99) {
        return;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 == 0) {
        return;
    }
    if (slot == this->transferSlot_C07C) {
        return;
    }
    this->friends_3568.entries_0x004[slot].id_0x00.exportTo(userId, 8);
    state = getFriendTransferState(slot);
    sendNtcLayerUserTransfer(getInstance_(), state, userId, flag);
}

/* Stores the flag in the base and the mediator; in a two-member layer outside a request, refreshes the transfer
 * friend's active flag and re-sends every friend's transfer state. */
void NetworkLayerPat::setFlag75(u32 value)
{
    BOOL active;
    s32 i;

    NetworkLayer::setFlag75(value);
    setMediatorTransferMode(getInstance(), value);
    if (this->memberCount_4C8 == 2 && this->busy_3D1 == 0) {
        active = FALSE;
        if (this->friendTransfer_C0E8 != NULL && isFriendTransferActive_E4(this->transferSlot_C07C) != 0) {
            active = TRUE;
        }
        this->friendTransfer_C0E8[this->transferSlot_C07C] = active;
        for (i = 0; i < 100; i++) {
            refreshFriendTransfer(i, 0);
        }
    }
}

/* Sets friend slot `slot`'s mediator transfer mode; refreshes the transfer friend's active flag, or re-sends
 * another friend's state when it changed. */
void NetworkLayerPat::setFriendTransferMode_E0(s8 slot, u32 mode)
{
    u32 before;
    BOOL active;

    if ((u8)slot > 99) {
        return;
    }
    if (this->friends_3568.entries_0x004[slot].valid_0x35 == 0) {
        return;
    }
    before = getFriendTransferState(slot);
    getInstance()->setTransferSlotMode(slot, mode);
    if (slot == this->transferSlot_C07C) {
        active = FALSE;
        if (this->friendTransfer_C0E8 != NULL && isFriendTransferActive_E4(this->transferSlot_C07C) != 0) {
            active = TRUE;
        }
        this->friendTransfer_C0E8[this->transferSlot_C07C] = active;
    } else if (before != getFriendTransferState(slot)) {
        refreshFriendTransfer(slot, 0);
    }
}

/* The interval (seconds) between two position notices; the server's login record sets it (0 disables them). */
f32 layerPositionInterval;

/* Publishes this console's position to the layer when it changed and the notice interval has passed. */
void NetworkLayerPat::sendUserPosition_60(const NetUserPosition* position)
{
    NetUserPosition notice;

    if (this->memberCount_4C8 >= 0 && this->memberUsed_F02C > 1 && position != NULL &&
        memcmp(&this->lastPosition_414, position, sizeof(NetUserPosition)) != 0 && layerPositionInterval != 0.0f) {
        if (this->timers_3C4[0] + layerPositionInterval > getNetworkLogger()->getTime_60()) {
            return;
        }
        this->timers_3C4[0] = getNetworkLogger()->getTime_60();
        memcpy(&this->lastPosition_414, position, sizeof(NetUserPosition));
        notice.position_00[0] = position->position_00[0];
        notice.position_00[1] = position->position_00[1];
        notice.position_00[2] = position->position_00[2];
        notice.value_0C[0] = position->value_0C[0];
        notice.value_0C[1] = position->value_0C[1];
        notice.value_0C[2] = position->value_0C[2];
        sendNtcLayerUserPosition(getInstance_(), &notice);
    }
}

/* Takes this console's layer login record: its name, the position notice interval, the host flag and the member
 * counts. */
void NetworkLayerPat::applyLoginRecord(const NetLayerLoginRecord* record)
{
    memcpy(this->userName_484, record->name_14, sizeof(this->userName_484) - 1);
    this->userName_484[sizeof(this->userName_484) - 1] = 0;
    layerPositionInterval = 0.001f * record->positionInterval_78;
    this->hostMode_4C4 = record->role_7C == 1;
    this->memberUsed_F02C = record->memberUsed_5C;
    this->status_F030 = record->memberPeak_58;
    this->status_F034 = record->status_64;
    this->status_F038 = record->status_6C;
}

/* Logs the layer in (event 1): the account check, the reflect callback, the server's login, FMP and binary stages
 * (each ended with a shut request), then enters the layer and takes this console's id and hunter name. */
s32 NetworkLayerPat::handleConnect(NetworkLayerRequest* request)
{
    char hunterName[0x20];
    NetworkRequestError error;
    u8 userId[8];
    u8 options[8];
    s32 result;

    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 != 0) {
            setCollectionLog(request, LAYER_ERR_SERVER, 106, 0);
            request->state_00 = STEP_CANCELLED;
        } else {
            this->connected_3D0 = 1;
            request->state_00 = 5;
        }
        break;
    case 5:
        result = getNetworkLogger()->checkAccount_38(0, (NetworkErrorInfo*)&error);
        if (result < 0) {
            setCollectionLog(request, error.code_00, error.arg_04, error.arg_08);
            request->state_00 = STEP_CANCELLED;
        } else if (result > 0) {
            request->state_00 = 10;
        }
        break;
    case 10:
        openPatInterface(getInstance_());
        setCallback(getInstance_(), (void (*)())networkLayerReflectCallback, this, 4);
        if (getErrorInfo654c(getInstance_(), NULL) != 0) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if ((u8)getNetworkBinaryState(getInstance_()) != 0) {
            request->state_00 = 60;
        } else {
            setConnectServerType(getInstance_(), 0);
            this->requestFlags_310[SLOT_CONNECT] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 15;
        }
        break;
    case 15:
        if (this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CONNECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2(getInstance_());
            if (result < 0) {
                setCollectionLog(request, LAYER_ERR_SERVER, 108, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                this->requestFlags_310[SLOT_CONNECT] = 0;
                sendReqShut(getInstance_(), 2);
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        if ((this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) ||
            (this->requestFlags_310[SLOT_CONNECT] & FLAG_CANCELLED) || (this->requestFlags_310[SLOT_CONNECT] & 0x10)) {
            this->requestFlags_310[SLOT_CONNECT] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 25;
        }
        break;
    case 25:
        if (this->requestFlags_310[SLOT_CONNECT] & 0x8) {
            if (isSubState_894F_4or6((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80060034, 0, 0);
                request->state_00 = STEP_CANCELLED;
            } else if (isSubState_894F_5((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80060035, 0, 0);
                request->state_00 = STEP_CANCELLED;
            } else if (isSubState_894F_3((PatInterface*)getInstance_()) != 0) {
                request->setRecord(LAYER_ERR_SERVER, 114, 0);
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        if (this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if ((s8)getSomething4(getInstance_()) < 0 || this->requests_0C[SLOT_DISCONNECT] != 0) {
            request->setRecord(LAYER_ERR_NO_LAYER, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if ((s8)getSomething4(getInstance_()) > 0) {
            this->requestFlags_310[SLOT_CONNECT] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 35;
        }
        setSomething(getInstance_(), 0);
        break;
    case 35:
        if (this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CONNECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2Fmp(getInstance_());
            if (result < 0) {
                setCollectionLog(request, LAYER_ERR_SERVER, 109, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                this->requestFlags_310[SLOT_CONNECT] = 0;
                sendReqShut(getInstance_(), 2);
                request->state_00 = 40;
            }
        }
        break;
    case 40:
        if ((this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) ||
            (this->requestFlags_310[SLOT_CONNECT] & FLAG_CANCELLED) || (this->requestFlags_310[SLOT_CONNECT] & 0x10)) {
            this->requestFlags_310[SLOT_CONNECT] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 45;
        }
        break;
    case 45:
        if (this->requestFlags_310[SLOT_CONNECT] & 0x8) {
            request->state_00 = 50;
        }
        break;
    case 50:
        setConnectServerType(getInstance_(), 1);
        this->requestFlags_310[SLOT_CONNECT] = 0;
        resetNetworkState(getInstance_());
        request->state_00 = 55;
        break;
    case 55:
        if (this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CONNECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setCollectionLog(request, LAYER_ERR_SERVER, 0, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                request->state_00 = 60;
            }
        }
        break;
    case 60:
        this->requestFlags_310[SLOT_CONNECT] = 0;
        sendReqLayerStart(getInstance_());
        request->state_00 = 65;
        break;
    case 65:
        if (this->requestFlags_310[SLOT_CONNECT] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_CONNECT] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_CONNECT] & FLAG_LAYER_START_REPLY) {
            request->state_00 = 70;
        }
        break;
    case 70:
        this->busy_3D1 = 0;
        getSelectedID(getInstance_(), userId);
        this->serverId_3DC.importFrom(3, userId, sizeof(userId));
        getSelectedHunterName(getInstance_(), hunterName);
        memcpy(this->serverName_3FC, hunterName, 19);
        this->serverName_3FC[19] = 0;
        memset(&this->serverFlag_410, 0, 1);
        this->serverValue_42C = getSomething8(getInstance_());
        getNetworkLogger()->readMatchOptions_78(0, (PatMatchOptions*)options);
        memcpy(&this->serverValue_430, options, 4);
        notifyLayerEvent(EVENT_CONNECT, 0, 0, NULL, this->context_08);
        return 1;
    case STEP_CANCELLED:
        this->busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_CONNECT, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        this->busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(EVENT_CONNECT, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* The descriptor `requestLayerCreate` passes by value: `{0, -1, handler}`. */
NetworkLayerPatHandler networkLayerPatRequestDescReserve = &NetworkLayerPat::handleLayerReserve;

/* Starts the layer's own request on `handleLayerReserve` with `layerId` and the reserve flag, while none is parked. */
void NetworkLayerPat::requestLayerCreate(u32 layerId, s32 reserve)
{
    NetworkLayerPatRequest* request;

    if (this->ownRequests_1C0[0] == 0) {
        request = allocRequest();
        if (request != 0) {
            this->ownRequests_1C0[0] = request;
            request->begin(this, networkLayerPatRequestDescReserve, 2, layerId, reserve);
        }
    }
}

/* Reserves (create head, kept as the pending id) or releases (create foot) layer `layerId` and reports event 36
 * with the reserve flag. */
s32 NetworkLayerPat::handleLayerReserve(NetworkLayerPatRequest* request)
{
    s32 layerId = request->getArgument(0);
    s32 reserve = request->getArgument(1);
    NetworkRequestError error;
    u8 reserved;

    if (this->memberCount_4C8 < 0) {
        setCollectionLog(request, LAYER_ERR_NO_LAYER, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->connected_3D0 == 0) {
            setCollectionLog(request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (layerId < 0) {
            setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, layerId);
            request->state_00 = STEP_CANCELLED;
        } else {
            if (reserve != 0) {
                if (this->pendingRequestId_F19C >= 0) {
                    if (this->pendingRequestId_F19C != layerId) {
                        setCollectionLog(request, LAYER_ERR_CANNOT_START, 0, layerId);
                        request->state_00 = STEP_CANCELLED;
                    } else {
                        request->state_00 = 20;
                    }
                    break;
                }
            } else if (this->pendingRequestId_F19C != layerId) {
                setCollectionLog(request, LAYER_ERR_BAD_ARGUMENT, 0, 0);
                request->state_00 = STEP_CANCELLED;
            }
            this->requestFlags_310[SLOT_LAYER_RESERVE] = 0;
            if (reserve != 0) {
                this->requestIds_368[SLOT_LAYER_RESERVE] = sendReqLayerCreateHead(getInstance_(), layerId);
                request->state_00 = 5;
            } else {
                this->requestIds_368[SLOT_LAYER_RESERVE] = sendReqLayerCreateFoot(getInstance_(), layerId, 1);
                request->state_00 = 15;
            }
        }
        break;
    case 5:
        if (this->requestFlags_310[SLOT_LAYER_RESERVE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_RESERVE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_RESERVE] & FLAG_CREATE_HEAD_REPLY) {
            request->state_00 = 20;
        }
        break;
    case 15:
        if (this->requestFlags_310[SLOT_LAYER_RESERVE] & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->requestFlags_310[SLOT_LAYER_RESERVE] & FLAG_CANCELLED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->requestFlags_310[SLOT_LAYER_RESERVE] & FLAG_CREATE_FOOT_REPLY) {
            request->state_00 = 20;
        }
        break;
    case 20:
        reserved = reserve != 0;
        this->pendingRequestId_F19C = reserve != 0 ? layerId : -1;
        notifyLayerEvent(EVENT_LAYER_RESERVE, 0, 1, (NetworkRequestError*)&reserved, this->context_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_RESERVE, error.code_00, 1, &error, this->context_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyLayerEvent(EVENT_LAYER_RESERVE, error.code_00, 1, &error, this->context_08);
        notifyLayerEvent(EVENT_ERROR, error.code_00, 1, &error, this->context_08);
        return 1;
    }
    return 0;
}

/* Records the session-lost error (the singleton's own record, else 0x80060033) as the own request's error. */
void NetworkLayerPat::setCollectionLogSessionLost(NetworkLayerPatRequest* request)
{
    NetworkErrorInfo info;

    getErrorInfoOrCode654c(getInstance_(), 0x80060033, &info);
    request->setRecord(info.code_00, info.param1_04, info.param2_08);
}

/* Records the cancelled-request error (0x80060012) as the own request's error. */
void NetworkLayerPat::setCollectionLogAborted(NetworkLayerPatRequest* request)
{
    NetworkErrorInfo info;

    buildErrorInfo613c(getInstance_(), 0x80060012, &info);
    request->setRecord(info.code_00, info.param1_04, info.param2_08);
}

/* Reports `code` (+ two arguments) to the server, clears the singleton's record and keeps it as the own request's
 * error. */
void NetworkLayerPat::setCollectionLog(NetworkLayerPatRequest* request, u32 code, u32 arg_a, u32 arg_b)
{
    u32 values[3];

    values[0] = code;
    values[1] = arg_a;
    values[2] = arg_b;
    sendServerTimeout(getInstance_(), values);
    clearErrorRecord613c(getInstance_());
    request->setRecord(code, arg_a, arg_b);
}

/* Builds the record's mutex, then resets it. */
inline NetworkLayerPatRequest::NetworkLayerPatRequest()
{
    networkInstance_initMutex(this->mutex_78);
    reset();
}

/* Whether the request is running (it has an owner). */
inline s32 NetworkLayerPatRequest::isOwned()
{
    return this->owner_94 != 0;
}

/* Runs the request's handler through its member-function pointer; a handler that reports completion resets the
 * record. */
inline void NetworkLayerPatRequest::run()
{
    if (this->owner_94 != 0 && (this->owner_94->*this->handler_98)(this) != 0) {
        clear();
    }
}

/* Starts the request for `owner`: resets it, stamps the start time and a fresh id, takes the handler and copies up to
 * eight word arguments. */
inline void NetworkLayerPatRequest::begin(NetworkLayerPat* owner, NetworkLayerPatHandler handler, u32 count, ...)
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

/* Copies the request's error record out under its mutex; false while none is set. */
inline s32 NetworkLayerPatRequest::getRecord(NetworkRequestError* out)
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

/* The starter's word argument `index`, 0 (and a warning) past the count it was given. */
inline s32 NetworkLayerPatRequest::getArgument(u32 index)
{
    u32 count;

    count = this->count_28;
    if (count <= index) {
        getNetworkLogger()->warn_10("NetworkRequest::getArgument: arg no over %d <= %d\n", count, index);
        return 0;
    }
    return (s32)this->args_2C[index];
}

/* Sets the request's error record under its mutex. */
inline void NetworkLayerPatRequest::setRecord(u32 code, u32 arg_a, u32 arg_b)
{
    LockMutex(this->mutex_78);
    this->record_58 = arg_a;
    this->record_5C = arg_b;
    this->record_54 = code;
    UnlockMutex(this->mutex_78);
}

/* Resets the record and releases its mutex. */
inline NetworkLayerPatRequest::~NetworkLayerPatRequest()
{
    clear();
    networkInstance_destroyMutex(this->mutex_78, -1);
}

inline void NetworkLayerPatRequest::clear()
{
    reset();
}

/* Returns a request record to its idle state. */
inline void NetworkLayerPatRequest::reset()
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

/* The server's message callback `setCallback(..., 4)` installs: re-orders the six arguments and runs `reflect`. */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
extern "C" void networkLayerReflectCallback(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5)
{
    ((NetworkLayerPat*)a5)->reflect((s32)a0, (s32)a1, (s32)a2, (s32)a3, (const u8*)a4);
}
#pragma peephole on
