/* Declarations owned by `src/Network/network_opening.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself.
 * The base library class `sNetworkLibrary` is its own unit's (`Network/sNetworkLibrary.h`, included below). */
#ifndef MHTRI_NETWORK_NETWORK_OPENING_H
#define MHTRI_NETWORK_NETWORK_OPENING_H

#include "types.h"
#include "Network/network_transport.h"
#include "Network/sNetworkLibrary.h"   /* sNetworkLibrary - the base library class, its own unit */

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct PatTerms;
class NetworkWiiMediator;     /* include/Network/NetworkWiiMediator.h */
typedef struct NetworkInstance NetworkInstance;   /* include/unsplit/Network.h */

#ifdef __cplusplus
extern "C" {
#endif

u32 isTermsUpdateFinished(struct PatTerms* terms);

/* 0x804166E0 / 0x80416620 - copy the mediator's nine DWC game-info words out / in (strings are copied
 * into the mediator's own buffers on the way in). */
void getGameInfo2d1c(NetworkWiiMediator* self, u32* out);
void setGameInfo2d1c(NetworkWiiMediator* self, u32* info);

/* 0x80416890 / 0x8041690C - start the terms check / the terms update on the mediator singleton. */
void startTermsCheck(NetworkWiiMediator* self);
void startTermsUpdate(NetworkWiiMediator* self);

/* The mediator's terms and transfer-state entry points the network pat control drives (GUESS names from the
 * bodies; the fields are the mediator's +0x6DD0 mode byte, the +0x6DD1/+0x6DD2 flag bytes and the +0x6DD4
 * level float):
 *  - 0x80416800 hands the terms object its buffer, resets the per-slot state and sets mode 1, flags 0 and the
 *    default level;
 *  - 0x804168C0 the terms object's status (0 when there is none);
 *  - 0x8041696C stores the transfer mode, resetting the four slots when it changes;
 *  - 0x804172CC / 0x804172DC / 0x804172EC store the +0x6DD1 flag, the +0x6DD2 flag and the level;
 *  - 0x80415FAC posts the community profile's record through the opening step (modes 0 and 2), nonzero
 *    when either accepted it. */
/* untyped: byte range - the MEM2 buffer handed to the terms object */
void initMediatorTerms(NetworkWiiMediator* self, void* buffer, u32 size);
s32 getMediatorTermsStatus(NetworkWiiMediator* self);
void setMediatorTransferMode(NetworkWiiMediator* self, u32 mode);
/* 0x804169D4 - 0 while the transfer mode is 0 or no terms object exists, else `isTermsUpdateFinished` on it
 * (GUESS name). */
s32 isMediatorTermsUpdateFinished(NetworkWiiMediator* self);
void setMediatorTransferFlag6DD1(NetworkWiiMediator* self, u8 flag);
void setMediatorTransferFlag6DD2(NetworkWiiMediator* self, u8 flag);
void setMediatorTransferLevel(NetworkWiiMediator* self, f32 level);
s32 postMediatorRecord(NetworkWiiMediator* self, u8* record);

/* The NAS login token and the user id/password paths the session state machine hands the opening
 * (`Network/network_state.cpp` passes the mediator singleton). */
char* getNASToken(NetworkWiiMediator* self);
void setConnectionPaths(NetworkInstance* connection, const char* userId, const char* password);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_OPENING_H */
