/*
 * Network/NetworkCommunity.cpp - the community layer base class `NetworkCommunity` that `NetworkCommunityPat` builds on,
 *   the counterpart of `NetworkLayer`: twelve request slots, a pool of two `NetworkCommunityRequest` records and the
 *   lazy request starters.
 * RANGE. .text 0x803EF668-0x803F0294 (27 functions); .data 0x805FC390-0x805FC4D0 (the ten request descriptors, the
 *   deleteRequest string, the table 0x805FC440), .sdata2 0x8079C7A0-0x8079C7A8 (the record's 0.0f), extab, extabindex.
 *   The left edge is 0x803EF668, not 0x803EF300: the `setCollectionLog*` siblings before it take a `NetworkLayerPat*`
 *   and close `NetworkLayerPat`'s band.
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md); file-scope
 *   `#pragma peephole off` (with it on, `reset` fuses the `__ptmf_null` copy into `lwzu`).
 * NAMES. The class name is its own "NetworkCommunity::deleteRequest" string's; the descriptors are the named
 *   `NetworkCommunityHandler` globals `networkCommunityRequestDescNN`; every other name is a GUESS from the slot offset,
 *   the field it touches or what the pat control passes.
 * RESIDUALS. `__ct__16NetworkCommunityFv`: the pool loop keeps its counter and pointer registers swapped (r31/r30
 *   against retail r30/r31; the comma-form loop is the closest spelling of those measured).  `extab`: retail records
 *   the record's mutex as a member object (`~NetworkCommunityRequest` names 0x803CA338), while here it is a byte block
 *   with explicit constructor and destructor calls - the mutex class is
 *   `Network/NetworkStreamSink.cpp`'s and shared with `NetworkLayer`, `NetworkSessionManager` and five other units.  `.sdata2`: the object carries 4 B of the claimed 8.
 * SHAPES. `__vt__16NetworkCommunity` is emitted here from the class (key function the destructor, rule 10).
 */
#include "Network/NetworkCommunity.h"            /* the unit's own header */
#include "Network/NetworkSessionManager.h"       /* NetworkRequest_idCounter, NetworkVaState */
#include "unsplit/Network.h"                     /* getNetworkLogger */
#include "types.h"

/* Retail keeps the unfused lis/addi/lwz forms of the null member pointer copy (see the file header). */
#pragma peephole off

NetworkCommunity::NetworkCommunity()
{
    s32 i;
    NetworkCommunityRequest* req;

    this->reflectCallback_04 = 0;
    this->reflectUser_08 = 0;
    for (i = 0; i < 12; i++) {
        this->requests_0C[i] = 0;
    }
    for (i = 0, req = this->pool_3C; i < 2; i++, req++) {
        req->reset();
    }
}

/* Returns a request record to its idle state. */
void NetworkCommunityRequest::reset()
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

NetworkCommunityRequest::~NetworkCommunityRequest()
{
    clear();
}

void NetworkCommunityRequest::clear()
{
    reset();
}

NetworkCommunityRequest::NetworkCommunityRequest()
{
    reset();
}

NetworkCommunity::~NetworkCommunity()
{
    NetworkCommunity::release();
}

void NetworkCommunity::setReflectCallback(u32 callback, u32 user)
{
    this->reflectCallback_04 = (NetworkCommunityReflectCallback)callback;
    this->reflectUser_08 = user;
    NetworkCommunity::clear();
}

void NetworkCommunity::clear()
{
    s32 i;

    for (i = 0; i < 12; i++) {
        this->requests_0C[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        this->pool_3C[i].reset();
    }
}

void NetworkCommunity::release()
{
    s32 i;

    for (i = 0; i < 12; i++) {
        deleteRequest(&this->requests_0C[i]);
    }
    for (i = 0; i < 2; i++) {
        this->pool_3C[i].clear();
    }
}

/* One tick of the request state machine: runs every pending request and retires the ones whose owner has let go. */
void NetworkCommunity::move()
{
    s32 i;
    NetworkCommunityRequest* req;

    for (i = 0; i < 12; i++) {
        req = this->requests_0C[i];
        if (req != 0) {
            req->run();
            if (req->isOwned() == 0) {
                deleteRequest(&this->requests_0C[i]);
            }
        }
    }
}

#pragma dont_inline on
s32 NetworkCommunityRequest::isOwned()
{
    return this->owner_94 != 0;
}
#pragma dont_inline off

/* Runs the request's handler through its member-function pointer; a handler that reports completion resets the
 * record. */
void NetworkCommunityRequest::run()
{
    if (this->owner_94 != 0 && (this->owner_94->*this->handler_98)(this) != 0) {
        clear();
    }
}

/* The request descriptors the starters pass by value: `{0, handler slot, 0}`, a member-function pointer to the pure
 * handler slot of the same request, in the order the target lays them out. */
NetworkCommunityHandler networkCommunityRequestDesc1C = &NetworkCommunity::handle_68;
NetworkCommunityHandler networkCommunityRequestDesc20 = &NetworkCommunity::handle_6C;
NetworkCommunityHandler networkCommunityRequestDesc30 = &NetworkCommunity::handle_70;
NetworkCommunityHandler networkCommunityRequestDesc34 = &NetworkCommunity::handle_74;
NetworkCommunityHandler networkCommunityRequestDesc38 = &NetworkCommunity::handle_78;
NetworkCommunityHandler networkCommunityRequestDesc3C = &NetworkCommunity::handle_7C;
NetworkCommunityHandler networkCommunityRequestDesc48 = &NetworkCommunity::handle_80;
NetworkCommunityHandler networkCommunityRequestDesc58 = &NetworkCommunity::handle_84;
NetworkCommunityHandler networkCommunityRequestDesc60 = &NetworkCommunity::handle_88;
NetworkCommunityHandler networkCommunityRequestDesc64 = &NetworkCommunity::handle_8C;

void NetworkCommunity::openCommunity_1C()
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[1] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[1] = req;
            req->begin(this, networkCommunityRequestDesc1C, 0);
        }
    }
}

/* Starts the request for `owner`: resets it, stamps the start time and a fresh id, takes the handler and copies up to
 * eight word arguments. */
void NetworkCommunityRequest::begin(NetworkCommunity* owner, NetworkCommunityHandler handler, u32 count, ...)
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

void NetworkCommunity::shutdown_20()
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[2] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[2] = req;
            req->begin(this, networkCommunityRequestDesc20, 0);
        }
    }
}

void NetworkCommunity::request_30()
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[4] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[4] = req;
            req->begin(this, networkCommunityRequestDesc30, 0);
        }
    }
}

void NetworkCommunity::request_34()
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[5] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[5] = req;
            req->begin(this, networkCommunityRequestDesc34, 0);
        }
    }
}

void NetworkCommunity::requestNews_38()
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[6] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[6] = req;
            req->begin(this, networkCommunityRequestDesc38, 0);
        }
    }
}

void NetworkCommunity::request_3C()
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[7] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[7] = req;
            req->begin(this, networkCommunityRequestDesc3C, 0);
        }
    }
}

void NetworkCommunity::request_48(u32 a, u32 b, u32 c)
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[8] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[8] = req;
            req->begin(this, networkCommunityRequestDesc48, 3, a, b, c);
        }
    }
}

void NetworkCommunity::request_54(u32 a, u32 b)
{
    request_58(a, b, 0);
}

void NetworkCommunity::request_58(u32 a, u32 b, u32 c)
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[9] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[9] = req;
            req->begin(this, networkCommunityRequestDesc58, 3, a, b, c);
        }
    }
}

void NetworkCommunity::request_5C(const u8* data, s32 id, u32 size)
{
    request_60(data, id, size, 0);
}

void NetworkCommunity::request_60(const u8* data, s32 id, u32 size, u32 flags)
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[10] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[10] = req;
            req->begin(this, networkCommunityRequestDesc60, 4, data, id, size, flags);
        }
    }
}

void NetworkCommunity::request_64(u32 a, u32 b, u32 c)
{
    NetworkCommunityRequest* req;

    if (this->requests_0C[11] == 0) {
        req = allocRequest();
        if (req != 0) {
            this->requests_0C[11] = req;
            req->begin(this, networkCommunityRequestDesc64, 3, a, b, c);
        }
    }
}

/* Hands out a free request of the pool, 0 when both are owned. */
NetworkCommunityRequest* NetworkCommunity::allocRequest()
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (this->pool_3C[i].isOwned() == 0) {
            return &this->pool_3C[i];
        }
    }
    return 0;
}

/* Releases the request in `slot` (warning when it is still running) and empties the slot. */
void NetworkCommunity::deleteRequest(NetworkCommunityRequest** slot)
{
    if (*slot != 0) {
        if ((*slot)->isOwned() != 0) {
            getNetworkLogger()->log_14("NetworkCommunity::deleteRequest: request is moving.\n");
        }
        (*slot)->clear();
    }
    *slot = 0;
}
