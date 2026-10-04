/*
 * include/Network/network_layer_io.h - the declarations `src/Network/network_layer_io.cpp` owns
 * (`.text` 0x804006A8..0x80413C64, the network layer's `sendReq*` request builders and their handlers).
 *
 * The unit has no bodies yet, so every parameter list is the one its callers' calls demonstrate.  Moved
 * here from the band header `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares).
 */
#ifndef MHTRI_NETWORK_NETWORK_LAYER_IO_H
#define MHTRI_NETWORK_NETWORK_LAYER_IO_H

#include "types.h"

typedef struct NetworkInstance NetworkInstance;   /* include/unsplit/Network.h */
typedef struct NetworkStateMachine NetworkStateMachine;   /* include/Network/network_state.h */
typedef struct PatCircleInfo PatCircleInfo;       /* include/Network/NetworkSessionManager.h */
typedef struct NetworkWiiMediatorFields NetworkWiiMediatorFields;   /* include/Network/NetworkWiiMediator.h */
class NetworkWiiMediator;                         /* include/Network/NetworkWiiMediator.h */
class NetworkReflectService;                      /* include/Network/NetworkReflectService.h */

/* The network library's linear-congruential random generator (`sNetworkLibrary::mpRandom`): the constructor
 * 0x80413384 stores the table 0x806024A0 and the classic `rand` constants (seed 1, multiplier 0x41C64E6D,
 * increment 12345, result shift 16, mask 0x7FFF); 0x80413424 steps it.  Class name GUESSED.  size: 0x18 */
class NetworkRandom {
public:
    NetworkRandom();
    /* +0x08 */ virtual ~NetworkRandom();

    /* +0x04 */ u32 seed;
    /* +0x08 */ u32 multiplier;
    /* +0x0C */ u32 increment;
    /* +0x10 */ u32 shift;
    /* +0x14 */ u32 mask;
};

/* The 0x3A20-byte network singleton `getNetworkPool` returns (.sbss 0x80794CB8): the constructor 0x80412528
 * stores the table 0x80602490 and publishes itself, 0x8041275C clears its state (the +0x6B progress byte,
 * the +0x3A10 timestamp among it), and the mediator's forwarding wrappers create it with `new` (0x3A20).
 * Class name GUESSED from the runtime dump's `GetPool` on the accessor.  The virtual is declared and not
 * defined here, so no table is emitted by a consumer (rule 10).  size: 0x3A20 */
class NetworkPool {
public:
    /* +0x08 */ virtual ~NetworkPool();

    /* 0x804128C4 - resets the state and starts the EC (shop) sequence (GUESS name). */
    void start();
    /* 0x8041793C (`Network/network_opening.cpp`) - whether the EC sequence is running (+0x44). */
    BOOL isECStarted();

    /* +0x0004 */ u8  pad_0004[0x40];
    /* +0x0044 */ u8  ec_started;
    /* +0x0045 */ u8  pad_0045[0x26];
    /* +0x006B */ u8  progress;        /* the opening progress the mediator adds to 90 while step 6 runs */
    /* +0x006C */ u8  pad_006C[0x39A4];
    /* +0x3A10 */ u64 timestamp;       /* the server timestamp the mediator stamps the account with */
    /* +0x3A18 */ u8  pad_3A18[0x08];
};

#ifdef __cplusplus
extern "C" {
#endif

/* the channel requests the GameSpy band sends */
void sendReqChannelInfo(NetworkInstance* self, u32 handle);
void sendReqChannelData(NetworkInstance* self, u32 handle, u32 offset, u32 size);
void sendReqConnect(NetworkInstance* self);

/* The layer requests: each writes its op-code and returns the request id (the callee narrows it to 16
 * bits, but its caller stores the full register, so the declared type is the wide one - playbook 66). */
u32 sendReqLayerUp(NetworkInstance* self);
u32 sendReqLayerChildInfo(NetworkInstance* self, s16 layer_id, u32 unused_arg);
u32 sendReqLayerUserList(NetworkInstance* self);

/* 0x80413AEC - whether the server is in maintenance (the status word reads 1). */
s32 isMaintenanceMode(NetworkWiiMediator* self);

/* the circle-info request the Pat session manager's `move` sends (the block is `PatCircleInfo`) */
void sendReqCircleInfoSet(NetworkInstance* instance, u32 request_id, PatCircleInfo* info, const char* name);

/* The request emitters the session state machine (`Network/network_state.cpp`) drives. */
void sendReqAuthenticationToken(NetworkInstance* self, const char* token);
void sendReqMaintenance(NetworkInstance* self);
void sendReqTermsVersion(NetworkInstance* self);
void sendReqTerms(NetworkInstance* self, u32 a, u32 b, u32 len);
void sendReqAnnounce(NetworkInstance* self);
void sendReqNoCharge(NetworkInstance* self);
void sendReqVulgarityInfoLow(NetworkInstance* self, s32 mode);
void sendReqVulgarityLow(NetworkInstance* self, s32 mode, u32 slice, u32 len);
void sendReqLmpConnect(NetworkInstance* self);
void sendReqMediaVersionInfo(NetworkInstance* self);
void sendReqRfpConnect(NetworkInstance* self);
void sendReqFmpListVersion(NetworkInstance* self);
void sendReqFmpListHead(NetworkInstance* self, s32 a, s32 b);
void sendReqFmpListData(NetworkInstance* self, u32 start, u32 count);
void sendReqFmpListFoot(NetworkInstance* self);
void sendReqFmpInfo(NetworkInstance* self, u32 value, s32 flag);
void sendReqBinaryHead(NetworkInstance* self, s32 a, s32 b);
void sendReqBinaryData(NetworkInstance* self, s32 a, u32 size, s32 flag);
void sendReqBinaryFoot(NetworkInstance* self, s32 a);
void sendReqCircleInfoNoticeSet(NetworkInstance* self);
void reqUserSearchInfoMine(NetworkInstance* self, s32 mode);

/* The item writers and the hand-off dispatch the state machine calls with its own view of the session. */
void writeUInt8Array2(NetworkStateMachine* self, u8 count, const u8* data);
void putItemAny(NetworkStateMachine* self, const u8* values, u32 count, const u8* tags);
void putItemTaggedLongs(NetworkStateMachine* self, const u32* values, u8 count, const u8* tags);
void putItemTaggedBytes(NetworkStateMachine* self, const u8* values, u8 count, const u8* tags);
void putUserSlotObjects(NetworkStateMachine* self, const u8* row, u8 count, const u8* tags);
void dispatchSessionHandlers(NetworkStateMachine* self, u32 code, s32 a, s32 b, u32 count, const u8* data);

/* The singletons the mediator band forwards to and the language/event queries it reads. */
NetworkReflectService* getReflectService(void);
NetworkPool* getNetworkPool(void);
/* 0x80413450 - the Pat interface callback the mediator installs (slot 1): forwards the event to the
 * mediator passed as the callback argument (GUESS name). */
void mediatorEventCallback(u32 code, s32 a, s32 b, s32 c, const union GameSpyEventMsg* msg,
                           NetworkWiiMediator* mediator);
s32 getLanguage(void);
s32 getReflectEventId(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_LAYER_IO_H */
