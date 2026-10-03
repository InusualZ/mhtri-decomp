/*
 * Network/NetworkLayerPatStep.cpp - `NetworkLayerPat`'s request state machine (`.text`
 * 0x803E44C8..0x803E4888, 1 function / 960 B).
 *
 * NAMING (GUESS).  The range carries no `__FILE__` string and the dump answers `zz_03e44c8_`.  What
 * it is: the vtable slot +0x104 of `lbl_805FC1E0` (0x805FC1E0), the table `fn_803E0C18` stores into the
 * `NetworkLayerPat` it builds (the class name is the pool strings' own: "NetworkLayerPat::move ...").
 * The body advances the `NetworkRequest` state at +0x00 (0 -> 5 -> 10 -> 15 -> 20 -> 25 -> 30, or 100
 * / 110 on failure) and issues the layer requests `sendReqLayerUp`, `sendReqLayerChildInfo` and
 * `sendReqLayerUserList`.  Method name `stepRequest` and the file name are guesses from that (evidence
 * class 3, module `Network/` beside the other layer units); the callees' names are marked GUESS where
 * they are declared (`include/unsplit/Network.h`).
 *
 * SECTIONS.  `.text` plus the function's own `extab` (0x8001ACC0) and `extabindex` (0x8003B010).  The
 * table `lbl_805FC1E0` is not claimed: it belongs to the class's key function, elsewhere in the band.
 *
 * Levers: file-scope `#pragma peephole off` (playbook 39: retail keeps `clrlwi`+`cmpwi` unfused) and
 * the `sendReqLayer*` results declared `u32` (the callee returns a 16-bit value, but the caller stores
 * the whole register - playbook 66).  The field at +0xF19C is reached with `addis`+`lwz`, which is what a real offset that
 * size compiles to.
 *
 * Flags: the lib's `-O4,p` is replaced by `-O3` like the sibling session units (see configure.py).
 */
#include "Network/NetworkLayerPat.h"
#include "Network/NetworkPat.h"
#include "Network/network_pat_control.h"
#include "enemy/em020_ai.h"
#include "unsplit/Network.h"
#include "Network/NetworkSessionManagerPat.h"
#include "Network/NetworkCommunityPat.h"

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
bool NetworkLayerPat::stepRequest(NetworkRequest* request)
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
        NetworkRequest_getError(request, &error);
        notifyLayerEvent(this, 4, error.code_00, 1, &error, context_08);
        pollLayerSlots(this);
        notifyLayerSlotSummary(this);
        return true;

    case STEP_FAILED:
        busy_3D1 = 0;
        NetworkRequest_getError(request, &error);
        notifyLayerEvent(this, 4, error.code_00, 1, &error, context_08);
        notifyLayerEvent(this, 3, error.code_00, 1, &error, context_08);
        return true;
    }
    return false;
}
#pragma peephole on
