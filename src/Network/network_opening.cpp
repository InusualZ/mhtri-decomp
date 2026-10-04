/*
 * Network/network_opening.cpp - the network mediator's opening steps, its game-info copy and its four
 * transfer slots (the `NetworkWiiMediator::ECStart()` / `openingStart()` / `openingStop()` strings).
 *
 * `.text` 0x804155D4..0x80417BC0.  Sections: extab 0x8001CBF4..0x8001CD94; extabindex 0x8003D524..0x8003D764;
 * .rodata 0x80570E70..0x80570E98 (read by 0x80416A30); .data 0x80602824..0x80602968; .sdata2 0x8079C874..0x8079C878 (read by `initMediatorTerms`).
 *
 * WHAT IT IS.  Methods of `NetworkWiiMediator` (the strings name the class): the game-info copy pair, the
 * terms entry points, the transfer-slot family (a 0x1000-byte record queue, an activity byte, a timer and a
 * mode record per slot, all in the mediator's fields from +0x2D96) and the opening/EC steps.  The functions
 * other units already call by a C name (`setGameInfo2d1c`, `initMediatorTerms`, `setMediatorTransfer*`, ...)
 * keep it; the ones no source names yet are members (`NetworkWiiMediator::openTransferSlot`, ...).
 *
 * NAMES.  `ECStart`, `openingStart` and `openingStop` are the strings' own; every other member name is a
 * GUESS from the body (the field it reads or writes), and so are the callees this pass named in their
 * owners' headers (`reportPatError`, `notifyPatEvent`, `openPatInterface`, `mediatorEventCallback`,
 * `NetworkPool::start`/`isECStarted`).  The slot family's fields are named in include/Network/NetworkWiiMediator.h.
 * `NetworkPool::isECStarted` (0x8041793C) is the pool's accessor, emitted in this range.
 *
 * BOUNDARY.  The base library class `sNetworkLibrary` that followed (0x80417BC0..0x8041891C) is its own unit,
 * `Network/sNetworkLibrary.cpp`, with the whole `.data` and `.sbss` this unit used to claim (its header has the
 * seam evidence); the worker-thread entry points after it are `Network/sNetworkLibraryWii.cpp`'s.  `.data`
 * 0x80602824..0x80602968 is the five strings only the opening steps read (emitted here: 321 of the claimed 324 B,
 * the rest is the trailing alignment).  The two tables after them (0x80602968, 0x80602978)
 * close this TU's `.data` by MWCC's order but are read by `Network/network_layer_io.cpp` and are code-pointer
 * tables (rule 10), so they stay unclaimed until a class here emits them.
 *
 * FLAGS.  `-O3` in place of the lib's `-O4,p` (configure.py).  `#pragma peephole off` (retail keeps `extsb` +
 * `cmpwi` on the slot argument: getTransferSlotFlag 31.25 -> 93.44 -> 100 with the range spelling below),
 * back on around `setMediatorTransferMode` (its raw `stb` of the u32 mode: 95.96 -> 100); `#pragma
 * auto_inline off` (retail calls `clearTransferQueue` where the lib's `-inline auto` inlines it); `#pragma
 * pool_data off` (each log string its own `lis`/`addi`).  Shapes: the slot range test is written
 * `slot < 0 || 4 <= slot` with an early return (MWCC folds `slot >= 4` into one unsigned compare), the queue
 * is one flat array indexed by `slot << 12`, and `applyEvent` takes the code as `s32` (the `addis` before the
 * unsigned case compares).
 *
 * RESIDUALS.  `popTransferRecord` 97.19 (retail computes `&length` before the queue address for the first
 * `memcpy`, and keeps the activity byte's address in r5); `pushTransferRecord` 98.71 (callee-saved registers
 * r29-r31 permuted; declaration order and a pointer-free spelling measured, neither moves it);
 * `setGameInfo2d1c` 99.85 (the title length lands in r29 where retail reuses r28).  `getNASToken` returns
 * the token inside `DWCSvlResult` (owner `DWCi/dwc_nasfunc.h`).
 *
 * UNWRITTEN.  The word filter 0x804155D4 (2520 B) and `postMediatorRecord`; the forwarding wrappers over the
 * `NetworkPool` singleton 0x80416028..0x80416578 (its methods are `Network/network_layer_io.cpp`'s, unnamed);
 * the sound helpers 0x804170D8..0x804171A4.
 *
 * TERMS WRAPPERS (0x80416800..0x80416C18, written from request net2-l4-cff5#3).  Every name this pass gave -
 * `cancelTermsUpdate`, `getTermsProgressLevel` (a forwarding thunk), `getMediatorTermsProgressLevel`,
 * `getMediatorTermsProgress`, `set/getMediatorTermsFlag`, the leaf accessors `isPatTermsReady` and
 * `set/getPatTermsFlag`, and the `menu/menu_plsearch.cpp` callees `initPatTerms`, `requestPatTermsCheck`,
 * `requestPatTermsUpdate`, `cancelPatTermsUpdate`, `getPatTermsProgress` - is a GUESS from the bodies.  Each leaf
 * accessor sits right after its first caller, the layout MWCC gives an uninlined inline function.
 * `setPatTermsFlag` takes a `u32` under `#pragma peephole on` (the raw `stb`, like `setMediatorTransferMode`);
 * a `u8` parameter puts a `clrlwi` in it (47.50) or in `setMediatorTermsFlag` (96.25).
 */
#include "Network/network_opening.h"
#include "Network/NetworkWiiMediator.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_layer_io.h"      /* NetworkPool, getNetworkPool, mediatorEventCallback */
#include "Network/network_state.h"         /* resetNetworkState, resetNetworkState3, handleNetworkState1, sendReqShut */
#include "Network/PatInterface.h"
#include "Network/NetworkSessionBase.h"    /* getNetworkBinaryState */
#include "enemy/em020_ai.h"                /* getInstance_ */
#include "unsplit/Network.h"               /* getNetworkLogger */
#include "Network/network_pat_control.h"     /* struct PatTerms; getPatTerms (through NetworkSessionManagerPat.h) */
#include "menu/menu_plsearch.h"            /* initPatTerms and the terms object's requests */
#include "MSL_C/alloc.h"                   /* memmove, snprintf */
#include "MSL/strlen.h"

#pragma peephole off
#pragma auto_inline off
#pragma pool_data off

/* Starts the EC sequence on the pool singleton, refused while the opening runs. */
void NetworkWiiMediator::ECStart()
{
    if (flag_1C != 0) {
        ((sNetworkLibrary*)getNetworkLogger())
            ->logError("NetworkWiiMediator::ECStart() must not be called, during openingStart()\n");
        return;
    }
    if (getNetworkPool() != NULL) {
        getNetworkPool()->start();
    }
}

/* Copies the reflect name (at most 31 characters) into `out` and returns the mediator's own copy. */
char* NetworkWiiMediator::getReflectName(char* out, u32 size)
{
    if (size != 0) {
        u32 length = (size < 32) ? size - 1 : 31;

        memcpy(out, reflect_name_3C, length);
        out[length] = 0;
    }
    return (char*)reflect_name_3C;
}

char* getNASToken(NetworkWiiMediator* self)
{
    return self->svl_result.svltoken;
}

/* Keeps the nine DWC game-info words; the two names and the title are copied into the mediator's own
 * buffers and the words point at those copies. */
void setGameInfo2d1c(NetworkWiiMediator* self, u32* info)
{
    const char* text;
    const u16* title;
    u32 length;

    self->game_info[0] = info[0];
    self->game_info[1] = info[1];
    self->game_info[2] = info[2];
    self->game_info_name[0] = 0;
    text = (const char*)info[3];
    if (text != NULL) {
        length = (strlen(text) < 17) ? strlen(text) : 16;
        memcpy(self->game_info_name, text, length);
        self->game_info_name[length] = 0;
    }
    self->game_info[3] = (u32)self->game_info_name;
    self->game_info[4] = info[4];
    self->game_info[5] = info[5];
    self->game_info_secret[0] = 0;
    text = (const char*)info[6];
    if (text != NULL) {
        length = (strlen(text) < 17) ? strlen(text) : 16;
        memcpy(self->game_info_secret, text, length);
        self->game_info_secret[length] = 0;
    }
    self->game_info[6] = (u32)self->game_info_secret;
    self->game_info_title[0] = 0;
    title = (const u16*)info[7];
    if (title != NULL) {
        u32 count = 0;

        while (title[count] != 0) {
            count++;
        }
        count = (count < 26) ? count : 25;
        memcpy(self->game_info_title, title, count * 2);
        self->game_info_title[count] = 0;
    }
    self->game_info[7] = (u32)self->game_info_title;
    self->game_info[8] = info[8];
}

void getGameInfo2d1c(NetworkWiiMediator* self, u32* out)
{
    out[0] = self->game_info[0];
    out[1] = self->game_info[1];
    out[2] = self->game_info[2];
    out[3] = self->game_info[3];
    out[4] = self->game_info[4];
    out[5] = self->game_info[5];
    out[6] = self->game_info[6];
    out[7] = self->game_info[7];
    out[8] = self->game_info[8];
}

/* Hands the terms object its buffer, empties every transfer slot and sets mode 1, both flags clear and the
 * default level. */
/* untyped: byte range - the MEM2 buffer handed to the terms object */
void initMediatorTerms(NetworkWiiMediator* self, void* buffer, u32 size)
{
    s32 i;

    initPatTerms(getPatTerms(), buffer, size);
    self->closeTransferSlots();
    for (i = 0; i < 4; i++) {
        self->clearTransferQueue(i);
    }
    self->transfer_mode = 1;
    self->transfer_flag_6DD1 = 0;
    self->transfer_flag_6DD2 = 0;
    self->transfer_level = 1.0f;
}

/* Starts the terms check, when there is a terms object. */
void startTermsCheck(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        requestPatTermsCheck(getPatTerms());
    }
}

/* The terms object's ready state, 0 when there is none. */
s32 getMediatorTermsStatus(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        return isPatTermsReady(getPatTerms());
    }
    return 0;
}

/* Whether the terms object's ready byte is set. */
u32 isPatTermsReady(PatTerms* terms)
{
    return terms->ready_0x0D != 0;
}

/* Starts the terms update, when there is a terms object. */
void startTermsUpdate(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        requestPatTermsUpdate(getPatTerms());
    }
}

/* Cancels the terms update, when there is a terms object. */
void cancelTermsUpdate(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        cancelPatTermsUpdate(getPatTerms());
    }
}

/* Stores the transfer mode, dropping every slot's queue when it changes. */
#pragma peephole on
void setMediatorTransferMode(NetworkWiiMediator* self, u32 mode)
{
    s32 i;

    if (self->transfer_mode != mode) {
        for (i = 0; i < 4; i++) {
            self->clearTransferQueue(i);
        }
    }
    self->transfer_mode = mode;
}
#pragma peephole off

/* Whether the terms update finished: 0 while the transfer mode is 0 or there is no terms object. */
s32 isMediatorTermsUpdateFinished(NetworkWiiMediator* self)
{
    if (self->transfer_mode != 0 && getPatTerms() != NULL) {
        return isTermsUpdateFinished(getPatTerms());
    }
    return 0;
}

/* Whether the terms object reached its update-finished state (11). */
u32 isTermsUpdateFinished(PatTerms* terms)
{
    return terms->state_0x0C == 11;
}

/* Forwards to the graded terms progress. */
s32 getTermsProgressLevel(NetworkWiiMediator* self)
{
    return getMediatorTermsProgressLevel(self);
}

/* Grades the terms progress count against the threshold table: the index of the first threshold it does not
 * exceed (16 past the sixteenth), 0 when there is no terms object. */
s32 getMediatorTermsProgressLevel(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        u16 progress = getPatTermsProgress(getPatTerms());
        u16 thresholds[17] = {
            83, 117, 165, 234, 330, 467, 659, 931, 1316, 1859, 2626, 3709, 5239, 7401, 10455, 14768, 20860
        };
        s32 level;

        for (level = 0; level < 16; level++) {
            if (progress <= thresholds[level]) {
                break;
            }
        }
        return level;
    }
    return 0;
}

/* The terms progress count, 0 when there is no terms object. */
u16 getMediatorTermsProgress(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        return getPatTermsProgress(getPatTerms());
    }
    return 0;
}

/* Stores the terms object's +0xE2 byte, when there is a terms object. */
void setMediatorTermsFlag(NetworkWiiMediator* self, u32 flag)
{
    if (getPatTerms() != NULL) {
        setPatTermsFlag(getPatTerms(), flag);
    }
}

#pragma peephole on
void setPatTermsFlag(PatTerms* terms, u32 flag)
{
    terms->flag_0xE2 = flag;
}
#pragma peephole off

/* The terms object's +0xE2 byte, 0 when there is no terms object. */
u8 getMediatorTermsFlag(NetworkWiiMediator* self)
{
    if (getPatTerms() != NULL) {
        return getPatTermsFlag(getPatTerms());
    }
    return 0;
}

u8 getPatTermsFlag(PatTerms* terms)
{
    return terms->flag_0xE2;
}

/* Activates slot `slot` with its mode and level (while the terms object is up) and empties its queue. */
void NetworkWiiMediator::openTransferSlot(s8 slot, u8 mode, f32 level)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (getMediatorTermsStatus(this) == 0) {
        return;
    }
    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active == 0) {
        entry->active = 1;
        entry->mode = mode;
        entry->flag = 0;
        entry->level = level;
        clearTransferQueue(slot);
    }
}

void NetworkWiiMediator::closeTransferSlot(s8 slot)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        entry->active = 0;
    }
}

void NetworkWiiMediator::closeTransferSlots()
{
    s32 i;

    for (i = 0; i < 4; i++) {
        closeTransferSlot(i);
    }
}

void NetworkWiiMediator::clearTransferQueue(s8 slot)
{
    memset(&transfer_queue[slot << 12], 0, 0x1000);
    transfer_queued[slot] = 0;
    transfer_activity[slot] = 0;
    transfer_timer[slot] = 0;
}

/* Appends one record (a u16 length, then the bytes) to the slot's queue, restarting a full queue;
 * returns the bytes taken, or 0 for a bad slot or an empty record. */
s32 NetworkWiiMediator::pushTransferRecord(s8 slot, const u8* data, s32 size)
{
    u16 length;
    u8* queue;
    s32* queued;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    if (size <= 0) {
        return 0;
    }
    queued = &transfer_queued[slot];
    if ((u32)(size + *queued) > 0x1000) {
        *queued = 0;
    }
    length = size;
    queue = &transfer_queue[slot << 12];
    memcpy(queue + *queued, &length, sizeof(length));
    memcpy(*queued + queue + 2, data, size);
    *queued = size + *queued + 2;
    return size + 2;
}

/* Takes the oldest record of the slot's queue into `out` (when it fits in `max` bytes) and returns its
 * length; also ages the slot's activity, re-judging it every 15 polls. */
u16 NetworkWiiMediator::popTransferRecord(s8 slot, u8* out, s32 max)
{
    u16 length;
    s32* queued;
    s32 offset;
    s32 taken;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    if (transfer_timer[slot] <= 0) {
        if ((transfer_activity[slot] & 0xF0) >= 0xB0) {
            transfer_activity[slot] = 1;
        } else {
            transfer_activity[slot] = 0;
        }
        transfer_timer[slot] = 15;
    }
    transfer_timer[slot]--;
    queued = &transfer_queued[slot];
    if (*queued > 2) {
        transfer_activity[slot] += 0x10;
    }
    if (*queued <= 2) {
        *queued = 0;
        return 0;
    }
    offset = slot << 12;
    memcpy(&length, &transfer_queue[offset], sizeof(length));
    if (max < length) {
        *queued = 0;
        return 0;
    }
    memcpy(out, &transfer_queue[offset] + 2, length);
    taken = length + 2;
    *queued -= taken;
    if (*queued > 0) {
        memmove(&transfer_queue[offset], &transfer_queue[offset] + taken, *queued);
    }
    return length;
}

void setMediatorTransferFlag6DD1(NetworkWiiMediator* self, u8 flag)
{
    self->transfer_flag_6DD1 = flag;
}

u8 NetworkWiiMediator::getTransferFlag6DD1()
{
    return transfer_flag_6DD1;
}

void setMediatorTransferFlag6DD2(NetworkWiiMediator* self, u8 flag)
{
    self->transfer_flag_6DD2 = flag;
}

u8 NetworkWiiMediator::getTransferFlag6DD2()
{
    return transfer_flag_6DD2;
}

void setMediatorTransferLevel(NetworkWiiMediator* self, f32 level)
{
    self->transfer_level = level;
}

f32 NetworkWiiMediator::getTransferLevel()
{
    return transfer_level;
}

/* Sets an active slot's mode; mode 0 drops its queue. */
void NetworkWiiMediator::setTransferSlotMode(s8 slot, u8 mode)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        entry->mode = mode;
        if (entry->mode == 0) {
            clearTransferQueue(slot);
        }
    }
}

u8 NetworkWiiMediator::getTransferSlotMode(s8 slot)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        return entry->mode;
    }
    return 0;
}

void NetworkWiiMediator::setTransferSlotFlag(s8 slot, u8 flag)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        entry->flag = flag;
    }
}

u8 NetworkWiiMediator::getTransferSlotFlag(s8 slot)
{
    NetworkWiiMediatorTransferSlot* entry;

    if (slot < 0 || 4 <= slot) {
        return 0;
    }
    entry = &transfer_slots[slot];
    if (entry->active != 0) {
        return entry->flag;
    }
    return 0;
}

/* Whether an active slot's ready flag (the activity's low nibble) is set. */
BOOL NetworkWiiMediator::isTransferSlotReady(s8 slot)
{
    if (slot < 0 || 4 <= slot) {
        return FALSE;
    }
    if (transfer_slots[slot].active == 0) {
        return FALSE;
    }
    return (transfer_activity[slot] & 0xF) != 0;
}

/* One step of the opening: start the library, wait for the NAS service-locator token, open the Pat
 * interface and run its login, then parse the reflect texts; every failure is reported to the Pat
 * interface's session handlers and stops the sequence. */
void NetworkWiiMediator::openingStart()
{
    sNetworkLibraryError error;
    s32 result;

    if (flag_1C != 0) {
        if (getInstance_() == NULL || GameSpyInterfaceThread::getInstance() == NULL) {
            ((sNetworkLibrary*)getNetworkLogger())
                ->logError("NetworkWiiMediator::openingStart() maybe be called before openingInit()\n");
            flag_1C = 0;
        }
        if (getNetworkPool() != NULL && getNetworkPool()->isECStarted()) {
            ((sNetworkLibrary*)getNetworkLogger())
                ->logError("NetworkWiiMediator::openingStart() must be called before ECStart()\n");
            flag_1C = 0;
            error.facility = 0x80000007;
            error.step = 0;
            error.code = 0;
            reportPatError(getInstance_(), 1, error);
        }
    }
    switch (flag_1C) {
    case 1:
        if (library_started != 0) {
            error.facility = 0x80000007;
            error.step = 106;
            error.code = 0;
            reportPatError(getInstance_(), 1, error);
            break;
        }
        result = ((sNetworkLibrary*)getNetworkLogger())->start(0, &error);
        if (result < 0) {
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (result > 0) {
            library_started = 1;
            memset(&svl_result, 0, sizeof(svl_result));
            if (field_24 != 0) {
                snprintf(svl_result.svltoken, sizeof(svl_result.svltoken), "DEBUG AUTHENTICATION TOKEN");
                flag_1C += 2;
            } else {
                GameSpyInterfaceThread::getInstance()->setWaitHandle(&svl_result);
                flag_1C++;
            }
        }
        break;
    case 2:
        result = GameSpyInterfaceThread::getInstance()->runNasLogin();
        if (result < 0) {
            GameSpyInterfaceThread::getInstance()->getErrorStruct((NetworkErrorInfo*)&error);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (result > 0) {
            flag_1C++;
        }
        break;
    case 3:
        openPatInterface(getInstance_());
        setCallback(getInstance_(), (void (*)())mediatorEventCallback, this, 1);
        setConnectServerType(getInstance_(), 2);
        event_flags = 0;
        resetNetworkState(getInstance_());
        flag_1C++;
        break;
    case 4:
        if (event_flags & 1) {
            getErrorInfo654c(getInstance_(), (u32*)&error);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (event_flags & 2) {
            error.facility = 0x80000007;
            error.step = 0;
            error.code = errorRecordCode613c(getInstance_(), NULL);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if ((u8)getNetworkBinaryState(getInstance_()) != 0) {
            flag_1C++;
        }
        break;
    case 5:
        if (event_flags & 1) {
            getErrorInfo654c(getInstance_(), (u32*)&error);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (event_flags & 2) {
            error.facility = 0x80000007;
            error.step = 0;
            error.code = errorRecordCode613c(getInstance_(), NULL);
            reportPatError(getInstance_(), 1, error);
            flag_1C = 0;
        } else if (handleNetworkState1(getInstance_()) != 0 || (event_flags & 0x10)) {
            flag_1C++;
        } else if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            flag_1C--;
        }
        break;
    case 6:
        parseReflectLines(this, 0);
        parseReflectLines(this, 1);
        parseReflectLines(this, 2);
        notifyPatEvent(getInstance_(), 1, 0, NULL);
        flag_1C = 0;
        break;
    }
}

BOOL NetworkPool::isECStarted()
{
    return ec_started;
}

/* One step of the closing: shut the Pat session down, release the Pat interface, then stop the library. */
void NetworkWiiMediator::openingStop()
{
    s32 result;

    if (flag_1D != 0 && getInstance_() == NULL) {
        ((sNetworkLibrary*)getNetworkLogger())
            ->logError("Maybe, NetworkWiiMediator::openingStop() is called after openingFinal()\n");
        flag_1D = 0;
    }
    switch (flag_1D) {
    case 1:
        if (isCallback(getInstance_(), 1) == 0) {
            flag_1D = 5;
            break;
        }
        if (hasMultipleRefs60d4(getInstance_()) != 0) {
            flag_1D = 4;
            break;
        }
        if ((u8)getNetworkBinaryState(getInstance_()) == 0) {
            flag_1D = 4;
            break;
        }
        event_flags = 0;
        sendReqShut(getInstance_(), 1);
        flag_1D++;
        break;
    case 2:
        if ((event_flags & 1) || (event_flags & 2) || (event_flags & 0x10)) {
            event_flags = 0;
            resetNetworkState3(getInstance_());
            flag_1D++;
        }
        break;
    case 3:
        if (event_flags & 8) {
            flag_1D++;
        }
        break;
    case 4:
        resetCallback(getInstance_(), 1);
        decrement60d4(getInstance_());
        flag_1D++;
        break;
    case 5:
        result = 1;
        if (library_started != 0) {
            result = ((sNetworkLibrary*)getNetworkLogger())->stop();
        }
        if (result > 0) {
            notifyPatEvent(getInstance_(), 2, 0, NULL);
            library_started = 0;
            flag_1D = 0;
        }
        break;
    }
}

/* Folds a Pat interface event into the event flags the opening and closing steps poll. */
void NetworkWiiMediator::applyEvent(s32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg)
{
    switch (code) {
    case 0x8000:
    case 0x8007:
        event_flags |= 1;
        break;
    case 0x8002:
        event_flags |= 2;
        break;
    case 0x8004:
        if (b != 0) {
            event_flags |= 1;
        }
        break;
    case 0x8005:
        event_flags |= 8;
        break;
    case 0x8006:
        event_flags |= 0x10;
        break;
    }
}
