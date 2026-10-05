/*
 * Network/NetworkCommunityPat.cpp - the community layer `NetworkCommunityPat` (constructor 0x803F02C4, allocation
 *   0x25EC), derived from `NetworkCommunity`: the session handlers of the base's twelve request slots, a second pool of
 *   nine friend-request slots (`NetworkCommunityPatRequest`), the profile writers and the Pat event callback.
 * RANGE. .text 0x803F0294-0x803F6458 (87 functions); .data 0x805FC4D0-0x805FC820 (the deleteRequest string, the eight
 *   friend-request descriptors, the event switch's jump table, the table 0x805FC728, then the two inline `getArgument`
 *   strings, generated last), .sdata 0x80793948-0x80793950, .sdata2 0x8079C7A8-0x8079C7B8, extab, extabindex.
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md).  File-scope
 *   `#pragma dont_inline on` (retail calls every inline member out of line, the generated record ctors/dtors included)
 *   and `#pragma peephole off` (the `__ptmf_null` copy keeps its unfused lis/addi/lwz).
 * NAMES. The class name is its own "NetworkCommunityPat::deleteRequest" string's; every other name is a GUESS from what
 *   the bodies store, compare and pass (the `setCollectionLog*` helpers by analogy with `NetworkLayerPat`'s).  The
 *   senders are `Network/PatInterface.cpp`'s, named from their Pat op-codes and the `recvAns*` trace strings (request N,
 *   answer N+1): `sendReqTell` 245, `sendReqBinaryUser` 248, `sendReqUserSearchInfo` 265, `sendReqUserStatusSet` 269,
 *   `sendReqFriendAdd` 273, `sendReqFriendAccept` 276, `sendReqFriendDelete` 279, `sendReqFriendList` 281,
 *   `sendReqBlackAdd` 283, `sendReqBlackDelete` 285, `sendReqBlackList` 287; hence `blockPlayer`/`unblockPlayer`
 *   (BlackAdd/BlackDelete); `inviteFriend` (sends FriendAccept) and `sendFriendMessage` (FriendAdd with a text) keep the
 *   consumer's names.  `NetworkFriendInfo` is `menu/movie.cpp`'s (`menu/movie.h`), `strtok` MSL's.
 * RESIDUALS. `__dt__17NetworkFriendInfoFv`: retail re-extends the flag (`extsh r0,r4`), the empty inline destructor
 *   here compares r4 directly - 4 bytes short (`.text` 0x61C0 against 0x61C4); a virtual and a non-virtual destructor
 *   score the same.  `writeProfileRange_50`: one `add` keeps its operands swapped.  `onPatEvent`: the strtok loop's
 *   token/index registers swap (r26/r27) and the two by-value error copies sit in named locals (retail inlines
 *   `NetworkInstanceDispatch::postError(NetworkPostedError)`, kept out of line by the file-scope `dont_inline`).
 *   `handleUnblockPlayer`/`onPatEvent`: `setFriendTransferModeById` (NetworkLayerPat) takes `u8 mode`, so this caller
 *   emits a `clrlwi` retail lacks; a `u32` parameter restores both and lowers the callee, so the owner's u8 stays.
 *   `.sdata2`: retail emits `communityProfileSendInterval` (0x8079C7A8) before the inline record `reset`'s 0.0 and
 *   `clear`'s -3600.0, ours after (its definition has to follow `move`).  `extab`: two records differ by the
 *   mutex-member action `NetworkCommunity` records.
 * SHAPES. The records a handler delivers are declared in the case block that fills them (retail constructs them there);
 *   the event switch's cases follow retail's body order; `move`'s 1.0 is the named constant
 *   `communityProfileSendInterval` defined after its user (defined before, MWCC folds it into a pooled literal loaded
 *   in the other order).  The request records' members are inline: retail emits each out-of-line copy right after the
 *   first function that calls it.
 */
#include "Network/NetworkCommunityPat.h"         /* the unit's own header */
#include "Network/NetworkSessionManager.h"       /* networkInstance_initMutex, NetworkRequest_idCounter, NetworkVaState */
#include "Network/NetworkSessionBase.h"          /* LockMutex/UnlockMutex, getNetworkBinaryState */
#include "Network/PatInterface.h"                /* PatInterface and its C surface */
#include "Network/network_pat_control.h"         /* NetRosterSync */
#include "Network/NetworkPat.h"                  /* getNetworkLayerPat */
#include "Network/NetworkSessionManagerPat.h"    /* getPatsObject */
#include "Network/NetworkLayer.h"                /* NetworkLayerIdImportFrom */
#include "enemy/em020_ai.h"                      /* getInstance_ */
#include "unsplit/Network.h"                     /* getNetworkLogger, dtor_803CA338 - no registered owner */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL/strlen.h"
#include "MSL_C/alloc.h"                         /* memmove - owner MSL_C/alloc.cpp */
#include "Network/gamespy_interface_types.h"     /* NetworkErrorInfo */
#include "types.h"

/* The community error codes (0x8007xxxx is this layer's range). */
enum {
    COMMUNITY_ERR_NOT_OPEN = 0x80070001,
    COMMUNITY_ERR_BAD_ARGUMENT = 0x80070002,
    COMMUNITY_ERR_UNSUPPORTED = 0x80070011,
    COMMUNITY_ERR_PROTOCOL = 0x80070012,
    COMMUNITY_ERR_NOT_LOGGED_IN = 0x80070031,
    COMMUNITY_ERR_SESSION = 0x80070032
};

/* The request steps every handler shares. */
enum {
    STEP_START = 0,
    STEP_WAIT = 5,
    STEP_DONE = 10,
    STEP_CANCELLED = 100,
    STEP_FAILED = 110
};

/* `replyFlags_2F0` bits every request reads. */
enum {
    REPLY_SESSION_LOST = 0x1,
    REPLY_ABORTED = 0x2
};

/* How long `move` waits between two profile sends (seconds of the logger's clock); defined at the end of the file, because a
 * definition ahead of `move` is folded into a pooled literal (see the file header). */
extern const f32 communityProfileSendInterval;

/* Whether the state machine's community session is up (+0x6559, set when the binary exchange reaches phase 3). */
inline s32 isPatShutdown(NetworkStateMachine* self)
{
    return self->shutdownFlag_6559;
}

/* Retail calls every inline member out of line (each copy follows its first caller). */
#pragma dont_inline on
/* Retail keeps the unfused lis/addi/lwz forms of the null member pointer copy. */
#pragma peephole off

/* The Pat callback slot 6: hands the event to the layer installed with it. */
static void communityPatEventCallback(s32 code, s32 requestId, s32 flag, s32 count, const u8* data,
                                      NetworkCommunityPat* layer)
{
    layer->onPatEvent(code, requestId, flag, count, data);
}

NetworkCommunityPat::NetworkCommunityPat()
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    clear();
}

inline NetworkCommunityPatRequest::~NetworkCommunityPatRequest()
{
    clear();
    dtor_803CA338(this->mutex_78, -1);
}

inline void NetworkCommunityPatRequest::clear()
{
    reset();
}

/* Returns a request record to its idle state. */
inline void NetworkCommunityPatRequest::reset()
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

inline NetworkCommunityPatRequest::NetworkCommunityPatRequest()
{
    networkInstance_initMutex(this->mutex_78);
    reset();
}

NetworkCommunityPat::~NetworkCommunityPat()
{
    release();
}

void NetworkCommunityPat::setReflectCallback(u32 callback, u32 user)
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    NetworkCommunity::setReflectCallback(callback, user);
    clear();
}

void NetworkCommunityPat::clear()
{
    s32 i;

    NetworkCommunity::clear();
    for (i = 0; i < 9; i++) {
        this->patRequests_184[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        this->patPool_1A8[i].reset();
    }
    for (i = 0; i < 21; i++) {
        this->pendingIds_344[i] = -1;
    }
    this->lastProfileSend_398 = -3600.0f;
    this->open_39C = 0;
    this->profileDirty_39D = 0;
    this->flag_39E = 0;
    this->selfId_3A0.clear();
    memset(this->name_3C0, 0, sizeof(this->name_3C0));
    memset(this->tag_3D4, 0, sizeof(this->tag_3D4));
    memset(&this->profile_3D8, 0, sizeof(this->profile_3D8));
    this->sentOffset_500 = 0x100;
    this->sentSize_504 = 0;
    this->friends_89C.count_00 = 0;
    this->blocked_2268.count_00 = 0;
}

void NetworkCommunityPat::release()
{
    s32 i;
    PatInterface* pat;

    if (getInstance_() != NULL) {
        PatInterface_clear((PatInterface*)getInstance_());
        if (PatInterface_isReady((PatInterface*)getInstance_()) == 0) {
            pat = (PatInterface*)getInstance_();
            delete pat;
        }
    }
    NetworkCommunity::release();
    for (i = 0; i < 9; i++) {
        deletePatRequest(&this->patRequests_184[i]);
    }
    for (i = 0; i < 2; i++) {
        this->patPool_1A8[i].clear();
    }
}

/* One tick: the base's requests, the friend requests, then the profile bytes once a second while they are dirty. */
void NetworkCommunityPat::move()
{
    u32 offset;

    NetworkCommunity::move();
    movePatRequests();
    if (getInstance_() != NULL && (u8)getNetworkBinaryState(getInstance_()) != 0
        && isPatShutdown((NetworkStateMachine*)getInstance_()) != 0 && this->profileDirty_39D != 0
        && this->requests_0C[10] == 0
        && this->lastProfileSend_398 + communityProfileSendInterval < getNetworkLogger()->getTime_60()) {
        this->lastProfileSend_398 = getNetworkLogger()->getTime_60();
        offset = this->profile_3D8.windowOffset_020;
        sendReqUserBinarySet(getInstance_(), offset, &this->profile_3D8.data_028[offset],
                             (u16)this->profile_3D8.windowSize_024);
        this->profileDirty_39D = 0;
    }
}

/* Hands out a free friend-request record, 0 when both are owned. */
NetworkCommunityPatRequest* NetworkCommunityPat::allocPatRequest()
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (this->patPool_1A8[i].isOwned() == 0) {
            return &this->patPool_1A8[i];
        }
    }
    return 0;
}

inline s32 NetworkCommunityPatRequest::isOwned()
{
    return this->owner_94 != 0;
}

/* Releases the friend request in `slot` (warning when it is still running) and empties the slot. */
void NetworkCommunityPat::deletePatRequest(NetworkCommunityPatRequest** slot)
{
    if (*slot != 0) {
        if ((*slot)->isOwned() != 0) {
            getNetworkLogger()->log_14("NetworkCommunityPat::deleteRequest: request is moving.\n");
        }
        (*slot)->clear();
    }
    *slot = 0;
}

/* Runs every pending friend request and retires the ones whose owner has let go. */
void NetworkCommunityPat::movePatRequests()
{
    s32 i;
    NetworkCommunityPatRequest* req;

    for (i = 0; i < 9; i++) {
        req = this->patRequests_184[i];
        if (req != 0) {
            req->run();
            if (req->isOwned() == 0) {
                deletePatRequest(&this->patRequests_184[i]);
            }
        }
    }
}

/* Runs the request's handler through its member-function pointer; a handler that reports completion resets the
 * record. */
inline void NetworkCommunityPatRequest::run()
{
    if (this->owner_94 != 0 && (this->owner_94->*this->handler_98)(this) != 0) {
        clear();
    }
}

/* The friend-request descriptors the starters pass by value: `{0, -1, handler}`, a member-function pointer to the
 * (non-virtual) handler of the same request, in the order the target lays them out. */
NetworkCommunityPatHandler networkCommunityPatRequestDesc1 = &NetworkCommunityPat::handleRequestPeerProfile;
NetworkCommunityPatHandler networkCommunityPatRequestDesc2 = &NetworkCommunityPat::handleSyncFriends;
NetworkCommunityPatHandler networkCommunityPatRequestDesc3 = &NetworkCommunityPat::handleSendFriendMessage;
NetworkCommunityPatHandler networkCommunityPatRequestDesc4 = &NetworkCommunityPat::handleInviteFriend;
NetworkCommunityPatHandler networkCommunityPatRequestDesc5 = &NetworkCommunityPat::handleRemoveFriend;
NetworkCommunityPatHandler networkCommunityPatRequestDesc6 = &NetworkCommunityPat::handleRequestBlockList;
NetworkCommunityPatHandler networkCommunityPatRequestDesc7 = &NetworkCommunityPat::handleBlockPlayer;
NetworkCommunityPatHandler networkCommunityPatRequestDesc8 = &NetworkCommunityPat::handleUnblockPlayer;

/* Requests the profile of the peer `id`. */
void NetworkCommunityPat::requestPeerProfile(const NetworkUniqueId* id)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[1] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[1] = req;
            req->begin(this, networkCommunityPatRequestDesc1, 1, id);
        }
    }
}

/* Starts the request for `owner`: resets it, stamps the start time and a fresh id, takes the handler and copies up to
 * eight word arguments. */
inline void NetworkCommunityPatRequest::begin(NetworkCommunityPat* owner, NetworkCommunityPatHandler handler, u32 count, ...)
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

void NetworkCommunityPat::syncFriends(NetRosterSync* roster)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[2] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[2] = req;
            req->begin(this, networkCommunityPatRequestDesc2, 1, roster);
        }
    }
}

/* Sends the message `text` to the friend `id`. */
void NetworkCommunityPat::sendFriendMessage(const NetworkUniqueId* id, const char* text)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[3] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[3] = req;
            req->begin(this, networkCommunityPatRequestDesc3, 2, id, text);
        }
    }
}

void NetworkCommunityPat::inviteFriend(const NetworkUniqueId* id, s32 kind)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[4] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[4] = req;
            req->begin(this, networkCommunityPatRequestDesc4, 2, id, kind);
        }
    }
}

void NetworkCommunityPat::removeFriend(const NetworkUniqueId* id)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[5] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[5] = req;
            req->begin(this, networkCommunityPatRequestDesc5, 1, id);
        }
    }
}

void NetworkCommunityPat::requestBlockList(void)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[6] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[6] = req;
            req->begin(this, networkCommunityPatRequestDesc6, 0);
        }
    }
}

void NetworkCommunityPat::blockPlayer(const NetworkUniqueId* id)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[7] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[7] = req;
            req->begin(this, networkCommunityPatRequestDesc7, 1, id);
        }
    }
}

void NetworkCommunityPat::unblockPlayer(const NetworkUniqueId* id)
{
    NetworkCommunityPatRequest* req;

    if (this->patRequests_184[8] == 0) {
        req = allocPatRequest();
        if (req != 0) {
            this->patRequests_184[8] = req;
            req->begin(this, networkCommunityPatRequestDesc8, 1, id);
        }
    }
}

void NetworkCommunityPat::getSelfId(NetworkUniqueId* out)
{
    if (out != NULL) {
        out->copyFrom((const u8*)&this->selfId_3A0);
    }
}

void NetworkCommunityPat::getName(char* out, s32 size)
{
    u32 length;

    if (size > 0) {
        length = (u32)(size - 1) < sizeof(this->name_3C0) - 1 ? (u32)(size - 1) : sizeof(this->name_3C0) - 1;
        memcpy(out, this->name_3C0, length);
        out[length] = 0;
    }
}

void NetworkCommunityPat::getTag(char* out, s32 size)
{
    u32 length;

    if (size > 0) {
        length = (u32)(size - 1) < sizeof(this->tag_3D4) - 1 ? (u32)(size - 1) : sizeof(this->tag_3D4) - 1;
        memcpy(out, this->tag_3D4, length);
        out[length] = 0;
    }
}

/* Opens the community session (command 1): checks the account, installs the Pat callback, then runs the server's
 * login, FMP and binary stages (each ended with a shut request) and finally takes this player's id and name. */
s32 NetworkCommunityPat::handle_68(NetworkCommunityRequest* request)
{
    s32 kind;
    s32 index;
    u8 userId[8];
    NetworkErrorInfo error;
    char name[32];
    NetworkFmpSlot slot;
    s32 result;

    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C != 0) {
            setCollectionLog(request, COMMUNITY_ERR_PROTOCOL, 106, 0);
            request->state_00 = STEP_CANCELLED;
        } else {
            this->open_39C = 1;
            request->state_00 = 5;
        }
        break;
    case 5:
        result = getNetworkLogger()->checkAccount_38(0, &error);
        if (result < 0) {
            setCollectionLog(request, error.code_00, error.param1_04, error.param2_08);
            request->state_00 = STEP_CANCELLED;
        } else if (result > 0) {
            request->state_00 = 10;
        }
        break;
    case 10:
        openPatInterface(getInstance_());
        setCallback(getInstance_(), (void (*)())communityPatEventCallback, this, 6);
        if (getErrorInfo654c(getInstance_(), NULL) != 0) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if ((u8)getNetworkBinaryState(getInstance_()) != 0) {
            request->state_00 = 90;
        } else {
            setConnectServerType(getInstance_(), 0);
            this->replyFlags_2F0[1] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 15;
        }
        break;
    case 15:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->replyFlags_2F0[1] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2(getInstance_());
            if (result < 0) {
                setCollectionLog(request, COMMUNITY_ERR_PROTOCOL, 108, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                this->replyFlags_2F0[1] = 0;
                sendReqShut(getInstance_(), 2);
                request->state_00 = 20;
            }
        }
        break;
    case 20:
        if ((this->replyFlags_2F0[1] & REPLY_SESSION_LOST) || (this->replyFlags_2F0[1] & REPLY_ABORTED) ||
            (this->replyFlags_2F0[1] & 0x10)) {
            this->replyFlags_2F0[1] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 25;
        }
        break;
    case 25:
        if (this->replyFlags_2F0[1] & 0x8) {
            if (isSubState_894F_4or6((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80060034, 0, 0);
                request->state_00 = STEP_CANCELLED;
            } else if (isSubState_894F_5((PatInterface*)getInstance_()) != 0) {
                request->setRecord(0x80060035, 0, 0);
                request->state_00 = STEP_CANCELLED;
            } else if (isSubState_894F_3((PatInterface*)getInstance_()) != 0) {
                request->setRecord(COMMUNITY_ERR_PROTOCOL, 114, 0);
                request->state_00 = STEP_CANCELLED;
            } else {
                request->state_00 = 30;
            }
        }
        break;
    case 30:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (getSomething4(getInstance_()) < 0) {
            request->setRecord(COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (getSomething4(getInstance_()) > 0) {
            this->replyFlags_2F0[1] = 0;
            resetNetworkState(getInstance_());
            request->state_00 = 35;
        }
        setSomething(getInstance_(), 0);
        break;
    case 35:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->replyFlags_2F0[1] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2Fmp(getInstance_());
            if (result < 0) {
                setCollectionLog(request, COMMUNITY_ERR_PROTOCOL, 109, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                this->replyFlags_2F0[1] = 0;
                sendReqShut(getInstance_(), 2);
                request->state_00 = 40;
            }
        }
        break;
    case 40:
        if ((this->replyFlags_2F0[1] & REPLY_SESSION_LOST) || (this->replyFlags_2F0[1] & REPLY_ABORTED) ||
            (this->replyFlags_2F0[1] & 0x10)) {
            this->replyFlags_2F0[1] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 45;
        }
        break;
    case 45:
        if (this->replyFlags_2F0[1] & 0x8) {
            request->state_00 = 50;
        }
        break;
    case 50:
        setConnectServerType(getInstance_(), 1);
        this->replyFlags_2F0[1] = 0;
        resetNetworkState(getInstance_());
        request->state_00 = 55;
        break;
    case 55:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->replyFlags_2F0[1] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setCollectionLog(request, 0x80050012, 110, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                request->state_00 = 60;
            }
        }
        break;
    case 60:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (getSomething4(getInstance_()) < 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
            request->state_00 = STEP_CANCELLED;
        } else if (getSomething4(getInstance_()) > 0) {
            getFmpSelection(getInstance_(), &kind, &index);
            if (kind != 1 || index < 0) {
                setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
                request->state_00 = STEP_CANCELLED;
                break;
            }
            copyFmpSlot(getInstance_(), &slot, index);
            this->replyFlags_2F0[1] = 0;
            sendReqFmpInfo(getInstance_(), slot.payload_00, 1);
            request->state_00 = 65;
        }
        setSomething(getInstance_(), 0);
    case 65:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->replyFlags_2F0[1] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else if (this->replyFlags_2F0[1] & 0x20) {
            this->replyFlags_2F0[1] = 0;
            sendReqShut(getInstance_(), 2);
            request->state_00 = 70;
        }
        break;
    case 70:
        if ((this->replyFlags_2F0[1] & REPLY_SESSION_LOST) || (this->replyFlags_2F0[1] & REPLY_ABORTED) ||
            (this->replyFlags_2F0[1] & 0x10)) {
            this->replyFlags_2F0[1] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 75;
        }
        break;
    case 75:
        if (this->replyFlags_2F0[1] & 0x8) {
            request->state_00 = 80;
        }
        break;
    case 80:
        setConnectServerType(getInstance_(), 1);
        this->replyFlags_2F0[1] = 0;
        resetNetworkState(getInstance_());
        request->state_00 = 85;
        break;
    case 85:
        if (this->replyFlags_2F0[1] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
        } else if (this->replyFlags_2F0[1] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
        } else {
            result = handleNetworkState2Binary(getInstance_());
            if (result < 0) {
                setCollectionLog(request, COMMUNITY_ERR_PROTOCOL, 110, result);
                request->state_00 = STEP_CANCELLED;
            } else if (result > 0) {
                request->state_00 = 90;
            }
        }
        break;
    case 90:
        getSelectedID(getInstance_(), userId);
        this->selfId_3A0.importFrom(3, userId, sizeof(userId));
        getSelectedHunterName(getInstance_(), name);
        memcpy(this->name_3C0, name, sizeof(this->name_3C0) - 1);
        this->name_3C0[sizeof(this->name_3C0) - 1] = 0;
        memset(this->tag_3D4, 0, sizeof(this->tag_3D4));
        notifyReflect(1, 0, 0, NULL, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(1, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(1, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Copies the request's error record out under its mutex; false while none is set. */
inline s32 NetworkCommunityRequest::getRecord(NetworkErrorInfo* out)
{
    s32 result;

    result = 0;
    LockMutex(this->mutex_78);
    if (this->record_54 != 0) {
        result = 1;
        out->code_00 = this->record_54;
        out->param1_04 = this->record_58;
        out->param2_08 = this->record_5C;
    }
    UnlockMutex(this->mutex_78);
    return result;
}

/* Sets the request's error record under its mutex. */
inline void NetworkCommunityRequest::setRecord(u32 code, u32 arg0, u32 arg1)
{
    LockMutex(this->mutex_78);
    this->record_58 = arg0;
    this->record_5C = arg1;
    this->record_54 = code;
    UnlockMutex(this->mutex_78);
}

/* Closes the community session (command 2): shuts the server side down when this layer holds the last reference. */
s32 NetworkCommunityPat::handle_6C(NetworkCommunityRequest* request)
{
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            request->state_00 = 40;
            break;
        }
        if (getInstance_() == NULL) {
            request->state_00 = 30;
            break;
        }
        if (isCallback(getInstance_(), 6) == 0) {
            request->state_00 = 30;
            break;
        }
        if (hasMultipleRefs60d4(getInstance_()) != 0) {
            request->state_00 = 20;
            break;
        }
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            request->state_00 = 20;
            break;
        }
        this->replyFlags_2F0[2] = 0;
        sendReqShut(getInstance_(), 1);
        request->state_00 = 5;
        break;
    case 5:
        if ((this->replyFlags_2F0[2] & REPLY_SESSION_LOST) || (this->replyFlags_2F0[2] & REPLY_ABORTED) ||
            (this->replyFlags_2F0[2] & 0x10)) {
            this->replyFlags_2F0[2] = 0;
            resetNetworkState3(getInstance_());
            request->state_00 = 10;
        }
        break;
    case 10:
        if (this->replyFlags_2F0[2] & 0x8) {
            request->state_00 = 20;
        }
        break;
    case 20:
        resetCallback(getInstance_(), 6);
        decrement60d4(getInstance_());
        request->state_00 = 30;
        break;
    case 30:
        if (getNetworkLogger()->isVerbose_3C() > 0) {
            request->state_00 = 40;
        }
        break;
    case 40:
        this->open_39C = 0;
        notifyReflect(2, 0, 0, NULL, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Reports command 4 with the value 22 and finishes. */
s32 NetworkCommunityPat::handle_70(NetworkCommunityRequest* request)
{
    s32 value;

    value = 22;
    notifyReflect(4, 0, 1, &value, this->reflectUser_08);
    return 1;
}

/* Reports command 5 with the value 5 and finishes. */
s32 NetworkCommunityPat::handle_74(NetworkCommunityRequest* request)
{
    s32 value;

    value = 5;
    notifyReflect(5, 0, 1, &value, this->reflectUser_08);
    return 1;
}

/* Fetches the friend list (command 6, at most 50 entries) and delivers it. */
s32 NetworkCommunityPat::handle_78(NetworkCommunityRequest* request)
{
    NetworkErrorInfo error;

    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->friends_89C.count_00 = 0;
        this->replyFlags_2F0[6] = 0;
        this->pendingIds_344[6] = sendReqFriendList(getInstance_(), 1, 50);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[6] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[6] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[6] & 0x10000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        notifyReflect(6, 0, 1, &this->friends_89C, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(6, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(6, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Reports the unsupported-request code for command 7 and finishes. */
s32 NetworkCommunityPat::handle_7C(NetworkCommunityRequest* request)
{
    NetworkErrorInfo error;

    error.code_00 = COMMUNITY_ERR_UNSUPPORTED;
    error.param1_04 = 0;
    error.param2_08 = 0;
    notifyReflect(7, error.code_00, 1, &error, this->reflectUser_08);
    return 1;
}

/* Sends the mail `text` with the word `value` to the player `id` (command 9) and delivers it as a mail record. */
s32 NetworkCommunityPat::handle_80(NetworkCommunityRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    u32 options[12];
    const char* text;
    u32 value;
    NetworkUniqueId* id;
    u32 length;

    text = (const char*)request->getArgument(0);
    value = request->getArgument(1);
    id = (NetworkUniqueId*)request->getArgument(2);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if ((s8)text[0] == 0 || id == NULL || id->isValid() == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(options, 0, sizeof(options));
        options[0] = value;
        this->replyFlags_2F0[8] = 0;
        this->pendingIds_344[8] = sendReqTell(getInstance_(), recipient, options, text);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[8] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[8] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[8] & 0x800) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE: {
        NetworkCommunityMail mail;

        mail.sender_000.copyFrom((const u8*)&this->selfId_3A0);
        memcpy(mail.name_020, this->name_3C0, sizeof(mail.name_020) - 1);
        mail.name_020[sizeof(mail.name_020) - 1] = 0;
        memcpy(mail.tag_034, this->tag_3D4, sizeof(mail.tag_034) - 1);
        mail.tag_034[sizeof(mail.tag_034) - 1] = 0;
        length = strlen(text) < sizeof(mail.text_035) - 2 ? strlen(text) : sizeof(mail.text_035) - 2;
        memcpy(mail.text_035, text, length);
        mail.text_035[length] = 0;
        mail.text_035[sizeof(mail.text_035) - 1] = 0;
        mail.value_238 = value;
        mail.time_23C = getServerDateTime(getInstance_());
        notifyReflect(9, 0, 1, &mail, this->reflectUser_08);
        return 1;
    }
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(9, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(9, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* The starter's word argument `index`, 0 (and a warning) past the count it was given. */
inline s32 NetworkCommunityRequest::getArgument(u32 index)
{
    u32 count;

    count = this->count_28;
    if (count <= index) {
        getNetworkLogger()->warn_10("NetworkRequest::getArgument: arg no over %d <= %d\n", count, index);
        return 0;
    }
    return (s32)this->args_2C[index];
}

/* Fetches `size` bytes from `offset` of the peer `id`'s profile (command 11) and delivers the peer profile block. */
s32 NetworkCommunityPat::handle_84(NetworkCommunityRequest* request)
{
    NetworkErrorInfo error;
    u8 query[560];
    NetworkUniqueId* id;
    u32 size;
    u32 offset;

    id = (NetworkUniqueId*)request->getArgument(0);
    size = request->getArgument(1);
    offset = request->getArgument(2);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->flag_39E != 0) {
            break;
        }
        if (id == NULL || size == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(query, 0, sizeof(query));
        id->exportTo(query, 8);
        if ((s8)query[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->flag_39E = 1;
        this->replyFlags_2F0[9] = 0;
        this->pendingIds_344[9] = sendReqUserSearchInfo(getInstance_(), query, 1);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[9] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[9] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[9] & 0x40) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        this->peerProfile_508.windowOffset_020 = offset;
        this->peerProfile_508.windowSize_024 -= this->peerProfile_508.windowSize_024 < offset
            ? this->peerProfile_508.windowSize_024 : offset;
        if (this->peerProfile_508.windowSize_024 < size) {
            size = this->peerProfile_508.windowSize_024;
        }
        this->peerProfile_508.windowSize_024 = size;
        if (this->peerProfile_508.windowOffset_020 != 0 && size != 0) {
            memmove(this->peerProfile_508.data_028, &this->peerProfile_508.data_028[offset], size);
        }
        notifyReflect(11, 0, 1, &this->peerProfile_508, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(11, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(11, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Writes `size` profile bytes at `offset` (command 12), sends them, and announces the change to `target`. */
s32 NetworkCommunityPat::handle_88(NetworkCommunityRequest* request)
{
    u8 targetId[8];
    u8 noticeId[8];
    NetworkErrorInfo error;
    const u8* data;
    NetworkCommunityTarget* target;
    u32 sendOffset;
    u32 size;
    u32 offset;
    s32 mode;
    u8 kind;

    target = (NetworkCommunityTarget*)request->getArgument(0);
    data = (const u8*)request->getArgument(1);
    size = request->getArgument(2);
    offset = request->getArgument(3);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (target == NULL || data == NULL || offset + size > 0x100) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        switch (target->kind_00) {
        case 0:
            mode = 0;
            break;
        case 1:
            mode = 1;
            break;
        case 2:
            mode = 2;
            break;
        case 3:
            mode = 3;
            break;
        case 4:
            mode = 4;
            break;
        default:
            mode = -1;
            break;
        }
        if (target->kind_00 == 1) {
            memset(targetId, 0, sizeof(targetId));
            target->id_04->exportTo(targetId, sizeof(targetId));
            if ((s8)targetId[0] == 0) {
                mode = -1;
            }
        }
        if (mode < 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        writeProfileRange_50(data, size, offset);
        this->lastProfileSend_398 = getNetworkLogger()->getTime_60();
        this->replyFlags_2F0[10] = 0;
        sendOffset = this->profile_3D8.windowOffset_020;
        this->pendingIds_344[10] = sendReqUserBinarySet(getInstance_(), sendOffset,
                                                        &this->profile_3D8.data_028[sendOffset],
                                                        (u16)this->profile_3D8.windowSize_024);
        this->profileDirty_39D = 0;
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[10] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[10] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[10] & 0x80) {
            if (target->kind_00 == 0) {
                request->state_00 = 20;
            } else {
                request->state_00 = STEP_DONE;
            }
        }
        break;
    case STEP_DONE:
        kind = target->kind_00;
        if (target->kind_00 == 1) {
            memset(noticeId, 0, sizeof(noticeId));
            target->id_04->exportTo(noticeId, sizeof(noticeId));
        }
        this->replyFlags_2F0[10] = 0;
        this->pendingIds_344[10] = sendReqUserBinaryNotice(getInstance_(), kind,
                                                           target->kind_00 == 1 ? (const char*)noticeId : NULL,
                                                           offset, size);
        request->state_00 = 15;
        break;
    case 15:
        if (this->replyFlags_2F0[10] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[10] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[10] & 0x100) {
            request->state_00 = 20;
        }
        break;
    case 20:
        notifyReflect(12, 0, 0, NULL, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(12, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(12, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Sends `size` bytes (at most 0x100) to the player `id` (command 14). */
s32 NetworkCommunityPat::handle_8C(NetworkCommunityRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    const u8* data;
    u32 size;
    NetworkUniqueId* id;

    data = (const u8*)request->getArgument(0);
    size = request->getArgument(1);
    id = (NetworkUniqueId*)request->getArgument(2);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (id == NULL || size > 0x100) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->replyFlags_2F0[11] = 0;
        this->pendingIds_344[11] = sendReqBinaryUser(getInstance_(), recipient, data, (u16)size);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[11] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[11] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[11] & 0x1000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        notifyReflect(14, 0, 0, NULL, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(14, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(14, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Requests the peer profile of `id` (command 17) and delivers the peer block. */
s32 NetworkCommunityPat::handleRequestPeerProfile(NetworkCommunityPatRequest* request)
{
    NetworkErrorInfo error;
    u8 query[560];
    NetworkUniqueId* id;

    id = (NetworkUniqueId*)request->getArgument(0);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->flag_39E != 0) {
            break;
        }
        if (id == NULL) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(query, 0, sizeof(query));
        id->exportTo(query, 8);
        if ((s8)query[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->flag_39E = 1;
        this->replyFlags_2F0[13] = 0;
        this->pendingIds_344[13] = sendReqUserSearchInfo(getInstance_(), query, 2);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[13] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[13] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[13] & 0x200) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        notifyReflect(17, 0, 1, &this->peer_630, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(17, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(17, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Copies the request's error record out under its mutex; false while none is set. */
inline s32 NetworkCommunityPatRequest::getRecord(NetworkErrorInfo* out)
{
    s32 result;

    result = 0;
    LockMutex(this->mutex_78);
    if (this->record_54 != 0) {
        result = 1;
        out->code_00 = this->record_54;
        out->param1_04 = this->record_58;
        out->param2_08 = this->record_5C;
    }
    UnlockMutex(this->mutex_78);
    return result;
}

/* The starter's word argument `index`, 0 (and a warning) past the count it was given. */
inline s32 NetworkCommunityPatRequest::getArgument(u32 index)
{
    u32 count;

    count = this->count_28;
    if (count <= index) {
        getNetworkLogger()->warn_10("NetworkRequest::getArgument: arg no over %d <= %d\n", count, index);
        return 0;
    }
    return (s32)this->args_2C[index];
}

/* Sends the roster settings (command 18): the first seven keys of `roster` packed into one byte each. */
s32 NetworkCommunityPat::handleSyncFriends(NetworkCommunityPatRequest* request)
{
    u8 settings[7];
    NetworkErrorInfo error;
    NetRosterSync* roster;
    s32 i;

    roster = (NetRosterSync*)request->getArgument(0);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (roster == NULL || roster->count_0x00 == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(settings, -1, sizeof(settings));
        for (i = 0; i < roster->count_0x00; i++) {
            switch ((u32)roster->items_0x04[i].key_0x00) {
            case 1:
                settings[0] = roster->items_0x04[i].value_0x04;
                break;
            case 2:
                settings[1] = roster->items_0x04[i].value_0x04;
                break;
            case 3:
                settings[2] = roster->items_0x04[i].value_0x04;
                break;
            case 4:
                settings[3] = roster->items_0x04[i].value_0x04;
                break;
            case 5:
                settings[4] = roster->items_0x04[i].value_0x04;
                break;
            case 6:
                settings[5] = roster->items_0x04[i].value_0x04;
                break;
            case 7:
                settings[6] = roster->items_0x04[i].value_0x04;
                break;
            }
        }
        this->replyFlags_2F0[14] = 0;
        this->pendingIds_344[14] = sendReqUserStatusSet(getInstance_(), settings);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[14] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[14] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[14] & 0x400) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        notifyReflect(18, 0, 1, roster, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(18, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(18, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Sends the message `text` to the friend `id` (command 19) and delivers it as a message record. */
s32 NetworkCommunityPat::handleSendFriendMessage(NetworkCommunityPatRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    u32 options[12];
    NetworkUniqueId* id;
    const char* text;
    u32 length;

    id = (NetworkUniqueId*)request->getArgument(0);
    text = (const char*)request->getArgument(1);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if ((s8)text[0] == 0 || id == NULL || id->isValid() == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(options, 0, sizeof(options));
        this->replyFlags_2F0[15] = 0;
        this->pendingIds_344[15] = sendReqFriendAdd(getInstance_(), recipient, options, text);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[15] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[15] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[15] & 0x2000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE: {
        NetworkCommunityMessage message;

        message.sender_000.copyFrom((const u8*)id);
        length = strlen(text) < sizeof(message.text_035) - 2 ? strlen(text) : sizeof(message.text_035) - 2;
        memset(message.name_020, 0, sizeof(message.name_020));
        memset(message.tag_034, 0, sizeof(message.tag_034));
        memcpy(message.text_035, text, length);
        message.text_035[length] = 0;
        notifyReflect(19, 0, 1, &message, this->reflectUser_08);
        return 1;
    }
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(19, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(19, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Sends an invite of type `kind` to the friend `id` (command 21) and delivers it as an invite record. */
s32 NetworkCommunityPat::handleInviteFriend(NetworkCommunityPatRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    NetworkUniqueId* id;
    s32 kind;

    id = (NetworkUniqueId*)request->getArgument(0);
    kind = request->getArgument(1);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (id == NULL || id->isValid() == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->replyFlags_2F0[16] = 0;
        this->pendingIds_344[16] = sendReqFriendAccept(getInstance_(), (const char*)recipient, (u8)kind);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[16] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[16] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[16] & 0x4000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE: {
        NetworkCommunityInvite invite;

        invite.id_000.copyFrom((const u8*)id);
        memset(invite.name_020, 0, sizeof(invite.name_020));
        memset(invite.tag_034, 0, sizeof(invite.tag_034));
        invite.type_035 = kind == 1 ? 1 : 2;
        notifyReflect(21, 0, 1, &invite, this->reflectUser_08);
        return 1;
    }
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(21, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(21, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Removes the friend `id` (command 23): asks the server, then drops the entry from the friend list and tells the layer. */
s32 NetworkCommunityPat::handleRemoveFriend(NetworkCommunityPatRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    NetworkUniqueId* id;
    s32 i;
    NetworkCommunityFriend* dst;
    NetworkCommunityFriend* src;

    id = (NetworkUniqueId*)request->getArgument(0);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (id == NULL || id->isValid() == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->replyFlags_2F0[17] = 0;
        this->pendingIds_344[17] = sendReqFriendDelete(getInstance_(), (const char*)recipient);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[17] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[17] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[17] & 0x8000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        if (this->friends_89C.count_00 > 50) {
            this->friends_89C.count_00 = 50;
        }
        for (i = 0; i < this->friends_89C.count_00; i++) {
            if (id->equals(&this->friends_89C.entries_04[i].id_00) != 0) {
                break;
            }
        }
        if (i < this->friends_89C.count_00) {
            this->friends_89C.count_00--;
            for (; i < this->friends_89C.count_00; i++) {
                dst = &this->friends_89C.entries_04[i];
                src = &this->friends_89C.entries_04[i + 1];
                dst->id_00.copyFrom((const u8*)&src->id_00);
                memcpy(dst->name_20, src->name_20, sizeof(dst->name_20) - 1);
                dst->name_20[sizeof(dst->name_20) - 1] = 0;
                memcpy(dst->tag_34, src->tag_34, sizeof(dst->tag_34) - 1);
                dst->tag_34[sizeof(dst->tag_34) - 1] = 0;
            }
            if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                getNetworkLayerPat(getPatsObject(), 0)->setFriendTransferModeById(id, 0);
            }
        }
        notifyReflect(23, 0, 1, id, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(23, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(23, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Fetches the block list (command 24, at most 16 entries) and delivers it. */
s32 NetworkCommunityPat::handleRequestBlockList(NetworkCommunityPatRequest* request)
{
    NetworkErrorInfo error;

    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->blocked_2268.count_00 = 0;
        this->replyFlags_2F0[18] = 0;
        this->pendingIds_344[18] = sendReqBlackList(getInstance_(), 1, 16);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[18] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[18] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[18] & 0x80000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        notifyReflect(24, 0, 1, &this->blocked_2268, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(24, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(24, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Blocks the player `id` (command 25): asks the server, then appends the entry to the block list and tells the layer. */
s32 NetworkCommunityPat::handleBlockPlayer(NetworkCommunityPatRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    u32 options[11];
    NetworkUniqueId* id;
    NetworkCommunityBlocked* entry;

    id = (NetworkUniqueId*)request->getArgument(0);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (id == NULL || id->isValid() == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(options, 0, sizeof(options));
        this->replyFlags_2F0[19] = 0;
        this->pendingIds_344[19] = sendReqBlackAdd(getInstance_(), recipient, options);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[19] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[19] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[19] & 0x20000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        if (this->blocked_2268.count_00 < 0) {
            this->blocked_2268.count_00 = 0;
        }
        if (this->blocked_2268.count_00 < 16) {
            entry = &this->blocked_2268.entries_04[this->blocked_2268.count_00];
            entry->id_00.copyFrom((const u8*)id);
            memset(entry->name_20, 0, sizeof(entry->name_20));
            memset(entry->tag_34, 0, sizeof(entry->tag_34));
            this->blocked_2268.count_00++;
            if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                getNetworkLayerPat(getPatsObject(), 0)->setFriendTransferModeById(id, 0);
            }
        }
        notifyReflect(25, 0, 1, id, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(25, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(25, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Unblocks the player `id` (command 26): asks the server, then drops the entry from the block list and tells the
 * layer whether the player is a friend. */
s32 NetworkCommunityPat::handleUnblockPlayer(NetworkCommunityPatRequest* request)
{
    u8 recipient[8];
    NetworkErrorInfo error;
    NetworkUniqueId* id;
    s32 i;
    NetworkCommunityBlocked* dst;
    NetworkCommunityBlocked* src;

    id = (NetworkUniqueId*)request->getArgument(0);
    if (isPatShutdown((NetworkStateMachine*)getInstance_()) == 0) {
        setCollectionLog(request, COMMUNITY_ERR_NOT_LOGGED_IN, 0, 0);
        request->state_00 = STEP_CANCELLED;
    }
    switch (request->state_00) {
    case STEP_START:
        if (this->open_39C == 0) {
            setCollectionLog(request, COMMUNITY_ERR_NOT_OPEN, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (id == NULL || id->isValid() == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        memset(recipient, 0, sizeof(recipient));
        id->exportTo(recipient, sizeof(recipient));
        if ((s8)recipient[0] == 0) {
            setCollectionLog(request, COMMUNITY_ERR_BAD_ARGUMENT, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        this->replyFlags_2F0[20] = 0;
        this->pendingIds_344[20] = sendReqBlackDelete(getInstance_(), (const char*)recipient);
        request->state_00 = STEP_WAIT;
        break;
    case STEP_WAIT:
        if (this->replyFlags_2F0[20] & REPLY_SESSION_LOST) {
            setCollectionLogSessionLost(request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (this->replyFlags_2F0[20] & REPLY_ABORTED) {
            setCollectionLogAborted(request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (this->replyFlags_2F0[20] & 0x40000) {
            request->state_00 = STEP_DONE;
        }
        break;
    case STEP_DONE:
        if (this->blocked_2268.count_00 > 16) {
            this->blocked_2268.count_00 = 16;
        }
        for (i = 0; i < this->blocked_2268.count_00; i++) {
            if (id->equals(&this->blocked_2268.entries_04[i].id_00) != 0) {
                break;
            }
        }
        if (i < this->blocked_2268.count_00) {
            this->blocked_2268.count_00--;
            for (; i < this->blocked_2268.count_00; i++) {
                dst = &this->blocked_2268.entries_04[i];
                src = &this->blocked_2268.entries_04[i + 1];
                dst->id_00.copyFrom((const u8*)&src->id_00);
                memcpy(dst->name_20, src->name_20, sizeof(dst->name_20) - 1);
                dst->name_20[sizeof(dst->name_20) - 1] = 0;
                memcpy(dst->tag_34, src->tag_34, sizeof(dst->tag_34) - 1);
                dst->tag_34[sizeof(dst->tag_34) - 1] = 0;
            }
            if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                getNetworkLayerPat(getPatsObject(), 0)->setFriendTransferModeById(id, isFriend(id));
            }
        }
        notifyReflect(26, 0, 1, id, this->reflectUser_08);
        return 1;
    case STEP_CANCELLED:
        request->getRecord(&error);
        notifyReflect(26, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    case STEP_FAILED:
        request->getRecord(&error);
        notifyReflect(26, error.code_00, 1, &error, this->reflectUser_08);
        notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
        return 1;
    }
    return 0;
}

/* Whether `id` is a friend and not on the block list. */
s32 NetworkCommunityPat::isAcceptedPeer(const NetworkUniqueId* id)
{
    s32 i;

    if (id == NULL) {
        return 0;
    }
    for (i = 0; i < this->blocked_2268.count_00; i++) {
        if (id->equals(&this->blocked_2268.entries_04[i].id_00) != 0) {
            return 0;
        }
    }
    return isFriend(id);
}

/* Hands `command` and its result to the reflect callback; a failure with a record attached reports the kept Pat error
 * instead when there is one. */
/* untyped: caller-owned payload - the record each command delivers */
void NetworkCommunityPat::notifyReflect(u32 command, s32 result, s32 count, void* data, u32 user)
{
    if (result < 0 && data != NULL && getInstance_() != NULL && getErrorInfo654c(getInstance_(), NULL) != 0) {
        getErrorInfoOrCode654c(getInstance_(), COMMUNITY_ERR_SESSION, (NetworkErrorInfo*)data);
        result = ((NetworkErrorInfo*)data)->code_00;
    }
    this->reflectCallback_04(command, result, count, data, user);
}

/* The Pat community message payloads `onPatEvent` reads (layouts from the reads; GUESS on the names). */
struct NetworkCommunityAbortPayload {        /* 0x8002 */
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ s32 reason_04;               /* 266 when the peer profile fetch was the aborted request */
};   /* size: 0x08 (approximation: the fields read) */

struct NetworkCommunityMailPayload {         /* 0x806B, 0x8079 */
    /* +0x000 */ char text_000[256];
    /* +0x100 */ u32 value_100;
    /* +0x104 */ s32 time_104;
    /* +0x108 */ u8 sender_108[8];
    /* +0x110 */ char name_110[20];
};   /* size: 0x124 (approximation: the fields read) */

struct NetworkCommunityDataPayload {         /* 0x806D */
    /* +0x000 */ u8 data_000[256];
    /* +0x100 */ u16 size_100;
    /* +0x102 */ u8 pad_102[2];
    /* +0x104 */ u32 value_104;
    /* +0x108 */ u8 sender_108[8];
    /* +0x110 */ char name_110[20];
};   /* size: 0x124 (approximation: the fields read) */

struct NetworkCommunityNoticePayload {       /* 0x8071 */
    /* +0x000 */ u8 owner_000[8];
    /* +0x008 */ u8 unused_008;
    /* +0x009 */ u8 data_009[256];
    /* +0x109 */ u8 pad_109[3];
    /* +0x10C */ u32 size_10C;
    /* +0x110 */ u32 offset_110;
};   /* size: 0x114 */

struct NetworkCommunityProfilePayload {      /* 0x8072, 0x8073 */
    /* +0x000 */ u8 owner_000[8];
    /* +0x008 */ char name_008[32];
    /* +0x028 */ u8 data_028[256];
    /* +0x128 */ u16 size_128;
    /* +0x12A */ u8 pad_12A[2];
    /* +0x12C */ u8 layerId_12C[16];
    /* +0x13C */ u8 unused_13C;
    /* +0x13D */ char names_13D[192];        /* tab-separated */
    /* +0x1FD */ s8 kind_1FD;
    /* +0x1FE */ char title_1FE[34];
    /* +0x220 */ u32 value_220;
    /* +0x224 */ u32 value_224;
};   /* size: 0x228 */

struct NetworkCommunityInvitePayload {       /* 0x8077 */
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u8 sender_04[8];
    /* +0x0C */ char name_0C[32];
    /* +0x2C */ u8 type_2C;
};   /* size: 0x30 (approximation: the fields read) */

struct NetworkCommunityFriendPayload {       /* 0x807B, one per friend */
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u8 id_04[8];
    /* +0x0C */ char name_0C[36];
};   /* size: 0x30 (the stride) */

struct NetworkCommunityBlockedPayload {      /* 0x807E, one per blocked player */
    /* +0x00 */ u32 unused_00;
    /* +0x04 */ u8 id_04[8];
    /* +0x0C */ char name_0C[32];
};   /* size: 0x2C (the stride) */

/* The Pat server's community messages: each code sets the reply bits the waiting request handlers poll, or delivers a
 * received record (mail, data, profile notice, invite, message) or the friend and block lists. */
void NetworkCommunityPat::onPatEvent(s32 code, s32 requestId, s32 flag, s32 count, const u8* data)
{
    char* token;
    s32 i;
    s32 j;
    u32 length;
    NetworkCommunityFriend* entry;
    NetworkCommunityBlocked* blocked;
    const NetworkCommunityMailPayload* mailIn;
    const NetworkCommunityDataPayload* dataIn;
    const NetworkCommunityNoticePayload* noticeIn;
    const NetworkCommunityProfilePayload* profileIn;
    const NetworkCommunityInvitePayload* inviteIn;
    const NetworkCommunityFriendPayload* friendIn;
    const NetworkCommunityBlockedPayload* blockedIn;

    switch (code) {
    case 0x8000:
    case 0x8007:
        for (i = 0; i < 12; i++) {
            if (this->requests_0C[i] != 0) {
                break;
            }
        }
        if (i == 12) {
            for (j = 0; j < 9; j++) {
                if (this->patRequests_184[j] != 0) {
                    break;
                }
            }
            if (j == 9) {
                NetworkErrorInfo error;

                getErrorInfoOrCode654c(getInstance_(), COMMUNITY_ERR_SESSION, &error);
                notifyReflect(3, error.code_00, 1, &error, this->reflectUser_08);
            }
        }
        this->flag_39E = 0;
        for (i = 0; i < 21; i++) {
            this->replyFlags_2F0[i] |= REPLY_SESSION_LOST;
        }
        break;
    case 0x8006:
        this->flag_39E = 0;
        this->replyFlags_2F0[1] |= 0x10;
        this->replyFlags_2F0[2] |= 0x10;
        for (i = 0; i < 21; i++) {
            this->replyFlags_2F0[i] |= REPLY_SESSION_LOST;
        }
        break;
    case 0x8002:
        if (((const NetworkCommunityAbortPayload*)data)->reason_04 == 266) {
            this->flag_39E = 0;
        }
        this->replyFlags_2F0[1] |= REPLY_ABORTED;
        this->replyFlags_2F0[2] |= REPLY_ABORTED;
        for (i = 0; i < 21; i++) {
            if (requestId == this->pendingIds_344[i]) {
                this->replyFlags_2F0[i] |= REPLY_ABORTED;
            }
        }
        break;
    case 0x8004:
        if (flag != 0) {
            for (i = 0; i < 21; i++) {
                this->replyFlags_2F0[i] |= REPLY_SESSION_LOST;
            }
        }
        break;
    case 0x8005:
        this->replyFlags_2F0[1] |= 0x8;
        this->replyFlags_2F0[2] |= 0x8;
        break;
    case 0x8008:
        this->replyFlags_2F0[1] |= 0x20;
        break;
    case 0x806A:
        this->replyFlags_2F0[8] |= 0x800;
        break;
    case 0x806B:
        mailIn = (const NetworkCommunityMailPayload*)data;
        if ((s8)mailIn->text_000[0] != 0 && (s8)mailIn->sender_108[0] != 0) {
            NetworkCommunityMail mail;

            mail.sender_000.importFrom(3, mailIn->sender_108, sizeof(mailIn->sender_108));
            memcpy(mail.name_020, mailIn->name_110, sizeof(mail.name_020) - 1);
            mail.name_020[sizeof(mail.name_020) - 1] = 0;
            memset(mail.tag_034, 0, sizeof(mail.tag_034));
            memcpy(mail.text_035, mailIn->text_000, sizeof(mailIn->text_000) - 1);
            mail.text_035[sizeof(mailIn->text_000) - 1] = 0;
            mail.text_035[sizeof(mail.text_035) - 1] = 0;
            mail.value_238 = mailIn->value_100;
            mail.time_23C = mailIn->time_104;
            notifyReflect(10, 0, 1, &mail, this->reflectUser_08);
        }
        break;
    case 0x8072:
        profileIn = (const NetworkCommunityProfilePayload*)data;
        this->flag_39E = 0;
        this->peerProfile_508.owner_000.importFrom(3, profileIn->owner_000, sizeof(profileIn->owner_000));
        this->peerProfile_508.windowOffset_020 = 0;
        this->peerProfile_508.windowSize_024 = profileIn->size_128 < sizeof(this->peerProfile_508.data_028)
            ? profileIn->size_128 : sizeof(this->peerProfile_508.data_028);
        memcpy(this->peerProfile_508.data_028, profileIn->data_028, this->peerProfile_508.windowSize_024);
        this->replyFlags_2F0[9] |= 0x40;
        break;
    case 0x8073:
        profileIn = (const NetworkCommunityProfilePayload*)data;
        this->flag_39E = 0;
        this->peer_630.profile_000.owner_000.importFrom(3, profileIn->owner_000, sizeof(profileIn->owner_000));
        this->peer_630.profile_000.windowOffset_020 = 0;
        this->peer_630.profile_000.windowSize_024 = profileIn->size_128 < sizeof(this->peer_630.profile_000.data_028)
            ? profileIn->size_128 : sizeof(this->peer_630.profile_000.data_028);
        memcpy(this->peer_630.profile_000.data_028, profileIn->data_028, this->peer_630.profile_000.windowSize_024);
        NetworkLayerIdImportFrom(&this->peer_630.layerId_128, 3, profileIn->layerId_12C,
                                 sizeof(profileIn->layerId_12C));
        this->peer_630.nameCount_168 = 0;
        this->peer_630.names_16C[0][0] = 0;
        this->peer_630.names_16C[1][0] = 0;
        this->peer_630.names_16C[2][0] = 0;
        token = strtok((char*)profileIn->names_13D, "\t");
        while (token != NULL) {
            i = this->peer_630.nameCount_168;
            if (i >= 3) {
                break;
            }
            length = strlen(token) < sizeof(this->peer_630.names_16C[0]) - 1 ? strlen(token)
                                                                              : sizeof(this->peer_630.names_16C[0]) - 1;
            memcpy(this->peer_630.names_16C[i], token, length);
            this->peer_630.names_16C[this->peer_630.nameCount_168][length] = 0;
            this->peer_630.nameCount_168++;
            token = strtok(NULL, "\t");
        }
        this->peer_630.isRemote_22C = profileIn->kind_1FD == 2;
        length = strlen(profileIn->title_1FE) < sizeof(this->peer_630.title_22D) - 1 ? strlen(profileIn->title_1FE)
                                                                                     : sizeof(this->peer_630.title_22D) - 1;
        memcpy(this->peer_630.title_22D, profileIn->title_1FE, length);
        this->peer_630.title_22D[length] = 0;
        memcpy(this->peer_630.name_24D, profileIn->name_008, sizeof(this->peer_630.name_24D) - 1);
        this->peer_630.name_24D[sizeof(this->peer_630.name_24D) - 1] = 0;
        memset(this->peer_630.tag_261, 0, sizeof(this->peer_630.tag_261));
        this->peer_630.value_264 = profileIn->value_224;
        this->peer_630.value_268 = profileIn->value_220;
        this->replyFlags_2F0[13] |= 0x200;
        break;
    case 0x806F:
        if (requestId == this->pendingIds_344[10]) {
            this->replyFlags_2F0[10] |= 0x80;
        }
        break;
    case 0x8070:
        this->replyFlags_2F0[10] |= 0x100;
        break;
    case 0x8071: {
        NetworkCommunityProfile notice;

        noticeIn = (const NetworkCommunityNoticePayload*)data;
        if ((s8)noticeIn->owner_000[0] == 0) {
            break;
        }
        notice.owner_000.importFrom(3, noticeIn->owner_000, sizeof(noticeIn->owner_000));
        notice.windowOffset_020 = noticeIn->offset_110;
        notice.windowSize_024 = noticeIn->size_10C < sizeof(notice.data_028) ? noticeIn->size_10C : sizeof(notice.data_028);
        memcpy(notice.data_028, noticeIn->data_009, notice.windowSize_024);
        notifyReflect(13, 0, 1, &notice, this->reflectUser_08);
        break;
    }
    case 0x8074:
        this->replyFlags_2F0[14] |= 0x400;
        break;
    case 0x806C:
        this->replyFlags_2F0[11] |= 0x1000;
        break;
    case 0x806D: {
        NetworkCommunityData record;

        dataIn = (const NetworkCommunityDataPayload*)data;
        if ((s8)dataIn->sender_108[0] == 0) {
            break;
        }
        record.sender_000.importFrom(3, dataIn->sender_108, sizeof(dataIn->sender_108));
        memcpy(record.name_020, dataIn->name_110, sizeof(record.name_020) - 1);
        record.name_020[sizeof(record.name_020) - 1] = 0;
        memset(record.tag_034, 0, sizeof(record.tag_034));
        record.value_038 = dataIn->value_104;
        record.size_03C = dataIn->size_100 < sizeof(record.data_040) ? dataIn->size_100 : sizeof(record.data_040);
        memcpy(record.data_040, dataIn->data_000, record.size_03C);
        notifyReflect(15, 0, 1, &record, this->reflectUser_08);
        break;
    }
    case 0x8076:
        this->replyFlags_2F0[15] |= 0x2000;
        break;
    case 0x8077: {
        NetworkCommunityInvite invite;

        inviteIn = (const NetworkCommunityInvitePayload*)data;
        if ((s8)inviteIn->sender_04[0] == 0) {
            break;
        }
        invite.id_000.importFrom(3, inviteIn->sender_04, sizeof(inviteIn->sender_04));
        memcpy(invite.name_020, inviteIn->name_0C, sizeof(invite.name_020) - 1);
        invite.name_020[sizeof(invite.name_020) - 1] = 0;
        memset(invite.tag_034, 0, sizeof(invite.tag_034));
        invite.type_035 = inviteIn->type_2C;
        if (invite.type_035 == 1) {
            if (this->friends_89C.count_00 < 0) {
                this->friends_89C.count_00 = 0;
            }
            if (this->friends_89C.count_00 < 50) {
                NetworkUniqueId id;

                id.importFrom(3, inviteIn->sender_04, sizeof(inviteIn->sender_04));
                for (i = 0; i < this->friends_89C.count_00; i++) {
                    if (id.equals(&this->friends_89C.entries_04[i].id_00) != 0) {
                        break;
                    }
                }
                if (i >= this->friends_89C.count_00) {
                    entry = &this->friends_89C.entries_04[this->friends_89C.count_00];
                    entry->id_00.importFrom(3, inviteIn->sender_04, sizeof(inviteIn->sender_04));
                    memcpy(entry->name_20, inviteIn->name_0C, sizeof(entry->name_20) - 1);
                    entry->name_20[sizeof(entry->name_20) - 1] = 0;
                    memset(entry->tag_34, 0, sizeof(entry->tag_34));
                    this->friends_89C.count_00++;
                    if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                        getNetworkLayerPat(getPatsObject(), 0)->setFriendTransferModeById(&entry->id_00,
                                                                                      isAcceptedPeer(&entry->id_00));
                    }
                }
            }
        }
        notifyReflect(22, 0, 1, &invite, this->reflectUser_08);
        break;
    }
    case 0x8078:
        this->replyFlags_2F0[16] |= 0x4000;
        break;
    case 0x8079: {
        NetworkCommunityMessage message;

        mailIn = (const NetworkCommunityMailPayload*)data;
        if ((s8)mailIn->sender_108[0] == 0) {
            break;
        }
        message.sender_000.importFrom(3, mailIn->sender_108, sizeof(mailIn->sender_108));
        memcpy(message.name_020, mailIn->name_110, sizeof(message.name_020) - 1);
        message.name_020[sizeof(message.name_020) - 1] = 0;
        memset(message.tag_034, 0, sizeof(message.tag_034));
        memcpy(message.text_035, mailIn->text_000, sizeof(mailIn->text_000) - 1);
        message.text_035[sizeof(mailIn->text_000) - 1] = 0;
        notifyReflect(20, 0, 1, &message, this->reflectUser_08);
        break;
    }
    case 0x807A:
        this->replyFlags_2F0[17] |= 0x8000;
        break;
    case 0x807B:
        if (count < 0) {
            NetworkErrorInfo error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            NetworkPostedError posted(*(NetworkPostedError*)&error);
            ((NetworkInstanceDispatch*)getInstance_())->postError((NetworkErrorInfo*)&posted);
            break;
        }
        this->friends_89C.count_00 = count < 50 ? count : 50;
        friendIn = (const NetworkCommunityFriendPayload*)data;
        for (i = 0; i < this->friends_89C.count_00; i++) {
            entry = &this->friends_89C.entries_04[i];
            entry->id_00.importFrom(3, friendIn->id_04, sizeof(friendIn->id_04));
            memcpy(entry->name_20, friendIn->name_0C, sizeof(entry->name_20) - 1);
            entry->name_20[sizeof(entry->name_20) - 1] = 0;
            memset(entry->tag_34, 0, sizeof(entry->tag_34));
            if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                getNetworkLayerPat(getPatsObject(), 0)->setFriendTransferModeById(&entry->id_00, isAcceptedPeer(&entry->id_00));
            }
            friendIn++;
        }
        this->replyFlags_2F0[6] |= 0x10000;
        break;
    case 0x807C:
        this->replyFlags_2F0[19] |= 0x20000;
        break;
    case 0x807D:
        this->replyFlags_2F0[20] |= 0x40000;
        break;
    case 0x807E:
        if (count < 0) {
            NetworkErrorInfo error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            NetworkPostedError posted(*(NetworkPostedError*)&error);
            ((NetworkInstanceDispatch*)getInstance_())->postError((NetworkErrorInfo*)&posted);
            break;
        }
        this->blocked_2268.count_00 = count < 16 ? count : 16;
        blockedIn = (const NetworkCommunityBlockedPayload*)data;
        for (i = 0; i < this->blocked_2268.count_00; i++) {
            blocked = &this->blocked_2268.entries_04[i];
            blocked->id_00.importFrom(3, blockedIn->id_04, sizeof(blockedIn->id_04));
            memcpy(blocked->name_20, blockedIn->name_0C, sizeof(blocked->name_20) - 1);
            blocked->name_20[sizeof(blocked->name_20) - 1] = 0;
            memset(blocked->tag_34, 0, sizeof(blocked->tag_34));
            if (getNetworkLayerPat(getPatsObject(), 0) != NULL) {
                getNetworkLayerPat(getPatsObject(), 0)->setFriendTransferModeById(&blocked->id_00, 0);
            }
            blockedIn++;
        }
        this->replyFlags_2F0[18] |= 0x80000;
        break;
    case 0x8001:
        break;
    case 0x8003:
        break;
    }
}

/* Reports the unsupported-request code for command 7. */
void NetworkCommunityPat::rejectRequest_40()
{
    NetworkErrorInfo error;

    error.code_00 = COMMUNITY_ERR_UNSUPPORTED;
    error.param1_04 = 0;
    error.param2_08 = 0;
    notifyReflect(7, error.code_00, 1, &error, this->reflectUser_08);
}

/* Whether `id` is on the friend list. */
s32 NetworkCommunityPat::isFriend(const NetworkUniqueId* id)
{
    s32 i;

    if (id == NULL) {
        return 0;
    }
    for (i = 0; i < this->friends_89C.count_00; i++) {
        if (id->equals(&this->friends_89C.entries_04[i].id_00) != 0) {
            return 1;
        }
    }
    return 0;
}

/* Writes the whole profile block. */
void NetworkCommunityPat::writeProfile_4C(const u8* data, u32 size)
{
    writeProfileRange_50(data, size, 0);
}

/* Copies `size` bytes at `offset` of the block `data` into this player's profile and widens the dirty window and the
 * window still to be sent to cover them. */
void NetworkCommunityPat::writeProfileRange_50(const u8* data, u32 size, u32 offset)
{
    u32 end;

    end = offset + size;
    if (end > 0x100) {
        return;
    }
    if (this->profileDirty_39D != 0) {
        if (this->profile_3D8.windowOffset_020 > offset) {
            this->profile_3D8.windowSize_024 += this->profile_3D8.windowOffset_020 - offset;
            this->profile_3D8.windowOffset_020 = offset;
        }
        if (this->profile_3D8.windowOffset_020 + this->profile_3D8.windowSize_024 < end) {
            this->profile_3D8.windowSize_024 = end - this->profile_3D8.windowOffset_020;
        }
    } else {
        this->profile_3D8.windowSize_024 = size;
        this->profile_3D8.windowOffset_020 = offset;
        this->profileDirty_39D = 1;
    }
    if (this->sentOffset_500 > this->profile_3D8.windowOffset_020) {
        if (this->sentOffset_500 == 0x100) {
            this->sentOffset_500 = this->profile_3D8.windowOffset_020;
        } else {
            this->sentSize_504 += this->sentOffset_500 - this->profile_3D8.windowOffset_020;
            this->sentOffset_500 = this->profile_3D8.windowOffset_020;
        }
    }
    if (this->sentOffset_500 + this->sentSize_504 < this->profile_3D8.windowSize_024 + this->profile_3D8.windowOffset_020) {
        this->sentSize_504 = this->profile_3D8.windowOffset_020 + this->profile_3D8.windowSize_024 - this->sentOffset_500;
    }
    memcpy(&this->profile_3D8.data_028[offset], &data[offset], size);
}

/* Records the kept Pat session error on `request`. */
void NetworkCommunityPat::setCollectionLogSessionLost(NetworkCommunityRequest* request)
{
    NetworkErrorInfo error;

    getErrorInfoOrCode654c(getInstance_(), COMMUNITY_ERR_SESSION, &error);
    request->setRecord(error.code_00, error.param1_04, error.param2_08);
}

/* Records the kept Pat session error on the friend request `request`. */
void NetworkCommunityPat::setCollectionLogSessionLost(NetworkCommunityPatRequest* request)
{
    NetworkErrorInfo error;

    getErrorInfoOrCode654c(getInstance_(), COMMUNITY_ERR_SESSION, &error);
    request->setRecord(error.code_00, error.param1_04, error.param2_08);
}

/* Sets the request's error record under its mutex. */
inline void NetworkCommunityPatRequest::setRecord(u32 code, u32 arg0, u32 arg1)
{
    LockMutex(this->mutex_78);
    this->record_58 = arg0;
    this->record_5C = arg1;
    this->record_54 = code;
    UnlockMutex(this->mutex_78);
}

/* Records the aborted-request error on `request`. */
void NetworkCommunityPat::setCollectionLogAborted(NetworkCommunityRequest* request)
{
    NetworkErrorInfo error;

    buildErrorInfo613c(getInstance_(), COMMUNITY_ERR_PROTOCOL, &error);
    request->setRecord(error.code_00, error.param1_04, error.param2_08);
}

/* Records the aborted-request error on the friend request `request`. */
void NetworkCommunityPat::setCollectionLogAborted(NetworkCommunityPatRequest* request)
{
    NetworkErrorInfo error;

    buildErrorInfo613c(getInstance_(), COMMUNITY_ERR_PROTOCOL, &error);
    request->setRecord(error.code_00, error.param1_04, error.param2_08);
}

/* Sends the error `code` with its two words to the server, clears the kept error and records it on `request`. */
void NetworkCommunityPat::setCollectionLog(NetworkCommunityRequest* request, u32 code, u32 arg0, u32 arg1)
{
    u32 values[3];

    values[0] = code;
    values[1] = arg0;
    values[2] = arg1;
    sendServerTimeout(getInstance_(), values);
    clearErrorRecord613c(getInstance_());
    request->setRecord(code, arg0, arg1);
}

/* Sends the error `code` with its two words to the server, clears the kept error and records it on the friend request
 * `request`. */
void NetworkCommunityPat::setCollectionLog(NetworkCommunityPatRequest* request, u32 code, u32 arg0, u32 arg1)
{
    u32 values[3];

    values[0] = code;
    values[1] = arg0;
    values[2] = arg1;
    sendServerTimeout(getInstance_(), values);
    clearErrorRecord613c(getInstance_());
    request->setRecord(code, arg0, arg1);
}

/* How long `move` waits between two profile sends (declared at the top of the file). */
extern const f32 communityProfileSendInterval = 1.0f;
