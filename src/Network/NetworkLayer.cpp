/*
 * Network/NetworkLayer.cpp - the layer base class `NetworkLayer` (`.text` 0x803DF2EC..0x803E0BE8).
 *
 * WHAT IT IS.  The base `NetworkLayerPat` builds on: its own "NetworkLayer::move"/"NetworkLayer::deleteRequest" strings, the
 *   table 0x805FB5D0 = `__vt__12NetworkLayer` (emitted here from the class - key function `~NetworkLayer`), declared in
 *   `Network/NetworkLayer.h`.  It parallels `NetworkSessionManager` one word earlier; its request record `NetworkLayerRequest`
 *   carries a real pointer-to-member handler (the reset is MWCC's null-member-pointer copy of `__ptmf_null`, `run` its
 *   `__ptmf_scall` call) and a real ctor/dtor (the pool's `__construct_array` pair).  The 22 descriptor constants are the
 *   named `NetworkLayerHandler` globals `networkLayerRequestDescNN`.  The layer id helpers (`NetworkLayerIdImportFrom`/
 *   `ExportTo`, `NetworkUniqueIdEquals`, named by their own log strings) sit in the same range.
 *
 * WHY IT SITS HERE (recut, network pilot round 3).  The bodies were written by pilot lane L2 inside
 *   `Network/NetworkSessionManagerPat.cpp`; the `.data` there read as three TUs (V->S seam at 0x805FB2B8), and the unit
 *   holds exactly the run that seam opens: `.text` from the constructor 0x803DF2EC (the manager's last function,
 *   `setSessionLogSessionLost`, ends there) to 0x803E0BE8 (`NetworkLayerPat`'s first), `.data` 0x805FB2B8..0x805FB718 (its
 *   strings, the descriptors 0x805FB494.. and the table), `.sdata2` 0x8079C778..0x8079C780 (read only from this range),
 *   extab 0x8001A6A8..0x8001A810, extabindex 0x8003AC38..0x8003AE0C.  The bodies moved byte-identical.
 *
 * FLAGS.  Measured on this unit's own rows (configure.py carries the evidence): `cflags_main` with `-pool off` (playbook 43 -
 *   the id helpers address each warning string with their own lis/addi).  File-scope `#pragma peephole off`: the bodies
 *   were written under the source file's peephole-off region and keep it.
 *
 * TABLE (round 3).  79 slots (+0x08..+0x140): the derived table 0x805FC1E0 is 0x144 B, so the base has no slot +0x144 -
 *   the map's `__vt__12NetworkLayer` is 0x144 B and the claim's last 4 bytes (0x805FB714) are the alignment before the next
 *   object's `.data` (our object's `.data` is 0x45C of the claimed 0x460).  The pure slots carry the names and parameters of
 *   `NetworkLayerPat`'s overrides; the request starters the game calls carry the consumers' spellings (`closeSession_1C`,
 *   `requestServers_24`, ... - `Network/network_pat_control.cpp`), with the argument types those call sites pass.
 *
 * RESIDUALS.  `NetworkLayer::move` keeps the unrolled inner scan's trip count 3 in r0 where retail holds it in a
 *   callee-saved register (r29) - one extra saved register shifts every allocation; the constructor's second pool loop is
 *   spelled with an explicit pointer (98.17; the indexed spelling scores 97.32) and still swaps the counter/pointer
 *   registers; the id helpers differ only in their string labels' names.  Every name except the class's own is derived
 *   from the slot offset or the field it touches (GUESS).
 */
#include "Network/NetworkLayer.h"                /* the unit's own header: the layer base class and its free functions */
#include "Network/NetworkSessionManager.h"       /* networkInstance_initMutex - the record's mutex */
#include "unsplit/Network.h"                     /* getNetworkLogger, dtor_803CA338 - no registered owner */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"                         /* memcmp - owner MSL_C/alloc.cpp */
#include "types.h"

/* The bodies were written under a peephole-off region (see the file header). */
#pragma peephole off

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

void NetworkLayer::setReflectCallback(u32 callback, u32 user)
{
    this->context_04 = callback;
    this->context_08 = user;
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

/* Sets `id` to `size` bytes (at most 0x3C) of id kind `kind`, warning on a bad argument. */
void NetworkLayerIdImportFrom(NetworkLayerId* id, u8 kind, const u8* data, u32 size)
{
    if (id == NULL) {
        getNetworkLogger()->warn_10("NetworkLayerIdImportFrom: arg->this is null.\n");
        return;
    }
    if (data == NULL) {
        getNetworkLogger()->warn_10("NetworkLayerIdImportFrom: arg->data is null.\n");
        return;
    }
    if (size > 0x3C) {
        getNetworkLogger()->warn_10("NetworkLayerIdImportFrom: this->max < arg->size\n");
        return;
    }
    if (size == 0) {
        getNetworkLogger()->log_14("NetworkLayerIdImportFrom: arg->size is zero.\n");
        return;
    }
    memset(id, 0, sizeof(id->data_04));
    id->kind_00 = kind;
    id->pad_01[0] = 0;
    id->pad_01[1] = 0;
    id->pad_01[2] = 0;
    memcpy(id->data_04, data, size);
}

/* Copies `size` id bytes (at most 0x3C) out of `id`, warning on a bad argument. */
void NetworkLayerIdExportTo(const NetworkLayerId* id, u8* out, u32 size)
{
    if (id == NULL) {
        getNetworkLogger()->warn_10("NetworkLayerIdExportTo: arg->layer_id is null.\n");
        return;
    }
    if (out == NULL) {
        getNetworkLogger()->warn_10("NetworkLayerIdExportTo: arg->data is null.\n");
        return;
    }
    if (size > 0x3C) {
        getNetworkLogger()->warn_10("NetworkLayerIdExportTo: this->len < arg->size\n");
        return;
    }
    memcpy(out, id->data_04, size);
}

/* True when both ids are of the same known kind (1..5) and their first 0x40 bytes agree. */
BOOL NetworkUniqueIdEquals(const NetworkLayerId* a, const NetworkLayerId* b)
{
    s32 kind;

    if (a == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdEquals: arg1 is null.\n");
        return FALSE;
    }
    if (b == NULL) {
        getNetworkLogger()->warn_10("NetworkUniqueIdEquals: arg2 is null.\n");
        return FALSE;
    }
    kind = a->kind_00;
    if (kind != (s32)b->kind_00) {
        return FALSE;
    }
    switch (kind) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return memcmp(a, b, 0x40) == 0;
    }
    return FALSE;
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
NetworkLayerHandler networkLayerRequestDesc1C = &NetworkLayer::handleConnect;
NetworkLayerHandler networkLayerRequestDesc20 = &NetworkLayer::handleDisconnect;
NetworkLayerHandler networkLayerRequestDesc24 = &NetworkLayer::handleServerList;
NetworkLayerHandler networkLayerRequestDesc28 = &NetworkLayer::handleServerSelect;
NetworkLayerHandler networkLayerRequestDesc38 = &NetworkLayer::stepRequest;
NetworkLayerHandler networkLayerRequestDesc3C = &NetworkLayer::handleLayerCreate;
NetworkLayerHandler networkLayerRequestDesc40 = &NetworkLayer::handleLayerCreate;
NetworkLayerHandler networkLayerRequestDesc44 = &NetworkLayer::handleLayerInfo;
NetworkLayerHandler networkLayerRequestDesc48 = &NetworkLayer::handleChildList;
NetworkLayerHandler networkLayerRequestDesc4C = &NetworkLayer::handleSiblingList;
NetworkLayerHandler networkLayerRequestDesc50 = &NetworkLayer::handleUserList;
NetworkLayerHandler networkLayerRequestDesc54 = &NetworkLayer::handleUserList;
NetworkLayerHandler networkLayerRequestDesc58 = &NetworkLayer::handleUserInfo;
NetworkLayerHandler networkLayerRequestDesc64 = &NetworkLayer::handleChat;
NetworkLayerHandler networkLayerRequestDesc68 = &NetworkLayer::handleDetailSearch;
NetworkLayerHandler networkLayerRequestDesc6C = &NetworkLayer::handleUserSearch;
NetworkLayerHandler networkLayerRequestDesc70 = &NetworkLayer::handleLayerJump;
NetworkLayerHandler networkLayerRequestDesc74 = &NetworkLayer::handleLayerInfoById;
NetworkLayerHandler networkLayerRequestDesc78 = &NetworkLayer::handleLayerInfoSet;
NetworkLayerHandler networkLayerRequestDesc7C = &NetworkLayer::handleMediationLock;
NetworkLayerHandler networkLayerRequestDesc80 = &NetworkLayer::handleMediationUnlock;
NetworkLayerHandler networkLayerRequestDesc84 = &NetworkLayer::handleMediationList;

void NetworkLayer::closeSession_1C()
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

void NetworkLayer::shutdown_20()
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

void NetworkLayer::requestServers_24(s32 a)
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

void NetworkLayer::selectServer_28(s32 a)
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

void NetworkLayer::selectCity_3C(s32 a)
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

void NetworkLayer::requestCities_48(s32 a)
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

void NetworkLayer::sendMessage_64(const char* a, s32 b, s32 c, u32 d)
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

void NetworkLayer::setPageSize_68(s32 a)
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

void NetworkLayer::requestRefresh_6C(s32 a, s32 b)
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

void NetworkLayer::requestAccount_84(s32 a)
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

#pragma peephole on
void NetworkLayer::setFlag75(u32 value)
{
    this->flag_75 = value;
}
#pragma peephole off

u8 NetworkLayer::getFlag75()
{
    return this->flag_75;
}

void NetworkLayer::setFlag76(u8 value)
{
    this->flag_76 = value;
}
