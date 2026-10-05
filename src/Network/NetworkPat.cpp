/*
 * Network/NetworkPat.cpp - the four-slot Pat holder: its constructor, destructor and drive functions, then the "*Pat"
 *   accessor family (each slot +0x00/+0x04/+0x08/+0x0C has an install setter, an uninstall, a delete helper and, for
 *   three slots, a getter).  C++, every symbol unmangled (`extern "C"`); the types live in `Network/NetworkPat.h`.
 * RANGE. .text 0x80419AD4-0x8041A194 (19 functions, 1728 B); extab, extabindex.  Left seam: the holder block sits right
 *   after `sNetworkLibraryWii`'s last table slot (0x80419AD0) and `.data` order bounds the next TU start in
 *   0x80418A60..0x8041A1C4.  Right seam: the map's `clearNetworkLayerPat` starts at 0x8041A170 (+0x24), so the unit
 *   ends at 0x8041A194.  Both are weak as code cuts (`tudiscover at getNetworkLayerPat`: share 0.075).
 * FLAGS. `-O3 -inline noauto` (configure.py; measured in docs/network.md); the lib's `-Cpp_exceptions on`
 *   gives the target's `extab` and `extabindex`.
 * NAMES. The three getters (`getNetworkSessionManagerPat` +0x00, `getNetworkCommunityPat` +0x08, `getNetworkLayerPat`
 *   +0x0C) are the runtime dump's.  The helpers are GUESSes from the slot and the action (`setNetworkCommunityPat`/
 *   `clearNetworkCommunityPat`/`deleteNetworkCommunityPat`, the same scheme for the `layer`/`sessionManager` families);
 *   slot +0x04 has no getter, so its helpers are `clearNetworkPatSlot04`/`deleteNetworkPatSlot04`.
 * RESIDUALS. none.
 * SHAPES. The slots are typed from the install site `initNetworkPatControl` (0x804294A0..0x8042951C), which stores what
 *   each class constructor returns: +0x00 `NetworkSessionManagerPat`, +0x08 `NetworkCommunityPat`, +0x0C
 *   `NetworkLayerPat` (root `__vt__12NetworkLayer` 0x805FB5D0).  Slot +0x04 is never installed anywhere in the DOL, so
 *   it stays an untyped handle and its delete/clear pair always sees NULL.
 *  - the three `deleteNetwork*Pat` helpers call the element's release hook through vtable +0x14, clear the slot through
 *    the sibling `clearNetwork*Pat`, then run its deleting destructor at +0x08; `NetworkPatSlotProtocol` only declares
 *    those two virtuals, so no table of ours is emitted (rule 10);
 *  - each `deleteNetwork*Pat` is wrapped in its own `#pragma peephole off`/`on` pair (retail reloads the vptr through
 *    r3, `mr r3,r31; lwz r12,0(r3)`; `-opt nopeephole` is accepted and changes nothing, playbook 39/41); one region
 *    over all three would also cover the setters/getters between them;
 *  - definitions in address order (`flipcheck` refuses the unit otherwise).
 */
#include "Network/NetworkPat.h"
#include "Network/sNetworkLibraryWii.h"   /* sNetworkLibrary, constructNetworkWiiMediator, registerNetworkObject */
#include "Network/NetworkWiiMediator.h"   /* NetworkMediator - the interface the per-frame update dispatches through */
#include "Network/PatInterface.h"         /* getInstance_, stepPatInterface */
#include "unsplit/Network.h"              /* getNetworkLogger */
#include "sound/fn_800E46E8.h"            /* getInstance - the mediator accessor */
#include "Runtime.PPCEABI.H/memset.h"
#include "sys_mem.h"
#include "MSL_C/alloc.h"                  /* srand */

/* The slot calling convention, as a view: only the two slots the `deleteNetwork*Pat` helpers call are
 * named, and no slot is defined here - the element's own table is emitted by its class's band.  The
 * +0x18 driver `updateNetworkPat` dispatches is reached through `NetworkPatSlotDriver` below (the moved block's
 * own view, kept as it was measured); a declared virtual at index i sits at +8+4*i. */
class NetworkPatSlotProtocol {
public:
    /* +0x08 */ virtual void destroy_08(u32 flags);
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void release_14();
};   /* size: 0x04 - the element's own leading vtable word; only ever reached through a pointer */

/* The protocol every Pat holder slot's element follows (its own band emits the table): the deleting
 * destructor at +0x08, the release hook at +0x14 and the per-frame drive at +0x18.  Declared, never
 * defined, so no table is emitted here (rule 10). */
class NetworkPatSlotDriver {
public:
    /* +0x08 */ virtual ~NetworkPatSlotDriver();
    /* +0x0C */ virtual void pad_0C();
    /* +0x10 */ virtual void pad_10();
    /* +0x14 */ virtual void release();
    /* +0x18 */ virtual void drive();
};   /* size: 0x04 (the object's leading table word) */

#pragma peephole off

NetworkPat* constructNetworkPat(NetworkPat* holder)
{
    sNetworkPatInstance = holder;
    holder->installed_10 = -1;
    registerNetworkObject(holder, 1);
    memset(&holder->sessionManager_00, 0, sizeof(holder->sessionManager_00));
    memset(&holder->slot04_04, 0, sizeof(holder->slot04_04));
    memset(&holder->community_08, 0, sizeof(holder->community_08));
    memset(&holder->layer_0C, 0, sizeof(holder->layer_0C));
    return holder;
}

NetworkPat* destroyNetworkPat(NetworkPat* holder, s16 flags)
{
    if (holder != NULL) {
        clearNetworkPat(holder);
        sNetworkPatInstance = NULL;
        if (flags > 0) {
            operator delete(holder);
        }
    }
    return holder;
}

s32 initNetworkLibrary(NetworkPat* holder, sNetworkLibraryInitParam* param)
{
    constructNetworkWiiMediator();
    if (((sNetworkLibrary*)getNetworkLogger())->init(param) < 0) {
        return -1;
    }
    srand((u32)((sNetworkLibrary*)getNetworkLogger())->getTime(0));
    return 0;
}

void clearNetworkPat(NetworkPat* holder)
{
    if (holder->sessionManager_00 != NULL) {
        deleteNetworkSessionManagerPat(holder, 0);
    }
    if (holder->slot04_04 != NULL) {
        deleteNetworkPatSlot04(holder, 0);
    }
    if (holder->community_08 != NULL) {
        deleteNetworkCommunityPat(holder, 0);
    }
    if (holder->layer_0C != NULL) {
        deleteNetworkLayerPat(holder, 0);
    }
    if (getNetworkLogger() != NULL) {
        ((sNetworkLibrary*)getNetworkLogger())->final();
        delete (sNetworkLibrary*)getNetworkLogger();
    }
}

void updateNetworkPat(NetworkPat* holder)
{
    if (getNetworkLogger() != NULL) {
        ((sNetworkLibrary*)getNetworkLogger())->updateTime();
    }
    if (getInstance() != NULL) {
        ((NetworkMediator*)getInstance())->update();
    }
    if (getInstance_() != NULL) {
        stepPatInterface(getInstance_());
    }
    if ((holder->installed_10 & 1) && holder->sessionManager_00 != NULL) {
        ((NetworkPatSlotDriver*)holder->sessionManager_00)->drive();
    }
    if ((holder->installed_10 & 2) && holder->slot04_04 != NULL) {
        ((NetworkPatSlotDriver*)holder->slot04_04)->drive();
    }
    if ((holder->installed_10 & 4) && holder->community_08 != NULL) {
        ((NetworkPatSlotDriver*)holder->community_08)->drive();
    }
    if ((holder->installed_10 & 8) && holder->layer_0C != NULL) {
        ((NetworkPatSlotDriver*)holder->layer_0C)->drive();
    }
}

void deleteNetworkSessionManagerPat(NetworkPat* holder, s32 index)
{
    NetworkPatSlotDriver* entry;

    if (index == 0) {
        entry = (NetworkPatSlotDriver*)(&holder->sessionManager_00)[index];
        if (entry != NULL) {
            entry->release();
            clearNetworkSessionManagerPat(holder, (NetworkSessionManagerPat*)entry);
            delete entry;
        }
    }
}

s32 setNetworkSessionManagerPat(NetworkPat* holder, NetworkSessionManagerPat* value)
{
    if (holder->sessionManager_00 == NULL) {
        holder->sessionManager_00 = value;
        return 0;
    }
    return -1;
}

#pragma peephole on

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
