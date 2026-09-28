/*
 * Network/NetworkPat.cpp - the `sNetworkLibrary` "*Pat" accessor class
 * (`.text` 0x80419EC4..0x8041A194, 12 functions / 720 B).
 *
 * BOUNDARY.  Both seams are weak and **unproven** as *code* cuts, but the right edge is now settled
 * from the map instead: `symbols.txt` has `getNetworkLayerPat` 0x8041A150 (+0x20) and then
 * `clearNetworkLayerPat` at *exactly* 0x8041A170 (+0x24), so 0x8041A170 was that function's
 * **start**, not its end - the pre-fix claim orphaned it into `auto_03_8041A170_text`, where it
 * scored nothing.  The right edge is therefore 0x8041A194 (the map's `fn_8041A194`, a different TU's
 * function) and the unit now holds all twelve rows the accessor pair owns, the new one at 100 %.
 * `tudiscover at getNetworkSessionManagerPat` still gives the closure as the function alone and its
 * left candidates are far away (0x80418988 weak, share 0.094); `tudiscover at getNetworkLayerPat`
 * reports share 0.075 with right candidates 0x8041AD30 / 0x8041AEA8.  Sections: `.text`
 * 0x80419EC4..0x8041A194, `extab` 0x8001CF70..0x8001CF88, `extabindex` 0x8003D92C..0x8003D950 (3
 * unwind records, byte-identical to ours under the exceptions pragma below).
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
 * EXCEPTIONS.  The lib is built with `-Cpp_exceptions off` while this range's target object carries
 * `extab` 24 B and `extabindex` 36 B, so the file turns the front-end's exceptions back on
 * (playbook 30): without the pragma our object emits neither section and `datagap` reports both as
 * `target-extra`; with it both sizes match the target exactly and no `.text` byte moves.
 *
 * RESIDUAL.  The three `deleteNetwork*Pat` helpers are 99.71429 %: the target loads the entry's
 * vptr through r3, the register it was just moved into for the call (`mr r3,r31; lwz r12,0(r3)`),
 * where MWCC 1.3/1.5/1.6/1.7 and GC 3.0a3/a5 all load it through the home register r31.  Ruled out
 * in turn: the source shape (a pointer local, a reference, `(*p).m()`, a cast at the call site, an
 * untyped local, an explicit vtable-struct view, a dead copy - all give r31), the compiler version
 * (ten releases measure identically) and `-opt nopeephole`/`noscheduling`/`nofusion`.  Nothing else
 * in the range differs.
 *
 * BODIES.  All twelve functions are reconstructed from the disassembly.  The three `0x8C`-byte
 * `deleteNetwork*Pat` helpers share one shape: with `index == 0`, take the entry from the slot
 * (`(&self->slot)[index]`), call its release hook through the vtable (+0x14), clear the slot through
 * the sibling `clearNetwork*Pat`, and - when the entry is still non-NULL - run its deleting
 * destructor (+0x08) with the delete flag 1.  The entry's own vtable lives in another TU's `.data`,
 * so `NetworkPatEntry` below only *declares* the four virtuals whose offsets the calls need: declaring
 * virtuals without defining any emits no table of ours (rule 10).
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

/* The polymorphic base every installed entry shares: only two of its slots are ever called from this
 * unit - the +0x14 release hook and the +0x08 deleting destructor the `deleteNetwork*Pat` helpers
 * run - and its table is emitted by the class's own TU, not this one.  Declaring the virtuals and
 * defining none is what keeps MWCC from emitting a table here (rule 10); the two unnamed slots
 * between them hold the entry's own entry points, so they are padding in this view. */
class NetworkPatEntry {
public:
    /* +0x08 */ virtual void destroy_08(u32 flags);
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void release_14();
};   /* size: 0x04 (the entry's leading vtable word) */

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

/* delete helpers */
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

void deleteNetworkPatSlot04(NetworkPat* self, s32 index)
{
    if (index == 0) {
        NetworkPatEntry* entry = (NetworkPatEntry*)(&self->unused_04)[index];
        if (entry != NULL) {
            entry->release_14();
            clearNetworkPatSlot04(self, entry);
            if (entry != NULL) {
                entry->destroy_08(1);
            }
        }
    }
}

void deleteNetworkCommunityPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        NetworkPatEntry* entry = (NetworkPatEntry*)(&self->community_08)[index];
        if (entry != NULL) {
            entry->release_14();
            clearNetworkCommunityPat(self, entry);
            if (entry != NULL) {
                entry->destroy_08(1);
            }
        }
    }
}

void deleteNetworkLayerPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        NetworkPatEntry* entry = (NetworkPatEntry*)(&self->layer_0C)[index];
        if (entry != NULL) {
            entry->release_14();
            clearNetworkLayerPat(self, entry);
            if (entry != NULL) {
                entry->destroy_08(1);
            }
        }
    }
}
