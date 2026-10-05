/*
 * Network/NetworkPat.h - the four-slot "*Pat" holder and its accessor family (owner `Network/NetworkPat.cpp`).
 * SHAPES. The record is 0x14 B: `constructNetworkPat` writes `+0x10 = -1` and `updateNetworkPat` reads it as the
 *   per-slot enable mask before calling each slot's `+0x18` driver.  The slots hold unrelated classes (their
 *   constructors share no base call) that share the deleting destructor at vtable `+0x08` and the release hook at
 *   `+0x14`; they are declared, never defined, here (their tables belong to the bands that construct them).
 */
#ifndef MHTRI_NETWORK_NETWORKPAT_H
#define MHTRI_NETWORK_NETWORKPAT_H

#include "types.h"

/* The slot +0x00 element; its table `__vt__24NetworkSessionManagerPat` (0x805FB0F0) is emitted by
 * `Network/NetworkSessionManagerPat.cpp` (the key function `move`), the class is declared in
 * `Network/NetworkSessionManager.h`. */
class NetworkSessionManagerPat;

/* The slot +0x08 element, named by its own "NetworkCommunityPat::deleteRequest" string: constructor
 * `__ct__19NetworkCommunityPatFv` (allocation 0x25EC), table 0x805FC728 over the root table 0x805FC440. */
class NetworkCommunityPat;

/* The slot +0x0C element: a class derived from the root whose table the map names
 * `__vt__12NetworkLayer` (0x805FB5D0).  The installed object is built by `__ct__15NetworkLayerPatFv`, which calls
 * `NetworkLayer::NetworkLayer` (the root, which stores 0x805FB5D0) first. */
class NetworkLayerPat;

/* `sNetworkLibrary::init`'s parameter block (`Network/sNetworkLibrary.h`). */
struct sNetworkLibraryInitParam;

typedef struct NetworkPat {
    /* +0x00 */ NetworkSessionManagerPat* sessionManager_00;
    /* +0x04 */ void* slot04_04;   /* never installed in this build - see `clearNetworkPatSlot04` */
    /* +0x08 */ NetworkCommunityPat* community_08;
    /* +0x0C */ NetworkLayerPat* layer_0C;
    /* +0x10 */ u32 installed_10;  /* ctor sets -1; bits 0..3 enable slots 0..3 for the +0x18 dispatch */
} NetworkPat;   /* size: 0x14 */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80419AD4 / 0x80419B54 - the Pat holder's constructor (publishes it as `sNetworkPatInstance`, clears the
 * four slots, enables all of them) and its deleting destructor. */
struct NetworkPat* constructNetworkPat(struct NetworkPat* holder);
struct NetworkPat* destroyNetworkPat(struct NetworkPat* holder, s16 flags);

/* 0x80419BB4 - creates the library, runs its `init` and seeds the C random generator from its clock. */
s32 initNetworkLibrary(struct NetworkPat* holder, struct sNetworkLibraryInitParam* param);

/* 0x80419C2C / 0x80419E1C - the Pat holder reset and the session-manager slot delete. */
void clearNetworkPat(struct NetworkPat* holder);

/* 0x80419CF0 - the per-frame drive: the library clock, the mediator, the Pat interface, then every enabled slot. */
void updateNetworkPat(struct NetworkPat* holder);

void deleteNetworkSessionManagerPat(struct NetworkPat* holder, s32 index);

/* 0x80419EA4 - installs the session-manager slot when it is empty (0), else -1. */
s32 setNetworkSessionManagerPat(struct NetworkPat* holder, NetworkSessionManagerPat* value);

/* Getters.  `index` is signed (`cmpwi r4,0`) and only 0 is accepted. */
NetworkSessionManagerPat* getNetworkSessionManagerPat(NetworkPat* self, s32 index);
NetworkCommunityPat* getNetworkCommunityPat(NetworkPat* self, s32 index);
NetworkLayerPat* getNetworkLayerPat(NetworkPat* self, s32 index);

/* Installers: take the slot only when it is empty. */
s32 setNetworkCommunityPat(NetworkPat* self, NetworkCommunityPat* value);
s32 setNetworkLayerPat(NetworkPat* self, NetworkLayerPat* value);

/* Uninstallers: clear the slot only when `value` is the entry it holds, else -1. */
s32 clearNetworkSessionManagerPat(NetworkPat* self, NetworkSessionManagerPat* value);
s32 clearNetworkCommunityPat(NetworkPat* self, NetworkCommunityPat* value);
s32 clearNetworkLayerPat(NetworkPat* self, NetworkLayerPat* value);
/* untyped: opaque handle - slot +0x04 is never installed (the DOL has no setter for it) */
s32 clearNetworkPatSlot04(NetworkPat* self, void* value);

/* Delete helpers: release (+0x14), clear the slot, then the deleting destructor (+0x08) with flag 1. */
void deleteNetworkPatSlot04(NetworkPat* self, s32 index);
void deleteNetworkCommunityPat(NetworkPat* self, s32 index);
void deleteNetworkLayerPat(NetworkPat* self, s32 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NETWORK_NETWORKPAT_H */
