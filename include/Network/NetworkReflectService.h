/*
 * include/Network/NetworkReflectService.h - the declarations `src/Network/NetworkReflectService.cpp` owns
 * (`.text` 0x8041A194..0x8041A87C, the `NetworkReflectService` class's C-linkage entry points).
 *
 * The unit has no bodies yet, so every parameter list is the one its caller (the mediator band,
 * `Network/NetworkWiiMediator.cpp`) demonstrates.  Moved here from that caller (docs/plan.md 6.5 rule 2:
 * the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORKREFLECTSERVICE_H
#define MHTRI_NETWORK_NETWORKREFLECTSERVICE_H

#include "types.h"

/* the DWC event message `applyEvent` folds in (defined in `Network/GameSpyInterfaceThread.h`, beside its two views) */
union GameSpyEventMsg;

/* One entry of the service's channel table (and of the channel list a DWC event carries). */
typedef struct GameSpyChannel {
    /* +0x00 */ u8  ownerId_00;
    /* +0x01 */ u8  pad_01[0x03];
    /* +0x04 */ u32 peerId_04;
    /* +0x08 */ u8  address_08[0x1F];
    /* +0x27 */ u8  tail_27;
} GameSpyChannel;   /* size: 0x28 */

/* The reflect callback the mediator registers (`reflectInit` stores it at +0x78C and the service
 * calls it back with four words and two payload pointers - the `PFllllPvPv_v` half of
 * `reflectInit__18NetworkWiiMediatorFPFllllPvPv_vPv`). */
typedef void (*NetworkWiiMediatorReflectFn)(s32, s32, s32, s32, void*, void*);

/* The reflect service: the GameSpy search/connect state machine the mediator starts, stops and agrees
 * through.  One definition for both of its users (unified 2026-10-03 from the two compatible views the
 * mediator band and the GameSpy band carried; every offset and the size are unchanged):
 *   - the mediator band (`Network/NetworkWiiMediator.cpp`) dispatches into the table's first slot
 *     (retail's `lwz r12, 0(r3)` / `lwz r12, 8(r12)`) and allocates the object with `new` - 0x816C bytes,
 *     the allocation `reflectInit` makes - so the class carries the real virtual and the out-of-line
 *     constructor (the unit-level builder at 0x8041A1C4, `__ct__21NetworkReflectServiceFv`);
 *   - the GameSpy band (`Network/GameSpyInterfaceThread.cpp`) defines five of its members and addresses the fields
 *     below.
 * The virtual is declared and never defined in either user, so no vtable is emitted there (the table
 * belongs to the unit that defines `finalize`). */
class NetworkReflectService {
public:
    NetworkReflectService();
    /* +0x0000 - the vtable pointer; slot +0x08 is the finalizer `reflectFinal` calls with flag 1 */
    virtual void finalize(s32 flags);

    /* +0x0004 */ NetworkWiiMediatorReflectFn callback_04;
    /* +0x0008 */ void* callbackArg_08;
    /* +0x000C */ u32 flags_0C;
    /* +0x0010 */ s32 task_10;
    /* +0x0014 */ u8  searchStep_14;
    /* +0x0015 */ u8  connectStep_15;
    /* +0x0016 */ u8  startStep_16;
    /* +0x0017 */ u8  callbackStep_17;
    /* +0x0018 */ u8  channel_18;
    /* +0x0019 */ u8  pad_19[0x03];
    /* +0x001C */ u32 peerId_1C;
    /* +0x0020 */ u8  recvArea_20[0x8000];
    /* +0x8020 */ s32 channelCount_8020;
    /* +0x8024 */ GameSpyChannel channels_8024[8];
    /* +0x8164 */ u32 limit_8164;
    /* +0x8168 */ u32 writePos_8168;

    /* forwards a work record (code, three words, payload) to the callback pair at +0x04 / +0x08 */
    /* untyped: caller-owned payload - each work code carries its own record */
    void notify(u32 code, s32 a, s32 b, s32 c, void* data);
    /* one step of the connect-attempt callback sub-machine */
    void updateCallbackStep();
    /* dispatches to the search (task 1) or the connect (task 2) sub-machine */
    s32 dispatchTask();
    /* one step of the GameSpy search sub-machine */
    s32 runSearch();
    /* one step of the GameSpy NAT/connect sub-machine */
    s32 runConnect();
    /* applies a DWC event, then folds the event bits into the state machine's flags */
    void applyEvent(u32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg);
};   /* size: 0x816C (the allocation `reflectInit` makes; the last field the GameSpy band addresses is +0x8168) */


#ifdef __cplusplus
extern "C" {
#endif

void initReflectService(NetworkReflectService* service, NetworkWiiMediatorReflectFn callback,
                        void* arg);   /* untyped: the callback's user payload, caller-owned */
void finalizeReflectService(NetworkReflectService* service);
void setReflectServicePage(NetworkReflectService* service, u8 page);
void reflectServiceStart(NetworkReflectService* service);
void reflectServiceStop(NetworkReflectService* service);
void reflectServiceAgree(NetworkReflectService* service);

/* 0x8041A194 - the Pat interface's event callback slot 7 (installed by the start sequence): hands the DWC
 * event to `applyEvent` on the service passed as the callback argument. */
void reflectServiceEventCallback(u32 code, s32 a, s32 b, s32 c, const GameSpyEventMsg* msg,
                                 NetworkReflectService* service);
/* 0x8041A354 / 0x8041A368 - clear the callback pair and the start flag, then the task and step state. */
void resetReflectService(NetworkReflectService* service);
void resetReflectServiceTask(NetworkReflectService* service);
/* 0x8041A3E4 - the per-frame step: the callback sub-machine, else the start sequence, else the task. */
void updateReflectService(NetworkReflectService* service);
/* 0x8041A5D0 - one step of the start sequence (library start, Pat interface open, server choice). */
void stepReflectServiceStart(NetworkReflectService* service);

/* 0x80794CD8 - the live service (the constructor publishes it, `getReflectService` reads it). */
extern NetworkReflectService* sNetworkReflectService;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKREFLECTSERVICE_H */
