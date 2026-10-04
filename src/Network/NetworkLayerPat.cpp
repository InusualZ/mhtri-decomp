/*
 * Network/NetworkLayerPat.cpp - the `NetworkLayerPat` layer (`.text` 0x803E0BE8..0x803EF668).
 *
 * RECUT (network pilot round 3).  The unit is the class's whole band: from 0x803E0BE8 (after `NetworkLayer`'s last
 *   function, `setFlag76`) through the constructor 0x803E0C18, the request handlers, the member helpers and the three
 *   `setCollectionLog*` siblings, to 0x803EF668 where `NetworkCommunity`'s constructor opens the next unit.  The left part
 *   (0x803E0BE8..0x803E44C8) was the tail of `Network/NetworkSessionManagerPat.cpp`, the middle the former one-function unit
 *   `Network/NetworkLayerPatStep.cpp` (folded in, renamed to this file), the rest the head of the phase 4 stub
 *   `Network/NetworkCommunityPat.cpp`.  The right edge copies `Network/NetworkSessionManagerPat.cpp`'s shape: the manager
 *   ends with its `setSessionLog*` trio and the layer base class follows; this band ends with `setCollectionLog*` and the
 *   community base class follows (its constructor stores the table 0x805FC440).  `.data` 0x805FB718..0x805FC390 (the class's
 *   strings, its table `lbl_805FC1E0`, and the two inline `NetworkRequest::getArgument` strings 0x805FC324/0x805FC358 the
 *   handlers at 0x803E3C70/0x803E9C00 read - not a seam), `.sdata` 0x80793940, `.sbss` 0x80794CA8, `.sdata2`
 *   0x8079C780..0x8079C7A0 (every word read only from this range), extab 0x8001A810..0x8001B0A0, extabindex
 *   0x8003AE0C..0x8003B448.  Its bodies are the two below; the rest is unwritten (`ledger.py unit Network/NetworkLayerPat.cpp`).
 *
 * NAMING (GUESS) of the state machine (0x803E44C8, 960 B).  The range carries no `__FILE__` string and the dump answers
 * `zz_03e44c8_`.  It is the vtable slot +0x104 of `lbl_805FC1E0` (0x805FC1E0), the table `__ct__15NetworkLayerPatFv`
 * stores into the `NetworkLayerPat` it builds (the class name is the pool strings' own: "NetworkLayerPat::move ...").
 * The body advances the `NetworkRequest` state at +0x00 (0 -> 5 -> 10 -> 15 -> 20 -> 25 -> 30, or 100 / 110 on failure)
 * and issues the layer requests `sendReqLayerUp`, `sendReqLayerChildInfo` and `sendReqLayerUserList`; `stepRequest` is a
 * guess from that; the callees' names are marked GUESS where they are declared.
 *
 * Levers: file-scope `#pragma peephole off` (playbook 39: retail keeps `clrlwi`+`cmpwi` unfused; `getRecord` was written
 * under the old file's peephole-off region too) and the `sendReqLayer*` results declared `u32` (the callee returns a
 * 16-bit value, but the caller stores the whole register - playbook 66).  The field at +0xF19C is reached with
 * `addis`+`lwz`, which is what a real offset that size compiles to.
 *
 * Flags: the lib's `-O4,p` is replaced by `-O3` like the sibling session units (configure.py carries the per-row
 * measurement, `getRecord` included).
 */
#include "Network/NetworkLayerPat.h"
#include "Network/NetworkPat.h"
#include "Network/network_pat_control.h"
#include "enemy/em020_ai.h"
#include "unsplit/Network.h"
#include "Network/NetworkSessionManagerPat.h"
#include "Network/NetworkSessionBase.h"   /* LockMutex/UnlockMutex - owner Network/NetworkSessionBase.cpp */
#include "Network/network_layer_io.h"   /* sendReqLayerUp / sendReqLayerChildInfo / sendReqLayerUserList */
#include "Network/gamespy_interface_types.h"  /* GameSpyInterfaceThread / NetworkErrorInfo - owner Network/GameSpyInterfaceThread.cpp */

/* The log codes the request reports (0x8006xxxx = the layer's own error range). */
enum {
    LAYER_ERR_NOT_CONNECTED = 0x80060001,
    LAYER_ERR_CANNOT_START = 0x80060011,
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

/* `requestFlags_324` reply bits. */
enum {
    FLAG_SESSION_LOST = 0x1,
    FLAG_CANCELLED = 0x2,
    FLAG_UP_REPLY = 0x200,
    FLAG_CHILD_INFO_REPLY = 0x100000,
    FLAG_USER_LIST_REPLY = 0x400
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

bool NetworkLayerPat::stepRequest(NetworkLayerRequest* request)
{
    NetworkRequestError error;

    switch (request->state_00) {
    case STEP_START:
        if (connected_3D0 == 0) {
            setCollectionLog(this, request, LAYER_ERR_NOT_CONNECTED, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (memberCount_4C8 <= 0) {
            setCollectionLog(this, request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (pendingRequestId_F19C >= 0) {
            setCollectionLog(this, request, LAYER_ERR_CANNOT_START, 0, 0);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        requestFlags_324 = 0;
        requestResult_37C = sendReqLayerUp(getInstance_());
        request->state_00 = STEP_WAIT_UP;
        break;

    case STEP_WAIT_UP:
        if (requestFlags_324 & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(this, request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_324 & FLAG_CANCELLED) {
            setCollectionLogAborted(this, request);
            request->state_00 = STEP_CANCELLED;
            break;
        }
        if (requestFlags_324 & FLAG_UP_REPLY) {
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
        requestFlags_324 = 0;
        requestResult_37C = sendReqLayerChildInfo(getInstance_(), -1, 0);
        request->state_00 = STEP_WAIT_CHILD_INFO;
        break;

    case STEP_WAIT_CHILD_INFO:
        if (requestFlags_324 & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(this, request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_324 & FLAG_CANCELLED) {
            setCollectionLogAborted(this, request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_324 & FLAG_CHILD_INFO_REPLY) {
            if (hostMode_4C4 != 0) {
                request->state_00 = STEP_USER_LIST;
            } else {
                request->state_00 = STEP_DONE;
            }
        }
        break;

    case STEP_USER_LIST:
        requestFlags_324 = 0;
        requestResult_37C = sendReqLayerUserList(getInstance_());
        request->state_00 = STEP_WAIT_USER_LIST;
        break;

    case STEP_WAIT_USER_LIST:
        if (requestFlags_324 & FLAG_SESSION_LOST) {
            setCollectionLogSessionLost(this, request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_324 & FLAG_CANCELLED) {
            setCollectionLogAborted(this, request);
            request->state_00 = STEP_FAILED;
            break;
        }
        if (requestFlags_324 & FLAG_USER_LIST_REPLY) {
            request->state_00 = STEP_DONE;
        }
        break;

    case STEP_DONE:
        busy_3D1 = 0;
        notifyLayerEvent(this, 4, 0, 0, NULL, context_08);
        pollLayerSlots(this);
        notifyLayerSlotSummary(this);
        return true;

    case STEP_CANCELLED:
        busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(this, 4, error.code_00, 1, &error, context_08);
        pollLayerSlots(this);
        notifyLayerSlotSummary(this);
        return true;

    case STEP_FAILED:
        busy_3D1 = 0;
        request->getRecord(&error);
        notifyLayerEvent(this, 4, error.code_00, 1, &error, context_08);
        notifyLayerEvent(this, 3, error.code_00, 1, &error, context_08);
        return true;
    }
    return false;
}
#pragma peephole on
