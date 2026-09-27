/*
 * Network/NetworkPat.cpp - the `sNetworkLibrary` "*Pat" accessor class
 * (`.text` 0x80419EC4..0x8041A170, 11 functions / 328 B).
 *
 * BOUNDARY.  Both seams are weak and **unproven**.  `tudiscover at getNetworkSessionManagerPat`
 * gives the closure as the function alone and its left candidates are far away (0x80418988 weak,
 * share 0.094); the right edge 0x8041A170 is the end of the third accessor's `+0xC` unset helper
 * (`tudiscover at getNetworkLayerPat`, share 0.075, right candidates 0x8041AD30 / 0x8041AEA8 weak).
 * The task's table proposed 0x80419EC4..0x8041A150, but the third accessor `getNetworkLayerPat`
 * starts *exactly* at 0x8041A150, so that edge would have orphaned it; the closure through the three
 * accessors is 0x8041A170, taken here.  Sections: `.text` 0x80419EC4..0x8041A170, `extab`
 * 0x8001CF70..0x8001CF88, `extabindex` 0x8003D92C..0x8003D950 (3 unwind records).
 *
 * WHAT IT IS.  A four-slot holder class: each slot +0x00 / +0x04 / +0x08 / +0x0C has an "install"
 * setter, an "uninstall" (`self->slot != value ? -1 : clear`), a "delete" helper and - for three of
 * the slots - a named getter (`getNetworkSessionManagerPat` +0x00, `getNetworkCommunityPat` +0x08,
 * `getNetworkLayerPat` +0x0C).  C++ but every symbol is unmangled (`extern "C"`).
 *
 * NAMES.  The three `*Pat` getters are the runtime dump's and already in `symbols.txt`.  The nine
 * helpers answer only `zz_...` in the dump, so this batch's naming pass named them from the slot each
 * operates on and what it does: `setNetworkCommunityPat` / `clearNetworkCommunityPat` /
 * `deleteNetworkCommunityPat` install, unset and delete slot +0x08 (the getter `getNetworkCommunityPat`
 * reads +0x08), and the `layer`/`sessionManager` families follow the same scheme at +0x0C / +0x00.
 * Slot +0x04 has no getter in the dump, so its three helpers are named after the offset:
 * `clearNetworkPatSlot04` / `deleteNetworkPatSlot04`.  `0x80419EA4` (the +0x00 installer,
 * 0x80419EA4) is the same class and lies just below the left seam - a residual of the boundary,
 * recorded rather than claimed.
 *
 * NAMING GUESS.  The `+0x04` slot's type is unproven (no getter in the runtime map), so
 * `clearNetworkPatSlot04` / `deleteNetworkPatSlot04` record the slot offset, not an invented type.
 *
 * Every symbol this file defines is named; the `0x80419EA4` above is a comment mention of the
 * unclaimed +0x00 installer just below the seam, not a reference this unit makes (rule 7 clean).
 *
 * BODIES.  The three getters and the four install/uninstall helpers are reconstructed from the
 * disassembly (all 100 %); the three `0x8C`-byte delete helpers are stubs.
 */
#include "types.h"

/* The holder the accessors walk: four `void*` slots at +0x00/+0x04/+0x08/+0x0C.  The three named
 * getters read slot +0x00 / +0x08 / +0x0C through the caller's `index`; the `+0x04` slot has no
 * getter (its installer `0x80419EA4` sits just below the unit's left seam). */
typedef struct NetworkPat {
    /* +0x00 */ void* sessionManager_00;
    /* +0x04 */ void* unused_04;
    /* +0x08 */ void* community_08;
    /* +0x0C */ void* layer_0C;
} NetworkPat;  /* size: 0x10 */

extern "C" {

/* getters */
void* getNetworkSessionManagerPat(NetworkPat* self, s32 index);
void* getNetworkCommunityPat(NetworkPat* self, s32 index);
void* getNetworkLayerPat(NetworkPat* self, s32 index);

/* install helpers */
s32 setNetworkCommunityPat(NetworkPat* self, void* value);
s32 setNetworkLayerPat(NetworkPat* self, void* value);

/* uninstall helpers */
s32 clearNetworkSessionManagerPat(NetworkPat* self, void* value);
s32 clearNetworkPatSlot04(NetworkPat* self, void* value);
s32 clearNetworkCommunityPat(NetworkPat* self, void* value);
s32 clearNetworkLayerPat(NetworkPat* self, void* value);

/* delete helpers (bodies are the next pass) */
void deleteNetworkPatSlot04(NetworkPat* self, s32 index);
void deleteNetworkCommunityPat(NetworkPat* self, s32 index);
void deleteNetworkLayerPat(NetworkPat* self, s32 index);

} /* extern "C" */

void* getNetworkSessionManagerPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        return (&self->sessionManager_00)[index];
    }
    return NULL;
}

void* getNetworkCommunityPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        return (&self->community_08)[index];
    }
    return NULL;
}

void* getNetworkLayerPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        return (&self->layer_0C)[index];
    }
    return NULL;
}

s32 setNetworkCommunityPat(NetworkPat* self, void* value)
{
    if (self->community_08 == NULL) {
        self->community_08 = value;
        return 0;
    }
    return -1;
}

s32 setNetworkLayerPat(NetworkPat* self, void* value)
{
    if (self->layer_0C == NULL) {
        self->layer_0C = value;
        return 0;
    }
    return -1;
}

s32 clearNetworkSessionManagerPat(NetworkPat* self, void* value)
{
    if (self->sessionManager_00 == value) {
        self->sessionManager_00 = NULL;
        return 0;
    }
    return -1;
}

s32 clearNetworkPatSlot04(NetworkPat* self, void* value)
{
    if (self->unused_04 == value) {
        self->unused_04 = NULL;
        return 0;
    }
    return -1;
}

s32 clearNetworkCommunityPat(NetworkPat* self, void* value)
{
    if (self->community_08 == value) {
        self->community_08 = NULL;
        return 0;
    }
    return -1;
}

s32 clearNetworkLayerPat(NetworkPat* self, void* value)
{
    if (self->layer_0C == value) {
        self->layer_0C = NULL;
        return 0;
    }
    return -1;
}

void deleteNetworkPatSlot04(NetworkPat* self, s32 index) { (void)self; (void)index; }
void deleteNetworkCommunityPat(NetworkPat* self, s32 index) { (void)self; (void)index; }
void deleteNetworkLayerPat(NetworkPat* self, s32 index) { (void)self; (void)index; }
