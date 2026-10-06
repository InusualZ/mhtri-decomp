/*
 * Network/NetworkSessionManagerPat.h - the free functions of `Network/NetworkSessionManagerPat.cpp`; the class is
 *   declared in `Network/NetworkSessionManager.h`.
 */
#ifndef MHTRI_NETWORK_NETWORKSESSIONMANAGERPAT_H
#define MHTRI_NETWORK_NETWORKSESSIONMANAGERPAT_H

#include "types.h"
#include "Network/NetworkPat.h"

class NetworkSessionManagerPat;

#ifdef __cplusplus
extern "C" {
#endif

/* The holder `getPatsObject` returns is `NetworkPat` (Network/NetworkPat.h).  The accessor is at 0x803DA020. */
NetworkPat* getPatsObject(void);
/* `isNetworkSessionManagerPatReady` (0x803DF1A8) is the session manager's readiness probe: it is handed the object slot
 * +0x00 holds, so its parameter is that object's class. */
BOOL isNetworkSessionManagerPatReady(NetworkSessionManagerPat* session_manager);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `unsplit/Network.h` (docs/plan.md 6.5 rule 2: the owner declares). */
class NetworkSessionManagerPat;    /* Network/NetworkSessionManager.h */
typedef struct NetLayerRequest NetLayerRequest; /* Network/NetworkLayerPat.h */
typedef struct NetworkRequest NetworkRequest;   /* Network/NetworkSessionManager.h */
typedef struct NetworkRequestError NetworkRequestError;   /* unsplit/Network.h */
typedef struct NetworkNameList NetworkNameList;           /* Network/NetworkSessionManager.h */
class NetworkBuffer;                                      /* Network/NetworkStreamSink.h */
typedef struct NetworkLayerId NetworkLayerId;             /* Network/NetworkLayerPat.h */
struct PatTerms;

#ifdef __cplusplus
extern "C" {
#endif

/* GUESS: 0x803DECF0 clears the session manager's busy byte and releases its buffers. */
void closeNetworkSessionManagerPat(NetworkSessionManagerPat* self);

/* The Pat band's helpers (moved here from `Network/NetworkSessionManager.h`, which defines the records
 * they take).  `buildCircleInfoName` was the map's `fn_803DE524` until the Pat pass renamed it from what
 * its body does (GUESS); the others are the map's own names. */
void buildCircleInfoName(NetworkSessionManagerPat* self, struct PatCircleOptionList* dst, NetworkNameList* src);
s32 circleAvailable(NetworkSessionManagerPat* self);
void networkPatResetCircleInfo(NetworkSessionManagerPat* self, s32 index);
/* The circle-list handlers (dump names): add, remove and update one circle entry from a received block. */
void createCircleLayer(NetworkSessionManagerPat* self, const struct PatCircleInfo* info, struct PatCircleOptionList* options);
void deleteCircleListLayer(NetworkSessionManagerPat* self, s32 id);
void changeCircleListLayer(NetworkSessionManagerPat* self, const struct PatCircleInfo* info, struct PatCircleOptionList* options);
void networkPatAttachBuffer(NetworkSessionManagerPat* self);
void networkPatReleaseBuffer(NetworkSessionManagerPat* self);
/* 0x803DEA30 - opens the NetworkSessionStable a layer session needs on the manager (its session pointer, ready flag
 * and work buffer, viewed as `NetworkSessionStableInit`); the session's index, or -1 when one is already open. */
struct NetworkSessionStableInit;
s32 initNetworkSessionStable(struct NetworkSessionStableInit* self);

/* the two reflection adapters the session manager installs as callbacks */
/* untyped: caller-owned payload - the six arguments are forwarded unchanged */
void networkSessionReflect0(void* a0, void* a1, s8 a2, void* a3, void* a4, void* a5);
/* the server message callback: the manager, the message code, the request id it answers, a flag, the
 * element count and the message bytes */
void networkSessionReflect1(NetworkSessionManagerPat* self, s32 code, s32 requestId, s32 flag, s32 count,
                            const u8* data);

/* 0x80793930 / 0x80793934 - the two `.sdata` floats `initNetworkSessionStable` copies into a new
 * session through the vtable's +0x54/+0x58 slots; 0x8079C764 the `.sdata2` float it publishes
 * through +0x60. */
extern f32 networkSessionTimeoutSeconds;
extern f32 networkSessionIntervalSeconds;
extern f32 networkSessionLimitSeconds;   /* 0x80793938 (GUESS: the third timeout the circle block can set, 20..100 s) */
extern f32 networkSessionPeriodSeconds;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKSESSIONMANAGERPAT_H */
