/*
 * include/Network/NetworkPat.h - the four-slot "*Pat" holder and the accessor family that walks it.
 *
 * Owner: `Network/NetworkPat.cpp` (`.text` 0x80419EC4..0x8041A194).  The record and the family moved
 * here with the `Network/network_pat_control.cpp` pass: a second unit needed the type, and a
 * declaration belongs with the unit that owns the symbol (docs/plan.md 6.5 rules 1 and 2).
 *
 * The holder's extent comes from this band's own evidence, not from the accessors: the constructor
 * `fn_80419AD4` (0x80419AD4, in the unclaimed range just below this unit's left seam) writes
 * `+0x10 = -1`, and the drive method `fn_80419CF0` reads that word as a four-bit per-slot enable mask
 * before dispatching each enabled slot at vtable `+0x18`.  The record is therefore **0x14** bytes,
 * four slot pointers plus the mask.
 *
 * Each slot holds a pointer to a polymorphic object of its **own, unrelated** class: the three
 * constructors (`__ct__24NetworkSessionManagerPatFv` 0x803D68D0, `fn_803E0C18`, `fn_803F02C4`) share
 * no base-ctor call, so this is a container, not a class hierarchy.  What the elements do share is the
 * calling convention this unit and the holder impose on a slot: the **deleting destructor at vtable
 * `+0x08`** (called with the delete flag) and the **release hook at `+0x14`** - the offsets the four
 * `deleteNetwork*` helpers read - plus the **`+0x18` driver** the holder's own dispatch calls.  The
 * element classes are only ever reached through a pointer from here, so they are declared, never
 * defined, in this header; their tables belong to the bands that construct them.
 */
#ifndef MHTRI_NETWORK_NETWORKPAT_H
#define MHTRI_NETWORK_NETWORKPAT_H

#include "types.h"

/* The class `Network/NetworkSessionManager.cpp` reconstructs (table `__vt__24NetworkSessionManagerPat`,
 * 0x805FB0F0); its declaration lives in that unit's header. */
class NetworkSessionManagerPat;

/* The slot +0x08 element.  Its constructor is `fn_803F02C4` (allocation 0x25EC, table 0x805FC728 over
 * the root table 0x805FC440) and the map names neither the class nor the table, so the name is
 * **GUESSED** from the accessor's own name `getNetworkCommunityPat` - the only name the binary gives
 * the slot. */
class NetworkCommunityPat;

/* The slot +0x0C element: a class derived from the root whose table the map names
 * `NetworkLayer_VTable` (0x805FB5D0).  The installed object is built by `fn_803E0C18`, which calls
 * `fn_803DF2EC` (the root, which stores 0x805FB5D0) first. */
class NetworkLayerPat;

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
