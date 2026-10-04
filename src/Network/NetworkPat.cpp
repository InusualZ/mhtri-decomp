/*
 * Network/NetworkPat.cpp - the `sNetworkLibrary` "*Pat" accessor family
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
 * WHAT IT IS.  The four-slot holder's accessor family: each slot +0x00 / +0x04 / +0x08 / +0x0C has an
 * "install" setter, an "uninstall" (`self->slot != value ? -1 : clear`), a "delete" helper and - for
 * three of the slots - a named getter (`getNetworkSessionManagerPat` +0x00, `getNetworkCommunityPat`
 * +0x08, `getNetworkLayerPat` +0x0C).  C++ but every symbol is unmangled (`extern "C"`).  The holder
 * type and the family's declarations live in `include/Network/NetworkPat.h` (this file's own header,
 * rule 2); the type moved there when the `network_pat_control` unit needed it (rule 1).
 *
 * TYPES.  The slots are typed from the install site `fn_804292F8` (0x804294A0..0x8042951C), which
 * stores the pointer each slot's own **class constructor** has just returned, and from the accessor's
 * own value dispatch: slot +0x00 is a `NetworkSessionManagerPat` (`__ct__24NetworkSessionManagerPatFv`
 * 0x803D68D0), +0x08 the object built by `fn_803F02C4`, +0x0C the `NetworkLayer`-derived object built
 * by `fn_803E0C18`.  Slot +0x04 is never installed - no setter and no getter for it exists anywhere in
 * the DOL (a whole-image scan for the setter shape finds +0x00/+0x08/+0x0C only) - so its value stays
 * an untyped handle and the `delete`/`clear` pair is dead code that always sees NULL.
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
 * NAMING GUESS.  `NetworkCommunityPat` is this batch's name for slot +0x08's class: the map carries no
 * mangled row, no `__vt__` and no ctor for it, and the only name the binary gives the slot is the
 * accessor `getNetworkCommunityPat`.  `NetworkLayer` (+0x0C) is the map's own table name
 * (`__vt__12NetworkLayer` 0x805FB5D0, formerly `NetworkLayer_VTable`), i.e. the root of that slot's hierarchy.
 *
 * SLOT PROTOCOL.  The three `deleteNetwork*Pat` helpers share one shape and it is the only thing the
 * four unrelated element classes have in common - not a shared base class.  With `index == 0` they
 * take the entry from the slot (`(&self->slot)[index]`), call its **release hook through vtable
 * `+0x14`**, clear the slot through the sibling `clearNetwork*Pat`, and - when the entry is still
 * non-NULL - run its **deleting destructor at `+0x08`** with the delete flag 1.  The element's own
 * table is emitted by its class's band, so `NetworkPatSlotProtocol` below only *declares* the two
 * virtuals those calls need: declaring virtuals without defining any emits no table of ours (rule 10),
 * and the cast to it is a reinterpretation of the same pointer, not a vtable we model.
 *
 * EXCEPTIONS.  The target object carries `extab` 24 B and `extabindex` 36 B and our object emits the
 * same two sections with no pragma at all, so this file needs none: the lib's flags already match
 * this range's front end (the earlier note here claimed the file turned the front end's exceptions
 * back on - it never did, and `datagap` is clean either way).
 *
 * PEEPHOLE.  The three `deleteNetwork*Pat` helpers need the peephole pass **off**, scoped to each
 * one by a `#pragma peephole off`/`on` pair: with it on MWCC copies the entry into r3 for the
 * vcall's `this` and then loads the vptr through the home register r31 (`lwz r12,0(r31)`), while the
 * target loads it back through r3 (`mr r3,r31; lwz r12,0(r3)`) - 99.71429 % on all three rows.  The
 * pass is the lever, not the flag (playbook 39/41): `-opt nopeephole` on the command line is
 * silently accepted and changes nothing, which is how this residual was first recorded as
 * unreachable from the source side.  With the pragma every row is 100 %.  The pairs are per-function -
 * the `on` half opens after each delete, so the setters/getters between them keep the pass (which is
 * how their own 100 % rows are measured); one region spanning the three deletes would put the pass off
 * over them as well.
 *
 * ORDER.  The definitions are laid out in address order (an object's `.text` follows the source's
 * definition order, not the map's addresses); `flipcheck` refuses the unit otherwise, even with
 * every row at 100 %.
 */
#include "Network/NetworkPat.h"

/* The slot calling convention, as a view: only the two slots the `deleteNetwork*Pat` helpers call are
 * named, and no slot is defined here - the element's own table is emitted by its class's band.  The
 * +0x18 driver the holder's `fn_80419CF0` dispatches is deliberately absent: this unit never reaches
 * it, and a declared virtual at index i sits at +8+4*i. */
class NetworkPatSlotProtocol {
public:
    /* +0x08 */ virtual void destroy_08(u32 flags);
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void release_14();
};   /* size: 0x04 - the element's own leading vtable word; only ever reached through a pointer */

NetworkSessionManagerPat* getNetworkSessionManagerPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        return (&self->sessionManager_00)[index];
    }
    return NULL;
}

s32 clearNetworkSessionManagerPat(NetworkPat* self, NetworkSessionManagerPat* value)
{
    if (self->sessionManager_00 == value) {
        self->sessionManager_00 = NULL;
        return 0;
    }
    return -1;
}

#pragma peephole off
void deleteNetworkPatSlot04(NetworkPat* self, s32 index)
{
    if (index == 0) {
        NetworkPatSlotProtocol* entry = (NetworkPatSlotProtocol*)(&self->slot04_04)[index];
        if (entry != NULL) {
            entry->release_14();
            clearNetworkPatSlot04(self, entry);
            if (entry != NULL) {
                entry->destroy_08(1);
            }
        }
    }
}
#pragma peephole on

/* untyped: opaque handle - slot +0x04 is never installed (the DOL has no setter for it) */
s32 clearNetworkPatSlot04(NetworkPat* self, void* value)
{
    if (self->slot04_04 == value) {
        self->slot04_04 = NULL;
        return 0;
    }
    return -1;
}

#pragma peephole off
void deleteNetworkCommunityPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        NetworkPatSlotProtocol* entry = (NetworkPatSlotProtocol*)(&self->community_08)[index];
        if (entry != NULL) {
            entry->release_14();
            clearNetworkCommunityPat(self, (NetworkCommunityPat*)entry);
            if (entry != NULL) {
                entry->destroy_08(1);
            }
        }
    }
}
#pragma peephole on

s32 setNetworkCommunityPat(NetworkPat* self, NetworkCommunityPat* value)
{
    if (self->community_08 == NULL) {
        self->community_08 = value;
        return 0;
    }
    return -1;
}

NetworkCommunityPat* getNetworkCommunityPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        return (&self->community_08)[index];
    }
    return NULL;
}

s32 clearNetworkCommunityPat(NetworkPat* self, NetworkCommunityPat* value)
{
    if (self->community_08 == value) {
        self->community_08 = NULL;
        return 0;
    }
    return -1;
}

#pragma peephole off
void deleteNetworkLayerPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        NetworkPatSlotProtocol* entry = (NetworkPatSlotProtocol*)(&self->layer_0C)[index];
        if (entry != NULL) {
            entry->release_14();
            clearNetworkLayerPat(self, (NetworkLayerPat*)entry);
            if (entry != NULL) {
                entry->destroy_08(1);
            }
        }
    }
}
#pragma peephole on

s32 setNetworkLayerPat(NetworkPat* self, NetworkLayerPat* value)
{
    if (self->layer_0C == NULL) {
        self->layer_0C = value;
        return 0;
    }
    return -1;
}

NetworkLayerPat* getNetworkLayerPat(NetworkPat* self, s32 index)
{
    if (index == 0) {
        return (&self->layer_0C)[index];
    }
    return NULL;
}

s32 clearNetworkLayerPat(NetworkPat* self, NetworkLayerPat* value)
{
    if (self->layer_0C == value) {
        self->layer_0C = NULL;
        return 0;
    }
    return -1;
}
