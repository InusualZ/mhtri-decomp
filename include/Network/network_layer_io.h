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
void sendReqAuthenticationToken(NetworkInstance* self, u32 token);
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
NetworkWiiMediatorFields* getNetworkWiiMediator(void);
s32 getLanguage(void);
s32 getReflectEventId(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_LAYER_IO_H */
