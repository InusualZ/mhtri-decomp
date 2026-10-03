/* Declarations owned by `src/Network/network_opening.*` (docs/plan.md 6.5 rule 2): a consumer includes this header instead of declaring the symbols itself. */
#ifndef MHTRI_NETWORK_NETWORK_OPENING_H
#define MHTRI_NETWORK_NETWORK_OPENING_H

#include "types.h"
#include "Network/network_transport.h"

/* Declarations moved here from `include/unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
struct PatTerms;
class NetworkLogger;          /* include/unsplit/Network.h */
class NetworkWiiMediator;     /* include/Network/NetworkWiiMediator.h */
typedef struct NetworkInstance NetworkInstance;   /* include/unsplit/Network.h */

#ifdef __cplusplus
extern "C" {
#endif

u32 isTermsUpdateFinished(struct PatTerms* terms);

/* The socket pool the transport peers register their socket with (0x804187F0 / 0x80418864).  Both
 * bodies walk a four-entry table at the manager's +0x04 and take the socket from the manager's own +0x8C
 * slot, so the argument is the manager the transport band reaches through `getNetworkLogger` (GUESS on
 * both names: they carry the acquire / release roles the two call sites give them, nothing in the range
 * spells them). */
NetworkSocketHandle* networkSocketPool_acquire(NetworkLogger* pool);
s32 networkSocketPool_release(NetworkLogger* pool, NetworkSocketHandle* socket);

void getGameInfo2d1c(NetworkWiiMediator* self, u32* out);

/* 0x80416890 / 0x8041690C - start the terms check / the terms update on the mediator singleton. */
void startTermsCheck(NetworkWiiMediator* self);
void startTermsUpdate(NetworkWiiMediator* self);

/* The NAS login token and the user id/password paths the session state machine hands the opening
 * (`Network/network_state.cpp` passes the mediator singleton). */
u32 getNASToken(NetworkInstance* instance);
void setConnectionPaths(NetworkInstance* connection, const char* userId, const char* password);

/* The network log context the Pat session releases on shutdown. */
/* untyped: opaque handle passed through - the context's layout belongs to the Pat band */
void networkLog_destroyContext(NetworkSessionManagerLogger* log, void* context);

/* 0x80794CC0 - the `.sbss` mediator singleton slot the `constructNetworkWiiMediator` constructor
 * publishes into. */
extern void* sNetworkWiiMediatorInstance;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORK_OPENING_H */
