/*
 * Network/NetworkSessionManager.cpp - the request pool and state machine of `NetworkSessionManager` (21 request slots, a
 *   two-slot `NetworkRequest` pool at +0x7C) and the first `NetworkSessionManagerPat` methods (its request pair, the
 *   per-player records, the 32-entry circle list).
 * RANGE. .text 0x803D4904-0x803D70B8 (89 functions); .data 0x805FA788-0x805FAB48, .sbss 0x80794CA0-0x80794CA8, extab,
 *   extabindex.  The left edge is `NetworkSessionStable`'s `.data` seam: its op-code writers, rate governor,
 *   `moveOutOfBand` and `getUsableSlot` (0x803D3CE8..0x803D4904) have their strings before its tables. `datagap.py
 *   --unit` reads an emission-order seam inside: retail puts the base table between `deleteRequest`'s string and
 *   `getArgument`'s, so the original TU ends after the table (`.text` boundary in 0x803D6514..0x803D65F4) - not cut.
 * FLAGS. `-O3` (configure.py; measured in docs/network.md).  `#pragma dont_inline on` scopes keep the Pat
 *   methods' calls to the base's `clear`/`init`/`release` and the record destructors as `bl`s (else MWCC inlines 220 of
 *   `clear`'s 276 bytes); `#pragma peephole off` scopes (below).
 * NAMES. The file name is a GUESS (no `__FILE__` string; `dumpmap.py` answers only `zz_` names).  The log strings name
 *   their emitters ("NetworkSessionManager::move: request[%d] is moving ..." 0x805FA788,
 *   "NetworkSessionManagerPat::final ..." 0x805FAB08); each `requestNNN` is named by the op code of its 12-byte `{0,
 *   opcode, 0}` descriptor; the float constants are named from their use (`networkRateMax` 1.0f, `networkRateScale`
 *   2.0f, `networkMillisecondsPerSecond` 1000.0f).  GUESSes: `NetworkSessionSlotInfo`,
 *   `NetworkSessionCircleInfo`/`List`, `NetworkSessionPlayerRecord`, `NetworkRequestPat` (container offsets,
 *   `net_va_arg`/`memset` use), `networkSessionReflectCallbackEx` and the `network<span>*` accessors (the callee each
 *   forwards to), the `slot_14C`..`slot_168` wrappers (their vtable slot).
 *   GUESS: NetworkRequestHead (the request fields ahead of the mutex both request records share).
 *   GUESS: clear, reset (`NetworkRequestPat`'s own record reset and its forwarder).
 * RESIDUALS. `move`: one callee-saved register fewer (`_savegpr_23`/`_restgpr_23` against retail's `_savegpr_22`/`_restgpr_22`); the log strings
 *   retail names (`NetworkSessionManager_deleteRequestMessage`, `NetworkRequest_getArgumentMessage`,
 *   `NetworkSessionManager_moveStandByMessage`,
 *   `NetworkSessionManagerPat_finalMessage`) are this object's anonymous literals.
 *   `.text` 0x2714 against 0x27B4; `extab` 0x428 against 0x40C; `.data` (flip blocker) 0x3B8 against 0x3C0 (-8; 225 of 960
 *   bytes differ), written but out of order (the base table last, see RANGE); `.sbss` emits the 4 B word of the 8 B
 *   claim.
 *  - `NetworkRequest_copyRecord`: retail copies the 96-byte record as two words then eleven word pairs, `*dst = *src`
 *    emits `lmw`/`stmw` (`-use_lmw_stmw off` changes nothing), so `copyRecord` is 96 B short; it is referenced only from
 *    this object's `extab`;
 *  - `slot_144`, `sendBatch_138`, `slot_13C`, `slot_140`: the staging order of the mapped byte array, and the `clrlwi`
 *    retail gives the `u8` flags argument at the call (no spelling tried reaches the `mr`/`clrlwi` pair);
 *  - `networkSessionReflectCallback`: retail saves all six incoming argument registers before building the callee's,
 *    ours does the minimal four-move rotation;
 *  - `~NetworkRequest`: retail's unfused `extsh` of the deleting flag;
 *  - the constructor, `release`: the loop counter and element pointer take swapped registers (r30/r31);
 *    `NetworkSessionManager_allocRequest`: the pointer steps after the counter (a pointer loop costs it the saved `self`);
 *    `clear`, `move`: not characterised one by one (the objdiff rows).
 * SHAPES. Both tables are compiler output (rule 10): the base table 0x805FA908..0x805FAAD0 (456 B) is emitted here;
 *   `NetworkSessionManagerPat` declares `move` FIRST so it is the key function (its body opens the next unit), so the
 *   Pat constructor stores `__vt__24NetworkSessionManagerPat` without a second table here.  `NetworkBuffer` (table
 *   0x805F9150) is a declared class with no virtual defined here, so no table is emitted for it.
 *  - the record classes embed a `NetworkUniqueId` with out-of-line constructors/destructors in the target's order; the
 *    unique id's constructor is out of line, so no element's vptr is stored inline;
 *  - the 21 `request*` rows sit in two `#pragma peephole off`/`on` pairs (on, MWCC fuses the descriptor address into
 *    `lwzu r4,@l(r5)`, 4 B short; `-opt nopeephole` changes nothing, playbook 39/41); the Pat
 *    constructor/`init`/`release` (with the 0x44A0-byte `GameSpyInterfaceThread` view), destructor, `clear` and record
 *    ctors/dtors run under `#pragma peephole off` too (retail's `lwz r12,0(r3)` after the `mr r3,this` copy, the
 *    unfused `extsh`+`cmpwi` of the deleting flag);
 *  - `NetworkSessionSlot` is 0x924 B (the target's `mulli` stride); `NetworkStreamWriter` is 0x20 B (`send8`'s frame is
 *    0x30 with the writer at +0x10);
 *  - `NetworkRequest_begin`'s inlined `va_list` setup is `__builtin_va_info(&ap)` (the `net_va_start` macro, not the
 *    `__va_start` call `va_start` would give); the request handler is a real pointer-to-member (`NetworkRequestDesc`):
 *    the resets copy `__ptmf_null` (0x80572428, owned by `Runtime.PPCEABI.H/ptmf.c`), `NetworkRequest::run` is MWCC's
 *    `__ptmf_scall`; `release`'s close loop is a `do {} while`.
 */

#include "types.h"
#include "Network/NetworkSessionManager.h"
#include "Network/NetworkSessionManagerPat.h"   /* the Pat buffer helpers and the reflection adapters */
#include "Network/NetworkUniqueId.h"            /* NetworkUniqueId - the records' address objects */
#include "Network/gamespy_interface_types.h"    /* GameSpyInterfaceThread / NetworkErrorInfo - owner Network/GameSpyInterfaceThread.cpp */
#include "Network/GameSpyInterfaceThread.h"     /* sGameSpyInterfaceThread - owner Network/GameSpyInterfaceThread.cpp */
#include "Network/PatInterface.h"              /* PatInterface - owner Network/PatInterface.cpp */

/* `NetworkVaState` and the two variadic intrinsics live in this unit's header (rule 2 keeps the
   declaration with the TU that needs it). */
#define net_va_start(ap) __builtin_va_info(&(ap))
#define net_va_arg(ap, type) (*(type*)__va_arg(&(ap), 1))

/* Rule 2: the symbols this unit calls but does not own come from their owner's headers, never from a
   declaration here - `getInstance_` (0x803768F0) is inside `enemy/em020_ai.cpp`'s registered range,
   `memset` is `Runtime.PPCEABI.H/memset.c`, and `getNetworkLogger` (0x803C9974, the band's log-manager
   getter) is owned by nobody, so it is declared with the rest of the Network band in
   `unsplit/Network.h` - where it is typed as the class `NetworkLogger` the logging band uses.  The
   sites below keep the older `NetworkSessionManagerLogger` view of that object and cast. */
#include "enemy/em020_ai.h"
#include "unsplit/Network.h"
#include "Runtime.PPCEABI.H/memset.h"

/* EXCEPTIONS.  The lib builds with `-Cpp_exceptions off` while this range's target object carries
   `extab` 1260 B and `extabindex` 912 B (the C++ unwind records of the class below); the pragma turns the
   front-end's exceptions back on and moves no `.text` byte. */

/* ---- this unit's own forward declarations ---- */
extern "C" {

void NetworkRequest_reset(NetworkRequest*);
void NetworkRequest_clear(NetworkRequest*);
s32 NetworkRequest_isOwned(NetworkRequest*);
void NetworkRequest_begin(NetworkRequest*, NetworkSessionManager*, NetworkRequestDesc, u32, ...);
void NetworkRequest_cancel(NetworkRequest*);
s32 NetworkRequest_isCancelled(NetworkRequest*);
NetworkRequest* NetworkSessionManager_allocRequest(NetworkSessionManager*);
void NetworkSessionManager_deleteRequest(NetworkSessionManager*, NetworkRequest**);
}

/* ---- extra neighbouring globals ---- */
/* The four addresses themselves are declared in the band's data header (rule 2). */

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - construction / pool                                                */
/* ----------------------------------------------------------------------------------------- */

#pragma peephole off
NetworkSessionManager::NetworkSessionManager()
{
    s32 i;
    NetworkRequest* request;

    this->unused_04 = 0;
    this->unused_08 = 0;
    this->buffer = 0;
    for (i = 0; i < 21; i++) {
        this->requests_10[i] = 0;
        this->request_state_64[i] = 0;
    }
    this->unused_79 = 1;
    this->unused_7A = 1;
    for (i = 0, request = this->pool_7C; i < 2; i++, request++) {
        NetworkRequest_reset(request);
    }
}

#pragma peephole off
extern "C" void NetworkRequest_reset(NetworkRequest* self)
{
    self->state_00 = 0;
    self->interval_4C = networkRequestZero;
    self->timeout_50 = networkRequestZero;
    self->requestId_70 = 0;
    self->unused_24 = 0;
    self->cancelled_74 = 0;
    self->owner_94 = 0;
    self->handler_98 = 0;
    self->count_28 = 0;
    self->record_54 = 0;
    self->record_58 = 0;
    self->record_5C = 0;
    self->unused_60 = 0;
    self->unused_64 = 0;
    self->unused_68 = 0;
    self->unused_6C = 0;
    self->unused_04 = 0;
    self->unused_08 = 0;
    self->buffer = 0;
    self->unused_10 = 0;
    self->unused_14 = 0;
    self->unused_18 = 0;
    self->unused_1C = 0;
    self->unused_20 = 0;
    self->args_2C[0] = 0;
    self->args_2C[1] = 0;
    self->args_2C[2] = 0;
    self->args_2C[3] = 0;
    self->args_2C[4] = 0;
    self->args_2C[5] = 0;
    self->args_2C[6] = 0;
    self->args_2C[7] = 0;
}

/* Empties the pooled request; its mutex goes with the member. */
NetworkRequest::~NetworkRequest()
{
    NetworkRequest_clear(this);
}
#pragma peephole on

#pragma dont_inline on
extern "C" void NetworkRequest_clear(NetworkRequest* self)
{
    NetworkRequest_reset(self);
}
#pragma dont_inline off

/* Builds the pooled request: its mutex (the member), then an empty record. */
NetworkRequest::NetworkRequest()
{
    NetworkRequest_reset(this);
}

#pragma peephole off
NetworkSessionManager::~NetworkSessionManager()
{
    NetworkSessionManager::release();
}
#pragma peephole on


void NetworkSessionManager::init(u32 a, u32 b)
{
    this->unused_04 = a;
    this->unused_08 = b;
    NetworkSessionManager::clear();
}

void NetworkSessionManager::clear()
{
    s32 i;

    this->buffer = 0;
    for (i = 0; i < 21; i++) {
        this->requests_10[i] = 0;
        this->request_state_64[i] = 0;
    }
    for (i = 0; i < 2; i++) {
        NetworkRequest_reset(&this->pool_7C[i]);
    }
}

void NetworkSessionManager::release()
{
    NetworkBuffer* buf;
    s32 i;
    NetworkRequest** slot;
    NetworkRequest* request;

    buf = this->buffer;
    if (buf != 0) {
        buf->begin();
        buf = this->buffer;
        if (buf != 0) {
            if (buf != 0) {
                buf->destroy(1);
            }
            this->buffer = 0;
        }
    }
    for (i = 0, slot = this->requests_10; i < 0x15; i++, slot++) {
        NetworkSessionManager_deleteRequest(this, slot);
    }
    for (i = 0, request = this->pool_7C; i < 2; i++, request++) {
        NetworkRequest_clear(request);
    }
}

/* One tick of the request state machine: opens the slot's request, resolves the ones that may
   move this tick and retires the ones whose target has gone away. */
void NetworkSessionManager::move()
{
    NetworkRequest* req;
    NetworkLogger* log;
    NetworkBuffer* buf;
    s32 i;

    buf = this->buffer;
    if (buf != 0) {
        buf->end();
    }
    for (i = 0; i < 21; i++) {
        req = this->requests_10[i];
        if (req != 0 && this->request_state_64[i] == 0) {
            if (i != 2 && this->requests_10[2] != 0) {
                continue;
            }
            switch (i) {
            case 1:
                if (this->request_state_64[2] != 0) {
                    continue;
                }
                break;
            case 2:
                {
                    s32 j;
                    for (j = 0; j < 21; j++) {
                        if (this->request_state_64[j] != 0) {
                            break;
                        }
                    }
                    if (j == 21) {
                        break;
                    }
                    log = getNetworkLogger();
                    log->log_14("NetworkSessionManager::move: request[%d] is moving, stand by...\n", j);
                    continue;
                }
            case 3:
            case 6:
                if (this->request_state_64[11] != 0 || this->request_state_64[12] != 0 ||
                    this->request_state_64[2] != 0 || this->request_state_64[1] != 0) {
                    continue;
                }
                break;
            case 12:
                if (this->request_state_64[11] != 0) {
                    continue;
                }
                /* falls through */
            case 11:
                if (this->request_state_64[2] != 0 || this->request_state_64[1] != 0 ||
                    this->request_state_64[3] != 0 || this->request_state_64[6] != 0 ||
                    this->request_state_64[10] != 0) {
                    continue;
                }
                break;
            case 10:
                if (this->request_state_64[11] != 0) {
                    continue;
                }
                break;
            default:
                break;
            }
            this->request_state_64[i] = 1;
        }
        if (req != 0 && this->request_state_64[i] != 0) {
            req->run();
            if (NetworkRequest_isOwned(req) == 0) {
                NetworkSessionManager_deleteRequest(this, &this->requests_10[i]);
                this->request_state_64[i] = 0;
            }
        }
    }
}

#pragma dont_inline on
extern "C" s32 NetworkRequest_isOwned(NetworkRequest* self)
{
    return self->owner_94 != 0;
}
#pragma dont_inline off

void NetworkRequest::run()
{
    if (this->owner_94 != 0 && (this->owner_94->*this->handler_98)(this) != 0) {
        NetworkRequest_clear(this);
    }
}

/* The request descriptors `request364`..`request444` pass by value: `{0, op code, 0}`, in the order the
   target lays them out. */
NetworkRequestDesc networkRequestDesc364 = &NetworkSessionManager::updateSession;
NetworkRequestDesc networkRequestDesc368 = &NetworkSessionManager::shutdown;
NetworkRequestDesc networkRequestDesc372 = &NetworkSessionManager::handleCircleCreate;
NetworkRequestDesc networkRequestDesc376 = &NetworkSessionManager::slot_178;
NetworkRequestDesc networkRequestDesc380 = &NetworkSessionManager::handleCircleListLayer;
NetworkRequestDesc networkRequestDesc384 = &NetworkSessionManager::handleCircleJoin;
NetworkRequestDesc networkRequestDesc388 = &NetworkSessionManager::slot_184;
NetworkRequestDesc networkRequestDesc432 = &NetworkSessionManager::handleCircleMatchStart;
NetworkRequestDesc networkRequestDesc416 = &NetworkSessionManager::slot_1A0;
NetworkRequestDesc networkRequestDesc420 = &NetworkSessionManager::slot_1A4;
NetworkRequestDesc networkRequestDesc424 = &NetworkSessionManager::slot_1A8;
NetworkRequestDesc networkRequestDesc404 = &NetworkSessionManager::handleCircleInfoSet;
NetworkRequestDesc networkRequestDesc436 = &NetworkSessionManager::moveStartSession;
NetworkRequestDesc networkRequestDesc440 = &NetworkSessionManager::slot_1B8;
NetworkRequestDesc networkRequestDesc444 = &NetworkSessionManager::handleCircleMatchEnd;
NetworkRequestDesc networkRequestDesc408 = &NetworkSessionManager::handleCircleLeave;
NetworkRequestDesc networkRequestDesc412 = &NetworkSessionManager::slot_19C;
NetworkRequestDesc networkRequestDesc392 = &NetworkSessionManager::slot_188;
NetworkRequestDesc networkRequestDesc396 = &NetworkSessionManager::handleServerTimeout;
NetworkRequestDesc networkRequestDesc400 = &NetworkSessionManager::slot_190;
NetworkRequestDesc networkRequestDesc428 = &NetworkSessionManager::handleCircleMatchOptionSet;

/* The request-id source `NetworkRequest_begin` post-increments. */
u32 NetworkRequest_idCounter;

#pragma peephole off
void NetworkSessionManager::request364()
{
    NetworkRequest* req;

    if (this->requests_10[1] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[1] = req;
            NetworkRequest_begin(req, this, networkRequestDesc364, 0);
        }
    }
}
#pragma peephole on

extern "C" void NetworkRequest_begin(NetworkRequest* req, NetworkSessionManager* owner,
                            NetworkRequestDesc desc, u32 count, ...)
{
    NetworkVaState args;
    NetworkLogger* log;
    u32 i;

    NetworkRequest_reset(req);
    log = getNetworkLogger();
    req->timeout_50 = log->getTime_60();
    req->requestId_70 = NetworkRequest_idCounter;
    NetworkRequest_idCounter = req->requestId_70 + 1;
    req->owner_94 = owner;
    req->handler_98 = desc;
    req->count_28 = count > 8 ? 8 : count;
    net_va_start(args);
    for (i = 0; i < req->count_28; i++) {
        req->args_2C[i] = net_va_arg(args, u32);
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - the lazy request allocators                                        */
/* ----------------------------------------------------------------------------------------- */

#pragma peephole off
void NetworkSessionManager::request368()
{
    NetworkRequest* req;

    if (this->requests_10[2] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[2] = req;
            NetworkRequest_begin(req, this, networkRequestDesc368, 0);
        }
    }
}

s32 NetworkSessionManager::hasBuffer()
{
    return this->buffer != 0;
}

void NetworkSessionManager::request372(u32 a, u32 b)
{
    NetworkRequest* req;

    if (this->requests_10[3] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[3] = req;
            NetworkRequest_begin(req, this, networkRequestDesc372, 2, a, b);
        }
    }
}

void NetworkSessionManager::request376(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[4] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[4] = req;
            NetworkRequest_begin(req, this, networkRequestDesc376, 1, a);
        }
    }
}

void NetworkSessionManager::request380(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[5] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[5] = req;
            NetworkRequest_begin(req, this, networkRequestDesc380, 1, a);
        }
    }
}

void NetworkSessionManager::request384(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[6] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[6] = req;
            NetworkRequest_begin(req, this, networkRequestDesc384, 1, a);
        }
    }
}

void NetworkSessionManager::request388(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[6] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[6] = req;
            NetworkRequest_begin(req, this, networkRequestDesc388, 1, a);
        }
    }
}

void NetworkSessionManager::request432()
{
    NetworkRequest* req;

    if (this->requests_10[7] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[7] = req;
            NetworkRequest_begin(req, this, networkRequestDesc432, 0);
        }
    }
}

void NetworkSessionManager::request416(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[16] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[16] = req;
            NetworkRequest_begin(req, this, networkRequestDesc416, 1, a);
        }
    }
}

void NetworkSessionManager::request420(s8 a)
{
    NetworkRequest* req;

    if (this->requests_10[17] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[17] = req;
            NetworkRequest_begin(req, this, networkRequestDesc420, 1, (u32)a);
        }
    }
}

void NetworkSessionManager::request424()
{
    NetworkRequest* req;

    if (this->requests_10[20] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[20] = req;
            NetworkRequest_begin(req, this, networkRequestDesc424, 0);
        }
    }
}

void NetworkSessionManager::request404(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[8] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[8] = req;
            NetworkRequest_begin(req, this, networkRequestDesc404, 1, a);
        }
    }
}

void NetworkSessionManager::request436()
{
    NetworkRequest* req;

    if (this->requests_10[9] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[9] = req;
            NetworkRequest_begin(req, this, networkRequestDesc436, 0);
        }
    }
}

void NetworkSessionManager::request440()
{
    NetworkRequest* req;

    if (this->requests_10[19] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[19] = req;
            NetworkRequest_begin(req, this, networkRequestDesc440, 0);
        }
    }
}

void NetworkSessionManager::request444()
{
    NetworkRequest* req;

    if (this->requests_10[10] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[10] = req;
            NetworkRequest_begin(req, this, networkRequestDesc444, 0);
        }
    }
}

void NetworkSessionManager::request408()
{
    NetworkRequest* req;

    if (this->requests_10[11] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[11] = req;
            NetworkRequest_begin(req, this, networkRequestDesc408, 0);
        }
    }
}

void NetworkSessionManager::request412(u32 a, u32 b, s8 c)
{
    NetworkRequest* req;

    if (this->requests_10[15] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[15] = req;
            NetworkRequest_begin(req, this, networkRequestDesc412, 3, a, b, (u32)c);
        }
    }
}

void NetworkSessionManager::request392()
{
    NetworkRequest* req;

    if (this->requests_10[12] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[12] = req;
            NetworkRequest_begin(req, this, networkRequestDesc392, 0);
        }
    }
}

void NetworkSessionManager::request396(u32 a, u32 b)
{
    NetworkRequest* req;

    if (this->requests_10[13] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[13] = req;
            NetworkRequest_begin(req, this, networkRequestDesc396, 2, a, b);
        }
    }
}

void NetworkSessionManager::request400(u32 a, u32 b)
{
    NetworkRequest* req;

    if (this->requests_10[14] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[14] = req;
            NetworkRequest_begin(req, this, networkRequestDesc400, 2, a, b);
        }
    }
}

void NetworkSessionManager::request428(u32 a)
{
    NetworkRequest* req;

    if (this->requests_10[18] == 0) {
        req = NetworkSessionManager_allocRequest(this);
        if (req != 0) {
            this->requests_10[18] = req;
            NetworkRequest_begin(req, this, networkRequestDesc428, 1, a);
        }
    }
}
#pragma peephole on

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - accessors / small virtuals                                         */
/* ----------------------------------------------------------------------------------------- */

void NetworkSessionManager::abortRequest4()
{
    if (this->requests_10[4] != 0 && NetworkRequest_isCancelled(this->requests_10[4]) == 0) {
        NetworkRequest_cancel(this->requests_10[4]);
    }
}

#pragma dont_inline on
extern "C" void NetworkRequest_cancel(NetworkRequest* self)
{
    self->cancelled_74 = 1;
}

extern "C" s32 NetworkRequest_isCancelled(NetworkRequest* self)
{
    return self->cancelled_74;
}
#pragma dont_inline off

void NetworkSessionManager::abortRequest14()
{
    if (this->requests_10[14] != 0 && NetworkRequest_isCancelled(this->requests_10[14]) == 0) {
        NetworkRequest_cancel(this->requests_10[14]);
    }
}

void NetworkSessionManager::setFlag79(s8 value)
{
    this->unused_79 = value;
}

void NetworkSessionManager::notify(s32 value)
{
    NetworkSessionStable_setNotifyValue(value);
}

void NetworkSessionManager::setFlag7A(s8 value)
{
    this->unused_7A = value;
}

s32 NetworkSessionManager::getInt(s8 value)
{
    if (this->buffer == 0) {
        return 0;
    }
    s8 slot = mapId_1C0(value);

    return this->buffer->getInt(slot);
}

f32 NetworkSessionManager::getFloat(s8 value)
{
    if (this->buffer == 0) {
        return networkRequestZero;
    }
    s8 slot = mapId_1C0(value);

    return this->buffer->getFloat(slot);
}

void NetworkSessionManager::broadcastPlayerSlots(u32 a, u32 b)
{
    s8 data[4];

    data[0] = 0;
    data[1] = 1;
    data[2] = 2;
    data[3] = 3;
    sendBatch_138(a, b, 4, data);
}

#pragma peephole off
void NetworkSessionManager::putTerminatorA(u32 a, u32 b, u8 c)
{
    s8 data;
    NetworkBuffer* buf;

    data = -1;
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 1, 1, &data, c);
    }
}

void NetworkSessionManager::putTerminatorB(u32 a, u32 b)
{
    s8 data;
    NetworkBuffer* buf;

    data = -2;
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 0, 1, &data, 0xFF);
    }
}

void NetworkSessionManager::putTerminatorC(u32 a, u32 b, u8 c)
{
    s8 data;
    NetworkBuffer* buf;

    data = -2;
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 1, 1, &data, c);
    }
}
#pragma peephole on

/* Maps each requested player slot and puts the batch on the current stream buffer. */
void NetworkSessionManager::sendBatch_138(u32 a, u32 b, s32 count, const s8* data)
{
    s8 mapped[8];
    s32 n;
    s32 i;

    n = 0;
    for (i = 0; i < count; i++) {
        s8 value = mapId_1C0(data[i]);
        if (value >= 0) {
            mapped[n++] = value;
        }
    }
    if (this->buffer != 0) {
        this->buffer->put(a, b, 0, n, mapped, 0xFF);
    }
}

/* The same batch write, addressed to an explicit packet header and with the caller's flags. */
void NetworkSessionManager::slot_13C(u32 a, u32 b, s32 count, const s8* data, u8 flags)
{
    s8 mapped[8];
    s32 n;
    s32 i;

    n = 0;
    for (i = 0; i < count; i++) {
        s8 value = mapId_1C0(data[i]);
        if (value >= 0) {
            mapped[n++] = value;
        }
    }
    if (this->buffer != 0) {
        this->buffer->put(a, b, 1, n, mapped, flags);
    }
}

/* Maps one player slot and puts the single byte on the current stream buffer. */
void NetworkSessionManager::slot_140(u32 a, u32 b, s8 idx)
{
    s8 data;
    NetworkBuffer* buf;

    data = mapId_1C0(idx);
    if (data < 0) {
        return;
    }
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 0, 1, &data, 0xFF);
    }
}

/* The same single-slot write with the caller's flags. */
void NetworkSessionManager::slot_144(u32 a, u32 b, s8 idx, u8 flags)
{
    s8 data;
    NetworkBuffer* buf;

    data = mapId_1C0(idx);
    if (data < 0) {
        return;
    }
    buf = this->buffer;
    if (buf != 0) {
        buf->put(a, b, 1, 1, &data, flags);
    }
}

void NetworkSessionManager::flush()
{
    if (this->buffer != 0 && canSend_28() != 0) {
        this->buffer->flush();
    }
}

void NetworkSessionManager::slot_14C()
{
    if (this->buffer != 0) {
        this->buffer->slot_30();
    }
}

void NetworkSessionManager::slot_150()
{
    if (this->buffer != 0) {
        this->buffer->slot_34();
    }
}

void NetworkSessionManager::slot_154()
{
    if (this->buffer != 0) {
        this->buffer->slot_50();
    }
}

void NetworkSessionManager::slot_158()
{
    if (this->buffer != 0) {
        this->buffer->slot_54();
    }
}

void NetworkSessionManager::slot_15C()
{
    if (this->buffer != 0) {
        this->buffer->slot_60();
    }
}

void NetworkSessionManager::slot_160()
{
    if (this->buffer != 0) {
        this->buffer->slot_64();
    }
}

void NetworkSessionManager::slot_164()
{
    if (this->buffer != 0) {
        this->buffer->slot_68();
    }
}

void NetworkSessionManager::slot_168()
{
    if (this->buffer != 0) {
        this->buffer->slot_5C();
    }
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManager - pool + mutex helpers                                               */
/* ----------------------------------------------------------------------------------------- */

extern "C" NetworkRequest* NetworkSessionManager_allocRequest(NetworkSessionManager* self)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (NetworkRequest_isOwned(&self->pool_7C[i]) == 0) {
            return &self->pool_7C[i];
        }
    }
    return 0;
}

extern "C" void NetworkSessionManager_deleteRequest(NetworkSessionManager* self, NetworkRequest** slot)
{
    if (*slot != 0) {
        if (NetworkRequest_isOwned(*slot) != 0) {
            NetworkLogger* log = getNetworkLogger();
            log->log_14("NetworkSessionManager::deleteRequest: request is moving.\n");
        }
        NetworkRequest_clear(*slot);
    }
    *slot = 0;
}

/* Copies the request's error record out under its mutex; false while none is set. */
s32 NetworkRequest::getRecord(NetworkErrorInfo* out)
{
    s32 result;

    result = 0;
    LockMutex(&this->mutex_78);
    if (this->record_54 != 0) {
        result = 1;
        out->code_00 = this->record_54;
        out->param1_04 = this->record_58;
        out->param2_08 = this->record_5C;
    }
    UnlockMutex(&this->mutex_78);
    return result;
}

/* Stores the request's error record under its mutex. */
void NetworkRequest::setRecord(u32 a, u32 b, u32 c)
{
    LockMutex(&this->mutex_78);
    this->record_58 = b;
    this->record_5C = c;
    this->record_54 = a;
    UnlockMutex(&this->mutex_78);
}

/* The starter's word argument `idx`, 0 (and a warning) past the count it was given. */
s32 NetworkRequest::getArgument(u32 idx)
{
    u32 count;
    NetworkLogger* log;

    count = this->count_28;
    if (count <= idx) {
        log = getNetworkLogger();
        log->warn_10("NetworkRequest::getArgument: arg no over %d <= %d\n", count, idx);
        return 0;
    }
    return (s32)this->args_2C[idx];
}

/* ----------------------------------------------------------------------------------------- */
/* NetworkRequest timers                                                                     */
/* ----------------------------------------------------------------------------------------- */

/* True once the request has been waiting longer than its own interval. */
s32 NetworkRequest::isTimedOut()
{
    NetworkLogger* log;
    s32 result;

    result = 0;
    LockMutex(&this->mutex_78);
    if (networkRequestTimerIdle != this->interval_4C) {
        log = getNetworkLogger();
        if (log->getTime_60() - this->timeout_50 > this->interval_4C) {
            result = 1;
        }
    }
    UnlockMutex(&this->mutex_78);
    return result;
}

/* Restarts the wait: the current clock becomes the baseline and the interval is replaced. */
void NetworkRequest::restartTimer(f32 interval)
{
    NetworkLogger* log;

    LockMutex(&this->mutex_78);
    log = getNetworkLogger();
    this->timeout_50 = log->getTime_60();
    this->interval_4C = interval;
    UnlockMutex(&this->mutex_78);
}

/* Moves the 0x60-byte record block an out-of-band request carries. */
extern "C" void NetworkRequest_copyRecord(NetworkSessionRecordBlock* dst, const NetworkSessionRecordBlock* src)
{
    *dst = *src;
}

/* ----------------------------------------------------------------------------------------- */
/* The small per-player records the Pat layer owns                                            */
/* ----------------------------------------------------------------------------------------- */

/* The record classes below each hold one `NetworkUniqueId`; their constructors and destructors are the pairs the
   containers' array construction passes to `__construct_array`.  Retail keeps the unfused `extsh`+`cmpwi` of the
   deleting flag, so the peephole pass is off for the records. */
#pragma peephole off

/* Builds the chat record around its sender's address. */
NetworkSessionSlotInfo::NetworkSessionSlotInfo()
{
}

/* Destroys the chat record's sender address. */
NetworkSessionSlotInfo::~NetworkSessionSlotInfo()
{
}

/* The reflection callbacks: both re-order the six incoming arguments and tail-call the real body. */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
extern "C" void networkSessionReflectCallback(void* a0, void* a1, s8 a2, void* a3, void* a4, void* a5)
{
    networkSessionReflect0(a5, a0, a2, a3, a4, a1);
}

/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
extern "C" void networkSessionReflectCallbackEx(void* a0, void* a1, void* a2, void* a3, void* a4, void* a5)
{
    networkSessionReflect1((NetworkSessionManagerPat*)a5, (s32)a0, (s32)a1, (s32)a2, (s32)a3, (const u8*)a4);
}

/* Destroys the 32 circle entries. */
NetworkSessionCircleList::~NetworkSessionCircleList()
{
}

/* Builds a circle entry around its address. */
NetworkSessionCircleInfo::NetworkSessionCircleInfo()
{
}

/* Destroys a circle entry's address. */
NetworkSessionCircleInfo::~NetworkSessionCircleInfo()
{
}

GameSpyInterfaceThread* GameSpyInterfaceThread::getInstance()
{
    return sGameSpyInterfaceThread;
}

/* Builds the 32 circle entries. */
NetworkSessionCircleList::NetworkSessionCircleList()
{
}

/* Destroys a player record's address. */
NetworkSessionPlayerRecord::~NetworkSessionPlayerRecord()
{
}

/* Builds a player record around its address. */
NetworkSessionPlayerRecord::NetworkSessionPlayerRecord()
{
}

/* ----------------------------------------------------------------------------------------- */
/* The Pat layer's own NetworkRequest array                                                   */
/* ----------------------------------------------------------------------------------------- */

/* Empties the request record and destroys its mutex. */
NetworkRequestPat::~NetworkRequestPat()
{
    clear();
}

/* Empties the record. */
void NetworkRequestPat::clear()
{
    reset();
}

/* Clears every field of the record. */
void NetworkRequestPat::reset()
{
    NetworkRequestPat* self = this;

    self->state_00 = 0;
    self->interval_4C = networkRequestTimerReset;
    self->timeout_50 = networkRequestTimerReset;
    self->requestId_70 = 0;
    self->unused_24 = 0;
    self->cancelled_74 = 0;
    self->owner_94 = 0;
    self->handler_98 = 0;
    self->count_28 = 0;
    self->record_54 = 0;
    self->record_58 = 0;
    self->record_5C = 0;
    self->unused_60 = 0;
    self->unused_64 = 0;
    self->unused_68 = 0;
    self->unused_6C = 0;
    self->unused_04 = 0;
    self->unused_08 = 0;
    self->buffer = 0;
    self->unused_10 = 0;
    self->unused_14 = 0;
    self->unused_18 = 0;
    self->unused_1C = 0;
    self->unused_20 = 0;
    self->args_2C[0] = 0;
    self->args_2C[1] = 0;
    self->args_2C[2] = 0;
    self->args_2C[3] = 0;
    self->args_2C[4] = 0;
    self->args_2C[5] = 0;
    self->args_2C[6] = 0;
    self->args_2C[7] = 0;
}

/* Builds the request record: its mutex, then an empty record. */
NetworkRequestPat::NetworkRequestPat()
{
    reset();
}
#pragma peephole on

/* ----------------------------------------------------------------------------------------- */
/* NetworkSessionManagerPat                                                                   */
/* ----------------------------------------------------------------------------------------- */

/* Builds the Pat side of the session: its own request pair, its player records and the circle
   list, then the two on-demand singletons the Pat layer drives. */
#pragma dont_inline on
#pragma peephole off
NetworkSessionManagerPat::NetworkSessionManagerPat()
{
    if (getInstance_() == 0) {
        new PatInterface();
    }
    if (GameSpyInterfaceThread::getInstance() == 0) {
        new GameSpyInterfaceThread();
    }
    this->field_6E75 = 0;
    this->clear();
}

#pragma dont_inline on
/* Releases the Pat session; MWCC destroys the members and the base after the body. */
NetworkSessionManagerPat::~NetworkSessionManagerPat()
{
    this->release();
}

#pragma dont_inline on
/* Re-arms the Pat session for a new match: its player records, the -1 slot table, the circle list
   and the stream buffer's timing fields are all reset. */
void NetworkSessionManagerPat::clear()
{
    s32 i;

    this->NetworkSessionManager::clear();
    for (i = 0; i < 21; i++) {
        this->requestIds_360[i] = -1;
    }
    this->field_3BC = this->field_3B8 = networkSessionPatTimeOrigin;
    this->connected_3C0 = 0;
    this->matchRunning_3C1 = 0;
    this->field_3C2 = 0;
    this->field_3C3 = 0;
    this->field_3C4 = 0;
    this->field_3C8 = 0;
    this->field_3CC.clear();
    memset(&this->matchOptions_3EC, 0, sizeof(this->matchOptions_3EC));
    this->tcp_658 = 0;
    this->udp_65C = 0;
    this->resolver_660 = 0;
    networkPatAttachBuffer(this);
    this->circleList_AF0.count_00 = 0;
    for (i = 0; i < 32; i++) {
        networkPatResetCircleInfo(this, i);
    }
    this->field_6E74 = 0;
}
#pragma peephole reset

/* Starts a Pat session: the interface singletons, the base's own init and then a clear. */
#pragma dont_inline on
#pragma peephole off
void NetworkSessionManagerPat::init(u32 a, u32 b)
{
    if (getInstance_() == 0) {
        new PatInterface();
    }
    this->NetworkSessionManager::init(a, b);
    if (GameSpyInterfaceThread::getInstance() == 0) {
        new GameSpyInterfaceThread();
    }
    this->field_6E75 = 0;
    this->clear();
}

#pragma peephole reset
/* Tears the Pat session down: drains the game-spy thread, then runs the base class's teardown. */
void NetworkSessionManagerPat::release()
{
    GameSpyInterfaceThread* thread;
    PatInterface* pat;
    void* context;

    networkPatReleaseBuffer(this);
    if (getInstance_() != 0) {
        PatInterface_clear((PatInterface*)getInstance_());
        if (PatInterface_isReady((PatInterface*)getInstance_()) == 0) {
            pat = (PatInterface*)getInstance_();
            delete pat;
        }
    }
    context = this->resolver_660;
    if (context != 0) {
        networkLog_destroyContext((NetworkSessionManagerLogger*)getNetworkLogger(), context);
        this->resolver_660 = 0;
    }
    if (GameSpyInterfaceThread::getInstance() != 0) {
        if (this->field_6E75 != 0) {
            NetworkLogger* log = getNetworkLogger();
            log->warn_10("NetworkSessionManagerPat::final: finalNetwork have not done.\n");
            this->field_6E75 = 0;
            thread = GameSpyInterfaceThread::getInstance();
            thread->canClose();
            thread = GameSpyInterfaceThread::getInstance();
            thread->armCancel();
        }
        do {
        } while (GameSpyInterfaceThread::getInstance()->requestClose());
        thread = GameSpyInterfaceThread::getInstance();
        delete thread;
    }
    this->NetworkSessionManager::release();
}
#pragma dont_inline off
