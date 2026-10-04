/*
 * Network/NetworkReflectService.cpp - the head of the `NetworkReflectService` class (the GameSpy reflect
 * service the mediator starts, stops and agrees): its constructor and deleting destructor, the
 * init/finalize/start/stop/page/agree entry points, the per-frame step, the start sequence and the GameSpy
 * sub-machines (`updateCallbackStep`, `dispatchTask`, `runSearch`, `runConnect`, `applyEvent`).
 *
 * `.text` 0x8041A194..0x8041B194.  Sections: extab 0x8001CF88..0x8001D010; extabindex 0x8003D950..0x8003D9EC;
 * .data 0x80603118..0x806031A0 (the start and stop log strings, then the class table the destructor emits);
 * .sbss 0x80794CD8..0x80794CE0 (`sNetworkReflectService`).
 *
 * WHAT IT IS.  Names from the map (`__ct__21NetworkReflectServiceFv`, `notify`, the `reflectService*` family);
 * the helpers this pass named (`resetReflectService`, `resetReflectServiceTask`, `updateReflectService`,
 * `stepReflectServiceStart`, `reflectServiceEventCallback`, `sNetworkReflectService`) are GUESSES from the
 * bodies.  The log string says `ReflectInterface::start()`, so the retail class name may differ.
 *
 * BOUNDARY.  The right edge 0x8041B194 / 0x806031A0 is request net2-l4-cff5#1: the five sub-machines moved here
 * from `Network/GameSpyInterfaceThread.cpp`, and the class table 0x80603190 follows this unit's strings - the
 * layout one object gives when the destructor is the key function (rule 10: the table is compiler output).
 * `.data` residual: 132 of 136 B - the target's last 4 bytes are the link's 8-byte alignment pad before the
 * neighbour's `.data`, which dtk sizes into the table symbol.
 *
 * FLAGS.  `-O3`/`-inline noauto` in place of the lib's `-O4,p`/`-inline auto` (configure.py, with the
 * numbers), and `#pragma peephole off` like the rest of the library.  `setReflectServicePage` takes the page
 * as `u8` (retail stores it with no `clrlwi`).
 */
#include "types.h"
#include "Network/NetworkReflectService.h"
#include "Network/PatInterface.h"
#include "Network/network_state.h"
#include "Network/network_layer_io.h"  /* sendReqChannelInfo / sendReqChannelData / sendReqConnect */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Network/sNetworkLibrary.h"    /* sNetworkLibrary, sNetworkLibraryError */
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

/* Drops the reflect callback slot when the Pat interface still holds it, finalizes the service and
 * unpublishes it. */
NetworkReflectService::~NetworkReflectService()
{
    if (getInstance_() != NULL && isCallback(getInstance_(), 7) != 0) {
        resetCallback(getInstance_(), 7);
    }
    finalizeReflectService(this);
    sNetworkReflectService = NULL;
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

/* Runs the connect-attempt callback sub-machine, one step per frame. */
void NetworkReflectService::updateCallbackStep()
{
    u8 step;

    if (callbackStep_17 != 0 && getInstance_() == NULL) {
        NetworkLogger* lm = getNetworkLogger();
        lm->warn_10("Maybe, ReflectInterface::stop() is called after final()\n");
        callbackStep_17 = 0;
    }
    step = callbackStep_17;
    switch (step) {
    case 1:
        if (connectStep_15 == 0) {
            callbackStep_17 = 6;
            break;
        }
        if (isCallback(getInstance_(), 7) == 0) {
            callbackStep_17 = 5;
            break;
        }
        if (hasMultipleRefs60d4(getInstance_()) != 0) {
            callbackStep_17 = 4;
            break;
        }
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            callbackStep_17 = 4;
            break;
        }
        flags_0C = 0;
        sendReqShut(getInstance_(), 1);
        callbackStep_17 += 1;
        break;
    case 2:
        if ((flags_0C & 1) != 0 || (flags_0C & 2) != 0 || (flags_0C & 0x10) != 0) {
            flags_0C = 0;
            resetNetworkState3(getInstance_());
            callbackStep_17 += 1;
        }
        break;
    case 3:
        if ((flags_0C & 8) != 0) {
            callbackStep_17 += 1;
        }
        break;
    case 4:
        resetCallback(getInstance_(), 7);
        callbackStep_17 += 1;
        break;
    case 5:
        decrement60d4(getInstance_());
        callbackStep_17 += 1;
        break;
    case 6: {
        NetworkLogger* lm = getNetworkLogger();
        if (lm->isVerbose_3C() > 0) {
            callbackStep_17 += 1;
        }
        break;
    }
    case 7:
        callbackStep_17 = 0;
        connectStep_15 = 0;
        notify(0x6003, 0, 0, 0, NULL);
        break;
    default:
        break;
    }
}

/* Dispatches to the search sub-machine (task 1) or the connect sub-machine (task 2). */
s32 NetworkReflectService::dispatchTask()
{
    switch (task_10) {
    case 1:
        return runSearch();
    case 2:
        return runConnect();
    default:
        return 0;
    }
}

/* Runs the GameSpy connect sub-machine, one step per frame. */
s32 NetworkReflectService::runSearch()
{
    u32 pending;
    u32 offset;
    u32 size;
    u32 info[3];

    switch (searchStep_14) {
    case 0:
        flags_0C = 0;
        sendReqChannelInfo(getInstance_(), channel_18);
        searchStep_14 = 5;
        break;
    case 5:
        pending = flags_0C;
        if ((pending & 1) != 0) {
            searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            searchStep_14 = 0x64;
        } else if ((pending & 0x80) != 0) {
            writePos_8168 = 0;
            searchStep_14 = 0x0A;
        }
        break;
    case 0x0A:
        flags_0C = 0;
        offset = writePos_8168;
        size = limit_8164 - offset;
        size = size >= 0x2000 ? 0x2000 : size;
        sendReqChannelData(getInstance_(), channel_18, offset, size);
        searchStep_14 = 0x0F;
        break;
    case 0x0F:
        pending = flags_0C;
        if ((pending & 1) != 0) {
            searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            searchStep_14 = 0x64;
        } else if ((pending & 0x100) != 0) {
            if (limit_8164 - writePos_8168 != 0) {
                searchStep_14 = 0x0A;
            } else {
                searchStep_14 = 0x14;
            }
        }
        break;
    case 0x14:
        notify(0x6004, 0, 0, 1, &channel_18);
        return 1;
    case 0x64:
        info[0] = 0x80000007;
        info[1] = 0;
        info[2] = (u32)errorRecordCode613c(getInstance_(), 0);
        notify(0x6004, 0, (s32)info[0], 1, info);
        return 1;
    case 0x6E:
        getErrorInfo654c(getInstance_(), info);
        notify(0x6004, 0, (s32)info[0], 1, info);
        notify(0x6001, 0, (s32)info[0], 1, info);
        return 1;
    default:
        break;
    }
    return 0;
}

/* Runs the GameSpy NAT/connect sub-machine, one step per frame. */
s32 NetworkReflectService::runConnect()
{
    u32 pending;
    u32 info[3];

    switch (searchStep_14) {
    case 0:
        flags_0C = 0;
        sendReqConnect(getInstance_());
        searchStep_14 = 5;
        break;
    case 5:
        pending = flags_0C;
        if ((pending & 1) != 0) {
            searchStep_14 = 0x6E;
        } else if ((pending & 2) != 0) {
            searchStep_14 = 0x64;
        } else if ((pending & 0x200) != 0) {
            searchStep_14 = 0x0A;
        }
        break;
    case 0x0A:
        notify(0x6005, 0, 0, 0, NULL);
        return 1;
    case 0x64:
        info[0] = 0x80000007;
        info[1] = 0;
        info[2] = (u32)errorRecordCode613c(getInstance_(), 0);
        notify(0x6005, 0, (s32)info[0], 1, info);
        return 1;
    case 0x6E:
        getErrorInfo654c(getInstance_(), info);
        notify(0x6005, 0, (s32)info[0], 1, info);
        notify(0x6001, 0, (s32)info[0], 1, info);
        return 1;
    default:
        break;
    }
    return 0;
}

/* Applies a DWC event to the interface, then folds the event bits into the state machine's flags. */
void NetworkReflectService::applyEvent(u32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg)
{
    u32 limit;
    GameSpyChannel* dst;
    u32 size;
    const GameSpyChannel* src;
    s32 i;

    switch (code) {
    case 0x8000:
    case 0x8007: {
        u32 info[3];

        if (task_10 <= 0) {
            getErrorInfo654c(getInstance_(), info);
            notify(0x6001, 0, (s32)info[0], 1, info);
        }
        flags_0C |= 1;
        break;
    }
    case 0x8002:
        flags_0C |= 2;
        break;
    case 0x8004:
        if (b != 0) {
            flags_0C |= 1;
        }
        break;
    case 0x8005:
        flags_0C |= 8;
        break;
    case 0x8006:
        flags_0C |= 1;
        break;
    case 0x8080:
        src = msg->channelView.channels_10;
        if (msg->channelView.channel_00 != channel_18) {
            NetworkPostedError error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            ((NetworkInstanceDispatch*)getInstance_())->postError(error);
            break;
        }
        peerId_1C = msg->channelView.peerId_04;
        channelCount_8020 = msg->channelView.count_0C;
        if (channelCount_8020 > 8) {
            channelCount_8020 = 8;
        }
        limit = msg->channelView.limit_08;
        limit_8164 = limit;
        if (limit_8164 > 0x8000) {
            limit_8164 = 0x7FFF;
        }
        for (i = 0, dst = channels_8024; i < channelCount_8020; i++) {
            dst->ownerId_00 = src->ownerId_00;
            dst->peerId_04 = src->peerId_04;
            memcpy(dst->address_08, src->address_08, 0x1F);
            dst->tail_27 = 0;
            src++;
            dst++;
        }
        flags_0C |= 0x80;
        break;
    case 0x8081:
        if (msg->dataView.channel_00 != channel_18 ||
            msg->dataView.writePos_04 != writePos_8168) {
            NetworkPostedError error;

            error.code_00 = 0x80000000;
            error.param1_04 = 0;
            error.param2_08 = 0;
            ((NetworkInstanceDispatch*)getInstance_())->postError(error);
            break;
        }
        limit = limit_8164 - writePos_8168;
        size = msg->dataView.size_08;
        if (limit < size) {
            size = limit;
        }
        memcpy(recvArea_20 + writePos_8168, msg->dataView.data_0C, size);
        writePos_8168 += size;
        flags_0C |= 0x100;
        break;
    case 0x8082:
        flags_0C |= 0x200;
        break;
    default:
        break;
    }
}
