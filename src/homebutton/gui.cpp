/*
 * homebutton/gui.cpp - the homebutton::gui band at the tail of the DOL .text.
 *
 * .text 0x80569DAC-0x8056BBF0 (52 functions, 7748 B).  This is one maximal unclaimed run
 * (docs/plan.md 12): the left seam is unproven, the right neighbour is `tiHKBManager.cpp`
 * (0x8056BBF0, `main` lib).  No `__FILE__` string is emitted anywhere in the range (checked with
 * tools/symbols/dumpmap.py: no `_<fnaddr>s_<file>_<addr>` local symbol has a `<fnaddr>` inside the
 * range), so evidence class 1 does not apply here.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `grep -n "fn_80569\|fn_8056A\|fn_8056B" config/RMHE08/symbols.txt` - every defined name is a bare
 * `.text` entry, and tools/symbols/dumpmap.py lookup over the whole inventory answers only `FUN_`/
 * `zz_` placeholders or unrelated J3D/GX names for most of it).
 *
 * Name evidence (docs/plan.md 12, evidence class 2): the shared runtime dump names six functions in
 * the range with the real spellings `homebutton::gui::drawLine_`, `homebutton::gui::Manager::~Manager`,
 * `homebutton::gui::Manager::getComponent`, `homebutton::gui::PaneManager::getPaneComponentByPane`
 * and `homebutton::gui::PaneComponent::contain`; two of them were verified against the code
 * (0x8056A478 emits two GX_LINES vertices with the line width and a colour word, and 0x8056A888 is a
 * deleting destructor over a list built by fn_80501C60/fn_80501BF4).  config/RMHE08/hbm_data/symbols.txt
 * (the HomeButton RSO export table) carries the same class hierarchy - homebutton::FrameController,
 * homebutton::GroupAnmController, homebutton::gui::{Component, Manager, Interface, PaneManager,
 * PaneComponent, EventHandler} - which fixes the module as `homebutton`.  The DOL owns no
 * `homebutton` lib, so the unit takes the `main` lib and cflags_main, the lib its link neighbour
 * `tiHKBManager.cpp` uses; the file is `homebutton/gui.cpp` (the namespace is `gui`).
 *
 * Sections claimed: .text 0x80569DAC-0x8056BBF0 only.  The unit's own `.data` vtables live at
 * lbl_80658290/lbl_806582FC/lbl_80658358/lbl_806583A0, outside this proposal's range (a second
 * measured pass, playbook 23/29); they are referenced here through the object's vtable pointer
 * (rule 10), never re-emitted by a `virtual` declaration.
 *
 * Reconstruction status: 34 of the 52 functions have a body; 27 measure >= 80 % (16 byte-identical).
 * Measured with `tools/units/recompile.py homebutton/gui.cpp --measure <sym>` against the split
 * object; the per-symbol numbers are in the outbox .pi/outbox/80569dac-fn-80569dac-9631.json.
 *
 * Residuals (the functions with a body below the bar, and the reason):
 *   - fn_8056A85C / fn_8056A870 59.0 % - the counter clear; the target reuses the index register for
 *     the zero (`slwi r0,r4,1; li r4,0; add; sth r4`) where MWCC emits a fresh register (`li r5,0`)
 *     before the shift.  Four spellings tried (`= 0`, via a `u16*`, via a local); all 59.0.
 *   - fn_8056A888 75.3 % - the Manager destructor; the null check is hoisted above the register
 *     saves in the target and the vtable address materialises into r5 (r4 holds the list-walk 0).
 *   - fn_80569DAC 73.2 %, fn_8056A638 74.5 %, fn_8056ADB8 78.7 %, fn_8056B0AC 79.9 %,
 *     fn_8056AA6C/AA98 79.8 %, fn_8056B6D0 76.0 % - sizes exact; the shortfall is which GPR the
 *     vtable pointer is materialised into (`lwz r12,0(self)` vs a temp) and the prologue scheduling
 *     of the incoming argument saves, not the logic.
 *   - fn_8056AE78 has a body but is unmeasurable until the orchestrator re-splits: MAIN's fallback
 *     split object spells that address with a different symbol, so the report cannot pair it
 *     (`recompile.py --measure` reports "no pairing" rather than a score).
 *
 * Not attempted yet (18): the three `addi r3,r3,-4; b <victim>` this-adjusting thunks
 * (fn_80569E4C/E54/E5C - a `-4` needs pointer arithmetic, rule 6), the two GX immediate-mode
 * quad/line drawers (fn_80569E64/fn_8056A1D8/fn_8056A478), the component dispatchers
 * (fn_8056AAC4/fn_8056AD7C), the layout-scene walkers (fn_8056B23C/fn_8056B320), the component
 * constructor/adder (fn_8056B440), the pane-component drawer (fn_8056B9DC), the RTTI walker
 * fn_8056B7D8 and PaneComponent::contain (fn_8056B8AC).
 */

#include "types.h"
#include "sys_mem.h"

/* The unowned helpers this band dispatches into.  They sit in the unsplit `main` band before this
 * unit (fn_80501xxx / fn_8055xxxx), so rule 2 leaves them a counted gap (tools/units/stylelint.py,
 * "address band interleaves modules"); MEMAlloc-from / MEMFree-to are the same. */
extern "C" {
void* fn_80501C60(void* list, void* prev);      /* MEMGetNextListObject */
void  fn_80501BF4(void* list, void* node);       /* MEMList_Remove */
void  fn_80501A64(void* list, void* node);       /* MEMAppendListObject */
void* fn_80501C9C(void* list, u16 n);            /* List_GetNth */
void* fn_8055B8F0(void* o, void* a, u32 b);
s32   fn_8055F580(const char* a, const char* b);
void* MEMAllocFromAllocator(void* alloc, u32 size);
void  MEMFreeToAllocator(void* alloc, void* p);
}

/* The `.data` vtables this unit's objects point at (they live outside the claimed .text range).
 * They are the map's `lbl_` objects; referencing them keeps MWCC from emitting a table of its own. */
extern "C" {
extern char lbl_806582FC[];
extern char lbl_80658358[];
}

/* An nw4hbm::ut::List embedded in an object: a sentinel node (`next`, `prev`) followed by the
 * element count and the node offset.  size: 0xC */
struct UList {
    /* +0x00 */ void* mNext;
    /* +0x04 */ void* mPrev;
    /* +0x08 */ u16   mSize;
    /* +0x0A */ u16   mOffset;
}; /* size: 0xC */

/* The 16-byte list node this unit allocates: a key (the pane / the component's virtual result) and
 * the component.  size: 0x10 */
struct Node {
    /* +0x00 */ void* mKey;
    /* +0x04 */ void* mValue;
    /* +0x08 */ u8    pad_0x08[8];
}; /* size: 0x10 */

/* A 4-byte function-pointer slot.  The vtable layout is per class (lbl_80658290 has 27 slots,
 * lbl_80658358 has 18); every member is declared `void (*)(void)` so a call site casts to the exact
 * shape it needs without inventing a fake type. */
struct VTable {
    /* +0x00 */ void (*slot_00)(void); /* +0x04 */ void (*slot_04)(void);
    /* +0x08 */ void (*slot_08)(void); /* +0x0C */ void (*slot_0C)(void);
    /* +0x10 */ void (*slot_10)(void); /* +0x14 */ void (*slot_14)(void);
    /* +0x18 */ void (*slot_18)(void); /* +0x1C */ void (*slot_1C)(void);
    /* +0x20 */ void (*slot_20)(void); /* +0x24 */ void (*slot_24)(void);
    /* +0x28 */ void (*slot_28)(void); /* +0x2C */ void (*slot_2C)(void);
    /* +0x30 */ void (*slot_30)(void); /* +0x34 */ void (*slot_34)(void);
    /* +0x38 */ void (*slot_38)(void); /* +0x3C */ void (*slot_3C)(void);
    /* +0x40 */ void (*slot_40)(void); /* +0x44 */ void (*slot_44)(void);
    /* +0x48 */ void (*slot_48)(void); /* +0x4C */ void (*slot_4C)(void);
    /* +0x50 */ void (*slot_50)(void); /* +0x54 */ void (*slot_54)(void);
    /* +0x58 */ void (*slot_58)(void); /* +0x5C */ void (*slot_5C)(void);
    /* +0x60 */ void (*slot_60)(void); /* +0x64 */ void (*slot_64)(void);
    /* +0x68 */ void (*slot_68)(void);
}; /* size: 0x6C */

/* homebutton::gui::Component - the 0xA0-byte element/trigger record.  +0x05 is one colour byte per
 * index, +0x0D one flag byte, +0x18 a 12-byte float position, +0x80 a u16 counter; +0x94/+0x98/+0x9C
 * are the owner manager, an event target and the pane.  size: 0xA0 */
struct StageRec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
}; /* size: 0xC */

struct Component {
    /* +0x00 */ VTable* vtable;
    /* +0x04 */ void*   field_0x04;
    /* +0x08 */ u8      pad_0x08[0x0D - 0x08];
    /* +0x0D */ u8      flags_0x0D[0x18 - 0x0D];
    /* +0x18 */ StageRec recs_0x18[4];
    /* +0x48 */ u8      pad_0x48[0x78 - 0x48];
    /* +0x78 */ u32     field_0x78;      /* trigger mask */
    /* +0x7C */ u32     field_0x7C;      /* component id */
    /* +0x80 */ u16     counts_0x80[4];
    /* +0x88 */ u8      pad_0x88[0x90 - 0x88];
    /* +0x90 */ u8      field_0x90;
    /* +0x91 */ u8      pad_0x91[0x94 - 0x91];
    /* +0x94 */ struct Component* field_0x94;  /* owner manager */
    /* +0x98 */ struct Component* field_0x98;
    /* +0x9C */ void*   field_0x9C;      /* PaneComponent's pane */
}; /* size: 0xA0 */

/* The per-stage colour/counter block fn_8056A82C..fn_8056A870 view (colour byte at +5, u16 counter
 * at +0x80).  size: 0x88 (approximate). */
struct ColorBlock {
    /* +0x00 */ u8  pad_0x00[0x05];
    /* +0x05 */ u8  colors_0x05[0x80 - 0x05];
    /* +0x80 */ u16 counts_0x80[4];
}; /* size: 0x88 */

/* homebutton::gui::Manager - owns the component list at +0xC and the allocator at +8.
 * size: 0x20 (approximate - evidenced to +0x1C) */
struct Manager {
    /* +0x00 */ VTable* vtable;
    /* +0x04 */ u32     field_0x04;
    /* +0x08 */ void*   allocator;      /* MEMAllocator*; null -> operator new/delete */
    /* +0x0C */ UList   list_0x0C;
    /* +0x18 */ void*   field_0x18;
    /* +0x1C */ u32     field_0x1C;
}; /* size: 0x20 */

/* homebutton::gui::PaneManager - Manager plus the pane-keyed list at +0x24 and its id counter at +0x20.
 * size: 0x30 (approximate) */
struct PaneManager : Manager {
    /* +0x20 */ u32   field_0x20;
    /* +0x24 */ UList list_0x24;
}; /* size: 0x30 */

/* The linked node fn_8056BB7C walks: a `next` at +0xC and a visibility bit at +0xBB. */
struct VisNode {
    /* +0x00 */ u8       pad_0x00[0x0C];
    /* +0x0C */ VisNode* next_0x0C;
    /* +0x10 */ u8       pad_0x10[0xBB - 0x10];
    /* +0xBB */ u8       flags_0xBB;
}; /* size: 0xBC */

/* The intrusive node fn_8056BBD0 links (`prev` at +4, `next` at +8). */
struct CNode {
    /* +0x00 */ u32    field_0x00;
    /* +0x04 */ CNode* prev_0x04;
    /* +0x08 */ CNode* next_0x08;
}; /* size: 0xC */

/* The nw4hbm::lyt::Pane view fn_8056B6D0 reads: only its inline name at +0xBC. */
struct Pane {
    /* +0x00 */ u8   pad_0x00[0xBC];
    /* +0xBC */ char name_0xBC[0x20];
}; /* size: 0xDC */

/* The per-update event record fn_8056A638 reads: the stage index at +0, the point at +4/+8 and the
 * event mask at +0x10.  size: 0x14 */
struct StageEvent {
    /* +0x00 */ u32 idx;
    /* +0x04 */ f32 x;
    /* +0x08 */ f32 y;
    /* +0x0C */ u32 field_0x0C;
    /* +0x10 */ u32 field_0x10;
}; /* size: 0x14 */

/* --- 0x80569DAC: re-init the object at +4 with `a`, then run its virtual slot 3. --- */
extern "C" void fn_80569DAC(Manager* self, void* a) {
    fn_8055B8F0(&self->field_0x04, a, 0);
    ((void (*)(Manager*))self->vtable->slot_0C)(self);
}

/* --- 0x80569DF4: retarget the allocator's slot 0x38 and free the +0x1C block through slot 0x30. --- */
extern "C" void fn_80569DF4(Manager* self) {
    Manager* a = (Manager*)self->allocator;
    ((void (*)(Manager*, int))a->vtable->slot_38)(a, 0);
    a = (Manager*)self->allocator;
    ((void (*)(Manager*, void*))a->vtable->slot_30)(a, &self->field_0x1C);
}

/* --- 0x8056A824: constant virtual (always-true predicate). --- */
extern "C" u32 fn_8056A824(void) { return 1; }

/* --- 0x8056A82C: per-index colour-selector byte. --- */
extern "C" u8 fn_8056A82C(ColorBlock* self, u32 idx) { return self->colors_0x05[idx]; }

/* --- 0x8056A838: return the old per-index counter, then increment it. --- */
extern "C" u16 fn_8056A838(ColorBlock* self, u32 idx) {
    u16 v = self->counts_0x80[idx];
    self->counts_0x80[idx] = v + 1;
    return v;
}

/* --- 0x8056A850: store a per-index colour-selector byte. --- */
extern "C" void fn_8056A850(ColorBlock* self, u32 idx, u8 v) { self->colors_0x05[idx] = v; }

/* --- 0x8056A85C / 0x8056A870: clear a per-index counter. --- */
extern "C" void fn_8056A85C(ColorBlock* self, u32 idx) {
    u16* p = &self->counts_0x80[idx];
    *p = 0;
}
extern "C" void fn_8056A870(ColorBlock* self, u32 idx) {
    u16* p = &self->counts_0x80[idx];
    *p = 0;
}

/* --- 0x8056A884: empty virtual. --- */
extern "C" void fn_8056A884(void) {}

/* --- 0x8056A888: Manager deleting destructor - free the component list, then the manager. --- */
extern "C" Manager* fn_8056A888(Manager* self, int flag) {
    if (self != 0) {
        self->vtable = (VTable*)lbl_80658358;
        Node* n = (Node*)fn_80501C60(&self->list_0x0C, 0);
        while (n != 0) {
            fn_80501BF4(&self->list_0x0C, n);
            if (self->allocator != 0) {
                MEMFreeToAllocator(self->allocator, n);
            } else {
                operator delete(n);
            }
            n = (Node*)fn_80501C60(&self->list_0x0C, 0);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* --- 0x8056A944: run virtual slot 4 over every component. --- */
extern "C" void fn_8056A944(Manager* self) {
    Node* n = (Node*)fn_80501C60(&self->list_0x0C, 0);
    while (n != 0) {
        Component* c = (Component*)n->mValue;
        ((void (*)(Component*))c->vtable->slot_10)(c);
        n = (Node*)fn_80501C60(&self->list_0x0C, n);
    }
}

/* --- 0x8056A9B4: append `comp` to the manager's list, keyed by its virtual slot 8 result. --- */
extern "C" void fn_8056A9B4(Manager* self, Component* comp) {
    void* key = ((void* (*)(Component*))comp->vtable->slot_20)(comp);
    comp->field_0x94 = (Component*)self;
    if (self->allocator != 0) {
        Node* node = (Node*)MEMAllocFromAllocator(self->allocator, sizeof(Node));
        if (node != 0) {
            node->mKey = key;
            node->mValue = comp;
        }
        fn_80501A64(&self->list_0x0C, node);
    } else {
        Node* node = (Node*)operator new(sizeof(Node));
        if (node != 0) {
            node->mKey = key;
            node->mValue = comp;
        }
        fn_80501A64(&self->list_0x0C, node);
    }
}

/* --- 0x8056AA64: component id. --- */
extern "C" u32 fn_8056AA64(Component* self) { return self->field_0x7C; }

/* --- 0x8056AA6C / 0x8056AA98: indexed component lookup (Manager::getComponent). --- */
extern "C" Component* fn_8056AA6C(Manager* self, u32 index) {
    return (Component*)((Node*)fn_80501C9C(&self->list_0x0C, (u16)index))->mValue;
}
extern "C" Component* fn_8056AA98(Manager* self, u32 index) {
    return (Component*)((Node*)fn_80501C9C(&self->list_0x0C, (u16)index))->mValue;
}

/* --- 0x8056ADB8: forward a 3-arg event to the +4 handler and the +0x98 handler. --- */
extern "C" void fn_8056ADB8(Component* self, void* a, void* b, void* c) {
    Manager* h = (Manager*)self->field_0x04;
    if (h != 0) {
        ((void (*)(Manager*, void*))h->vtable->slot_10)(h, c);
        h = (Manager*)self->field_0x04;
        ((void (*)(Manager*, void*, void*, void*))h->vtable->slot_0C)(h, a, b, c);
    }
    h = (Manager*)self->field_0x98;
    if (h != 0) {
        ((void (*)(Manager*, void*, void*, void*))h->vtable->slot_0C)(h, a, b, c);
    }
}

/* --- 0x8056AE70: per-component flag byte. --- */
extern "C" u8 fn_8056AE70(Component* self) { return self->field_0x90; }

/* --- 0x8056AE78: latch a stage position and arm its flag when the mask matches. --- */
extern "C" void fn_8056AE78(Component* self, u32 idx, u32 mask, StageRec* v) {
    if ((mask & self->field_0x78) != 0) {
        self->recs_0x18[idx] = *v;
        self->flags_0x0D[idx] = 1;
        self->counts_0x80[idx] = 0;
    }
}

/* --- 0x8056AEC4 / 0x8056AF34: run virtual slot 5 / slot 7 over every component. --- */
extern "C" void fn_8056AEC4(Manager* self) {
    Node* n = (Node*)fn_80501C60(&self->list_0x0C, 0);
    while (n != 0) {
        Component* c = (Component*)n->mValue;
        ((void (*)(Component*))c->vtable->slot_14)(c);
        n = (Node*)fn_80501C60(&self->list_0x0C, n);
    }
}
extern "C" void fn_8056AF34(Manager* self) {
    Node* n = (Node*)fn_80501C60(&self->list_0x0C, 0);
    while (n != 0) {
        Component* c = (Component*)n->mValue;
        ((void (*)(Component*))c->vtable->slot_1C)(c);
        n = (Node*)fn_80501C60(&self->list_0x0C, n);
    }
}

/* --- 0x8056AFA4: forward an argument to virtual slot 21 of every component. --- */
extern "C" void fn_8056AFA4(Manager* self, void* arg) {
    Node* n = (Node*)fn_80501C60(&self->list_0x0C, 0);
    while (n != 0) {
        Component* c = (Component*)n->mValue;
        ((void (*)(Component*, void*))c->vtable->slot_54)(c, arg);
        n = (Node*)fn_80501C60(&self->list_0x0C, n);
    }
}

/* --- 0x8056B024: forward an argument to virtual slot 17 of every component. --- */
extern "C" void fn_8056B024(Manager* self, void* arg) {
    Node* n = (Node*)fn_80501C60(&self->list_0x0C, 0);
    while (n != 0) {
        Component* c = (Component*)n->mValue;
        ((void (*)(Component*, void*))c->vtable->slot_44)(c, arg);
        n = (Node*)fn_80501C60(&self->list_0x0C, n);
    }
}

/* --- 0x8056B0A4: store the trigger mask. --- */
extern "C" void fn_8056B0A4(Component* self, u32 v) { self->field_0x78 = v; }

/* --- 0x8056B0AC: PaneManager deleting destructor - free the pane list, then the base Manager. --- */
extern "C" PaneManager* fn_8056B0AC(PaneManager* self, int flag) {
    if (self != 0) {
        self->vtable = (VTable*)lbl_806582FC;
        Node* n = (Node*)fn_80501C60(&self->list_0x24, 0);
        while (n != 0) {
            fn_80501BF4(&self->list_0x24, n);
            if (self->allocator != 0) {
                Component* c = (Component*)n->mValue;
                ((void (*)(Component*, int))c->vtable->slot_08)(c, -1);
                MEMFreeToAllocator(self->allocator, n->mValue);
                MEMFreeToAllocator(self->allocator, n);
            } else {
                Component* c = (Component*)n->mValue;
                if (c != 0) {
                    ((void (*)(Component*, int))c->vtable->slot_08)(c, 1);
                }
                operator delete(n);
            }
            n = (Node*)fn_80501C60(&self->list_0x24, 0);
        }
        fn_8056A888(self, 0);
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* --- 0x8056B1BC / 0x8056B1FC: conditional-delete helpers. --- */
extern "C" void* fn_8056B1BC(void* self, int flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}
extern "C" void* fn_8056B1FC(void* self, int flag) {
    if (self != 0 && flag > 0) {
        operator delete(self);
    }
    return self;
}

/* --- 0x8056B41C: the component of a list node. --- */
extern "C" void* fn_8056B41C(Node* n) { return n->mValue; }

/* --- 0x8056B424: `*a != *b`. --- */
extern "C" u32 fn_8056B424(u32* a, u32* b) { return *a != *b; }

/* --- 0x8056B6C8: PaneComponent::setPane. --- */
extern "C" void fn_8056B6C8(Component* self, void* pane) { self->field_0x9C = pane; }

/* --- 0x8056B6D0: getPaneComponentByName - find the component whose pane's name matches `name`. --- */
extern "C" Component* fn_8056B6D0(PaneManager* self, const char* name) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)fn_80501C9C(&self->list_0x24, (u16)i);
        Pane* pane = (Pane*)n->mKey;
        if (fn_8055F580(pane->name_0xBC, name)) {
            return (Component*)n->mValue;
        }
    }
    return 0;
}

/* --- 0x8056B760: PaneManager::getPaneComponentByPane - find the component keyed by `pane`. --- */
extern "C" Component* fn_8056B760(PaneManager* self, void* pane) {
    for (u32 i = 0; i < self->list_0x24.mSize; i++) {
        Node* n = (Node*)fn_80501C9C(&self->list_0x24, (u16)i);
        if (n->mKey == pane) {
            return (Component*)n->mValue;
        }
    }
    return 0;
}

/* --- 0x8056B9D4: the manager's +0x18 word. --- */
extern "C" void* fn_8056B9D4(Manager* self) { return self->field_0x18; }

/* --- 0x8056BB7C: every node on the +0xC chain must carry the low visibility bit. --- */
extern "C" u32 fn_8056BB7C(Component* self) {
    VisNode* p = (VisNode*)self->field_0x9C;
    if (p == 0) {
        return 0;
    }
    while (p != 0) {
        if ((p->flags_0xBB & 1) == 0) {
            return 0;
        }
        p = p->next_0x0C;
    }
    return 1;
}

/* --- 0x8056BBC0: store the +4 word. --- */
extern "C" void fn_8056BBC0(Manager* self, u32 v) { self->field_0x04 = v; }

/* --- 0x8056BBC8: constant virtual (always-false predicate). --- */
extern "C" u32 fn_8056BBC8(void) { return 0; }

/* --- 0x8056A638: per-stage update dispatcher - returns 0/1/2/3 by which transition fired. --- */
extern "C" u32 fn_8056A638(Component* self, StageEvent* p) {
    u32 result = 0;
    u32 idx = p->idx;
    f32 x = p->x;
    f32 y = p->y;
    if (((u32 (*)(Component*, StageEvent*))self->vtable->slot_58)(self, p) != 0) {
        if (((u32 (*)(Component*, f32, f32))self->vtable->slot_64)(self, x, y) != 0) {
            if (((u32 (*)(Component*, u32))self->vtable->slot_24)(self, idx) != 0) {
                ((void (*)(Component*, u32, f32, f32))self->vtable->slot_3C)(self, idx, x, y);
                result = 3;
            } else {
                ((void (*)(Component*, u32, u32))self->vtable->slot_2C)(self, idx, 1);
                ((void (*)(Component*, u32))self->vtable->slot_30)(self, idx);
                result = 1;
            }
        } else if (((u32 (*)(Component*, u32))self->vtable->slot_24)(self, idx) != 0) {
            ((void (*)(Component*, u32, u32))self->vtable->slot_2C)(self, idx, 0);
            ((void (*)(Component*, u32))self->vtable->slot_34)(self, idx);
            result = 2;
        }
        if (self->flags_0x0D[idx] != 0) {
            StageRec* r = &self->recs_0x18[idx];
            ((void (*)(Component*, f32, f32))self->vtable->slot_38)(self, x - r->x, y - r->y);
            r->x = x;
            r->y = y;
        }
    }
    if (self->flags_0x0D[idx] != 0 && (p->field_0x10 & self->field_0x78) == 0) {
        self->flags_0x0D[idx] = 0;
    }
    return result;
}

/* --- 0x8056BBBC: tail-call forward to fn_8056A638 (stage update with the point record). --- */
extern "C" u32 fn_8056BBBC(Component* self, StageEvent* p) { return fn_8056A638(self, p); }

/* --- 0x8056BBD0: append `node` at the tail of the +8/+4 chain rooted at `head`. --- */
extern "C" void fn_8056BBD0(CNode* head, CNode* node) {
    CNode* p = head;
    while (p->next_0x08 != 0) {
        p = p->next_0x08;
    }
    p->next_0x08 = node;
    node->prev_0x04 = p;
}
