/*
 * Network/NetworkReflectService.cpp - the head of the `NetworkReflectService` class (the GameSpy reflect
 * service the mediator starts, stops and agrees): its constructor, the init/finalize/start/stop/page/agree
 * entry points, the per-frame step and the start sequence.
 *
 * `.text` 0x8041A194..0x8041A87C.  Sections: extab 0x8001CF88..0x8001CFF0; extabindex 0x8003D950..0x8003D9BC;
 * .data 0x80603118..0x80603154 (the start sequence's one log string); .sbss 0x80794CD8..0x80794CE0
 * (`sNetworkReflectService`).
 *
 * WHAT IT IS.  Names from the map (`__ct__21NetworkReflectServiceFv`, `notify`, the `reflectService*` family);
 * the helpers this pass named (`resetReflectService`, `resetReflectServiceTask`, `updateReflectService`,
 * `stepReflectServiceStart`, `reflectServiceEventCallback`, `sNetworkReflectService`) are GUESSES from the
 * bodies.  The log string says `ReflectInterface::start()`, so the retail class name may differ.
 *
 * BOUNDARY (residual, reported).  This range and `Network/GameSpyInterfaceThread.cpp` are one TU: the class's remaining
 * methods (`updateCallbackStep`, `dispatchTask`, `runSearch`, `runConnect`, `applyEvent`) open the neighbour,
 * and the class's table 0x80603190 follows this unit's strings in the unclaimed gap before the neighbour's
 * `.data` (0x806031A0) - the layout one object gives when the destructor here is the key function.
 *
 * UNWRITTEN.  The deleting destructor 0x8041A238 (156 B): defining it makes MWCC emit the class table into
 * this object, while the table (0x80603190) and the stop string before it (0x80603154) are unclaimed and read
 * from the neighbour's range (ours-extra `.data`); it waits for the seam request net2-l4-cff5#1 (.text to
 * 0x8041B194, .data to 0x806031A0, the five members moved here; splitcheck: both units ok, NRS data-order and
 * vtable unknown -> ok).
 *
 * FLAGS.  `-O3`/`-inline noauto` in place of the lib's `-O4,p`/`-inline auto` (configure.py, with the
 * numbers), and `#pragma peephole off` like the rest of the library.  `setReflectServicePage` takes the page
 * as `u8` (retail stores it with no `clrlwi`).
 */
#include "types.h"
#include "Network/NetworkReflectService.h"
#include "Network/PatInterface.h"
#include "Network/network_state.h"
#include "Network/network_opening.h"    /* sNetworkLibrary, sNetworkLibraryError */
#include "unsplit/Network.h"            /* getNetworkLogger */
#include "enemy/em020_ai.h"             /* getInstance_ */
#include "Runtime.PPCEABI.H/memset.h"
#include "sys_mem.h"

#pragma peephole off

/* The live service. */
NetworkReflectService* sNetworkReflectService;

void reflectServiceEventCallback(u32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg,
                                 NetworkReflectService* service)
{
    service->applyEvent(code, a, b, c, msg);
}

/* Publishes the service, creates the Pat interface singleton when there is none and clears the state. */
NetworkReflectService::NetworkReflectService()
{
    sNetworkReflectService = this;
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    resetReflectService(this);
}

/* Creates the Pat interface when needed, clears the state and installs the reflect callback pair. */
/* untyped: the callback's user payload, caller-owned */
void initReflectService(NetworkReflectService* service, NetworkWiiMediatorReflectFn callback, void* arg)
{
    if (getInstance_() == NULL) {
        new PatInterface();
    }
    resetReflectService(service);
    service->callback_04 = callback;
    service->callbackArg_08 = arg;
}

void resetReflectService(NetworkReflectService* service)
{
    service->callback_04 = NULL;
    service->callbackArg_08 = NULL;
    service->connectStep_15 = 0;
    resetReflectServiceTask(service);
}

void resetReflectServiceTask(NetworkReflectService* service)
{
    service->flags_0C = 0;
    service->task_10 = 0;
    service->searchStep_14 = 0;
    service->startStep_16 = 0;
    service->callbackStep_17 = 0;
}

/* Clears the Pat interface and deletes it once nothing holds it any more. */
void finalizeReflectService(NetworkReflectService* service)
{
    if (getInstance_() != NULL) {
        PatInterface_clear((PatInterface*)getInstance_());
        if (!PatInterface_isReady((PatInterface*)getInstance_())) {
            PatInterface* pat = (PatInterface*)getInstance_();

            if (pat != NULL) {
                pat->finalize(1);
            }
        }
    }
}

void updateReflectService(NetworkReflectService* service)
{
    if (service->callbackStep_17 != 0) {
        service->updateCallbackStep();
    } else if (service->startStep_16 != 0) {
        stepReflectServiceStart(service);
    } else if (service->task_10 > 0 && service->dispatchTask() != 0) {
        service->searchStep_14 = 0;
        service->task_10 = 0;
    }
}

void reflectServiceStart(NetworkReflectService* service)
{
    resetReflectServiceTask(service);
    service->startStep_16 = 1;
}

void reflectServiceStop(NetworkReflectService* service)
{
    service->callbackStep_17 = 1;
    service->startStep_16 = 0;
}

/* Starts a search for `page`, or reports busy (0x80000006) while a task runs. */
void setReflectServicePage(NetworkReflectService* service, u8 page)
{
    sNetworkLibraryError error;

    if (service->task_10 > 0) {
        error.facility = 0x80000006;
        error.step = 0;
        error.code = 0;
        service->notify(0x6004, 0, error.facility, 1, &error);
        return;
    }
    memset(&service->channel_18, 0, 0x814C);
    service->channel_18 = page;
    service->limit_8164 = 0;
    service->searchStep_14 = 0;
    service->task_10 = 1;
}

/* Starts the connect task, or reports busy (0x80000006) while a task runs. */
void reflectServiceAgree(NetworkReflectService* service)
{
    sNetworkLibraryError error;

    if (service->task_10 > 0) {
        error.facility = 0x80000006;
        error.step = 0;
        error.code = 0;
        service->notify(0x6005, 0, error.facility, 1, &error);
        return;
    }
    service->searchStep_14 = 0;
    service->task_10 = 2;
}

/* untyped: caller-owned payload - each work code carries its own record */
void NetworkReflectService::notify(u32 code, s32 a, s32 b, s32 c, void* data)
{
    callback_04(code, a, b, c, data, callbackArg_08);
}

/* One step of the start sequence: start the library, open the Pat interface and pick the server, then
 * wait for the session state machine; every outcome is reported through `notify` (0x6002). */
void stepReflectServiceStart(NetworkReflectService* service)
{
    sNetworkLibraryError error;
    s32 result;

    if (service->startStep_16 != 0 && getInstance_() == NULL) {
        ((sNetworkLibrary*)getNetworkLogger())->logError("Maybe, ReflectInterface::start() is called before init()\n");
        service->startStep_16 = 0;
    }
    switch (service->startStep_16) {
    case 1:
        if (service->connectStep_15 != 0) {
            error.facility = 0x80000007;
            error.step = 106;
            error.code = 0;
            service->notify(0x6002, 0, error.facility, 1, &error);
            break;
        }
        result = ((sNetworkLibrary*)getNetworkLogger())->start(0, &error);
        if (result < 0) {
            service->startStep_16 = 0;
            service->notify(0x6002, 0, error.facility, 1, &error);
        } else if (result > 0) {
            service->connectStep_15 = 1;
            service->startStep_16++;
        }
        break;
    case 2:
        increment60d4(getInstance_());
        setCallback(getInstance_(), (void (*)())reflectServiceEventCallback, service, 7);
        chooseServerAddress((NetworkStateMachine*)getInstance_(), 3, 0);
        setConnectServerType(getInstance_(), 3);
        resetNetworkState(getInstance_());
        service->startStep_16++;
        break;
    case 3:
        if (service->flags_0C & 1) {
            service->startStep_16 = 0;
            getErrorInfo654c(getInstance_(), (u32*)&error);
            service->notify(0x6002, 0, error.facility, 1, &error);
        } else if (service->flags_0C & 2) {
            service->startStep_16 = 0;
            error.facility = 0x80000007;
            error.step = 0;
            error.code = errorRecordCode613c(getInstance_(), NULL);
            service->notify(0x6002, 0, error.facility, 1, &error);
        } else {
            result = advanceNetworkState5(getInstance_());
            if (result < 0) {
                service->startStep_16 = 0;
                error.facility = 0x80000007;
                error.step = 111;
                error.code = result;
                service->notify(0x6002, 0, error.facility, 1, &error);
            } else if (result > 0) {
                service->startStep_16++;
            }
        }
        break;
    case 4:
        service->startStep_16 = 0;
        service->notify(0x6002, 0, 0, 0, NULL);
        break;
    }
}
