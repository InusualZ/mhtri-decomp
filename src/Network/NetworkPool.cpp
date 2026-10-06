/*
 * Network/NetworkPool.cpp - the network pool singleton (`getNetworkPool`, the EC (Wii Shop) client `NetworkPool`: its
 *   NAND/NHTTP startup and cleanup steps, the purchase step and the empty download/ticket hooks) and the library's
 *   random generator `NetworkRandom`.
 * RANGE. .text 0x804123F8-0x80413450 (49 functions); .data 0x80602428-0x806024B8, .sdata 0x80793988-0x80793990, .sbss
 *   0x80794CB8-0x80794CC0, .sdata2 0x8079C868-0x8079C870, extab, extabindex.  Left edge: the `.data` V->S seam at
 *   0x80602428 (PatInterface's table 0x80602198, then this range's first string "NHTTPStartup"); `onNHTTPDestroyed`
 *   (0x804123F8) is the `NHTTPDestroy` callback only `stepCleanup` takes.  Right edge: `mediatorEventCallback`
 *   0x80413450 (`Network/NetworkWiiMediator.cpp`).
 * FLAGS. `-O3 -inline noauto` (configure.py; docs/network.md) and a file-wide `#pragma peephole off`: retail keeps
 *   `rlwinm`+`cmpwi` / `or`+`cmpwi` unfused (`stepPurchase`, `startDownload`), stages the table address in r0, keeps
 *   `extsh`+`cmpwi` on the destructor flag and sets up a `memset`'s address before its constants (`reset`,
 *   `clearState`); the pass on costs those five rows.
 * NAMES. `getNetworkPool` is a GUESS named by the mediator band (it returns the singleton); every `NetworkPool`
 *   method and field name is a GUESS from its body; `setAccount` from the mediator's `setConnectionPaths` (user id,
 *   password), `getPointBalance` from `purchase` comparing the row price with it.
 * RESIDUALS. None in `.text`, `extab` or `extabindex`; `.sbss`/`.sdata`/`.sdata2` are the target's 8-byte padded
 *   fragments of our 4/5/4 bytes (the link places them identically: the DOL hash holds with the unit linked).
 * SHAPES. The two tables come out in retail's order (`NetworkPool`'s 0x80602490, then `NetworkRandom`'s 0x806024A0)
 *   from one TU, so the zigzag `splitcheck --unit` reads at 0x806024A0 is not a seam.
 *  - the timed handler is a real `new NetworkTimedHandler()` (its extab cleanup record) and `delete session` (the
 *    virtual deleting destructor, slot +0x08).
 */

#include "Network/NetworkPool.h"
#include "Network/GameSpyInterfaceThread.h"   /* NetworkTimedHandler; getInstance */
#include "Network/NetworkWiiMediator.h"       /* getMediaVersionString */
#include "Network/NetworkStreamSink.h"        /* getNetworkLogger */
#include "NAND/nand.h"
#include "NHTTP/d_nhttp.h"
#include "SC/sc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "MSL_C/alloc.h"                      /* snprintf, sscanf */

#pragma peephole off


/* 0x80794CB8 (.sbss) - the live pool (the constructor publishes it, the destructor clears it). */
NetworkPool* sNetworkPool;

extern "C" {

/* 0x804123F8 (0x24): `NHTTPDestroy`'s completion; advances the cleanup step. */
void onNHTTPDestroyed(void)
{
    getNetworkPool()->advanceCleanup();
}

/* 0x8041241C (0x8): returns the live pool. */
NetworkPool* getNetworkPool(void)
{
    return sNetworkPool;
}

/* 0x80412424 (0x34): `NANDCheckAsync`'s completion. */
void onNANDCheckDone(s32 result, struct NANDCommandBlock* block)
{
    getNetworkPool()->handleCheckResult(result);
}

/* 0x80412458 (0x34): the `ec.cfg` open's completion. */
void onConfigOpened(s32 result, struct NANDCommandBlock* block)
{
    getNetworkPool()->handleOpenResult(result);
}

/* 0x8041248C (0x34): the `ec.cfg` read's completion. */
void onConfigRead(s32 result, struct NANDCommandBlock* block)
{
    getNetworkPool()->handleReadResult(result);
}

/* 0x804124C0 (0x34): the `ec.cfg` close's completion. */
void onConfigClosed(s32 result, struct NANDCommandBlock* block)
{
    getNetworkPool()->handleCloseResult(result);
}

/* 0x804124F4 (0x34): the `ec.cfg` delete's completion. */
void onConfigDeleted(s32 result, struct NANDCommandBlock* block)
{
    getNetworkPool()->handleDeleteResult(result);
}

}

/* 0x80412528 (0x48): publishes the pool and resets it. */
NetworkPool::NetworkPool()
{
    sNetworkPool = this;
    session = NULL;
    reset();
}

/* 0x80412570 (0x6C): deletes the timed handler and unpublishes the pool. */
NetworkPool::~NetworkPool()
{
    destroySession();
    sNetworkPool = NULL;
}


/* 0x804125DC (0x8): `init` without the timed handler. */
void NetworkPool::init(NetworkPoolCallback callback, void* arg, const NetworkPoolConfig* config) /* untyped: caller-owned payload */
{
    init(callback, arg, config, 0);
}

/* 0x804125E4 (0xF4): resets the pool, installs the event sink and copies the setup record. */
void NetworkPool::init(NetworkPoolCallback callback, void* arg, const NetworkPoolConfig* config, s32 timed) /* untyped: caller-owned payload */
{
    reset();
    this->callback = callback;
    callbackArg = arg;
    if (config != NULL) {
        memcpy(name, config->name, sizeof(config->name));
        name[0x10] = 0;
        memcpy(region, config->region, sizeof(config->region));
        region[0x0A] = 0;
        period = config->period;
        commandCallback = config->commandCallback;
        commandCallbackEx = config->commandCallbackEx;
        option_3C = config->option_30;
        option_40 = config->option_34;
        this->timed = timed;
        if (timed != 0 && session == NULL) {
            session = new NetworkTimedHandler();
            session->init(config->period);
        }
    }
}

/* 0x804126D8 (0x84): clears the event sink and the setup record, then the state. */
void NetworkPool::reset()
{
    callback = NULL;
    callbackArg = NULL;
    memset(name, 0, sizeof(name));
    memset(region, 0, sizeof(region));
    period = 0;
    commandCallback = NULL;
    commandCallbackEx = NULL;
    option_3C = 0;
    option_40 = 0;
    ec_started = 0;
    timed = 0;
    clearState();
}

/* 0x8041275C (0x8C): clears the operation state, the catalogue and the timestamp. */
void NetworkPool::clearState()
{
    transfer_total = 0;
    flag_45 = 0;
    flag_46 = 0;
    time_48 = 0.0f;
    mode = 0;
    pending = 0;
    startup_step = 0;
    cleanup_step = 0;
    operation_step = 0;
    progress = 0;
    memset(catalog_data, 0, sizeof(catalog_data));
    catalog_status = 0;
    item_count = 0;
    timestamp = 0;
    restriction_bypassed = 0;
    config_read = 0;
}

/* 0x804127E8 (0x54): deletes the timed handler. */
void NetworkPool::destroySession()
{
    if (session != NULL) {
        delete session;
        session = NULL;
    }
}

/* 0x8041283C (0x88): runs the cleanup, the startup or the current operation's step. */
void NetworkPool::update()
{
    if (cleanup_step != 0) {
        stepCleanup();
    } else if (startup_step != 0) {
        stepStartup();
    } else {
        switch (mode) {
        case 1:
            stepPurchase();
            break;
        case 2:
            stepDownload();
            break;
        }
        if (pending != 0) {
            stepPending();
        }
    }
}

/* 0x804128C4 (0x40): resets the state and starts the startup sequence. */
void NetworkPool::start()
{
    clearState();
    startup_step = 1;
    progress++;
}

/* 0x80412904 (0x14): arms the cleanup sequence. */
void NetworkPool::startCleanup()
{
    cleanup_step = 1;
    startup_step = 0;
}

/* 0x80412918 (0x10): advances the cleanup sequence. */
void NetworkPool::advanceCleanup()
{
    cleanup_step++;
}

/* 0x80412928 (0xAC): the quota check's result: an error ends the operation, success advances it. */
void NetworkPool::handleCheckResult(s32 result)
{
    NetworkErrorInfo error;

    if (result != 0) {
        operation_step = 0;
        error.code_00 = 0x80000007;
        error.param1_04 = 85;
        error.param2_08 = result;
        switch (mode) {
        case 1:
            postEvent(0x4005, 0, error.code_00, 1, &error);
            break;
        case 2:
            postEvent(0x4006, 0, error.code_00, 1, &error);
            break;
        }
        mode = 0;
    } else {
        operation_step++;
    }
}

/* 0x804129D4 (0x24): the `ec.cfg` open's result. */
void NetworkPool::handleOpenResult(s32 result)
{
    if (result == 0) {
        startup_step++;
    } else {
        startup_step = 11;
    }
}

/* 0x804129F8 (0x3C): the `ec.cfg` read's result; a corrupt file is deleted after the close. */
void NetworkPool::handleReadResult(s32 result)
{
    if (result >= 0) {
        config_read = 1;
    } else if (result == -5 || result == -15) {
        delete_config = 1;
    }
    startup_step++;
}

/* 0x80412A34 (0x3C): the `ec.cfg` close's result. */
void NetworkPool::handleCloseResult(s32 result)
{
    if (result == 0) {
        if (delete_config != 0) {
            startup_step++;
        } else {
            startup_step = 11;
        }
    } else {
        startup_step = 13;
    }
}

/* 0x80412A70 (0x20): the `ec.cfg` delete's result. */
void NetworkPool::handleDeleteResult(s32 result)
{
    if (result == 0) {
        startup_step = 11;
    } else {
        startup_step = 13;
    }
}

/* 0x80412A90 (0x4): an empty hook. */
void NetworkPool::syncTickets()
{
}

/* 0x80412A94 (0x4): an empty hook. */
void NetworkPool::deleteTicket(s32 itemId)
{
}

/* 0x80412A98 (0x168): starts the purchase of catalogue item `itemId` when no operation runs and the points suffice. */
void NetworkPool::purchase(s32 itemId)
{
    NetworkErrorInfo error;
    s32 index;
    s32 balance;

    if (mode > 0 || pending > 0) {
        error.code_00 = 0x80000006;
        error.param1_04 = 0;
        error.param2_08 = 0;
        postEvent(0x4005, 0, error.code_00, 1, &error);
        return;
    }
    for (index = 0; index < item_count; index++) {
        if (itemId == items[index].id) {
            break;
        }
    }
    if (index >= item_count) {
        error.code_00 = 0x80000003;
        error.param1_04 = 0;
        error.param2_08 = 0;
        postEvent(0x4005, 0, error.code_00, 1, &error);
        return;
    }
    memset(&error, 0, sizeof(error));
    balance = getPointBalance(&error);
    if (items[index].price > balance) {
        if (error.code_00 == 0) {
            error.code_00 = 0x80000007;
            error.param1_04 = 83;
        }
        postEvent(0x4005, 0, error.code_00, 1, &error);
        return;
    }
    item_index = index;
    mode = 1;
    operation_step = 1;
}

/* 0x80412C00 (0x8): whether the startup finished. */
u8 NetworkPool::isConfigRead()
{
    return config_read;
}

/* 0x80412C08 (0x8): always 1. */
s32 NetworkPool::isShopAvailable()
{
    return 1;
}

/* 0x80412C10 (0x8): always -1. */
s32 NetworkPool::setAccount(const char* userId, const char* password)
{
    return -1;
}

/* 0x80412C18 (0x8): always 0. */
s32 NetworkPool::getPointBalance(struct NetworkErrorInfo* error)
{
    return 0;
}

/* 0x80412C20 (0x40): whether the parental controls restrict the shop (0 while the check is bypassed). */
s32 NetworkPool::isPurchaseRestricted()
{
    if (restriction_bypassed != 0) {
        return 0;
    }
    return SCCheckPCShopRestriction() == 1;
}

/* 0x80412C60 (0x8): whether the parental-control check is bypassed. */
u8 NetworkPool::isRestrictionBypassed(s32 kind)
{
    return restriction_bypassed;
}

/* 0x80412C68 (0x4): opens the Wii Shop Channel's help page. */
void NetworkPool::launchShopHelp()
{
    OSLaunchShopChannelHelp();
}

/* 0x80412C6C (0xC): stores the transfer total. */
void NetworkPool::setTransferTotal(u64 total)
{
    transfer_total = total;
}

/* 0x80412C78 (0x18): clears the transfer total and marks the progress complete. */
void NetworkPool::finishTransfer()
{
    transfer_total = 0;
    progress = 10;
}

/* 0x80412C90 (0x4): an empty hook. */
void NetworkPool::setTransferOption(s32 option, s32 value)
{
}

/* 0x80412C94 (0x4): an empty hook. */
void NetworkPool::selectTransferMode(s32 mode)
{
}

/* 0x80412C98 (0x108): starts the download operation when no operation runs and the media version parses. */
void NetworkPool::startDownload()
{
    u64 version;
    NetworkErrorInfo error;
    char text[0x20];

    progress++;
    if (mode > 0 || pending > 0) {
        error.code_00 = 0x80000006;
        error.param1_04 = 0;
        error.param2_08 = 0;
        postEvent(0x4006, 0, error.code_00, 1, &error);
        return;
    }
    version = 0;
    getMediaVersionString(getInstance(), text, sizeof(text));
    sscanf(text, "%llu", &version);
    if (version == 0) {
        error.code_00 = 0x80000000;
        error.param1_04 = 0;
        error.param2_08 = 0;
        postEvent(0x4006, 0, error.code_00, 1, &error);
        return;
    }
    mode = 2;
    operation_step = 1;
}

/* 0x80412DA0 (0x28): calls the event sink with the event and the sink's argument. */
void NetworkPool::postEvent(u32 code, s32 a, s32 b, s32 c, const struct NetworkErrorInfo* error)
{
    callback(code, a, b, c, error, callbackArg);
}

/* 0x80412DC8 (0x324): the startup sequence: the account check, NHTTP, then the `ec.cfg` open/read/close/delete. */
void NetworkPool::stepStartup()
{
    NetworkErrorInfo error;
    s32 result;

    switch (startup_step) {
    case 1:
        if (ec_started != 0) {
            startup_step = 0;
            error.code_00 = 0x80000007;
            error.param1_04 = 81;
            error.param2_08 = 0;
            postEvent(0x4001, 0, error.code_00, 1, &error);
            break;
        }
        result = getNetworkLogger()->checkAccount_38(0, &error);
        if (result < 0) {
            startup_step = 0;
            postEvent(0x4001, 0, error.code_00, 1, &error);
        } else if (result > 0) {
            ec_started = 1;
            if (timed != 0) {
                startup_step = 11;
            } else {
                startup_step++;
            }
        }
        break;
    case 2:
        result = NHTTPi_RegisterCallbacks(commandCallback, commandCallbackEx, 15);
        getNetworkLogger()->signal_0C(3, "NHTTPStartup(%d)\n", result);
        if (result < 0) {
            startup_step = 0;
            error.code_00 = 0x80000007;
            error.param1_04 = 51;
            error.param2_08 = NHTTPGetError() + 207900;
            postEvent(0x4001, 0, error.code_00, 1, &error);
        } else {
            startup_step = 11;
        }
        break;
    case 3:
        if (NANDGetHomeDir(path) != 0) {
            startup_step = 11;
            break;
        }
        snprintf(path, sizeof(path), "%s/ec.cfg", path);
        startup_step = 4;
        if (NANDOpenAsync(path, (NANDFileInfo*)file_info, 1, onConfigOpened, (NANDCommandBlock*)command_block) != 0) {
            startup_step = 11;
        }
        break;
    case 5:
        delete_config = 0;
        startup_step = 6;
        if (NANDReadAsync((NANDFileInfo*)file_info, read_buffer, sizeof(read_buffer), onConfigRead,
                          (NANDCommandBlock*)command_block) != 0) {
            startup_step = 7;
        }
        break;
    case 7:
        startup_step = 8;
        if (NANDCloseAsync((NANDFileInfo*)file_info, onConfigClosed, (NANDCommandBlock*)command_block) != 0) {
            startup_step = 11;
        }
        break;
    case 9:
        startup_step = 10;
        if (NANDDeleteAsync(path, onConfigDeleted, (NANDCommandBlock*)command_block) != 0) {
            startup_step = 11;
        }
        break;
    case 11:
        config_read = 1;
        startup_step = 0;
        postEvent(0x4001, 0, 0, 0, NULL);
        break;
    case 13:
        startup_step = 0;
        error.code_00 = 0x80000007;
        error.param1_04 = 92;
        error.param2_08 = 0;
        postEvent(0x4001, 0, error.code_00, 1, &error);
        break;
    }
}

/* 0x804130EC (0x104): the cleanup sequence: `NHTTPDestroy`, then the wait for the logger before posting 0x4002. */
void NetworkPool::stepCleanup()
{
    s32 done;

    switch (cleanup_step) {
    case 1:
        if (timed != 0) {
            cleanup_step = 3;
        } else if (ec_started != 0) {
            cleanup_step = 2;
            NHTTPDestroy(onNHTTPDestroyed);
            getNetworkLogger()->signal_0C(3, "NHTTPCleanup\n");
        } else {
            cleanup_step = 3;
        }
        break;
    case 3:
        done = 1;
        if (ec_started != 0) {
            done = getNetworkLogger()->isVerbose_3C();
        }
        if (done > 0) {
            cleanup_step = 0;
            ec_started = 0;
            postEvent(0x4002, 0, 0, 0, NULL);
        }
        break;
    }
}

/* 0x804131F0 (0x18C): the purchase sequence: the NAND quota check, then the parental-control check. */
void NetworkPool::stepPurchase()
{
    NetworkErrorInfo error;
    s32 result;

    switch (operation_step) {
    case 1:
        quota_answer = -1;
        operation_step++;
        result = NANDCheckAsync(1, 2, &quota_answer, onNANDCheckDone, (NANDCommandBlock*)command_block);
        if (result != 0) {
            mode = 0;
            operation_step = 0;
            error.code_00 = 0x80000007;
            error.param1_04 = 85;
            error.param2_08 = result;
            postEvent(0x4005, 0, error.code_00, 1, &error);
        }
        break;
    case 3:
        mode = 0;
        operation_step = 0;
        if (quota_answer & 4) {
            error.code_00 = 0x80000007;
            error.param1_04 = 86;
            error.param2_08 = 0;
            postEvent(0x4005, 0, error.code_00, 1, &error);
        } else if (quota_answer & 8) {
            error.code_00 = 0x80000007;
            error.param1_04 = 88;
            error.param2_08 = 0;
            postEvent(0x4005, 0, error.code_00, 1, &error);
        } else if (isPurchaseRestricted()) {
            error.code_00 = 0x80000007;
            error.param1_04 = 82;
            error.param2_08 = 0;
            postEvent(0x4005, 0, error.code_00, 1, &error);
        } else {
            pending = 3;
        }
        break;
    }
}

/* 0x8041337C (0x4): an empty step. */
void NetworkPool::stepDownload()
{
}

/* 0x80413380 (0x4): an empty step. */
void NetworkPool::stepPending()
{
}

/* 0x80413384 (0x3C): seeds the generator with the C library's `rand` constants. */
NetworkRandom::NetworkRandom()
{
    seed = 1;
    multiplier = 0x41C64E6D;
    increment = 12345;
    shift = 16;
    mask = 0x7FFF;
}

/* 0x804133C0 (0x44): the generator's destructor. */
NetworkRandom::~NetworkRandom()
{
}


/* 0x80413404 (0x18): stores all five generator words. */
void NetworkRandom::setParameters(u32 seed, u32 multiplier, u32 increment, u32 shift, u32 mask)
{
    this->seed = seed;
    this->multiplier = multiplier;
    this->increment = increment;
    this->shift = shift;
    this->mask = mask;
}

/* 0x8041341C (0x8): stores the seed. */
void NetworkRandom::setSeed(u32 seed)
{
    this->seed = seed;
}

/* 0x80413424 (0x2C): steps the generator and returns the masked high bits. */
u32 NetworkRandom::next()
{
    seed = seed * multiplier + increment;
    return (seed >> shift) & mask;
}
